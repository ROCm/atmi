__kernel void super_encode(__global const char*in, __global char* out) {
	int num = get_global_id(0);
        out[num] = in[num] - 2;
}
__kernel void super_decode(__global const char*in, __global char* out) {
	int num = get_global_id(0);
        out[num] = in[num] + 2;
}
