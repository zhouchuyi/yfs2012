// this is the lock server
// the lock client has a similar interface

#ifndef lock_server_h
#define lock_server_h

#include <string>
#include <map>
#include <mutex>
#include <condition_variable>
#include "lock_protocol.h"
#include "lock_client.h"
#include "rpc.h"

class lock_server {

 protected:
  int nacquire;
  enum LOCKSTATE { FREE, LOCKED };
  struct lock
  {
    LOCKSTATE state_{FREE};
    int owner_{-1};
    std::condition_variable cond_{};
  };
  
  typedef std::map<lock_protocol::lockid_t, lock > lockMap;
  lockMap locks_;
  std::mutex mutex_;

 public:
  lock_server();
  ~lock_server() {};
  lock_protocol::status stat(int clt, lock_protocol::lockid_t lid, int &);
  lock_protocol::status acquire(int clt,lock_protocol::lockid_t lid, int&);
  lock_protocol::status release(int clt,lock_protocol::lockid_t lid, int&);
};

#endif 







