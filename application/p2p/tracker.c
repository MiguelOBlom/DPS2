// rm p2p.db; ./tracker 192.168.178.73 8080 p2p.db

#include "common.h"

// If no heartbeat is obtained from a peer after 60 seconds, 
// it is considered dead.
const int timeout_threshold = 60; 
const int tracker_queue_maxsize = 20;



// Removes peer from the database
// may have no effect when messages arrive out-of-order and a heartbeat is received after exit
// however it is still nice to clean up after yourself :)
void handle_exit(sqlite3* db, struct queue_item* queue_item) {
	struct peer_address_header* peer_address_header = queue_item->message;

	if (check_message_crc(peer_address_header, sizeof(struct peer_address_header))) {
		db_remove_peer(db, &peer_address_header->peer_address);
	} else {
		printf("The CRC did not match for your received exit data!\n");
	}
}


void handle_heartbeat(const int* sockfd, sqlite3* db, struct queue_item* queue_item) {
	static int calls = 0;
	calls++;
	printf("%d\n", calls);

	struct peer_address_header* peer_address_header = queue_item->message;
	struct sockaddr_in clntaddr = queue_item->clntaddr;

	struct peer_address * network_information;
	size_t n_peers;

	void * message;
	size_t message_len;

	// Receive the message
	//recv_message(sockfd, (void*) &peer_address_header, sizeof(peer_address_header), MSG_WAITALL, &clntaddr, &clntaddr_len);
	//printf("Checking CRC heartbeat!\n");
	//POLY_TYPE crc = peer_address_header.message_header.message_header_checksum;
	//peer_address_header.message_header.message_header_checksum = 0;
	//if (   check_crc(&peer_address_header.message_header, sizeof(peer_address_header.message_header) - sizeof(peer_address_header.message_header.message_header_checksum), crc)
	//	&& check_crc(&peer_address_header.peer_address, sizeof(peer_address_header.peer_address), peer_address_header.message_header.message_data_checksum)) {
	if (check_message_crc(peer_address_header, sizeof(struct peer_address_header))) {
		printf("Checked CRC heartbeat!\n");
		// Check checksum
		// if (peer_address_header.message_header.type != HEARTBEAT) {
		// 	printf("Not a heartbeat...\n");
		// 	return;
		// }

		// The order of updating the peer and adding it to our database is important.
		// If the server gets overloaded, then heartbeats may not be registered in time.
		// Effectively leading to clients being "kicked" out of the list of alive peers.
		// Thus, first kicking all peers, then when a heartbeat is registered of a peer
		// which is not in the network, and sending the list of known peers prior to adding
		// it to the list, would show that it had been offline or joins as a new peer.
		// The latter two are mainly the same when it comes to reliability of the data
		// of the peer, it is not considered up-to-date anymore.

		// We could use ioctl to check whether there are any messages and THEN kicking
		// dead peers from the list of known alive peers, however, this would only
		// move the problem. If a heartbeat would appear just when we are removing
		// the corresponding peer, it would have to be registered again.
		// Any changes in the data of the peers (e.g. the blockchain) would then
		// not get sent to the kicked peer by the others in the meantime.

		// When a peer is not up-to-date, it has to retrieve the data from all other 
		// peers, by a consensus algorithm. However, overloading the server too much 
		// could leave only a single peer, if it is spamming heartbeats.
		// This means that it has full regime over the state of the blockchain.
		// Thus, the reliability of the data is dependent on the throughput of the tracker.

		// Send back a list of peer_address objects
		db_get_all_peer_addresses(db, &network_information, &n_peers);

		for (size_t i = 0; i < n_peers; ++i) {
			printf("family: %d, port: %d, addr: %d\n", network_information[i].family, network_information[i].port, network_information[i].addr);
		}

		printf("There are %lu peers online!\n", n_peers);
		message_len = n_peers * sizeof(struct peer_address);
		printf("Sending netinfo\n");
		message = init_network_information(network_information, &message_len);

		print_bytes(message, message_len);
		send_message(sockfd, message, message_len, MSG_CONFIRM, &clntaddr);

		
		// for (size_t j = 0; j < message_len; j++) {
		// 	printf("%02x ",((char*)&message)[j]);
		// }
		// printf("\n");
		
		free(message);
		free(network_information);

		
		// for (size_t j = 0; j < sizeof(peer_address_header.peer_address); j++) {
		// 	printf("%02x ",((char*)&peer_address_header.peer_address)[j]);
		// }
		

		// Update the database with this new information
		if (db_peer_exists(db, &peer_address_header->peer_address)) {
			// Only have to update the heartbeat if the peer exists
			db_update_peer_heartbeat(db, &peer_address_header->peer_address);
		} else {
			// Otherwise we need to add it to our list
			//db_insert_peer(db, &peer_address_header.peer_address);
		}
	} else {
		printf("The CRC did not match for your received heartbeat data!\n");
	}
}


void handle_acknowledge_netinfo (sqlite3* db, struct queue_item* queue_item) {
	struct peer_address_header* peer_address_header = queue_item->message;
	// struct sockaddr_in clntaddr;
	// socklen_t clntaddr_len;
	//if (receive_peer_address_header(sockfd, &peer_address_header, &clntaddr, &clntaddr_len)) {
	if (check_message_crc(peer_address_header, sizeof(struct peer_address_header))) {
		if (!db_peer_exists(db, &peer_address_header->peer_address)){
			db_insert_peer(db, &peer_address_header->peer_address);
			printf("Insterted!\n");
		}
	} else {
		printf("The CRC did not match for your received acknowledge netinfo data!\n");
	}
}

