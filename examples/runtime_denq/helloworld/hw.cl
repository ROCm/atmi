/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/

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
    
    atmid_task_launch(lparm, K_ID_print_taskId_cpu, (void *)&args, sizeof(args_t));
}

__kernel void mainTask_gpu(long int numTasks) {
	int gid = get_global_id(0);
    if(gid % 64 == 0) {
        ATMI_LPARM_1D(lparm, 1);
        lparm->place = (atmi_place_t)ATMI_PLACE_GPU(0, 0);
        // default case for kernel enqueue: lparm->groupable = ATMI_TRUE;
        args_t args;
        args.arg1 = gid;

        atmid_task_launch(lparm, K_ID_subTask_gpu, (void *)&args, sizeof(args_t));
    }
}

__kernel void print_taskId_gpu(long int taskId) {
}

