#include "rpc.h"
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "extent_server.h"

// Main loop of extent server

int
main(int argc, char *argv[])
{
  int count = 0;

  if(argc != 2){
    fprintf(stderr, "Usage: %s port\n", argv[0]);
    exit(1);
  }

  setvbuf(stdout, NULL, _IONBF, 0);

  char *count_env = getenv("RPC_COUNT");
  if(count_env != NULL){
    count = atoi(count_env);
  }

  rpcs server(atoi(argv[1]), count);
  extent_server ls;

  server.reg(extent_protocol::get, &ls, &extent_server::get);
  server.reg(extent_protocol::getattr, &ls, &extent_server::getattr);
  server.reg(extent_protocol::put, &ls, &extent_server::put);
  server.reg(extent_protocol::remove, &ls, &extent_server::remove);
  server.reg(extent_protocol::setattr,&ls,&extent_server::setattr);
  server.reg(extent_protocol::read,&ls,&extent_server::read);
  server.reg(extent_protocol::write,&ls,&extent_server::write);
  server.reg(extent_protocol::create,&ls,&extent_server::create);
  server.reg(extent_protocol::lookup,&ls,&extent_server::lookup);
  server.reg(extent_protocol::readDir,&ls,&extent_server::readDir);
  while(1)
    sleep(1000);
}
