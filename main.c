#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <linux/ioctl.h>

#include "util.h"
#include "ipcunix.h"

int numThread = 3;
int listenFD;

typedef struct {
  int pid;
  int connectFD;
} ProcessInfo;

static const char* response = "Here is some information.\n\0";

int writeBytes(int fd, const char* str, size_t size) {
  char buf[size];
  memcpy(buf, str, size);
  int n = write(fd, buf, size);
  if (n < 0) {
    return 0;
  }
  return 1;
}

void* dealData(void* arg) {
  int userFD = *((int*)arg);
  char buf[8192];
  memset(buf, 0, sizeof(buf));
  struct timespec start, end;
  clockid_t id = 1;
  int n;
  int fd = open("currentJson.json", O_RDWR | O_CREAT | O_APPEND, 0777);
  clock_gettime(id, &start);
  while ((n = read(userFD, buf, sizeof(buf))) > 0) {
    printf("The thread id is : %ld, The n value is : %d.\n", pthread_self(), n);
    write(fd, buf, n);
    memset(buf, 0, sizeof(buf));
  }  
  if (n < 0 && errno != EAGAIN) {
    perror("Read");
    exit(1);
  } else {
    printf("EOF.\n");
  }
  clock_gettime(id, &end);
  printf("The thread id is : %ld, The read operation costs %ld nanoseconds.\n", pthread_self(), end.tv_nsec - start.tv_nsec);

  return ((void*)0);
}

void* initWorkerThread(void* arg) {
  int epfd = epoll_create1(0);
  if (epfd < 0) {
    perror("Epoll Create Error");
    printf("The thread id is : %ld.\n", pthread_self());
    pthread_exit((void*)1);
  }

  struct epoll_event ev;
  memset(&ev, 0, sizeof(ev));
  ev.data.fd = listenFD;
  ev.events = EPOLLIN | EPOLLET; 

  int ss = epoll_ctl(epfd, EPOLL_CTL_ADD, listenFD, &ev);
  if (ss < 0) {
    perror("Epoll Add Error");
    pthread_exit((void*)1);
  } else {
    printf("Start Epoll Wait.\n");
  }

  struct epoll_event events[128];
  while (1) {
    int num = epoll_wait(epfd, events, 128, -1);
    printf("Thread Wake up.\n");
    
    for (int i = 0; i < num; ++i) {
      if (events[i].data.fd == listenFD) {
        struct sockaddr_in client;
        socklen_t len;
        int connFD = accept(listenFD, (struct sockaddr*)&client, &len);
        if (connFD < 0 && errno == EAGAIN) {
          printf("The resource is not exist.\n");
        } else {
          printf("Thread id is : %ld, The client port is : %d.\n", pthread_self(), ntohs(client.sin_port));
        }
      }
    }
  }
  return ((void*)0);
}

int getRecvBufSize(int sock) {
  int value;
  socklen_t len = sizeof(value);
  int status = getsockopt(sock, SOL_SOCKET, SO_RCVBUF, &value, &len);
  if (status < 0) {
    perror("Get Opt Error");
  }
  printf("The recv buf size is : %d.\n", value);
  return value;
}

int setRecvBufSize(int sock, size_t len) {
  int value = len;
  socklen_t l = sizeof(value);
  int status = setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &value, l);
  if (status < 0) {
    perror("Set Opt Error");
  }
}

struct event_relate_data {
  int fd;
  int msg_len;
};

