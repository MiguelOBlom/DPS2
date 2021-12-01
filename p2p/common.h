#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>

#ifndef COMMON_H_
#define COMMON_H_

void initialize_srvr(int* sockfd, const int* domain, const int* port, const uint32_t* address);
void initialize_clnt(int* sockfd, const int* domain, const int* port, const uint32_t* address, struct sockaddr_in* sockaddr);
void wait_message(const int* sockfd, void* data, const size_t* data_len, struct sockaddr_in* sockaddr);
void send_message(const int* sockfd, const void* data, const size_t* data_len, int flags, const struct sockaddr_in* sockaddr);

#endif // COMMON_H_