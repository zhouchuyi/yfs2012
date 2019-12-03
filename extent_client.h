// extent client interface.

#ifndef extent_client_h
#define extent_client_h

#include <mutex>
#include <string>
#include "extent_protocol.h"
#include "rpc.h"

class extent_client {
 private:
  rpcc *cl;
  enum cacheState
  {
    NONE,
    REMOVED,
    DIRTY,
    UPDATED,
  };
  struct extent_cache
  {
    extent_cache(cacheState state,std::string data,extent_protocol::attr attr)
     : state_(state),
       data_(data),
       attr_(attr)
    {

    }
    extent_cache() = default;
    extent_cache(extent_cache&&) = default;
    cacheState state_{cacheState::NONE};
    std::string data_;
    extent_protocol::attr attr_;
  };
  std::map<extent_protocol::extentid_t,extent_cache> extent_caches;
  std::mutex mutex_;
 public:
  extent_client(std::string dst);
  void flush(extent_protocol::extentid_t eid);
  extent_protocol::status get(extent_protocol::extentid_t eid, 
			      std::string &buf);
  extent_protocol::status getattr(extent_protocol::extentid_t eid, 
				  extent_protocol::attr &a);
  extent_protocol::status put(extent_protocol::extentid_t eid, std::string buf);
  extent_protocol::status remove(extent_protocol::extentid_t eid);
};

#endif 

