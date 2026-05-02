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
  if(entry==NULL || entry->client_info==NULL){
    return 0;
  }
  
  client_info* ci = entry->client_info;
  time_t now = time(NULL);
  
  if(now - ci->last_msg_time > RATE_LIMIT_WINDOW){
    ci->msg_count = 0;
    ci->last_msg_time = now;
  }
  
  ci->msg_count++;
  if(ci->msg_count > RATE_LIMIT_COUNT){
    return 1;
  }
  return 0;
}


int is_valid_utf8(const char* buffer, int len){

  int i=0;
  while(i<len){
  
    unsigned char c = (unsigned char)buffer[i];
    int extra = 0;
    
    if(c <= 0x7F){
      extra = 0;
    }else if(c >= 0xC0 && c <= 0xDF){
      extra = 1;
    }else if(c >= 0xE0 && c <= 0xEF){
      extra = 2;
    }else if(c >= 0xF0 && c <= 0xF7){
      extra = 3;
    }else{
      return 0;
    }
    
    for(int j=1; j<=extra; j++){
    
      if(i+j >= len){
        return 0;
      }
      
      unsigned char after = (unsigned char)buffer[i+j];
      if(after < 0x80 || after > 0xBF){
        return 0;
      }
    
    }
    i += 1 + extra;
  }
  return 1;
}

/* ik the code above is cryptic looking its not trust me this is just how utf-8 checking is done which is needed for emoji support since emojis are not not ASCII characters (excluding ascii emojis like :D)

the thing with non-ASCII i.e, UTF-8 encoded characters is they take up more than one byte (unless they're characters that fall in the orginal ASCII range)

in UTF-8 encoding, the following format is used
  0xxxxxxx => this is a one byte aka ASCII character, it needs no extra bytes [in hex the value ranges from 0x00 till 0x7F]
  
  110xxxxx 10xxxxxx => a 2 byte character=> [0xC0 till 0xDF]
  
  1110xxxx 10xxxxxx 10xxxxxx => 3 byte => [0xE0 - 0xEF]
  
  11110xxx 10xxxxxx 10xxxxxx 10xxxxxx => 4 byte => [0xF0 - 0xF7]
  
the bits written are pre defined bits for these characters, with the 'x' bits holding the actual data. 10xxxxxx represents the continuation bytes for all characters that are more than one byte. continuation bits themselves have their own valid range, i.e, [0x80 - 0xB7]

So yea the code above is just checking that if a char lies in a given range, it should have so and so continuation bytes in front of it, and once valid, we skip over all those bytes and move onto the the next

*/

void handle_client(int fd){

  char buffer[1024];
  ssize_t n;
  
  // <<<<>>>> authorizing the user <<<<>>>> 
  const char* prompt = "Welcome || Enter your username: ";
  send(fd, prompt, strlen(prompt), 0)

  memset(buffer, 0, sizeof(buffer));
  n = recv(fd, buffer, sizeof(buffer)-1, 0);
  
  if(n<=0){
    close(fd);
    registry_remove(fd);
    return;
  }
  
  buffer[n] = '\0';
  buffer[strcspn(buffer, "\r\n")] = '\0';
  
  if(strlen(buffer) == 0){ 
    const char* invalid = "<<<>>> INVALID USERNAME <<<>>>\r\n";
    send(fd, invalid, strlen(invalid), 0);
    close(fd);
    registry_remove(fd);
    return;
  }
  
  if(registry_find_by_nick(buffer) != NULL){
    const char* taken = "<<<>>> This username is already taken :( <<<>>>\r\n";
    send(fd, taken, strlen(taken), 0);
    close(fd);
    registry_remove(fd);
    return;
  }
  
  char nickname[30];
  strncpy(nickname, buffer, sizeof(nickname)-1);
  nickname[sizeof(nickname) - 1] = '\0';
  
  pthread_mutex_lock(&client_list_mutex);
  
  list_entry* entry = llist_find_by_sockfd(&client_list, fd);
  if(entry && entry->client_info){
    strncpy(entry->client_info->nickname, nickname, sizeof(entry->client_info->nickname)-1)
  }
  
  pthread_mutex_unlock(&client_list_mutex);

  send_welcome(fd);
  
  broadcast("%s<===> %s has joined the chat! <===>%s\r\n", color_green, nickname, color_normal);
  
  logline(LOG_INFO, "'%s' connected on fd = %d", nickname, fd);
  
  
  char msg[1024];
  memset(msg, 0, sizeof(msg));
  
  while(1){
  
    memset(buffer, 0, sizeof(buffer));
    n = recv(fd, buffer, sizeof(buffer)-1, 0);
    
    if(n==0){
      logline(LOG_INFO, "fd = %d disconnected", fd);
      break;
    }
    
    if(n==0){
      logline(LOG_ERROR, "recv error on fd=%d", fd);
      break;
    }
    
    buffer[n] = '\0';
    strncat(msg, buffer, sizeof(msg) - strlen(msg) - 1);
  
    if(strchr(msg, '\n') != NULL){
      
      msg[strcspn(msg, "\r\n")] = '\0';
      if(strlen(msg) > 0){
        process_msg(msg,fd);
      }
      memset(msg, 0, sizeof(msg));
    }
  
  }
  
  entry = registry_find_by_fd(fd);
  
  if(entry && entry->client_info){
    strncpy(nickname, entry->client_info->nickname, sizeof(nickname)-1);
  }
  
  broadcast("%s<===> %s has left the chat <===>%s\r\n", color_red, nickname, color_normal);
  
  registry_remove(fd);
  close(fd);
  logline(LOG_INFO, "'%s' fd = %d session ended",nickname, fd);
  
}


