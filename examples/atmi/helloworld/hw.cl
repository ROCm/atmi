#include "atmi.h"
__kernel void decode_gpu(__global atmi_task_t *thisTask, __global char* in, __global unsigned long int *out, const size_t strlength) {
	int num = get_global_id(0);
    if(num == 0)
    {
        *out = in;
    }
    //if(num < strlength) 
    //out[num] = in[num] + 1;
}
