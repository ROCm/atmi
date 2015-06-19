#ifndef __SNK_INTERNAL
#define __SNK_INTERNAL
#include "snk.h"
#include <pthread.h>
/* ---------------------------------------------------------------------------------
 * Simulated CPU Data Structures and API
 * --------------------------------------------------------------------------------- */

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
    atmi_task_t *task;
    atmi_devtype_t devtype;
    struct snk_task_list_s *next;
} snk_task_list_t;

typedef struct atmi_stream_table_s {
    atmi_stream_t *stream;
    snk_task_list_t *tasks;
    hsa_queue_t *gpu_queue;
    hsa_queue_t *cpu_queue;
    atmi_devtype_t last_device_type;
}atmi_stream_table_t;

/*
typedef struct atmi_task_table_s {
    atmi_task_t *task;
    hsa_signal_t handle;
} atmi_task_table_t;
*/
extern atmi_task_t   SNK_Tasks[SNK_MAX_TASKS];
extern int           SNK_NextTaskId;

int get_stream_id(atmi_stream_t *stream);
hsa_queue_t *acquire_and_set_next_cpu_queue(atmi_stream_t *stream);
hsa_queue_t *acquire_and_set_next_gpu_queue(atmi_stream_t *stream);
status_t clear_saved_tasks(atmi_stream_t *stream);
status_t register_task(atmi_stream_t *stream, atmi_task_t *task, atmi_devtype_t devtype);
status_t register_stream(atmi_stream_t *stream);
void set_task_state(atmi_task_t *t, atmi_state_t state);
void set_task_metrics(atmi_task_t *task, atmi_devtype_t devtype);

uint16_t create_header(hsa_packet_type_t type, int barrier);

#if 1
typedef struct hsa_amd_profiling_dispatch_time_s { 
    uint64_t start; 
    uint64_t end; 
} hsa_amd_profiling_dispatch_time_t;
hsa_status_t HSA_API hsa_amd_profiling_set_profiler_enabled(hsa_queue_t* queue, int enable);
hsa_status_t HSA_API hsa_amd_profiling_get_dispatch_time(
    hsa_agent_t agent, hsa_signal_t signal,
    hsa_amd_profiling_dispatch_time_t* time);

#endif 
#endif //__SNK_INTERNAL
