// the extent server implementation

#include "extent_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

extent_server::extent_server()
 : mutex_(),
   keyVal_(),
   attrs_()
{
  createRoot();
}

int 
extent_server::putunLocked(extent_protocol::extentid_t id, std::string buf)
{
  printf("put id %lld \n",id);
  auto it = keyVal_.find(id);
  if(it != keyVal_.end())
  {
    printf("not unique id");
    return extent_protocol::IOERR;
  }
  keyVal_[id] = std::move(buf);
  extent_protocol::attr attr_;
  time_t now = time(nullptr);
  attr_.mtime = attr_.ctime = now;
  attr_.size = buf.size();
  auto it_ = keyVal_.find(id);
  attrs_[id] = std::move(attr_);
  return extent_protocol::OK;
  
}


int extent_server::put(extent_protocol::extentid_t id, std::string buf, int &)
{
  std::lock_guard<std::mutex> lk(mutex_);
  return putunLocked(id,std::move(buf));
  // You fill this in for Lab 2.
}

int extent_server::get(extent_protocol::extentid_t id, std::string &buf)
{
  // You fill this in for Lab 2.
  std::lock_guard<std::mutex> lk(mutex_);
  auto it = keyVal_.find(id);
  if(it == keyVal_.end() || attrs_.find(id) == attrs_.end())
  {
    printf("could not get %lld \n",id);
    return extent_protocol::IOERR;
  }
  time_t now = time(nullptr);
  attrs_[id].mtime = now;
  buf = keyVal_[id];
  return extent_protocol::OK;
}

int extent_server::getattr(extent_protocol::extentid_t id, extent_protocol::attr& a)
{
  // You fill this in for Lab 2.
  // You replace this with a real implementation. We send a phony response
  // for now because it's difficult to get FUSE to do anything (including
  // unmount) if getattr fails.
  std::lock_guard<std::mutex> lk(mutex_);
  auto it = keyVal_.find(id);
  if(it == keyVal_.end() || attrs_.find(id) == attrs_.end())
    return extent_protocol::IOERR;
  a = attrs_[id];
  // a.size = attr.size;
  // a.atime = attr.atime;
  // a.mtime = attr.mtime;
  // a.ctime = attr.ctime;
  return extent_protocol::OK;
}

int 
extent_server::setattr(extent_protocol::extentid_t id, extent_protocol::attr attr, int&)
{
  std::lock_guard<std::mutex> lk(mutex_);
  printf("in set attr %lld %d \n",id,attr.size);
  auto it = keyVal_.find(id);
  if(it == keyVal_.end() || attrs_.find(id) == attrs_.end())
    return extent_protocol::IOERR;
  std::string& content = it->second;
  if(attr.size > attrs_[id].size)
  {
    content.append(attr.size > attrs_[id].size,'\0');
  }
  attrs_[id].size = attr.size;
  keyVal_[id].resize(attr.size);
  // attrs_[id].size = attr.size;
  return extent_protocol::OK;
}

int extent_server::remove(extent_protocol::extentid_t id, int &)
{
  // You fill this in for Lab 2.
  printf("in remove\n");
  std::lock_guard<std::mutex> lk(mutex_);
  auto it = keyVal_.find(id);
  if(it == keyVal_.end() || attrs_.find(id) == attrs_.end())
    return extent_protocol::IOERR;
  assert(attrs_.erase(id) == 1);
  assert(keyVal_.erase(id) == 1);
  return extent_protocol::OK;
}

