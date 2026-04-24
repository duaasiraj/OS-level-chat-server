#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>

#define MAX_WORKERS  8 //max threads
#define QUEUE_SIZE   64 //max tasks

typedef struct {
    int             queue[QUEUE_SIZE]; //task queue
    int             head; //thread picks up task from this index
    int             tail; //main server drops a task at this index
    int             count; //keeps count of number of tasks in queue
    pthread_mutex_t mutex; //used for synchronization. locks when a thread is picking a task to avoid race condition. unlocks once the thread has picked a task
    pthread_cond_t  not_empty; //puts a condition. thread can only pick a task if there are any tasks in queue
    int             shutdown; //used if a server needs to close
    pthread_t       threads[MAX_WORKERS];
} threadpool_t;

void  tp_init(threadpool_t *pool); 
void  tp_enqueue(threadpool_t *pool, int client_fd);
void  tp_destroy(threadpool_t *pool);
void *tp_worker(void *pool_ptr);

#endif
