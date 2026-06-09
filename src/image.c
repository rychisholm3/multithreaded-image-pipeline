#include "image.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

Image *image_create(int width, int height, int channels) {
    Image *img = malloc(sizeof(Image));
    if (!img) return NULL;
    img->width    = width;
    img->height   = height;
    img->channels = channels;
    img->data     = calloc((size_t)(width * height * channels), 1);
    if (!img->data) { free(img); return NULL; }
    return img;
}

Image *image_clone(const Image *src) {
    Image *dst = image_create(src->width, src->height, src->channels);
    if (!dst) return NULL;
    memcpy(dst->data, src->data,
           (size_t)(src->width * src->height * src->channels));
    return dst;
}

void image_free(Image *img) {
    if (img) { free(img->data); free(img); }
}

Image *image_load_ppm(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    char magic[3];
    int  width, height, maxval;
    if (fscanf(f, "%2s %d %d %d ", magic, &width, &height, &maxval) != 4) {
        fclose(f); return NULL;
    }
    int channels = (magic[1] == '6') ? 3 : 1;
    Image *img = image_create(width, height, channels);
    if (!img) { fclose(f); return NULL; }

    size_t n = (size_t)(width * height * channels);
    if (fread(img->data, 1, n, f) != n) {
        image_free(img); fclose(f); return NULL;
    }
    fclose(f);
    return img;
}

int image_save_ppm(const Image *img, const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    const char *magic = (img->channels == 3) ? "P6" : "P5";
    fprintf(f, "%s\n%d %d\n255\n", magic, img->width, img->height);
    fwrite(img->data, 1,
           (size_t)(img->width * img->height * img->channels), f);
    fclose(f);
    return 0;
}

/* ---- synthetic generators ---- */

Image *image_gen_gradient(int width, int height) {
    Image *img = image_create(width, height, 3);
    if (!img) return NULL;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint8_t *p = image_pixel(img, x, y);
            p[0] = (uint8_t)(255 * x / (width  - 1));
            p[1] = (uint8_t)(255 * y / (height - 1));
            p[2] = 128;
        }
    }
    return img;
}

Image *image_gen_checkerboard(int width, int height, int cell_size) {
    Image *img = image_create(width, height, 3);
    if (!img) return NULL;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint8_t v = ((x / cell_size + y / cell_size) % 2) ? 240 : 20;
            uint8_t *p = image_pixel(img, x, y);
            p[0] = p[1] = p[2] = v;
        }
    }
    return img;
}

static uint32_t lcg_next(uint32_t *s) {
    *s = *s * 1664525u + 1013904223u;
    return *s;
}

Image *image_gen_noise(int width, int height, unsigned int seed) {
    Image *img = image_create(width, height, 3);
    if (!img) return NULL;
    uint32_t s = (uint32_t)seed;
    for (int i = 0; i < width * height * 3; i++)
        img->data[i] = (uint8_t)(lcg_next(&s) >> 24);
    return img;
}

Image *image_gen_circles(int width, int height) {
    Image *img = image_create(width, height, 3);
    if (!img) return NULL;
    float cx = width  / 2.0f;
    float cy = height / 2.0f;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float dx   = x - cx, dy = y - cy;
            float dist = sqrtf(dx*dx + dy*dy);
            float v    = (sinf(dist * 0.25f) + 1.0f) * 0.5f;
            uint8_t *p = image_pixel(img, x, y);
            p[0] = (uint8_t)(v * 255);
            p[1] = (uint8_t)((1.0f - v) * 200);
            p[2] = (uint8_t)(128 + v * 127);
        }
    }
    return img;
}
