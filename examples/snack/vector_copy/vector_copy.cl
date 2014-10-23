__kernel void vcopy(__global int *a, __global int *b) {
  int id = get_global_id(0);
  b[id] = a[id];
}
