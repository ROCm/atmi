/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/

__kernel void decode_gpu(__global const char* in, __global char* out, const size_t strlength) {
	int num = get_global_id(0);
    if(num < strlength) 
    	out[num] = in[num] + 1;
}
