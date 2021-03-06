// Author: Miguel Blom

#include "common.h"

// Constructs a message_header with the type and length of the full dataframe
// NOTE: the message_header_checksum depends on the data_checksum
struct message_header init_message_header (enum message_type type, size_t len, POLY_TYPE data_checksum) {
	struct message_header message_header;
	memset(&message_header, 0, sizeof(message_header));
	message_header.type = type;
	message_header.len = len;
	message_header.message_data_checksum = data_checksum;
	message_header.message_header_checksum = get_crc(&message_header, sizeof(message_header) - sizeof(message_header.message_header_checksum));
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
	return peer_address;
}

// Create a heartbeat header, the header the peer sends to the tracker to
// register itself and tell it is alive.
struct peer_address_header init_peer_address_header (const struct peer_address* pa, enum message_type mtype) {
	POLY_TYPE data_checksum;
	struct peer_address_header peer_address_header;
	peer_address_header.peer_address = *pa;
	data_checksum = get_crc(pa, sizeof(struct peer_address));
	peer_address_header.message_header = init_message_header(mtype, sizeof(peer_address_header), data_checksum);
	return peer_address_header;
}

int check_message_crc(void * data, size_t data_len) {
	int retval;
	POLY_TYPE crc = ((struct message_header*) data)->message_header_checksum;
	((struct message_header*) data)->message_header_checksum = 0;
	retval =  (check_crc((struct message_header*) data, sizeof(struct message_header) - sizeof(((struct message_header*) data)->message_header_checksum), crc)
		&& (data_len == sizeof(struct message_header) || check_crc(data + sizeof(struct message_header), data_len - sizeof(struct message_header), ((struct message_header*) data)->message_data_checksum)));
	((struct message_header*) data)->message_header_checksum = crc;
	return retval;
}

// Create a pointer to a data object consisting of a configured message header 
// and attached data of size data_len
// Updates data_len with the data of the full frame
void* init_network_information (const void * data, size_t* data_len) {
	POLY_TYPE data_checksum = get_crc(data, *data_len);
	struct message_header message_header;
	// The size of the message header + data
	size_t total_len = sizeof(message_header) + *data_len;
	// Initialize the message header
	message_header = init_message_header(NETINFO, total_len, data_checksum);
	// Allocate memory for the whole frame, copy the header and data
	void * frame = malloc(total_len);
	memcpy(frame, &message_header, sizeof(message_header));
	memcpy(frame + sizeof(message_header), data, *data_len);

	*data_len = total_len;
	return frame;
}

// Create a socket and store the file descriptor in server_fd
// We will work with UDP, thus the socket type is set to SOCK_DGRAM
// Automatically assign the best protocol
void _create_udp_socket(int* sockfd, const short unsigned int family) {
	if ((*sockfd = socket(family, SOCK_DGRAM, 0)) < 0) {
		perror("Failed creating socket");
		exit(EXIT_FAILURE);
	} else {
		///perror("Successfully created socket");
	}
}

// Binds address to socket using the socket file descriptor and socket address
void _bind_socket(const int* sockfd, const struct sockaddr_in* sockaddr) {
	if (bind(*sockfd, (const struct sockaddr*) sockaddr, sizeof(*sockaddr)) < 0) {
		perror("Failed binding address to socket");
		exit(EXIT_FAILURE);
	} else {
		///perror("Successfully bound address to socket");
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

// Converts a port coded in string format to a short unsigned int,
// which is used in the peer address
// We apply htons on the port to configure it
// Returns -1 on error, on success 0
int convert_port(char* chport, short unsigned int* port) {
	unsigned long int uliport = strtoul(chport, NULL, 10);
	// Check if conversion was successful	
	if (uliport == 0 && (errno == EINVAL || errno == ERANGE)) {
		///perror("Could not convert port");
		*port = 0;
		return -1;
	}

	*port = htons(uliport);
	return 0;
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

// Flips a random bit in the data specified by the pointer data of length data_len
void _flip_random_bit(void * data, const size_t data_len) {
	// Random bit in byte
	size_t e = rand() % 8;
	// Random byte in the data
	size_t r = rand() % data_len;
	// Flip the bit
	((char*)data)[r] ^= 1 << e;
}


// Receive a message on the socket file descriptor sockfd
// The message is provided through data, with a max length of data_len, the actual size (or error) is returned as ssize_t
// Flags flags are passed through to the socket recvfrom call
// Furthermore, a sockaddr should be passed, an uninitialized sockaddr_len can be passed
// in these values the information about the sender is stored
ssize_t recv_message(const int* sockfd, void* data, const size_t data_len, int flags, struct sockaddr_in* sockaddr, socklen_t* sockaddr_len) {
	*sockaddr_len = sizeof(*sockaddr);
	ssize_t msg_len = recvfrom(*sockfd, data, data_len, flags, (struct sockaddr*) sockaddr, sockaddr_len);
	if (msg_len < 0) {
		perror("Failed receiving message");
	} else {
		//perror("Successfully received message");
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

	// Randomly manipulate bit to simulate bad network reliability
	void * temp = malloc(data_len);
	memcpy(temp, data, data_len);

	int r = rand() % 100;
	if (r < BITFLIP_CHANCE) {
		printf("[ _flip_random_bit ] Sent contains a mutation!\n");
		_flip_random_bit(temp, data_len);
	} else {
		//printf("[ _flip_random_bit ] No mutation!\n");
	}


	if ((msg_len = sendto(*sockfd, temp, data_len, flags, (const struct sockaddr*) sockaddr, sizeof(*sockaddr))) < 0) {
		perror("Failed sending message");
	} else {
		//perror("Successfully sent message");
	}

	free(temp);
	return msg_len;
}

// Checks whether the socket file descriptor has data available
// If so, return true, otherwise false
int is_data_available(const int* sockfd) {
	int size;
	int ret = ioctl(*sockfd, FIONREAD, &size);

	if (ret == 0) {
		return size > 0;
	}
	
	return 0;
}

// Print some data with specified length data_len as bytes
void print_bytes(void * data, size_t data_len) {
	printf("Printing %lu bytes from %p.\n", data_len, data);
	printf("--- start bytes ---");
	for (size_t i = 0; i < data_len; i += 1) {
		if (i % 16 == 0) {
			printf("\n");
		}
		printf("%02x ", 0xff & (unsigned int) ((char*)data)[i]);
	}
	printf("\n--- end bytes ---\n");
}

