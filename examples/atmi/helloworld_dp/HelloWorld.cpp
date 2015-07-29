#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <stdio.h>
using namespace std;
#include "atmi.h"
#include "atmi_kl.h"
#include "hsa_kl.h"
#include <sched.h>

__kernel void split_gpu(__global atmi_task_t *thisTask, __global const char* in, __global char *out, const size_t strlength) __attribute__((atmi_kernel("split", "gpu")));


// Declare decode as the PIF for the GPU kernel implementation decode_gpu
__kernel void decode_gpu(__global atmi_task_t *thisTask, __global const char* in, __global char *out, const size_t strlength, int kid) __attribute__((atmi_kernel("decode", "gpu")));


// Declare decode as the PIF for the CPU kernel decode_cpu
extern "C" void decode_cpu(atmi_task_t *thisTask, const char* in, char* out, const size_t strlength, int kid) __attribute__((atmi_kernel("decode", "cpu")));

extern "C" void decode_cpu(atmi_task_t *thisTask, const char* in, char* out, const size_t strlength, int kid) {
    int num;
    for (num = 0; num < strlength; num++) {
        out[num] = in[num] + 1;
    }
}

extern "C" void print_cpu(__global atmi_task_t *thisTask, __global char* out, const size_t strlength, int kid) __attribute__((atmi_kernel("print", "cpu")));

extern "C" void print_cpu(__global atmi_task_t *thisTask, __global char* out, const size_t strlength, int kid)
{
    out[strlength] = '\0';
    //cout << "Output from decode_gpu kernel " << kid  << " : "<< out << endl;
}


extern _CPPSTRING_ void print_kl_init(atmi_lparm_t *lparm);
extern _CPPSTRING_ void decode_kl_init(atmi_lparm_t *lparm);

int main(int argc, char* argv[]) {
	const char* input = "Gdkkn\x1FGR@\x1FVnqkc";
	size_t strlength = strlen(input);
    int numTasks = 1024;
	char *output_gpu = (char*) malloc((strlength + 1) * numTasks);

    ATMI_LPARM_1D(lparm, numTasks);
    lparm->synchronous = ATMI_TRUE;

    lparm->kernel_id = 0;
    decode_kl_init(lparm);
    print_kl_init(lparm);
    atmi_task_t *task = split(lparm, input, output_gpu, strlength);

	free(output_gpu);
	return 0;
}
