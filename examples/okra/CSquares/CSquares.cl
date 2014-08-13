kernel csquares(global float *out, global float *in) {
	int i = get_global_id(0);
	out[i] = in[i] * in[i];
}

