#include "epollWeb.h"

//功能:从头开始查找str2在str1中的位置
//参数：str1,str2
//返回值：str2的位置，若不存在则返回-1
int indexOf(char *str1,char *str2){  
    char *p=str1;  
    int i=0;  
    p=strstr(str1,str2);  
    if(p==NULL)  
        return -1;
    else{  
        while(str1!=p){  
            str1++;  
            i++;  
        }  
    }  
    return i;  
}  

//功能:创建和绑定一个TCP socket  
//参数:端口  
//返回值:创建的socket  
static int create_and_bind (char *port){  
  struct addrinfo hints;  
  struct addrinfo *result, *rp;  
  int s, sfd;  
  
  memset (&hints, 0, sizeof (struct addrinfo));  
  hints.ai_family = AF_UNSPEC;     /* Return IPv4 and IPv6 choices */  
  hints.ai_socktype = SOCK_STREAM; /* We want a TCP socket */  
  hints.ai_flags = AI_PASSIVE;     /* All interfaces */  
  
  s = getaddrinfo (NULL, port, &hints, &result);  
  if (s != 0)  
    {  
      fprintf (stderr, "getaddrinfo: %s\n", gai_strerror (s));  
      return -1;  
    }  
  
  for (rp = result; rp != NULL; rp = rp->ai_next)  
    {  
      sfd = socket (rp->ai_family, rp->ai_socktype, rp->ai_protocol);  
      if (sfd == -1)  
        continue;  
  
      s = bind (sfd, rp->ai_addr, rp->ai_addrlen);  
      if (s == 0)  
        {  
          /* We managed to bind successfully! */  
          break;  
        }  
  
      close (sfd);  
    }  
  
  if (rp == NULL)  
    {  
      fprintf (stderr, "Could not bind\n"); 
      return -1;  
    }  
  
  freeaddrinfo (result);  
  
  return sfd;  
}  
  
 
//功能:设置socket为非阻塞的  
static int make_socket_non_blocking (int sfd)  
{  
  int flags, s;  
  
  //得到文件状态标志  
  flags = fcntl (sfd, F_GETFL, 0);  
  if (flags == -1)  
    {  
      perror ("fcntl");  
      return -1;  
    }  
  
  //设置文件状态标志  
  flags |= O_NONBLOCK;  
  s = fcntl (sfd, F_SETFL, flags);  
  if (s == -1)  
    {  
      perror ("fcntl");  
      return -1;  
    }  
  
  return 0;  
}  


void read_all(schedule_t * sch,char * totalbuf){
  ssize_t count;
  int idx = 0;
  coroutine_data *cdata;
  cdata =(coroutine_data *)uthread_getArg(sch);
  while (1){
      char buf[256];
      count = read(cdata->fd, buf, sizeof(buf)); 
      if (count == -1){
          if (errno != EAGAIN){  
              perror ("read but not eagain");  
          }
          cdata->iostate = 'r';
          uthread_yield(sch);
      }else{
          if (idx >= 1023){
            perror("reaching the max buf size!");
          }
          if (indexOf(buf,"\r\n\r\n") >= 0){
            return;
          }
          printf("get:%s\n",buf);
          strcpy(totalbuf+idx,buf);
          idx += count-1;
          
      }
  }
  return;
}

void write_and_flush(schedule_t * sch,char * temp){
  struct epoll_event event; 
  int s;
  coroutine_data *cdata =(coroutine_data *)uthread_getArg(sch);
  int id = uthread_getId(sch);
  if (id < 0){
    perror("uthread_getId() return -1");
  }

  event.data.fd = id;
  event.events = EPOLLOUT | EPOLLET | EPOLLERR | EPOLLHUP;
  s = epoll_ctl (cdata->efd, EPOLL_CTL_MOD,cdata->fd, &event);
  if(s == -1){  
    perror("ready to write failed!,epoll_ctl");
    abort();  
  }
  uthread_yield(sch);
  write(cdata->fd, temp, MAXBUFSIZE);
  s = epoll_ctl (cdata->efd, EPOLL_CTL_DEL,cdata->fd, &event);
  if(s == -1){  
    perror("ready to close,epoll_ctl");  
    abort (); 
  }
  close(cdata->fd);
}


