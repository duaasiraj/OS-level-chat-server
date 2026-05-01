#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <sys/socket.h>
#include <pthread.h>
#include "client.h"
#include "server.h"
#include "log.h"
#include "colors.h"

list_entry client_list;
pthread_mutex_t client_list_mutex = PTHREAD_MUTEX_INITIALIZER;

//<<<<<<>>>>>> Registry function implementations below <<<<<<>>>>>>


void registry_init(void){
  
  llist_init(&client_list);
  logline(LOG_INFO, "Client registry initialised.");
  
}


void registry_add(int fd, struct sockaddr_in addr){

  client_info* ci = malloc(sizeof(client_info));
  
  if(!ci){
    logline(LOG_ERROR, "malloc failed in registry_add");
    return;
  }
  
  ci->sockfd = fd;
  ci->address = addr;
  ci->msg_count = 0;
  ci->last_msg_time = time(NULL);
  snprintf(ci->nickname, sizeof(ci->nickname), "anonymous_user_%d", fd);
  
  pthread_mutex_lock(&client_list_mutex);
  llist_insert(&client_list, ci);
  pthread_mutex_unlock(&client_list_mutex);
  
  logline(LOG_DEBUG, "registry_add: fd = %d", fd);
  
} 

void registry_remove(int fd){

  pthread_mutex_lock(&client_list_mutex);
  llist_remove_by_sockfd(&client_list, fd);
  pthread_mutex_unlock(&client_list_mutex);
  
  logline(LOG_DEBUG, "registry_remove: fd = %d", fd);
}


list_entry* registry_find_by_fd(int fd){

  return llist_find_by_sockfd(&client_list, fd);

}


list_entry* registry_find_by_nick(const char* nick){

  return llist_find_by_nickname(&client_list, (char*)nick);

}

//<<<<<<>>>>>> end of registry related functions <<<<<<>>>>>>

void broadcast(const char* fmt, ...){

  char buffer[1024];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);
  
  int fds[MAX_CLIENTS];
  int count = 0;
  
  pthread_mutex_lock(&client_list_mutex);
  list_entry* curr = client_list.next;
  
  while(curr != NULL && count < MAX_CLIENTS){
  
    pthread_mutex_lock(curr->mutex);
    
    if(curr->client_info != NULL){
      fds[count] = curr->client_info->sockfd;
      count++;
    }
    
    pthread_mutex_unlock(curr->mutex);
    curr = curr->next;
  }
  
  pthread_mutex_unlock(&client_list_mutex);
  
  for(int i=0; i<count; i++){
    if(send(fds[i], buffer, strlen(buffer), 0)<0 && errno == EPIPE){
      registry_remove(fds[i]);
      close(fds[i]);
    }
  }

}

int send_private(const char* nickname, const char* fmt, ...){

  char buffer[1024];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);
  
  list_entry* entry = registry_find_by_nick(nickname);
  if(entry==NULL || entry->client_info == NULL){
    return -1;
  }
  
  send(entry->client_info->sockfd, buffer, strlen(buffer), 0);
  return 0;

}


void send_welcome(int fd){

  char banner[512];
  snprintf(banner, sizeof(banner), "%s╔══════════════════════════════════╗\r\n"
        "║     Welcome to OS Chat Server    ║\r\n"
        "╚══════════════════════════════════╝%s\r\n"
        "Commands: /nick <name>  /pm <nick> <msg>  /who  /me <text>  /quit\r\n\r\n", color_cyan, color_normal);
  
  send(fd, banner, strlen(banner), 0);

}

int rate_limited(int fd){

  list_entry* entry = registry_find_by_fd(fd);

}
