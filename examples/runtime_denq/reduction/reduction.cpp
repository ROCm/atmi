/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
using namespace std;
#include "atmi_runtime.h"

extern "C" void reduction_cpu(int **in_ptr, long unsigned *length_ptr) {
    int *in = *in_ptr;
    long unsigned length = *length_ptr;
    int num;
    for (num = length; num > 0; num >>= 1) 
        for(int j = 0; j < num; j++) 
            in[j] += in[j + num];
}

int main(int argc, char* argv[]) {
    atmi_status_t err = atmi_init(ATMI_DEVTYPE_ALL);
    if(err != ATMI_STATUS_SUCCESS) return -1;
    const char *module = "reduction.hsaco";
    atmi_platform_type_t module_type = AMDGCN;
    err = atmi_module_register(&module, &module_type, 1);

    atmi_kernel_t reduction_kernel;
    const unsigned int num_args = 2;
    size_t arg_sizes[] = { sizeof(int *), sizeof(long unsigned) };
    atmi_kernel_create(&reduction_kernel, num_args, arg_sizes, 
                       2, 
                       ATMI_DEVTYPE_GPU, "reduction_gpu",
                       ATMI_DEVTYPE_CPU, (atmi_generic_fp)reduction_cpu);

    long unsigned length = 1024;
	int *input;
    atmi_mem_place_t cpu = ATMI_MEM_PLACE(ATMI_DEVTYPE_CPU, 0, 0); 
    atmi_malloc((void **)&input, sizeof(int) * length, cpu);

    for(int ii = 0; ii < length; ii++)
        input[ii] = ii;

    ATMI_LPARM_1D(lparm, length >> 1);
    lparm->synchronous = ATMI_TRUE;
    lparm->place = ATMI_PLACE_GPU(0, 0);
    lparm->groupable = ATMI_TRUE;
    //lparm->kernel_id = K_ID_reduction_gpu;

    long unsigned arg_length = length >> 1;
    void *args[] = { &input, &arg_length };
    atmi_task_launch(lparm, reduction_kernel, args);

    printf("Sum: %d\n", input[0]);

	atmi_free(input);
    atmi_kernel_release(reduction_kernel);
    atmi_finalize();
	return 0;
}
