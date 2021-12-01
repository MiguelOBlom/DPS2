#include "common.h"
#include "list.h"

const int domain = AF_INET;
const int port = 8080;
const uint32_t address = INADDR_ANY;

void handle_acknowledgement() {

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

	//printf("init_peer_info.port: %d\n", init_peer_info.port);
	memcpy(peer_info_ptr, &peer_info, sizeof(peer_info));
	//printf("peer_info_ptr->port: %d\n", peer_info_ptr->port);
	list_add_element(peer_infos, join_header.peer_info.port, peer_info_ptr);

	
	elements = list_list_elements(peer_infos, &data_len);

	//printf("first: %p\n", peer_infos->first);
	//printf("first->data: %p\n", peer_infos->first->data);
	//printf("*(first->data): %d\n", *((int*)(peer_infos->first->data)));
	//for (size_t i = 0; i < peer_infos->n_elements; ++i) {
	//	printf("%p: port = %d\n", (elements + i * peer_infos->element_size), ((struct peer_info *)(elements + i * peer_infos->element_size))->port);
	//}

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