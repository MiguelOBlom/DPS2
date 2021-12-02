#include "common.h"
#include <errno.h>

const int domain = AF_INET;
const uint16_t tracker_port = 8080;
const uint32_t address = INADDR_ANY;

void join_network(const int* sockfd, struct sockaddr_in* srvraddr, uint16_t port) {
	void* network_info;
	struct message_header message_header;
	struct join_header join_header = init_join_header(port);
	send_message(sockfd, &join_header, sizeof(join_header), MSG_CONFIRM, srvraddr);
	recv_message(sockfd, &message_header, sizeof(message_header), MSG_WAITALL | MSG_PEEK, srvraddr);
	if (message_header.type != NETWORKINFO) {
		printf("Expected network info, but got something else...\n");
	} else {
		network_info = malloc(message_header.len);
		recv_message(sockfd, network_info, message_header.len, MSG_WAITALL, srvraddr);
		struct peer_info* i = network_info + sizeof(message_header);

		printf("start: %p, end: %p, len:%lu\n", i, (struct peer_info*)(network_info + message_header.len), message_header.len);

		for (; i < (struct peer_info*)(network_info + message_header.len); i ++) {
			printf("%d\n", ((struct peer_info*) i)->port);

			int sockfd_;
			struct sockaddr_in srvraddr_;

			initialize_clnt(&sockfd_, &domain, &((struct peer_info*) i)->port, &address, &srvraddr_);

			struct message_header message_header_ = init_message_header (HELLO, sizeof(message_header_));
			send_message(&sockfd_, &message_header_, sizeof(message_header_), MSG_CONFIRM, &srvraddr_);

		}
	}

}

int convert_port(char* chport, uint16_t* port){
	*port = strtoul(chport, NULL, 10);
	// Check if conversion was successful	
	if (port == 0 && (errno == EINVAL || errno == ERANGE)) {
		perror("Could not convert port");
		return -1;
	}
	return 0;
}


int main(int argc, char ** argv) {
	uint16_t port;
	int tracker_sockfd;
	struct sockaddr_in srvraddr;
	
	printf("%d\n", ntohs(htons(4711)));

	int sockfd;


	if (argc != 2) {
		printf("Usage: %s <port>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if (convert_port(argv[1], &port) < 0) {
		exit(EXIT_FAILURE);
	}

	initialize_clnt(&tracker_sockfd, &domain, &tracker_port, &address, &srvraddr);
	join_network(&tracker_sockfd, &srvraddr, port);

	

	initialize_srvr(&sockfd, &domain, &port, &address);
	
	struct message_header message_header_;
	struct sockaddr_in clntaddr;
	while (1) {
		recv_message(&sockfd, &message_header_, sizeof(message_header_), MSG_WAITALL, &clntaddr);
		printf("Got a message!\n");
	}

	//send_message(&sockfd, buf, &buf_len, MSG_CONFIRM, &srvraddr);
	//wait_message(&sockfd, (void*) buf, &buf_len, MSG_WAITALL, &srvraddr);

	return 0;
}