/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <iostream>

#include "atmi_runtime.h"

//share data size between host and device
#include "hw.h"

using namespace std;

enum {
  CPU_IMPL = 10565,
  GPU_IMPL = 42
};

extern _CPPSTRING_ void decode_cpu(const char **in, char **out, size_t *strlength, char **extra);

int main(int argc, char **argv) {
  atmi_init(ATMI_DEVTYPE_ALL);

  //input
  const char* input = "Gdkkn\x1FGR@\x1FVnqkc";
  size_t strlength = strlen(input);

  //extra buffer
  const char* empty = "";
  char *buffer = (char*) calloc(BUFFER_SIZE, sizeof(char));

  // create kernel args
  size_t arg_sizes[] = {sizeof(const char *), sizeof(char *), sizeof(size_t), sizeof(char *)};
  const unsigned int num_args = sizeof(arg_sizes) / sizeof(size_t);

  // create kernel
  atmi_kernel_t kernel;
  atmi_kernel_create_empty(&kernel, num_args, arg_sizes);

  {
    // gpu kernel implementation
    {
#ifndef USE_BRIG
      const char *module = "hw_gpu.hsaco";
      atmi_platform_type_t module_type = AMDGCN;
#else
      const char *module = "hw_gpu.brig";
      atmi_platform_type_t module_type = BRIG;
#endif
      atmi_module_register(&module, &module_type, 1);

      atmi_kernel_add_gpu_impl(kernel, "decode_gpu", GPU_IMPL);
    }

    //output on gpu
    char *output_gpu = (char*) calloc(strlength + 1, sizeof(char));

    // choose gpu id and memory place
    int gpu_id = 0;
    {
      atmi_machine_t *machine = atmi_machine_get_info();
      int gpu_count = machine->device_count_by_type[ATMI_DEVTYPE_GPU];

      if(argv[1] != NULL)
        gpu_id = (atoi(argv[1]) % gpu_count);

      printf("Choosing GPU %d/%d\n", gpu_id, gpu_count);
    }
    atmi_mem_place_t gpu = ATMI_MEM_PLACE(ATMI_DEVTYPE_GPU, gpu_id, 0);

    // Alloc
    void *d_input, *d_output;
    void *extra;
    {
      atmi_malloc(&d_input, strlength+1, gpu);
      atmi_malloc(&d_output, strlength+1, gpu);
      atmi_malloc(&extra, BUFFER_SIZE, gpu);
    }

    /* Run HelloWorld on GPU */
    {
      ATMI_CPARM(cparm_gpu);
      // copy to
      atmi_task_handle_t h2d_gpu = atmi_memcpy_async(cparm_gpu, d_input, input, strlength+1);

      ATMI_CPARM(cparm_gpu_p);
      atmi_task_handle_t h2d_gpu_p = atmi_memcpy_async(cparm_gpu_p, extra, empty, 1);

      // launching args
      ATMI_LPARM_GPU_1D(lparm_gpu, gpu_id, strlength);
      void *gpu_args[] = {&d_input, &d_output, &strlength, &extra};

      // Set two dependencies on the same line
      ATMI_PARM_SET_DEPENDENCIES(lparm_gpu, h2d_gpu, h2d_gpu_p);

      // launch
      atmi_task_handle_t k_gpu = atmi_task_launch(lparm_gpu, kernel, gpu_args);

      // Read back the extra buffer
      {
        ATMI_PARM_SET_DEPENDENCIES(cparm_gpu_p, k_gpu);

        atmi_task_handle_t d2h_gpu_p = atmi_memcpy_async(cparm_gpu_p, buffer, extra, BUFFER_SIZE);

        atmi_task_wait(d2h_gpu_p);

        if (buffer[0] != '\0') {
          printf("Out put the buffer\n");
        }
      }

      // Read back the output
      {
        ATMI_PARM_SET_DEPENDENCIES(cparm_gpu, k_gpu);
        // copy from
        atmi_task_handle_t d2h_gpu = atmi_memcpy_async(cparm_gpu, output_gpu, d_output, strlength+1);

        atmi_task_wait(d2h_gpu);

        if (1) {
          output_gpu[strlength] = '\0';
          cout << "Output from the GPU: " << output_gpu << endl;
        }
      }
    }

    // Free
    {
      atmi_free(extra);
      atmi_free(d_output);
      atmi_free(d_input);
    }

    /* cleanup */
    free(output_gpu);
  }

  {
    // cpu kernel implementation
    {
      atmi_kernel_add_cpu_impl(kernel, (atmi_generic_fp)decode_cpu, CPU_IMPL);
    }

    //output on cpu
    char *output_cpu = (char*) calloc(strlength + 1, sizeof(char));

    // choose cpu id and memory place
    int cpu_id = 0;
    atmi_mem_place_t cpu = ATMI_MEM_PLACE(ATMI_DEVTYPE_CPU, cpu_id, 0);

    // Alloc
    void *h_input, *h_output;
    void *extra;
    {
      atmi_malloc(&h_input, strlength+1, cpu);
      atmi_malloc(&h_output, strlength+1, cpu);
      atmi_malloc(&extra, BUFFER_SIZE, cpu);
    }

    /* Run HelloWorld on CPU */
    {
      ATMI_CPARM(cparm_cpu);
      // copy to
      atmi_task_handle_t h2d_cpu = atmi_memcpy_async(cparm_cpu, h_input, input, strlength+1);

      // launching args
      ATMI_LPARM_CPU_1D(lparm_cpu, cpu_id, strlength);
      void *cpu_args[] = {&h_input, &h_output, &strlength, &extra};

      ATMI_PARM_SET_DEPENDENCIES(lparm_cpu, h2d_cpu);

      // launch
      atmi_task_handle_t k_cpu = atmi_task_launch(lparm_cpu, kernel, cpu_args);

      // Read back the extra buffer
      {

      }

      // Read back the output
      {
        ATMI_PARM_SET_DEPENDENCIES(cparm_cpu, k_cpu);
        // copy from
        atmi_task_handle_t d2h_cpu = atmi_memcpy_async(cparm_cpu, output_cpu, h_output, strlength+1);

        atmi_task_wait(d2h_cpu);

        if (1) {
          output_cpu[strlength] = '\0';
          cout << "Output from the CPU: " << output_cpu << endl;
        }
      }
    }

    // Free
    {
      atmi_free(extra);
      atmi_free(h_output);
      atmi_free(h_input);
    }

    /* cleanup */
    free(output_cpu);
  }

  atmi_kernel_release(kernel);
  atmi_finalize();
  return 0;
}
