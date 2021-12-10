// rm p2p.db; ./tracker 192.168.178.73 8080 p2p.db

// TODO: If the tracker is adding the peer to the db, it needs 
// a confirmation, from the peer after sending the list, so that
// we know it registered itself as "new"

#include "common.h"


// If no heartbeat is obtained from a peer after 60 seconds, 
// it is considered dead.
const int timeout_threshold = 60; 

/*
void handle_acknowledgement() {

}

*/

int receive_peer_address_header(const int* sockfd, struct peer_address_header* peer_address_header, struct sockaddr_in* clntaddr, socklen_t* clntaddr_len) {
	recv_message(sockfd, (void*) peer_address_header, sizeof(*peer_address_header), MSG_WAITALL, clntaddr, clntaddr_len);
	
	POLY_TYPE crc = peer_address_header->message_header.message_header_checksum;
	peer_address_header->message_header.message_header_checksum = 0;
	if (   check_crc(&peer_address_header->message_header, sizeof(peer_address_header->message_header) - sizeof(peer_address_header->message_header.message_header_checksum), crc)
		&& check_crc(&peer_address_header->peer_address, sizeof(peer_address_header->peer_address), peer_address_header->message_header.message_data_checksum)) {
		return 1;
	}
	return 0;
}

// Removes peer from the database
// may have no effect when messages arrive out-of-order and a heartbeat is received after exit
// however it is still nice to clean up after yourself :)
void handle_exit(const int* sockfd, sqlite3* db) {
	struct peer_address_header peer_address_header;
	struct sockaddr_in clntaddr;
	socklen_t clntaddr_len;
	//recv_message(sockfd, (void*) &peer_address_header, sizeof(peer_address_header), MSG_WAITALL, &clntaddr, &clntaddr_len);
	
	//POLY_TYPE crc = peer_address_header.message_header.message_header_checksum;
	//peer_address_header.message_header.message_header_checksum = 0;
	//if (   check_crc(&peer_address_header.message_header, sizeof(peer_address_header.message_header) - sizeof(peer_address_header.message_header.message_header_checksum), crc)
	//	&& check_crc(&peer_address_header.peer_address, sizeof(peer_address_header.peer_address), peer_address_header.message_header.message_data_checksum)) {
	if (receive_peer_address_header(sockfd, &peer_address_header, &clntaddr, &clntaddr_len)) {
		// if (peer_address_header.message_header.type != EXIT) {
		// 	printf("Not an exit...\n");
		// 	return;
		// }

		//if (db_peer_exists(db, &peer_address_header.peer_address)) {
			db_remove_peer(db, &peer_address_header.peer_address);
		//}
	} else {
		printf("The CRC did not match for your received exit data!\n");
	}

}


void handle_heartbeat(const int* sockfd, sqlite3* db) {
	static int calls = 0;
	calls++;
	printf("%d\n", calls);

	struct peer_address_header peer_address_header;
	struct sockaddr_in clntaddr;
	socklen_t clntaddr_len;

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
	if (receive_peer_address_header(sockfd, &peer_address_header, &clntaddr, &clntaddr_len)) {
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

		/*
		for (size_t j = 0; j < message_len; j++) {
			printf("%02x ",((char*)&message)[j]);
		}
		printf("\n");
		*/
		free(message);
		free(network_information);

		/*
		for (size_t j = 0; j < sizeof(peer_address_header.peer_address); j++) {
			printf("%02x ",((char*)&peer_address_header.peer_address)[j]);
		}
		*/

		// Update the database with this new information
		if (db_peer_exists(db, &peer_address_header.peer_address)) {
			// Only have to update the heartbeat if the peer exists
			db_update_peer_heartbeat(db, &peer_address_header.peer_address);
		} else {
			// Otherwise we need to add it to our list
			//db_insert_peer(db, &peer_address_header.peer_address);
		}
	} else {
		printf("The CRC did not match for your received heartbeat data!\n");
	}
}


void handle_acknowledge_netinfo (const int* sockfd, sqlite3* db) {
	struct peer_address_header peer_address_header;
	struct sockaddr_in clntaddr;
	socklen_t clntaddr_len;
	if (receive_peer_address_header(sockfd, &peer_address_header, &clntaddr, &clntaddr_len)) {
		if (!db_peer_exists(db, &peer_address_header.peer_address)){
			db_insert_peer(db, &peer_address_header.peer_address);
			printf("Insterted!\n");
		}
	} else {
		printf("The CRC did not match for your received acknowledge netinfo data!\n");
	}
}


void handle_requests (const int* sockfd, sqlite3* db) {
	struct message_header header;
	struct sockaddr_in clntaddr;
	socklen_t clntaddr_len;

	while(1) {
		db_remove_outdated_peers(db, timeout_threshold);

		if (!is_data_available(sockfd)) {
			sleep(1);
		} else {
			// Receive a message header
			recv_message(sockfd, (void*) &header, sizeof(header), MSG_WAITALL | MSG_PEEK, &clntaddr, &clntaddr_len);
			printf("type: %d, message_header_checksum: %u, message_data_checksum: %u, len: %lu\n", header.type, header.message_header_checksum, header.message_data_checksum, header.len);

			switch(header.type) {
				case HEARTBEAT:
					handle_heartbeat(sockfd, db);
					break;
				case EXIT:
					handle_exit(sockfd, db);
					break;
				case ACKNOWLEDGENETINFO:
					printf("Handling acknowledgement\n");
					handle_acknowledge_netinfo(sockfd, db);
					break;
				default:
					break;
			}
		}
	}
}


int main(int argc, char ** argv) {
	struct sockaddr_in sockaddr;
	int sockfd;
	struct peer_address pa;
	short unsigned int port;
	uint32_t addr;
	sqlite3 * db;

	if (argc != 4) {
		printf("Usage: %s <addr> <port> <db_name>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	addr = inet_addr(argv[1]);
	if (convert_port(argv[2], &port) < 0) {
		exit(EXIT_FAILURE);
	}
	pa = init_peer_address(domain, port, addr);
	initialize_srvr(&sockfd, &pa);
	db_open(&db, argv[3]);

	handle_requests(&sockfd, db);

	db_close(db);
	

	
	/*
	while (1) {
		wait_message(&sockfd, (void*) buf, &buf_len, MSG_WAITALL | MSG_PEEK, &clntaddr);
		send_message(&sockfd, buf, &buf_len, MSG_CONFIRM, &clntaddr);
	}
	*/

	return 0;
}
