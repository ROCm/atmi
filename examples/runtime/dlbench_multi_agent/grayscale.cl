/*
MIT License

Copyright © 2016 Advanced Micro Devices, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software
without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "dlbench.h"

__kernel void grayscale_aos(__global pixel *src_images, __global pixel *dst_images, int num_imgs) {
  int i = get_global_id(0);
  DATA_ITEM_TYPE gs;
  for (int k = 0; k < ITERS; k++) {
    for (int j = 0; j < num_imgs * PIXELS_PER_IMG; j = j + PIXELS_PER_IMG) {
      gs = (0.3 * src_images[i + j].r + 0.59 *
	    src_images[i + j].g + 0.11 * src_images[i + j].b + 1.0 *
	    src_images[i + j].x);
      dst_images[i + j].r = gs;
      dst_images[i + j].g = gs;
      dst_images[i + j].b = gs;
      dst_images[i + j].x = gs;
    }
  }
}


__kernel void grayscale_da(__global DATA_ITEM_TYPE *r, __global DATA_ITEM_TYPE *g, 
			   __global DATA_ITEM_TYPE *b, __global DATA_ITEM_TYPE *x, __global DATA_ITEM_TYPE *d_r, 
			   __global DATA_ITEM_TYPE *d_g, __global
			   DATA_ITEM_TYPE *d_b, __global DATA_ITEM_TYPE *d_x,
			   int num_imgs) {
  size_t i = get_global_id(0);
  DATA_ITEM_TYPE gs;
  for (int k = 0; k < ITERS; k++) {
    for (int j = 0; j < num_imgs * PIXELS_PER_IMG; j = j + PIXELS_PER_IMG) {
      gs = (0.3 * r[i + j] + 0.59 * g[i + j] + 0.11 * b[i + j] + 1.0 * x[i + j]);
      d_r[i + j] = gs;
      d_g[i + j] = gs;
      d_b[i + j] = gs;
      d_x[i + j] = gs;
    }
  }
}


