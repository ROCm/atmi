/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/

#include "atmi.h"
__kernel void subTask_gpu(atmi_task_handle_t thisTask, int taskId) {
    ATMI_KLPARM_1D(klparm, 1, thisTask);
    klparm->kernel_id = K_ID_print_taskId_cpu; //tell print_taskId to use print_taskId_cpu
    print(klparm, taskId);
}

__kernel void mainTask_gpu(atmi_task_handle_t thisTask, int numTasks) {
	int gid = get_global_id(0);
    ATMI_KLPARM_1D(klparm, 1, thisTask);
    klparm->kernel_id = K_ID_subTask_gpu; //tell decode to use decode_gpu kernel
    subTask(klparm, gid);
    //klparm->kernel_id = K_ID_print_taskId_cpu; //tell print_taskId to use print_taskId_cpu
    //print(klparm, gid);
}
