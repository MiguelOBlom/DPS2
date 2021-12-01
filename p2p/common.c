#include "common.h"

// Create a socket and store the file descriptor in server_fd
// We will work with UDP, thus the socket type is set to SOCK_DGRAM
// Automatically assign the best protocol
void _create_udp_socket(int* sockfd, const int* domain) {
	if ((*sockfd = socket(*domain, SOCK_DGRAM, 0)) == 0) {
		perror("Failed creating socket.");
		exit(EXIT_FAILURE);
	} else {
		perror("Successfully created socket.");
	}
}

void _bind_socket(const int* sockfd, const struct sockaddr_in* sockaddr) {
	if (bind(*sockfd, (const struct sockaddr *) sockaddr, sizeof(*sockaddr))) {
		perror("Failed binding address to socket.");
		exit(EXIT_FAILURE);
	} else {
		perror("Successfully bound address to socket.");
	}

}

// Create and initialize a sockaddr_in structure
struct sockaddr_in create_sockaddr_in (const int* domain, const int* port, const uint32_t* address) {
	struct sockaddr_in sockaddr;
	memset(&sockaddr, 0, sizeof(sockaddr));
	// Use the address family specified by domain
	sockaddr.sin_family = *domain;
	// Configure connection port
	sockaddr.sin_port = htons(*port);
	// Use any address for binding
	sockaddr.sin_addr.s_addr = *address;
}

void initialize(int* sockfd, const int* domain, const struct sockaddr_in* sockaddr) {
	_create_udp_socket(sockfd, domain);
	_bind_socket(sockfd, sockaddr);
}