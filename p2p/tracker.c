#include "common.h"


int main(int argc, char ** argv) {
	int sockfd;
	int domain = AF_INET;
	const int port = 8080;
	uint32_t address = INADDR_ANY; 
	initialize(&sockfd, &domain, &port, &address);



	return 0;
}