/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/

#ifdef TUNE
#define NUM_IMGS __NUM_IMGS_FROM_TUNER
#define PIXELS_PER_IMG __PIXELS_PER_IMG_FROM_TUNER
#define DATA_ITEM_TYPE __DATA_ITEM_TYPE_FROM_TUNER
#else 
#define DATA_ITEM_TYPE float
#define NUM_IMGS 1000
#define PIXELS_PER_IMG 1024
#endif

typedef struct pixel_type {
    float r;
    float g;
    float b;
    float x;
  } pixel;


typedef struct arg_aos_struct_type {
  pixel *src;
  pixel *dst;
  int start_index;
  int end_index;
} args_aos;


typedef struct arg_da_struct_type {
  float *r;
  float *g;
  float *b; 
  float *x;
  float *d_r;
  float *d_g;
  float *d_b; 
  float *d_x;
  int start_index;
  int end_index;
} args_da;

#define ITERS 1

#define DEVICES 2
#define CPU_THREADS 4

#define THREADS PIXELS_PER_IMG
#define WORKGROUP 256 

#define STREAMS 8
#define FLOP 6                         // floating-point ops in one iteration of kernel loop

#define ERROR_THRESH NUM_IMGS * 0.01   // relaxed FP-precision checking 

#ifdef HETERO
#define HOST
#define DEVICE
#endif
