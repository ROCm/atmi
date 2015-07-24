#include <string.h>
#include <stdlib.h>
#include <iostream>
using namespace std;
#include "atmi.h"
#include <stdio.h>

// Declare decode as the PIF for the CPU kernel decode_cpu
extern "C" void decode_cpu(atmi_task_t *thisTask, char* in, char ** out, const size_t strlength) __attribute__((atmi_kernel("decode", "cpu")));

// Declare decode as the PIF for the GPU kernel implementation decode_gpu
__kernel void decode_gpu(__global atmi_task_t *thisTask, __global char* in, __global char **out, const size_t strlength) __attribute__((atmi_kernel("decode", "gpu")));

extern "C" void decode_cpu(atmi_task_t *thisTask, char* in, char** out, const size_t strlength) {
    int num;
    *out = in;
}

int main(int argc, char* argv[]) {
    //size_t strlength = strlen(input);
    //char *output_cpu = (char*) malloc(strlength + 1);
    //char *output_gpu = (char*) malloc(strlength + 1);

    ATMI_LPARM_1D(lparm, 1);
    lparm->synchronous = ATMI_TRUE;

    char t;
	char* a = &t;
	char* b = NULL;
    lparm->kernel_id = K_ID_decode_gpu;
    decode(lparm, a, &b, 1);
    //output_gpu[strlength] = '\0';
    printf("a: %p b: %p\n", a, b);
    
    lparm->kernel_id = K_ID_decode_cpu;
    decode(lparm, a, &b, 1);
    //output_cpu[strlength] = '\0';

    printf("a: %p b: %p\n", a, b);
    //cout << "Output from the CPU: " << output_cpu << endl;
    //cout << "Output from the GPU: " << output_gpu << endl;
    //free(output_cpu);
    //free(output_gpu);
	return 0;
}
