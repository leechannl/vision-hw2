#include "image.h"
#include "log.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define TWOPI 6.2831853

image make_ones_image(int w, int h, int c) {
  image im = make_image(w, h, c);
  for (int i = 0; i < im.w * im.h * im.c; i++) {
    im.data[i] = 1;
  }
  return im;
}

void l1_normalize(image im) {
  float sum = 0;
  for (int i = 0; i < (im.w * im.h * im.c); i++) {
    sum += im.data[i];
  }

  for (int i = 0; i < im.w; i++) {
    for (int j = 0; j < im.h; j++) {
      for (int k = 0; k < im.c; k++) {
        set_pixel(im, i, j, k, get_pixel(im, i, j, k) / sum);
      }
    }
  }
}

image make_box_filter(int w) {
  image im = make_ones_image(w, w, 1);
  l1_normalize(im);
  return im;
}

float get_conv(image im, int col, int row, int chn, image f, int f_chn) {
  assert(f_chn < f.c || f_chn >= 0);
  float v = 0;

  for (int i = 0; i < f.w; i++) {
    for (int j = 0; j < f.h; j++) {
      v += get_pixel(f, i, j, f_chn) * get_pixel(im, col - f.w / 2 + i, row - f.h / 2 + j, chn);
    }
  }

  return v;
}

image convolve_image(image im, image filter, int preserve) {
  assert(filter.c == 1 || filter.c == im.c);
  image new_im;
  if (filter.c == im.c) {
    if (preserve != 1) { // normal convolution, generate a 1 channel image
      new_im = make_image(im.w, im.h, 1);
      for (int i = 0; i < im.w; i++) {
        for (int j = 0; j < im.h; j++) {
          float v = 0;
          for (int k = 0; k < im.c; k++) {
            v += get_conv(im, i, j, k, filter, k);
          }
          set_pixel(new_im, i, j, 0, v);
        }
      }
    } else { // preserve channels, keep original channel number
      float v = 0;
      new_im = make_image(im.w, im.h, im.c);
      for (int i = 0; i < im.w; i++) {
        for (int j = 0; j < im.h; j++) {
          for (int k = 0; k < im.c; k++) {
            v = get_conv(im, i, j, k, filter, k);
            set_pixel(new_im, i, j, k, v);
          }
        }
      }
    }
  } else {               // filter.c == 1
    if (preserve != 1) { // generate a 1 channel image
      new_im = make_image(im.w, im.h, 1);
      for (int i = 0; i < im.w; i++) {
        for (int j = 0; j < im.h; j++) {
          float v = 0;
          for (int k = 0; k < im.c; k++) {
            v += get_conv(im, i, j, k, filter, 0);
          }
          set_pixel(new_im, i, j, 0, v);
        }
      }
    } else { // keep orginal channel number
      float v = 0;
      new_im = make_image(im.w, im.h, im.c);
      for (int i = 0; i < im.w; i++) {
        for (int j = 0; j < im.h; j++) {
          for (int k = 0; k < im.c; k++) {
            v = get_conv(im, i, j, k, filter, 0);
            set_pixel(new_im, i, j, k, v);
          }
        }
      }
    }
  }

  return new_im;
}

image make_highpass_filter() {
  //  0 -1  0
  // -1  4 -1
  //  0 -1  0
  image f = make_image(3, 3, 1);
  float values[] = {0, -1, 0, -1, 4, -1, 0, -1, 0};
  set_pixels(f, values, ARRAY_SIZE(values));
  return f;
}

image make_sharpen_filter() {
  //  0 -1  0
  // -1  5 -1
  //  0 -1  0
  image f = make_image(3, 3, 1);
  float values[] = {0, -1, 0, -1, 5, -1, 0, -1, 0};
  set_pixels(f, values, ARRAY_SIZE(values));
  return f;
}

image make_emboss_filter() {
  // -2 -1  0
  // -1  1  1
  //  0  1  2
  image f = make_image(3, 3, 1);
  float values[] = {-2, -1, 0, -1, 1, 1, 0, 1, 2};
  set_pixels(f, values, ARRAY_SIZE(values));
  return f;
}

image make_vemboss_filter() {
  //  0  1  0
  //  0  1  0
  //  0 -1  0
  image f = make_image(3, 3, 1);
  float values[] = {0, 1, 0, 0, 1, 0, 0, -1, 0};
  set_pixels(f, values, ARRAY_SIZE(values));
  return f;
}

image make_hemboss_filter() {
  //  0  0  0
  // -1  1  1
  //  0  0  0
  image f = make_image(3, 3, 1);
  float values[] = {0, 0, 0, -1, 1, 1, 0, 0, 0};
  set_pixels(f, values, ARRAY_SIZE(values));
  return f;
}

// Question 2.2.1: Which of these filters should we use preserve when we run our convolution and which ones should we
// not? Why?
// Answer:
//    Kernel sum to 1, then the original color will be preseved, so `preserve` should be 1, otherwise be 0.
//    So for highpass filter `preserve` be 0, others be 1.

