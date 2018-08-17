/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/

#include "hw_structs.h"
__kernel void decode_gpu(__global void *args) {
    decode_args_t *gpu_args = (decode_args_t *)args;
    size_t strlength = gpu_args->strlength; 
    const char *in = gpu_args->in;
    char *out = gpu_args->out;
	int num = get_global_id(0);
    if(num < strlength) 
    	out[num] = in[num] + 1;
}
