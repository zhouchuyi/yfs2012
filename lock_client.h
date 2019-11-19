// lock client interface.

#ifndef lock_client_h
#define lock_client_h

#include <string>
#include "lock_protocol.h"
#include "rpc.h"
#include <vector>
#include <map>
#include <condition_variable>
#include <mutex>
// Client interface to the lock server
class lock_client {
 protected:
  enum LOCKSTATE { FREE , LOCKED };
  struct lock
  {
    LOCKSTATE state_;
    std::condition_variable cond_{};
  };
  
  rpcc *cl;
  std::mutex mutex_;
  std::map<lock_protocol::lockid_t,lock> locks_;
 public:
  lock_client(std::string d);
  virtual ~lock_client() {};
  virtual lock_protocol::status acquire(lock_protocol::lockid_t);
  virtual lock_protocol::status release(lock_protocol::lockid_t);
  virtual lock_protocol::status stat(lock_protocol::lockid_t);
};


#endif 
