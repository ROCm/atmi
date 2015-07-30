#include "atmi.h"
__kernel void reduction_gpu(__global atmi_task_t *thisTask, __global int* in, int length) {
	int num = get_global_id(0);

    in[num] += in[num + length];

    barrier(CLK_GLOBAL_MEM_FENCE);

    if(num == 0)
    {
        length = length >> 1;
        if(length > 0)
        {
            ATMI_KLPARM_1D(klparm, length);
            klparm->kernel_id = 0;
            reduction(klparm, in, length);
        }
    }
}
