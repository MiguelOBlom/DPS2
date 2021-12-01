#include "common.h"

const int domain = AF_INET;
const int port = 8080;
const uint32_t address = INADDR_ANY;

void join_network(const int* sockfd, struct sockaddr_in* srvraddr, int port) {
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
		}


	}

}



int main(int argc, char ** argv) {
	int sockfd;
	struct sockaddr_in srvraddr;
	
	initialize_clnt(&sockfd, &domain, &port, &address, &srvraddr);
	join_network(&sockfd, &srvraddr, port);


	//send_message(&sockfd, buf, &buf_len, MSG_CONFIRM, &srvraddr);
	//wait_message(&sockfd, (void*) buf, &buf_len, MSG_WAITALL, &srvraddr);

	return 0;
}