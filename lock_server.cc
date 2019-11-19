// the lock server implementation

#include "lock_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

lock_server::lock_server():
  nacquire (0),
  locks_(),
  mutex_()
{
}

lock_protocol::status
lock_server::stat(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  printf("stat request from clt %d\n", clt);
  r = nacquire;
  return ret;
}

lock_protocol::status 
lock_server::acquire(int clt,lock_protocol::lockid_t lid, int&)
{
	std::unique_lock<std::mutex> lk(mutex_);
	lockMap::iterator it = locks_.find(lid);
	if(it == locks_.end())
	{
		locks_[lid].state_ = FREE;
	}
	while (locks_[lid].state_ == LOCKED)
	{
		locks_[lid].cond_.wait(lk,[&](){ return locks_[lid].state_ == FREE; });
	}
	assert(locks_[lid].state_ == FREE);
	locks_[lid].state_ = LOCKED;
	locks_[lid].owner_ = clt;
	return lock_protocol::OK;
}

lock_protocol::status 
lock_server::release(int clt,lock_protocol::lockid_t lid, int&)
{
	std::unique_lock<std::mutex> lk(mutex_);
	lockMap::iterator it = locks_.find(lid);
	assert(it != locks_.end());
	if(locks_[lid].owner_ != clt)
		return lock_protocol::OK;
	locks_[lid].state_ = FREE;
	locks_[lid].owner_ = -1;
	assert(locks_[lid].state_ == FREE);
	locks_[lid].cond_.notify_one();
	return lock_protocol::OK;
}
