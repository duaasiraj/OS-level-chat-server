#include "llist2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void llist_init(list_entry *list_start)
{
    list_start->client_info = NULL;
    list_start->next = NULL;
    list_start->mutex = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(list_start->mutex, NULL);
} // basic initialisation of list and mutex for head of list

int llist_insert(list_entry *list_start, client_info *element)
{
    list_entry *curr = list_start;
    list_entry *prev = list_start;
    int inserted = FALSE;
    while (curr != NULL)
    {                                    // check if any block free without client info ex head after initialising
        pthread_mutex_lock(curr->mutex); // lock current node mutex when entering to avoid race condition otherwise loop till last node in list
        if (curr->client_info == NULL)
        {
            curr->client_info = element;
            pthread_mutex_unlock(curr->mutex); // unlock after storing if current is free
        }
        pthread_mutex_unlock(curr->mutex); // unlock it while leaving
        prev = curr;
        curr = curr->next;
        inserted = TRUE;
    }
    if (!inserted)
    { // if not inserted insert after last node after alllocating proper space
        pthread_mutex_lock(prev->mutex);
        list_entry *new_entry = malloc(sizeof(list_entry));
        new_entry->client_info = element;
        new_entry->next = NULL;
        new_entry->mutex = malloc(sizeof(pthread_mutex_t));
        pthread_mutex_init(new_entry->mutex, NULL);
        prev->next = new_entry;
        pthread_mutex_unlock(prev->mutex);
        inserted = TRUE;
    }
    if (inserted)
        return 0;
    else
        return -1;
}

int llist_remove_by_sockfd(list_entry *list_start, int sockfd)
{
    list_entry *cur = list_start;
    while (cur != NULL)
    {
        pthread_mutex_lock(cur->mutex); // lock so no edits can be made when checking
        if (cur->client_info != NULL &&
            cur->client_info->sockfd == sockfd)
        {
            free(cur->client_info); // if found free the client info struct (cause we allocated dynamic space) so it can be reused when inserting
            cur->client_info = NULL;
            pthread_mutex_unlock(cur->mutex); // unlock the mutex
            return 0;
        }
        pthread_mutex_unlock(cur->mutex); // otherwise unlock and keep searching
        cur = cur->next;
    }
    return -1; // failed to remove
}

list_entry *llist_find_by_sockfd(list_entry *list_start, int sockfd)
{ // simple searching function
    list_entry *cur = list_start;
    while (cur != NULL)
    {
        pthread_mutex_lock(cur->mutex);
        if (cur->client_info != NULL &&
            cur->client_info->sockfd == sockfd)
        {
            pthread_mutex_unlock(cur->mutex);
            return cur;
        }
        pthread_mutex_unlock(cur->mutex);
        cur = cur->next;
    }
    return NULL;
}

list_entry *llist_find_by_nickname(list_entry *list_start, char *nickname)
{//simple searching function
    list_entry *cur = list_start;
    while (cur != NULL)
    {
        pthread_mutex_lock(cur->mutex);
        if (cur->client_info != NULL &&
            strcmp(cur->client_info->nickname, nickname) == 0)
        {
            pthread_mutex_unlock(cur->mutex);
            return cur;
        }
        pthread_mutex_unlock(cur->mutex);
        cur = cur->next;
    }
    return NULL;
}
//count of nodes with actual clients
int llist_get_count(list_entry *list_start)
{
    list_entry *cur = list_start;
    int count = 0;
    while (cur != NULL) {
        pthread_mutex_lock(cur->mutex);
        if (cur->client_info != NULL) count++;
        pthread_mutex_unlock(cur->mutex);
        cur = cur->next;
    }
    return count;
}
//copies nicknames to array and returns their count
int llist_get_nicknames(list_entry *list_start, char **nicks)
{
    list_entry *cur = list_start;
    int count = 0;
    while (cur != NULL) {
        pthread_mutex_lock(cur->mutex);
        if (cur->client_info != NULL)
            strncpy(nicks[count++], cur->client_info->nickname, 19);
        pthread_mutex_unlock(cur->mutex);
        cur = cur->next;
    }
    return count;
}