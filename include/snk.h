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

#ifdef __cplusplus
extern "C" {
#endif

#define ATMI_MAX_STREAMS            8 
#define ATMI_MAX_TASKS_PER_STREAM   125

#define SNK_MAX_CPU_QUEUES 4
#define SNK_MAX_GPU_QUEUES 4 
#define SNK_MAX_CPU_FUNCTIONS   100

#define SNK_MAX_TASKS  ((ATMI_MAX_STREAMS) * (ATMI_MAX_TASKS_PER_STREAM))

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

typedef struct snk_kernel_args_s {
    uint64_t args[20];
} snk_kernel_args_t;

typedef struct snk_pif_kernel_table_s {
    const char *pif_name; 
    int num_params;
    snk_cpu_kernel_t cpu_kernel;
    snk_gpu_kernel_t gpu_kernel;
} snk_pif_kernel_table_t;

#define COMMA ,
#define REPEAT(name)   COMMA name
#define REPEAT2(name)  REPEAT(name)   REPEAT(name) 
#define REPEAT4(name)  REPEAT2(name)  REPEAT2(name)
#define REPEAT8(name)  REPEAT4(name)  REPEAT4(name)
#define REPEAT16(name) REPEAT8(name) REPEAT8(name)

status_t snk_init_context(
                        char _CN__HSA_BrigMem[],
                        hsa_region_t *_CN__KernargRegion,
                        hsa_agent_t *_CN__CPU_Agent,
                        hsa_region_t *_CN__CPU_KernargRegion
                        );
status_t snk_init_cpu_context();
status_t snk_init_gpu_context(
                        char _CN__HSA_BrigMem[],
                        hsa_region_t *_CN__KernargRegion
                        );

status_t snk_init_cpu_kernel(
                             const char *pif_name, 
                             const int num_params, 
                             const char *kernel_name, 
                             snk_generic_fp fn_ptr);
status_t snk_pif_init(snk_pif_kernel_table_t pif_fn_table[], int sz);
status_t snk_init_gpu_kernel(hsa_executable_symbol_t          *_KN__Symbol,
                            const char *kernel_symbol_name,
                            uint64_t                         *_KN__Kernel_Object,
                            uint32_t                         *_KN__Kernarg_Segment_Size, /* May not need to be global */
                            uint32_t                         *_KN__Group_Segment_Size,
                            uint32_t                         *_KN__Private_Segment_Size
                            );

atmi_task_t *snk_gpu_kernel(const atmi_lparm_t *lparm, 
                 uint64_t                         _KN__Kernel_Object,
                 uint32_t                         _KN__Group_Segment_Size,
                 uint32_t                         _KN__Private_Segment_Size,
                 void *thisKernargAddress);

atmi_task_t *snk_cpu_kernel(const atmi_lparm_t *lparm, 
                 const char *pif_name,
                 snk_kernel_args_t *kernel_args);

#ifdef __cplusplus
}
#endif
#endif // __SNK_H__
