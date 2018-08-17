/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/

#include <string.h>
#include <stdlib.h>
#include <iostream>
using namespace std;
#include "atmi.h"
#include "atmi_runtime.h"

// Declare decode as the PIF for the CPU kernel decode_cpu
extern "C" void decode_cpu(const char* in, char* out, const size_t strlength) __attribute__((atmi_kernel("decode", "cpu")));

// Declare decode as the PIF for the GPU kernel decode_gpu
__kernel void decode_gpu(__global const char* in, __global char *out, const size_t strlength) __attribute__((atmi_kernel("decode", "gpu")));

extern "C" void decode_cpu(const char* in, char* out, const size_t strlength) {
    int num;
    for (num = 0; num < strlength; num++) {
        out[num] = in[num] + 1;
    }
}

int main(int argc, char* argv[]) {
    atmi_status_t err = atmi_init(ATMI_DEVTYPE_ALL);
    if(err != ATMI_STATUS_SUCCESS) return -1;

    const char* input = "Gdkkn\x1FGR@\x1FVnqkc";
    size_t strlength = strlen(input);
    atmi_mem_place_t place = ATMI_MEM_PLACE_CPU_MEM(0, 0, 0);
    char *input_gpu;
    atmi_malloc((void **)&input_gpu, strlength + 1, place);
    memcpy(input_gpu, input, strlength);
    input_gpu[strlength] = 0;

    char *output_cpu = (char*) malloc(strlength + 1);
    char *output_gpu;
    atmi_malloc((void **)&output_gpu, strlength + 1, place);

    ATMI_LPARM_1D(lparm, strlength);
    lparm->synchronous = ATMI_TRUE;

    lparm->kernel_id = K_ID_decode_gpu;
    lparm->place = ATMI_PLACE_GPU(0, 0);
    decode(lparm, input_gpu, output_gpu, strlength);
    output_gpu[strlength] = '\0';

    lparm->kernel_id = K_ID_decode_cpu;
    lparm->place = ATMI_PLACE_CPU(0, 0);
    lparm->WORKITEMS = 1;
    decode(lparm, input, output_cpu, strlength);
    output_cpu[strlength] = '\0';

    cout << "Output from the CPU: " << output_cpu << endl;
    cout << "Output from the GPU: " << output_gpu << endl;
    free(output_cpu);
    atmi_free(output_gpu);
    atmi_free(input_gpu);
    return 0;
}
