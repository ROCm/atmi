#ifndef __SNK_INTERNAL
#define __SNK_INTERNAL
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

#include "snk.h"

#include <pthread.h>

#define SNK_MAX_CPU_QUEUES 4
#define SNK_MAX_GPU_QUEUES 8
#define SNK_MAX_CPU_FUNCTIONS   100

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

typedef struct snk_kernel_args_s {
    uint64_t args[20];
} snk_kernel_args_t;

/* ---------------------------------------------------------------------------------
 * Simulated CPU Data Structures and API
 * --------------------------------------------------------------------------------- */

typedef enum status_t {
    STATUS_SUCCESS=0,
    STATUS_UNKNOWN=1,
    STATUS_ERROR=2
} status_t;

typedef struct agent_t
{
  int num_queues;
  int id;
  hsa_queue_t *queue;
  //hsa_agent_t cpu_agent;
  //hsa_region_t cpu_region;
} agent_t;

enum {
    PROCESS_PKT = 0,
    FINISH,
    IDLE
};

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

void set_cpu_kernel_table(const cpu_kernel_table_t *kernel_table); 
agent_t get_cpu_q_agent(int id);
void cpu_agent_init(hsa_agent_t cpu_agent, hsa_region_t cpu_region, 
                const size_t num_queues, const size_t capacity
                );
void agent_fini();
hsa_queue_t* get_cpu_queue(int id);
void signal_worker(hsa_queue_t *queue, int signal);
void *agent_worker(void *agent_args);
int process_packet(hsa_queue_t *queue, int id);

typedef struct snk_task_list_s {
    snk_task_t *task;
    struct snk_task_list_s *next;
} snk_task_list_t;

typedef struct snk_stream_table_s {
    snk_stream_t *stream;
    snk_task_list_t *tasks;
    hsa_queue_t *gpu_queue;
    hsa_queue_t *cpu_queue;
    snk_device_type_t last_device_type;
}snk_stream_table_t;

/*
typedef struct snk_task_table_s {
    snk_task_t *task;
    hsa_signal_t handle;
} snk_task_table_t;
*/
extern snk_task_t   SNK_Tasks[SNK_MAX_TASKS];
extern int          SNK_NextTaskId;

int get_stream_id(snk_stream_t *stream);
hsa_queue_t *acquire_and_set_next_cpu_queue(snk_stream_t *stream);
hsa_queue_t *acquire_and_set_next_gpu_queue(snk_stream_t *stream);
status_t reset_tasks(snk_stream_t *stream);
status_t register_cpu_task(snk_stream_t *stream, snk_task_t *task);
status_t register_gpu_task(snk_stream_t *stream, snk_task_t *task);
status_t register_task(int stream_num, snk_task_t *task);
status_t register_stream(snk_stream_t *stream);

status_t snk_init_context(
                        hsa_agent_t *_CN__Agent, 
                        hsa_ext_module_t **_CN__BrigModule,
                        hsa_ext_program_t *_CN__HsaProgram,
                        hsa_executable_t *_CN__Executable,
                        hsa_region_t *_CN__KernargRegion,
                        hsa_agent_t *_CN__CPU_Agent,
                        hsa_region_t *_CN__CPU_KernargRegion
                        );

status_t snk_init_kernel(hsa_executable_symbol_t          *_KN__Symbol,
                            const char *kernel_symbol_name,
                            uint64_t                         *_KN__Kernel_Object,
                            uint32_t                         *_KN__Kernarg_Segment_Size, /* May not need to be global */
                            uint32_t                         *_KN__Group_Segment_Size,
                            uint32_t                         *_KN__Private_Segment_Size,
                            hsa_agent_t _CN__Agent, 
                            hsa_executable_t _CN__Executable
                            );

snk_task_t *snk_gpu_kernel(const snk_lparm_t *lparm, 
                 uint64_t                         _KN__Kernel_Object,
                 uint32_t                         _KN__Group_Segment_Size,
                 uint32_t                         _KN__Private_Segment_Size,
                 void *thisKernargAddress);

snk_task_t *snk_cpu_kernel(const snk_lparm_t *lparm, 
                 const cpu_kernel_table_t *_CN__CPU_kernels,
                 const char *kernel_name,
                 const uint32_t _KN__cpu_task_num_args,
                 const snk_kernel_args_t *kernel_args);

uint16_t create_header(hsa_packet_type_t type, int barrier);

#endif //__SNK_INTERNAL
