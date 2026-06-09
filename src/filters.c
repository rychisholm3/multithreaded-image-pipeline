#include "filters.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

Image *filter_grayscale(const Image *src) {
    Image *dst = image_create(src->width, src->height, 1);
    if (!dst) return NULL;
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            const uint8_t *s = image_pixel(src, x, y);
            uint8_t *d = dst->data + y * dst->width + x;
            if (src->channels == 3)
                *d = (uint8_t)(0.299f * s[0] + 0.587f * s[1] + 0.114f * s[2]);
            else
                *d = *s;
        }
    }
    return dst;
}

Image *filter_gaussian_blur(const Image *src, int radius) {
    if (radius < 1) return image_clone(src);

    int    size   = 2 * radius + 1;
    float *kernel = malloc((size_t)size * sizeof(float));
    if (!kernel) return NULL;

    float sigma = (radius > 2) ? radius / 2.0f : 1.0f;
    float sum   = 0.0f;
    for (int i = 0; i < size; i++) {
        int d      = i - radius;
        kernel[i]  = expf(-(float)(d * d) / (2.0f * sigma * sigma));
        sum       += kernel[i];
    }
    for (int i = 0; i < size; i++) kernel[i] /= sum;

    int    w = src->width, h = src->height, c = src->channels;
    Image *tmp = image_create(w, h, c);
    Image *dst = image_create(w, h, c);
    if (!tmp || !dst) {
        free(kernel); image_free(tmp); image_free(dst); return NULL;
    }

    /* horizontal pass */
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            for (int ch = 0; ch < c; ch++) {
                float acc = 0.0f;
                for (int k = -radius; k <= radius; k++) {
                    int sx = x + k;
                    sx = sx < 0 ? 0 : (sx >= w ? w - 1 : sx);
                    acc += kernel[k + radius] *
                           src->data[(y * w + sx) * c + ch];
                }
                tmp->data[(y * w + x) * c + ch] = (uint8_t)(acc + 0.5f);
            }
        }
    }

    /* vertical pass */
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            for (int ch = 0; ch < c; ch++) {
                float acc = 0.0f;
                for (int k = -radius; k <= radius; k++) {
                    int sy = y + k;
                    sy = sy < 0 ? 0 : (sy >= h ? h - 1 : sy);
                    acc += kernel[k + radius] *
                           tmp->data[(sy * w + x) * c + ch];
                }
                dst->data[(y * w + x) * c + ch] = (uint8_t)(acc + 0.5f);
            }
        }
    }

    free(kernel);
    image_free(tmp);
    return dst;
}

Image *filter_sobel_edge(const Image *src) {
    /* convert to grayscale if needed */
    Image *gray = (src->channels == 1) ? image_clone(src) : filter_grayscale(src);
    if (!gray) return NULL;

    int    w   = gray->width, h = gray->height;
    Image *dst = image_create(w, h, 1);
    if (!dst) { image_free(gray); return NULL; }

    static const int GX[3][3] = { {-1,0,1},{-2,0,2},{-1,0,1} };
    static const int GY[3][3] = { {-1,-2,-1},{0,0,0},{1,2,1} };

    for (int y = 1; y < h - 1; y++) {
        for (int x = 1; x < w - 1; x++) {
            int sx = 0, sy = 0;
            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    int px = gray->data[(y + ky) * w + (x + kx)];
                    sx += GX[ky + 1][kx + 1] * px;
                    sy += GY[ky + 1][kx + 1] * px;
                }
            }
            int mag = (int)sqrtf((float)(sx * sx + sy * sy));
            dst->data[y * w + x] = (uint8_t)(mag > 255 ? 255 : mag);
        }
    }

    image_free(gray);
    return dst;
}

Image *filter_brightness(const Image *src, int delta) {
    Image *dst = image_clone(src);
    if (!dst) return NULL;
    int n = src->width * src->height * src->channels;
    for (int i = 0; i < n; i++) {
        int v        = src->data[i] + delta;
        dst->data[i] = (uint8_t)(v < 0 ? 0 : (v > 255 ? 255 : v));
    }
    return dst;
}

Image *filter_invert(const Image *src) {
    Image *dst = image_clone(src);
    if (!dst) return NULL;
    int n = src->width * src->height * src->channels;
    for (int i = 0; i < n; i++)
        dst->data[i] = 255 - src->data[i];
    return dst;
}
