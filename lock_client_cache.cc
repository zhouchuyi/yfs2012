// RPC stubs for clients to talk to lock_server, and cache the locks
// see lock_client.cache.h for protocol details.

#include "lock_client_cache.h"
#include "rpc.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <utility>
#include "tprintf.h"


int lock_client_cache::last_port = 0;

lock_client_cache::lock_client_cache(std::string xdst, 
				     class lock_release_user *_lu)
  : lock_client(xdst), lu(_lu)
{
  srand(time(NULL)^last_port);
  rlock_port = ((rand()%32000) | (0x1 << 10));
  const char *hname;
  // VERIFY(gethostname(hname, 100) == 0);
  hname = "127.0.0.1";
  std::ostringstream host;
  host << hname << ":" << rlock_port;
  id = host.str();
  last_port = rlock_port;
  rpcs *rlsrpc = new rpcs(rlock_port);
  rlsrpc->reg(rlock_protocol::revoke, this, &lock_client_cache::revoke_handler);
  rlsrpc->reg(rlock_protocol::retry, this, &lock_client_cache::retry_handler);
}

lock_protocol::status
lock_client_cache::acquire(lock_protocol::lockid_t lid)
{
  int ret = lock_protocol::OK;
  pthread_t threadId = pthread_self();
  std::unique_lock<std::mutex> lk(mutex_);
  int r = 0;
  auto it = lockInfos_.find(lid);
  if(it == lockInfos_.end() || it->second.cachestate_ == CacheState::NONE)
  {
    //insert
    lockInfos_[lid].cachestate_ = CacheState::ACQUIRING;
    lk.unlock();
    tprintf("client %s acquiring...\n",id.c_str());
    ret = cl->call(lock_protocol::acquire,lid,id,r);
    if(ret == lock_protocol::OK)
    {
      lk.lock();
      auto it = lockInfos_.find(lid);
      if(r)
        it->second.retryR_ = 1;
      goto success;
    }
retry:    //receive RETRY
    assert(ret == lock_protocol::RETRY);
    lk.lock();
    auto it = lockInfos_.find(lid);
    lockInfos_[lid].threadWaiterNum_++;
    tprintf("client %s RETRY wait \n",id.c_str());
    if(it->second.cachestate_ != CacheState::FREE)
      it->second.cond_.wait(lk,[&](){ return lockInfos_[lid].cachestate_ == CacheState::FREE; });
    lockInfos_[lid].threadWaiterNum_--;
    goto success;
  }
  else
  {
    if(it->second.cachestate_ == CacheState::LOCKED || it->second.cachestate_ == CacheState::ACQUIRING)
    {
      it->second.threadWaiterNum_++;
      tprintf("client %s acquiring/locked wait \n",id.c_str());
      it->second.cond_.wait(lk,[&](){ return lockInfos_[lid].cachestate_ == CacheState::FREE; });
      lockInfos_[lid].threadWaiterNum_--;
      goto success;
    }
    else if (it->second.cachestate_ == CacheState::FREE)
    {
      goto success;
    }
    else
    {
      assert(it->second.cachestate_ == CacheState::RELEASING);
      tprintf("client %s releasing wait \n",id.c_str());
      it->second.threadWaiterNum_++;
      it->second.cond_.wait(lk,[&](){ return lockInfos_[lid].cachestate_ != CacheState::RELEASING; });
      it->second.threadWaiterNum_--;
      lk.unlock();
      return acquire(lid);
    }
    
  } 
success: 
  tprintf("client %s got in %ld \n",id.c_str(),threadId);
  it = lockInfos_.find(lid);
  it->second.cachestate_ = CacheState::LOCKED;
  it->second.threadOwner_ = threadId;
  lk.unlock();
  return lock_protocol::OK;
}

lock_protocol::status
lock_client_cache::release(lock_protocol::lockid_t lid)
{
  int ret = lock_protocol::OK;
  int r;
  std::unique_lock<std::mutex> lk(mutex_);
  auto it = lockInfos_.find(lid);
  assert(it != lockInfos_.end());
  it->second.cachestate_ = CacheState::FREE;
  if(it->second.threadWaiterNum_ > 0 || it->second.isRevoked)
  {
    it->second.cond_.notify_one();
    return ret;
  }
  if(it->second.retryR_)
  {
    it->second.cachestate_ = CacheState::RELEASING;
    // lk.unlock();
    tprintf("client %s releasing... %lld \n",id.c_str(),lid);
    lu->dorelease(lid);
    ret = cl->call(lock_protocol::release,lid,id,r);
    assert(ret == lock_protocol::OK);
    {
      // lk.lock();
      it = lockInfos_.find(lid);
      it->second.cachestate_ = CacheState::NONE;
      it->second.retryR_ = 0;
      it->second.cond_.notify_one();
      lk.unlock();
    }
    return lock_protocol::OK;
  }
  return ret;

}

rlock_protocol::status
lock_client_cache::revoke_handler(lock_protocol::lockid_t lid, 
                                  int &)
{
  int ret = lock_protocol::OK;
  int r;
  std::unique_lock<std::mutex> lk(mutex_);
  auto it = lockInfos_.find(lid);
  assert(it != lockInfos_.end());
  it->second.isRevoked = true;
  tprintf("client %s revoke wait \n",id.c_str());
  it->second.cond_.wait(lk,[&](){ return lockInfos_[lid].threadWaiterNum_ == 0 && 
                                          lockInfos_[lid].cachestate_ == CacheState::FREE
                                          && lockInfos_[lid].threadOwner_ != 0; });    

  tprintf("client %s revoke finish %lld \n",id.c_str(),lid);
  it->second.cachestate_ = CacheState::RELEASING;
  it->second.isRevoked = false;
  it->second.retryR_ = 0;
  // lk.unlock();
  lu->dorelease(lid);
  // lk.lock();
  lockInfos_[lid].cachestate_ = CacheState::NONE;
  lockInfos_[lid].cond_.notify_one();
  return lock_protocol::OK;
}

rlock_protocol::status
lock_client_cache::retry_handler(lock_protocol::lockid_t lid,int retryR,
                                 int & r)
{
  int ret = rlock_protocol::OK;
  tprintf("client %s retry receive %lld \n",id.c_str(),lid);
  std::unique_lock<std::mutex> lk(mutex_);
  auto it = lockInfos_.find(lid);
  assert(it != lockInfos_.end());
  it->second.cachestate_ = CacheState::FREE;
  if(retryR)
  {
    it->second.retryR_ = 1;
    // tprintf("client %s set retryR %lld \n",id.c_str(),lid);
  }

  it->second.cond_.notify_one();
  return ret;
}



