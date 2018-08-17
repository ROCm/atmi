/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/

// Have printf natively

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// But no OpenCL header
#include "atmi_runtime.h"

// CPU implementation using function pointer.
static void decode_cpu_fn(const char *in, char *out, size_t strlength, char *extra);

_CPPSTRING_ void decode_cpu(const char **in, char **out, size_t *strlength, char **extra) {
  decode_cpu_fn(*in, *out, *strlength, *extra);
}

/**********************************************************************************/
//#include "atmi_device.h"
#include "hw.h"
/**********************************************************************************/

void decode_cpu_fn(
    const char *in,
    char *out,
    size_t strlength,
    char *extra
    ) {

  int num = get_global_id(0);

  if(num < strlength)
    out[num] = in[num] + 1;

#if 1
  if (!num) {
    printf("hello world from CPU, %d, %f\n", num, 1.0);
  }
#endif

}

