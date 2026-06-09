#ifndef IMAGE_H
#define IMAGE_H

#include <stdint.h>

typedef struct {
    int     width;
    int     height;
    int     channels;   /* 1 = grayscale, 3 = RGB */
    uint8_t *data;
} Image;

Image  *image_create(int width, int height, int channels);
Image  *image_clone(const Image *src);
Image  *image_load_ppm(const char *path);
int     image_save_ppm(const Image *img, const char *path);
void    image_free(Image *img);

/* Synthetic test image generators */
Image  *image_gen_gradient(int width, int height);
Image  *image_gen_checkerboard(int width, int height, int cell_size);
Image  *image_gen_noise(int width, int height, unsigned int seed);
Image  *image_gen_circles(int width, int height);

static inline uint8_t *image_pixel(const Image *img, int x, int y) {
    return img->data + (y * img->width + x) * img->channels;
}

#endif
