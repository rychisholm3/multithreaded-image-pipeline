#ifndef PIPELINE_H
#define PIPELINE_H

#include "thread_pool.h"
#include <pthread.h>

typedef enum {
    FILTER_GRAYSCALE,
    FILTER_GAUSSIAN_BLUR,
    FILTER_SOBEL_EDGE,
    FILTER_BRIGHTNESS,
    FILTER_INVERT,
} FilterType;

typedef struct {
    FilterType type;
    int        param;   /* blur radius or brightness delta */
} FilterSpec;

#define MAX_FILTERS 8
#define MAX_PATH    512

typedef struct {
    int        id;
    char       input_path [MAX_PATH];
    char       output_path[MAX_PATH];
    FilterSpec filters[MAX_FILTERS];
    int        num_filters;
} PipelineJob;

/* Shared stats — all fields guarded by lock */
typedef struct {
    pthread_mutex_t lock;
    int             jobs_completed;
    int             jobs_failed;
    long long       pixels_processed;
    double          total_worker_ms;   /* sum of per-job wall times */
} PipelineStats;

void pipeline_stats_init (PipelineStats *s);
void pipeline_stats_print(const PipelineStats *s, double wall_ms);

void pipeline_run(PipelineJob *jobs, int num_jobs,
                  int num_threads, PipelineStats *stats);

#endif