int 
extent_server::write(extent_protocol::extentid_t id,int size,int off ,std::string buf,int&)
{
  std::lock_guard<std::mutex> lk(mutex_);
  // printf("write to %lld %d \n",id,size);
  auto it = keyVal_.find(id);
  if(it == keyVal_.end() || attrs_.find(id) == attrs_.end())
  {
    printf("counld not find father\n");
    return extent_protocol::IOERR;
  }  
  extent_protocol::attr& attr = attrs_[id];
  std::string& content = it->second;
  // assert(buf.size() == size);
  // size = buf.size();
  char* start;
  printf("write to %lld size = %d off = %d\n",id,size,off);
  if(attr.size > off + size)
  {
    // start = const_cast<char*>(content.data());
    // start += off;
    // memcpy(start + off,buf.data(),size);
    // printf("write ok\n");
    for (size_t i = off; i < off + size; i++)
    {
      content[i] = buf[i - off];
    }
    
    return extent_protocol::OK;
    // attr.size = off + size;
  }
  else if (attr.size < off)
  {
    // content.reserve(off + size + 1);
    //to end
    // start = const_cast<char*>(content.data());
    // start += attr.size;
    
    // memset(start,'\0',off - attr.size);
    // start += off - attr.size;
    // memcpy(start,buf.c_str(),size);
    // attr.size = off + size;
    // content[attr.size] = '\0';
    // content.resize(attr.size);
    // printf("write ok\n");
    // return extent_protocol::OK;
    content.append(off - attr.size,'\0');
    content.append(buf.substr(0,size));
    // attr.size = off + size;
    attr.size = content.size();
    return extent_protocol::OK;

  }
  else if (attr.size == off)
  {
    // attr.size += size;
    content.append(buf.substr(0,size));
    attr.size = content.size();
    return extent_protocol::OK;
  }
  else 
  {
    // content.reserve(off + size + 1);
    // start = const_cast<char*>(content.data());
    // memcpy(start + off,buf.c_str(),size);
    // attr.size = off + size - (attr.size  - off);    
    // content[attr.size] = '\0';
    // content.resize(attr.size);
    // printf("write ok %s\n",content.data());
    content = content.substr(0,off);
    content.append(buf);
    // attr.size = off + size - (attr.size  - off);
    attr.size = content.size();
    return extent_protocol::OK;
  }
  return extent_protocol::IOERR;
}

int 
extent_server::read(extent_protocol::extentid_t id,int size,int off,std::string& buf)
{
  std::lock_guard<std::mutex> lk(mutex_);
  auto it = keyVal_.find(id);
  if(it == keyVal_.end() || attrs_.find(id) == attrs_.end())
    return extent_protocol::IOERR;
  extent_protocol::attr& attr = attrs_[id];
  std::string& content = it->second;
  printf("read form %ld size %d off %d content_size %ld \n",id,size,off,attr.size);
  buf.resize(0);
  if(off > attr.size)
  {
    return extent_protocol::IOERR;
  }
  else if (off == attr.size)
  {
    buf = {};
    return extent_protocol::OK;
  }
  else if (off + size > attr.size)
  {
    // printf("%s\n",content.data());
    buf = content.substr(off);
    // printf("over read %s\n",buf.data());
    return extent_protocol::OK;
  }
  else
  {
    // printf("normal read\n");
    buf = content.substr(off, size);
    return extent_protocol::OK;
  }
  
  return extent_protocol::IOERR;
  
}

//return IOERROR if exit
//else inster OK
int 
extent_server::create(extent_protocol::extentid_t parentid,
                              extent_protocol::extentid_t id,
                              std::string name,int&)
{
  std::lock_guard<std::mutex> lk(mutex_);
  printf("in extent_serever create %s %lld\n",name.c_str(),parentid);
  auto it = keyVal_.find(parentid);
  auto attrsIt = attrs_.find(parentid);
  if(it == keyVal_.end() || attrsIt == attrs_.end())
  {
    printf("could not find father %lld \n",parentid);
    return extent_protocol::IOERR;
  }
  std::string& content = it->second;
  auto dirSet = parseDir(content);
  for (auto & pair : dirSet)
  {
    if(pair.second == name)
    {
      printf("name exits\n");
      return extent_protocol::IOERR;
    }
  }
  int r;
  printf("begin to put \n");
  if(putunLocked(id,std::string{}) != extent_protocol::OK)
      return extent_protocol::IOERR;
  content = content + std::to_string(id) + '\n';
  content = content + name + '\n';
  printf("return \n");
  return extent_protocol::OK;
}
  
int 
extent_server::lookup(extent_protocol::extentid_t parent,std::string name,extent_protocol::extentid_t& child)
{
  std::lock_guard<std::mutex> lk(mutex_);
  auto it = keyVal_.find(parent);
  if(it == keyVal_.end() || attrs_.find(parent) == attrs_.end())
  {
    printf("lookup could not find parent \n");
    return extent_protocol::IOERR;
  }
  std::string& content = it->second;
  auto dirSet = parseDir(content);
  for (auto & pair : dirSet)
  {
    if(pair.second == name)
    {
      child = pair.first;
      return extent_protocol::OK;
    }
  }
  printf("lookup could not find file %s\n",name.c_str());
  return extent_protocol::IOERR;
}

int 
extent_server::readDir(extent_protocol::extentid_t id,
                      std::map<extent_protocol::extentid_t, std::string>& contents)
{
  std::lock_guard<std::mutex> lk(mutex_);
  auto it = keyVal_.find(id);
  if(it == keyVal_.end() || attrs_.find(id) == attrs_.end())
    return extent_protocol::IOERR;
  contents = parseDir(it->second);
  return extent_protocol::OK;
}


