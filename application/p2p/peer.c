// Author: Miguel Blom

#include "peer.h"

// Initializes the netinfo lock with the addresses of the variables corresponding to the members of the struct
struct netinfo_lock init_netinfo_lock(struct peer_address** network_info, size_t * n_peers, int* was_offline, int* is_online) {
	struct netinfo_lock nl;
	nl.network_info = network_info;
	nl.n_peers = n_peers;
	nl.was_offline = was_offline;
	nl.is_online = is_online;

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
	int sockfd;
	struct sockaddr_in sockaddr;
	// Message to be sent
	struct message_header message_header = init_message_header(P2P, sizeof(struct message_header) + data_len, data_checksum);
	void * message = malloc(message_header.len);
	
	// Copy the message into 
	memcpy(message, &message_header, sizeof(struct message_header));

	// If we are sending data as well, copy it
	if (data) {
		memcpy(message + sizeof(struct message_header), data, data_len);
	}

	// Lock the network info
	if (pthread_mutex_lock(&netinfo_lock->lock) == 0) {
		// If there is network info
		if (*(netinfo_lock->network_info)) {
			// For every peer in the network info
			for (size_t i = 0; i < *(netinfo_lock->n_peers); ++i) {
				// If this is not us (current peer)
				if(!cmp_peer_address(&(*(netinfo_lock->network_info))[i], own_pa)) {
					// Send the message
					printf("Broadcast to %d, %u, %u\n", (*(netinfo_lock->network_info))[i].family, (*(netinfo_lock->network_info))[i].port, (*(netinfo_lock->network_info))[i].addr);
					initialize_clnt(&sockfd, &(*(netinfo_lock->network_info))[i], &sockaddr);
					send_message(&sockfd, message, message_header.len, MSG_CONFIRM, &sockaddr);	
					++n_messages_sent;
				}
			}
		}

		// Unlock the lock
		if (pthread_mutex_unlock(&netinfo_lock->lock) != 0) {
			perror("Error unlocking lock in thread process");
			exit(EXIT_FAILURE);
		}

	} else {
		perror("Error locking lock in thread process");
		exit(EXIT_FAILURE);
	}

	// Cean up and return the number of peers the message is sent to
	free(message);
	return n_messages_sent;
}


void receive(struct peer* p, void ** message, size_t* message_len, struct sockaddr_in* clntaddr) {
	socklen_t clntaddr_len;
	int* sockfd = &p->own_sockfd;
	struct message_header header;

	void * data;

	// If there is something to receive
	if (is_data_available(sockfd)) {
		// Peek and check the crc of the header
		recv_message(sockfd, &header, sizeof(struct message_header), MSG_WAITALL | MSG_PEEK, clntaddr, &clntaddr_len);
		if (check_message_crc(&header, sizeof(header))) {

			// Receive the data in a buffer
			data = malloc(header.len);
			recv_message(sockfd, data, header.len, MSG_WAITALL, clntaddr, &clntaddr_len);

			// Check the crc of the message
			if (check_message_crc(data, header.len) && header.type == P2P){
				*message_len = header.len - sizeof(header);
				*message = malloc(*message_len);
				memcpy(*message, data + sizeof(struct message_header), *message_len);
			} else {
				*message_len = 0;
				*message = NULL;
			}
			// Clean up
			free(data);
		} else {
			printf("The CRC did not match for the sneakpeek of the netinfo data!\n");
			// Skip the message
			recv_message(sockfd, &header, sizeof(struct message_header), MSG_WAITALL, clntaddr, &clntaddr_len);
		}

	} else {
		*message_len = 0;
		*message = NULL;
	}
}

void respond(void * message, size_t message_len, struct peer_address* pa) {
	int sockfd;
	struct sockaddr_in clntaddr;
	POLY_TYPE data_checksum = get_crc(message, message_len);
	struct message_header message_header = init_message_header(P2P, sizeof(message_header) + message_len, data_checksum);
	void * data = malloc(message_header.len);
	memcpy(data, &message_header, sizeof(message_header));
	memcpy(data + sizeof(message_header), message, message_len);
	
	// Send the message to the client from the peer_address information pa
	initialize_clnt(&sockfd, pa, &clntaddr);
	send_message(&sockfd, data, message_header.len, MSG_CONFIRM, &clntaddr);
	free(data);
}

// Send network information to the tracker
void send_netinfo (const int* sockfd, struct sockaddr_in* srvraddr, const struct peer_address* pa, enum message_type mtype) {
	struct peer_address_header peer_address_header = init_peer_address_header(pa, mtype);
	send_message(sockfd, &peer_address_header, sizeof(peer_address_header), MSG_CONFIRM, srvraddr);
}

