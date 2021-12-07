/*
	Author: Miguel Blom

	This code and that in the corresponding .c file set up functionality commonly
	used among the tracker and the peer code. We provide structures for sharing
	information about the location of a peer (connection family, ip and port).
	Furthermore, there are some helper functions for setting up, connecting to
	and receiving data from and sending to sockets.

*/

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>

#include "db.h"

#ifndef COMMON_H_
#define COMMON_H_

const short unsigned int domain;

// ************************ //
// Peer address definitions //
// ************************ //
struct peer_address init_peer_address(short unsigned int family, short unsigned int port, uint32_t addr);

// ************************** //
// Message header definitions //
// ************************** //
enum message_type {
	HEARTBEAT,
	NETINFO,
	//ACKNOWLEDGEMENT,
	//JOIN,
	//NETWORKINFO,
	HELLO
};

struct message_header {
	enum message_type type;
	size_t len;
	uint32_t message_header_checksum;
	uint32_t message_data_checksum;
	
};

struct message_header init_message_header (enum message_type type, size_t len, uint32_t data_checksum);

// **************************** //
// Heartbeat header definitions //
// **************************** //
struct heartbeat_header {
	struct message_header message_header;
	struct peer_address peer_address;
};

struct heartbeat_header init_heartbeat_header (const struct peer_address* pa);

// ******************************* //
// Network information definitions //
// ******************************* //
void* init_network_information (const void * data, size_t* data_len); 


// **************** //
// Global functions //
// **************** //
int convert_port(char* chport, short unsigned int* port);
void initialize_srvr(int* sockfd, const struct peer_address* pa);
void initialize_clnt(int* sockfd, const struct peer_address* pa, struct sockaddr_in* sockaddr);
ssize_t recv_message(const int* sockfd, void* data, const size_t data_len, int flags, struct sockaddr_in* sockaddr, socklen_t* sockaddr_len);
ssize_t send_message(const int* sockfd, const void* data, const size_t data_len, int flags, const struct sockaddr_in* sockaddr);


void print_bytes(void * data, size_t data_len);

#endif // COMMON_H_
