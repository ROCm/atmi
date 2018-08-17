/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/

#include "atmi.h"
__kernel void subTask_gpu(atmi_task_handle_t thisTask) {
return;
    ATMI_KLPARM_1D(klparm, 1, thisTask);
    klparm->kernel_id = K_ID_subTask_cpu; 
    subTask(klparm);
}

__kernel void mainTask_gpu(atmi_task_handle_t thisTask, int numTasks) {
    int gid = get_global_id(0);
    ATMI_KLPARM_1D(klparm, 1, thisTask);
    klparm->kernel_id = K_ID_subTask_gpu; 
    int i;
    for(i = 0; i < numTasks; i++)
      subTask(klparm);
}

__kernel void mainTask_recursive_gpu(atmi_task_handle_t thisTask, int numTasks) {
    int gid = get_global_id(0);
    int gsize = get_global_size(0);
    //if(gid == 0) {
    if(numTasks > 1) {
        int new_numTasks;; 
        int new_workitems;
        if(gsize >= numTasks) {
            new_numTasks = 1; 
            new_workitems = numTasks;
        }
        else {
            new_numTasks = numTasks/gsize;
            new_workitems = gsize;
        }
        ATMI_KLPARM_1D(klparm, new_workitems, thisTask);
        klparm->kernel_id = K_ID_mainTask_recursive_gpu; 
        mainTask(klparm, new_numTasks);
    }
    ATMI_KLPARM_1D(klparm_sub, 1, thisTask);
    klparm_sub->kernel_id = K_ID_subTask_gpu; 
    subTask(klparm_sub);
    //}
}

__kernel void mainTask_binary_tree_gpu(atmi_task_handle_t thisTask, int numTasks) {
    int gid = get_global_id(0);
    //if(gid == 0) {
    ATMI_KLPARM_1D(klparm, 1, thisTask);
    klparm->kernel_id = K_ID_mainTask_recursive_gpu; 
    if(numTasks > 1) {
        mainTask(klparm, numTasks/2);
        mainTask(klparm, numTasks/2 - 1);
    }
    //}
}

__kernel void mainTask_flat_gpu(atmi_task_handle_t thisTask, int numTasks) {
    if(get_global_id(0) % 64 == 0) {
        ATMI_KLPARM_1D(klparm, 1, thisTask);
        klparm->kernel_id = K_ID_subTask_gpu; 
        subTask(klparm);
    }
}
