// lock client interface.

#ifndef lock_client_cache_h

#define lock_client_cache_h

#include <string>
#include "lock_protocol.h"
#include "rpc.h"
#include "lock_client.h"
#include "lang/verify.h"

// Classes that inherit lock_release_user can override dorelease so that 
// that they will be called when lock_client releases a lock.
// You will not need to do anything with this class until Lab 5.
class lock_release_user {
 public:
  virtual void dorelease(lock_protocol::lockid_t) = 0;
  virtual ~lock_release_user() {};
};

class lock_client_cache : public lock_client {
 private:
  class lock_release_user *lu;
  int rlock_port;
  std::string hostname;
  std::string id;
  enum class CacheState 
                  { 
                    NONE,
                    FREE,
                    LOCKED,
                    ACQUIRING,
                    RELEASING 
                  };
  class CacheInfo
  {
  public:
    CacheInfo()
     : threadWaiterNum_(0),
       threadOwner_(0),
       cachestate_(CacheState::NONE),
       cond_(),
       isRevoked(false)
    {

    }
    CacheInfo(CacheInfo&) = delete;
    CacheInfo& operator=(const CacheInfo&) = delete;
    uint32_t threadWaiterNum_{0};
    pthread_t threadOwner_{0};
    CacheState cachestate_{CacheState::NONE};
    std::condition_variable cond_{};
    bool isRevoked{false};
    int retryR_{0};
  };

  std::map<lock_protocol::lockid_t,
       lock_client_cache::CacheInfo> lockInfos_;

 public:
  static int last_port;
  lock_client_cache(std::string xdst, class lock_release_user *l = 0);
  virtual ~lock_client_cache() {};
  lock_protocol::status acquire(lock_protocol::lockid_t) override;
  lock_protocol::status release(lock_protocol::lockid_t) override;
  rlock_protocol::status revoke_handler(lock_protocol::lockid_t,
                                        int &);
  rlock_protocol::status retry_handler(lock_protocol::lockid_t, int retryR,
                                       int &);
};


#endif
