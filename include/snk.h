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

#define ATMI_MAX_STREAMS            8 
#define ATMI_MAX_TASKS_PER_STREAM   500

#define SNK_MAX_CPU_QUEUES 4
#define SNK_MAX_GPU_QUEUES 8
#define SNK_MAX_CPU_FUNCTIONS   100

#define SNK_MAX_TASKS   ((ATMI_MAX_STREAMS) * (ATMI_MAX_TASKS_PER_STREAM))

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

typedef struct snk_kernel_args_s {
    uint64_t args[20];
} snk_kernel_args_t;

#define COMMA ,
#define REPEAT(name)   COMMA name
#define REPEAT2(name)  REPEAT(name)   REPEAT(name) 
#define REPEAT4(name)  REPEAT2(name)  REPEAT2(name)
#define REPEAT8(name)  REPEAT4(name)  REPEAT4(name)
#define REPEAT16(name) REPEAT8(name) REPEAT8(name)

typedef struct cpu_kernel_table_s {
    const char *name; 
    union {
        void (*function0) (void);
        void (*function1) (uint64_t);
        void (*function2) (uint64_t,uint64_t);
        void (*function3) (uint64_t,uint64_t,uint64_t);
        void (*function4) (uint64_t,uint64_t,uint64_t,uint64_t);
        void (*function5) (uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t);
        void (*function6) (uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t);
        void (*function7) (uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t);
        void (*function8) (uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t);
        void (*function9) (uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t);
        void (*function10) (uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t);
        void (*function11) (uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t);
        void (*function12) (uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t);
        void (*function13) (uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t);
        void (*function14) (uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t);
        void (*function15) (uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t);
        void (*function16) (uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t);
        void (*function17) (uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t);
        void (*function18) (uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t);
        void (*function19) (uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t);
        void (*function20) (uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t,
                            uint64_t,uint64_t);
    } function;
} cpu_kernel_table_t;


status_t snk_init_context(
                        hsa_agent_t *_CN__Agent, 
                        hsa_ext_module_t **_CN__BrigModule,
                        hsa_ext_program_t *_CN__HsaProgram,
                        hsa_executable_t *_CN__Executable,
                        hsa_region_t *_CN__KernargRegion,
                        hsa_agent_t *_CN__CPU_Agent,
                        hsa_region_t *_CN__CPU_KernargRegion
                        );

status_t snk_init_cpu_kernel();
status_t snk_init_gpu_kernel(hsa_executable_symbol_t          *_KN__Symbol,
                            const char *kernel_symbol_name,
                            uint64_t                         *_KN__Kernel_Object,
                            uint32_t                         *_KN__Kernarg_Segment_Size, /* May not need to be global */
                            uint32_t                         *_KN__Group_Segment_Size,
                            uint32_t                         *_KN__Private_Segment_Size,
                            hsa_agent_t _CN__Agent, 
                            hsa_executable_t _CN__Executable
                            );

atmi_task_t *snk_gpu_kernel(const atmi_lparm_t *lparm, 
                 uint64_t                         _KN__Kernel_Object,
                 uint32_t                         _KN__Group_Segment_Size,
                 uint32_t                         _KN__Private_Segment_Size,
                 void *thisKernargAddress);

atmi_task_t *snk_cpu_kernel(const atmi_lparm_t *lparm, 
                 const cpu_kernel_table_t *_CN__CPU_kernels,
                 const char *kernel_name,
                 const uint32_t _KN__cpu_task_num_args,
                 const snk_kernel_args_t *kernel_args);
#endif // __SNK_H__
