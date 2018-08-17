/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/

#include "atmi.h"
__kernel void reduction_gpu(atmi_task_handle_t thisTask, __global int* in, int length) {
	int num = get_global_id(0);

    in[num] += in[num + length];

    barrier(CLK_GLOBAL_MEM_FENCE);

    if(num == 0)
    {
        length = length >> 1;
        ATMI_KLPARM_1D(klparm, length, thisTask);
        if(length > 8)
            klparm->kernel_id = K_ID_reduction_gpu;
        else 
            klparm->kernel_id = K_ID_reduction_cpu;
        reduction(klparm, in, length);
    }
}
