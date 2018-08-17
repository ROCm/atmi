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
#include <hsa.h>
#include <hsa_ext_finalize.h>
#include <mapi.h>
#include <dlbench.h>

#define check(msg, status) \
if (status != HSA_STATUS_SUCCESS) { \
    printf("%s failed.\n", #msg); \
    exit(1); \
} 
double mysecond() {
  struct timeval tp;
  int i;

  i = gettimeofday(&tp,NULL);
  return ( (double) tp.tv_sec + (double) tp.tv_usec * 1.e-6 );
}

/*                                                                                                
 * Loads a BRIG module from a specified file. This                                               
 * function does not validate the module.                                                        
 */
int load_module_from_file(const char* file_name, hsa_ext_module_t* module) {
  int rc = -1;
  FILE *fp = fopen(file_name, "rb");
  if (!fp) {
    printf("Could not fine module file. Exiting\n");
    exit(0);
  }

  rc = fseek(fp, 0, SEEK_END);
  size_t file_size = (size_t) (ftell(fp) * sizeof(char));
  rc = fseek(fp, 0, SEEK_SET);
  char* buf = (char*) malloc(file_size);
  memset(buf,0,file_size);
  size_t read_size = fread(buf,sizeof(char),file_size,fp);

  if(read_size != file_size) {
    free(buf);
  } else {
    rc = 0;
    *module = (hsa_ext_module_t) buf;
  }

  fclose(fp);

  return rc;
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

    hsa_status_t err;
    int i = 0;

    err = hsa_init();
    check(Initializing the hsa runtime, err);

    /* Count available CPU and GPU agents */
    unsigned num_agents = 0;
    err = hsa_iterate_agents(count_agents, &num_agents);
    check(Getting number of agents, err);
    printf("Number of available agents: %d\n", num_agents);

    unsigned num_gpu_agents = 0;
    err = hsa_iterate_agents(count_gpu_agents, &num_gpu_agents);
    check(Getting number of agents, err);
    if (num_gpu_agents < 1) {
      printf("No GPU agents found. Exiting");
      exit(0);
    }
    if (gpu_agents_used > num_gpu_agents) {
      printf("Too many GPU agents requested, setting to max available: %d.\n", num_gpu_agents);
      gpu_agents_used = num_gpu_agents;
    }
    printf("Number of available GPU agents: %d\n", num_gpu_agents);
    
    /* Get a handle on all GPU agents (whether we use them or not) */
    hsa_agent_t agent;
    hsa_agent_t agents[num_agents];
    err = hsa_iterate_agents(get_all_gpu_agents, &agents);

    /* Get a handle on CPU agents. Need CPU agents for coarse-grain allocation */
    hsa_agent_t cpu_agents[num_agents];
    err = hsa_iterate_agents(get_all_cpu_agents, &cpu_agents);
    check(Getting CPU agents, err);

    /*
     * Determine if the finalizer 1.0 extension is supported.
     */
    bool support;
    err = hsa_system_extension_supported(HSA_EXTENSION_FINALIZER, 1, 0, &support);
    check(Checking finalizer 1.0 extension support, err);

    /*
     * Generate the finalizer function table.
     */
    hsa_ext_finalizer_1_00_pfn_t table_1_00;
    err = hsa_system_get_extension_table(HSA_EXTENSION_FINALIZER, 1, 0, &table_1_00);
    check(Generating function table for finalizer, err);

    /*
     * Query maximum size queue for each GPU agent.
     */    
    uint32_t queue_sizes[gpu_agents_used];
    for (i = 0; i < gpu_agents_used; i++) {
      err = hsa_agent_get_info(agents[i], HSA_AGENT_INFO_QUEUE_MAX_SIZE, &queue_sizes[i]);
      check(Querying the agent maximum queue size, err);
    }

    /*
     * Create queues using the maximum size.
     */
    hsa_queue_t* queues[gpu_agents_used]; 
    for (i = 0; i < gpu_agents_used; i++) {
      err = hsa_queue_create(agents[i], queue_sizes[i], HSA_QUEUE_TYPE_SINGLE, 
			     NULL, NULL, UINT32_MAX, UINT32_MAX, &queues[i]);
      check(Creating queues, err);
    }

    /*
     * Obtain GPU machine model
     */
    hsa_machine_model_t machine_models[gpu_agents_used];
    for (i = 0; i < gpu_agents_used; i++) {
      err = hsa_agent_get_info(agents[i], HSA_AGENT_INFO_MACHINE_MODEL, &machine_models[i]);
      check("Obtaining machine model",err);
    }

    /*
     * Obtain agent profile (profiles not used as we cannot generate code for base profile)
     */
    hsa_profile_t profiles[gpu_agents_used];
    for (i = 0; i < gpu_agents_used; i++) {
      err = hsa_agent_get_info(agents[i], HSA_AGENT_INFO_PROFILE, &profiles[i]);
      check("Getting agent profile",err);
    }

    /*
     * Load the BRIG binary.
     */
    hsa_ext_module_t module; 
    load_module_from_file("grayscale.brig", &module);

    /*
     * Create hsa program.
     */
    hsa_ext_program_t program;
    memset(&program, 0, sizeof(hsa_ext_program_t));
    err = table_1_00.hsa_ext_program_create(HSA_MACHINE_MODEL_LARGE, HSA_PROFILE_FULL, 
					    HSA_DEFAULT_FLOAT_ROUNDING_MODE_DEFAULT, 
					    NULL, &program);
    check(Create the program, err);


    /* 
     * Add module to program 
     */
    err = table_1_00.hsa_ext_program_add_module(program, module);

    /*
     * Determine the agents ISA.
     */
    hsa_isa_t isas[gpu_agents_used];
    for (i = 0; i < gpu_agents_used; i++) {
      err = hsa_agent_get_info(agents[i], HSA_AGENT_INFO_ISA, &isas[i]);
      check(Query the agents isa, err);
    }
    /*
     * Finalize the program and extract the code object.
     */
    hsa_ext_control_directives_t control_directives[gpu_agents_used];
    hsa_code_object_t code_objects[gpu_agents_used];

    /* must create multiple code objects, otherwise cannot execute on multiple agents */
    for (i = 0; i < gpu_agents_used; i++) {
      memset(&control_directives[i], 0, sizeof(hsa_ext_control_directives_t));
      err = table_1_00.hsa_ext_program_finalize(program, isas[i], 0, control_directives[i], 
						"", HSA_CODE_OBJECT_TYPE_PROGRAM, &code_objects[i]);
      check(Finalizing the program, err);
    }

    /*
     * Destroy the program, it is no longer needed.
     */
    err=table_1_00.hsa_ext_program_destroy(program);
      
    /*
     * Create the empty executable.
     */
    hsa_executable_t executables[gpu_agents_used];
    for (i = 0; i < gpu_agents_used; i++) {
      err = hsa_executable_create(HSA_PROFILE_FULL, HSA_EXECUTABLE_STATE_UNFROZEN, "", &executables[i]);
      check(Create the executable, err);
    }
    /*
     * Load the code object.
     */
    for (i = 0; i < gpu_agents_used; i++) {
      err = hsa_executable_load_code_object(executables[i], agents[i], code_objects[i], "");
      check(Loading the code object, err);
    }

    /*
     * Freeze the executable; it can now be queried for symbols.
     */
    for (i = 0; i < gpu_agents_used; i++) {
      err = hsa_executable_freeze(executables[i], "");
      check(Freeze the executable, err);
    }

   /*
    * Extract the symbol from the executable.
    */
    hsa_executable_symbol_t symbols[gpu_agents_used];
    for (i = 0; i < gpu_agents_used; i++) {
#ifdef AOS
      err = hsa_executable_get_symbol(executables[i], NULL, "&__OpenCL_grayscale_aos_kernel", 
				      agents[i], 0, &symbols[i]);
#endif
#ifdef DA
      err = hsa_executable_get_symbol(executables[i], NULL, "&__OpenCL_grayscale_da_kernel", 
				      agents[i], 0, &symbols[i]);
#endif
      check(Extract the symbol from the executable, err);
    }
    /*
     * Extract dispatch information from the symbol
     */
    uint64_t kernel_objects[i];
    uint32_t kernarg_segment_sizes[i];
    uint32_t group_segment_sizes[i];
    uint32_t private_segment_sizes[i];

    for (i = 0; i < gpu_agents_used; i++) {
      err = hsa_executable_symbol_get_info(symbols[i], HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_OBJECT, 
					   &kernel_objects[i]);
      check(Extracting the symbol from the executable, err);
      err = hsa_executable_symbol_get_info(symbols[i], 
					   HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_KERNARG_SEGMENT_SIZE, 
					   &kernarg_segment_sizes[i]);
      check(Extracting the kernarg segment size from the executable, err);
      err = hsa_executable_symbol_get_info(symbols[i], 
					   HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_GROUP_SEGMENT_SIZE, 
					   &group_segment_sizes[i]);
      check(Extracting the group segment size from the executable, err);
      err = hsa_executable_symbol_get_info(symbols[i], 
					   HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_PRIVATE_SEGMENT_SIZE, 
					   &private_segment_sizes[i]);
      check(Extracting the private segment from the executable, err);
    }

    /*
     * Create signals to wait for the dispatch to finish.
     */ 
    hsa_signal_t signals[gpu_agents_used];
    for (i = 0; i < gpu_agents_used; i++) {
      err=hsa_signal_create(1, 0, NULL, &signals[i]);
      check(Creating a HSA signal, err);
    }

    /* ********************************************
     * 
     * Data allocation and distribution code BEGIN 
     *
     *********************************************/

    int host_start = 0;
    hsa_signal_value_t value;
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
      src_images[i] = (pixel *) malloc_fine_grain_agent(agents[i], segment_size);
      dst_images[i] = (pixel *) malloc_fine_grain_agent(agents[i], segment_size);
#endif
#ifdef COARSE
      src_images[i] = (pixel *) malloc_coarse_grain_agent(cpu_agents[0], segment_size);
      dst_images[i] = (pixel *) malloc_coarse_grain_agent(cpu_agents[0], segment_size);
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
      r[i] = (DATA_ITEM_TYPE *) malloc_fine_grain_agent(agents[i], segment_size);
      g[i] = (DATA_ITEM_TYPE *) malloc_fine_grain_agent(agents[i], segment_size);
      b[i] = (DATA_ITEM_TYPE *) malloc_fine_grain_agent(agents[i], segment_size);
      x[i] = (DATA_ITEM_TYPE *) malloc_fine_grain_agent(agents[i], segment_size);
      
      d_r[i] = (DATA_ITEM_TYPE *) malloc_fine_grain_agent(agents[i], segment_size);
      d_g[i] = (DATA_ITEM_TYPE *) malloc_fine_grain_agent(agents[i], segment_size);
      d_b[i] = (DATA_ITEM_TYPE *) malloc_fine_grain_agent(agents[i], segment_size);
      d_x[i] = (DATA_ITEM_TYPE *) malloc_fine_grain_agent(agents[i], segment_size);
#endif
#ifdef COARSE
      r[i] = (DATA_ITEM_TYPE *) malloc_fine_grain_agent(cpu_agents[0], segment_size);
      g[i] = (DATA_ITEM_TYPE *) malloc_fine_grain_agent(cpu_agents[0], segment_size);
      b[i] = (DATA_ITEM_TYPE *) malloc_fine_grain_agent(cpu_agents[0], segment_size);
      x[i] = (DATA_ITEM_TYPE *) malloc_fine_grain_agent(cpu_agents[0], segment_size);
      
      d_r[i] = (DATA_ITEM_TYPE *) malloc_fine_grain_agent(cpu_agents[0], segment_size);
      d_g[i] = (DATA_ITEM_TYPE *) malloc_fine_grain_agent(cpu_agents[0], segment_size);
      d_b[i] = (DATA_ITEM_TYPE *) malloc_fine_grain_agent(cpu_agents[0], segment_size);
      d_x[i] = (DATA_ITEM_TYPE *) malloc_fine_grain_agent(cpu_agents[0], segment_size);
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
    hsa_signal_t copy_sig[gpu_agents_used];
#ifdef AOS
    pixel *dev_src_images[gpu_agents_used];
    pixel *dev_dst_images[gpu_agents_used];

    for (i = 0; i < gpu_agents_used; i++) {
      if (i == gpu_agents_used - 1) {
	items_per_device = items_per_device + trailing_items;
	segment_size = items_per_device * PIXELS_PER_IMG * sizeof(pixel); 
      }
      dev_src_images[i] = (pixel *) malloc_device_mem_agent(agents[i], segment_size);
      dev_dst_images[i] = (pixel *) malloc_device_mem_agent(agents[i], segment_size);
      if (!dev_src_images[i] || !dev_dst_images) {
	printf("Unable to malloc buffer to device memory. Exiting\n");
	exit(0);
      }
#ifdef VERBOSE
      printf("Successfully malloc'ed %lu MB to memory pool at %p and %p\n",
	     segment_size/(1024 * 1024), dev_src_images[i], dev_dst_images[i]);
#endif

      err=hsa_signal_create(1, 0, NULL, &copy_sig[i]);
      check(Creating a HSA signal, err);
      hsa_amd_memory_async_copy(dev_src_images[i], agents[i], src_images[i], agents[i],
				segment_size, 0, NULL, copy_sig[i]);
      value = hsa_signal_wait_acquire(copy_sig[i], HSA_SIGNAL_CONDITION_LT, 1, UINT64_MAX,
				      HSA_WAIT_STATE_BLOCKED);
      err=hsa_signal_destroy(copy_sig[i]);
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
      dev_r[i] = (DATA_ITEM_TYPE *) malloc_device_mem_agent(agents[i], segment_size);
      dev_g[i] = (DATA_ITEM_TYPE *) malloc_device_mem_agent(agents[i], segment_size);
      dev_b[i] = (DATA_ITEM_TYPE *) malloc_device_mem_agent(agents[i], segment_size);
      dev_x[i] = (DATA_ITEM_TYPE *) malloc_device_mem_agent(agents[i], segment_size);
      dev_d_r[i] = (DATA_ITEM_TYPE *) malloc_device_mem_agent(agents[i], segment_size);
      dev_d_g[i] = (DATA_ITEM_TYPE *) malloc_device_mem_agent(agents[i], segment_size);
      dev_d_b[i] = (DATA_ITEM_TYPE *) malloc_device_mem_agent(agents[i], segment_size);
      dev_d_x[i] =  (DATA_ITEM_TYPE *) malloc_device_mem_agent(agents[i], segment_size);

      if (!dev_r[i] || !dev_g[i] || !dev_b[i] || !dev_g[i] || !dev_d_r[i] 
                    || !dev_d_g[i] || !dev_d_b[i] || !dev_d_x[i]) {
	printf("Unable to malloc discrete arrays to device memory. Exiting\n");
	exit(0);
      }

      err=hsa_signal_create(1, 0, NULL, &copy_sig[i]);
      check(Creating a HSA signal, err);
      hsa_amd_memory_async_copy(dev_r[i], agents[i], r[i], agents[i],
				segment_size, 0, NULL, copy_sig[i]);
      value = hsa_signal_wait_acquire(copy_sig[i], HSA_SIGNAL_CONDITION_LT, 1, UINT64_MAX,
				      HSA_WAIT_STATE_BLOCKED);
      err=hsa_signal_destroy(copy_sig[i]);
      
      err=hsa_signal_create(1, 0, NULL, &copy_sig[i]);
      check(Creating a HSA signal, err);
      hsa_amd_memory_async_copy(dev_g[i], agents[i], g[i], agents[i],
				segment_size, 0, NULL, copy_sig[i]);
      value = hsa_signal_wait_acquire(copy_sig[i], HSA_SIGNAL_CONDITION_LT, 1, UINT64_MAX,
				      HSA_WAIT_STATE_BLOCKED);
      err=hsa_signal_destroy(copy_sig[i]);
      
      err=hsa_signal_create(1, 0, NULL, &copy_sig[i]);
      check(Creating a HSA signal, err);
      hsa_amd_memory_async_copy(dev_b[i], agents[i], b[i], agents[i],
				segment_size, 0, NULL, copy_sig[i]);
      value = hsa_signal_wait_acquire(copy_sig[i], HSA_SIGNAL_CONDITION_LT, 1, UINT64_MAX,
				      HSA_WAIT_STATE_BLOCKED);
      err=hsa_signal_destroy(copy_sig[i]);
      
      err=hsa_signal_create(1, 0, NULL, &copy_sig[i]);
      check(Creating a HSA signal, err);
      hsa_amd_memory_async_copy(dev_x[i], agents[i], x[i], agents[i],
				segment_size, 0, NULL, copy_sig[i]);
      value = hsa_signal_wait_acquire(copy_sig[i], HSA_SIGNAL_CONDITION_LT, 1, UINT64_MAX,
				      HSA_WAIT_STATE_BLOCKED);
      err=hsa_signal_destroy(copy_sig[i]);

    }

    // reset this for rest of the program
    items_per_device =  NUM_IMGS / gpu_agents_used;
    segment_size = items_per_device * PIXELS_PER_IMG * sizeof(DATA_ITEM_TYPE);

#endif
#endif

#ifdef AOS
    struct __attribute__ ((aligned(16))) args_t {
      uint64_t global_offset_0;
      uint64_t global_offset_1;
      uint64_t global_offset_2;
      uint64_t printf_buffer;
      uint64_t vqueue_pointer;
      uint64_t aqlwrap_pointer;
      void* in;
      void* out;
      int num_imgs;
    } args[gpu_agents_used];

    
    for (i = 0; i < gpu_agents_used; i++) {
      if (i == gpu_agents_used - 1)
	items_per_device = items_per_device + trailing_items;
      memset(&args[i], 0, sizeof(args[i]));
#ifdef DEVMEM
      args[i].in = dev_src_images[i];
      args[i].out = dev_dst_images[i];
#else
      args[i].in = src_images[i];
      args[i].out = dst_images[i];
#endif
      args[i].num_imgs = items_per_device;
    }
    // reset this for rest of the program
    items_per_device =  NUM_IMGS / gpu_agents_used;
#endif
#ifdef DA
    struct __attribute__ ((aligned(16))) args_t {
      uint64_t global_offset_0;
      uint64_t global_offset_1;
      uint64_t global_offset_2;
      uint64_t printf_buffer;
      uint64_t vqueue_pointer;
      uint64_t aqlwrap_pointer;
      void* r;
      void* g;
      void* b;
      void* x;
      void* d_r;
      void* d_g;
      void* d_b;
      void* d_x;
      int num_imgs;
    } args[gpu_agents_used];

    for (i = 0; i < gpu_agents_used; i++) {
      if (i == gpu_agents_used - 1)
	items_per_device = items_per_device + trailing_items;
      memset(&args[i], 0, sizeof(args[i]));
#ifdef DEVMEM
      args[i].r = dev_r[i];
      args[i].g = dev_g[i];
      args[i].b = dev_b[i];
      args[i].x = dev_x[i];
      args[i].d_r = dev_d_r[i];
      args[i].d_g = dev_d_g[i];
      args[i].d_b = dev_d_b[i];
      args[i].d_x = dev_d_x[i];
#else
      args[i].r = r[i];
      args[i].g = g[i];
      args[i].b = b[i];
      args[i].x = x[i];
      args[i].d_r = d_r[i];
      args[i].d_g = d_g[i];
      args[i].d_b = d_b[i];
      args[i].d_x = d_x[i];
#endif
      args[i].num_imgs = items_per_device;
    }
    // reset this for rest of the program
    items_per_device =  NUM_IMGS / gpu_agents_used;
#endif

    /* ********************************************
     * 
     * Data allocation and distribution code BEGIN 
     *
     *********************************************/


    /*
     * Allocate (and copy) kernel arguments 
     */
    void* kernarg_addresses[gpu_agents_used]; 
    for (i = 0; i < gpu_agents_used; i++) {
      kernarg_addresses[i] = malloc_kernarg_agent(agents[i],kernarg_segment_sizes[i]);
      memcpy(kernarg_addresses[i], &args[i], sizeof(args[i]));
    }

    /*
     * Obtain queue write indices.
     */
    uint64_t indices[gpu_agents_used];
    for (i = 0; i < gpu_agents_used; i++) {
      indices[i] = hsa_queue_load_write_index_relaxed(queues[i]);
    }

    /*
     * Write the aql packet at the calculated queue index address.
     */
    hsa_kernel_dispatch_packet_t* dispatch_packets[gpu_agents_used];
    for (i = 0; i < gpu_agents_used; i++) {
      const uint32_t queueMask = queues[i]->size - 1;
      dispatch_packets[i] = &(((hsa_kernel_dispatch_packet_t*) 
			       (queues[i]->base_address))[indices[i]&queueMask]);

      dispatch_packets[i]->setup  |= 1 << HSA_KERNEL_DISPATCH_PACKET_SETUP_DIMENSIONS;
      dispatch_packets[i]->workgroup_size_x = (uint16_t) WORKGROUP;
      dispatch_packets[i]->workgroup_size_y = (uint16_t)1;
      dispatch_packets[i]->workgroup_size_z = (uint16_t)1;
      dispatch_packets[i]->grid_size_x = (uint32_t) (THREADS);
      dispatch_packets[i]->grid_size_y = 1;
      dispatch_packets[i]->grid_size_z = 1;
      dispatch_packets[i]->completion_signal = signals[i];
      dispatch_packets[i]->kernel_object = kernel_objects[i];
      dispatch_packets[i]->kernarg_address = (void*) kernarg_addresses[i];
      dispatch_packets[i]->private_segment_size = private_segment_sizes[i];
      dispatch_packets[i]->group_segment_size = group_segment_sizes[i];

      uint16_t header = 0;
      header |= HSA_FENCE_SCOPE_SYSTEM << HSA_PACKET_HEADER_ACQUIRE_FENCE_SCOPE;
      header |= HSA_FENCE_SCOPE_SYSTEM << HSA_PACKET_HEADER_RELEASE_FENCE_SCOPE;
      header |= HSA_PACKET_TYPE_KERNEL_DISPATCH << HSA_PACKET_HEADER_TYPE;
      
      __atomic_store_n((uint16_t*)(&dispatch_packets[i]->header), header, __ATOMIC_RELEASE);
    }

    double t = mysecond();
    /*
     * Increment the write index and ring the doorbell to dispatch the kernel.
     */
    for (i = 0; i < gpu_agents_used; i++) {
      hsa_queue_store_write_index_relaxed(queues[i], indices[i]+1);
      hsa_signal_store_relaxed(queues[i]->doorbell_signal, indices[i]);
      check(Dispatching the kernel, err);
    }
    /*
     * Wait on the dispatch completion signal until the kernel is finished.
     */
    for (i = 0; i < gpu_agents_used; i++) {
      value = hsa_signal_wait_acquire(signals[i], HSA_SIGNAL_CONDITION_LT, 1, 
				      UINT64_MAX, HSA_WAIT_STATE_ACTIVE);
    }

    t = 1.0E6 * (mysecond() - t);

#ifdef DEVMEM
#ifdef AOS
    for (i = 0; i < gpu_agents_used; i++) {
      if (i == gpu_agents_used - 1) {
	items_per_device = items_per_device + trailing_items;
	segment_size = items_per_device * PIXELS_PER_IMG * sizeof(pixel); 
      }
      err=hsa_signal_create(1, 0, NULL, &copy_sig[i]);
      check(Creating a HSA signal, err);
      
      hsa_amd_memory_async_copy(dst_images[i], agents[i], dev_dst_images[i], agents[i],
				segment_size, 0, NULL, copy_sig[i]);
      value = hsa_signal_wait_acquire(copy_sig[i], HSA_SIGNAL_CONDITION_LT, 1, UINT64_MAX, 
				      HSA_WAIT_STATE_BLOCKED);
      err=hsa_signal_destroy(copy_sig[i]);
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
      err=hsa_signal_create(1, 0, NULL, &copy_sig[i]);
      check(Creating a HSA signal, err);
      
      hsa_amd_memory_async_copy(d_r[i], agents[i], dev_d_r[i], agents[i], segment_size, 0, NULL, copy_sig[i]);
      value = hsa_signal_wait_acquire(copy_sig[i], HSA_SIGNAL_CONDITION_LT, 1, UINT64_MAX, 
				      HSA_WAIT_STATE_BLOCKED);
      err=hsa_signal_destroy(copy_sig[i]);
      
      err=hsa_signal_create(1, 0, NULL, &copy_sig[i]);
      check(Creating a HSA signal, err);
      
      hsa_amd_memory_async_copy(d_g[i], agents[i], dev_d_g[i], agents[i],
			      segment_size, 0, NULL, copy_sig[i]);
      value = hsa_signal_wait_acquire(copy_sig[i], HSA_SIGNAL_CONDITION_LT, 1, UINT64_MAX, 
				      HSA_WAIT_STATE_BLOCKED);
      err=hsa_signal_destroy(copy_sig[i]);
      err=hsa_signal_create(1, 0, NULL, &copy_sig[i]);
      check(Creating a HSA signal, err);
      
      hsa_amd_memory_async_copy(d_b[i], agents[i], dev_d_b[i], agents[i],
			      segment_size, 0, NULL, copy_sig[i]);
      value = hsa_signal_wait_acquire(copy_sig[i], HSA_SIGNAL_CONDITION_LT, 1, UINT64_MAX, 
                                    HSA_WAIT_STATE_BLOCKED);
      err=hsa_signal_destroy(copy_sig[i]);
      err=hsa_signal_create(1, 0, NULL, &copy_sig[i]);
      check(Creating a HSA signal, err);
      
      hsa_amd_memory_async_copy(d_x[i], agents[i], dev_d_x[i], agents[i],
				segment_size, 0, NULL, copy_sig[i]);
      value = hsa_signal_wait_acquire(copy_sig[i], HSA_SIGNAL_CONDITION_LT, 1, UINT64_MAX, 
				      HSA_WAIT_STATE_BLOCKED);
      err=hsa_signal_destroy(copy_sig[i]);
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
      err = hsa_memory_free(kernarg_addresses[i]);
      check(Freeing kernel argument memory buffer, err);
      
      err=hsa_signal_destroy(signals[i]);
      check(Destroying the signal, err);
      
      err=hsa_queue_destroy(queues[i]);
      check(Destroying the queue, err);
      
#ifdef AOS
#ifdef FINE
      free_fine_grain(src_images[i]);
      free_fine_grain(dst_images[i]);
#endif
#ifdef COARSE
      free_coarse_grain(src_images[i]);
      free_coarse_grain(dst_images[i]);
#endif
#ifdef DEVMEM
      free_device_mem(dev_src_images[i]);
      free_device_mem(dev_dst_images[i]);
#endif
#endif

#ifdef DA
#ifdef FINE
    free_fine_grain(r[i]);
    free_fine_grain(g[i]);
    free_fine_grain(b[i]);
    free_fine_grain(x[i]);
    free_fine_grain(d_r[i]);
    free_fine_grain(d_g[i]);
    free_fine_grain(d_b[i]);
    free_fine_grain(d_x[i]);
#endif
#ifdef COARSE
    free_coarse_grain(r[i]);
    free_coarse_grain(g[i]);
    free_coarse_grain(b[i]);
    free_coarse_grain(x[i]);
    free_coarse_grain(d_r[i]);
    free_coarse_grain(d_g[i]);
    free_coarse_grain(d_b[i]);
    free_coarse_grain(d_x[i]);
#endif
#ifdef DEVMEM
    free_device_mem(dev_r[i]);
    free_device_mem(dev_g[i]);
    free_device_mem(dev_b[i]);
    free_device_mem(dev_x[i]);
    free_device_mem(dev_d_r[i]);
    free_device_mem(dev_d_g[i]);
    free_device_mem(dev_d_b[i]);
    free_device_mem(dev_d_x[i]);
#endif
#endif
      err=hsa_executable_destroy(executables[i]);
      check(Destroying the executable, err);
      
      err=hsa_code_object_destroy(code_objects[i]);
      check(Destroying the code object, err);
    }

    err=hsa_shut_down();
    check(Shutting down the runtime, err);

    return 0;
}
