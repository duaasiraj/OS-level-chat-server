#include "server.h"
#include "client.h"
#include "threadpool.h"
#include "socket_util.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

threadpool_t g_pool;
static int g_server_fd = -1;
static int g_running = 1;

int server_init(const char *ip, int port)
{
    g_server_fd = createTCPIPv4Socket();
    if (g_server_fd < 0)
    {
        logline(LOG_ERROR, "socket creation failed");
        return -1;
    }
    int opt = 1;
    setsockopt(g_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
// Allows the server to reuse the same port immediately after restart/crash
    struct sockaddr_in *addr = createIPv4Address((char *)ip, port);//ensures socket is available instantly
    if (bind(g_server_fd, (struct sockaddr *)addr, sizeof(*addr)) < 0)//binds the server to the address for all coming instances
    {
        logline(LOG_ERROR, "bind failed on port %d", port);
        free(addr);
        close(g_server_fd);
        return -1;
    }
    free(addr);

    if (listen(g_server_fd, 10) < 0)//start a queue
    {
        logline(LOG_ERROR, "listen() failed");
        close(g_server_fd);
        return -1;
    }

    logline(LOG_INFO, "Listening on %s:%d", (ip && ip[0]) ? ip : "0.0.0.0", port);//if both true use provided ip orhterwise default to 0.0.0.0
    return g_server_fd;
}

void server_run(int server_fd, threadpool_t *pool)
{
}
void server_shutdown(int sig)
{
}