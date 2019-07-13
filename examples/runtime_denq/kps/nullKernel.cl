/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/

#include "atmi_kl.h"
enum {
    K_ID_mainTask = 0,
    K_ID_subTask = 1
};

__kernel void subTask_gpu() {
    return;
}

__kernel void mainTask_gpu(long int numTasks) {
	int gid = get_global_id(0);
    int num_wavefronts = get_num_groups(0);
    if(gid % 64 == 0) {
        ATMI_LPARM_1D(lparm, 1);
        lparm->place = (atmi_place_t)ATMI_PLACE_GPU(0, 0);
        // default case for kernel enqueue: lparm->groupable = ATMI_TRUE;
        for(long int i = 0; i < numTasks/num_wavefronts; i++)
            atmid_task_launch(lparm, K_ID_subTask, NULL, 0);
    }
}

