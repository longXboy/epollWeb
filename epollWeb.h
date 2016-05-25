
#ifndef MY_EPOLL_H
#define MY_EPOLL_H

#include <stdio.h>
#include <string.h>  
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <string.h>
#include "uthread.h"

#define MAXEVENTS 64  
#define MAXBUFSIZE 2048

struct coroutine_data;
typedef struct coroutine_data{
  int fd;
  int efd;
  char iostate;
}coroutine_data;

typedef void (*PFun)(void *sch);

int listen_and_bind(char *port,PFun handler);
void read_all(schedule_t * sch,char * totalbuf);
void write_and_flush(schedule_t * sch,char * temp);

#endif

