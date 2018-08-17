/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/

#include <iostream>
#include <stdio.h>
using namespace std;
#include "atmi_runtime.h"

enum {
    GPU_IMPL = 42,
    CPU_IMPL = 10565
};

extern "C" void print_taskId_cpu(long int *taskId)
{
    cout << "Leaf Sub-task ID" << ": " << *taskId << endl;
}

int main(int argc, char* argv[]) {
    atmi_status_t err = atmi_init(ATMI_DEVTYPE_ALL);
    if(err != ATMI_STATUS_SUCCESS) return -1;
    const char *module = "hw.hsaco";
    atmi_platform_type_t module_type = AMDGCN;
    err = atmi_module_register(&module, &module_type, 1);

    atmi_kernel_t main_kernel, sub_kernel, print_kernel;
    const unsigned int num_args = 1;
    size_t arg_sizes[] = { sizeof(long int) };
    atmi_kernel_create(&main_kernel, num_args, arg_sizes, 
                       1, 
                       ATMI_DEVTYPE_GPU, "mainTask_gpu");
    atmi_kernel_create(&print_kernel, num_args, arg_sizes,
                       2,
                       ATMI_DEVTYPE_GPU, "print_taskId_gpu",
                       ATMI_DEVTYPE_CPU, (atmi_generic_fp)print_taskId_cpu);
    atmi_kernel_create(&sub_kernel, num_args, arg_sizes,
                       1,
                       ATMI_DEVTYPE_GPU, "subTask_gpu");

    unsigned long int numTasks = 16;
    ATMI_LPARM_1D(lparm, 64 * numTasks);
    //lparm->WORKITEMS = numTasks;
    //lparm->groupDim[0] = numTasks;
    lparm->synchronous = ATMI_TRUE;
    lparm->place = ATMI_PLACE_GPU(0, 0);
    lparm->groupable = ATMI_TRUE;
    //lparm->kernel_id = 0;//GPU_IMPL;

    void *args[] = { &numTasks };
    atmi_task_launch(lparm, main_kernel, args);
    
    printf("Done!\n");

    atmi_kernel_release(main_kernel);
    atmi_kernel_release(print_kernel);
    atmi_kernel_release(sub_kernel);
    atmi_finalize();
	return 0;
}
