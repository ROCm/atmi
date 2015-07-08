#include "atmi_rt.h"
#include "atmi.h"
#include <assert.h>

/* Null kernel */
#ifdef __cplusplus
#define _CPPSTRING_ "C" 
#endif
#ifndef __cplusplus
#define _CPPSTRING_ 
#endif
extern _CPPSTRING_ void __sync_kernel(atmi_task_t *thisTask) {}
static int cpu_initalized = 0;
snk_pif_kernel_table_t __sync_kernel_pif_fn_table[] = {
{.pif_name="__sync_kernel_pif",.devtype=ATMI_DEVTYPE_CPU,.num_params=1,.cpu_kernel={.kernel_name="__sync_kernel",.function=(snk_generic_fp)__sync_kernel},.gpu_kernel={.kernel_name=NULL}},
};

static int                              __sync_kernel_CPU_FK = 0 ; 
atmi_task_t* __sync_kernel_pif(atmi_lparm_t * lparm) {
  int k_id = lparm->kernel_id;
  assert(k_id == 0);
  atmi_devtype_t devtype = __sync_kernel_pif_fn_table[k_id].devtype;
  if(devtype == ATMI_DEVTYPE_GPU) { 
  } 
  else if(devtype == ATMI_DEVTYPE_CPU) { 
      if(cpu_initalized == 0) {
          snk_init_cpu_context();
          cpu_initalized = 1;
      }

      /* Kernel initialization has to be done before kernel arguments are set/inspected */ 
      int num_args = 1; 
      const char *kernel_name = "__sync_kernel"; 
      if (__sync_kernel_CPU_FK == 0 ) { 
          snk_pif_init(__sync_kernel_pif_fn_table, 1); 
          __sync_kernel_CPU_FK = 1; 
      } 
      snk_kernel_args_t *cpu_kernel_arg_list = (snk_kernel_args_t *)malloc(sizeof(snk_kernel_args_t)); 
      cpu_kernel_arg_list->args[0] = (uint64_t)NULL; 
      return snk_cpu_kernel(lparm, 
              "__sync_kernel_pif",
              cpu_kernel_arg_list);
  } 
} 

