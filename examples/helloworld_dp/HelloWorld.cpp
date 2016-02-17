#include <iostream>
#include <stdio.h>
using namespace std;
#include "atmi.h"

__kernel void mainTask_gpu(__global atmi_task_handle_t thisTask, int numTasks) __attribute__((atmi_kernel("mainTask", "gpu")));

__kernel void subTask_gpu(__global atmi_task_handle_t thisTask, int taskId) __attribute__((atmi_kernel("subTask", "gpu")));

extern "C" void print_taskId_cpu(__global atmi_task_handle_t thisTask, int taskId) __attribute__((atmi_kernel("print", "cpu")));

extern "C" void print_taskId_cpu(__global atmi_task_handle_t thisTask, int taskId)
{
    cout << "Leaf Sub-task ID" << ": " << taskId << endl;
}


int main(int argc, char* argv[]) {
    int numTasks = 16;

    ATMI_LPARM_1D(lparm, numTasks);
    lparm->synchronous = ATMI_TRUE;

    lparm->kernel_id = K_ID_mainTask_gpu;
    mainTask(lparm, numTasks);

	return 0;
}
