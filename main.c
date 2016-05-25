
#include "epollWeb.h"

int connectIdx = 0 ;

void processConnect(void * sch){
  schedule_t *s = (schedule_t *)sch;
  char totalbuf[1024];
  read_all(s,totalbuf);
  printf("read end!\n");
  char *wb;
  char temp[MAXBUFSIZE];
  char temphello[MAXBUFSIZE];
  sprintf(temphello,"hello %d Guest",connectIdx++);
  sprintf(temp, "HTTP/1.0 200 OK\r\nContent-length:%lu\r\nContent-type:text/html\r\n\r\n%s", strlen(temphello),temphello);
  write_and_flush(s,temp);
  return;
}

//端口由参数argv[1]指定  
int main (int argc, char *argv[]){  
  if (argc != 2){  
      fprintf (stderr, "Usage: %s [port]\n", argv[0]);  
      exit (EXIT_FAILURE);  
  }  
  
  int ret = listen_and_bind(argv[1],processConnect);  
 
  return ret;
}  
