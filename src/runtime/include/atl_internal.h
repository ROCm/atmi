/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/
#ifndef __SNK_INTERNAL
#define __SNK_INTERNAL
#include "atl_rt.h"
#include <pthread.h>
#include <vector>
#include <queue>
#include <deque>
#include <map>
#include <atomic>
#include <RealTimerClass.h>
#define MAX_PIPE_SIZE   (1024)
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
//#define ATMI_WAIT_STATE HSA_WAIT_STATE_ACTIVE

typedef struct atl_kernel_enqueue_args_s {
    char        num_gpu_queues;
    void*       gpu_queue_ptr;
    char        num_cpu_queues;
    void*       cpu_worker_signals;
    void*       cpu_queue_ptr;
    int         kernel_counter;
    void*       kernarg_template_ptr;
    // ___________________________________________________________________________
    // | num_kernels | GPU AQL k0 | CPU AQL k0 | kernarg | GPU AQL k1 | CPU AQL k1 | ... |
    // ___________________________________________________________________________
} atl_kernel_enqueue_args_t;

typedef struct agent_t
{
  int id;
  hsa_signal_t worker_sig;
  hsa_queue_t *queue;
  pthread_t thread;
  Global::RealTimer timer;
} agent_t;

enum {
    PROCESS_PKT = 0,
    FINISH,
    IDLE
};

agent_t *get_cpu_q_agent(int cpu_id, int id);
void cpu_agent_init(int cpu_id, const size_t num_queues);
void agent_fini();
void signal_worker_id(int cpu_id, int tid, int signal);
hsa_queue_t* get_cpu_queue(int cpu_id, int id);
void signal_worker(hsa_queue_t *queue, int signal);
void *agent_worker(void *agent_args);
int process_packet(hsa_queue_t *queue, int id);

// ---------------------- Kernel Start -------------
typedef struct atl_kernel_impl_s {
    // FIXME: would anyone need to reverse engineer the 
    // user-specified ID from the impls index? 
    unsigned int kernel_id;
    std::string kernel_name;
    atmi_platform_type_t kernel_type;
    atmi_devtype_t devtype;

    /* CPU kernel info */
    atmi_generic_fp function;

    /* GPU kernel info */
    uint64_t *kernel_objects;
    uint32_t *group_segment_sizes;
    uint32_t *private_segment_sizes;
    uint32_t kernarg_segment_size; // differs for CPU vs GPU
    
    /* Kernel argument map */
    pthread_mutex_t mutex; // to lock changes to the free pool
    void *kernarg_region;
    std::queue<int> free_kernarg_segments;
} atl_kernel_impl_t;

