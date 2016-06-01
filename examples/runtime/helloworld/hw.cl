#include "atmi.h"
#include "hw_structs.h"
__kernel void decode_gpu(atmi_task_handle_t thisTask, __global void *args) {
    decode_args_t *gpu_args = (decode_args_t *)args;
    size_t strlength = gpu_args->strlength; 
    const char *in = gpu_args->in;
    char *out = gpu_args->out;
	int num = get_global_id(0);
    if(num < strlength) 
    	out[num] = in[num] + 1;
}
