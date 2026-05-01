#ifndef CLIENT_H
#define CLIENT_H

#include <pthread.h>
#include <netinet/in.h>
#include "llist2.h"

extern list_entry client_list;
extern pthread_mutex_t client_list_mutex;

//<<<<<<>>>>>> Registry function declarations below <<<<<<>>>>>>
void registry_init(void);
void registry_add(int fd, struct sockaddr_in addr);
void registry_remove(int fd);
list_entry* registry_find_by_fd(int fd);
list entry* registry_find_by_nick(const char* nick);
//<<<<<<>>>>>> end of registry related functions <<<<<<>>>>>>

void handle_client(int fd);

void process_msg(const char* message, int sender_fd);

void broadcast(const char* fmt, ...);

int send_private(const char* nickname, const char* fmt, ...);

void send_welcome(int fd);

int rate_limited(int fd);

int is_valid_utf8(const char* buf, int len);

#endif
