/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/

kernel void decode_gpu(global const char *in, int strlength, global char *out) {
  int num = get_global_id(0);
  if(num < strlength)
    out[num] = in[num] + 1;
}
