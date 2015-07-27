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
/*  set NOTCOHERENT needs this include
#include "hsa_ext_amd.h"
*/

#include "atmi.h"
#include "atmi_kl.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ATMI_MAX_STREAMS            8 
#define ATMI_MAX_TASKS_PER_STREAM   125

#define SNK_MAX_CPU_QUEUES 3
#define SNK_MAX_GPU_QUEUES 4 
#define SNK_MAX_FUNCTIONS   100

#define SNK_MAX_TASKS 100000 // ((ATMI_MAX_STREAMS) * (ATMI_MAX_TASKS_PER_STREAM))

#define SNK_WAIT    1
#define SNK_NOWAIT  0

#define check(msg, status) \
if (status != HSA_STATUS_SUCCESS) { \
    printf("%s failed.\n", #msg); \
    exit(1); \
}
//#define DEBUG_SNK
//#define VERBOSE_SNK
#ifdef DEBUG_SNK
#define DEBUG_PRINT(...) do{ fprintf( stderr, __VA_ARGS__ ); } while( false )
#else
#define DEBUG_PRINT(...) do{ } while ( false )
#endif

#ifdef VERBOSE_SNK
#define VERBOSE_PRINT(...) do{ fprintf( stderr, __VA_ARGS__ ); } while( false )
#else
#define VERBOSE_PRINT(...) do{ } while ( false )
#endif

#ifndef HSA_RUNTIME_INC_HSA_H_
typedef struct hsa_signal_s { uint64_t handle; } hsa_signal_t;
#endif

typedef enum status_t {
    STATUS_SUCCESS=0,
    STATUS_UNKNOWN=1,
    STATUS_ERROR=2
} status_t;

typedef void (*snk_generic_fp)(void);
typedef struct snk_cpu_kernel_s {
    const char *kernel_name; 
    snk_generic_fp function;
} snk_cpu_kernel_t;

typedef struct snk_gpu_kernel_s {
    const char *kernel_name; 
    /* other fields? 
     * hsa_program?
     * hsa_executable?
     */
} snk_gpu_kernel_t;

typedef struct snk_pif_kernel_table_s {
    const char *pif_name; 
    atmi_devtype_t devtype;
    int num_params;
    snk_cpu_kernel_t cpu_kernel;
    snk_gpu_kernel_t gpu_kernel;
} snk_pif_kernel_table_t;


status_t snk_init_context();
status_t snk_init_cpu_context();
status_t snk_init_gpu_context();
status_t snk_gpu_create_program();
status_t snk_gpu_add_brig_module(char _CN__HSA_BrigMem[]);
status_t snk_gpu_build_executable(hsa_executable_t *executable);
status_t snk_gpu_memory_allocate(const atmi_lparm_t *lparm,
                 hsa_executable_t executable,
                 const char *pif_name,
                 void **thisKernargAddress);

status_t snk_init_kernel(
                             const char *pif_name, 
                             const atmi_devtype_t devtype,
                             const int num_params, 
                             const char *cpu_kernel_name, 
                             snk_generic_fp fn_ptr,
                             const char *gpu_kernel_name);
status_t snk_pif_init(snk_pif_kernel_table_t pif_fn_table[], int sz);
status_t snk_get_gpu_kernel_info(
                            hsa_executable_t executable,
                            const char *kernel_symbol_name,
                            uint64_t                         *_KN__Kernel_Object,
                            uint32_t                         *_KN__Group_Segment_Size,
                            uint32_t                         *_KN__Private_Segment_Size
                            );

atmi_task_t *snk_gpu_kernel(const atmi_lparm_t *lparm,
                 hsa_executable_t executable,
                 const char *pif_name,
                 void *thisKernargAddress);

atmi_task_t *snk_cpu_kernel(const atmi_lparm_t *lparm, 
                 const char *pif_name,
                 void *kernel_args);

void snk_kl_init(const atmi_lparm_t *lparm,
                 atmi_klist_t *atmi_klist,
                 hsa_executable_t g_executable,
                 const char *pif_name,
                 const int pif_id);


#ifdef __cplusplus
}
#endif
#endif // __SNK_H__
