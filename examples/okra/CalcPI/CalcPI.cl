__kernel void calcPI(global float *x, global float *y, global int *out) {
	int i = get_global_id(0);

	float c = x[i]*x[i] + y[i]*y[i];
	out[i] = 0;
	if (c <= 1) out[i] = 1;
}
