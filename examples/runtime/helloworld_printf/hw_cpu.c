/*
 * MIT License
 *
 * Copyright © 2016 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software
 * without restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies of the
 * Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 * */

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
#include "atmi_device.h"
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

