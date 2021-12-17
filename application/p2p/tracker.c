/*
	This code implements the tracker, which is used by the peers to find others in
	the network.

	To call it, use:

	rm p2p.db; ./tracker <TRACKER_IP> <TRACKER_PORT> p2p.db

	Author: Miguel Blom

*/


#include "common.h"
#include "../config.h"
// If no heartbeat is obtained from a peer after 60 seconds, 
// it is considered dead.


// Removes peer from the database
// may have no effect when messages arrive out-of-order and a heartbeat is received after exit
// however it is still nice to clean up after yourself :)
void handle_exit(sqlite3* db, struct queue_item* queue_item) {
	struct peer_address_header* peer_address_header = queue_item->message;
	db_remove_peer(db, &peer_address_header->peer_address);
}

// The peer wants to remind us it is online and receive a list of network information about
// the whereabouts of other peers
void handle_heartbeat(const int* sockfd, sqlite3* db, struct queue_item* queue_item) {
	struct peer_address_header* peer_address_header = queue_item->message;
	struct sockaddr_in clntaddr = queue_item->clntaddr;

	struct peer_address * network_information;
	size_t n_peers;

	void * message;
	size_t message_len;

	// Send back a list of peer_address objects and print useful information
	db_get_all_peer_addresses(db, &network_information, &n_peers, TIMEOUT_THRESHOLD);

	for (size_t i = 0; i < n_peers; ++i) {
		printf("family: %d, port: %d, addr: %d\n", network_information[i].family, network_information[i].port, network_information[i].addr);
	}

	printf("There are %lu peers online!\n", n_peers);
	message_len = n_peers * sizeof(struct peer_address);
	printf("Sending netinfo\n");
	message = init_network_information(network_information, &message_len);

	print_bytes(message, message_len);
	send_message(sockfd, message, message_len, MSG_CONFIRM, &clntaddr);


	// Clean up
	free(message);
	free(network_information);

	// Update the database with this new information
	if (db_peer_exists(db, &peer_address_header->peer_address)) {
		// Only have to update the heartbeat if the peer exists
		db_update_peer_heartbeat(db, &peer_address_header->peer_address);
	}

}

// The peer wants to register as online
void handle_acknowledge_netinfo (sqlite3* db, struct queue_item* queue_item) {
	struct peer_address_header* peer_address_header = queue_item->message;

	// If the peer does not exist, insert it, otherwise update the heartbeat
	if (!db_peer_exists(db, &peer_address_header->peer_address)){
		db_insert_peer(db, &peer_address_header->peer_address);
		printf("Insterted!\n");
	} else {
		db_update_peer_heartbeat(db, &peer_address_header->peer_address);
	}

}

// Arguments for calling the handle_request_thread
struct handle_request_thread_args {
	const int * sockfd;
	sqlite3* db;
	struct queue_item* queue_item;
};

void* handle_request_thread (void * args) {
	const int* sockfd = ((struct handle_request_thread_args*)args)->sockfd;
	sqlite3* db = ((struct handle_request_thread_args*)args)->db;
	struct queue_item* queue_item = ((struct handle_request_thread_args*)args)->queue_item;
	
	// Switch for the message type
	switch(((struct message_header*)queue_item->message)->type){
		case HEARTBEAT:
			printf("handle_heartbeat(sockfd, db);\n");
			handle_heartbeat(sockfd, db, queue_item);
			break;
		case EXIT:
			printf("handle_exit(sockfd, db);\n");
			handle_exit(db, queue_item);
			break;
		case ACKNOWLEDGENETINFO:
			printf("handle_acknowledge_netinfo(sockfd, db);\n");
			handle_acknowledge_netinfo(db, queue_item);
			break;
		default:
			break;
	}
	// Clean up the data and the thread
	free(args);
	free(queue_item->message);
	free(queue_item);
	pthread_detach(pthread_self());
	return NULL;
}

// Loop for starting threads for handling requests
void handle_requests (const int* sockfd, sqlite3* db, struct queue_lock* ql) {
	pthread_t handle_request_thread_id;
	struct queue_item* queue_item;
	struct handle_request_thread_args* hrt_args;
	while (1){
		// Wait if there are no messages
		if(queue_is_empty(ql)) {
			sleep(1);
		} else {
			// Otherwise take the item off of the queue and start a thread for handling it
			queue_item = queue_dequeue(ql);
			hrt_args = malloc(sizeof(struct handle_request_thread_args));
			hrt_args->sockfd = sockfd;
			hrt_args->db = db;
			hrt_args->queue_item = queue_item;
			// Start seperate process to handle the data
			if(pthread_create(&handle_request_thread_id, NULL, handle_request_thread, hrt_args) != 0) {
				perror("Could not start mailbox pthread");
				exit(EXIT_FAILURE);
			} else {
				printf("Started proc!\n");
			}

		}
	}
}

// Arguments to handle_mailbox
struct handle_mailbox_args {
	struct queue_lock* ql;
	int * sockfd;
};

