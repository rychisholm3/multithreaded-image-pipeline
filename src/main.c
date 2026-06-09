#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "image.h"
#include "pipeline.h"

#ifdef _WIN32
#  include <direct.h>
#  define make_dir(p) _mkdir(p)
#else
#  include <sys/stat.h>
#  define make_dir(p) mkdir((p), 0755)
#endif

static double mono_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

static void generate_inputs(void) {
    printf("--- Generating synthetic input images ---\n");

    struct { const char *path; Image *(*gen)(void); } table[] = {
        /* generated inline via lambdas won't work in C, call helpers */
    };
    (void)table;

    struct { const char *path; Image *img; } imgs[] = {
        { "input/gradient.ppm",         image_gen_gradient(512, 512)           },
        { "input/checkerboard.ppm",      image_gen_checkerboard(512, 512, 32)   },
        { "input/noise_a.ppm",           image_gen_noise(512, 512, 42)          },
        { "input/noise_b.ppm",           image_gen_noise(512, 512, 137)         },
        { "input/circles.ppm",           image_gen_circles(512, 512)            },
        { "input/gradient_large.ppm",    image_gen_gradient(1024, 1024)         },
        { "input/circles_large.ppm",     image_gen_circles(1024, 1024)          },
        { "input/noise_small.ppm",       image_gen_noise(256, 256, 999)         },
        { "input/checkerboard_lg.ppm",   image_gen_checkerboard(800, 600, 40)   },
        { "input/noise_c.ppm",           image_gen_noise(640, 480, 314159)      },
    };

    int n = (int)(sizeof(imgs) / sizeof(imgs[0]));
    for (int i = 0; i < n; i++) {
        if (!imgs[i].img) { fprintf(stderr, "  OOM generating %s\n", imgs[i].path); continue; }
        image_save_ppm(imgs[i].img, imgs[i].path);
        printf("  %s  (%dx%d)\n",
               imgs[i].path, imgs[i].img->width, imgs[i].img->height);
        image_free(imgs[i].img);
    }
    printf("\n");
}

int main(int argc, char *argv[]) {
    int num_threads = 4;
    if (argc > 1) {
        num_threads = atoi(argv[1]);
        if (num_threads < 1)  num_threads = 1;
        if (num_threads > 32) num_threads = 32;
    }

    printf("╔══════════════════════════════════════╗\n");
    printf("║   Multithreaded Image Pipeline       ║\n");
    printf("║   threads: %-3d                       ║\n", num_threads);
    printf("╚══════════════════════════════════════╝\n\n");

    make_dir("input");
    make_dir("output");

    generate_inputs();

    /* Each job: input path, output path, ordered list of filter specs */
    PipelineJob jobs[] = {
        {
            .id = 1,
            .input_path  = "input/gradient.ppm",
            .output_path = "output/gradient_edges.ppm",
            .filters     = { {FILTER_GRAYSCALE},
                             {FILTER_GAUSSIAN_BLUR, .param=3},
                             {FILTER_SOBEL_EDGE} },
            .num_filters = 3,
        },
        {
            .id = 2,
            .input_path  = "input/checkerboard.ppm",
            .output_path = "output/checkerboard_blurred.ppm",
            .filters     = { {FILTER_GAUSSIAN_BLUR, .param=6} },
            .num_filters = 1,
        },
        {
            .id = 3,
            .input_path  = "input/noise_a.ppm",
            .output_path = "output/noise_a_edges.ppm",
            .filters     = { {FILTER_GAUSSIAN_BLUR, .param=2},
                             {FILTER_SOBEL_EDGE} },
            .num_filters = 2,
        },
        {
            .id = 4,
            .input_path  = "input/noise_b.ppm",
            .output_path = "output/noise_b_inverted.ppm",
            .filters     = { {FILTER_INVERT} },
            .num_filters = 1,
        },
        {
            .id = 5,
            .input_path  = "input/circles.ppm",
            .output_path = "output/circles_edges.ppm",
            .filters     = { {FILTER_GRAYSCALE},
                             {FILTER_GAUSSIAN_BLUR, .param=2},
                             {FILTER_SOBEL_EDGE} },
            .num_filters = 3,
        },
        {
            .id = 6,
            .input_path  = "input/gradient_large.ppm",
            .output_path = "output/gradient_large_gray_blur.ppm",
            .filters     = { {FILTER_BRIGHTNESS, .param=30},
                             {FILTER_GAUSSIAN_BLUR, .param=5},
                             {FILTER_GRAYSCALE} },
            .num_filters = 3,
        },
        {
            .id = 7,
            .input_path  = "input/circles_large.ppm",
            .output_path = "output/circles_large_edges.ppm",
            .filters     = { {FILTER_GAUSSIAN_BLUR, .param=4},
                             {FILTER_SOBEL_EDGE} },
            .num_filters = 2,
        },
        {
            .id = 8,
            .input_path  = "input/noise_small.ppm",
            .output_path = "output/noise_small_denoised.ppm",
            .filters     = { {FILTER_GAUSSIAN_BLUR, .param=3},
                             {FILTER_BRIGHTNESS, .param=-20} },
            .num_filters = 2,
        },
        {
            .id = 9,
            .input_path  = "input/checkerboard_lg.ppm",
            .output_path = "output/checker_lg_edges.ppm",
            .filters     = { {FILTER_GRAYSCALE},
                             {FILTER_GAUSSIAN_BLUR, .param=1},
                             {FILTER_SOBEL_EDGE} },
            .num_filters = 3,
        },
        {
            .id = 10,
            .input_path  = "input/noise_c.ppm",
            .output_path = "output/noise_c_full_pipeline.ppm",
            .filters     = { {FILTER_GAUSSIAN_BLUR, .param=3},
                             {FILTER_BRIGHTNESS, .param=40},
                             {FILTER_GRAYSCALE},
                             {FILTER_SOBEL_EDGE} },
            .num_filters = 4,
        },
    };

    int num_jobs = (int)(sizeof(jobs) / sizeof(jobs[0]));

    PipelineStats stats;
    pipeline_stats_init(&stats);

    printf("--- Processing ---\n");
    double t0 = mono_ms();
    pipeline_run(jobs, num_jobs, num_threads, &stats);
    double wall_ms = mono_ms() - t0;

    pipeline_stats_print(&stats, wall_ms);
    pthread_mutex_destroy(&stats.lock);

    printf("Output written to: output/\n");
    return (stats.jobs_failed > 0) ? 1 : 0;
}
