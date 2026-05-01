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
    signal(SIGINT,  server_shutdown);//registers if control c pressed close server gracefully by calling it
    signal(SIGTERM, server_shutdown);//registers commands like kill and does same
    signal(SIGPIPE, SIG_IGN);//ignore when the client tries writing into a closed socket     
    logline(LOG_INFO, "Waiting for connections...");

    while (g_running) {
        struct sockaddr_in client_addr;//client data structure
        socklen_t addr_len = sizeof(client_addr);
        int client_fd = accept(server_fd,(struct sockaddr *)&client_addr,&addr_len);//accept client and return new socket fd
        if (client_fd < 0) {
            if (errno == EINTR) continue;   //interrupted by signal
            logline(LOG_ERROR, "accept() failed");
            break;
        }
        logline(LOG_INFO, "New connection: fd=%d from %s",client_fd, inet_ntoa(client_addr.sin_addr));
        registry_add(client_fd, client_addr);//add the client by registering it
        tp_enqueue(pool, client_fd);//push it top queue of threadpool
    }
}


void server_shutdown(int sig)
{
    (void)sig;
    logline(LOG_INFO, "Shutting down...");
    g_running = 0;
    broadcast("Server is shutting down. Goodbye.\r\n");
    tp_destroy(&g_pool);//destroy threadpool
    close(g_server_fd);//close server
    logline(LOG_INFO, "Done.");
    exit(0);
}
