#ifndef _UTIL_H_ 
#define _UTIL_H_ 

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <arpa/inet.h>

extern int Listen(int listenFd, int size);

extern int Accept(int fd, struct sockaddr* c, socklen_t* len);

extern int Bind(int fd, struct sockaddr* c, socklen_t len);

extern int Socket(int family, int type, int pro);

extern int Getsockopt(int fd, int level, int optname, void* optval, socklen_t* optlen);

extern int Setsockopt(int fd, int level, int optname, void* optval, socklen_t optlen);

extern int setSocketNonBlock(int fd);
#endif
