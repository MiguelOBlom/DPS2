// ./peer 192.168.178.73 8080 192.168.178.73 1234
#include "common.h"

void broadcast(const struct peer_address* network_info, const struct peer_address* own_pa, size_t n_peers, void * data, size_t data_len) {
	uint32_t data_checksum = 0; // TODO
	struct message_header message_header = init_message_header(BROADCAST, sizeof(struct message_header) + data_len, data_checksum);
	void * message = malloc(message_header.len);
	
	memcpy(message, &message_header, sizeof(struct message_header));

	if (data) {
		memcpy(message, data, data_len);
	}

	int sockfd;
	struct sockaddr_in sockaddr;

	for (size_t i = 0; i < n_peers; ++i) {
		initialize_clnt(&sockfd, &network_info[i], &sockaddr);

		if(!cmp_peer_address(&network_info[i], own_pa)) {
			send_message(&sockfd, message, message_header.len, MSG_CONFIRM, &sockaddr);	
		}
	}
}

void handle_broadcast(const int* sockfd) {
	static unsigned int handled = 0;
	handled++;
	printf("%u\n", handled);
}

void receive(const int* sockfd) {
	struct message_header header;
	struct sockaddr_in clntaddr;
	socklen_t clntaddr_len;
	if (is_data_available(sockfd)) {
		recv_message(sockfd, &header, sizeof(struct message_header), MSG_WAITALL | MSG_PEEK, &clntaddr, &clntaddr_len);
		switch (header.type) {
			case BROADCAST:
				handle_broadcast(sockfd);
				break;

			default:
				break;
		}
	}
}

void send_heartbeat(const int* sockfd, struct sockaddr_in* srvraddr, const struct peer_address* pa, struct peer_address ** network_info, size_t * n_peers) {
	size_t data_len;
	void * message;
	socklen_t srvraddr_len;
	struct message_header message_header;
	struct heartbeat_header heartbeat_header = init_heartbeat_header(pa);
	send_message(sockfd, &heartbeat_header, sizeof(heartbeat_header), MSG_CONFIRM, srvraddr);
	recv_message(sockfd, &message_header, sizeof(message_header), MSG_WAITALL | MSG_PEEK, srvraddr, &srvraddr_len);
	if (message_header.type != NETINFO) {
		printf("Expected network info, but got something else...\n");
	} else {
		message = malloc(message_header.len);
		recv_message(sockfd, message, message_header.len, MSG_WAITALL, srvraddr, &srvraddr_len);

		data_len = message_header.len - sizeof(message_header);

		print_bytes(message, message_header.len );

		if (*network_info) {
			free(*network_info);
		}

		*network_info = malloc(data_len);
		memcpy(*network_info, message + sizeof(message_header), data_len);
		free(message);

		*n_peers = data_len / sizeof(struct peer_address);
		/*for (size_t i = 0; i < *n_peers; ++i) {
			print_bytes(&(*network_info)[i], sizeof(struct peer_address));	
		}*/
	}

}

int is_present(const struct peer_address* network_info, const struct peer_address* own_pa, size_t n_peers) {
	for (size_t i = 0; i < n_peers; ++i) {
		if(cmp_peer_address(&network_info[i], own_pa)) {
			return 1;
		}
	}
	return 0;
}

/*
		for (; i < (struct peer_info*)(network_info + message_header.len); i ++) {
			printf("%d\n", ((struct peer_info*) i)->port);

			int sockfd_;
			struct sockaddr_in srvraddr_;

			initialize_clnt(&sockfd_, &domain, &((struct peer_info*) i)->port, &address, &srvraddr_);

			struct message_header message_header_ = init_message_header (HELLO, sizeof(message_header_));
			send_message(&sockfd_, &message_header_, sizeof(message_header_), MSG_CONFIRM, &srvraddr_);

		}
*/

void* threadproc(void * arg) {
	while(1) {
		sleep(5);
		printf("%p\n", *((struct peer_address**) arg));
	}
}


int main(int argc, char ** argv) {

	struct sockaddr_in tracker_sockaddr;
	int tracker_sockfd;
	struct peer_address tracker_pa;
	short unsigned int tracker_port;
	uint32_t tracker_addr;


	struct peer_address own_pa;
	short unsigned int own_port;
	uint32_t own_addr;

	struct peer_address * network_info = NULL;
	size_t n_peers;

	int own_sockfd;



	if (argc != 5) {
		printf("Usage: %s <tracker_addr> <tracker_port> <addr> <port>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	tracker_addr = inet_addr(argv[1]);
	if (convert_port(argv[2], &tracker_port) < 0) {
		exit(EXIT_FAILURE);
	}
	tracker_pa = init_peer_address(domain, tracker_port, tracker_addr);

	own_addr = inet_addr(argv[3]);
	if (convert_port(argv[4], &own_port) < 0) {
		exit(EXIT_FAILURE);
	}
	own_pa = init_peer_address(domain, own_port, own_addr);

	initialize_clnt(&tracker_sockfd, &tracker_pa, &tracker_sockaddr);

	printf("family: %d, port: %d, addr: %d\n", own_pa.family, own_pa.port, own_pa.addr);


	send_heartbeat(&tracker_sockfd, &tracker_sockaddr, &own_pa, &network_info, &n_peers);


	/*
	for (size_t i = 0; i < n_peers; i++) {
		int sockfd;
		struct sockaddr_in sa;
		initialize_clnt(&sockfd, &network_info[i], &sa);
		struct message_header mh = init_message_header(HELLO, sizeof(struct message_header), 0);
		send_message(&sockfd, &mh, sizeof(struct message_header), MSG_CONFIRM, &sa);	 
	}


	initialize_srvr(&own_sockfd, &own_pa);
	*/
	initialize_srvr(&own_sockfd, &own_pa);


/*
	while(1) {
		broadcast(network_info, &own_pa, n_peers, NULL, 0);
		receive(&own_sockfd);
		send_heartbeat(&tracker_sockfd, &tracker_sockaddr, &own_pa, &network_info, &n_peers);
		sleep(2);
	}
	free(network_info);
*/
	


	
//	join_network(&tracker_sockfd, &srvraddr, port, address);

/*
	Stress testing example
	for (int i = 0; i < 1000; i++) {
		send_heartbeat(&tracker_sockfd, &tracker_sockaddr, &own_pa);
	}
*/

	
/*
	initialize_srvr(&sockfd, &domain, &port, &address);
	
	struct message_header message_header_;
	struct sockaddr_in clntaddr;
	while (1) {
		recv_message(&sockfd, &message_header_, sizeof(message_header_), MSG_WAITALL, &clntaddr);
		printf("Got a message!\n");
	}
*/
	//send_message(&sockfd, buf, &buf_len, MSG_CONFIRM, &srvraddr);
	//wait_message(&sockfd, (void*) buf, &buf_len, MSG_WAITALL, &srvraddr);

	return 0;
}
