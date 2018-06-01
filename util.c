#include "util.h"

extern int Listen(int listenFd, int size) {
	int status = listen(listenFd, size);
	if (status < 0) {
		perror("Listen Error");
		exit(1);
	}
	return status;
}

extern int Accept(int fd, struct sockaddr* c, socklen_t* len) {
	int status = accept(fd, c, len);
	if (status < 0 && errno != EAGAIN) {
		perror("Accept Error");
		exit(1);
	}
	return status;
}

extern int Bind(int fd, struct sockaddr* c, socklen_t len) {
	int status = bind(fd, c, len);
	if (status < 0 && errno != EAGAIN) {
		perror("Bind Error");
		exit(1);
	}
	return status;
}

extern int Socket(int family, int type, int pro) {
	int status = socket(family, type, pro);
	if (status < 0) {
		perror("Socket Error");
		exit(1);
	}
	return status;
}

extern int Getsockopt(int fd, int level, int optname, void* optval, socklen_t* optlen) {
	int status = getsockopt(fd, level, optname, optval, optlen);
	if (status < 0) {
		perror("Get Sock Opt Error");
		exit(1);
	}
	return status;
}

extern int Setsockopt(int fd, int level, int optname, void* optval, socklen_t optlen) {
	int status = setsockopt(fd, level, optname, optval, optlen);
	if (status < 0) {
		perror("Set Sock Opt Error");
		exit(1);
	}
	return status;
}

extern int setSocketNonBlock(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);
	flags |= O_NONBLOCK;
	int status = fcntl(fd, F_SETFL, flags);
	if (status < 0) {
		perror("File Control Error");
		exit(1);
	}
	return status;
}

