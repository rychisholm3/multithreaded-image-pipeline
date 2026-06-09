#include "pipeline.h"
#include "image.h"
#include "filters.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static double mono_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

typedef struct {
    PipelineJob   *job;
    PipelineStats *stats;
} JobArg;

static void execute_job(void *arg) {
    JobArg        *ja    = (JobArg *)arg;
    PipelineJob   *job   = ja->job;
    PipelineStats *stats = ja->stats;
    free(ja);

    double t0 = mono_ms();

    Image *img = image_load_ppm(job->input_path);
    if (!img) {
        fprintf(stderr, "[job %02d] ERROR: cannot load '%s'\n",
                job->id, job->input_path);
        pthread_mutex_lock(&stats->lock);
        stats->jobs_failed++;
        pthread_mutex_unlock(&stats->lock);
        return;
    }

    printf("[job %02d] start  %-30s  %dx%d  %dch\n",
           job->id, job->input_path, img->width, img->height, img->channels);

    for (int i = 0; i < job->num_filters; i++) {
        FilterSpec *fs   = &job->filters[i];
        Image      *next = NULL;

        switch (fs->type) {
            case FILTER_GRAYSCALE:     next = filter_grayscale(img);              break;
            case FILTER_GAUSSIAN_BLUR: next = filter_gaussian_blur(img, fs->param); break;
            case FILTER_SOBEL_EDGE:    next = filter_sobel_edge(img);             break;
            case FILTER_BRIGHTNESS:    next = filter_brightness(img, fs->param);  break;
            case FILTER_INVERT:        next = filter_invert(img);                 break;
        }

        if (!next) {
            fprintf(stderr, "[job %02d] ERROR: filter %d failed\n", job->id, i);
            image_free(img);
            pthread_mutex_lock(&stats->lock);
            stats->jobs_failed++;
            pthread_mutex_unlock(&stats->lock);
            return;
        }
        image_free(img);
        img = next;
    }

    long long pixels = (long long)img->width * img->height;
    int       saved  = image_save_ppm(img, job->output_path);
    image_free(img);

    double elapsed_ms = mono_ms() - t0;

    if (saved != 0) {
        fprintf(stderr, "[job %02d] ERROR: cannot save '%s'\n",
                job->id, job->output_path);
        pthread_mutex_lock(&stats->lock);
        stats->jobs_failed++;
        pthread_mutex_unlock(&stats->lock);
        return;
    }

    printf("[job %02d] done   %-30s  %.1f ms\n",
           job->id, job->output_path, elapsed_ms);

    pthread_mutex_lock(&stats->lock);
    stats->jobs_completed++;
    stats->pixels_processed  += pixels;
    stats->total_worker_ms   += elapsed_ms;
    pthread_mutex_unlock(&stats->lock);
}

void pipeline_stats_init(PipelineStats *s) {
    memset(s, 0, sizeof(*s));
    pthread_mutex_init(&s->lock, NULL);
}

void pipeline_stats_print(const PipelineStats *s, double wall_ms) {
    printf("\n+----------------------------------+\n");
    printf("|       Pipeline Statistics        |\n");
    printf("+----------------------------------+\n");
    printf("  Jobs completed  : %d\n",   s->jobs_completed);
    printf("  Jobs failed     : %d\n",   s->jobs_failed);
    printf("  Pixels processed: %lld\n", s->pixels_processed);
    if (s->jobs_completed > 0) {
        printf("  Avg time / job  : %.1f ms\n",
               s->total_worker_ms / s->jobs_completed);
        printf("  Total worker ms : %.1f ms\n", s->total_worker_ms);
    }
    printf("  Wall-clock time : %.1f ms\n", wall_ms);
    if (wall_ms > 0 && s->total_worker_ms > 0)
        printf("  Parallelism gain: %.2fx\n",
               s->total_worker_ms / wall_ms);
    printf("+----------------------------------+\n\n");
}

void pipeline_run(PipelineJob *jobs, int num_jobs,
                  int num_threads, PipelineStats *stats) {
    printf("Pipeline: %d jobs across %d worker thread(s)\n\n",
           num_jobs, num_threads);

    ThreadPool *pool = thread_pool_create(num_threads);
    if (!pool) { fprintf(stderr, "thread_pool_create failed\n"); return; }

    for (int i = 0; i < num_jobs; i++) {
        JobArg *ja = malloc(sizeof(JobArg));
        if (!ja) { fprintf(stderr, "OOM\n"); continue; }
        ja->job   = &jobs[i];
        ja->stats = stats;
        thread_pool_submit(pool, execute_job, ja);
    }

    thread_pool_wait(pool);
    thread_pool_destroy(pool);
}
