#include "socket_util.h"
#include <stdio.h>
#include <sys/socket.h> // contains socket functions 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>

int createTCPIPv4Socket(void){
	return socket(AF_INET,SOCK_STREAM,0);//takes address family, ipv4(AF_INET) or ipv6,type of socket tcp(SOCKET_STREAM) or udp ,layer we are using in the network mostly just set to 0 
}

struct sockaddr_in* createIPv4Address(char *ip,int port){
	struct sockaddr_in *address=malloc(sizeof(struct sockaddr_in));
	address->sin_port=htons(port);//dont use lower than 1000, htons gets bytes from port number , tcp expects bing endian format , this does that for it 
	address->sin_family=AF_INET;
	if(strlen(ip)==0){
		address->sin_addr.s_addr=INADDR_ANY;
	}else{
		inet_pton(AF_INET, ip, &address->sin_addr.s_addr);
	}
	return address;
}