// Receive network information from the tracker
void receive_netinfo(struct netinfo_lock* netinfo_lock, const int* sockfd, struct sockaddr_in* srvraddr) {
	size_t data_len;
	void * message;
	socklen_t srvraddr_len;
	struct message_header message_header;

	// Peek at the message and check the header
	recv_message(sockfd, &message_header, sizeof(message_header), MSG_WAITALL | MSG_PEEK, srvraddr, &srvraddr_len);
	if (check_message_crc(&message_header, sizeof(message_header)) && message_header.type == NETINFO) {
		
		// Receive the message in a buffer
		message = malloc(message_header.len);
		recv_message(sockfd, message, message_header.len, MSG_WAITALL, srvraddr, &srvraddr_len);
		if(check_message_crc(message, message_header.len)) {

			// Copy the network information to our own local data
			data_len = message_header.len - sizeof(message_header);
			if(pthread_mutex_lock(&netinfo_lock->lock) == 0) {
				if (*(netinfo_lock->network_info)) {
					free(*(netinfo_lock->network_info));
				}
				
				*(netinfo_lock->network_info) = (struct peer_address*) malloc(data_len);
				memcpy(*(netinfo_lock->network_info), message + sizeof(message_header), data_len);
				*(netinfo_lock->n_peers) = data_len / sizeof(struct peer_address);
				
				if (pthread_mutex_unlock(&netinfo_lock->lock) != 0) {
					perror("Error unlocking mutex");
					exit(EXIT_FAILURE);
				}

			} else {
				perror("Error while locking the netinfo mutex");
				exit(EXIT_FAILURE);
			}
		} else {
			printf("The CRC did not match for your received netinfo data!\n");
		}

		// Clean up
		free(message);
	} else {
		printf("The CRC did not match for the sneakpeek of the netinfo data!\n");
		// Skip the message
		recv_message(sockfd, &message_header, sizeof(message_header), MSG_WAITALL, srvraddr, &srvraddr_len);
	}

	printf("[ receive_netinfo ] %lu peers online!\n", *(netinfo_lock->n_peers));
}


int is_present(const struct netinfo_lock* netinfo_lock, const struct peer_address* own_pa) {
	if (*(netinfo_lock->network_info)) {
		// Check if we find ourselves in the network information
		for (size_t i = 0; i < *(netinfo_lock->n_peers); ++i) {
			if(cmp_peer_address(&(*(netinfo_lock->network_info))[i], own_pa)) {
				return 1;
			}
		}
	}
	return 0;
}


int should_refresh(struct peer* p) {
	// If we have been offline, we should refresh
	int was_offline = p->was_offline; 
	p->was_offline = 0; // Reset the flag
	return was_offline;
}


void* tracker_communication(void * args) {
	struct peer* p = (struct peer*) args;

	printf("Thread started!\n");
 	while(1) {
 		// Keep sending a heartbeat to the tracker
		send_netinfo(&p->tracker_sockfd, &p->tracker_sockaddr, &p->own_pa, HEARTBEAT);

		// While we wait for sending the next heartbeat
		for (int i = 0; i < HEARTBEAT_PERIOD; i++) {
			sleep(1);

			// Check whether we receive new network information 
			while(is_data_available (&p->tracker_sockfd)) {
	 			receive_netinfo(&p->netinfo_lock, &p->tracker_sockfd, &p->tracker_sockaddr);
	 			// If the peer can not find itself, it means that it has been offline
	 			if (!is_present(&p->netinfo_lock, &p->own_pa)) { 
	 				print_bytes(*(p->netinfo_lock.network_info), *(p->netinfo_lock.n_peers) * sizeof(struct peer_address));
	 				
	 				// We are offline now
	 				*(p->netinfo_lock.is_online) = 0;

	 				// Send an acknowledgement so that we get registered again
	 				send_netinfo(&p->tracker_sockfd, &p->tracker_sockaddr, &p->own_pa, ACKNOWLEDGENETINFO);
	 				
	 				// We registered that we were offline!
	 				*(p->netinfo_lock.was_offline) = 1; 

	 				i = 0;
	 			} else {
	 				printf("We are online...\n");
	 				*(p->netinfo_lock.is_online) = 1;
	 			}
	 		}

		}
	}
}



void init_peer(struct peer* p, char* c_tracker_addr, char* c_tracker_port, char* c_addr, char* c_port) {
	short unsigned int tracker_port;
	uint32_t tracker_addr;
	struct peer_address tracker_pa;

	short unsigned int own_port;
	uint32_t own_addr;

	p->network_info = NULL;
	p->n_peers = 0;
	p->was_offline = 1;
	p->is_online = 0;

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
	initialize_clnt(&p->tracker_sockfd, &tracker_pa, &p->tracker_sockaddr);

	// Create a lock for the netinfo_lock information
	p->netinfo_lock = init_netinfo_lock(&p->network_info, &p->n_peers, &p->was_offline, &p->is_online);

	// Start the thread that sends a heartbeat periodically to the tracker
	if(pthread_create(&p->thread_id, NULL, tracker_communication, p) != 0) {
		perror("Could not start pthread");
		exit(EXIT_FAILURE);
	} else {
		printf("Started proc!\n");
	}

	// Set up own port for peer-to-peer communication
	initialize_srvr(&p->own_sockfd, &p->own_pa);
}

void exit_peer(struct peer* p) {
	// Clean up data
	printf("Exiting...\n");
	if(p->network_info) {
		free(p->network_info);
	}

	// Clean up threads
	pthread_cancel(p->thread_id);
	pthread_join(p->thread_id, NULL);
	pthread_mutex_destroy(&p->netinfo_lock.lock);	
}


// By uncommenting this code and building the peer, we can run it using
// ./peer <TRACKER_IP> <TRACKER_PORT> <OWN_IP> <OWN_PORT>
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

	// Exit
	exit_peer(&p);

	return 0;
}
*/
