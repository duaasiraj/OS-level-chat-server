#ifndef SOCKET_UTIL
#define SOCKET_UTIL

#include <stdio.h>
#include <sys/socket.h> // contains socket functions 
#include <netinet/in.h>
#include <arpa/inet.h>//for facilitating ip address parsing 

int createTCPIPv4Socket(void);

struct sockaddr_in* createIPv4Address(char *ip,int port);

#endif