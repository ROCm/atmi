typedef struct {
	float x;
	float y;
	float z;
} Point;

kernel void coop(global float *out, global Point *in) {
  	int id = get_global_id(0);
	out[id] = sqrt(in[id].x*in[id].x + in[id].y*in[id].y + in[id].z*in[id].z);
}
