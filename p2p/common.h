#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>

#ifndef COMMON_H_
#define COMMON_H_

enum message_type {
	ACKNOWLEDGEMENT,
	JOIN
};

struct peer_info {
	int port;
};

struct peer_info init_peer_info(int port);

struct message_header {
	enum message_type type;
	uint32_t header_checksum;
	size_t len;
};

struct message_header init_message_header (enum message_type type, size_t len);

struct join_header {
	struct message_header message_header;
	struct peer_info peer_info;
};

struct join_header init_join_header (int port);

struct data_header {
	uint32_t data_checksum;
};


void initialize_srvr(int* sockfd, const int* domain, const int* port, const uint32_t* address);
void initialize_clnt(int* sockfd, const int* domain, const int* port, const uint32_t* address, struct sockaddr_in* sockaddr);
void recv_message(const int* sockfd, void* data, const size_t data_len, int flags, struct sockaddr_in* sockaddr);
void send_message(const int* sockfd, const void* data, const size_t data_len, int flags, const struct sockaddr_in* sockaddr);


#endif // COMMON_H_