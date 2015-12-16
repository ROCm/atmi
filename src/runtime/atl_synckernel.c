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
extern _CPPSTRING_ void __sync_kernel(atmi_task_t *thisTask) {}
extern _CPPSTRING_ void __sync_kernel_wrapper(atmi_task_t *thisTaskPtr) {__sync_kernel(thisTaskPtr);}
static int cpu_initalized = 0;
static hsa_executable_t g_executable;

snk_pif_kernel_table_t __sync_kernel_pif_fn_table[] = {
{.pif_name="__sync_kernel_pif",.devtype=ATMI_DEVTYPE_CPU,.num_params=1,.cpu_kernel={.kernel_name="__sync_kernel_wrapper",.function=(atmi_generic_fp)__sync_kernel_wrapper},.gpu_kernel={.kernel_name=NULL}},
};

static int                              __sync_kernel_CPU_FK = 0 ; 
extern _CPPSTRING_ atmi_task_t* __sync_kernel_pif(atmi_lparm_t * lparm) {
  int k_id = lparm->kernel_id;
  assert(k_id == 0);
  atmi_devtype_t devtype = __sync_kernel_pif_fn_table[k_id].devtype;
  if(devtype == ATMI_DEVTYPE_GPU) { 
  } 
  else if(devtype == ATMI_DEVTYPE_CPU) { 

      /* Kernel initialization has to be done before kernel arguments are set/inspected */ 
      int num_args = 1; 
      const char *kernel_name = "__sync_kernel"; 
      if (__sync_kernel_CPU_FK == 0 ) { 
          snk_pif_init(__sync_kernel_pif_fn_table, 1); 
          __sync_kernel_CPU_FK = 1; 
      }
      if(cpu_initalized == 0) {
          snk_init_cpu_context();
          cpu_initalized = 1;
      }
      typedef struct cpu_args_s {
          //size_t arg0_size;
          atmi_task_t *arg0;
      } cpu_args_t;
      size_t cpu_args_total_size = sizeof(cpu_args_t);
      void *thisKernargAddress = malloc(cpu_args_total_size);
      cpu_args_t *args = (cpu_args_t *)thisKernargAddress;
      //args->arg0_size = sizeof(atmi_task_t **);
      args->arg0 = NULL;
      
      return atl_trylaunch_pif(lparm, 
              &g_executable,
              "__sync_kernel_pif",
              thisKernargAddress);
  } 
} 

