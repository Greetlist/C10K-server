#include "mepoll.h"

extern int getEpollInstance(int size) {
	int epollFileDescriptor;
	if ((epollFileDescriptor = epoll_create1(0)) < 0) {
		perror("Cannot use create1");
		if ((epollFileDescriptor = epoll_create(size)) < 0) {
			perror("Cannot create epoll");
		}
	}
	return epollFileDescriptor;
}

extern int controlEpollInstance(int epfd, int operation, int fd, struct epoll_event* event) {
	int status;
	if ((status = epoll_ctl(epfd, operation, fd, event)) == -1) {
		perror("An Error occurs when call epoll_ctl");
	}
	return status;
}

extern int waitEpollInstance(int epfd, struct epoll_event* events, int maxEvent, int timeOut) {
	int fds = -2;
	if ((fds = epoll_wait(epfd, events, maxEvent, timeOut)) == -1) {
		perror("An Error occurs when call epoll_wait");
	}
	return fds;
}
