/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/

// Do not have printf

// OpenCL header include by default through CL frontend

/**********************************************************************************/
//#include "atmi_device.h"
#include "hw.h"
/**********************************************************************************/

kernel void decode_gpu(
    global const char *in,
    global char *out,
    ulong strlength,
    global char *extra
    ) {

  int num = get_global_id(0);

  if(num < strlength)
    out[num] = in[num] + 1;

#if 1
  if (!num) {
    printf("hello world from GPU, %d, %f\n", num, 1.0);
  }
#endif

}

