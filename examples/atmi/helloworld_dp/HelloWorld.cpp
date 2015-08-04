#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <stdio.h>
#include "unistd.h"
using namespace std;
#include "atmi.h"
#include "atmi_kl.h"
#include <sched.h>

__kernel void mainTask_gpu(__global atmi_task_t *thisTask, int numTasks) __attribute__((atmi_kernel("mainTask", "gpu")));


// Declare decode as the PIF for the GPU kernel implementation decode_gpu
__kernel void subTask_gpu(__global atmi_task_t *thisTask, int taskId) __attribute__((atmi_kernel("subTask", "gpu")));

extern "C" void print_taskId_cpu(__global atmi_task_t *thisTask, int taskId) __attribute__((atmi_kernel("print", "cpu")));

extern "C" void print_taskId_cpu(__global atmi_task_t *thisTask, int taskId)
{
    cout << "GPU Task ID" << ": "<< taskId << endl;
}


int main(int argc, char* argv[]) {
    int numTasks = 16;

    ATMI_LPARM_1D(lparm, numTasks);
    lparm->synchronous = ATMI_FALSE;

    lparm->kernel_id = K_ID_mainTask_gpu;
    atmi_task_t *task = mainTask(lparm, numTasks);

    SYNC_TASK(task);

	return 0;
}
