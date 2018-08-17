/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/

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


