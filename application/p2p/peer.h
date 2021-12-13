#include "common.h"

#ifndef _P2PPEER_
#define _P2PPEER_

//#ifdef __cplusplus
//extern "C"{
//#endif 

const int heartbeat_period = 5;

struct netinfo_lock {
	struct peer_address** network_info;
	size_t* n_peers;
	int* was_offline;
	pthread_mutex_t lock;
};

struct peer {
	// Own info
	int own_sockfd;
	struct peer_address own_pa;
	// Tracker communication info
	struct netinfo_lock netinfo_lock;
	struct peer_address * network_info;
	size_t n_peers;
	pthread_t thread_id;
	int was_offline;
};

struct send_heartbeat_thread_args {
	struct netinfo_lock* netinfo_lock;
	int* sockfd;
	struct sockaddr_in* srvraddr;
	struct peer_address* pa;
};


size_t broadcast(struct peer* p, void * data, size_t data_len);
void receive(struct peer* p, void ** message, size_t* message_len, struct sockaddr_in* clntaddr);
void respond(struct peer* p, void * message, size_t message_len, const struct sockaddr_in* clntaddr);
int  should_refresh(struct peer* p);
void init_peer(struct peer* p, char* c_tracker_addr, char* c_tracker_port, char* c_addr, char* c_port);
void exit_peer(struct peer* p);

//#ifdef __cplusplus
//}
//#endif

#endif
