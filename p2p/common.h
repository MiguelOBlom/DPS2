#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>

#ifndef COMMON_H_
#define COMMON_H_



void initialize(int* sockfd, const int* domain, const int* port, const uint32_t* address);


#endif // COMMON_H_