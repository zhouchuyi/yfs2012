// RPC stubs for clients to talk to lock_server

#include "lock_client.h"
#include "rpc.h"
#include <arpa/inet.h>

#include <sstream>
#include <iostream>
#include <stdio.h>

lock_client::lock_client(std::string dst)
 : cl(nullptr),
   mutex_(),
   locks_()
{
  sockaddr_in dstsock;
  make_sockaddr(dst.c_str(), &dstsock);
  cl = new rpcc(dstsock);
  if (cl->bind() < 0) {
    printf("lock_client: call bind\n");
  }
}

int
lock_client::stat(lock_protocol::lockid_t lid)
{
  int r;
  int ret = cl->call(lock_protocol::stat, cl->id(), lid, r);
  VERIFY (ret == lock_protocol::OK);
  return r;
}

lock_protocol::status
lock_client::acquire(lock_protocol::lockid_t lid)
{
  std::unique_lock<std::mutex> lk(mutex_);
  auto it = locks_.find(lid);
  if(it == locks_.end())
  {
    locks_[lid].state_ = FREE;
  }
  locks_[lid].cond_.wait(lk,[&](){ return locks_[lid].state_ == FREE; });    
  int r;
  int ret = cl->call(lock_protocol::acquire, cl->id(), lid, r);
  locks_[lid].state_ = LOCKED;
  return ret;
}

lock_protocol::status
lock_client::release(lock_protocol::lockid_t lid)
{
    std::unique_lock<std::mutex> lk(mutex_);
    int r;
    int ret = cl->call(lock_protocol::release, cl->id(), lid, r);
    locks_[lid].state_ = FREE;
    locks_[lid].cond_.notify_one();
    return ret;
}

