#include "snk.h"
#include "atmi.h"

/* Kernel specific globals, one set for each kernel  */
int                              __sync_kernel_CPU_FK = 0 ; 
status_t                         __sync_kernel_cpu_init(const char *kernel_name, const int num_args, snk_generic_fp fn_ptr);
status_t                         __sync_kernel_stop();
hsa_agent_t                      ctx_CPU_Agent;
hsa_region_t                     ctx_CPU_KernargRegion;
int                              ctx_FC = 0; 

/* Null kernel */
#ifdef __cplusplus
#define _CPPSTRING_ "C" 
#endif
#ifndef __cplusplus
#define _CPPSTRING_ 
#endif
extern _CPPSTRING_ void __sync_kernel(atmi_task_t *thisTask) {}

status_t ctx_InitContext(){
    return snk_init_cpu_context(
                            &ctx_CPU_Agent,
                            &ctx_CPU_KernargRegion
                            );
} /* end of _hw_InitContext */


/* ------  Start of ATMI's PIF function __sync_kernel_pif ------ */ 
atmi_task_t* __sync_kernel_pif(atmi_lparm_t * lparm) {
  if(lparm->devtype == ATMI_DEVTYPE_GPU) { 
  } 
  else if(lparm->devtype == ATMI_DEVTYPE_CPU) { 
    /* Kernel initialization has to be done before kernel arguments are set/inspected */ 
    int num_args = 1; 
    const char *kernel_name = "__sync_kernel"; 
    if (__sync_kernel_CPU_FK == 0 ) { 
      status_t status = __sync_kernel_cpu_init(kernel_name, 
                                                num_args,
                                                (snk_generic_fp)__sync_kernel); 
      if ( status  != STATUS_SUCCESS ) return; 
      __sync_kernel_CPU_FK = 1; 
    } 
    snk_kernel_args_t *cpu_kernel_arg_list = (snk_kernel_args_t *)malloc(sizeof(snk_kernel_args_t)); 
    cpu_kernel_arg_list->args[0] = (uint64_t)NULL; 
    return snk_cpu_kernel(lparm, 
                "__sync_kernel",
                cpu_kernel_arg_list);
  } 
} 

extern status_t __sync_kernel_cpu_init(const char *kernel_name, 
                             const int num_args, 
                             snk_generic_fp fn_ptr) {
    if (ctx_FC == 0 ) {
       status_t status = ctx_InitContext();
       if ( status  != STATUS_SUCCESS ) return; 
       ctx_FC = 1;
    }
    return snk_init_cpu_kernel(kernel_name, 
                               num_args, fn_ptr);
} /* end of __sync_kernel_init */

extern status_t __sync_kernel_stop(){
    status_t err;
    if (ctx_FC == 0 ) {
       /* weird, but we cannot stop unless we initialized the context */
       err = ctx_InitContext();
       if ( err != STATUS_SUCCESS ) return err; 
       ctx_FC = 1;
    }
    if ( __sync_kernel_CPU_FK == 1 ) {
        /*  Currently nothing kernel specific must be recovered */
       __sync_kernel_CPU_FK = 0;
    }
    return STATUS_SUCCESS;

} /* end of __sync_kernel_stop */


