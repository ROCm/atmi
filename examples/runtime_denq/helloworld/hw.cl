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
    K_ID_mainTask_gpu = 0, 
    K_ID_print_taskId_cpu = 1,
    K_ID_subTask_gpu = 2,
};

typedef struct args_s {
    long int arg1;
} args_t;

kernel void subTask_gpu(long int taskId) {
    ATMI_LPARM_1D(lparm, 1);
    lparm->place = (atmi_place_t)ATMI_PLACE_CPU(0, 0);
    // default case for kernel enqueue: lparm->groupable = ATMI_TRUE;
    args_t args;
    args.arg1 = taskId;
    
    atmi_task_launch(lparm, K_ID_print_taskId_cpu, (void *)&args, sizeof(args_t));
}

__kernel void mainTask_gpu(long int numTasks) {
	int gid = get_global_id(0);
    if(gid % 64 == 0) {
        ATMI_LPARM_1D(lparm, 1);
        lparm->place = (atmi_place_t)ATMI_PLACE_GPU(0, 0);
        // default case for kernel enqueue: lparm->groupable = ATMI_TRUE;
        args_t args;
        args.arg1 = gid;

        atmi_task_launch(lparm, K_ID_subTask_gpu, (void *)&args, sizeof(args_t));
    }
}

__kernel void print_taskId_gpu(long int taskId) {
}

