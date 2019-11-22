// yfs client.  implements FS operations using extent and lock server
#include "yfs_client.h"
#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


yfs_client::yfs_client(std::string extent_dst, std::string lock_dst)
{
  ec = new extent_client(extent_dst);
  
}

yfs_client::inum
yfs_client::n2i(std::string n)
{
  std::istringstream ist(n);
  unsigned long long finum;
  ist >> finum;
  return finum;
}

std::string
yfs_client::filename(inum inum)
{
  std::ostringstream ost;
  ost << inum;
  return ost.str();
}

bool
yfs_client::isfile(inum inum)
{
  if(inum & 0x80000000)
    return true;
  return false;
}

bool
yfs_client::isdir(inum inum)
{
  return ! isfile(inum);
}

int
yfs_client::getfile(inum inum, fileinfo &fin)
{
  int r = OK;

  printf("getfile %016llx\n", inum);
  extent_protocol::attr a;
  if (ec->getattr(inum, a) != extent_protocol::OK) {
    r = IOERR;
    goto release;
  }

  fin.atime = a.atime;
  fin.mtime = a.mtime;
  fin.ctime = a.ctime;
  fin.size = a.size;
  printf("getfile %016llx -> sz %llu\n", inum, fin.size);

 release:

  return r;
}

int
yfs_client::getdir(inum inum, dirinfo& din)
{
  int r = OK;

  printf("getdir %016llx\n", inum);
  extent_protocol::attr a;
  if (ec->getattr(inum, a) != extent_protocol::OK) {
    r = IOERR;
    goto release;
  }
  din.atime = a.atime;
  din.mtime = a.mtime;
  din.ctime = a.ctime;

 release:
  return r;
}

int
yfs_client::setfile(inum inum, fileinfo& fin)
{
  int r = OK;
  printf("setfile %016llx\n", inum);
  extent_protocol::attr a;
  a.atime = fin.atime;
  a.mtime = fin.mtime;
  a.ctime = fin.ctime;
  a.size = fin.size;
  if(ec->setattr(inum,a) != extent_protocol::OK)
  {  
    r = IOERR;
  }

  return r;
}

int 
yfs_client::setdir(inum inum, dirinfo& din)
{
  int r = OK;
  printf("setfile %016llx\n", inum);
  extent_protocol::attr a;
  a.atime = din.atime;
  a.mtime = din.mtime;
  a.ctime = din.ctime;
  if(ec->setattr(inum,a) != extent_protocol::OK)
  {  
    r = IOERR;
  }

  return r;
}

int 
yfs_client::readfile(inum i, size_t size, off_t off, std::string& buf)
{
  int r = OK;
  printf("readfile %016llx\n", i);
  if(ec->read(i,size,off,buf) != extent_protocol::OK)
  {
    r =  IOERR;
  }
  return r;
}

int 
yfs_client::writefile(inum i, size_t size, off_t off, std::string& buf)
{
  int r = OK;
  printf("writefile %016llx\n", i);
  if(ec->write(i, size, off, buf) != extent_protocol::OK)
  {
    r =  IOERR;
  }
  return r;
}

int 
yfs_client::create(inum parent, inum child, std::string name)
{
  int r = OK;
  printf("createfile %s\n", name.c_str());
  if(ec->create(parent, child,name) != extent_protocol::OK)
  {
    r = EXIST;
  }
  return r;
}

int 
yfs_client::lookup(inum parent, std::string name,inum& child)
{
  int r = OK;
  printf("lookupfile %s\n", name.c_str());
  extent_protocol::extentid_t childReturn = child;
  if(ec->lookup(parent, name, childReturn) != extent_protocol::OK)
  {
    //don't find
    r = IOERR;
  }

  child = childReturn;
  return r;
}

int 
yfs_client::readDir(inum i,std::map<extent_protocol::extentid_t, 
                                std::string>& contents)
{
  int r = OK;  
  printf("writefile %016llx\n", i);
  if(ec->readDir(i, contents) != extent_protocol::OK)
    return IOERR;//dir dosen't exits
  return r;
}
