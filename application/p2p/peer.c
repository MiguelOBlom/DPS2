// ./peer 192.168.178.73 8080 192.168.178.73 1234
#include "peer.h"

struct netinfo_lock init_netinfo_lock(struct peer_address** network_info, size_t * n_peers, int* was_offline) {
	struct netinfo_lock nl;
	nl.network_info = network_info;
	printf("n_peers %p\n", n_peers);
	nl.n_peers = n_peers;
	printf("nl.n_peers %p\n", nl.n_peers);
	nl.was_offline = was_offline;


	if (pthread_mutex_init(&nl.lock, 0) != 0) {
		perror("Cannot initialize mutex lock");
		exit(EXIT_FAILURE);
	}

	return nl;
}


size_t broadcast(struct peer* p, void * data, size_t data_len) {
	size_t n_messages_sent = 0;
	struct netinfo_lock* netinfo_lock = &p->netinfo_lock;
	struct peer_address* own_pa = &p->own_pa;
	POLY_TYPE data_checksum = get_crc(data, data_len);
	struct message_header message_header = init_message_header(P2P, sizeof(struct message_header) + data_len, data_checksum);
	void * message = malloc(message_header.len);
	
	memcpy(message, &message_header, sizeof(struct message_header));

	if (data) {
		memcpy(message, data, data_len);
	}

	int sockfd;
	struct sockaddr_in sockaddr;
	
	if (pthread_mutex_lock(&netinfo_lock->lock) == 0) {
		if (*(netinfo_lock->network_info)) {
			for (size_t i = 0; i < *(netinfo_lock->n_peers); ++i) {
				if(!cmp_peer_address(&(*(netinfo_lock->network_info))[i], own_pa)) {
					initialize_clnt(&sockfd, &(*(netinfo_lock->network_info))[i], &sockaddr);
					send_message(&sockfd, message, message_header.len, MSG_CONFIRM, &sockaddr);	
					++n_messages_sent;
				}
			}
		}
		if (pthread_mutex_unlock(&netinfo_lock->lock) != 0) {
			perror("Error unlocking lock in thread process");
			exit(EXIT_FAILURE);
		}
	} else {
		perror("Error locking lock in thread process");
		exit(EXIT_FAILURE);
	}

	free(message);
	return n_messages_sent;
}


void handle_broadcast(struct peer* p) {
	int* sockfd = &p->own_sockfd;
	// TODO
	//recv_message(sockfd, &header, sizeof(struct message_header), MSG_WAITALL | MSG_PEEK, &clntaddr, &clntaddr_len);
	static unsigned int handled = 0;
	handled++;
	printf("%u\n", handled);
}


