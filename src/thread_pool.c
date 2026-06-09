#include "thread_pool.h"
#include <stdlib.h>
#include <stdio.h>

static void *worker_thread(void *arg) {
    TaskQueue *q = (TaskQueue *)arg;

    for (;;) {
        pthread_mutex_lock(&q->lock);

        /* sleep until there's work or a shutdown signal */
        while (q->pending == 0 && !q->shutdown)
            pthread_cond_wait(&q->work_available, &q->lock);

        if (q->shutdown && q->pending == 0) {
            pthread_mutex_unlock(&q->lock);
            break;
        }

        /* dequeue one task */
        Task *t  = q->head;
        q->head  = t->next;
        if (!q->head) q->tail = NULL;
        q->pending--;
        q->active++;

        pthread_mutex_unlock(&q->lock);

        /* execute outside the lock — this is where parallelism happens */
        t->fn(t->arg);
        free(t);

        pthread_mutex_lock(&q->lock);
        q->active--;
        /* signal thread_pool_wait() if everything is done */
        if (q->pending == 0 && q->active == 0)
            pthread_cond_broadcast(&q->all_idle);
        pthread_mutex_unlock(&q->lock);
    }

    return NULL;
}

ThreadPool *thread_pool_create(int num_threads) {
    ThreadPool *pool = calloc(1, sizeof(ThreadPool));
    if (!pool) return NULL;

    pool->num_threads = num_threads;
    pool->threads     = malloc((size_t)num_threads * sizeof(pthread_t));
    if (!pool->threads) { free(pool); return NULL; }

    TaskQueue *q = &pool->queue;
    pthread_mutex_init(&q->lock,           NULL);
    pthread_cond_init (&q->work_available, NULL);
    pthread_cond_init (&q->all_idle,       NULL);

    for (int i = 0; i < num_threads; i++)
        pthread_create(&pool->threads[i], NULL, worker_thread, q);

    return pool;
}

int thread_pool_submit(ThreadPool *pool, task_fn fn, void *arg) {
    Task *t = malloc(sizeof(Task));
    if (!t) return -1;
    t->fn   = fn;
    t->arg  = arg;
    t->next = NULL;

    TaskQueue *q = &pool->queue;
    pthread_mutex_lock(&q->lock);
    if (q->tail) q->tail->next = t;
    else         q->head       = t;
    q->tail = t;
    q->pending++;
    pthread_cond_signal(&q->work_available);   /* wake one idle worker */
    pthread_mutex_unlock(&q->lock);
    return 0;
}

void thread_pool_wait(ThreadPool *pool) {
    TaskQueue *q = &pool->queue;
    pthread_mutex_lock(&q->lock);
    while (q->pending > 0 || q->active > 0)
        pthread_cond_wait(&q->all_idle, &q->lock);
    pthread_mutex_unlock(&q->lock);
}

void thread_pool_destroy(ThreadPool *pool) {
    TaskQueue *q = &pool->queue;

    pthread_mutex_lock(&q->lock);
    q->shutdown = 1;
    pthread_cond_broadcast(&q->work_available);
    pthread_mutex_unlock(&q->lock);

    for (int i = 0; i < pool->num_threads; i++)
        pthread_join(pool->threads[i], NULL);

    /* drain any unprocessed tasks */
    Task *t = q->head;
    while (t) { Task *nx = t->next; free(t); t = nx; }

    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy (&q->work_available);
    pthread_cond_destroy (&q->all_idle);

    free(pool->threads);
    free(pool);
}
