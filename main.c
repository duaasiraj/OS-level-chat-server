#include "server.h"
#include "client.h"
#include "threadpool.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void print_usage(const char *prog) {
    printf("Usage: %s [-p port] [-l loglevel] [-h]\n", prog);
    printf("  -p port Port to listen on      (default: %d)\n", PORT);
    printf("  -l level 1=ERROR 2=INFO 3=DEBUG  (default: 2)\n");
    printf("  -h Show this help\n");
}

int main(int argc, char *argv[]) {
    int port= PORT;
    int loglevel = LOG_INFO;

    int opt;//A standard C function to parse CLI flags.
    while ((opt = getopt(argc, argv, "p:l:h")) != -1) {
        switch (opt) {
            case 'p':
                port = atoi(optarg);
                if (port < 1 || port > 65535) {
                    fprintf(stderr, "Invalid port: %s\n", optarg);
                    return 1;
                }
                break;
            case 'l':
                loglevel = atoi(optarg);
                if (loglevel < 1 || loglevel > 3) {
                    fprintf(stderr, "Log level must be 1, 2, or 3\n");
                    return 1;
                }
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    set_loglevel(loglevel);
    logline(LOG_INFO, "=== OS Chat Server starting (port=%d) ===", port);
    registry_init();
    tp_init(&g_pool);
    int server_fd = server_init(BIND_IP, port);
    if (server_fd < 0) {
        logline(LOG_ERROR, "Server init failed. Exiting.");
        return 1;
    }

    server_run(server_fd, &g_pool);   // blocks until shutdown 
    return 0;
}
