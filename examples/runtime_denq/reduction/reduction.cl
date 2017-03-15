/*
MIT License

Copyright © 2016 Advanced Micro Devices, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software
without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "atmi_kl.h"

enum { 
    reduction_task = 0, 
};

typedef struct args_r { 
    int *in;
    unsigned long length;
} args_t;

kernel void reduction_gpu(__global int* in, unsigned long length) {
	int num = get_global_id(0);

    in[num] += in[num + length];

    barrier(CLK_GLOBAL_MEM_FENCE);

    if(num == 0)
    {
        length = length >> 1;
        ATMI_LPARM_1D(lparm, length);
        if(length > 8) 
            lparm->place = (atmi_place_t)ATMI_PLACE_GPU(0, 0);
        else 
            lparm->place = (atmi_place_t)ATMI_PLACE_CPU(0, 0);
        args_t args;
        args.in = in;
        args.length = length;
        atmi_task_launch(lparm, reduction_task, (void *)&args, sizeof(args_t));
    }
}