int initWorker(int pipeFD) {
  close(listenFD);
  int epfd = epoll_create1(0);
  if (epfd < 0) {
    perror("Epoll Create Error");
  }
    struct event_relate_data* data = malloc(sizeof(struct event_relate_data));
    data->fd = pipeFD;
    data->msg_len = 0;
  struct epoll_event ev;
  memset(&ev, 0, sizeof(ev));
  ev.data.ptr = data;
  ev.events = EPOLLIN | EPOLLET; 

  int ss = epoll_ctl(epfd, EPOLL_CTL_ADD, pipeFD, &ev);
  if (ss < 0) {
    perror("Epoll Add Error");
    exit(1);
  } else {
    printf("Start Epoll Wait.\n");
  }

  int current = 0;
  struct epoll_event events[128];
  while (1) {
    int num = epoll_wait(epfd, events, 128, -1);
    if (num < 0) {
      perror("Epoll Error");
    }
    /*printf("The pid is : %d.\n", getpid());*/
    for (int i = 0; i < num; ++i) {
      struct event_relate_data* data = (struct event_relate_data*)events[i].data.ptr;
      if (data->fd == pipeFD) {
        int client;
        int status = recvFD(data->fd, NULL);
        if (status > 0) {
          client = status;
        } else {
          printf("Read File Descriptor Error.\n");
          continue;
        }

        struct epoll_event currentEvent;
        memset(&currentEvent, 0, sizeof(currentEvent));
        setSocketNonBlock(client);
        currentEvent.events = EPOLLET | EPOLLIN;
        struct event_relate_data* data = malloc(sizeof(struct event_relate_data));
        data->fd = client;
        data->msg_len = 0;
        currentEvent.data.ptr = data;
        status = epoll_ctl(epfd, EPOLL_CTL_ADD, client, &currentEvent);
        if (status < 0) {
          perror("Epoll Error");
        }
        /*printf("Deal With Pipe.The pid is : %d, The client is : %d.\n", getpid(), client);*/
      } else {
        if (events[i].events & EPOLLIN) {
          int buf_size = getRecvBufSize(data->fd);
          printf("BufSize is : %d.\n", buf_size);
          int left_buff_count = 0;
          ioctl(data->fd, FIONREAD, &left_buff_count);
          printf("Left Bytes Count:%d.\n", left_buff_count);
          char rBuff[1024];
          memset(rBuff, 0, sizeof(rBuff));
          int status = 0;
          int total_read = 0;

          struct event_relate_data* data = (struct event_relate_data*)events[i].data.ptr;
          if (data->msg_len == 0) {
            //read size first
            int cur_msg_len = 0;
            read(data->fd, &cur_msg_len, sizeof(int));
            data->msg_len = ntohl(cur_msg_len);
            printf("Total Need Read Bytes are: %d.\n", data->msg_len);
          }

          int cur_file = open("/home/greetlist/test/tar_file.tar.gz", O_RDWR | O_CREAT | O_APPEND, 0664);
          while ((status = read(data->fd, rBuff, sizeof(rBuff))) > 0) {
            write(cur_file, rBuff, status);
            total_read += status;
            data->msg_len -= status;
          }
          printf("Total Read: %d, msg_len: %d\n", total_read, data->msg_len);
          if (status < 0 && errno == EAGAIN) {
            printf("Try Again.\n");
          } else if (status == 0) {
            status = epoll_ctl(epfd, EPOLL_CTL_DEL, data->fd, NULL);
            shutdown(data->fd, SHUT_RDWR);
            close(data->fd);
            printf("Peer close the connection.\n");
          } else if (status < 0 && errno != EAGAIN) {
            printf("Wrong Pid is : %d.\n", getpid());
            perror("Read Error");
            status = epoll_ctl(epfd, EPOLL_CTL_DEL, data->fd, NULL);
            shutdown(data->fd, SHUT_RDWR);
            close(data->fd);
          }
        }
      }
    }
  }
}

int main(int argc, char** argv) {
  pthread_t threadArr[numThread];
  listenFD = socket(AF_INET, SOCK_STREAM, 0);
  if (listenFD < 0) {
    perror("Socket Error");
  }

  struct sockaddr_in server;
  memset(&server, 0, sizeof(server));
  server.sin_family = AF_INET;
  server.sin_port = htons(4321);
  server.sin_addr.s_addr = htonl(INADDR_ANY);

  int current;
  int l = sizeof(int);
  current |= SO_REUSEADDR; 
  setsockopt(listenFD, SOL_SOCKET, SO_REUSEADDR, &current, l);
  int ss = bind(listenFD, (struct sockaddr*)&server, sizeof(server)); 
  if (ss < 0) {
    perror("Bind Error");
  }

  ProcessInfo processes[numThread];
  int currentProcess = 0;

  int n;
  for (int i = 0; i < numThread; ++i) {
    /*ss = pthread_create(&threadArr[i], NULL, initWorkerThread, NULL);*/
    /*printf("The %dth thread id is : %ld.\n", i, threadArr[i]);*/
    /*if (ss < 0) {*/
      /*perror("Thread Create Error");*/
    /*}*/
    int currentfd[2];
    int currentSock = socketpair(AF_UNIX, SOCK_STREAM, 0, currentfd);
    if ((n = fork()) == 0) {
      close(currentfd[1]);
      initWorker(currentfd[0]);
      break;
    } else if (n > 0) {
      /*printf("The child process id is : %d.\n", n);*/
      close(currentfd[0]);
      processes[i].pid = n;
      processes[i].connectFD = currentfd[1];
    }
  }

  setSocketNonBlock(listenFD);
  ss = listen(listenFD, 64);
  if (ss < 0) {
    perror("Listen Error");
  }

  int epfd = epoll_create1(0);
  struct epoll_event ev;
  memset(&ev, 0, sizeof(ev));
  ev.data.fd = listenFD;
  ev.events = EPOLLET | EPOLLIN;
  int st = epoll_ctl(epfd, EPOLL_CTL_ADD, listenFD, &ev);

  struct epoll_event events[128];
  while (1) {
    int nums = epoll_wait(epfd, events, 128, -1);
    printf("The event num is : %d.\n", nums);
    for (int i = 0; i < nums; ++i) {
      if (events[i].data.fd == listenFD) {
        struct sockaddr_in cli;
        socklen_t len = sizeof(struct sockaddr_in);
        memset(&cli, 0, sizeof(cli));
        int clientFD = 0;
        while ((clientFD = accept(listenFD, (struct sockaddr*)&cli, &len)) > 0) {
          printf("Parent.The client is %d.\n", clientFD);
          int curStatus = sendFD(processes[(currentProcess++) % numThread].connectFD, clientFD);
          if (curStatus < 0) {
            perror("Send FD Error");
          }
          close(clientFD);
        }
        if (clientFD < 0 && errno != EAGAIN) {
          perror("Accept Error");
        }
      }
    }
  }
  perror("Final Error is ");

  return 0;
}


