#include "common.h"

// Constructs a message_header with the type and length of the full dataframe
// NOTE: the message_header_checksum depends on the data_checksum
struct message_header init_message_header (enum message_type type, size_t len, uint32_t data_checksum) {
	struct message_header message_header;
	memset(&message_header, 0, sizeof(message_header));
	message_header.type = type;
	message_header.len = len;
	message_header.message_data_checksum = data_checksum;

	// TODO: create hash for checksum
	message_header.message_header_checksum = 42;
	return message_header;
}

// Create and initialize a peer_address struct, given a family, port and address
// NOTE: configuration such as htons/htonl should be performed on the port beforehand
// NOTE: the same configuration holds for the address, e.g. inet_addr
struct peer_address init_peer_address(short unsigned int family, short unsigned int port, uint32_t addr) {
	struct peer_address peer_address;
	peer_address.family = family;
	peer_address.port = port;
	peer_address.addr = addr;
}

// Create a heartbeat header, the header the peer sends to the tracker to
// register itself and tell it is alive.
struct heartbeat_header init_heartbeat_header (const struct peer_address* pa) {
	uint32_t data_checksum;
	struct heartbeat_header heartbeat_header;
	heartbeat_header.peer_address = *pa;
	data_checksum = 0; // TODO
	heartbeat_header.message_header = init_message_header(HEARTBEAT, sizeof(heartbeat_header), data_checksum);
	return heartbeat_header;
}


// Create a socket and store the file descriptor in server_fd
// We will work with UDP, thus the socket type is set to SOCK_DGRAM
// Automatically assign the best protocol
void _create_udp_socket(int* sockfd, const short unsigned int family) {
	if ((*sockfd = socket(family, SOCK_DGRAM, 0)) == 0) {
		perror("Failed creating socket");
		exit(EXIT_FAILURE);
	} else {
		perror("Successfully created socket");
	}
}

// Binds address to socket using the socket file descriptor and socket address
void _bind_socket(const int* sockfd, const struct sockaddr_in* sockaddr) {
	if (bind(*sockfd, (const struct sockaddr*) sockaddr, sizeof(*sockaddr)) < 0) {
		perror("Failed binding address to socket");
		exit(EXIT_FAILURE);
	} else {
		perror("Successfully bound address to socket");
	}
}

// Create and initialize a sockaddr_in structure
struct sockaddr_in _create_sockaddr_in (const struct peer_address* pa) {
	struct sockaddr_in sockaddr;
	sockaddr.sin_family = pa->family; // Use the address family specified by domain
	sockaddr.sin_port = pa->port; // Configure connection port
	sockaddr.sin_addr.s_addr = pa->addr; // Use any address for binding
	return sockaddr;
}

// Initialize a socket, will create a socket file descriptor
// Requires a domain and initialized socket address
void initialize_srvr(int* sockfd, const struct peer_address* pa) {
	struct sockaddr_in sockaddr = _create_sockaddr_in(pa);
	_create_udp_socket(sockfd, pa->family);
	_bind_socket(sockfd, &sockaddr);
}

// Initialize a socket, will create a socket file descriptor
// Requires a domain and initialized socket address
void initialize_clnt(int* sockfd, const struct peer_address* pa, struct sockaddr_in* sockaddr) {
	*sockaddr = _create_sockaddr_in(pa);
	_create_udp_socket(sockfd, pa->family);
}

// Receive a message on the socket file descriptor sockfd
// The message is provided through data, with a max length of data_len, the actual size (or error) is returned as ssize_t
// Flags flags are passed through to the socket recvfrom call
// Furthermore, a sockaddr should be passed, an uninitialized sockaddr_len can be passed
// in these values the information about the sender is stored
ssize_t recv_message(const int* sockfd, void* data, const size_t data_len, int flags, struct sockaddr_in** sockaddr, socklen_t* sockaddr_len) {
	*sockaddr_len = sizeof(**sockaddr);
	ssize_t msg_len = recvfrom(*sockfd, data, data_len, flags, (struct sockaddr*) sockaddr, sockaddr_len);
	if (msg_len < 0) {
		perror("Failed receiving message");
	} else {
		perror("Successfully received message");
	}
	
	return msg_len;
}

// Send a message on the socket file descriptor sockfd
// A message containing data of size data_len is sent
// Flags flags are passed through to the socket sendto call
// A sockaddress is passed to whom the message is sent
// Returns the number of characters sent, or an error upon error sending the message
ssize_t send_message(const int* sockfd, const void* data, const size_t data_len, int flags, const struct sockaddr_in* sockaddr) {
	ssize_t msg_len;
	if ((msg_len = sendto(*sockfd, data, data_len, flags, (const struct sockaddr*) sockaddr, sizeof(*sockaddr))) < 0) {
		perror("Failed sending message");
	} else {
		perror("Successfully sent message");
	}
	return msg_len;
}
