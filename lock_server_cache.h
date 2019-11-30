#ifndef lock_server_cache_h
#define lock_server_cache_h

#include <string>

#include <map>
#include <set>
#include "lock_protocol.h"
#include "rpc.h"
#include "lock_server.h"

class lock_server_cache {
 private:

  int nacquire;

  struct lockState
  {
    using waitingSet = std::list<std::string>;
    std::string ownerID_{};
    bool isHold_{false};
    waitingSet waitingClients_{}; 
    std::condition_variable cond_{};
  };

  std::mutex mutex_;//protect states_
  std::map<lock_protocol::lockid_t, lockState> states_;
 
 public:
  lock_server_cache();
  lock_protocol::status stat(lock_protocol::lockid_t, int &);
  int acquire(lock_protocol::lockid_t, std::string id, int &);
  int release(lock_protocol::lockid_t, std::string id, int &);

};

#endif
