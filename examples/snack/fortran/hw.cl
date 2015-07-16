__kernel void decode(__global char* in, __global char* out) {
	int num = get_global_id(0);
	out[num] = in[num] + 1;
}
__kernel void encode(__global char*in, __global char* out) {
	int num = get_global_id(0);
        out[num] = in[num] - 1;
}

__kernel void super_encode(__global char*in, __global char* out) {
	int num = get_global_id(0);
        out[num] = in[num] - 2;
}
__kernel void super_decode(__global char*in, __global char* out) {
	int num = get_global_id(0);
        out[num] = in[num] + 2;
}
