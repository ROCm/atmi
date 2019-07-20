/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/

#include "atmi_runtime.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
using namespace std;
#ifdef __cplusplus
#define _CPPSTRING_ "C"
#endif
#ifndef __cplusplus
#define _CPPSTRING_
#endif

#define ErrorCheck(status) \
  if (status != ATMI_STATUS_SUCCESS) { \
    printf("Error at [%s:%d]\n", __FILE__, __LINE__); \
    exit(1); \
  }

extern _CPPSTRING_ void decode_cpu_fn(const char *in, char *out, int strlength) {
  int num = get_global_id(0);
  if(num < strlength)
    out[num] = in[num] + 1;
}

extern _CPPSTRING_ void decode_cpu(const char **in, int *strlength, char **out) {
  decode_cpu_fn(*in, *out, *strlength);
}


int main(int argc, char **argv) {
  ErrorCheck(atmi_init(ATMI_DEVTYPE_ALL));

  const char *module = "hw.hsaco";
  atmi_platform_type_t module_type = AMDGCN;
  ErrorCheck(atmi_module_register(&module, &module_type, 1));

  atmi_machine_t *machine = atmi_machine_get_info();

  atmi_kernel_t kernel;
  const unsigned int num_args = 3;
  size_t arg_sizes[] = {sizeof(const char *), sizeof(int), sizeof(char *)};
  ErrorCheck(atmi_kernel_create(&kernel, num_args, arg_sizes,
        2,
        ATMI_DEVTYPE_CPU, (atmi_generic_fp)decode_cpu,
        ATMI_DEVTYPE_GPU, "decode_gpu"));

  const char* input = "Gdkkn\x1FGR@\x1FVnqkc";
  int strlength = strlen(input);
  char *output_cpu = (char*) malloc(strlength + 1);
  char *output_gpu = (char*) malloc(strlength + 1);

  int gpu_id = 0;
  int cpu_id = 0;
  int gpu_count = machine->device_count_by_type[ATMI_DEVTYPE_GPU];
  if(argv[1] != NULL) gpu_id = (atoi(argv[1]) % gpu_count);
  printf("Choosing GPU %d/%d\n", gpu_id, gpu_count);

  /* Run HelloWorld on GPU */
  atmi_mem_place_t gpu = ATMI_MEM_PLACE(ATMI_DEVTYPE_GPU, gpu_id, 0);
  atmi_mem_place_t cpu = ATMI_MEM_PLACE(ATMI_DEVTYPE_CPU, cpu_id, 0);

  void *d_input;
  ErrorCheck(atmi_malloc(&d_input, strlength+1, gpu));
  ErrorCheck(atmi_memcpy(d_input, input, strlength+1));

  void *h_input;
  ErrorCheck(atmi_malloc(&h_input, strlength+1, cpu));
  ErrorCheck(atmi_memcpy(h_input, input, strlength+1));

  void *d_output;
  ErrorCheck(atmi_malloc(&d_output, strlength+1, gpu));

  void *h_output;
  ErrorCheck(atmi_malloc(&h_output, strlength+1, cpu));

  void *gpu_args[] = {&d_input, &strlength, &d_output};
  void *cpu_args[] = {&h_input, &strlength, &h_output};

  ATMI_LPARM_1D(lparm, strlength);
  lparm->synchronous = ATMI_TRUE;

  lparm->place = ATMI_PLACE_GPU(0, gpu_id);
  volatile atmi_task_handle_t task = atmi_task_launch(lparm, kernel, gpu_args);
  if(task == ATMI_NULL_TASK_HANDLE) {
    fprintf(stderr, "GPU Task Launch/Execution Error.\n");
    exit(1);
  }

  lparm->place = ATMI_PLACE_CPU(0, cpu_id);
  task = atmi_task_launch(lparm, kernel, cpu_args);
  if(task == ATMI_NULL_TASK_HANDLE) {
    fprintf(stderr, "GPU Task Launch/Execution Error.\n");
    exit(1);
  }

  ErrorCheck(atmi_memcpy(output_gpu, d_output, strlength+1));
  output_gpu[strlength] = '\0';
  ErrorCheck(atmi_memcpy(output_cpu, h_output, strlength+1));
  output_cpu[strlength] = '\0';

  cout << "Output from the GPU: " << output_gpu << endl;
  cout << "Output from the CPU: " << output_cpu << endl;

  /* cleanup */
  free(output_gpu);
  free(output_cpu);
  ErrorCheck(atmi_free(h_input));
  ErrorCheck(atmi_free(h_output));
  ErrorCheck(atmi_free(d_input));
  ErrorCheck(atmi_free(d_output));
  ErrorCheck(atmi_kernel_release(kernel));
  ErrorCheck(atmi_finalize());
  return 0;
}
