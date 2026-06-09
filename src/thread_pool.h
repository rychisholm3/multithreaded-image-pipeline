#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include <stddef.h>

typedef void (*task_fn)(void *arg);

typedef struct Task {
    task_fn      fn;
    void        *arg;
    struct Task *next;
} Task;

typedef struct {
    Task           *head;
    Task           *tail;
    size_t          pending;    /* tasks waiting in queue */
    size_t          active;     /* tasks currently executing */
    int             shutdown;
    pthread_mutex_t lock;
    pthread_cond_t  work_available;
    pthread_cond_t  all_idle;
} TaskQueue;

typedef struct {
    pthread_t  *threads;
    int         num_threads;
    TaskQueue   queue;
} ThreadPool;

ThreadPool *thread_pool_create(int num_threads);
int         thread_pool_submit(ThreadPool *pool, task_fn fn, void *arg);
void        thread_pool_wait(ThreadPool *pool);   /* blocks until queue is drained */
void        thread_pool_destroy(ThreadPool *pool);

#endif