void receive(struct peer* p, void ** message, size_t* message_len, struct sockaddr_in* clntaddr) {
	socklen_t clntaddr_len;
	int* sockfd = &p->own_sockfd;
	struct message_header header;

	void * data;
	//struct sockaddr_in clntaddr;
	//socklen_t clntaddr_len;
	if (is_data_available(sockfd)) {
		printf("YAY! Data is available!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		recv_message(sockfd, &header, sizeof(struct message_header), MSG_WAITALL | MSG_PEEK, clntaddr, &clntaddr_len);

		data = malloc(header.len);

		// Return pointer to allocated message data
		recv_message(sockfd, data, header.len, MSG_WAITALL, clntaddr, &clntaddr_len);
		if (header.type == P2P && check_message_crc(data, header.len)){
			*message_len = header.len - sizeof(header);
			*message = malloc(*message_len);
		} else {
			*message_len = 0;
			*message = NULL;
		}
		free(data);
	}
}

void respond(struct peer* p, void * message, size_t message_len, const struct sockaddr_in* clntaddr) {
	POLY_TYPE data_checksum = get_crc(message, message_len);
	struct message_header message_header = init_message_header(P2P, sizeof(message_header) + message_len, data_checksum);
	void * data = malloc(message_header.len);
	memcpy(data, &message_header, sizeof(message_header));
	memcpy(data + sizeof(message_header), message, message_len);
	send_message(&p->own_sockfd, data, message_header.len, MSG_CONFIRM, clntaddr);
	free(data);
}


void send_netinfo (const int* sockfd, struct sockaddr_in* srvraddr, const struct peer_address* pa, enum message_type mtype) {
	struct peer_address_header peer_address_header = init_peer_address_header(pa, mtype);
	send_message(sockfd, &peer_address_header, sizeof(peer_address_header), MSG_CONFIRM, srvraddr);
}


void receive_netinfo(struct netinfo_lock* netinfo_lock, const int* sockfd, struct sockaddr_in* srvraddr) {
	size_t data_len;
	void * message;
	socklen_t srvraddr_len;
	struct message_header message_header;
	recv_message(sockfd, &message_header, sizeof(message_header), MSG_WAITALL | MSG_PEEK, srvraddr, &srvraddr_len);
	message = malloc(message_header.len);

	printf("Receiving netinfo\n");
	recv_message(sockfd, message, message_header.len, MSG_WAITALL, srvraddr, &srvraddr_len);
	print_bytes(message, message_header.len);
	// if (message_header.type != NETINFO) {
	// 	printf("Expected network info, but got something else...\n");
	// } else {
	
		printf("Checking CRC heartbeat!\n");

		POLY_TYPE crc = message_header.message_header_checksum;
		((struct message_header*)message)->message_header_checksum = 0;
		if (   check_crc(message, sizeof(message_header) - sizeof(message_header.message_header_checksum), crc)
			&& check_crc(message + sizeof(message_header), message_header.len - sizeof(message_header), message_header.message_data_checksum)) {
			printf("Checked CRC heartbeat!\n");
			data_len = message_header.len - sizeof(message_header);

			//print_bytes(message, message_header.len );

			//printf("Trylock: %d\n", pthread_mutex_trylock(&netinfo_lock->lock));

			printf("Locking mutex!\n");
			if(pthread_mutex_lock(&netinfo_lock->lock) == 0) {
				printf("Locked mutex!\n");
				if (*(netinfo_lock->network_info)) {
					free(*(netinfo_lock->network_info));
				}
				
				*(netinfo_lock->network_info) = (struct peer_address*) malloc(data_len);
				memcpy(*(netinfo_lock->network_info), message + sizeof(message_header), data_len);
				print_bytes(*(netinfo_lock->network_info), data_len);
				*(netinfo_lock->n_peers) = data_len / sizeof(struct peer_address);
				
				if (pthread_mutex_unlock(&netinfo_lock->lock) != 0) {
					perror("Error unlocking mutex");
					exit(EXIT_FAILURE);
				}

			} else {
				perror("Error while locking the netinfo mutex");
				exit(EXIT_FAILURE);
			}
			printf("Exit critical section!\n");

		} else {
			printf("The CRC did not match for your received netinfo data!\n");
		}
	// }
	free(message);
}


int is_present(const struct netinfo_lock* netinfo_lock, const struct peer_address* own_pa) {
	printf("is_present: %p, %p\n", *(netinfo_lock->network_info), own_pa);
	if (*(netinfo_lock->network_info)) {
		printf("+ is_present: %lu\n", *(netinfo_lock->n_peers));
		print_bytes(*(netinfo_lock->network_info), sizeof(struct peer_address) * *(netinfo_lock->n_peers));
		for (size_t i = 0; i < *(netinfo_lock->n_peers); ++i) {
			print_bytes(&(*(netinfo_lock->network_info))[i], sizeof(struct peer_address));
			if(cmp_peer_address(&(*(netinfo_lock->network_info))[i], own_pa)) {
				return 1;
			}
		}
	}
	return 0;
}


int should_refresh(struct peer* p) {
	int was_offline = p->was_offline;
	p->was_offline = 0;
	return was_offline;
}


void* tracker_communication(void * args) {
	struct netinfo_lock* netinfo_lock = ((struct send_heartbeat_thread_args*)args)->netinfo_lock;
	int* tracker_sockfd = ((struct send_heartbeat_thread_args*)args)->sockfd;
	struct sockaddr_in* tracker_sockaddr = ((struct send_heartbeat_thread_args*)args)->srvraddr;
	struct peer_address* own_pa = ((struct send_heartbeat_thread_args*)args)->pa;

	printf("Thread started!\n");
 	while(1) {
		printf("netinfo_lock inside %p\n", netinfo_lock);
		printf("%p\n", netinfo_lock->n_peers);

		send_netinfo(tracker_sockfd, tracker_sockaddr, own_pa, HEARTBEAT);

		for (int i = 0; i < heartbeat_period; i++) {
			sleep(1);
			while(is_data_available (tracker_sockfd)) {
	 			receive_netinfo(netinfo_lock, tracker_sockfd, tracker_sockaddr);
	 			if (!is_present(netinfo_lock, own_pa)) { // If the peer can't find itself, it means that it has been offline
	 				// send acknowledgement
	 				printf("Sending acknowledgement\n");
	 				send_netinfo(tracker_sockfd, tracker_sockaddr, own_pa, ACKNOWLEDGENETINFO);
	 				
	 				*(netinfo_lock->was_offline) = 1; // We are online now!

	 				i = 0;
	 			}
	 		}

		}

		if (*(netinfo_lock->network_info)) {
			for (size_t i = 0; i < *(netinfo_lock->n_peers); ++i) {
				print_bytes(&(*(netinfo_lock->network_info))[i], sizeof(struct peer_address));	
			}
		}	
	}
}



void init_peer(struct peer* p, char* c_tracker_addr, char* c_tracker_port, char* c_addr, char* c_port) {
	int tracker_sockfd;
	short unsigned int tracker_port;
	uint32_t tracker_addr;
	struct sockaddr_in tracker_sockaddr;
	struct peer_address tracker_pa;

	short unsigned int own_port;
	uint32_t own_addr;

	p->network_info = NULL;
	p->n_peers = 0;
	p->was_offline = 0;

	// Construct tracker peer_address struct
	tracker_addr = inet_addr(c_tracker_addr);
	if (convert_port(c_tracker_port, &tracker_port) < 0) {
		exit(EXIT_FAILURE);
	}
	tracker_pa = init_peer_address(DOMAIN, tracker_port, tracker_addr);

	// Construct peer (own) peer_address struct 
	own_addr = inet_addr(c_addr);
	if (convert_port(c_port, &own_port) < 0) {
		exit(EXIT_FAILURE);
	}
	p->own_pa = init_peer_address(DOMAIN, own_port, own_addr);

	// Set up tracker communication
	// Initialize socket to talk to tracker
	initialize_clnt(&tracker_sockfd, &tracker_pa, &tracker_sockaddr);
	
	// Create a lock for the netinfo_lock information
	p->netinfo_lock = init_netinfo_lock(&p->network_info, &p->n_peers, &p->was_offline);

	// Initialize arguments for send_heartbeat_thread
	struct send_heartbeat_thread_args sht_args;
	sht_args.netinfo_lock = &p->netinfo_lock;
	sht_args.sockfd = &tracker_sockfd;
	sht_args.srvraddr = &tracker_sockaddr;
	sht_args.pa = &p->own_pa;

	// Start the thread that sends a heartbeat periodically to the tracker
	if(pthread_create(&p->thread_id, NULL, tracker_communication, &sht_args) != 0) {
		perror("Could not start pthread");
		exit(EXIT_FAILURE);
	} else {
		printf("Started proc!\n");
	}

	// Set up own port for peer-to-peer communication
	initialize_srvr(&p->own_sockfd, &p->own_pa);
}

void exit_peer(struct peer* p) {
	printf("Exiting...\n");
	if(p->network_info) {
		free(p->network_info);
	}
	pthread_cancel(p->thread_id);
	pthread_join(p->thread_id, NULL);
	pthread_mutex_destroy(&p->netinfo_lock.lock);	
}

/*
int main(int argc, char ** argv) {
	// Struct peer
	struct peer p;

	// Input
	if (argc != 5) {
		printf("Usage: %s <tracker_addr> <tracker_port> <addr> <port>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	// Init
	init_peer(&p, argv[1], argv[2], argv[3], argv[4]);

	broadcast(&p, NULL, 0);
		
	sleep(60);

	//receive(&p);

	// Exit
	exit_peer(&p);

	return 0;
}
*/