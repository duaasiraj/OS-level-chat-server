#include "llist2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



void llist_init(list_entry *list_start){

}

int llist_insert(list_entry *list_start, client_info *element){

}

int llist_remove_by_sockfd(list_entry *list_start, int sockfd){

}

list_entry  *llist_find_by_sockfd(list_entry *list_start, int sockfd){

}

list_entry  *llist_find_by_nickname(list_entry *list_start, char *nickname){

}

int llist_get_count(list_entry *list_start){

}

int llist_get_nicknames(list_entry *list_start, char **nicks){
    
}