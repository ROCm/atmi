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

enum {
    CPU_IMPL = 10565,
    GPU_IMPL = 42
};    

extern _CPPSTRING_ void decode_cpu_fn(const char *in, char *out, size_t strlength) {
    int num = get_global_id(0);
    if(num < strlength) 
        out[num] = in[num] + 1;
}

extern _CPPSTRING_ void decode_cpu(const char **in, char **out, size_t *strlength) {
    decode_cpu_fn(*in, *out, *strlength);
}

int main(int argc, char **argv) {
    atmi_init(ATMI_DEVTYPE_ALL);
#ifndef USE_BRIG
    const char *module = "hw.hsaco";
    atmi_platform_type_t module_type = AMDGCN;
#else
    const char *module = "hw.brig";
    atmi_platform_type_t module_type = BRIG;
#endif
    atmi_module_register(&module, &module_type, 1);

    atmi_kernel_t kernel;
    const unsigned int num_args = 3;
    size_t arg_sizes[] = {sizeof(const char *), sizeof(char *), sizeof(size_t)};
    atmi_kernel_create_empty(&kernel, num_args, arg_sizes);
    atmi_kernel_add_cpu_impl(kernel, (atmi_generic_fp)decode_cpu, CPU_IMPL);
    atmi_kernel_add_gpu_impl(kernel, "decode_gpu", GPU_IMPL);

    const char* input = "Gdkkn\x1FGR@\x1FVnqkc";
    size_t strlength = strlen(input);
    char *output_cpu = (char*) malloc(strlength + 1);
    char *output_gpu = (char*) malloc(strlength + 1);

    int gpu_id = 0;
    int cpu_id = 0;
    atmi_machine_t *machine = atmi_machine_get_info();
    int gpu_count = machine->device_count_by_type[ATMI_DEVTYPE_GPU];
    if(argv[1] != NULL) gpu_id = (atoi(argv[1]) % gpu_count);
    printf("Choosing GPU %d/%d\n", gpu_id, gpu_count);

    /* Run HelloWorld on GPU */
    atmi_mem_place_t gpu = ATMI_MEM_PLACE(ATMI_DEVTYPE_GPU, gpu_id, 0);
    void *d_input, *d_output;
    atmi_malloc(&d_input, strlength+1, gpu);
    atmi_malloc(&d_output, strlength+1, gpu);

    ATMI_CPARM(cparm_gpu);
    atmi_task_handle_t h2d_gpu = atmi_memcpy_async(cparm_gpu, d_input, input, strlength+1);

    ATMI_LPARM_GPU_1D(lparm_gpu, gpu_id, strlength);
    ATMI_PARM_SET_DEPENDENCIES(lparm_gpu, h2d_gpu);
    void *gpu_args[] = {&d_input, &d_output, &strlength};
    atmi_task_handle_t k_gpu = atmi_task_launch(lparm_gpu, kernel, gpu_args);

    ATMI_PARM_SET_DEPENDENCIES(cparm_gpu, k_gpu);
    atmi_task_handle_t d2h_gpu = atmi_memcpy_async(cparm_gpu, output_gpu, d_output, strlength+1);

    // wait only for the last task in the chain
    atmi_task_wait(d2h_gpu);
    output_gpu[strlength] = '\0';
    cout << "Output from the GPU: " << output_gpu << endl;

    /* Run HelloWorld on CPU */
    atmi_mem_place_t cpu = ATMI_MEM_PLACE(ATMI_DEVTYPE_CPU, cpu_id, 0);
	void *h_input, *h_output;
    atmi_malloc(&h_input, strlength+1, cpu);
    atmi_malloc(&h_output, strlength+1, cpu);

	ATMI_CPARM(cparm_cpu);
    atmi_task_handle_t h2d_cpu = atmi_memcpy_async(cparm_cpu, h_input, input, strlength+1);

    ATMI_LPARM_CPU_1D(lparm_cpu, cpu_id, strlength);
    ATMI_PARM_SET_DEPENDENCIES(lparm_cpu, h2d_cpu);
    void *cpu_args[] = {&h_input, &h_output, &strlength};
    atmi_task_handle_t k_cpu = atmi_task_launch(lparm_cpu, kernel, cpu_args);

    ATMI_PARM_SET_DEPENDENCIES(cparm_cpu, k_cpu);
    atmi_task_handle_t d2h_cpu = atmi_memcpy_async(cparm_cpu, output_cpu, h_output, strlength+1);
    
    // wait only for the last task in the chain
    atmi_task_wait(d2h_cpu);
    output_cpu[strlength] = '\0';
    cout << "Output from the CPU: " << output_cpu << endl;

    /* cleanup */
	free(output_gpu);
	free(output_cpu);
    atmi_free(h_input);
    atmi_free(h_output);
    atmi_free(d_input);
    atmi_free(d_output);
    atmi_kernel_release(kernel);
    atmi_finalize();
    return 0;
}