/*int main(int argc, char** argv) {
  int listenSock = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in server;
  socklen_t len;
  if (listenSock < 0) {
    perror("Create socket error");
  }
  memset(&server, 0, sizeof(struct sockaddr_in));
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = htonl(INADDR_ANY);
  server.sin_port = htons(4321);

  int current;
  int l = sizeof(int);
  current |= SO_REUSEADDR; 
  setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, &current, l);
  int status = getsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, &current, &l);
  if (status < 0) {
    perror("Get Sock Error");
  }
  printf("%d.\n", current);

  status = bind(listenSock, (struct sockaddr*)&server, sizeof(server));
  if (status < 0) {
    perror("Bind Error");
    exit(1);
  }

  setRecvBufSize(listenSock, 10000);
  getRecvBufSize(listenSock);

  status = listen(listenSock, 5);
  if (status < 0) {
    perror("Listen Error");
    exit(1);
  }

  int flags = fcntl(listenSock, F_SETFL, 0);
  if (flags < 0) {
    perror("Get file flags Error");
    exit(1);
  }
  flags |= O_NONBLOCK;

  fcntl(listenSock, F_SETFL, flags);

  int epfd = epoll_create(200);
  if (epfd < 0) {
    perror("Epoll Create Error");
    exit(1);
  }

  struct epoll_event ev;
  memset(&ev, 0, sizeof(struct epoll_event));
  ev.data.fd = listenSock;
  ev.events = EPOLLET | EPOLLIN;

  if ((status = epoll_ctl(epfd, EPOLL_CTL_ADD, listenSock, &ev)) < 0) {
    perror("Epoll Add Error");
    exit(1);
  }

  int ret;
  struct epoll_event events[200];
  len = sizeof(socklen_t);
  printf("Start the epoll loop.\n");
  while (1) {
    ret = epoll_wait(epfd, events, 200, -1);
    if (ret < 0) {
      perror("Epoll Wait Error");
      exit(1);
    }
    printf("The events is : %d.\n", ret);
    for (int i = 0; i < ret; ++i) {
      if (events[i].data.fd == listenSock) {
        struct sockaddr_in client;
        int connFd = accept(listenSock, (struct sockaddr*) &client, &len);
        printf("Accept a client.\n");
        if (connFd < 0) {
          perror("Accept Error");
          exit(1);
        }
        printf("The client port is : %d\n", ntohs(client.sin_port));
        int currentFlag = fcntl(connFd, F_GETFL, 0);
        currentFlag |= O_NONBLOCK;
        fcntl(connFd, F_SETFL, currentFlag);
        struct epoll_event currentEvent;
        memset(&currentEvent, 0, sizeof(struct epoll_event));
        currentEvent.data.fd = connFd;
        currentEvent.events = EPOLLIN | EPOLLET;
        if ((status = epoll_ctl(epfd, EPOLL_CTL_ADD, connFd, &currentEvent)) < 0) {
          perror("Add Client Error");
        }
        getRecvBufSize(connFd);
      } else {
        struct timespec start, end;
        clockid_t id = 1;
        int fd = open("current.json", O_RDWR | O_CREAT | O_APPEND, 0777);
        if (fd < 0) {
          perror("Open Error");
          exit(1);
        }
        char buf[8192];
        int rn, wn;
        clock_gettime(id, &start);
        sleep(10);
        while (1) {
          if ((rn = read(events[i].data.fd, buf, sizeof(buf))) > 0) {
            printf("Read %d bytes data.\n", rn);
            if ((wn = write(fd, buf, rn)) < 0) {
              perror("Write Error");
              break;
            }
          } else if (rn < 0) {
            if (errno != EAGAIN) {
              perror("Read Error");
            } else {
              printf("No data.\n");
            }
            break;
          } else {
            printf("EOF.\n");
            break;
          }
        }

        clock_gettime(id, &end);
        printf("The Total Read Time is : %ld.\n", end.tv_nsec - start.tv_nsec);
        pthread_t tid;
        pthread_create(&tid, NULL, dealData, (void*)&events[i].data.fd);
      }
    }
  }

  close(listenSock);
  return 0;
}*/

