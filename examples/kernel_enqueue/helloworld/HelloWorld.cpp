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
#include <stdio.h>
using namespace std;
#include "atmi.h"
#include "atmi_kl.h"

__kernel void mainTask_gpu(__global atmi_task_handle_t thisTask, int numTasks) __attribute__((atmi_kernel("mainTask", "gpu")));

__kernel void subTask_gpu(__global atmi_task_handle_t thisTask, int taskId) __attribute__((atmi_kernel("subTask", "gpu")));

extern "C" void print_taskId_cpu(__global atmi_task_handle_t thisTask, int taskId) __attribute__((atmi_kernel("print", "cpu")));

extern "C" void print_taskId_cpu(__global atmi_task_handle_t thisTask, int taskId)
{
    //cout << "Leaf Sub-task ID" << endl;
    cout << "Leaf Sub-task ID" << ": " << taskId << endl;
}

extern atmi_klist_t *atmi_klist;
int main(int argc, char* argv[]) {
    int numTasks = 16;

    ATMI_LPARM_1D(lparm, numTasks);
    lparm->synchronous = ATMI_TRUE;
    lparm->groupable = ATMI_TRUE;

    lparm->kernel_id = K_ID_mainTask_gpu;
    //for(int i = 0; i < numTasks; i++) 
    mainTask(lparm, numTasks);

    //SYNC_STREAM(0);
    cout << "Number: " << *(int *)atmi_klist << endl;
    cout << "Number: " << (void *)atmi_klist->tasks << endl;
	return 0;
}
