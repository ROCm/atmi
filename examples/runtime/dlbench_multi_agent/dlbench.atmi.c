/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include "atmi_runtime.h"
#include <dlbench.h>
//#define DEBUG

#define check(msg, status) \
if (status != ATMI_STATUS_SUCCESS) { \
    printf("%s failed.\n", #msg); \
    exit(1); \
} 
double mysecond() {
  struct timeval tp;
  int i;

  i = gettimeofday(&tp,NULL);
  return ( (double) tp.tv_sec + (double) tp.tv_usec * 1.e-6 );
}

void check_results_aos(pixel *src_images, pixel *dst_images, int host_start, int device_end) {

  int errors = 0;
  for (int j = 0; j < device_end * PIXELS_PER_IMG; j += PIXELS_PER_IMG)
    for (unsigned int i = j; i < j + PIXELS_PER_IMG; i++) {
      DATA_ITEM_TYPE exp_result = (0.3 * src_images[i].r + 0.59 * src_images[i].g
                                   + 0.11 * src_images[i].b + 1.0 * src_images[i].x);
#ifdef DEBUG
      if (i == 512)
	printf("%3.2f %3.2f\n", exp_result, dst_images[512].r);
#endif
      if (dst_images[i].r > exp_result + ERROR_THRESH ||
          dst_images[i].r < exp_result - ERROR_THRESH) {
        errors++;
#ifdef DEBUG
        if (errors < 10)
          printf("%d %f %f\n", i, exp_result, dst_images[i].r);
#endif
      }
    }
  fprintf(stderr, "%s\n", (errors > 0 ? "FAILED (GPU)" : "PASSED (GPU)"));
}

void check_results_da(DATA_ITEM_TYPE *r, DATA_ITEM_TYPE *g, DATA_ITEM_TYPE *b, DATA_ITEM_TYPE *x,
                      DATA_ITEM_TYPE *d_r, int host_start, int device_end) {

  int errors = 0;
  for (int j = 0; j < device_end * PIXELS_PER_IMG; j += PIXELS_PER_IMG)
    for (unsigned int i = j; i < j + PIXELS_PER_IMG; i++) {
      DATA_ITEM_TYPE exp_result =  (0.3 * r[i] + 0.59 * g[i] + 0.11 * b[i] + 1.0 * x[i]);
#ifdef DEBUG
      if (i == 512)
        printf("%f %f\n", exp_result, d_r[i]);
#endif
      if (d_r[i] > exp_result + ERROR_THRESH || d_r[i] < exp_result - ERROR_THRESH) {
        errors++;
#ifdef DEBUG
        printf("%f %f\n", exp_result, d_r[i]);
#endif
      }
    }
  fprintf(stderr, "%s\n", (errors > 0 ? "FAILED (GPU)" : "PASSED (GPU)"));
}

int main(int argc, char **argv) {
    int gpu_agents_used;
    if (argc < 2)
      gpu_agents_used = 1;
    else 
      gpu_agents_used = atoi(argv[1]);

    atmi_status_t err = atmi_init(ATMI_DEVTYPE_ALL);
    check(Initializing the ATMI runtime, err);
    
#ifdef MODULE_BRIG
    const char *module = "grayscale.brig";
    atmi_platform_type_t module_type = BRIG;
#else
    const char *module = "grayscale.hsaco";
    atmi_platform_type_t module_type = AMDGCN;
#endif
    err = atmi_module_register(&module, &module_type, 1);
    check(Registering modules, err);

    atmi_machine_t *machine = atmi_machine_get_info();
    /* Count available CPU and GPU agents */
    unsigned num_gpu_agents = machine->device_count_by_type[ATMI_DEVTYPE_GPU];
    unsigned num_cpu_agents = machine->device_count_by_type[ATMI_DEVTYPE_CPU];
    unsigned num_agents = num_cpu_agents + num_gpu_agents;
    int i = 0;
    printf("Number of available agents: %d\n", num_agents);
    if (num_gpu_agents < 1) {
      printf("No GPU agents found. Exiting");
      exit(0);
    }
    if (gpu_agents_used > num_gpu_agents) {
      printf("Too many GPU agents requested, setting to max available: %d.\n", num_gpu_agents);
      gpu_agents_used = num_gpu_agents;
    }
    printf("Number of available GPU agents: %d\n", num_gpu_agents);

    atmi_kernel_t kernel;
    const int GPU_IMPL = 42;
#ifdef AOS
    const unsigned int num_args = 3;
    size_t arg_sizes[] = {sizeof(void *), sizeof(void *), sizeof(int)};
    atmi_kernel_create_empty(&kernel, num_args, arg_sizes);
    atmi_kernel_add_gpu_impl(kernel, "grayscale_aos", GPU_IMPL);
#endif
#ifdef DA
    const unsigned int num_args = 9;
    size_t arg_sizes[] = {
                          sizeof(void *), sizeof(void *), 
                          sizeof(void *), sizeof(void *), 
                          sizeof(void *), sizeof(void *), 
                          sizeof(void *), sizeof(void *), 
                          sizeof(int)};
    atmi_kernel_create_empty(&kernel, num_args, arg_sizes);
    atmi_kernel_add_gpu_impl(kernel, "grayscale_da", GPU_IMPL);
#endif

    /* ********************************************
     * 
     * Data allocation and distribution code BEGIN 
     *
     *********************************************/

    int cpu_id = 0; // how about other CPU agents' memory pools? 
    atmi_mem_place_t *gpus = (atmi_mem_place_t *)malloc(sizeof(atmi_mem_place_t) * gpu_agents_used);
    for(int i = 0; i < gpu_agents_used; i++)
        gpus[i] = ATMI_MEM_PLACE(0, ATMI_DEVTYPE_GPU, i, 0);
    atmi_mem_place_t cpu_fine = ATMI_MEM_PLACE(0, ATMI_DEVTYPE_CPU, cpu_id, 0);
    atmi_mem_place_t cpu_coarse = ATMI_MEM_PLACE(0, ATMI_DEVTYPE_CPU, cpu_id, 1);
    int host_start = 0;
#ifdef AOS
    pixel *src_images[gpu_agents_used]; 
    pixel *dst_images[gpu_agents_used]; 
    
    unsigned items_per_device = NUM_IMGS / gpu_agents_used; 
    int trailing_items = NUM_IMGS % gpu_agents_used;
    unsigned long segment_size = items_per_device * PIXELS_PER_IMG * sizeof(pixel);
   
    for (i = 0; i < gpu_agents_used; i++) {
      if (i == gpu_agents_used - 1) {
	items_per_device = items_per_device + trailing_items;
	segment_size = items_per_device * PIXELS_PER_IMG * sizeof(pixel); 
      }
#if defined FINE || DEVMEM
      err = atmi_malloc((void **)&src_images[i], segment_size, cpu_fine);
      err = atmi_malloc((void **)&dst_images[i], segment_size, cpu_fine);
#endif
#ifdef COARSE
      err = atmi_malloc((void **)&src_images[i], segment_size, cpu_coarse);
      err = atmi_malloc((void **)&dst_images[i], segment_size, cpu_coarse);
#endif
      for (int j = 0; j < items_per_device * PIXELS_PER_IMG; j += PIXELS_PER_IMG)
	for (int k = j; k < j + PIXELS_PER_IMG; k++) {
	  src_images[i][k].r = (DATA_ITEM_TYPE)k;
	  src_images[i][k].g = k * 10.0f;
	  src_images[i][k].b = (DATA_ITEM_TYPE)k;
	  src_images[i][k].x = k * 10.0f;
	}
    }
    // reset for next phase 
    items_per_device =  NUM_IMGS / gpu_agents_used;
    segment_size = items_per_device * PIXELS_PER_IMG * sizeof(pixel);
#endif
#ifdef DA
    DATA_ITEM_TYPE *r[gpu_agents_used];
    DATA_ITEM_TYPE *g[gpu_agents_used];
    DATA_ITEM_TYPE *b[gpu_agents_used];
    DATA_ITEM_TYPE *x[gpu_agents_used];

    DATA_ITEM_TYPE *d_r[gpu_agents_used];
    DATA_ITEM_TYPE *d_g[gpu_agents_used];
    DATA_ITEM_TYPE *d_b[gpu_agents_used];
    DATA_ITEM_TYPE *d_x[gpu_agents_used];

    unsigned items_per_device = NUM_IMGS / gpu_agents_used; 
    int trailing_items = NUM_IMGS % gpu_agents_used;
    unsigned long segment_size = items_per_device * PIXELS_PER_IMG * sizeof(DATA_ITEM_TYPE);

    for (i = 0; i < gpu_agents_used; i++) {
      if (i == gpu_agents_used - 1) {
	items_per_device = items_per_device + trailing_items;
	segment_size = items_per_device * PIXELS_PER_IMG * sizeof(DATA_ITEM_TYPE);
      }
#if defined FINE || DEVMEM
      atmi_malloc((void **)&r[i], segment_size, cpu_fine);
      atmi_malloc((void **)&g[i], segment_size, cpu_fine);
      atmi_malloc((void **)&b[i], segment_size, cpu_fine);
      atmi_malloc((void **)&x[i], segment_size, cpu_fine);
      
      atmi_malloc((void **)&d_r[i], segment_size, cpu_fine);
      atmi_malloc((void **)&d_g[i], segment_size, cpu_fine);
      atmi_malloc((void **)&d_b[i], segment_size, cpu_fine);
      atmi_malloc((void **)&d_x[i], segment_size, cpu_fine);
#endif
#ifdef COARSE
      atmi_malloc((void **)&r[i], segment_size, cpu_coarse);
      atmi_malloc((void **)&g[i], segment_size, cpu_coarse);
      atmi_malloc((void **)&b[i], segment_size, cpu_coarse);
      atmi_malloc((void **)&x[i], segment_size, cpu_coarse);
      
      atmi_malloc((void **)&d_r[i], segment_size, cpu_coarse);
      atmi_malloc((void **)&d_g[i], segment_size, cpu_coarse);
      atmi_malloc((void **)&d_b[i], segment_size, cpu_coarse);
      atmi_malloc((void **)&d_x[i], segment_size, cpu_coarse);
#endif

      if (!r[i] || !g[i] || !b[i] || !g[i] || !d_r[i] || !d_g[i] || !d_b[i] || !d_x[i]) {
	printf("Unable to malloc discrete arrays to fine grain memory. Exiting\n");
	exit(0);
      }

      for (int j = 0; j < items_per_device * PIXELS_PER_IMG; j += PIXELS_PER_IMG)
	for (int k = j; k < j + PIXELS_PER_IMG; k++) {
	  r[i][k] = (DATA_ITEM_TYPE)k;
	  g[i][k] = k * 10.0f;
	  b[i][k] = (DATA_ITEM_TYPE)k;
	  x[i][k] = k * 10.0f;
	}
    }
    // reset this for rest of the program
    items_per_device =  NUM_IMGS / gpu_agents_used;
    segment_size = items_per_device * PIXELS_PER_IMG * sizeof(DATA_ITEM_TYPE);
#endif

#ifdef DEVMEM
#ifdef AOS
    pixel *dev_src_images[gpu_agents_used];
    pixel *dev_dst_images[gpu_agents_used];

    for (i = 0; i < gpu_agents_used; i++) {
      if (i == gpu_agents_used - 1) {
	items_per_device = items_per_device + trailing_items;
	segment_size = items_per_device * PIXELS_PER_IMG * sizeof(pixel); 
      }
      atmi_malloc((void **)&dev_src_images[i], segment_size, gpus[i]);
      atmi_malloc((void **)&dev_dst_images[i], segment_size, gpus[i]);
      if (!dev_src_images[i] || !dev_dst_images) {
	printf("Unable to malloc buffer to device memory. Exiting\n");
	exit(0);
      }
#ifdef VERBOSE
      printf("Successfully malloc'ed %lu bytes to host pool at %p and %p\n",
	     segment_size, src_images[i], dst_images[i]);
      printf("Successfully malloc'ed %lu bytes to device pool at %p and %p\n",
	     segment_size, dev_src_images[i], dev_dst_images[i]);
#endif
      atmi_memcpy(dev_src_images[i], src_images[i], segment_size);
    }
    // reset for next phase 
    items_per_device =  NUM_IMGS / gpu_agents_used;
    segment_size = items_per_device * PIXELS_PER_IMG * sizeof(pixel);
#endif

#ifdef DA

    DATA_ITEM_TYPE *dev_r[gpu_agents_used];
    DATA_ITEM_TYPE *dev_g[gpu_agents_used];
    DATA_ITEM_TYPE *dev_b[gpu_agents_used]; 
    DATA_ITEM_TYPE *dev_x[gpu_agents_used]; 

    DATA_ITEM_TYPE *dev_d_r[gpu_agents_used]; 
    DATA_ITEM_TYPE *dev_d_g[gpu_agents_used]; 
    DATA_ITEM_TYPE *dev_d_b[gpu_agents_used]; 
    DATA_ITEM_TYPE *dev_d_x[gpu_agents_used]; 

    for (i = 0; i < gpu_agents_used; i++) {
      if (i == gpu_agents_used - 1) {
	items_per_device = items_per_device + trailing_items;
	segment_size = items_per_device * PIXELS_PER_IMG * sizeof(DATA_ITEM_TYPE);
      }
      atmi_malloc((void **)&dev_r[i], segment_size, gpus[i]);
      atmi_malloc((void **)&dev_g[i], segment_size, gpus[i]);
      atmi_malloc((void **)&dev_b[i], segment_size, gpus[i]);
      atmi_malloc((void **)&dev_x[i], segment_size, gpus[i]);

      atmi_malloc((void **)&dev_d_r[i], segment_size, gpus[i]);
      atmi_malloc((void **)&dev_d_g[i], segment_size, gpus[i]);
      atmi_malloc((void **)&dev_d_b[i], segment_size, gpus[i]);
      atmi_malloc((void **)&dev_d_x[i], segment_size, gpus[i]);

      if (!dev_r[i] || !dev_g[i] || !dev_b[i] || !dev_g[i] || !dev_d_r[i] 
                    || !dev_d_g[i] || !dev_d_b[i] || !dev_d_x[i]) {
	printf("Unable to malloc discrete arrays to device memory. Exiting\n");
	exit(0);
      }
      
      atmi_memcpy(dev_r[i], r[i], segment_size); 
      atmi_memcpy(dev_g[i], g[i], segment_size); 
      atmi_memcpy(dev_b[i], b[i], segment_size); 
      atmi_memcpy(dev_x[i], x[i], segment_size); 
    }

    // reset this for rest of the program
    items_per_device =  NUM_IMGS / gpu_agents_used;
    segment_size = items_per_device * PIXELS_PER_IMG * sizeof(DATA_ITEM_TYPE);

#endif
#endif

    int ipd[gpu_agents_used];
#ifdef AOS
    void *args[gpu_agents_used][num_args];
    for (i = 0; i < gpu_agents_used; i++) {
      if (i == gpu_agents_used - 1)
	items_per_device = items_per_device + trailing_items;
#ifdef DEVMEM
      args[i][0] = &dev_src_images[i];
      args[i][1] = &dev_dst_images[i];
      ipd[i] = items_per_device;
      args[i][2] = &ipd[i];
#else
      args[i][0] = &src_images[i];
      args[i][1] = &dst_images[i];
      ipd[i] = items_per_device;
      args[i][2] = &ipd[i];
#endif
    }
    // reset this for rest of the program
    items_per_device =  NUM_IMGS / gpu_agents_used;
#endif
#ifdef DA
    void *args[gpu_agents_used][num_args];
    for (i = 0; i < gpu_agents_used; i++) {
      if (i == gpu_agents_used - 1)
	items_per_device = items_per_device + trailing_items;
#ifdef DEVMEM
      args[i][0] = &dev_r[i];
      args[i][1] = &dev_g[i];
      args[i][2] = &dev_b[i];
      args[i][3] = &dev_x[i];
      args[i][4] = &dev_d_r[i];
      args[i][5] = &dev_d_g[i];
      args[i][6] = &dev_d_b[i];
      args[i][7] = &dev_d_x[i];
      ipd[i] = items_per_device;
      args[i][8] = &ipd[i];
#else
      args[i][0] = &r[i];
      args[i][1] = &g[i];
      args[i][2] = &b[i];
      args[i][3] = &x[i];
      args[i][4] = &d_r[i];
      args[i][5] = &d_g[i];
      args[i][6] = &d_b[i];
      args[i][7] = &d_x[i];
      ipd[i] = items_per_device;
      args[i][8] = &ipd[i];
#endif
    }
    // reset this for rest of the program
    items_per_device =  NUM_IMGS / gpu_agents_used;
#endif

    ATMI_LPARM_1D(lparm, THREADS);
    lparm->groupDim[0] = WORKGROUP;
    lparm->kernel_id = GPU_IMPL;
    lparm->groupable = ATMI_TRUE;
    //lparm->synchronous = ATMI_TRUE;
    for (i = 0; i < gpu_agents_used; i++) {
      lparm->place = ATMI_PLACE_GPU(0, i);
      atmi_task_launch(lparm, kernel, args[i]);
    }

    double t = mysecond();
    atmi_task_group_sync(NULL);
    t = 1.0E6 * (mysecond() - t);

#ifdef DEVMEM
#ifdef AOS
    for (i = 0; i < gpu_agents_used; i++) {
      if (i == gpu_agents_used - 1) {
	items_per_device = items_per_device + trailing_items;
	segment_size = items_per_device * PIXELS_PER_IMG * sizeof(pixel); 
      }
      atmi_memcpy(dst_images[i], dev_dst_images[i], segment_size);
    }
    // reset for next phase 
    items_per_device =  NUM_IMGS / gpu_agents_used;
    segment_size = items_per_device * PIXELS_PER_IMG * sizeof(pixel);
#endif
#ifdef DA

    for (i = 0; i < gpu_agents_used; i++) {
      if (i == gpu_agents_used - 1) {
	items_per_device = items_per_device + trailing_items;
	segment_size = items_per_device * PIXELS_PER_IMG * sizeof(DATA_ITEM_TYPE);
      }
      /* Copy all device memory buffers back to host */
      atmi_memcpy(d_r[i], dev_d_r[i], segment_size);
      atmi_memcpy(d_g[i], dev_d_g[i], segment_size);
      atmi_memcpy(d_b[i], dev_d_b[i], segment_size);
      atmi_memcpy(d_x[i], dev_d_x[i], segment_size);
    }
    // reset this for rest of the program
    items_per_device =  NUM_IMGS / gpu_agents_used;
    segment_size = items_per_device * PIXELS_PER_IMG * sizeof(DATA_ITEM_TYPE);
#endif
#endif

#ifdef AOS
    for (i = 0; i < gpu_agents_used; i++) {
      if (i == gpu_agents_used - 1)
	items_per_device = items_per_device + trailing_items;
      check_results_aos(src_images[i], dst_images[i], host_start, items_per_device);
    }
#endif
#ifdef DA
    for (i = 0; i < gpu_agents_used; i++) {
      if (i == gpu_agents_used - 1)
	items_per_device = items_per_device + trailing_items;
      check_results_da(r[i], g[i], b[i], x[i], d_r[i], host_start, items_per_device);
    }
#endif

    /*   
     * Calculate performance metrics                                                                   
     */
    unsigned long dataMB = (NUM_IMGS * PIXELS_PER_IMG * sizeof(DATA_ITEM_TYPE) * STREAMS)/(1024 * 1024);
    double flop = (float) ((float) FLOP * (float) ITERS * (float) NUM_IMGS * (float) PIXELS_PER_IMG);
    double secs = t/1000000;

#ifdef VERBOSE
    fprintf(stdout, "Kernel execution time %3.2f ms\n", t/1000);
    fprintf(stdout, "Attained bandwidth: %3.2f MB/s\n", dataMB/secs);
    fprintf(stdout, "MFLOPS: %3.2f\n", (flop/secs)/(1024 * 1024));
    fprintf(stdout, "Arithmetic intensity (calculated): %3.2f FLOP/Byte\n", (flop/(dataMB * 1024 * 1024)));
#else
    fprintf(stdout, "%3.2f ", t/1000);
    fprintf(stdout, "%3.2f ", dataMB/secs);
    fprintf(stdout, "%3.2f ", (flop/secs)/(1024 * 1024));
    fprintf(stdout, "%3.2f\n", (flop/(dataMB * 1024 * 1024)));
#endif
    /*
     * Cleanup all allocated resources.
     */
    for (i = 0; i < gpu_agents_used; i++) {
#ifdef AOS
    atmi_free(src_images[i]);
    atmi_free(dst_images[i]);
#ifdef DEVMEM
    atmi_free(dev_src_images[i]);
    atmi_free(dev_dst_images[i]);
#endif
#endif

#ifdef DA
    atmi_free(r[i]);
    atmi_free(g[i]);
    atmi_free(b[i]);
    atmi_free(x[i]);
    atmi_free(d_r[i]);
    atmi_free(d_g[i]);
    atmi_free(d_b[i]);
    atmi_free(d_x[i]);
#endif
    }

    atmi_kernel_release(kernel);
    atmi_finalize();

    return 0;
}
