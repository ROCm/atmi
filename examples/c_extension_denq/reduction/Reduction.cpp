/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
using namespace std;
#include "atmi.h"

// Declare reduction as the PIF for the CPU kernel reduction_cpu
extern "C" void reduction_cpu(atmi_task_handle_t thisTask, int* in, int length) __attribute__((atmi_kernel("reduction", "cpu")));
extern "C" void reduction_cpu(atmi_task_handle_t thisTask, int* in, int length) {
    int num;
    for (num = length; num > 0; num >>= 1) {
        int j;
        for(j = 0; j < num; j++)
        {
            in[j] += in[j + num];
        }
    }
}

// Declare reduction as the PIF for the GPU kernel implementation reduction_gpu
__kernel void reduction_gpu(atmi_task_handle_t thisTask, __global int* in, int length) __attribute__((atmi_kernel("reduction", "gpu")));

int main(int argc, char* argv[]) {
    int length = 1024;
	int *input_gpu = (int*) malloc(sizeof(int)*(length));
	int *input_cpu = (int*) malloc(sizeof(int)*(length));

    for(int ii = 0; ii < length; ii++)
    {
        input_cpu[ii] = input_gpu[ii] = 1;
    }

    ATMI_LPARM_1D(lparm_gpu, length >> 1);
    lparm_gpu->synchronous = ATMI_TRUE;
    lparm_gpu->kernel_id = K_ID_reduction_gpu;

    reduction(lparm_gpu, input_gpu, length >> 1);

    ATMI_LPARM_1D(lparm_cpu, length >> 1);
    lparm_cpu->synchronous = ATMI_TRUE;
    lparm_cpu->kernel_id = K_ID_reduction_cpu;
    reduction(lparm_cpu, input_cpu, length >> 1);

    printf("GPU Sum: %d\n", input_gpu[0]);
    printf("CPU Sum: %d\n", input_cpu[0]);
	free(input_gpu);
    free(input_cpu);
	return 0;
}
