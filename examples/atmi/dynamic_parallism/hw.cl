#include "atmi.h"
__kernel void reduction_gpu(__global atmi_task_t *thisTask, __global int* in, int length) {
	int num = get_global_id(0);

    in[num] += in[num + length];

    barrier(CLK_GLOBAL_MEM_FENCE);

    if(num == 0)
    {
        //in[0] = thisTask->klist->num_kernel_packets;
        //in[1] = thisTask->klist->queues[0];
        //in[2] = thisTask->klist->kernel_packets->kernarg_address;
        //in[3] = thisTask->klist->kernel_packets->kernel_object;
        length = length >> 1;
        if(length > 0)
        {
            INIT_KLPARM_1D(klparm, length);
            reduction(klparm, thisTask, in, length);
        }
    }
}

