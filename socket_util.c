#include "socket_util.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>

int createTCPIPv4Socket(void) {
    return socket(AF_INET, SOCK_STREAM, 0);
}

struct sockaddr_in *createIPv4Address(char *ip, int port) {
    struct sockaddr_in *address = malloc(sizeof(struct sockaddr_in));
    memset(address, 0, sizeof(*address));
    address->sin_family = AF_INET;
    address->sin_port   = htons(port);

    if (ip == NULL || ip[0] == '\0')
        address->sin_addr.s_addr = INADDR_ANY;
    else
        inet_pton(AF_INET, ip, &address->sin_addr.s_addr);

    return address;
}