struct handle_request_thread_args {
	const int * sockfd;
	sqlite3* db;
	struct queue_item* queue_item;
};

void* handle_request_thread (void * args) {
	const int* sockfd = ((struct handle_request_thread_args*)args)->sockfd;
	sqlite3* db = ((struct handle_request_thread_args*)args)->db;
	struct queue_item* queue_item = ((struct handle_request_thread_args*)args)->queue_item;
	
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
			//printf("Handling acknowledgement\n");
			printf("handle_acknowledge_netinfo(sockfd, db);\n");
			handle_acknowledge_netinfo(db, queue_item);
			break;
		default:
			break;
	}
	free(args);
	free(queue_item->message);
	free(queue_item);
	pthread_detach(pthread_self());
	return NULL;
	//free args, message, queueitem
}

void handle_requests (const int* sockfd, sqlite3* db, struct queue_lock* ql) {
	//struct message_header header;
	//struct sockaddr_in clntaddr;
	//socklen_t clntaddr_len;
	pthread_t handle_request_thread_id;
	struct queue_item* queue_item;
	struct handle_request_thread_args* hrt_args;
	while (1){
		if(queue_is_empty(ql)) {
			sleep(1);
		} else {
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


struct handle_mailbox_args {
	struct queue_lock* ql;
	int * sockfd;
};

void* handle_mailbox(void * args) {
	struct queue_lock* ql = ((struct handle_mailbox_args*)args)->ql;
	int* sockfd = ((struct handle_mailbox_args*)args)->sockfd;

	printf("sockfd: %p\n", sockfd);
	struct message_header header;
	struct sockaddr_in clntaddr;
	socklen_t clntaddr_len;
	struct queue_item* queue_item;
	int found;
	//struct peer_address* pa;

	while(1) {
		if (!is_data_available(sockfd)) {
			sleep(1);
		} else {
			recv_message(sockfd, (void*) &header, sizeof(header), MSG_WAITALL | MSG_PEEK, &clntaddr, &clntaddr_len);
			// Allocate queue item of size (sizeof(void*) + sizeof(socklen_t) + socklen_t)
			// We do so, to take into account the variable length of the sockaddr_in
			queue_item = malloc(sizeof(void*) + sizeof(socklen_t) + clntaddr_len);
			// Allocate buffer for the whole message
			queue_item->message = malloc(header.len);
			recv_message(sockfd, queue_item->message, header.len, MSG_WAITALL, &queue_item->clntaddr, &queue_item->clntaddr_len);
			
			// Check for spamming heartbeat
			if (header.type == HEARTBEAT) {
				if(pthread_mutex_lock(&ql->lock) == 0) {
					found = 0;
					for(size_t i = 1; i <= ql->size; ++i) {
						//printf("own: %p, other: %p\n", own_pa, &((struct peer_address_header*)(queue_item->message))->peer_address);
						struct peer_address_header* other = ((struct queue_item*)(ql->data[(ql->top - i + ql->max_size) % ql->max_size]))->message;
						if(other->message_header.type == HEARTBEAT && cmp_peer_address(&other->peer_address, &((struct peer_address_header*)(queue_item->message))->peer_address )) {
							printf("Found!\n");
							found = 1;
							break;
						}
					}

					if (pthread_mutex_unlock(&ql->lock) != 0) {
						perror("Error unlocking queue mutex");
						exit(EXIT_FAILURE);
					}

				} else {
					perror("Error while locking queue mutex");
					exit(EXIT_FAILURE);
				}
			}

			// Push queue item to queue
			if (!found) {
				if (!queue_is_full(ql)) {
					queue_enqueue(ql, queue_item);
				} else {
					printf("Queue is full!\n");		
					free(queue_item->message);
					free(queue_item);
				}
			} else {
				printf("Someone is spamming a heartbeat!!!!!!!!!!\n");
			}
		}
	}
}


int main(int argc, char ** argv) {
	//struct sockaddr_in sockaddr;
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

	addr = inet_addr(argv[1]);
	//printf("ADDR: %u\n", addr);
	if (convert_port(argv[2], &port) < 0) {
		exit(EXIT_FAILURE);
	}
	//printf("PORT: %u\n", port);
	pa = init_peer_address(DOMAIN, port, addr);
	//printf("ADDR: %d, %u, %u\n", domain, port, addr);
	initialize_srvr(&sockfd, &pa);
	//return 0;
	db_open(&db, argv[3]);

	queue_init(&ql, tracker_queue_maxsize);

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

	handle_requests(&sockfd, db, &ql);

	void * tmp;
	while((tmp = queue_dequeue(&ql))) {
		free(((struct queue_item*)tmp)->message);
		free(tmp);
	}

	pthread_cancel(receive_thread_id);
	queue_delete(&ql);
	db_close(db);
	pthread_join(receive_thread_id, NULL);

	
	/*
	while (1) {
		wait_message(&sockfd, (void*) buf, &buf_len, MSG_WAITALL | MSG_PEEK, &clntaddr);
		send_message(&sockfd, buf, &buf_len, MSG_CONFIRM, &clntaddr);
	}
	*/

	return 0;
}
