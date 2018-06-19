#include "ipcunix.h"

extern int sendFD(int fd, int fdSend) {
	struct iovec iov[1];
	struct msghdr msg;
	memset(&msg, 0, sizeof(msg));
	char buf[128];

	const char* str = "Hello World.\n";
	memcpy(buf, str, strlen(str));
	iov[0].iov_base = buf;
	iov[0].iov_len = 128;

	msg.msg_iov = iov;
	msg.msg_iovlen = 1;
	msg.msg_name = NULL;
	msg.msg_namelen = 0;

	struct cmsghdr* cmsg = malloc(sizeof(struct cmsghdr));
	if (fdSend < 0) {
		return -1;
	} else {
		memset(cmsg, 0, sizeof(cmsg));
		cmsg->cmsg_level = SOL_SOCKET;
		cmsg->cmsg_type = SCM_RIGHTS;
		cmsg->cmsg_len = CONTROLLEN;

		msg.msg_control = cmsg;
		msg.msg_controllen = CONTROLLEN;
		*(int*)CMSG_DATA(cmsg) = fdSend;
	}

	int numSend;
	if ((numSend = sendmsg(fd, &msg, 0)) < 0) {
		perror("Send Msg Error");
		return -1;
	}
	free(cmsg);
	return 0;
}

extern int recvFD(int fd, ssize_t (*userfunc)(int, const void*, size_t)) {
	int newFD, nr;
	char* ptr;
	char buf[128];
	struct iovec iov[1];
	struct msghdr msg;
	struct cmsghdr* cmsg = malloc(sizeof(struct cmsghdr));
	memset(cmsg, 0, sizeof(struct cmsghdr));

	memset(buf, 0, 128);
	while (1) {
		iov[0].iov_base = buf;
		iov[0].iov_len = 128;
		msg.msg_iov = iov;
		msg.msg_iovlen = 1;
		msg.msg_name = NULL;
		msg.msg_namelen = 0;
		msg.msg_control = cmsg;
		msg.msg_controllen = CONTROLLEN;

		if ((nr = recvmsg(fd, &msg, 0)) < 0) {
			perror("Recv Msg Error");
			return -1;
		} else if (nr == 0) {
			printf("Connection closed by peer.\n");
			return -1;
		} else {
			newFD = *(int*)CMSG_DATA(cmsg);
			return newFD;
		}
	}
}