// Will read messages from the file descriptor and place them in the queue
void* handle_mailbox(void * args) {
	struct queue_lock* ql = ((struct handle_mailbox_args*)args)->ql;
	int* sockfd = ((struct handle_mailbox_args*)args)->sockfd;
	struct message_header header;
	struct sockaddr_in clntaddr;
	socklen_t clntaddr_len;
	struct queue_item* queue_item;
	int found;

	while(1) {
		// Wait if no data is available
		if (!is_data_available(sockfd)) {
			sleep(1);
		} else {
			// Peek at the message and check integrity
			recv_message(sockfd, (void*) &header, sizeof(header), MSG_WAITALL | MSG_PEEK, &clntaddr, &clntaddr_len);
			if (check_message_crc(&header, sizeof(header))) {
				
				// Allocate queue item of size (sizeof(void*) + sizeof(socklen_t) + socklen_t)
				// We do so, to take into account the variable length of the sockaddr_in
				queue_item = malloc(sizeof(void*) + sizeof(socklen_t) + clntaddr_len);
				
				// Allocate buffer for the whole message
				queue_item->message = malloc(header.len);
				
				// Actually receive the message and check the CRC
				recv_message(sockfd, queue_item->message, header.len, MSG_WAITALL, &queue_item->clntaddr, &queue_item->clntaddr_len);
				if (check_message_crc(queue_item->message, header.len)) {

					// Check for spamming heartbeat
					if (header.type == HEARTBEAT) {

						// Lock the queue and place 
						if(pthread_mutex_lock(&ql->lock) == 0) {
							found = 0;

							// Check if the peer address has already been found in the queue
							for(size_t i = 1; i <= ql->size; ++i) {
								struct peer_address_header* other = ((struct queue_item*)(ql->data[(ql->top - i + ql->max_size) % ql->max_size]))->message;
								if(other->message_header.type == HEARTBEAT && cmp_peer_address(&other->peer_address, &((struct peer_address_header*)(queue_item->message))->peer_address )) {
									printf("Found!\n");
									found = 1;
									break;
								}
							}

							// Unlock the queue
							if (pthread_mutex_unlock(&ql->lock) != 0) {
								perror("Error unlocking queue mutex");
								exit(EXIT_FAILURE);
							}

						} else {
							perror("Error while locking queue mutex");
							exit(EXIT_FAILURE);
						}
					}

					// Push queue item to queue if it does not create a 
					// duplicate heartbeat item 
					if (!found) {
						if (!queue_is_full(ql)) {
							queue_enqueue(ql, queue_item);
						} else {
							// Discard message if the queue is full
							printf("Queue is full!\n");		
							free(queue_item->message);
							free(queue_item);
						}
					} else {
						printf("Someone is spamming a heartbeat!\n");
					}
				} else{
					free(queue_item->message);
					free(queue_item);
					printf("The CRC did not match for the received data!\n");
				}
			} else {
				// If the CRC failed, skip the message
				queue_item = malloc(sizeof(void*) + sizeof(socklen_t) + clntaddr_len);
				queue_item->message = malloc(sizeof(struct message_header));
				recv_message(sockfd, queue_item->message, sizeof(struct message_header), MSG_WAITALL, &queue_item->clntaddr, &queue_item->clntaddr_len);
				free(queue_item->message);
				free(queue_item);
				printf("The CRC did not match for the received data!\n");
			}
		}
	}
}


int main(int argc, char ** argv) {
	int sockfd;
	struct peer_address pa;
	short unsigned int port;
	uint32_t addr;
	sqlite3 * db;
	pthread_t receive_thread_id;
	struct queue_lock ql;
	struct handle_mailbox_args hm_args;

	if (argc != 4) {
		printf("Usage: %s <addr> <port> <db_name>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	// Process address and port, create address struct from this
	addr = inet_addr(argv[1]);
	if (convert_port(argv[2], &port) < 0) {
		exit(EXIT_FAILURE);
	}

	pa = init_peer_address(DOMAIN, port, addr);
	
	// Create and bind to a socket
	initialize_srvr(&sockfd, &pa);

	// Open our database
	db_open(&db, argv[3]);

	// Start thread to receive messages
	queue_init(&ql, TRACKER_QUEUE_SIZE);

	hm_args.ql = &ql;
	hm_args.sockfd = &sockfd;
	printf("&sockfd: %p\n", &sockfd);
	printf("hm_args.sockfd : %p\n", hm_args.sockfd);

	if(pthread_create(&receive_thread_id, NULL, handle_mailbox, &hm_args) != 0) {
		perror("Could not start mailbox pthread");
		exit(EXIT_FAILURE);
	} else {
		printf("Started proc!\n");
	}

	// Start threads to handle messages
	handle_requests(&sockfd, db, &ql);

	// Clean up the queue items
	void * tmp;
	while((tmp = queue_dequeue(&ql))) {
		free(((struct queue_item*)tmp)->message);
		free(tmp);
	}

	// Clean up the mailbox thread, queue and database
	pthread_cancel(receive_thread_id);
	queue_delete(&ql);
	db_close(db);
	pthread_join(receive_thread_id, NULL);

	return 0;
}
