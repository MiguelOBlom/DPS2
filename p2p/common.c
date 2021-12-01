#include "common.h"

// Create a socket and store the file descriptor in server_fd
// We will work with UDP, thus the socket type is set to SOCK_DGRAM
// Automatically assign the best protocol
void _create_udp_socket(int* sockfd, const int* domain) {
	printf("Domain used: %d, AF_INET: %d\n", *domain, AF_INET);
	if ((*sockfd = socket(*domain, SOCK_DGRAM, 0)) == 0) {
		perror("Failed creating socket.");
		exit(EXIT_FAILURE);
	} else {
		perror("Successfully created socket.");
	}
	printf("Socket file descriptor: %d\n", *sockfd);
}

// Binds address to socket using the socket file descriptor and socket address
void _bind_socket(const int* sockfd, const struct sockaddr_in* sockaddr) {
	//printf("%d\n", *domain);
	printf("Socket file descriptor: %d\n", *sockfd);
	printf("sin_family: %u, sin_port: %u, s_addr: %u\n", sockaddr->sin_family, sockaddr->sin_port, sockaddr->sin_addr.s_addr);
	if (bind(*sockfd, (const struct sockaddr*) sockaddr, sizeof(*sockaddr)) < 0) {
		perror("Failed binding address to socket.");
		exit(EXIT_FAILURE);
	} else {
		perror("Successfully bound address to socket.");
	}

}

// Create and initialize a sockaddr_in structure
struct sockaddr_in _create_sockaddr_in (const int* domain, const int* port, const uint32_t* address) {
	struct sockaddr_in sockaddr;
	memset(&sockaddr, 0, sizeof(sockaddr));
	// Use the address family specified by domain
	sockaddr.sin_family = *domain;
	// Configure connection port
	sockaddr.sin_port = htons(*port);
	// Use any address for binding
	sockaddr.sin_addr.s_addr = *address;
	printf("%u %u %u\n", sockaddr.sin_family, sockaddr.sin_port, sockaddr.sin_addr.s_addr);
	return sockaddr;
}

// Initialize a socket, will create a socket file descriptor
// Requires a domain and initialized socket address
void initialize_srvr(int* sockfd, const int* domain, const int* port, const uint32_t* address) {
	struct sockaddr_in sockaddr = _create_sockaddr_in(domain, port, address);
	_create_udp_socket(sockfd, domain);
	_bind_socket(sockfd, &sockaddr);
}

// Initialize a socket, will create a socket file descriptor
// Requires a domain and initialized socket address
void initialize_clnt(int* sockfd, const int* domain, const int* port, const uint32_t* address, struct sockaddr_in* sockaddr) {
	*sockaddr = _create_sockaddr_in(domain, port, address);
	_create_udp_socket(sockfd, domain);
}

void _get_message(const int* sockfd, void* data, const size_t* data_len, const int flags, struct sockaddr_in* sockaddr) {
	socklen_t sockaddr_len = sizeof(*sockaddr);
	ssize_t msg_len = recvfrom(*sockfd, data, *data_len, flags, (struct sockaddr*) sockaddr, &sockaddr_len);
	perror("Received message.");
	// TODO Do something with msglen
	// What if clntaddr_len changed?
}

void wait_message(const int* sockfd, void* data, const size_t* data_len, struct sockaddr_in* sockaddr) {
	_get_message(sockfd, data, data_len, MSG_WAITALL, sockaddr);
}

void send_message(const int* sockfd, const void* data, const size_t* data_len, int flags, const struct sockaddr_in* sockaddr) {
	if (sendto(*sockfd, data, *data_len, flags, (const struct sockaddr*) sockaddr, sizeof(*sockaddr)) < 0) {
		perror("Failed sending message.");
	} else {
		perror("Successfully sent message.");
	}

}