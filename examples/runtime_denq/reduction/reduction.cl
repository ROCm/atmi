/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/

#include "atmi_kl.h"

enum { 
    reduction_task = 0, 
};

typedef struct args_r { 
    int *in;
    unsigned long length;
} args_t;

kernel void reduction_gpu(__global int* in, unsigned long length) {
	int num = get_global_id(0);

    in[num] += in[num + length];

    barrier(CLK_GLOBAL_MEM_FENCE);

    if(num == 0)
    {
        length = length >> 1;
        ATMI_LPARM_1D(lparm, length);
        if(length > 8) 
            lparm->place = (atmi_place_t)ATMI_PLACE_GPU(0, 0);
        else 
            lparm->place = (atmi_place_t)ATMI_PLACE_CPU(0, 0);
        args_t args;
        args.in = in;
        args.length = length;
        atmid_task_launch(lparm, reduction_task, (void *)&args, sizeof(args_t));
    }
}
