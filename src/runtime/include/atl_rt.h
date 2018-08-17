/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/
#ifndef __SNK_H__
#define __SNK_H__
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <inttypes.h>

#include "hsa.h"
#include "hsa_ext_finalize.h"
#include "hsa_ext_amd.h"

#include "atmi.h"
#include "atmi_runtime.h"
#include "atmi_kl.h"

#ifdef __cplusplus
extern "C" {
#define _CPPSTRING_ "C"
#endif
#ifndef __cplusplus
#define _CPPSTRING_ 
#endif

#define ATMI_MAX_STREAMS            8 
#define ATMI_MAX_TASKS_PER_STREAM   125

#define SNK_MAX_FUNCTIONS   100

//#define SNK_MAX_TASKS 32 //100000 //((ATMI_MAX_STREAMS) * (ATMI_MAX_TASKS_PER_STREAM))

#define SNK_WAIT    1
#define SNK_NOWAIT  0

#define SNK_OR      1
#define SNK_AND     0

#define check(msg, status) \
if (status != HSA_STATUS_SUCCESS) { \
    printf("%s failed.\n", #msg); \
    exit(1); \
}

#ifdef DEBUG

static const char* debug_mode=getenv("ATMI_DEBUG");

#define DEBUG_SNK
#define VERBOSE_SNK
#endif


#ifdef DEBUG_SNK
#define DEBUG_PRINT(fmt, ...) if (debug_mode) { fprintf ( stderr, "[%s:%d] " fmt, __FILE__, __LINE__, ##__VA_ARGS__);}
#else
#define DEBUG_PRINT(...) do{ } while ( false )
#endif

#ifdef VERBOSE_SNK
#define VERBOSE_PRINT(fmt, ...) if (debug_mode) { fprintf ( stderr, "[%s:%d] " fmt, __FILE__, __LINE__, ##__VA_ARGS__);}
#else
#define VERBOSE_PRINT(...) do{ } while ( false )
#endif

#ifndef HSA_RUNTIME_INC_HSA_H_
typedef struct hsa_signal_s { uint64_t handle; } hsa_signal_t;
#endif

atmi_status_t atl_init_context();
atmi_status_t atl_init_cpu_context();
atmi_status_t atl_init_gpu_context();

hsa_status_t init_hsa();
hsa_status_t finalize_hsa();


atmi_status_t atl_gpu_create_program();
atmi_status_t atl_gpu_add_brig_module(char _CN__HSA_BrigMem[]);
atmi_status_t atl_gpu_build_executable(hsa_executable_t *executable);

atmi_status_t atl_gpu_create_executable(hsa_executable_t *executable);
atmi_status_t atl_gpu_add_finalized_module(hsa_executable_t *executable, char *module, 
                const size_t module_sz);
atmi_status_t atl_gpu_freeze_executable(hsa_executable_t *executable);

atmi_status_t atl_gpu_memory_allocate(const atmi_lparm_t *lparm,
                 hsa_executable_t executable,
                 const char *pif_name,
                 void **thisKernargAddress);

atmi_status_t atl_init_kernel(
                             const char *pif_name, 
                             const atmi_devtype_t devtype,
                             const int num_params, 
                             const char *cpu_kernel_name, 
                             atmi_generic_fp fn_ptr,
                             const char *gpu_kernel_name);
//atmi_status_t atl_pif_init(atl_pif_kernel_table_t pif_fn_table[], int sz);
atmi_status_t atl_get_gpu_kernel_info(
                            hsa_executable_t executable,
                            const char *kernel_symbol_name,
                            uint64_t                         *_KN__Kernel_Object,
                            uint32_t                         *_KN__Group_Segment_Size,
                            uint32_t                         *_KN__Private_Segment_Size,
                            uint32_t                         *_KN__Kernarg_Size
                            );


/*  All global values go in this global structure */
typedef struct atl_context_s {
   int struct_initialized;
   int g_cpu_initialized;
   int g_hsa_initialized;
   int g_gpu_initialized;
   int g_tasks_initialized;
   int g_mutex_dag_initialized;
} atl_context_t ;
extern atl_context_t atlc ;
extern atl_context_t * atlc_p ;

#ifdef __cplusplus
}
#endif
#endif // __SNK_H__
