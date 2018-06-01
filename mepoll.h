#ifndef _MEPOLL_H_
#define _MEPOLL_H_

#include <sys/epoll.h>
#include <stdio.h>
#include <stdlib.h>

extern int getEpollInstance(int size);

extern int controlEpollInstance(int epfd, int operation, int fd, struct epoll_event* events);

extern int waitEpollEvents(int epfd, struct epoll_event* events, int maxEvent, int timeOut);

#endif