void process_msg(const char* msg, int sender_fd){

  list_entry* entry = registry_find_by_fd(fd);
  
  if(entry==NULL || entry->client_info == NULL){
    return;
  }
    
  char nick[30];
  strncpy(nick, entry->client_info->nickname, sizeof(nick)-1);
  nick[sizeof(nick) - 1] = '\0';
  
  if(rate_limited(sender_fd)){
    const char* rate_prompt = "HEY! You're sending messages too fast!\r\n";
    send(sender_fd, rate_prompt, strlen(rate_prompt), 0);
    return;
  }
  
  if(!is_valid_utf8(msg, strlen(msg))){
    const char* utf_prompt = "Invalid utf-8 char in msg.\r\n";
    send(sender_fd, utf_prompt, strlen(utf_prompt), 0);
    return;
  }
  
  char copy[1024];
  strncpy(copy, msg, sizeof(copy)-1);
  copy[sizeof(copy)-1] = '\0';
  
  if(strcmp(copy, "/quit")==0){
    broadcast("%s<===> %s has quit <===>%s\r\n", color_red, nick, color_normal);
  
    registry_remove(sender_fd);
    close(sender_fd);  
    pthread_exit(NULL);
  }
  
  if(strcmp(copy, "who")==0){
    
    char* nicks[MAX_CLIENTS];
    for(int i=0;i<MAX_CLIENTS;i++){
      
      nicks[i] = malloc(30);
      memset(nicks[i], 0, 30);

    }
    
    pthread_mutex_lock(&client_list_mutex);
    int num = llist_get_nicknames(&client_list, nicks);
    pthread_mutex_unlock(&client_list_mutex);
    
    char who_buffer[2048] = {0};
    snprintf(who_buffer, sizeof(who_buffer), "%s=== ONLINE USERS [%d] ===%s", color_cyan, num, color_normal);
    
    for(int i=0; i<num;i++){
      strncat(who_buffer, nicks[i], sizeof(who_buffer) - strlen(who_buffer) - 4);
      
      strncat(who_buffer, "\r\n", sizeof(who_buffer) - strlen(who_buffer) - 1);
      
      free(nicks[i]);
      
    }
    
    send(sender_fd, who_buffer, strlen(who_buffer), 0);
    return;
  }
  
  if(strncmp(copy, "/nick", 6)==0){
    
    char* newnick = copy + 6;
    
    if(strlen(newnick) ==0){
      send(sender_fd, "Usage: /nick <name>\r\n",21, 0);
      return;
    }
    
    if(registry_find_by_nick(newnick) != NULL){
      send(sender_fd, "That nickname is already taken.\r\n",33, 0);
      return;
    }
    
    pthread_mutex_lock(&client_list_mutex);
    
    list_entry* e = llist_find_by_sockfd(&client_list, sender_fd);
    if(e && e->client_info){
      strncpy(e->client_info->nickname, newnick, sizeof(e->client_info->nickname)-1);
    }
    
    pthread_mutex_unlock(&client_list_mutex);
    
    broadcast("%s====== %s is now known as %s ======%s\r\n", color_yellow, nick, newnick, color_normal);
    logline(LOG_INFO, "'%s' renamed to '%s'", nick, newnick);
    return;
  }
  
  
  if(strncmp(copy, "/me", 4)==0){
    char* action = copy + 4;
    broadcast("%s* %s %s%s\r\n", color_yellow, nick, action, color_normal);
    
    return;
  }
  
  if(strncmp(copy, "/pm", 4)==0){
    
    char* rest = copy+4;
    char* target = strtok(rest, " ");
    char* pmsg = strtok(NULL, "");
    
    if(target==NULL || pmsg == NULL){
      send(sender_fd, "Usage: /pm <nick> <message>\r\n", 29, 0);
      return;
    }
    
    char outbuffer[1100];
    snprintf(outbuffer, sizeof(outbuffer), "%s[PM from %s]: %s%s\r\n", color_magenta, nick, pmsg, color_normal);
    
    if(send_private(target, %s, outbuffer) < 0){
      send(sender_fd, "User not found.\r\n",17,0);
    }else{
      
      char confirm[1100];
      snprintf(confirm, sizeof(confirm), "%s[PM from %s]: %s%s\r\n", color_magenta, target, pmsg, color_normal);
      send(senedr_fd, confirm, strlen(confirm), 0)
      
    }
    return;
  }
  
  if(copy[0] == '/'){
    send(sender_fd, "Unknown command. Try /who /nick /pm /me /quit\r\n", 48, 0)
    return;
  }
  
  broadcast("%s%s%s: %s\r\n", color_white, nick, color_normal, message);
  logline(LOG_INFO, "<%s> %s", nick, message);
}
