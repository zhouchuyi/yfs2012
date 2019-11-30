// the caching lock server implementation

#include "lock_server_cache.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <algorithm>
#include "lang/verify.h"
#include "handle.h"
#include "tprintf.h"


lock_server_cache::lock_server_cache()
 : nacquire(0), 
   states_()
{

}

int lock_server_cache::acquire(lock_protocol::lockid_t lid, std::string id, 
                               int& retryR)
{
  retryR = 0;
  lock_protocol::status ret = lock_protocol::OK;
  mutex_.lock();
  auto it = states_.find(lid);
  if(it == states_.end() || it->second.isHold_ == false)
  {
    //insert
    states_[lid].isHold_ = true;
    states_[lid].ownerID_ = id;
    tprintf("%s insert and acquire %lld \n",id.c_str(),lid);
    mutex_.unlock();
    return lock_protocol::OK;
  }
  //lock exits and must be hold by a clients
  assert(it->second.isHold_);
  if (!it->second.waitingClients_.empty())
  {
    ret = lock_protocol::RETRY;
    for (auto &waiter : it->second.waitingClients_)
    {
      if(waiter == id)
        assert(false);
    }
    
    it->second.waitingClients_.push_back(id);
    tprintf("%s push and send RETRY %lld \n",id.c_str(),lid);
    mutex_.unlock();
    return ret;
  }
  //waitinglist is empty
  std::string ownerID = it->second.ownerID_;
  tprintf("%s first push %lld \n",id.c_str(),lid);
  it->second.waitingClients_.push_back(id);
  mutex_.unlock();

  //send revok
  handle ownerHandle(ownerID);
  handle retryHandle(id);
  rpcc* owner = ownerHandle.safebind();
  rpcc* retry = retryHandle.safebind();
  if(owner != nullptr && retry != nullptr)
  {
    int r = 0;
    tprintf("%s begin send revoke %lld \n",ownerID.c_str(),lid);
    ret = owner->call(rlock_protocol::revoke,lid,r);
    tprintf("%s revoke finish %lld \n",ownerID.c_str(),lid);
    int r_;
    //check is empty?
    {
      mutex_.lock();
      it = states_.find(lid);
      it->second.waitingClients_.pop_front();
      if(!it->second.waitingClients_.empty())
        retryR = 1;
      it->second.ownerID_ = id;
      mutex_.unlock();
    }
    return lock_protocol::OK;
  }
  else
  {
    assert(false);
  }
  
}

int 
lock_server_cache::release(lock_protocol::lockid_t lid, std::string id, 
         int &r)
{
  tprintf("%s begin release %lld \n",id.c_str(),lid);
  lock_protocol::status ret = lock_protocol::OK;
  int retryR = 0;
  std::unique_lock<std::mutex> lk(mutex_);
  auto it = states_.find(lid);
  assert(it != states_.end());
  assert(it->second.isHold_);
  assert(!it->second.waitingClients_.empty());
  tprintf("%s release %lld \n",id.c_str(),lid);
  std::string ownerID = it->second.waitingClients_.front();
  it->second.ownerID_ = ownerID;
  it->second.waitingClients_.pop_front();
  if(!it->second.waitingClients_.empty())
    retryR = 1;
  lk.unlock();
  handle retryHandle(ownerID);
  rpcc* retry = retryHandle.safebind();
  int r_;
  ret = retry->call(rlock_protocol::retry,lid,retryR,r_);
  tprintf("%s send retry down %lld \n",ownerID.c_str(),lid);
  //pop_fron set after the rpc_retry to prevent during the call a new require arrive
  //and the waitSet is empty 
  // {
  //   lk.lock();
  //   it = states_.find(lid);
  //   it->second.waitingClients_.pop_front();
  // }
  
  return lock_protocol::OK;

}

lock_protocol::status
lock_server_cache::stat(lock_protocol::lockid_t lid, int &r)
{
  tprintf("stat request\n");
  r = nacquire;
  return lock_protocol::OK;
}

