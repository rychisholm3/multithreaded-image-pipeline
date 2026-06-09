#ifndef FILTERS_H
#define FILTERS_H

#include "image.h"

/* Each function returns a newly allocated Image; caller must image_free() it.
   Returns NULL on allocation failure. */

Image *filter_grayscale(const Image *src);
Image *filter_gaussian_blur(const Image *src, int radius);
Image *filter_sobel_edge(const Image *src);   /* works on any input; converts to gray first */
Image *filter_brightness(const Image *src, int delta);
Image *filter_invert(const Image *src);

#endif
