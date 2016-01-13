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

#define ATMI_WAIT_STATE HSA_WAIT_STATE_BLOCKED
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
typedef struct atl_task_list_s {
    atl_task_t *task;
    atmi_devtype_t devtype;
    boolean profilable;
    struct atl_task_list_s *next;
} atl_task_list_t;

typedef struct atmi_stream_table_s {
    boolean ordered;
    atl_task_list_t *tasks;
    hsa_queue_t *gpu_queue;
    hsa_queue_t *cpu_queue;
    atmi_devtype_t last_device_type;
    int next_gpu_qid;
    int next_cpu_qid;
    hsa_signal_t stream_common_signal;
    pthread_mutex_t mutex;
}atmi_stream_table_t;

typedef enum atl_dep_sync_s {
    ATL_SYNC_BARRIER_PKT    = 0,
    ATL_SYNC_CALLBACK       = 1
} atl_dep_sync_t;

typedef struct atl_kernel_info_s {
    uint64_t kernel_object;
    uint32_t group_segment_size;
    uint32_t private_segment_size;
    uint32_t kernel_segment_size;
} atl_kernel_info_t;
extern struct timespec context_init_time;
extern pthread_mutex_t mutex_all_tasks_;
extern pthread_mutex_t mutex_readyq_;
typedef std::vector<atl_task_t *> atl_task_vector_t;

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
    uint32_t num_successors; // cant we get this from lparm?
    atl_dep_sync_t dep_sync_type;

    // reference to HSA signal and the applications task structure
    hsa_signal_t signal;
    atmi_task_t *atmi_task;
    atmi_stream_table_t *stream_obj;
    
    // other miscellaneous flags
    atmi_devtype_t devtype;
    boolean profilable;
    std::atomic<atmi_state_t> state;

    // per task mutex to reduce contention
    pthread_mutex_t mutex;

    // lparm to control some of the execution flow (synchronous, requires, etc)
    atmi_lparm_t lparm;
    // FIXME: queue or vector?
    atl_task_vector_t and_successors;
    atl_task_vector_t and_predecessors;
    atl_task_vector_t predecessors;
    std::vector<hsa_signal_t> barrier_signals;
    int id;
    // flag to differentiate between a regular task and a continuation
    // FIXME: probably make this a class hierarchy?
    boolean is_continuation;

    atl_task_s() : cpu_kernelargs(0), cpu_kernelid(-1), num_params(-1), gpu_kernargptr(0), kernel_object(0), private_segment_size(0),
                   group_segment_size(0), num_predecessors(0), num_successors(0), atmi_task(0)
    {
        and_successors.clear();
        and_predecessors.clear();
        memset(&lparm, 0, sizeof(atmi_lparm_t));
    }

#if 1
    atl_task_s(const atl_task_s &task) {
//        cpu_kernelargs = task.cpu_kernelargs;i
//        cpu_kernelid = task.cpu_kernelid;
//        num_params = task.num_params;
//        gpu_kernargptr = task.gpu_kernargptr;
    }
#endif
} atl_task_t;

extern std::map<atmi_stream_t *, atmi_stream_table_t *> StreamTable;
//atmi_task_table_t TaskTable[SNK_MAX_TASKS];
extern std::vector<atl_task_t *> AllTasks;
extern std::queue<atl_task_t *> ReadyTaskQueue;
extern std::queue<hsa_signal_t> FreeSignalPool;
extern std::map<atmi_task_t *, atl_task_t *> PublicTaskMap;
extern hsa_signal_t StreamCommonSignalPool[ATMI_MAX_STREAMS];
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
void dispatch_ready_task_or_release_signal(atl_task_t *task);
atmi_status_t dispatch_task(atl_task_t *task);
atmi_status_t check_change_in_device_type(atl_task_t *task, atmi_stream_table_t *stream_obj, hsa_queue_t *queue, atmi_devtype_t new_task_device_type);
hsa_signal_t enqueue_barrier_async(atl_task_t *task, hsa_queue_t *queue, const int dep_task_count, atl_task_t **dep_task_list, int barrier_flag);
void enqueue_barrier(atl_task_t *task, hsa_queue_t *queue, const int dep_task_count, atl_task_t **dep_task_list, int wait_flag, int barrier_flag, atmi_devtype_t devtype);

int get_stream_id(atmi_stream_table_t *stream_obj);
hsa_queue_t *acquire_and_set_next_cpu_queue(atmi_stream_table_t *stream_obj);
hsa_queue_t *acquire_and_set_next_gpu_queue(atmi_stream_table_t *stream_obj);
atmi_status_t clear_saved_tasks(atmi_stream_table_t *stream_obj);
atmi_status_t register_task(atmi_stream_table_t *stream_obj, atl_task_t *task, atmi_devtype_t devtype, boolean profilable);
atmi_status_t register_stream(atmi_stream_table_t *stream_obj);
void set_task_state(atl_task_t *t, atmi_state_t state);
void set_task_metrics(atl_task_t *task, atmi_devtype_t devtype, boolean profilable);

void packet_store_release(uint32_t* packet, uint16_t header, uint16_t rest);
uint16_t create_header(hsa_packet_type_t type, int barrier);
hsa_signal_t* get_worker_sig(hsa_queue_t *queue);

bool try_dispatch_barrier_pkt(atl_task_t *ret);
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
