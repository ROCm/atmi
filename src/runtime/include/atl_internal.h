#ifndef __SNK_INTERNAL
#define __SNK_INTERNAL
#include "atl_rt.h"
#include "ATLQueue.h"
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
  ATLCPUQueue *queue;
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
ATLCPUQueue* get_cpu_queue(int id);
void signal_worker(hsa_queue_t *queue, int signal);
void *agent_worker(void *agent_args);
int process_packet(hsa_queue_t *queue, int id);

// ---------------------- Kernel Start -------------
typedef struct atl_kernel_impl_s {
    // FIXME: would anyone need to reverse engineer the 
    // user-specified ID from the impls index? 
    unsigned int kernel_id;
    std::string kernel_name;
    atmi_devtype_t devtype;

    /* CPU kernel info */
    atmi_generic_fp function;

    /* GPU kernel info */
    uint64_t kernel_object;
    uint32_t group_segment_size;
    uint32_t private_segment_size;
    uint32_t kernarg_segment_size; // differs for CPU vs GPU
    
    /* Kernel argument map */
    pthread_mutex_t mutex; // to lock changes to the free pool
    void *kernarg_region;
    std::queue<int> free_kernarg_segments;
} atl_kernel_impl_t;

typedef struct atl_kernel_s {
    std::string pif_name; // FIXME: change this to ID later
    int num_args;
    std::vector<size_t> arg_sizes;
    std::vector<atl_kernel_impl_t *> impls;
    std::map<unsigned int, unsigned int> id_map;
} atl_kernel_t;

typedef struct atl_kernel_info_s {
    uint64_t kernel_object;
    uint32_t group_segment_size;
    uint32_t private_segment_size;
    uint32_t kernel_segment_size;
} atl_kernel_info_t;

extern std::map<std::string, atl_kernel_t *> KernelImplMap;

// ---------------------- Kernel End -------------

typedef struct atl_task_s atl_task_t;
typedef struct atl_task_list_s {
    atl_task_t *task;
    atmi_devtype_t devtype;
    boolean profilable;
    struct atl_task_list_s *next;
} atl_task_list_t;

typedef struct atmi_task_group_table_s {
    boolean ordered;
    atl_task_list_t *tasks;
    hsa_queue_t *gpu_queue;
    hsa_queue_t *cpu_queue;
    atmi_devtype_t last_device_type;
    atmi_place_t place;
    int next_gpu_qid;
    int next_cpu_qid;
    hsa_signal_t common_signal;
    pthread_mutex_t mutex;
    std::vector<atl_task_t *> running_groupable_tasks;
}atmi_task_group_table_t;

typedef enum atl_dep_sync_s {
    ATL_SYNC_BARRIER_PKT    = 0,
    ATL_SYNC_CALLBACK       = 1
} atl_dep_sync_t;

extern struct timespec context_init_time;
extern pthread_mutex_t mutex_all_tasks_;
extern pthread_mutex_t mutex_readyq_;
typedef std::vector<atl_task_t *> atl_task_vector_t;

typedef struct atl_task_s {
    // reference to HSA signal and the applications task structure
    hsa_signal_t signal;
    atmi_task_t *atmi_task;

    // all encmopassing task packet
    // hsa_kernel_dispatch_packet_t *packet;
    atl_kernel_t *kernel;
    uint32_t kernel_id;
    std::vector<void *> kernarg_region_ptrs;
    void *kernarg_region; // malloced or acquired from a pool
    size_t kernarg_region_size;
    int kernarg_region_index;
    bool kernarg_region_copied;

    // list of dependents
    uint32_t num_predecessors; // cant we get this from lparm?
    uint32_t num_successors; // cant we get this from lparm?
    atl_dep_sync_t dep_sync_type;

    atmi_task_group_table_t *stream_obj;
    boolean groupable;

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
    atmi_task_handle_t id;
    // flag to differentiate between a regular task and a continuation
    // FIXME: probably make this a class hierarchy?
    boolean is_continuation;
    atl_task_t *continuation_task;

    atmi_place_t place;

    atl_task_s() : num_predecessors(0), num_successors(0), atmi_task(0)
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

extern std::map<int, atmi_task_group_table_t *> StreamTable;
//atmi_task_table_t TaskTable[SNK_MAX_TASKS];
extern std::vector<atl_task_t *> AllTasks;
extern std::queue<atl_task_t *> ReadyTaskQueue;
extern std::queue<hsa_signal_t> FreeSignalPool;
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
atmi_status_t check_change_in_device_type(atl_task_t *task, atmi_task_group_table_t *stream_obj, hsa_queue_t *queue, atmi_devtype_t new_task_device_type);
hsa_signal_t enqueue_barrier_async(atl_task_t *task, hsa_queue_t *queue, const int dep_task_count, atl_task_t **dep_task_list, int barrier_flag);
void enqueue_barrier(atl_task_t *task, hsa_queue_t *queue, const int dep_task_count, atl_task_t **dep_task_list, int wait_flag, int barrier_flag, atmi_devtype_t devtype);

atl_kernel_impl_t *get_kernel_impl(atl_kernel_t *kernel, unsigned int kernel_id);
int get_kernel_index(atl_kernel_t *kernel, unsigned int kernel_id);
int get_stream_id(atmi_task_group_table_t *stream_obj);
ATLQueue *acquire_and_set_next_cpu_queue(atmi_task_group_table_t *stream_obj, atmi_place_t place);
ATLQueue *acquire_and_set_next_gpu_queue(atmi_task_group_table_t *stream_obj, atmi_place_t place);
atmi_status_t clear_saved_tasks(atmi_task_group_table_t *stream_obj);
atmi_status_t register_task(atmi_task_group_table_t *stream_obj, atl_task_t *task);
atmi_status_t register_stream(atmi_task_group_table_t *stream_obj);
void set_task_state(atl_task_t *t, atmi_state_t state);
void set_task_metrics(atl_task_t *task);

void packet_store_release(uint32_t* packet, uint16_t header, uint16_t rest);
uint16_t create_header(hsa_packet_type_t type, int barrier);
hsa_signal_t* get_worker_sig(hsa_queue_t *queue);

bool try_dispatch_barrier_pkt(atl_task_t *ret);
atl_task_t *get_task(atmi_task_handle_t t);
bool try_dispatch_callback(atl_task_t *t, void **args);
bool try_dispatch_barrier_pkt(atl_task_t *t, void **args);
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
