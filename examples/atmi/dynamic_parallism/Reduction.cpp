#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <stdio.h>
using namespace std;
#include "atmi.h"

// Declare reduction as the PIF for the CPU kernel reduction_cpu
extern "C" void reduction_cpu(atmi_task_t *thisTask, int* in, int length) __attribute__((atmi_task_impl("cpu", "reduction")));

extern "C" void reduction_cpu(atmi_task_t *thisTask, int* in, int length) {
    int num;
    for (num = 1; num < length; num++) {
        in[0] += in[num];
    }
}

// Declare reduction as the PIF for the GPU kernel implementation reduction_gpu
__kernel void reduction_gpu(__global atmi_task_t *thisTask, __global int* in, int length) __attribute__((atmi_task_impl("gpu", "reduction")));

extern "C" void reduction_kl_init(atmi_lparm_t *lparm);
extern "C" void reduction_kl_sync();

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
    reduction_kl_init(lparm_gpu);
    reduction(lparm_gpu, input_gpu, length >> 1);

    //reduction_kl_sync();
    //for(int ii = 0; ii < length; ii++)
    //{
    //printf("%d ", input_gpu[ii]);
    //}
    printf("sum: %d\n", input_gpu[0]);
	free(input_cpu);
	free(input_gpu);
	return 0;
}
