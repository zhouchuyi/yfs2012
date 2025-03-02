// yfs client.  implements FS operations using extent and lock server
#include "yfs_client.h"
#include "extent_client.h"
#include "lock_client.h"
#include "lock_client_cache.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

static lock_protocol::lockid_t lockId = 0;

yfs_client::yfs_client(std::string extent_dst, std::string lock_dst)
 : locks()
{
  printf("bind %s",lock_dst.c_str());
  ec = new extent_client(extent_dst);
  lc = new lock_client_cache(lock_dst,new lock_release_client(ec));
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
yfs_client::getfile(inum inum, fileinfo& fin)
{
  int r = OK;
  // You modify this function for Lab 3
  // - hold and release the file lock

  printf("getfile %016llx\n", inum);
  extent_protocol::attr a;
  lc->acquire(inum);
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
  lc->release(inum);

  return r;
}

int
yfs_client::getdir(inum inum, dirinfo &din)
{
  int r = OK;
  // You modify this function for Lab 3
  // - hold and release the directory lock
  
  printf("getdir %016llx\n", inum);
  extent_protocol::attr a;
  lc->acquire(inum);
  if (ec->getattr(inum, a) != extent_protocol::OK) {
    r = IOERR;
    goto release;
  }
  din.atime = a.atime;
  din.mtime = a.mtime;
  din.ctime = a.ctime;

 release:
  lc->release(inum);
  return r;
}

int 
yfs_client::setattr(inum inum, struct stat *attr)
{
	int r = OK;
	size_t size = attr->st_size;
	std::string buf;
	lc->acquire(inum);
	if (ec->get(inum, buf) != extent_protocol::OK) {
		r = IOERR;
		goto release;
	}
	buf.resize(size, '\0');

	if (ec->put(inum, buf) != extent_protocol::OK) {
		r = IOERR;
	}
release:
	lc->release(inum);
	return r;
}

int 
yfs_client::read(inum inum, off_t off, size_t size, std::string &buf)
{	
	int r = OK;
	std::string file_data;
	size_t read_size;
	lc->acquire(inum);
	if (ec->get(inum, file_data) != extent_protocol::OK) {
		r = IOERR;
		goto release;
	}
	if (off >= file_data.size())
		buf = std::string();
	read_size = size;
	if(off + size > file_data.size()) 
		read_size = file_data.size() - off;
	buf = file_data.substr(off, read_size);

release:
	lc->release(inum);
	return r;	
}

int 
yfs_client::write(inum inum, off_t off, size_t size, const char *buf)
{
	int r = OK;
	std::string file_data;
	lc->acquire(inum);
	if (ec->get(inum, file_data) != extent_protocol::OK) {
		r = IOERR;
		goto release;
	}
	if (size + off > file_data.size())
		file_data.resize(size + off, '\0');
	for(int i = 0; i < size; i++)
		file_data[off + i] = buf[i];
	if (ec->put(inum, file_data) != extent_protocol::OK) {
		r = IOERR;
		goto release;
	}
release:
	lc->release(inum);
	return r;
}

yfs_client::inum
yfs_client::random_inum(bool isfile)
{
	inum ret = (unsigned long long)
		((rand() & 0x7fffffff) | (isfile << 31));
	ret = 0xffffffff & ret;
	return ret;
}
int
yfs_client::create(inum parent, const char *name, inum &inum)
{
	int r = OK;
	std::string dir_data;
	std::string file_name;
	lc->acquire(parent);
	if (ec->get(parent, dir_data) != extent_protocol::OK) {
		r = IOERR;
		goto release;
	}	
	file_name = "/" + std::string(name) + "/";
	if (dir_data.find(file_name) != std::string::npos) {
		r = EXIST;
		goto release;
	}

	inum = random_inum(true);
	lc->acquire(inum);
	if (ec->put(inum, std::string()) != extent_protocol::OK) {
		r = IOERR;
		goto release;
	}
	lc->release(inum);

	dir_data.append(file_name + filename(inum) + "/");
	if (ec->put(parent, dir_data) != extent_protocol::OK) {
		r = IOERR;
		goto release;
	}
release:
	lc->release(parent);
	return r;
}

int
yfs_client::lookup(inum parent, const char *name, inum &inum, bool *found) 
{
	int r = OK;
	size_t pos, end;
	std::string dir_data;
	std::string file_name;
	std::string ino;
	lc->acquire(parent);
	if (ec->get(parent, dir_data) != extent_protocol::OK) {
		r = IOERR;
		goto release;
	}
	file_name = "/" + std::string(name) + "/";
	pos = dir_data.find(file_name);
	if (pos	!= std::string::npos) {
		*found = true;
		pos += file_name.size();
		end = dir_data.find_first_of("/", pos);
		if(end != std::string::npos) {
			ino = dir_data.substr(pos, end-pos);
			inum = n2i(ino.c_str());		
		} else {
			r = IOERR;
			goto release;
		}
	} else {
		r = IOERR;
		goto release;
	}
release:
	lc->release(parent);
	return r;
}

int 
yfs_client::readdir(inum inum, std::list<dirent>& dirents) 
{
	// if(locks.find(inum) == locks.end())
	// 	return IOERR;
	lc->acquire(inum);
	int r = readdirunlocked(inum,dirents);
	lc->release(inum);
	return r;
}

int 
yfs_client::readdirunlocked(inum inum, std::list<dirent> &dirents)
{
	int r = OK;
	std::string dir_data;
	std::string inum_str;
	size_t pos, name_end, name_len, inum_end, inum_len;

	if (ec->get(inum, dir_data) != extent_protocol::OK) {
		r = IOERR;
		goto release;
	}	
	pos = 0;
	while(pos != dir_data.size()) {
		dirent entry;
		pos = dir_data.find("/", pos);
		if(pos == std::string::npos)
			break;
		name_end = dir_data.find_first_of("/", pos + 1);
		name_len = name_end - pos - 1;
		entry.name = dir_data.substr(pos + 1, name_len);
		
		inum_end = dir_data.find_first_of("/", name_end + 1);
		inum_len = inum_end - name_end - 1;
		inum_str = dir_data.substr(name_end + 1, inum_len);
		entry.inum = n2i(inum_str.c_str());	
		dirents.push_back(entry);
		pos = inum_end + 1;
	}
release:
	
	return r;	
}


int 
yfs_client::mkdir(inum parent,const char* name,inum& id)
{
	int r = OK;
	std::string dir_data;
	std::string dir_name;
	printf("makdir begin get lock\n");
	lc->acquire(parent);
	if(ec->get(parent,dir_data) != extent_protocol::OK)
	{
		r = IOERR;
		goto release;
	}
	dir_name = "/" + std::string(name) + '/';
	if(dir_data.find(dir_name) != std::string::npos)
	{
		r = EXIST;
		goto release;
	}
	id = random_inum(false);
	// lc->acquire(id);
	if(ec->put(id,std::string{}) != extent_protocol::OK)
	{
		r = IOERR;
		goto release;
	}
	// lc->release(id);
	dir_data.append(dir_name + filename(id) + '/');
	if(ec->put(parent,dir_data) != extent_protocol::OK)
	{
		r = IOERR;
	}
release:
	printf("makdir begin release lock\n");
	lc->release(parent);
	return r;

}

int 
yfs_client::unlink(inum parent,const char* name)
{
	int r = OK;
	inum id = 0;
	std::list<dirent> entrys;
	std::string fileName(name);
	std::string dir_data;
	lc->acquire(parent);
	if(readdirunlocked(parent,entrys) != extent_protocol::OK)
	{
		r = IOERR;
		goto release;
	}
	for (auto it = entrys.begin(); it != entrys.end(); it++)
	{
		if(it->name == fileName)
		{
			id = it->inum;
			entrys.erase(it);
			break;
		}
	}
	if(id == 0)
	{
		//don't find
		r = IOERR;
		goto release;
	}

	for (auto & entry : entrys)
	{
		std::string file_name = "/" + std::string(entry.name) + "/";
		dir_data.append(file_name + filename(entry.inum) + "/");	
	}
	// lc->acquire(id);
	ec->remove(id);
	// lc->release(id);
	if(ec->put(parent,dir_data) != extent_protocol::OK)
	{
		r = IOERR;
		goto release;
	}

release:	
	lc->release(parent);
	return r;
}
