/*
MIT License 

Copyright © 2016 Advanced Micro Devices, Inc.  

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software
without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include "atl_rt.h"
#include "atmi.h"
#include <assert.h>

/* Null kernel */
#ifdef __cplusplus
#define _CPPSTRING_ "C" 
#endif
#ifndef __cplusplus
#define _CPPSTRING_ 
#endif
extern _CPPSTRING_ void __sync_kernel() {}
extern _CPPSTRING_ void __sync_kernel_wrapper() {__sync_kernel();}
static int cpu_initalized = 0;

typedef struct pif_kernel_table_s {
    atmi_devtype_t devtype; 
    atmi_generic_fp cpu_kernel; 
    const char *gpu_kernel; 
} pif_kernel_table_t;

pif_kernel_table_t __sync_kernel_pif_fn_table[] = {
{.devtype=ATMI_DEVTYPE_CPU,.cpu_kernel=(atmi_generic_fp)__sync_kernel_wrapper,.gpu_kernel=0},
};

static int                              __sync_kernel_CPU_FK = 0 ; 
static atmi_kernel_t                    __sync_kernel_obj;
extern _CPPSTRING_ atmi_task_handle_t __sync_kernel_pif(atmi_lparm_t * lparm) {
  int k_id = lparm->kernel_id;
  assert(k_id == 0);
  atmi_devtype_t devtype = __sync_kernel_pif_fn_table[k_id].devtype;
  if(devtype == ATMI_DEVTYPE_GPU) { 
  } 
  else if(devtype == ATMI_DEVTYPE_CPU) { 

      /* Kernel initialization has to be done before kernel arguments are set/inspected */ 
      const char *kernel_name = "__sync_kernel"; 
      const int num_args = 0; 
      if (__sync_kernel_CPU_FK == 0 ) { 
          atmi_kernel_create_empty(&__sync_kernel_obj, num_args, NULL);
          atmi_kernel_add_cpu_impl(__sync_kernel_obj, (atmi_generic_fp)(__sync_kernel_pif_fn_table[0].cpu_kernel), 0);
          __sync_kernel_CPU_FK = 1; 
      }
      if(cpu_initalized == 0) {
          atmi_init(ATMI_DEVTYPE_CPU);
          cpu_initalized = 1;
      }
      return atmi_task_launch(lparm, __sync_kernel_obj, NULL); 
  } 
} 

