#include "atmi.h"
__kernel void decode(__global atmi_task_t *thisTask, __global const char* in, __global char* out) {
	int num = get_global_id(0);
	out[num] = in[num] + 1;
}

