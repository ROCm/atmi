#include "atmi.h"
__kernel void subTask_gpu(atmi_task_handle_t thisTask) {
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
    //if(gid == 0) {
    ATMI_KLPARM_1D(klparm, 1, thisTask);
    klparm->kernel_id = K_ID_mainTask_recursive_gpu; 
    if(numTasks > 0)
        mainTask(klparm, numTasks-1);
    //}
}

__kernel void mainTask_binary_tree_gpu(atmi_task_handle_t thisTask, int numTasks) {
    int gid = get_global_id(0);
    //if(gid == 0) {
    ATMI_KLPARM_1D(klparm, 1, thisTask);
    klparm->kernel_id = K_ID_mainTask_binary_tree_gpu; 
    if(numTasks > 1) {
        mainTask(klparm, numTasks/2);
        mainTask(klparm, numTasks/2);
    }
    //}
}
