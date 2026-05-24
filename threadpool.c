#include "threadpool.h"
#include "log.h"
#include "client.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int tp_dequeue(threadpool_t *pool) //function takes task ID(fd) from the head and reassigns head to the next task ID
{
    int fd       = pool->queue[pool->head];
    pool->head   = (pool->head + 1) % QUEUE_SIZE; //modulo used for wrapping around (circular queue)
    pool->count--;
    return fd;
}


void *tp_worker(void *pool_ptr) {
    threadpool_t *pool = (threadpool_t *)pool_ptr; //pool pointer created of threadpool type

    while (1) {
        pthread_mutex_lock(&pool->mutex); //before the worker (thread) begins to take a task it locks to avoid race condition

        
        while (pool->count == 0 && !pool->shutdown) //to avoid CPU wasting its cycles workers (threads) are made to wait if the task queue is empty and server is still running
            pthread_cond_wait(&pool->not_empty, &pool->mutex);

        
        if (pool->shutdown && pool->count == 0) //if all the tasks have ended and the server is stopping, workers will unlock the mutex
        {
            pthread_mutex_unlock(&pool->mutex);
            return NULL;
        }

        int fd = tp_dequeue(pool); //worker takes up a task from the queue
        pthread_mutex_unlock(&pool->mutex); //once the worker has taken up a task mutex is unlocked for other workers to take up remaining tasks

        handle_client(fd);
     
    }

    return NULL;
}



void tp_init(threadpool_t *pool) {
    memset(pool, 0, sizeof(threadpool_t)); //used to clear struct of any garbage values
    pthread_mutex_init(&pool->mutex, NULL);
    pthread_cond_init(&pool->not_empty, NULL);

    for (int i = 0; i < MAX_WORKERS; i++) {
        if (pthread_create(&pool->threads[i], NULL, tp_worker, pool) != 0)
            logline(LOG_ERROR, "Failed to create worker thread %d", i);
    }

    logline(LOG_INFO, "Thread pool ready (%d workers)", MAX_WORKERS);
}

void tp_enqueue(threadpool_t *pool, int client_fd) {
    pthread_mutex_lock(&pool->mutex); //mutex locks out so that only the producer thread can access the queue and no other threads (worker threads) 

    //if the task amount reaches the max queue size no more tasks can be added
    if (pool->count == QUEUE_SIZE) {
        logline(LOG_ERROR, "Queue full — dropping fd=%d", client_fd);
        close(client_fd);
        pthread_mutex_unlock(&pool->mutex); //allows worker threads to take up tasks again
        return;
    }

    pool->queue[pool->tail] = client_fd; //new task added at the tail
    pool->tail  = (pool->tail + 1) % QUEUE_SIZE; //tail is updated and modulo is used to wrap around
    pool->count++;

    pthread_cond_signal(&pool->not_empty); //once the producer thread has added a task the condition is met and a signal is sent to the wait state above
    pthread_mutex_unlock(&pool->mutex); //mutex is unlocked for worker threads to take up tasks
}


void tp_destroy(threadpool_t *pool) {
    pthread_mutex_lock(&pool->mutex);
    pool->shutdown = 1;
    pthread_cond_broadcast(&pool->not_empty); //all workers are woken so that the shutdown of server is boradcasted
    pthread_mutex_unlock(&pool->mutex);
//all threads, mutex and condition are cleared from memory
    for (int i = 0; i < MAX_WORKERS; i++)
        pthread_join(pool->threads[i], NULL); 

    pthread_mutex_destroy(&pool->mutex);
    pthread_cond_destroy(&pool->not_empty);

    logline(LOG_INFO, "Thread pool destroyed.");
}
