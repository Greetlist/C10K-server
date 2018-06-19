#ifndef IPC_UNIX_H
#define IPC_UNIX_H

#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <string.h>

#define CONTROLLEN CMSG_LEN(sizeof(int))

extern int sendFD(int fd, int fdSend);

extern int recvFD(int fd, ssize_t (*userfunc)(int, const void*, size_t));

#endif
