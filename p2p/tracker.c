#include "common.h"

const int domain = AF_INET;
const int port = 8080;
const uint32_t address = INADDR_ANY;

int main(int argc, char ** argv) {
	int sockfd;
	size_t buf_len = 1024;
	char buf[buf_len];
	struct sockaddr_in clntaddr;
	
	initialize_srvr(&sockfd, &domain, &port, &address);
	wait_message(&sockfd, (void*) buf, &buf_len, &clntaddr);


	return 0;
}