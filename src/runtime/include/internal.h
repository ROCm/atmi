/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/
#ifndef SRC_RUNTIME_INCLUDE_INTERNAL_H_
#define SRC_RUNTIME_INCLUDE_INTERNAL_H_
#include <inttypes.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <atomic>
#include <cstring>
#include <deque>
#include <map>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include "hsa.h"
#include "hsa_ext_amd.h"
#include "hsa_ext_finalize.h"

#include "atmi.h"
#include "atmi_kl.h"
#include "atmi_runtime.h"
#include "realtimer.h"

#ifdef __cplusplus
extern "C" {
#define _CPPSTRING_ "C"
#endif
#ifndef __cplusplus
#define _CPPSTRING_
#endif

// #define ATMI_MAX_TASKGROUPS            8
// #define ATMI_MAX_TASKS_PER_TASKGROUP   125

#define SNK_MAX_FUNCTIONS 100

// #define SNK_MAX_TASKS 32 //100000 //((ATMI_MAX_TASKGROUPS) *
// (ATMI_MAX_TASKS_PER_TASKGROUP))

#define SNK_WAIT 1
#define SNK_NOWAIT 0

#define SNK_OR 1
#define SNK_AND 0

#define check(msg, status)            \
  if (status != HSA_STATUS_SUCCESS) { \
    printf("%s failed.\n", #msg);     \
    exit(1);                          \
  }

#ifdef DEBUG
#define DEBUG_SNK
#define VERBOSE_SNK
#endif

#ifdef DEBUG_SNK
#define DEBUG_PRINT(fmt, ...)                                           \
  if (core::Runtime::getInstance().getDebugMode()) {                    \
    fprintf(stderr, "[%s:%d] " fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
  }
#else
#define DEBUG_PRINT(...) \
  do {                   \
  } while (false)
#endif

#ifdef VERBOSE_SNK
#define VERBOSE_PRINT(fmt, ...)                                         \
  if (core::Runtime::getInstance().getDebugMode()) {                    \
    fprintf(stderr, "[%s:%d] " fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
  }
#else
#define VERBOSE_PRINT(...) \
  do {                     \
  } while (false)
#endif

#ifndef HSA_RUNTIME_INC_HSA_H_
typedef struct hsa_signal_s { uint64_t handle; } hsa_signal_t;
#endif

/*  All global values go in this global structure */
typedef struct atl_context_s {
  bool struct_initialized;
  bool g_cpu_initialized;
  bool g_hsa_initialized;
  bool g_gpu_initialized;
  bool g_tasks_initialized;
  bool g_mutex_dag_initialized;
} atl_context_t;
extern atl_context_t atlc;
extern atl_context_t *atlc_p;

#ifdef __cplusplus
}
#endif

/* ---------------------------------------------------------------------------------
 * Simulated CPU Data Structures and API
 * ---------------------------------------------------------------------------------
 */
typedef void *ARG_TYPE;
#define COMMA ,
#define REPEAT(name) COMMA name
#define REPEAT2(name) REPEAT(name) REPEAT(name)
#define REPEAT4(name) REPEAT2(name) REPEAT2(name)
#define REPEAT8(name) REPEAT4(name) REPEAT4(name)
#define REPEAT16(name) REPEAT8(name) REPEAT8(name)

#define ATMI_WAIT_STATE HSA_WAIT_STATE_BLOCKED
// #define ATMI_WAIT_STATE HSA_WAIT_STATE_ACTIVE

typedef struct atl_kernel_enqueue_args_s {
  char num_gpu_queues;
  void *gpu_queue_ptr;
  char num_cpu_queues;
  void *cpu_worker_signals;
  void *cpu_queue_ptr;
  int kernel_counter;
  void *kernarg_template_ptr;
  // ___________________________________________________________________________
  // | num_kernels | GPU AQL k0 | CPU AQL k0 | kernarg | GPU AQL k1 | CPU AQL k1
  // | ... |
  // ___________________________________________________________________________
} atl_kernel_enqueue_args_t;

typedef struct thread_agent_s {
  int id;
  hsa_signal_t worker_sig;
  hsa_queue_t *queue;
  pthread_t thread;
  core::RealTimer timer;
} thread_agent_t;

enum { PROCESS_PKT = 0, FINISH, IDLE };

thread_agent_t *get_cpu_q_agent(int cpu_id, int id);
void cpu_agent_init(int cpu_id, const size_t num_queues);
void agent_fini();
void signal_worker_id(int cpu_id, int tid, int signal);
hsa_queue_t *get_cpu_queue(int cpu_id, int id);
void signal_worker(hsa_queue_t *queue, int signal);
void *agent_worker(void *agent_args);
int process_packet(hsa_queue_t *queue, int id);

// ---------------------- Kernel Start -------------
typedef struct atl_kernel_info_s {
  uint64_t kernel_object;
  uint32_t group_segment_size;
  uint32_t private_segment_size;
  uint32_t kernel_segment_size;
  uint32_t num_args;
  std::vector<uint64_t> arg_alignments;
  std::vector<uint64_t> arg_offsets;
  std::vector<uint64_t> arg_sizes;
} atl_kernel_info_t;

typedef struct atl_symbol_info_s {
  uint64_t addr;
  uint32_t size;
} atl_symbol_info_t;

extern std::vector<std::map<std::string, atl_kernel_info_t> > KernelInfoTable;
extern std::vector<std::map<std::string, atl_symbol_info_t> > SymbolInfoTable;

// ---------------------- Kernel End -------------

typedef enum atl_task_type_s {
  ATL_KERNEL_EXECUTION = 0,
  ATL_DATA_MOVEMENT = 1
} atl_task_type_t;

typedef enum atl_dep_sync_s {
  ATL_SYNC_BARRIER_PKT = 0,
  ATL_SYNC_CALLBACK = 1
} atl_dep_sync_t;

extern struct timespec context_init_time;
extern pthread_mutex_t mutex_all_tasks_;
extern pthread_mutex_t mutex_readyq_;
namespace core {
class TaskgroupImpl;
class TaskImpl;
class Kernel;
class KernelImpl;
}  // namespace core

extern std::vector<core::TaskgroupImpl *> AllTaskgroups;
// atmi_task_table_t TaskTable[SNK_MAX_TASKS];
extern std::vector<core::TaskImpl *> AllTasks;
extern std::queue<core::TaskImpl *> ReadyTaskQueue;
extern std::queue<hsa_signal_t> FreeSignalPool;
extern std::vector<hsa_amd_memory_pool_t> atl_gpu_kernarg_pools;

namespace core {
atmi_status_t atl_init_context();
atmi_status_t atl_init_cpu_context();
atmi_status_t atl_init_gpu_context();

hsa_status_t init_hsa();
hsa_status_t finalize_hsa();
/*
 * Generic utils
 */
template <typename T>
inline T alignDown(T value, size_t alignment) {
  return (T)(value & ~(alignment - 1));
}

template <typename T>
inline T *alignDown(T *value, size_t alignment) {
  return reinterpret_cast<T *>(alignDown((intptr_t)value, alignment));
}

template <typename T>
inline T alignUp(T value, size_t alignment) {
  return alignDown((T)(value + alignment - 1), alignment);
}

template <typename T>
inline T *alignUp(T *value, size_t alignment) {
  return reinterpret_cast<T *>(
      alignDown((intptr_t)(value + alignment - 1), alignment));
}

template <typename T>
void clear_container(T *q) {
  T empty;
  std::swap(*q, empty);
}

long int get_nanosecs(struct timespec start_time, struct timespec end_time);

extern void register_allocation(void *addr, size_t size,
                                atmi_mem_place_t place);
extern hsa_agent_t get_compute_agent(atmi_place_t place);
extern hsa_amd_memory_pool_t get_memory_pool_by_mem_place(
    atmi_mem_place_t place);
extern bool atl_is_atmi_initialized();

// extern void atl_task_wait(TaskImpl *task);

void init_dag_scheduler();
bool handle_signal(hsa_signal_value_t value, void *arg);
bool handle_group_signal(hsa_signal_value_t value, void *arg);

void enqueue_barrier_tasks(std::vector<TaskImpl *> tasks);
hsa_signal_t enqueue_barrier_async(TaskImpl *task, hsa_queue_t *queue,
                                   const int dep_task_count,
                                   TaskImpl **dep_task_list, int barrier_flag,
                                   bool need_completion);
void enqueue_barrier(TaskImpl *task, hsa_queue_t *queue,
                     const int dep_task_count, TaskImpl **dep_task_list,
                     int wait_flag, int barrier_flag, atmi_devtype_t devtype,
                     bool need_completion = false);

void packet_store_release(uint32_t *packet, uint16_t header, uint16_t rest);
uint16_t create_header(
    hsa_packet_type_t type, int barrier,
    atmi_task_fence_scope_t acq_fence = ATMI_FENCE_SCOPE_SYSTEM,
    atmi_task_fence_scope_t rel_fence = ATMI_FENCE_SCOPE_SYSTEM);

Kernel *get_kernel_obj(atmi_kernel_t atmi_kernel);
TaskImpl *getTaskImpl(atmi_task_handle_t t);
core::TaskgroupImpl *getTaskgroupImpl(atmi_taskgroup_handle_t t);
void set_task_handle_ID(atmi_task_handle_t *t, int ID);
void lock(pthread_mutex_t *m);
void unlock(pthread_mutex_t *m);

TaskImpl *get_new_task();
void allow_access_to_all_gpu_agents(void *ptr);
}  // namespace core
hsa_signal_t *get_worker_sig(hsa_queue_t *queue);

const char *get_error_string(hsa_status_t err);
const char *get_atmi_error_string(atmi_status_t err);
int cpu_bindthread(int cpu_index);
atmi_status_t set_thread_affinity(int id);
#define ATMIErrorCheck(msg, status)                             \
  if (status != ATMI_STATUS_SUCCESS) {                          \
    printf("[%s:%d] %s failed: %s\n", __FILE__, __LINE__, #msg, \
           get_atmi_error_string(status));                      \
    exit(1);                                                    \
  } else {                                                      \
    /*  printf("%s succeeded.\n", #msg);*/                      \
  }

#define ErrorCheck(msg, status)                                 \
  if (status != HSA_STATUS_SUCCESS) {                           \
    printf("[%s:%d] %s failed: %s\n", __FILE__, __LINE__, #msg, \
           get_error_string(status));                           \
    exit(1);                                                    \
  } else {                                                      \
    /*  printf("%s succeeded.\n", #msg);*/                      \
  }

#define comgrErrorCheck(msg, status)                         \
  if (status != AMD_COMGR_STATUS_SUCCESS) {                  \
    printf("[%s:%d] %s failed\n", __FILE__, __LINE__, #msg); \
    return HSA_STATUS_ERROR_INVALID_CODE_OBJECT;             \
  } else {                                                   \
    /*  printf("%s succeeded.\n", #msg);*/                   \
  }

#define ELFErrorReturn(msg, status)                             \
  {                                                             \
    printf("[%s:%d] %s failed: %s\n", __FILE__, __LINE__, #msg, \
           get_error_string(status));                           \
    return status;                                              \
  }

#define ErrorCheckAndContinue(msg, status)                           \
  if (status != HSA_STATUS_SUCCESS) {                                \
    DEBUG_PRINT("[%s:%d] %s failed: %s\n", __FILE__, __LINE__, #msg, \
                get_error_string(status));                           \
    continue;                                                        \
  } else {                                                           \
    /*  printf("%s succeeded.\n", #msg);*/                           \
  }

#endif  // SRC_RUNTIME_INCLUDE_INTERNAL_H_
