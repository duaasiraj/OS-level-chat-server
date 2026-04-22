#ifndef SOCKET_UTIL_H
#define SOCKET_UTIL_H

#include <netinet/in.h>

int               createTCPIPv4Socket(void);
struct sockaddr_in *createIPv4Address(char *ip, int port);

#endif