int listen_and_bind(char *port,PFun handler){
  int sfd, s;  
  int efd;  
  struct epoll_event event;  
  struct epoll_event *events;  
  schedule_t *sch = (schedule_t *)calloc(1,sizeof (schedule_t));

  sfd = create_and_bind (port);  
  if (sfd == -1)  
    abort ();  
  
  s = make_socket_non_blocking (sfd);  
  if (s == -1)  
    abort ();  
  
  s = listen (sfd, SOMAXCONN);  
  if (s == -1){  
      perror ("listen");  
      abort ();  
  }  
  
  //除了参数size被忽略外,此函数和epoll_create完全相同  
  efd = epoll_create1 (0);  
  if (efd == -1){  
      perror ("epoll_create");  
      abort ();  
  }  
  
  event.data.fd = sfd;
  event.events = EPOLLIN | EPOLLET;//读入,边缘触发方式  
  s = epoll_ctl (efd, EPOLL_CTL_ADD, sfd, &event);  
  if (s == -1){  
      perror ("epoll_ctl");  
      abort ();  
  }  
  
  /* Buffer where events are returned */  
  events = calloc (MAXEVENTS, sizeof event);  
  
  /* The event loop */  
  while (1){  
      int n, i;  
  
      n = epoll_wait (efd, events, MAXEVENTS, -1);
      for (i = 0; i < n; i++){  
          if ((events[i].events & EPOLLERR) ||  
              (events[i].events & EPOLLHUP)){  
              /* An error has occured on this fd, or the socket is not 
                 ready for reading (why were we notified then?) */  
              fprintf (stderr, "epoll error\n");  
              close (events[i].data.fd);  
              continue;  
           }else if (sfd == events[i].data.fd){
              /* We have a notification on the listening socket, which 
                 means one or more incoming connections. */  
              while (1){
                  struct sockaddr in_addr;
                  socklen_t in_len;
                  int infd;
                  char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
  
                  in_len = sizeof in_addr;  
                  infd = accept(sfd, &in_addr, &in_len);  
                  if (infd == -1){
                      if ((errno == EAGAIN) ||  
                          (errno == EWOULDBLOCK)){
                          /* We have processed all incoming 
                             connections. */  
                          break;  
                      }else{
                          perror ("accept");  
                          break;  
                      }  
                  }
                                  //将地址转化为主机名或者服务名  
                  s = getnameinfo (&in_addr, in_len,  
                                   hbuf, sizeof hbuf,  
                                   sbuf, sizeof sbuf,  
                                   NI_NUMERICHOST | NI_NUMERICSERV);//flag参数:以数字名返回  
                                  //主机地址和服务地址  
  
                  if (s == 0){  
                      printf("Accepted connection on descriptor %d "  
                             "(host=%s, port=%s)\n", infd, hbuf, sbuf);  
                  }  
  
                  /* Make the incoming socket non-blocking and add it to the 
                     list of fds to monitor. */  
                  s = make_socket_non_blocking (infd);
                  if (s == -1)  
                    abort ();

                  coroutine_data *cdata = calloc(1,sizeof (coroutine_data));
                  cdata->fd = infd;
                  cdata->efd = efd;
                  cdata->iostate = 'n';
                  int utd = uthread_create(sch,handler,cdata);

                  event.data.fd = utd;
                  event.events = EPOLLIN | EPOLLET | EPOLLERR | EPOLLHUP;  
                  s = epoll_ctl (efd, EPOLL_CTL_ADD, infd, &event);  
                  if (s == -1){  
                      perror ("epoll_ctl");  
                      abort ();  
                  }

              }  
              continue;  
          }else if (events[i].events & EPOLLIN){
               uthread_resume(sch,events[i].data.fd);
          }else if (events[i].events & EPOLLOUT){
              uthread_resume(sch,events[i].data.fd);
          }else {
              printf("unsupported epoll type(%d)!",events[i].events);
              close (events[i].data.fd);  
          }
      }  
  }  
  
  free(events);  
  close(sfd);

  return EXIT_SUCCESS;  
}
