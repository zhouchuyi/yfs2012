// this is the extent server

#ifndef extent_server_h
#define extent_server_h

#include <string>
#include <map>
#include <mutex>
#include <sstream>
#include "extent_protocol.h"

static constexpr uint32_t kDefaultRootDir = 0x00000001; 

class extent_server {
 private:
 std::mutex mutex_;
 std::map<extent_protocol::extentid_t, std::string> keyVal_;
 std::map<extent_protocol::extentid_t,extent_protocol::attr>
    attrs_;
   //inum/name pair
 std::map<extent_protocol::extentid_t,std::string> parseDir(std::string content)
 {
   std::map<extent_protocol::extentid_t,std::string> dirSet;
   std::istringstream strm(content);
   unsigned long long inum;
   while (strm >> inum)
   {
      strm >> dirSet[inum];
   }
   return dirSet;
}
 void createRoot()
 {
   int r;
   put(kDefaultRootDir,std::string{},r);
 }
 int putunLocked(extent_protocol::extentid_t id, std::string);
 public:
  extent_server();
  
  int put(extent_protocol::extentid_t id, std::string, int &);
  int get(extent_protocol::extentid_t id, std::string &);
  int getattr(extent_protocol::extentid_t id, extent_protocol::attr &);
  int setattr(extent_protocol::extentid_t id, extent_protocol::attr, int&);
  int remove(extent_protocol::extentid_t id, int &);

  int write(extent_protocol::extentid_t id,int size,int off ,std::string buf, int&);

  int read(extent_protocol::extentid_t id,int size,int off,std::string& buf);

  int readDir(extent_protocol::extentid_t id,
               std::map<extent_protocol::extentid_t, 
               std::string>& contents); 

  int create(extent_protocol::extentid_t parentid,
                                extent_protocol::extentid_t id,
                                std::string name,int&);
   
  int lookup(extent_protocol::extentid_t parent,std::string name,extent_protocol::extentid_t& child);


};

#endif 







