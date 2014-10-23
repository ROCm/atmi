__kernel void csquares(__global float *out, __global float *in) {
	int i = get_global_id(0);
	out[i] = in[i] * in[i];
}

