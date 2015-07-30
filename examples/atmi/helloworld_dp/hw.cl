#include "atmi.h"
__kernel void decode_gpu(__global atmi_task_t *thisTask, __global const char* in, __global char* out, const size_t strlength, int kid) {
	int num = get_global_id(0);
    if(num < strlength) 
        out[num] = in[num] + 1;
    barrier(CLK_GLOBAL_MEM_FENCE);

    if(num == 0)
    {
        ATMI_KLPARM_1D(klparm, 1);
        klparm->kernel_id = 0; //tell print to use print_cpu kernel
        print(klparm, out, strlength, kid);
    }
}

__kernel void split_gpu(__global atmi_task_t *thisTask, __global const char* in, __global char* out, const size_t strlength) {
	int gid = get_global_id(0);
    ATMI_KLPARM_1D(klparm, strlength);
    klparm->kernel_id = 0; //tell decode to use decode_gpu kernel
    decode(klparm, in, out + ((strlength + 1) * gid), strlength, gid);
}
