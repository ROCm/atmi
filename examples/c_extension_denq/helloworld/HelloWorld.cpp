/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/

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
