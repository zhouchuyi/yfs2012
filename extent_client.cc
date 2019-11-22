// RPC stubs for clients to talk to extent_server

#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

// The calls assume that the caller holds a lock on the extent

extent_client::extent_client(std::string dst)
{
  sockaddr_in dstsock;
  make_sockaddr(dst.c_str(), &dstsock);
  cl = new rpcc(dstsock);
  if (cl->bind() != 0) {
    printf("extent_client: bind failed\n");
  }
}

extent_protocol::status
extent_client::get(extent_protocol::extentid_t eid, std::string &buf)
{
  extent_protocol::status ret = extent_protocol::OK;
  ret = cl->call(extent_protocol::get, eid, buf);
  return ret;
}

extent_protocol::status
extent_client::getattr(extent_protocol::extentid_t eid, 
		       extent_protocol::attr &attr)
{
  extent_protocol::status ret = extent_protocol::OK;
  ret = cl->call(extent_protocol::getattr, eid, attr);
  return ret;
}
extent_protocol::status 
extent_client::setattr(extent_protocol::extentid_t eid,
                  extent_protocol::attr a)
{
  extent_protocol::status ret = extent_protocol::OK;
  int r;
  ret = cl->call(extent_protocol::setattr, eid, a,r);
  return ret;
}

extent_protocol::status
extent_client::put(extent_protocol::extentid_t eid, std::string buf)
{
  extent_protocol::status ret = extent_protocol::OK;
  int r;
  ret = cl->call(extent_protocol::put, eid, buf, r);
  return ret;
}

extent_protocol::status
extent_client::remove(extent_protocol::extentid_t eid)
{
  extent_protocol::status ret = extent_protocol::OK;
  int r;
  ret = cl->call(extent_protocol::remove, eid, r);
  return ret;
}

extent_protocol::status 
extent_client::read(extent_protocol::extentid_t id,size_t size,off_t off,std::string& buf)
{
  extent_protocol::status ret = extent_protocol::OK;
  ret = cl->call(extent_protocol::read,id,int(size), int(off),buf);
  return ret;
}

extent_protocol::status 
extent_client::write(extent_protocol::extentid_t id,size_t size,off_t off,std::string& buf)
{
  extent_protocol::status ret = extent_protocol::OK;
  int r;
  ret = cl->call(extent_protocol::write, id, int(size), int(off), buf,r);
  return ret;
}

extent_protocol::status 
extent_client::create(extent_protocol::extentid_t parentid,
                                extent_protocol::extentid_t id,
                                std::string name)
{
  extent_protocol::status ret;
  int r;
  ret = cl->call(extent_protocol::create,parentid, id,name,r);
  printf("received ret %d \n",ret);
  return ret;
}

extent_protocol::status 
extent_client::lookup(extent_protocol::extentid_t parent,std::string name,extent_protocol::extentid_t& child)
{
  extent_protocol::status ret;
  int r;
  ret = cl->call(extent_protocol::lookup,parent, name,child);
  return ret;
}

extent_protocol::status 
extent_client::readDir(extent_protocol::extentid_t id,
                                std::map<extent_protocol::extentid_t, 
                                std::string>& contents)
{
  extent_protocol::status ret;
  int r;
  ret = cl->call(extent_protocol::readDir,id,contents);
  return ret;
}
