#ifndef LLIST2_H
#define LLIST2_H
#include <pthread.h>
#include <netinet/in.h>
#include <time.h>
#include "bool.h"
//this is going to create a header file for linked list 


//structure for storing client information
typedef struct client_info {
    int  sockfd;//socket file descripter
    char   nickname[20];//name it wants to be referred to as
    struct sockaddr_in address;//address of it
    int  msg_count;//how many messages it sent
    time_t   last_msg_time;//when was the last meessage sent
} client_info;

//actual linked list node
typedef struct list_entry {
    struct client_info *client_info;//pointer to client
    pthread_mutex_t    *mutex;//pointer to its mutex
    struct list_entry  *next;//pointer to next node
} list_entry;

void llist_init(list_entry *list_start);//initialisation
int  llist_insert(list_entry *list_start, client_info *element);//insertion 
int  llist_remove_by_sockfd(list_entry *list_start, int sockfd);//deletion by file descriptor
list_entry *llist_find_by_sockfd(list_entry *list_start, int sockfd);//search function
list_entry *llist_find_by_nickname(list_entry *list_start, char *nickname);//search function
int llist_get_count(list_entry *list_start);//counting number of clients
int llist_get_nicknames(list_entry *list_start, char **nicks);//getting nicknames

#endif 
