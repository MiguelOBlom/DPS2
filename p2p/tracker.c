#include "common.h"
#include "list.h"

const int domain = AF_INET;
const int port = 8080;
const uint32_t address = INADDR_ANY;

void handle_acknowledgement() {

}

void* init_network_info(void* peer_info_elements, size_t data_len, size_t* network_info_len) {
	*network_info_len = sizeof(struct message_header) + data_len;
	void * network_info = malloc(*network_info_len);
	struct message_header message_header = init_message_header(NETWORKINFO, *network_info_len);
	memcpy(network_info, &message_header, sizeof(message_header));
	memcpy(network_info + sizeof(message_header), peer_info_elements, data_len);
	return network_info;
}

void handle_join(const int * sockfd, struct list * peer_infos) {
	void * elements;
	size_t data_len;
	struct sockaddr_in clntaddr;
	struct join_header join_header;
	struct peer_info peer_info;
	struct peer_info* peer_info_ptr;

	recv_message(sockfd, (void*) &join_header, sizeof(join_header), MSG_WAITALL, &clntaddr);
	printf("Peer wants to join on port: %d\n", join_header.peer_info.port);

	peer_info = init_peer_info(join_header.peer_info.port);
	peer_info_ptr = malloc(sizeof(peer_info));

	memcpy(peer_info_ptr, &peer_info, sizeof(peer_info));
	list_add_element(peer_infos, join_header.peer_info.port, peer_info_ptr);

	elements = list_list_elements(peer_infos, &data_len);

	size_t network_info_len;
	void * network_info = init_network_info(elements, data_len, &network_info_len);
	send_message(sockfd, network_info, network_info_len, MSG_CONFIRM, &clntaddr);

}


void handle_requests (const int* sockfd, struct list* peer_infos) {
	struct message_header header;
	struct sockaddr_in clntaddr;

	while(1) {
		// Receive a message header
		recv_message(sockfd, (void*) &header, sizeof(header), MSG_WAITALL | MSG_PEEK, &clntaddr);
		printf("%d %u %lu\n", header.type, header.header_checksum, header.len);

		switch(header.type) {
			case ACKNOWLEDGEMENT:
				break;
			case JOIN:
				handle_join(sockfd, peer_infos);
				break;
			default:
				break;
		}

	}
}


int main(int argc, char ** argv) {
	int sockfd;
	struct list peer_infos = create_list(sizeof(struct peer_info));

	initialize_srvr(&sockfd, &domain, &port, &address);

	handle_requests(&sockfd, &peer_infos);


	
	/*
	while (1) {
		wait_message(&sockfd, (void*) buf, &buf_len, MSG_WAITALL | MSG_PEEK, &clntaddr);
		send_message(&sockfd, buf, &buf_len, MSG_CONFIRM, &clntaddr);
	}
	*/

	return 0;
}