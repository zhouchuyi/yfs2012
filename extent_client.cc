// RPC stubs for clients to talk to extent_server

#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

// The calls assume that the caller holds a lock on the extent

void 
extent_client::flush(extent_protocol::extentid_t eid)
{
  std::unique_lock<std::mutex> lk(mutex_);
  extent_protocol::status ret = extent_protocol::OK;
	int r;
	bool flag = extent_caches.count(eid);
	if (flag) {	
		switch(extent_caches[eid].state_) {
			case DIRTY:
				ret = cl->call(extent_protocol::put, eid, extent_caches[eid].data_, r);
				break;
			case REMOVED:
				ret = cl->call(extent_protocol::remove, eid);
				break;
			case NONE:
			case UPDATED:
			default:
				break;
		}
		extent_caches.erase(eid);	
	} else {
		ret = extent_protocol::NOENT;
	}
	
}


extent_client::extent_client(std::string dst)
 : extent_caches(),
   mutex_()
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
  std::unique_lock<std::mutex> lk(mutex_);
	bool flag = extent_caches.count(eid);
	if (flag) {
		switch (extent_caches[eid].state_) {
			case UPDATED:
			case DIRTY:
				buf = extent_caches[eid].data_;
				extent_caches[eid].attr_.atime = time(NULL);
				break;
			case NONE:
				ret = cl->call(extent_protocol::get, eid, buf);
				if (ret == extent_protocol::OK) {
					extent_caches[eid].data_ = buf;
					extent_caches[eid].state_ = UPDATED;
					extent_caches[eid].attr_.atime = time(NULL);
					extent_caches[eid].attr_.size = buf.size();
				}
				break;
			case REMOVED:
			default:
				ret = extent_protocol::NOENT;
				break;
		}
	} else {
		ret = cl->call(extent_protocol::get, eid, buf);
		if (ret == extent_protocol::OK) {	
			extent_caches[eid].data_ = buf;
			extent_caches[eid].state_ = UPDATED;
			extent_caches[eid].attr_.atime = time(NULL);
			extent_caches[eid].attr_.size = buf.size();
			extent_caches[eid].attr_.ctime = 0;
			extent_caches[eid].attr_.mtime = 0;
		}
	}
	buf = extent_caches[eid].data_;
	return ret;
  
  
}

extent_protocol::status
extent_client::getattr(extent_protocol::extentid_t eid, 
		       extent_protocol::attr &attr)
{
  extent_protocol::status ret = extent_protocol::OK;
	extent_protocol::attr *a, temp;
	std::unique_lock<std::mutex> lk(mutex_);
	bool flag = extent_caches.count(eid);
	if (flag) {
		switch (extent_caches[eid].state_) {
			case UPDATED:
			case DIRTY:
			case NONE:
				a = &extent_caches[eid].attr_;
				if (!a->atime || !a->ctime || !a->mtime) {
					ret = cl->call(extent_protocol::getattr, eid, temp);
					if (ret == extent_protocol::OK) { 
						if (!a->atime)
							a->atime = temp.atime;
						if (!a->ctime)
							a->ctime = temp.ctime;
						if (!a->mtime)
							a->mtime = temp.mtime;
						if (extent_caches[eid].state_ == NONE)
							a->size = temp.size;
						else 
							a->size = extent_caches[eid].attr_.size;
					}
				}
				break;
			case REMOVED:
			default:
				ret = extent_protocol::NOENT;
				break;
		}
	} else {
		extent_caches[eid].state_ = NONE;
		a = &extent_caches[eid].attr_;
		a->atime = a->mtime = a->ctime = 0;
		a->size = 0;
		ret = cl->call(extent_protocol::getattr, eid, temp);
		if (ret == extent_protocol::OK) {
			a->atime = temp.atime;
			a->ctime = temp.ctime;
			a->mtime = temp.mtime;
			a->size = temp.size;
		}
	}
	attr = *a;
	return ret;
  return ret;
}

extent_protocol::status
extent_client::put(extent_protocol::extentid_t eid, std::string buf)
{
  extent_protocol::status ret = extent_protocol::OK;
  	int r;
	std::unique_lock<std::mutex> lk(mutex_);
	bool flag = extent_caches.count(eid);

	if (flag) {
		switch (extent_caches[eid].state_) {
			case NONE:
			case UPDATED:
			case DIRTY:
				extent_caches[eid].data_ = buf;
				extent_caches[eid].state_ = DIRTY;
				extent_caches[eid].attr_.mtime = time(NULL);
				extent_caches[eid].attr_.ctime = time(NULL);
				extent_caches[eid].attr_.size = buf.size();
				break;
			case REMOVED:
				ret = extent_protocol::NOENT;
				break;
		}
	} else {
		extent_caches[eid].data_ = buf;
		extent_caches[eid].state_ = DIRTY;
		extent_caches[eid].attr_.atime = time(NULL);
		extent_caches[eid].attr_.mtime = time(NULL);
		extent_caches[eid].attr_.ctime = time(NULL);
		extent_caches[eid].attr_.size = buf.size();
	}
  	return ret;
}

extent_protocol::status
extent_client::remove(extent_protocol::extentid_t eid)
{
  std::unique_lock<std::mutex> lk(mutex_);
  extent_protocol::status ret = extent_protocol::OK;
  if(extent_caches.find(eid) == extent_caches.end())
  {
    return extent_protocol::NOENT;
  }
  extent_caches[eid].state_ = cacheState::REMOVED;
  return ret;
}


