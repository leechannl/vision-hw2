#include "image.h"
#include "log.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void image_info(image im) {
  if (im.w * im.h * im.c >= 10) {
    log_info("image size: %dx%dx%d, first 10 pixels: %f, %f, %f, %f, %f, %f, %f, %f, %f, %f", im.w, im.h, im.c,
             im.data[0], im.data[1], im.data[2], im.data[3], im.data[4], im.data[5], im.data[6], im.data[7], im.data[8],
             im.data[9]);
  } else {
    log_info("image size: %dx%dx%d", im.w, im.h, im.c);
    for (int i = 0; i < im.w * im.h * im.c; i++) {
      printf("%f, ", im.data[i]);
    }
    printf("\n");
  }
}

float get_pixel(image im, int x, int y, int c) {
  // clamp padding strategy
  if (x >= im.w) { x = im.w - 1; }
  if (x < 0) { x = 0; }
  if (y >= im.h) { y = im.h - 1; }
  if (y < 0) { y = 0; }
  if (c >= im.c) { c = im.c - 1; }
  if (c < 0) { c = 0; }

  return im.data[x + im.w * y + im.w * im.h * c];
}

void set_pixel(image im, int x, int y, int c, float v) {
  // checking bounds
  if (x > im.w || x < 0 || y > im.h || y < 0 || c > im.c || c < 0) { return; }

  im.data[x + im.w * y + im.w * im.h * c] = v;
}

void set_pixels(image im, float values[], size_t s) {
  assert(im.w * im.h == s);
  for (int i = 0; i < s; i++) {
    im.data[i] = values[i];
  }
}

image copy_image_bounds(image im, int width, int height) {
  image copy = make_image(width, height, im.c);

  for (int i = 0; i < width; i++) {
    for (int j = 0; j < height; j++) {
      for (int k = 0; k < im.c; k++) {
        set_pixel(copy, i, j, k, get_pixel(im, i, j, k));
      }
    }
  }

  return copy;
}

image copy_image(image im) {
  image copy = make_image(im.w, im.h, im.c);

  for (int i = 0; i < im.w; i++) {
    for (int j = 0; j < im.h; j++) {
      for (int k = 0; k < im.c; k++) {
        set_pixel(copy, i, j, k, get_pixel(im, i, j, k));
      }
    }
  }

  return copy;
}

// Y' = 0.299 R' + 0.587 G' + .114 B'
image rgb_to_grayscale(image im) {
  assert(im.c == 3);
  image gray = make_image(im.w, im.h, 1);

  for (int i = 0; i < im.w; i++) {
    for (int j = 0; j < im.h; j++) {
      // log_debug("get_pixel(im, i, j, 0): %f, get_pixel(im, i, j, 1): %f, "
      //           "get_pixel(im, i, j, 2): %f",
      //           get_pixel(im, i, j, 0), get_pixel(im, i, j, 1),
      //           get_pixel(im, i, j, 2));
      float y = 0.299 * get_pixel(im, i, j, 0) + 0.587 * get_pixel(im, i, j, 1) + 0.114 * get_pixel(im, i, j, 2);
      set_pixel(gray, i, j, 0, y);
    }
  }

  return gray;
}

void shift_image(image im, int c, float v) {
  for (int i = 0; i < im.w; i++) {
    for (int j = 0; j < im.h; j++) {
      set_pixel(im, i, j, c, get_pixel(im, i, j, c) + v);
    }
  }
}

void scale_image(image im, int c, float v) {
  for (int i = 0; i < im.w; i++) {
    for (int j = 0; j < im.h; j++) {
      set_pixel(im, i, j, c, get_pixel(im, i, j, c) * v);
    }
  }
}

void clamp_image(image im) {
  for (int i = 0; i < im.w; i++) {
    for (int j = 0; j < im.h; j++) {
      for (int k = 0; k < im.c; k++) {
        if (get_pixel(im, i, j, k) > 1) { set_pixel(im, i, j, k, 1); }
        if (get_pixel(im, i, j, k) < 0) { set_pixel(im, i, j, k, 0); }
      }
    }
  }
}

// These might be handy
float three_way_max(float a, float b, float c) { return (a > b) ? ((a > c) ? a : c) : ((b > c) ? b : c); }

float three_way_min(float a, float b, float c) { return (a < b) ? ((a < c) ? a : c) : ((b < c) ? b : c); }

void rgb_to_hsv(image im) {
  for (int y = 0; y < im.h; y++) {
    for (int x = 0; x < im.w; x++) {
      float r = get_pixel(im, x, y, 0);
      float g = get_pixel(im, x, y, 1);
      float b = get_pixel(im, x, y, 2);

      float v = three_way_max(r, g, b);
      float m = three_way_min(r, g, b);

      float c = v - m;
      float s = 0;

      if (v > 0) s = c / v;

      float h_ = 0;
      if (c != 0) {
        if (v == r) h_ = (g - b) / c;
        if (v == g) h_ = (b - r) / c + 2;
        if (v == b) h_ = (r - g) / c + 4;
      }
      float h = h_ / 6;
      if (h_ < 0) h = h_ / 6 + 1;

      set_pixel(im, x, y, 0, h);
      set_pixel(im, x, y, 1, s);
      set_pixel(im, x, y, 2, v);
    }
  }
}

void hsv_to_rgb(image im) {
  for (int i = 0; i < im.w; i++) {
    for (int j = 0; j < im.h; j++) {
      float h = get_pixel(im, i, j, 0) * 360;
      float s = get_pixel(im, i, j, 1);
      float v = get_pixel(im, i, j, 2);

      float r = 0;
      float g = 0;
      float b = 0;
      float c = 0;
      float m = 0;
      float x = 0;

      c = s * v;
      x = c * (1 - fabs(fmod(h / 60, 2) - 1));
      m = v - c;

      if (h < 60) {
        r = c, g = x, b = 0;
      } else if (h < 120) {
        r = x, g = c, b = 0;
      } else if (h < 180) {
        r = 0, g = c, b = x;
      } else if (h < 240) {
        r = 0, g = x, b = c;
      } else if (h < 300) {
        r = x, g = 0, b = c;
      } else if (h < 360) {
        r = c, g = 0, b = x;
      } else {
        log_error("hue overflow!");
        exit(1);
      }

      set_pixel(im, i, j, 0, r + m);
      set_pixel(im, i, j, 1, g + m);
      set_pixel(im, i, j, 2, b + m);
    }
  }
}
