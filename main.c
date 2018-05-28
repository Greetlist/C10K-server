#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>

void* dealData(void* arg) {
	int userFD = *((int*)arg);
	char buf[4096];
	memset(buf, 0, sizeof(buf));
	struct timespec start, end;
	clockid_t id = 1;
	int n;
	int fd = open("currentJson.json", O_RDWR | O_CREAT | O_APPEND);
	clock_gettime(id, &start);
	while ((n = read(userFD, buf, sizeof(buf))) != EOF) {
		if (n < 0) {
			perror("Read Socket Error");
			break;
		} else {
			write(fd, buf, n);
		}
		memset(buf, 0, sizeof(buf));
	}	
	if (n < 0) {
		if (errno != EAGAIN) {
			perror("Read");
			exit(1);
		}
	}
	clock_gettime(id, &end);
	printf("The read operation costs %ld nanoseconds.\n", end.tv_nsec - start.tv_nsec);
	printf("Read %d charactors. \n", n);

	return ((void*)0);
}

int main(int argc, char** argv) {
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
			} else {
				/*char buf[4096];*/
				/*memset(buf, 0, sizeof(buf));*/
				/*int n = read(events[i].data.fd, buf, sizeof(buf));*/
				/*n = read(events[i].data.fd, buf, sizeof(buf));*/
				/*if (n < 0) {*/
					/*perror("Read");*/
				/*}*/
				/*if (n == 0) {*/
					/*printf("Peer close the socket");*/
				/*}*/
				/*printf("Read %d charactors. \n", n);*/
				/*printf("There are some datas.The data is : \n");*/
				/*printf("%s.\n", buf);*/
				pthread_t tid;
				pthread_create(&tid, NULL, dealData, (void*)&events[i].data.fd);
			}
		}
	}

	close(listenSock);
	return 0;
}

