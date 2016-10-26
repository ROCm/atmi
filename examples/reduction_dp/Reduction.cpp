/*
 * MIT License
 *
 * Copyright © 2016 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software
 * without restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies of the
 * Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 * */

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