// Question 2.2.2: Do we have to do any post-processing for the above filters? Which ones and why?
// Answer:
//    Need to do clamp image. Because after doing convolution, the pixel value is likely greater than 1.
//    But if all coefficients is positive and sum to 1, then the pixel value will not greater than 1(nomal weighted
//    sum). So Gaussian filter no need to clamp.

image make_gaussian_filter(float sigma) {
  // g(x,y) = 1/(TWO_PI*pow(sigma,2)) * exp(-( (pow(x-mean,2)+pow(y-mean,2)) / (2*pow(sigma,2)) ))
  // g(x,y) = multiplier * exp(exponent)
  // multiplier = 1/(TWO_PI*pow(sigma,2))
  // exponent = -( (pow(x-mean,2)+pow(y-mean,2)) / (2*pow(sigma,2)) )

  int six_sigma = (int)ceilf(sigma * 6);
  int size = six_sigma % 2 ? six_sigma : six_sigma + 1;

  image f = make_image(size, size, 1);

  float multiplier = 1 / (TWOPI * sigma * sigma);
  int mean = size / 2;

  for (int i = 0; i < size; i++) {
    for (int j = 0; j < size; j++) {
      float exponent = -(pow(i - mean, 2) + pow(j - mean, 2)) / (2 * sigma * sigma);
      float v = multiplier * exp(exponent);
      set_pixel(f, i, j, 0, v);
    }
  }
  l1_normalize(f);
  return f;
}

image add_image(image a, image b) {
  assert(a.w == b.w);
  assert(a.h == b.h);
  assert(a.c == b.c);
  image new_im = make_image(a.w, a.h, a.c);
  for (int i = 0; i < a.w; i++) {
    for (int j = 0; j < a.h; j++) {
      for (int k = 0; k < a.c; k++) {
        set_pixel(new_im, i, j, k, get_pixel(a, i, j, k) + get_pixel(b, i, j, k));
      }
    }
  }
  return new_im;
}

image sub_image(image a, image b) {
  assert(a.w == b.w);
  assert(a.h == b.h);
  assert(a.c == b.c);
  image new_im = make_image(a.w, a.h, a.c);
  for (int i = 0; i < a.w; i++) {
    for (int j = 0; j < a.h; j++) {
      for (int k = 0; k < a.c; k++) {
        set_pixel(new_im, i, j, k, get_pixel(a, i, j, k) - get_pixel(b, i, j, k));
      }
    }
  }
  return new_im;
}

image make_gx_filter() {
  // -1  0  1
  // -2  0  2
  // -1  0  1
  image f = make_image(3, 3, 1);
  float values[] = {-1, 0, 1, -2, 0, 2, -1, 0, 1};
  set_pixels(f, values, ARRAY_SIZE(values));
  return f;
}

image make_gy_filter() {
  // -1 -2 -1
  //  0  0  0
  //  1  2  1
  image f = make_image(3, 3, 1);
  float values[] = {-1, -2, -1, 0, 0, 0, 1, 2, 1};
  set_pixels(f, values, ARRAY_SIZE(values));
  return f;
}

void feature_normalize(image im) {
  float max = im.data[0];
  float min = im.data[0];
  for (int i = 0; i < im.w * im.h * im.c; i++) {
    if (im.data[i] > max) { max = im.data[i]; }
    if (im.data[i] < min) { min = im.data[i]; }
  }

  float range = max - min;
  if (range == 0) {
    for (int i = 0; i < im.w * im.h * im.c; i++) {
      im.data[i] = 0;
    }
  } else {
    for (int i = 0; i < im.w * im.h * im.c; i++) {
      im.data[i] = (im.data[i] - min) / range;
    }
  }
}

image *sobel_image(image im) {
  // magnitude = sqrt(gx*gx + gy*gy)
  // direction = atanf(gy/gx)

  image *result = calloc(2, sizeof(image));
  result[0] = make_image(im.w, im.h, 1);
  result[1] = make_image(im.w, im.h, 1);
  image gx = convolve_image(im, make_gx_filter(), 0);
  image gy = convolve_image(im, make_gy_filter(), 0);
  for (int i = 0; i < im.w * im.h; i++) {
    float vx = gx.data[i];
    float vy = gy.data[i];
    result[0].data[i] = sqrtf(vx * vx + vy * vy);
    result[1].data[i] = atan2(vy, vx);
  }

  return result;
}

image colorize_sobel(image im) {
  image *result = sobel_image(im);
  feature_normalize(result[0]);
  feature_normalize(result[1]);
  im = make_image(im.w, im.h, im.c);
  for (int i = 0; i < im.w; i++) {
    for (int j = 0; j < im.h; j++) {
      set_pixel(im, i, j, 0, get_pixel(result[0], i, j, 0));
      set_pixel(im, i, j, 1, get_pixel(result[0], i, j, 0));
      set_pixel(im, i, j, 2, get_pixel(result[1], i, j, 0));
    }
  }
  hsv_to_rgb(im);
  return im;
}
