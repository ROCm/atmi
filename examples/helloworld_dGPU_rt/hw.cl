#include "atmi.h"
__kernel void decode_gpu(atmi_task_handle_t thisTask, __global const char *in, __global char *out, size_t strlength) {
	int num = get_global_id(0);
    if(num < strlength) 
    	out[num] = in[num] + 1;
}
