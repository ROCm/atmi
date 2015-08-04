#include "atmi.h"
__kernel void subTask_gpu(__global atmi_task_t *thisTask, int taskId) {
    ATMI_KLPARM_1D(klparm, 1, thisTask);
    klparm->kernel_id = K_ID_print_taskId_cpu; //tell print_taskId to use print_taskId_cpu
    print(klparm, taskId);
}

__kernel void mainTask_gpu(__global atmi_task_t *thisTask, int numTasks) {
	int gid = get_global_id(0);
    ATMI_KLPARM_1D(klparm, 1, thisTask);
    klparm->kernel_id = K_ID_subTask_gpu; //tell decode to use decode_gpu kernel
    subTask(klparm, gid);
}