typedef struct atl_kernel_s {
    uint64_t pif_id;
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

typedef struct atl_symbol_info_s {
    uint64_t addr;
    uint32_t size;
} atl_symbol_info_t;

extern std::vector<std::map<std::string, atl_kernel_info_t> > KernelInfoTable;
extern std::vector<std::map<std::string, atl_symbol_info_t> > SymbolInfoTable;

typedef void* atl_kernel_metadata_t;

extern std::vector<atl_kernel_metadata_t> AllMetadata;

extern std::map<uint64_t, atl_kernel_t *> KernelImplMap;

// ---------------------- Kernel End -------------

typedef struct atl_task_s atl_task_t;
typedef std::vector<atl_task_t *> atl_task_vector_t;

typedef struct atl_task_list_s {
    atl_task_t *task;
    atmi_devtype_t devtype;
    boolean profilable;
    struct atl_task_list_s *next;
} atl_task_list_t;

typedef struct atmi_task_group_table_s {
    boolean ordered;
    atl_task_t *last_task;
    hsa_queue_t *gpu_queue;
    hsa_queue_t *cpu_queue;
    atmi_devtype_t last_device_type;
    atmi_place_t place;
//    int next_gpu_qid;
//    int next_cpu_qid;
    // dependent tasks for the entire task group
    atl_task_vector_t and_successors;
    unsigned int group_queue_id;
    hsa_signal_t group_signal;
    std::atomic<unsigned int> task_count;
    pthread_mutex_t group_mutex;
    std::deque<atl_task_t *> running_ordered_tasks;
    std::vector<atl_task_t *> running_groupable_tasks;
    // TODO: for now, all waiting tasks (groupable and individual) are placed in a
    // single queue. does it make sense to have groupable waiting tasks separately
    // waiting in their own queue? perhaps not for now. should revisit if there
    // are more than one callback threads
    // std::vector<atl_task_t *> waiting_groupable_tasks;
    std::atomic_flag callback_started;
}atmi_task_group_table_t;

typedef enum atl_task_type_s {
    ATL_KERNEL_EXECUTION    = 0,
    ATL_DATA_MOVEMENT       = 1
} atl_task_type_t;

typedef enum atl_dep_sync_s {
    ATL_SYNC_BARRIER_PKT    = 0,
    ATL_SYNC_CALLBACK       = 1
} atl_dep_sync_t;

extern struct timespec context_init_time;
extern pthread_mutex_t mutex_all_tasks_;
extern pthread_mutex_t mutex_readyq_;
extern atmi_task_group_t atl_default_stream_obj;

typedef struct atl_task_s {
    // reference to HSA signal and the applications task structure
    hsa_signal_t signal;
    atmi_task_t *atmi_task;

    // all encmopassing task packet
    // hsa_kernel_dispatch_packet_t *packet;
    atl_kernel_t *kernel;
    uint32_t kernel_id;
    void *kernarg_region; // malloced or acquired from a pool
    size_t kernarg_region_size;
    int kernarg_region_index;
    bool kernarg_region_copied;

    // list of dependents
    uint32_t num_predecessors; 
    uint32_t num_successors; 
    atl_task_type_t type; 

    void *data_src_ptr;
    void *data_dest_ptr;
    size_t data_size;
    
    atmi_task_group_table_t *stream_obj;
    atmi_task_group_t group;
    boolean groupable;
    boolean synchronous;

    // for ordered task groups
    atl_task_t * prev_ordered_task;

    // other miscellaneous flags
    atmi_devtype_t devtype;
    boolean profilable;
    std::atomic<atmi_state_t> state;

    // per task mutex to reduce contention
    pthread_mutex_t mutex;

    unsigned long    gridDim[3];     /* # of global threads for each dimension */
    unsigned long    groupDim[3];    /* Thread group size for each dimension   */
    // FIXME: queue or vector?
    atl_task_vector_t and_successors;
    atl_task_vector_t and_predecessors;
    atl_task_vector_t predecessors;
    std::vector<hsa_signal_t> barrier_signals;
    std::vector<atmi_task_group_table_t *> pred_stream_objs;
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
extern atl_dep_sync_t g_dep_sync_type;

extern void register_allocation(void *addr, size_t size, atmi_mem_place_t place);
extern hsa_agent_t get_compute_agent(atmi_place_t place);
extern hsa_amd_memory_pool_t get_memory_pool_by_mem_place(atmi_mem_place_t place);
extern bool atl_is_atmi_initialized();

extern void atl_task_wait(atl_task_t *task);
extern void atl_stream_sync(atmi_task_group_table_t *stream_obj);

void init_dag_scheduler();
bool handle_signal(hsa_signal_value_t value, void *arg);
bool handle_group_signal(hsa_signal_value_t value, void *arg);

extern task_process_init_buffer_t task_process_init_buffer;
extern task_process_fini_buffer_t task_process_fini_buffer;

void do_progress(atmi_task_group_table_t *task_group, int progress_count = 0);
void dispatch_ready_task_for_free_signal();
void dispatch_ready_task_or_release_signal(atl_task_t *task);
atmi_status_t dispatch_task(atl_task_t *task);
atmi_status_t dispatch_data_movement(atl_task_t *task, void *dest, const void *src, const size_t size);
atmi_status_t check_change_in_device_type(atl_task_t *task, atmi_task_group_table_t *stream_obj, hsa_queue_t *queue, atmi_devtype_t new_task_device_type);
hsa_signal_t enqueue_barrier_async(atl_task_t *task, hsa_queue_t *queue, const int dep_task_count, atl_task_t **dep_task_list, int barrier_flag);
void enqueue_barrier(atl_task_t *task, hsa_queue_t *queue, const int dep_task_count, atl_task_t **dep_task_list, int wait_flag, int barrier_flag, atmi_devtype_t devtype);

atl_kernel_impl_t *get_kernel_impl(atl_kernel_t *kernel, unsigned int kernel_id);
int get_kernel_index(atl_kernel_t *kernel, unsigned int kernel_id);
int get_stream_id(atmi_task_group_table_t *stream_obj);
hsa_queue_t *acquire_and_set_next_cpu_queue(atmi_task_group_table_t *stream_obj, atmi_place_t place);
hsa_queue_t *acquire_and_set_next_gpu_queue(atmi_task_group_table_t *stream_obj, atmi_place_t place);
atmi_status_t clear_saved_tasks(atmi_task_group_table_t *stream_obj);
atmi_status_t register_task(atmi_task_group_table_t *stream_obj, atl_task_t *task);
atmi_status_t register_stream(atmi_task_group_t *stream_obj);
void set_task_state(atl_task_t *t, atmi_state_t state);
void set_task_metrics(atl_task_t *task);

void packet_store_release(uint32_t* packet, uint16_t header, uint16_t rest);
uint16_t create_header(hsa_packet_type_t type, int barrier);
hsa_signal_t* get_worker_sig(hsa_queue_t *queue);

bool try_dispatch_barrier_pkt(atl_task_t *ret);
atl_task_t *get_task(atmi_task_handle_t t);
bool try_dispatch_callback(atl_task_t *t, void **args);
bool try_dispatch_barrier_pkt(atl_task_t *t, void **args);
void set_task_handle_ID(atmi_task_handle_t *t, int ID);
void lock(pthread_mutex_t *m);
void unlock(pthread_mutex_t *m);

bool try_dispatch(atl_task_t *ret, void **args, boolean synchronous);
atl_task_t *get_new_task();
const char *get_error_string(hsa_status_t err);
const char *get_atmi_error_string(atmi_status_t err);
#define ATMIErrorCheck(msg, status) \
if (status != ATMI_STATUS_SUCCESS) { \
    printf("[%s:%d] %s failed: %s\n", __FILE__, __LINE__, #msg, get_atmi_error_string(status)); \
    exit(1); \
} else { \
 /*  printf("%s succeeded.\n", #msg);*/ \
}

#define ErrorCheck(msg, status) \
if (status != HSA_STATUS_SUCCESS) { \
    printf("[%s:%d] %s failed: %s\n", __FILE__, __LINE__, #msg, get_error_string(status)); \
    exit(1); \
} else { \
 /*  printf("%s succeeded.\n", #msg);*/ \
}

#define ErrorCheckAndContinue(msg, status) \
if (status != HSA_STATUS_SUCCESS) { \
    DEBUG_PRINT("[%s:%d] %s failed: %s\n", __FILE__, __LINE__, #msg, get_error_string(status)); \
    continue; \
} else { \
 /*  printf("%s succeeded.\n", #msg);*/ \
}

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
