#include <stdio.h>
#include <unistd.h>
#include "threadpool.h" 
#include "log.h"        
//add log.c and threadpool.c with gcc in terminal 
// gcc test_threadpool.c threadpool.c log.c -o testP -lpthread

int main() {
    threadpool_t pool;

    // Initialize our custom logger
    set_loglevel(LOG_DEBUG);
    logline(LOG_INFO, "=== Starting Threadpool Stress Test ===");

    //  Initialize the pool
    // This will spawn 8 threads that immediately go to sleep
    tp_init(&pool);

    // Enqueue 15 tasks (simulated client file descriptors)
    for (int i = 1; i <= 15; i++) {
        logline(LOG_DEBUG, "Main: Adding task %d to the queue", i);
        tp_enqueue(&pool, i);
        
        // Small delay so the log isn't a total blur
        usleep(50000); 
    }

    // Give the workers some time to "process"
    logline(LOG_INFO, "Main: Tasks dispatched. Waiting for workers...");
    sleep(2); 

    // Clean up
    logline(LOG_INFO, "=== Initiating Shutdown ===");
    tp_destroy(&pool);

    logline(LOG_INFO, "Test completed successfully.");

    return 0;
}