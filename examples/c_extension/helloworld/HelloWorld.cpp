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
#include <string.h>
#include <stdlib.h>
#include <iostream>
using namespace std;
#include "atmi.h"

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
    const char* input = "Gdkkn\x1FGR@\x1FVnqkc";
    size_t strlength = strlen(input);
    char *output_cpu = (char*) malloc(strlength + 1);
    char *output_gpu = (char*) malloc(strlength + 1);

    ATMI_LPARM_1D(lparm, strlength);
    lparm->synchronous = ATMI_TRUE;

    lparm->kernel_id = K_ID_decode_gpu;
    decode(lparm, input, output_gpu, strlength);
    output_gpu[strlength] = '\0';

    lparm->kernel_id = K_ID_decode_cpu;
    lparm->WORKITEMS = 1;
    decode(lparm, input, output_cpu, strlength);
    output_cpu[strlength] = '\0';

    cout << "Output from the CPU: " << output_cpu << endl;
    cout << "Output from the GPU: " << output_gpu << endl;
    free(output_cpu);
    free(output_gpu);
    return 0;
}
