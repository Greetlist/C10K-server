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
#include <arpa/inet.h>

#include "util.h"

int numThread = 8;
int listenFD;

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
		
		sleep(5);
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
	return status;
}

int setRecvBufSize(int sock, size_t len) {
	int value = len;
	socklen_t l = sizeof(value);
	int status = setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &value, l);
	if (status < 0) {
		perror("Set Opt Error");
	}
}

void initWorker() {
	int epfd = epoll_create1(0);
	if (epfd < 0) {
		perror("Epoll Create Error");
		/*printf("The thread id is : %ld.\n", pthread_self());*/
		/*pthread_exit((void*)1);*/
	}

	/*printf("The Process id is : %d.\n", getpid());*/
	/*while (1) {}*/

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

	int flags = fcntl(listenFD, F_GETFL, 0);
	flags &= O_NONBLOCK;
	if (flags) {
		printf("Has O_NONBLOCK.\n");
	}

	struct epoll_event events[128];
	while (1) {
		int num = epoll_wait(epfd, events, 128, -1);
		printf("Process Wake up.\n");
		
		for (int i = 0; i < num; ++i) {
			printf("In the For.\n");
			if (events[i].data.fd == listenFD) {
				struct sockaddr_in client;
				socklen_t len;
				int connFD = accept(listenFD, (struct sockaddr*)&client, &len);
				if (connFD < 0 && errno == EAGAIN) {
					printf("The resource is not exist.\n");
				} else if (connFD < 0) {
					perror("Accept Error");
				} else {
					/*printf("Thread id is : %d, The client port is : %d.\n", getpid(), ntohs(client.sin_port));*/
					setSocketNonBlock(connFD);
					struct epoll_event currentEvent;
					memset(&currentEvent, 0, sizeof(currentEvent));
					currentEvent.data.fd = connFD;
					currentEvent.events = EPOLLET | EPOLLIN | EPOLLOUT;
					int st = epoll_ctl(epfd, EPOLL_CTL_ADD, connFD, &currentEvent);
					if (st < 0) {
						perror("Epoll Control Error");
					}
				}
			} else if (events[i].events & EPOLLIN) {
				char buf[1024];
				int nu = read(events[i].data.fd, buf, sizeof(buf));
				write(STDOUT_FILENO, buf, strlen(buf));
			} else if (events[i].events & EPOLLOUT) {
				if (!writeBytes(events[i].events, response, strlen(response))) {
					perror("Write Error");
				}
			}
		}
		printf("Out the For.\n");
	}
}

int main(int argc, char** argv) {
	pthread_t threadArr[numThread];
	listenFD = socket(AF_INET, SOCK_STREAM, 0);
	if (listenFD < 0) {
		perror("Socket Error");
	}

	setSocketNonBlock(listenFD);

	struct sockaddr_in server;
	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(4321);
	server.sin_addr.s_addr = htonl(INADDR_ANY);

	int ss = bind(listenFD, (struct sockaddr*)&server, sizeof(server)); 
	if (ss < 0) {
		perror("Bind Error");
	}
	
	ss = listen(listenFD, 5);
	if (ss < 0) {
		perror("Listen Error");
	}

	int n;
	for (int i = 0; i < numThread; ++i) {
		/*ss = pthread_create(&threadArr[i], NULL, initWorkerThread, NULL);*/
		/*printf("The %dth thread id is : %ld.\n", i, threadArr[i]);*/
		/*if (ss < 0) {*/
			/*perror("Thread Create Error");*/
		/*}*/
		if ((n = fork()) == 0) {
			initWorker();
			break;
		} else if (n > 0) {
			printf("The child process id is : %d.\n", n);
		}
	}

	while (1) {}

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

