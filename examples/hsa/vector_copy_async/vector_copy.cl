__kernel void vector_copy(__global int *in, __global int *out, int offset) {
  int id = get_global_id(0);
  out[id + offset] = in[id +offset];
}
