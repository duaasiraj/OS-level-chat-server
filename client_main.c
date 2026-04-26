#define _POSIX_C_SOURCE 200809L
#include "socket_util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* Global file descriptor for the socket so both threads can access it */
static int g_sockfd = -1;

/* Background thread: constantly listens for messages sent by the server */
void *recv_thread(void *arg) {
    (void)arg; // Cast to void to avoid "unused parameter" warning
    char buf[1024];

    while (1) {
        /* Receive data from the server and store it in buf */
        ssize_t n = recv(g_sockfd, buf, sizeof(buf) - 1, 0);
        
        /* If recv returns 0 or less, the connection is closed or failed */
        if (n <= 0) {
            printf("\n[Disconnected from server]\n");
            fflush(stdout); // Ensure the message prints immediately
            break;
        }

        /* Null-terminate the string and print the server's message */
        buf[n] = '\0';
        printf("%s", buf);
        fflush(stdout);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    /* Default connection details: Localhost on Port 5000 */
    const char *ip   = "127.0.0.1";
    int         port = 5000;

    /* Override defaults if the user provides IP or Port in the terminal */
    if (argc >= 2) ip   = argv[1];
    if (argc >= 3) port = atoi(argv[2]);

    /* Create the socket using the helper function (IPv4, TCP) */
    g_sockfd = createTCPIPv4Socket();
    if (g_sockfd < 0) {
        fprintf(stderr, "Socket creation failed\n");
        return 1;
    }

    /* Prepare the server address structure and attempt to connect */
    struct sockaddr_in *addr = createIPv4Address((char *)ip, port);
    if (connect(g_sockfd, (struct sockaddr *)addr, sizeof(*addr)) != 0) {
        fprintf(stderr, "Failed to connect to %s:%d\n", ip, port);
        free(addr); // Clean up memory before exiting
        return 1;
    }
    free(addr); // Address info is no longer needed after connection

    printf("Connected to %s:%d\n", ip, port);

    /* Start the background thread to handle incoming messages */
    pthread_t tid;
    pthread_create(&tid, NULL, recv_thread, NULL);
    
    /* Detach the thread so its resources are freed automatically on exit */
    pthread_detach(tid);

    /* Main thread loop: Read input from keyboard and send to server */
    char *line     = NULL;
    size_t lineSize = 0;

    while (1) {
        /* Wait for user to type a line */
        ssize_t n = getline(&line, &lineSize, stdin);
        
        /* Stop if input fails or if user types "exit" */
        if (n <= 0) break;
        if (strncmp(line, "exit\n", 5) == 0) break;
        
        /* Send the typed line through the socket to the server */
        send(g_sockfd, line, n, 0);
    }

    /* Cleanup: free the input buffer and close the socket connection */
    free(line);
    close(g_sockfd);
    
    return 0;
}