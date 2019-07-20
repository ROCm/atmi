/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/

#include "atmi_runtime.h"
#include "atmi_interop_hsa.h"
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
    GPU_IMPL = 42
};    

int main(int argc, char **argv) {
    atmi_status_t err = atmi_init(ATMI_DEVTYPE_ALL);
    if(err != ATMI_STATUS_SUCCESS) return -1;
    const char *module = "globalsymbol.hsaco";
    atmi_platform_type_t module_type = AMDGCN;
    atmi_module_register(&module, &module_type, 1);

    atmi_kernel_t kernel;
    const unsigned int num_args = 2;
    size_t arg_sizes[] = {sizeof(float *), sizeof(size_t)};
    atmi_kernel_create_empty(&kernel, num_args, arg_sizes);
    atmi_kernel_add_gpu_impl(kernel, "multiply_gpu", GPU_IMPL);

    size_t a_len = 16;
    float *a = (float *) malloc(sizeof(float) * a_len);
    // init a
    cout << "Original array values" << endl;
    for(int i = 0; i < a_len; i++) {
        a[i] = i + 1;
        cout << a[i] << " ";
    }
    cout << endl;

    int gpu_id = 0;
    atmi_machine_t *machine = atmi_machine_get_info();
    int gpu_count = machine->device_count_by_type[ATMI_DEVTYPE_GPU];
    if(argv[1] != NULL) gpu_id = (atoi(argv[1]) % gpu_count);

    atmi_mem_place_t gpu = ATMI_MEM_PLACE(ATMI_DEVTYPE_GPU, gpu_id, 0);

	void *d_a;
    atmi_malloc(&d_a, sizeof(float) * a_len, gpu);
    atmi_memcpy(d_a, a, sizeof(float) * a_len);

    /* setup launch params */
    void *gpu_args[] = {&d_a, &a_len};
    ATMI_LPARM_1D(lparm, a_len);
    lparm->synchronous = ATMI_TRUE;
    lparm->kernel_id = GPU_IMPL;
    lparm->place = ATMI_PLACE_GPU(0, gpu_id);

    /* launch and wait for kernel */
    atmi_task_launch(lparm, kernel, gpu_args);
    atmi_memcpy(a, d_a, sizeof(float) * a_len);
    cout << "With default multiplier (4)" << endl;
    for(int i = 0; i < a_len; i++) {
        cout << a[i] << " ";
    }
    cout << endl;

    /* change the multiplier */ 
    int new_multiplier = 10;
    void *mul_addr;
    unsigned int mul_size;
    atmi_interop_hsa_get_symbol_info(gpu, "multiplier", &mul_addr, &mul_size);
    atmi_memcpy(mul_addr, &new_multiplier, mul_size);

    /* launch with new multiplier and wait for kernel */
    atmi_task_launch(lparm, kernel, gpu_args);
    atmi_memcpy(a, d_a, sizeof(float) * a_len);
    cout << "With modified multiplier (" << new_multiplier << ")" << endl;
    for(int i = 0; i < a_len; i++) {
        cout << a[i] << " ";
    }
    cout << endl;

    /* cleanup */
	free(a);
    atmi_free(d_a);
    atmi_kernel_release(kernel);
    atmi_finalize();
    return 0;
}
