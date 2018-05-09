#ifndef _MEPOLL_H_
#define _MEPOLL_H_

#include "util.h"

extern int getEpollInstance(int size);

extern int controlEpollInstance(int epfd, int operation, int fd, struct epoll_event* events);

extern int waitEpollInstance(int epfd, struct epoll_event* events, int maxEvent, int timeOut);

#endif
