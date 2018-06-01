#include "server.h"

extern void run(const char* ip, int port) {
	int listenSocket = Socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in addr;
	addr.sin_port = htons(port);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = (ip == NULL ? htonl(INADDR_ANY) : inet_pton(AF_INET, ip, &addr.sin_addr.s_addr));

	Bind(listenSocket, (strcut sockaddr*)&addr, sizeof(addr));
	Listen(listenSocket, 5);
	setSocketNonBlock(listenSocket);
	
}
