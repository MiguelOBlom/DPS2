#include "common.h"

const int domain = AF_INET;
const int port = 8080;
const uint32_t address = INADDR_ANY;

void join_network(const int* sockfd, const struct sockaddr_in* srvraddr, int port) {
	struct join_header join_header = init_join_header(port);
	send_message(sockfd, &join_header, sizeof(join_header), MSG_CONFIRM, srvraddr);


}



int main(int argc, char ** argv) {
	int sockfd;
	struct sockaddr_in srvraddr;
	
	initialize_clnt(&sockfd, &domain, &port, &address, &srvraddr);
	join_network(&sockfd, &srvraddr, port);


	//send_message(&sockfd, buf, &buf_len, MSG_CONFIRM, &srvraddr);
	//wait_message(&sockfd, (void*) buf, &buf_len, MSG_WAITALL, &srvraddr);

	return 0;
}