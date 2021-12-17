/*
	This code implements the peer framework which can be used by
	applications to run a C socket UDP peer to peer network.

	Author: Miguel Blom

*/


#include "common.h"
#include "../config.h"

#ifndef _P2PPEER_
#define _P2PPEER_

// A structure with locking mechanism in which
// information is maintained used to find other peers
struct netinfo_lock {
	struct peer_address** network_info;		// Other peers
	size_t* n_peers;						// Number of other peers
	int* was_offline;						// Flag for whether we were offline
	int* is_online;							// Flag for whether we are online now
	pthread_mutex_t lock;					// The lock for locking this struct
};

struct peer {
	// Tracker info
	int tracker_sockfd;
	struct sockaddr_in tracker_sockaddr;
	// Own info
	int own_sockfd;
	struct peer_address own_pa;
	// Tracker communication info
	struct netinfo_lock netinfo_lock;
	struct peer_address * network_info;
	// Netinfo lock info
	size_t n_peers;
	pthread_t thread_id;
	int was_offline;
	int is_online;
};

// Broadcast a message to all peers in the network info specified by data of size data_len
size_t broadcast(struct peer* p, void * data, size_t data_len);

// Receive in the out variable message with out variable message_len for the length
// The client address from which the message was received is placed in the out variable clntaddr  
void receive(struct peer* p, void ** message, size_t* message_len, struct sockaddr_in* clntaddr);

// Respond with a message of message_len length to a peer with address pa
void respond(struct peer* p, void * message, size_t message_len, struct peer_address* pa);

// Checks whether the peer has been offline, if so, we should refresh
// The flag to check whether we should refresh is reset
int  should_refresh(struct peer* p);

// Initialize the peer by creating its network info, create a socket for the tracker communication
// Another socket is created and bound to for the communication among peers
void init_peer(struct peer* p, char* c_tracker_addr, char* c_tracker_port, char* c_addr, char* c_port);

// The peer exits the network and lets the tracker know
void exit_peer(struct peer* p);

#endif
