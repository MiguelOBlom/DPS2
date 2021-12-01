#include "common.h"


int main(int argc, char ** argv) {
	const int port = 8080;
	uint32_t address = INADDR_ANY; 
	int sockfd;
	int domain = AF_INET;
	struct sockaddr_in sockaddr = create_sockaddr_in(&domain, &port, &address);
	initialize(&sockfd, &domain, &sockaddr);



	return 0;
}