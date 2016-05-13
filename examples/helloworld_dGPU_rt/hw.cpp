#include "atmi.h"
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

extern _CPPSTRING_ void decode_cpu_fn(atmi_task_handle_t thisTask, const char *in, char *out, size_t strlength) {
    int num;
    for (num = 0; num < strlength; num++) {
        out[num] = in[num] + 1;
    }
}

extern _CPPSTRING_ void decode_cpu(atmi_task_handle_t *thisTask, const char **in, char **out, size_t *strlength) {
    decode_cpu_fn(*thisTask, *in, *out, *strlength);
}


int main(int argc, char **argv) {
    atmi_status_t err = atmi_init(ATMI_DEVTYPE_ALL);
    #if 0
    const char *module = "hw.hsaco";
    atmi_platform_type_t module_type = AMDGCN;
    #else
    const char *module = "hw.brig";
    atmi_platform_type_t module_type = BRIG;
    #endif
    err = atmi_module_register(&module, &module_type, 1);

    atmi_machine_t *machine = atmi_machine_get_info();

    printf("Machine info...\n");
    printf("CPU devices: %d, GPU devices: %d, DSP devices: %d\n", 
            machine->device_count_by_type[ATMI_DEVTYPE_CPU],
            machine->device_count_by_type[ATMI_DEVTYPE_GPU],
            machine->device_count_by_type[ATMI_DEVTYPE_DSP]);

    int total_devices = machine->device_count_by_type[ATMI_DEVTYPE_CPU] +
            machine->device_count_by_type[ATMI_DEVTYPE_GPU] +
            machine->device_count_by_type[ATMI_DEVTYPE_DSP];

    for(int i = 0; i < total_devices; i++) {
        printf("Device[%d]: %d, %u\n", i, machine->devices[i].type, machine->devices[i].memory_pool_count);
    }

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

    atmi_mem_place_t gpu0 = ATMI_MEM_PLACE(0, ATMI_DEVTYPE_GPU, 0, 0);
    atmi_mem_place_t cpu0 = ATMI_MEM_PLACE(0, ATMI_DEVTYPE_CPU, 0, 0);

	void *d_input;
    atmi_malloc(&d_input, strlength+1, gpu0);
    atmi_copy_h2d(d_input, input, strlength+1, gpu0);

	void *h_input;
    atmi_malloc(&h_input, strlength+1, cpu0);
    atmi_copy_h2d(h_input, input, strlength+1, cpu0);

	void *d_output;
    atmi_malloc(&d_output, strlength+1, gpu0);

	void *h_output;
    atmi_malloc(&h_output, strlength+1, cpu0);

    void *gpu_args[] = {&d_input, &d_output, &strlength};
    void *cpu_args[] = {&h_input, &h_output, &strlength};
   
    ATMI_LPARM_1D(lparm, strlength);
    lparm->synchronous = ATMI_TRUE;

    lparm->kernel_id = GPU_IMPL;
    lparm->place = ATMI_PLACE_GPU(0, 0);
    atmi_task_launch(kernel, lparm, gpu_args);
    atmi_copy_d2h(output_gpu, d_output, strlength+1, gpu0);
    output_gpu[strlength] = '\0';

    lparm->kernel_id = CPU_IMPL;
    lparm->place = ATMI_PLACE_CPU(0, 0);
    atmi_task_launch(kernel, lparm, cpu_args);
    atmi_copy_d2h(output_cpu, h_output, strlength+1, cpu0);
    output_cpu[strlength] = '\0';
   
    cout << "Output from the GPU: " << output_gpu << endl;
    cout << "Output from the CPU: " << output_cpu << endl;
	free(output_gpu);
	free(output_cpu);
    atmi_kernel_release(kernel);
    return 0;
}
