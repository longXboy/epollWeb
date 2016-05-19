
#ifndef MY_EPOLL_H
#define MY_EPOLL_H

struct coroutine_data;
struct coroutine_data{
  int fd;
  int efd;
  char iostate;
}coroutine_data;

#endif

