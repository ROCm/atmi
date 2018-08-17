/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/

int multiplier = 4;

__kernel void multiply_gpu(__global float *a, size_t sz) {
	int gid = get_global_id(0);
    if(gid < sz) 
    	a[gid] *= multiplier;
}
