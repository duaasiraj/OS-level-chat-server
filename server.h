#ifndef SERVER_H
#define SERVER_H

#include "threadpool.h"

#ifndef PORT
#define PORT       5000
#endif

#ifndef BIND_IP
#define BIND_IP    ""
#endif

#ifndef MAX_CLIENTS
#define MAX_CLIENTS 64
#endif

#define RATE_LIMIT_COUNT   5
#define RATE_LIMIT_WINDOW  1

int  server_init(const char *ip, int port);
void server_run(int server_fd, threadpool_t *pool);
void server_shutdown(int sig);

extern threadpool_t g_pool;

#endif
