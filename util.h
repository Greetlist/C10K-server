#ifndef _UTIL_H_ 
#define _UTIL_H_ 

#include <sys/epoll.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>

extern int Listen(int listenFd, int size);

extern int Accept(int fd, );

extern int Fcntl();

extern int Bind();

extern int Write();

extern int Read();

#endif
