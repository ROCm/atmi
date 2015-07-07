#include <string.h>
#include <stdlib.h>
#include <iostream>
using namespace std;
#include "atmi.h"

// Declare decode as the PIF for the CPU kernel decode_cpu
extern "C" void decode_cpu(atmi_task_t *thisTask, const char* in, char* out, const size_t strlength) __attribute__((atmi_kernel("decode", "cpu")));

extern "C" void decode_cpu(atmi_task_t *thisTask, const char* in, char* out, const size_t strlength) {
    int num;
    for (num = 0; num < strlength; num++) {
        out[num] = in[num] + 1;
    }
}

// Declare decode as the PIF for the GPU kernel implementation decode_gpu
__kernel void decode_gpu(__global atmi_task_t *thisTask, __global const char* in, __global char *out, const size_t strlength) __attribute__((atmi_kernel("decode", "gpu")));

int main(int argc, char* argv[]) {
	const char* input = "Gdkkn\x1FGR@\x1FVnqkc";
	size_t strlength = strlen(input);
    char *output_cpu = (char*) malloc(strlength + 1);
	char *output_gpu = (char*) malloc(strlength + 1);

    ATMI_LPARM_1D(lparm_gpu, strlength);
    lparm_gpu->synchronous = ATMI_TRUE;
    decode(lparm_gpu, input, output_gpu, strlength);
    output_gpu[strlength] = '\0';
    
    ATMI_LPARM_CPU(lparm_cpu);
    lparm_cpu->synchronous = ATMI_TRUE;
    decode(lparm_cpu, input, output_cpu, strlength);
    output_cpu[strlength] = '\0';

    cout << "Output from the CPU: " << output_cpu << endl;
    cout << "Output from the GPU: " << output_gpu << endl;
	free(output_cpu);
	free(output_gpu);
	return 0;
}
