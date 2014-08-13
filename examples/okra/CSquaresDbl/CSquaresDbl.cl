kernel int squares(global double *out, global double *in, double adj) {
	int id = get_global_id(0);
	out[id] = in[id]*in[id] + adj;
}
