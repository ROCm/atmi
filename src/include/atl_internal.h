#ifndef __SNK_INTERNAL
#define __SNK_INTERNAL
#include "atl_rt.h"
#include <pthread.h>
#include <vector>
#include <queue>
#include <map>
#include <atomic>
/* ---------------------------------------------------------------------------------
 * Simulated CPU Data Structures and API
 * --------------------------------------------------------------------------------- */
long int get_nanosecs( struct timespec start_time, struct timespec end_time);
typedef void* ARG_TYPE;
#define COMMA ,
#define REPEAT(name)   COMMA name
#define REPEAT2(name)  REPEAT(name)   REPEAT(name) 
#define REPEAT4(name)  REPEAT2(name)  REPEAT2(name)
#define REPEAT8(name)  REPEAT4(name)  REPEAT4(name)
#define REPEAT16(name) REPEAT8(name) REPEAT8(name)

#define SNK_MAX_SIGNALS 4096
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

typedef struct atl_task_s atl_task_t;
typedef struct snk_task_list_s {
    atl_task_t *task;
    atmi_devtype_t devtype;
    boolean profilable;
    struct snk_task_list_s *next;
} snk_task_list_t;

typedef struct atmi_stream_table_s {
    snk_task_list_t *tasks;
    hsa_queue_t *gpu_queue;
    hsa_queue_t *cpu_queue;
    atmi_devtype_t last_device_type;
    int next_gpu_qid;
    int next_cpu_qid;
}atmi_stream_table_t;

typedef enum atl_dep_sync_s {
    ATL_SYNC_BARRIER_PKT    = 0,
    ATL_SYNC_CALLBACK       = 1
} atl_dep_sync_t;

extern struct timespec context_init_time;
extern pthread_mutex_t mutex_all_tasks_;
extern pthread_mutex_t mutex_readyq_;
typedef std::vector<atl_task_t *> atl_task_list_t;

typedef struct atl_task_s {
    // all encmopassing task packet
    // hsa_kernel_dispatch_packet_t *packet;

    void *cpu_kernelargs;
    int cpu_kernelid;
    int num_params;
    void *gpu_kernargptr;
    uint64_t kernel_object;
    uint32_t private_segment_size;
    uint32_t group_segment_size;

    // list of dependents
    uint32_t num_predecessors; // cant we get this from lparm?
    std::atomic<uint32_t> num_successors; // cant we get this from lparm?
    atl_dep_sync_t dep_sync_type;

    // reference to HSA signal and the applications task structure
    hsa_signal_t signal;
    atmi_task_t *atmi_task;
    
    // other miscellaneous flags
    atmi_devtype_t devtype;
    boolean profilable;
    volatile std::atomic<atmi_state_t> state;

    // per task mutex to reduce contention
    pthread_mutex_t mutex;
    const char *name;
    //
    // lparm to control some of the execution flow (synchronous, requires and so
    // on)
    atmi_lparm_t lparm;
    // FIXME: queue or vector?
    atl_task_list_t and_successors;
    atl_task_list_t and_predecessors;
} atl_task_t;

extern std::map<atmi_stream_t *, atmi_stream_table_t> StreamTable;
//atmi_task_table_t TaskTable[SNK_MAX_TASKS];
extern std::vector<atl_task_t *> AllTasks;
extern std::queue<atl_task_t *> ReadyTaskQueue;
extern std::queue<hsa_signal_t> FreeSignalPool;
extern std::map<atmi_task_t *, atl_task_t *> PublicTaskMap;

/*
typedef struct atmi_task_table_s {
    atmi_task_t *task;
    hsa_signal_t handle;
} atmi_task_table_t;
*/
extern int           SNK_NextTaskId;

void init_dag_scheduler();
bool handle_signal(hsa_signal_value_t value, void *arg);

void dispatch_ready_task_for_free_signal();
void dispatch_ready_task_or_release_signal(hsa_signal_t signal);
status_t dispatch_task(atl_task_t *task);
status_t check_change_in_device_type(atmi_stream_t *stream, hsa_queue_t *queue, atmi_devtype_t new_task_device_type);
hsa_signal_t enqueue_barrier_async(hsa_queue_t *queue, const int dep_task_count, atl_task_t **dep_task_list, int barrier_flag);
void enqueue_barrier(hsa_queue_t *queue, const int dep_task_count, atl_task_t **dep_task_list, int wait_flag, int barrier_flag, atmi_devtype_t devtype);

int get_stream_id(atmi_stream_t *stream);
hsa_queue_t *acquire_and_set_next_cpu_queue(atmi_stream_t *stream);
hsa_queue_t *acquire_and_set_next_gpu_queue(atmi_stream_t *stream);
status_t clear_saved_tasks(atmi_stream_t *stream);
status_t register_task(atmi_stream_t *stream, atl_task_t *task, atmi_devtype_t devtype, boolean profilable);
status_t register_stream(atmi_stream_t *stream);
void set_task_state(atl_task_t *t, atmi_state_t state);
void set_task_metrics(atl_task_t *task, atmi_devtype_t devtype, boolean profilable);

void packet_store_release(uint32_t* packet, uint16_t header, uint16_t rest);
uint16_t create_header(hsa_packet_type_t type, int barrier);
hsa_signal_t* get_worker_sig(hsa_queue_t *queue);

#if 0
#ifdef __cplusplus
extern "C" {
#endif 

typedef struct hsa_amd_profiling_dispatch_time_s { 
    uint64_t start; 
    uint64_t end; 
} hsa_amd_profiling_dispatch_time_t;
hsa_status_t HSA_API hsa_amd_profiling_set_profiler_enabled(hsa_queue_t* queue, int enable);
hsa_status_t HSA_API hsa_amd_profiling_get_dispatch_time(
    hsa_agent_t agent, hsa_signal_t signal,
    hsa_amd_profiling_dispatch_time_t* time);

#ifdef __cplusplus
}
#endif //__cplusplus
#endif // 0 
#endif //__SNK_INTERNAL
