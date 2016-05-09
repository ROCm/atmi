#include "atmi.h"
#include "atmi_runtime.h"
#include <string.h>
#include <stdlib.h>
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

#include "hw_structs.h"
extern _CPPSTRING_ void decode_cpu(atmi_task_handle_t *thisTask, void **args) {
    decode_args_t *cpu_args = *(decode_args_t **)args;
    size_t strlength = cpu_args->strlength; 
    const char *in = cpu_args->in;
    char *out = cpu_args->out;
    int num;
    for (num = 0; num < strlength; num++) {
        out[num] = in[num] + 1;
    }
}

int main(int argc, char **argv) {
    atmi_status_t err = atmi_init(ATMI_DEVTYPE_ANY);
    #if 0
    const char *module = "hw.hsaco";
    atmi_platform_type_t module_type = AMDGCN;
    #else
    const char *module = "hw.brig";
    atmi_platform_type_t module_type = BRIG;
    #endif
    err = atmi_module_register(&module, &module_type, 1);

    atmi_kernel_t kernel;
    const unsigned int num_args = 1;
    size_t arg_sizes[num_args];
    arg_sizes[0] = sizeof(void *);
    atmi_kernel_create_empty(&kernel, num_args, arg_sizes);
    atmi_kernel_add_cpu_impl(kernel, (atmi_generic_fp)decode_cpu, CPU_IMPL);
    atmi_kernel_add_gpu_impl(kernel, "decode_gpu", GPU_IMPL);

	const char* input = "Gdkkn\x1FGR@\x1FVnqkc";
	size_t strlength = strlen(input);
	char *output_gpu = (char*) malloc(strlength + 1);
    char *output_cpu = (char*) malloc(strlength + 1);

    decode_args_t decode_gpu_args = {.in=input, .out=output_gpu, .strlength=strlength};
    decode_args_t decode_cpu_args = {.in=input, .out=output_cpu, .strlength=strlength};

    void *gpu_args[num_args];
    void *cpu_args[num_args];
    
    void *tmp_gpu = &decode_gpu_args;
    gpu_args[0] = &tmp_gpu;
    void *tmp_cpu = &decode_cpu_args;
    cpu_args[0] = &tmp_cpu;

    ATMI_LPARM_1D(lparm, strlength);
    lparm->synchronous = ATMI_TRUE;

    lparm->kernel_id = GPU_IMPL;
    atmi_task_launch(kernel, lparm, gpu_args);
    output_gpu[strlength] = '\0';

    lparm->kernel_id = CPU_IMPL;
    atmi_task_launch(kernel, lparm, cpu_args);
    output_cpu[strlength] = '\0';
   
    cout << "Output from the GPU: " << output_gpu << endl;
    cout << "Output from the CPU: " << output_cpu << endl;
	free(output_cpu);
	free(output_gpu);

    atmi_kernel_release(kernel);
    return 0;
}
