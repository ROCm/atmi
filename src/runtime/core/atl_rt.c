/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/

#include "atl_internal.h"
#include "atl_profile.h"
#include "atl_bindthread.h"
#include "ATLQueue.h"
#include "ATLMachine.h"
#include "ATLData.h"
#include <pthread.h>
#include <time.h>
#include <assert.h>
#include <stdarg.h>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <vector>
#include <atomic>
#include <unistd.h>
#include <errno.h>
#include <algorithm>
#include <sys/syscall.h>
#include <malloc.h>
#include "RealTimerClass.h"
using namespace Global;

pthread_mutex_t mutex_all_tasks_;
pthread_mutex_t mutex_readyq_;
RealTimer SignalAddTimer("Signal Time");
RealTimer HandleSignalTimer("Handle Signal Time");;
RealTimer HandleSignalInvokeTimer("Handle Signal Invoke Time");
RealTimer TaskWaitTimer("Task Wait Time");
RealTimer TryLaunchTimer("Launch Time");
RealTimer ParamsInitTimer("Params Init Time");
RealTimer TryLaunchInitTimer("Launch Init Time");
RealTimer ShouldDispatchTimer("Dispatch Eval Time");
RealTimer RegisterCallbackTimer("Register Callback Time");
RealTimer LockTimer("Lock/Unlock Time");
static RealTimer TryDispatchTimer("Dispatch Time");
static size_t max_ready_queue_sz = 0;
static size_t waiting_count = 0;
static size_t direct_dispatch = 0;
static size_t callback_dispatch = 0;
#define NANOSECS 1000000000L

//  set NOTCOHERENT needs this include
#include "hsa_ext_amd.h"

/* -------------- Helper functions -------------------------- */
const char *get_atmi_error_string(atmi_status_t err) {
    switch(err) {
        case ATMI_STATUS_SUCCESS: return "ATMI_STATUS_SUCCESS";
        case ATMI_STATUS_ERROR: return "ATMI_STATUS_ERROR";
        default: return "";
    }
}

const char *get_error_string(hsa_status_t err) {
    switch(err) {
        case HSA_STATUS_SUCCESS: return "HSA_STATUS_SUCCESS";
        case HSA_STATUS_INFO_BREAK: return "HSA_STATUS_INFO_BREAK";
        case HSA_STATUS_ERROR: return "HSA_STATUS_ERROR";
        case HSA_STATUS_ERROR_INVALID_ARGUMENT: return "HSA_STATUS_ERROR_INVALID_ARGUMENT";
        case HSA_STATUS_ERROR_INVALID_QUEUE_CREATION: return "HSA_STATUS_ERROR_INVALID_QUEUE_CREATION";
        case HSA_STATUS_ERROR_INVALID_ALLOCATION: return "HSA_STATUS_ERROR_INVALID_ALLOCATION";
        case HSA_STATUS_ERROR_INVALID_AGENT: return "HSA_STATUS_ERROR_INVALID_AGENT";
        case HSA_STATUS_ERROR_INVALID_REGION: return "HSA_STATUS_ERROR_INVALID_REGION";
        case HSA_STATUS_ERROR_INVALID_SIGNAL: return "HSA_STATUS_ERROR_INVALID_SIGNAL";
        case HSA_STATUS_ERROR_INVALID_QUEUE: return "HSA_STATUS_ERROR_INVALID_QUEUE";
        case HSA_STATUS_ERROR_OUT_OF_RESOURCES: return "HSA_STATUS_ERROR_OUT_OF_RESOURCES";
        case HSA_STATUS_ERROR_INVALID_PACKET_FORMAT: return "HSA_STATUS_ERROR_INVALID_PACKET_FORMAT";
        case HSA_STATUS_ERROR_RESOURCE_FREE: return "HSA_STATUS_ERROR_RESOURCE_FREE";
        case HSA_STATUS_ERROR_NOT_INITIALIZED: return "HSA_STATUS_ERROR_NOT_INITIALIZED";
        case HSA_STATUS_ERROR_REFCOUNT_OVERFLOW: return "HSA_STATUS_ERROR_REFCOUNT_OVERFLOW";
        case HSA_STATUS_ERROR_INCOMPATIBLE_ARGUMENTS: return "HSA_STATUS_ERROR_INCOMPATIBLE_ARGUMENTS";
        case HSA_STATUS_ERROR_INVALID_INDEX: return "HSA_STATUS_ERROR_INVALID_INDEX";
        case HSA_STATUS_ERROR_INVALID_ISA: return "HSA_STATUS_ERROR_INVALID_ISA";
        case HSA_STATUS_ERROR_INVALID_ISA_NAME: return "HSA_STATUS_ERROR_INVALID_ISA_NAME";
        case HSA_STATUS_ERROR_INVALID_CODE_OBJECT: return "HSA_STATUS_ERROR_INVALID_CODE_OBJECT";
        case HSA_STATUS_ERROR_INVALID_EXECUTABLE: return "HSA_STATUS_ERROR_INVALID_EXECUTABLE";
        case HSA_STATUS_ERROR_FROZEN_EXECUTABLE: return "HSA_STATUS_ERROR_FROZEN_EXECUTABLE";
        case HSA_STATUS_ERROR_INVALID_SYMBOL_NAME: return "HSA_STATUS_ERROR_INVALID_SYMBOL_NAME";
        case HSA_STATUS_ERROR_VARIABLE_ALREADY_DEFINED: return "HSA_STATUS_ERROR_VARIABLE_ALREADY_DEFINED";
        case HSA_STATUS_ERROR_VARIABLE_UNDEFINED: return "HSA_STATUS_ERROR_VARIABLE_UNDEFINED";
        case HSA_STATUS_ERROR_EXCEPTION: return "HSA_STATUS_ERROR_EXCEPTION";
    }
}

bool g_atmi_initialized = false;
extern bool handle_signal(hsa_signal_value_t value, void *arg);

void print_atl_kernel(const char * str, const int i);
/* Stream table to hold the runtime state of the
 * stream and its tasks. Which was the latest
 * device used, latest queue used and also a
 * pool of tasks for synchronization if need be */
std::map<int, atmi_task_group_table_t *> StreamTable;

std::vector<atl_task_t *> AllTasks;
std::vector<atl_kernel_metadata_t> AllMetadata;
std::queue<atl_task_t *> ReadyTaskQueue;
std::queue<hsa_signal_t> FreeSignalPool;

std::vector<std::map<std::string, atl_kernel_info_t> > KernelInfoTable;
std::vector<std::map<std::string, atl_symbol_info_t> > SymbolInfoTable;
std::set<std::string> SymbolSet;

std::map<uint64_t, atl_kernel_t *> KernelImplMap;
//std::map<uint64_t, std::vector<std::string> > ModuleMap;
hsa_signal_t StreamCommonSignalPool[ATMI_MAX_STREAMS];
hsa_signal_t IdentityORSignal;
hsa_signal_t IdentityANDSignal;
hsa_signal_t IdentityCopySignal;
static std::atomic<unsigned int> StreamCommonSignalIdx(0);
std::queue<atl_task_t *> DispatchedTasks;

atmi_machine_t g_atmi_machine;
atl_kernel_enqueue_args_t g_ke_args;
ATLMachine g_atl_machine;
static std::vector<hsa_executable_t> g_executables;

atl_dep_sync_t g_dep_sync_type;
static int g_max_signals;
/* Stream specific globals */
hsa_agent_t atl_cpu_agent;
hsa_ext_program_t atl_hsa_program;
hsa_region_t atl_hsa_primary_region;
hsa_region_t atl_gpu_kernarg_region;
hsa_amd_memory_pool_t atl_gpu_kernarg_pool;
hsa_region_t atl_cpu_kernarg_region;
hsa_agent_t atl_gpu_agent;
hsa_profile_t atl_gpu_agent_profile;

atmi_task_group_t atl_default_stream_obj = {0, ATMI_FALSE};

struct timespec context_init_time;
static int context_init_time_init = 0;

atmi_task_handle_t ATMI_NULL_TASK_HANDLE = ATMI_TASK_HANDLE(0xFFFFFFFFFFFFFFFF);

long int get_nanosecs( struct timespec start_time, struct timespec end_time) {
    long int nanosecs;
    if ((end_time.tv_nsec-start_time.tv_nsec)<0) nanosecs =
        ((((long int) end_time.tv_sec- (long int) start_time.tv_sec )-1)*NANOSECS ) +
            ( NANOSECS + (long int) end_time.tv_nsec - (long int) start_time.tv_nsec) ;
    else nanosecs =
        (((long int) end_time.tv_sec- (long int) start_time.tv_sec )*NANOSECS ) +
            ( (long int) end_time.tv_nsec - (long int) start_time.tv_nsec );
    return nanosecs;
}

void packet_store_release(uint32_t* packet, uint16_t header, uint16_t rest){
  __atomic_store_n(packet,header|(rest<<16),__ATOMIC_RELEASE);
}

uint16_t create_header(hsa_packet_type_t type, int barrier) {
   uint16_t header = type << HSA_PACKET_HEADER_TYPE;
   header |= barrier << HSA_PACKET_HEADER_BARRIER;
   header |= HSA_FENCE_SCOPE_SYSTEM << HSA_PACKET_HEADER_ACQUIRE_FENCE_SCOPE;
   header |= HSA_FENCE_SCOPE_SYSTEM << HSA_PACKET_HEADER_RELEASE_FENCE_SCOPE;
   //__atomic_store_n((uint8_t*)(&header), (uint8_t)type, __ATOMIC_RELEASE);
   return header;
}

int get_task_handle_ID(atmi_task_handle_t t) {
    #if 1
    return (int)(0xFFFFFFFF & t);
    #else
    return t.lo;
    #endif
}

void set_task_handle_ID(atmi_task_handle_t *t, int ID) {
    #if 1
    unsigned long int task_handle = *t;
    task_handle |= 0xFFFFFFFFFFFFFFFF;
    task_handle &= ID;
    *t = task_handle;
    #else
    t->lo = ID;
    #endif
}

atmi_task_group_t get_task_group(atmi_task_handle_t t) {
    atl_task_t *task = get_task(t);
    atmi_task_group_t ret;
    if(task)
        ret = task->group;
    return ret;
}

atl_task_t *get_task(atmi_task_handle_t t) {
    /* FIXME: node 0 only for now */
    atl_task_t *ret = NULL;
    if(t != ATMI_NULL_TASK_HANDLE) {
        lock(&mutex_all_tasks_);
        ret = AllTasks[get_task_handle_ID(t)];
        unlock(&mutex_all_tasks_);
    }
    return ret;
    //return AllTasks[t.lo];
}

atl_task_t *get_continuation_task(atmi_task_handle_t t) {
    /* FIXME: node 0 only for now */
    //return AllTasks[t.hi];
    return NULL;
}

void allow_access_to_all_gpu_agents(void *ptr) {
    hsa_status_t err;
    std::vector<ATLGPUProcessor> &gpu_procs = g_atl_machine.getProcessors<ATLGPUProcessor>();
    std::vector<hsa_agent_t> agents;
    for(int i = 0; i < gpu_procs.size(); i++) {
        agents.push_back(gpu_procs[i].getAgent());
    }
    err = hsa_amd_agents_allow_access(agents.size(), &agents[0], NULL, ptr);
    ErrorCheck(Allow agents ptr access, err);
}

hsa_signal_t enqueue_barrier_async(atl_task_t *task, hsa_queue_t *queue, const int dep_task_count, atl_task_t **dep_task_list, int barrier_flag) {
    /* This routine will enqueue a barrier packet for all dependent packets to complete
       irrespective of their stream
     */

    long t_barrier_wait = 0L;
    long t_barrier_dispatch = 0L;
    /* Keep adding barrier packets in multiples of 5 because that is the maximum signals that
       the HSA barrier packet can support today
     */
    hsa_status_t err;
    hsa_signal_t last_signal;
    if(queue == NULL || dep_task_list == NULL || dep_task_count <= 0) return last_signal;
    if(barrier_flag == SNK_OR)
        last_signal = IdentityORSignal;
    else
        last_signal = IdentityANDSignal;
    atl_task_t **tasks = dep_task_list;
    int tasks_remaining = dep_task_count;
    const int HSA_BARRIER_MAX_DEPENDENT_TASKS = 4;
    /* round up */
    int barrier_pkt_count = (dep_task_count + HSA_BARRIER_MAX_DEPENDENT_TASKS - 1) / HSA_BARRIER_MAX_DEPENDENT_TASKS;
    int barrier_pkt_id = 0;

    for(barrier_pkt_id = 0; barrier_pkt_id < barrier_pkt_count; barrier_pkt_id++) {
        hsa_signal_t signal = task->barrier_signals[barrier_pkt_id];
        //hsa_signal_create(1, 0, NULL, &signal);
        hsa_signal_store_relaxed(signal, 1);
        /* Obtain the write index for the command queue for this stream.  */
        uint64_t index = hsa_queue_load_write_index_relaxed(queue);
        const uint32_t queueMask = queue->size - 1;
        hsa_barrier_and_packet_t* barrier = &(((hsa_barrier_and_packet_t*)(queue->base_address))[index&queueMask]);
        assert(sizeof(hsa_barrier_or_packet_t) == sizeof(hsa_barrier_and_packet_t));
        if(barrier_flag == SNK_OR) {
            /* Define the barrier packet to be at the calculated queue index address.  */
            memset(barrier, 0, sizeof(hsa_barrier_or_packet_t));
            barrier->header = create_header(HSA_PACKET_TYPE_BARRIER_OR, 0);
        }
        else {
            /* Define the barrier packet to be at the calculated queue index address.  */
            memset(barrier, 0, sizeof(hsa_barrier_and_packet_t));
            barrier->header = create_header(HSA_PACKET_TYPE_BARRIER_AND, 0);
        }
        int j;
        for(j = 0; j < 5; j++) {
            barrier->dep_signal[j] = last_signal;
        }
        /* populate all dep_signals */
        int dep_signal_id = 0;
        int iter = 0;
        for(dep_signal_id = 0; dep_signal_id < HSA_BARRIER_MAX_DEPENDENT_TASKS; dep_signal_id++) {
            if(*tasks != NULL && tasks_remaining > 0) {
                DEBUG_PRINT("Barrier Packet %d\n", iter);
                iter++;
                /* fill out the barrier packet and ring doorbell */
                barrier->dep_signal[dep_signal_id] = (*tasks)->signal;
                DEBUG_PRINT("Enqueue wait for task %lu signal handle: %" PRIu64 "\n", (*tasks)->id, barrier->dep_signal[dep_signal_id].handle);
                tasks++;
                tasks_remaining--;
            }
        }
        barrier->dep_signal[4] = last_signal;
        barrier->completion_signal = signal;
        last_signal = signal;
        /* Increment write index and ring doorbell to dispatch the kernel.  */
        hsa_queue_store_write_index_relaxed(queue, index+1);
        hsa_signal_store_relaxed(queue->doorbell_signal, index);
    }
    return last_signal;
}

void enqueue_barrier(atl_task_t *task, hsa_queue_t *queue, const int dep_task_count, atl_task_t **dep_task_list, int wait_flag, int barrier_flag, atmi_devtype_t devtype) {
    hsa_signal_t last_signal = enqueue_barrier_async(task, queue, dep_task_count, dep_task_list, barrier_flag);
    if(devtype == ATMI_DEVTYPE_CPU) {
        signal_worker(queue, PROCESS_PKT);
    }
    /* Wait on completion signal if blockine */
    if(wait_flag == SNK_WAIT) {
        hsa_signal_wait_acquire(last_signal, HSA_SIGNAL_CONDITION_EQ, 0, UINT64_MAX, ATMI_WAIT_STATE);
        hsa_signal_destroy(last_signal);
    }
}

extern void atl_task_wait(atl_task_t *task) {
    TaskWaitTimer.Start();
    if(task != NULL) {
        while(task->state != ATMI_COMPLETED) { }

        /* Flag this task as completed */
        /* FIXME: How can HSA tell us if and when a task has failed? */
        set_task_state(task, ATMI_COMPLETED);
    }

    TaskWaitTimer.Stop();
    return;// ATMI_STATUS_SUCCESS;
}

extern atmi_status_t atmi_task_wait(atmi_task_handle_t task) {
    DEBUG_PRINT("Waiting for task ID: %lu\n", task);
    atl_task_wait(get_task(task));
    return ATMI_STATUS_SUCCESS;
}

atmi_status_t queue_sync(hsa_queue_t *queue) {
    if(queue == NULL) return ATMI_STATUS_SUCCESS;
    /* This function puts a barrier packet into the queue
       This routine will wait for all packets to complete on this queue.
    */
    hsa_signal_t signal;
    hsa_signal_create(1, 0, NULL, &signal);

    /* Obtain the write index for the command queue for this stream.  */
    uint64_t index = hsa_queue_load_write_index_relaxed(queue);
    const uint32_t queueMask = queue->size - 1;

    /* Define the barrier packet to be at the calculated queue index address.  */
    hsa_barrier_and_packet_t* barrier = &(((hsa_barrier_and_packet_t*)(queue->base_address))[index&queueMask]);
    memset(barrier, 0, sizeof(hsa_barrier_and_packet_t));

    barrier->header = create_header(HSA_PACKET_TYPE_BARRIER_AND, 1);

    barrier->completion_signal = signal;

    /* Increment write index and ring doorbell to dispatch the kernel.  */
    hsa_queue_store_write_index_relaxed(queue, index+1);
    hsa_signal_store_relaxed(queue->doorbell_signal, index);

    /* Wait on completion signal til kernel is finished.  */
    hsa_signal_wait_acquire(signal, HSA_SIGNAL_CONDITION_EQ, 0, UINT64_MAX, ATMI_WAIT_STATE);
    hsa_signal_destroy(signal);

    return ATMI_STATUS_SUCCESS;
}
#if 1

// Implement memory_pool iteration function
static hsa_status_t get_memory_pool_info(hsa_amd_memory_pool_t memory_pool, void* data)
{
	ATLProcessor* proc = reinterpret_cast<ATLProcessor*>(data);
    hsa_status_t err = HSA_STATUS_SUCCESS;
	// Check if the memory_pool is allowed to allocate, i.e. do not return group
    // memory
	bool alloc_allowed = false;
	err = hsa_amd_memory_pool_get_info(memory_pool, HSA_AMD_MEMORY_POOL_INFO_RUNTIME_ALLOC_ALLOWED, &alloc_allowed);
	ErrorCheck(Alloc allowed in memory pool check, err);
    if(alloc_allowed) {
        uint32_t global_flag = 0;
        err = hsa_amd_memory_pool_get_info(memory_pool, HSA_AMD_MEMORY_POOL_INFO_GLOBAL_FLAGS, &global_flag);
        ErrorCheck(Get memory pool info, err);
        if(HSA_AMD_MEMORY_POOL_GLOBAL_FLAG_FINE_GRAINED & global_flag) {
            ATLMemory new_mem(memory_pool, *proc, ATMI_MEMTYPE_FINE_GRAINED);
            proc->addMemory(new_mem);
            if(HSA_AMD_MEMORY_POOL_GLOBAL_FLAG_KERNARG_INIT & global_flag) {
                DEBUG_PRINT("GPU kernel args pool handle: %lu\n", memory_pool.handle);
                atl_gpu_kernarg_pool = memory_pool;
            }
        }
        else {
            ATLMemory new_mem(memory_pool, *proc, ATMI_MEMTYPE_COARSE_GRAINED);
            proc->addMemory(new_mem);
        }
    }

	return err;
}

static hsa_status_t get_agent_info(hsa_agent_t agent, void *data) {
    hsa_status_t err = HSA_STATUS_SUCCESS;
    hsa_device_type_t device_type;
    err = hsa_agent_get_info(agent, HSA_AGENT_INFO_DEVICE, &device_type);
    ErrorCheck(Get device type info, err);
    switch(device_type) {
        case HSA_DEVICE_TYPE_CPU:
            {
            ;
            ATLCPUProcessor new_proc(agent);
            err = hsa_amd_agent_iterate_memory_pools(agent, get_memory_pool_info, &new_proc);
            ErrorCheck(Iterate all memory pools, err);
            g_atl_machine.addProcessor(new_proc);
            }
            break;
        case HSA_DEVICE_TYPE_GPU:
            {
            ;
            hsa_profile_t profile;
            err = hsa_agent_get_info(agent, HSA_AGENT_INFO_PROFILE, &profile);
            ErrorCheck(Query the agent profile, err);
            atmi_devtype_t gpu_type;
            gpu_type = (profile == HSA_PROFILE_FULL) ? ATMI_DEVTYPE_iGPU : ATMI_DEVTYPE_dGPU;
            ATLGPUProcessor new_proc(agent, gpu_type);
            err = hsa_amd_agent_iterate_memory_pools(agent, get_memory_pool_info, &new_proc);
            ErrorCheck(Iterate all memory pools, err);
            g_atl_machine.addProcessor(new_proc);
            }
            break;
        case HSA_DEVICE_TYPE_DSP:
            {
            ;
            ATLDSPProcessor new_proc(agent);
            err = hsa_amd_agent_iterate_memory_pools(agent, get_memory_pool_info, &new_proc);
            ErrorCheck(Iterate all memory pools, err);
            g_atl_machine.addProcessor(new_proc);
            }
            break;
    }

    return err;
}
//#else
/* Determines if the given agent is of type HSA_DEVICE_TYPE_GPU
   and sets the value of data to the agent handle if it is.
*/
static hsa_status_t get_gpu_agent(hsa_agent_t agent, void *data) {
    hsa_status_t status;
    hsa_device_type_t device_type;
    status = hsa_agent_get_info(agent, HSA_AGENT_INFO_DEVICE, &device_type);
    DEBUG_PRINT("Device Type = %d\n", device_type);
    if (HSA_STATUS_SUCCESS == status && HSA_DEVICE_TYPE_GPU == device_type) {
        uint32_t max_queues;
        status = hsa_agent_get_info(agent, HSA_AGENT_INFO_QUEUES_MAX, &max_queues);
        DEBUG_PRINT("GPU has max queues = %" PRIu32 "\n", max_queues);
        hsa_agent_t* ret = (hsa_agent_t*)data;
        *ret = agent;
        return HSA_STATUS_INFO_BREAK;
    }
    return HSA_STATUS_SUCCESS;
}

/* Determines if the given agent is of type HSA_DEVICE_TYPE_CPU
   and sets the value of data to the agent handle if it is.
*/
static hsa_status_t get_cpu_agent(hsa_agent_t agent, void *data) {
    hsa_status_t status;
    hsa_device_type_t device_type;
    status = hsa_agent_get_info(agent, HSA_AGENT_INFO_DEVICE, &device_type);
    if (HSA_STATUS_SUCCESS == status && HSA_DEVICE_TYPE_CPU == device_type) {
        uint32_t max_queues;
        status = hsa_agent_get_info(agent, HSA_AGENT_INFO_QUEUES_MAX, &max_queues);
        DEBUG_PRINT("CPU has max queues = %" PRIu32 "\n", max_queues);
        hsa_agent_t* ret = (hsa_agent_t*)data;
        *ret = agent;
        return HSA_STATUS_INFO_BREAK;
    }
    return HSA_STATUS_SUCCESS;
}

hsa_status_t get_fine_grained_region(hsa_region_t region, void* data) {
    hsa_region_segment_t segment;
    hsa_region_get_info(region, HSA_REGION_INFO_SEGMENT, &segment);
    if (segment != HSA_REGION_SEGMENT_GLOBAL) {
        return HSA_STATUS_SUCCESS;
    }
    hsa_region_global_flag_t flags;
    hsa_region_get_info(region, HSA_REGION_INFO_GLOBAL_FLAGS, &flags);
    if (flags & HSA_REGION_GLOBAL_FLAG_FINE_GRAINED) {
        hsa_region_t* ret = (hsa_region_t*) data;
        *ret = region;
        return HSA_STATUS_INFO_BREAK;
    }
    return HSA_STATUS_SUCCESS;
}

/* Determines if a memory region can be used for kernarg allocations.  */
static hsa_status_t get_kernarg_memory_region(hsa_region_t region, void* data) {
    hsa_region_segment_t segment;
    hsa_region_get_info(region, HSA_REGION_INFO_SEGMENT, &segment);
    if (HSA_REGION_SEGMENT_GLOBAL != segment) {
        return HSA_STATUS_SUCCESS;
    }

    hsa_region_global_flag_t flags;
    hsa_region_get_info(region, HSA_REGION_INFO_GLOBAL_FLAGS, &flags);
    if (flags & HSA_REGION_GLOBAL_FLAG_KERNARG) {
        hsa_region_t* ret = (hsa_region_t*) data;
        *ret = region;
        return HSA_STATUS_INFO_BREAK;
    }

    return HSA_STATUS_SUCCESS;
}
#endif
void init_dag_scheduler() {
    if(atlc.g_mutex_dag_initialized == 0) {
        pthread_mutex_init(&mutex_all_tasks_, NULL);
        pthread_mutex_init(&mutex_readyq_, NULL);
        AllTasks.clear();
        AllTasks.reserve(500000);
        //PublicTaskMap.clear();
        for(int i = 0; i < SymbolInfoTable.size(); i++)
            SymbolInfoTable[i].clear();
        SymbolInfoTable.clear();
        for(int i = 0; i < KernelInfoTable.size(); i++)
            KernelInfoTable[i].clear();
        KernelInfoTable.clear();
        atlc.g_mutex_dag_initialized = 1;
        DEBUG_PRINT("main tid = %lu\n", syscall(SYS_gettid));
    }
}

#if 0
lock(int *mutex) {
    // atomic cas of mutex with 1 and return 0
}

unlock(int *mutex) {
    // atomic exch with 1
}
#endif


void set_task_state(atl_task_t *t, const atmi_state_t state) {
    t->state = state;
    //t->state.store(state, std::memory_order_seq_cst);
    if(t->atmi_task != NULL) t->atmi_task->state = state;
}

void set_task_metrics(atl_task_t *task) {
    hsa_status_t err = HSA_STATUS_SUCCESS;
    //if(task->profile != NULL) {
    if(task->profilable == ATMI_TRUE) {
        hsa_signal_t signal = task->signal;
        hsa_amd_profiling_dispatch_time_t metrics;
        if(task->devtype == ATMI_DEVTYPE_GPU) {
            err = hsa_amd_profiling_get_dispatch_time(get_compute_agent(task->place),
                    signal, &metrics);
            ErrorCheck(Profiling GPU dispatch, err);
            if(task->atmi_task) {
                uint64_t freq;
                err = hsa_system_get_info(HSA_SYSTEM_INFO_TIMESTAMP_FREQUENCY,
                    &freq);
                ErrorCheck(Getting system timestamp frequency info, err);
                uint64_t start = metrics.start / (freq/NANOSECS);
                uint64_t end = metrics.end / (freq/NANOSECS);
                DEBUG_PRINT("Ticks: (%lu->%lu)\nFreq: %lu\nTime(ns: (%lu->%lu)\n",
                        metrics.start, metrics.end, freq, start, end);
                task->atmi_task->profile.start_time = start;
                task->atmi_task->profile.end_time = end;
                task->atmi_task->profile.dispatch_time = start;
                task->atmi_task->profile.ready_time = start;
            }
        }
        else {
            /* metrics for CPU tasks will be populated in the
             * worker pthread itself. No special function call */
        }
    }
}

extern void atl_stream_sync(atmi_task_group_table_t *stream_obj) {
    //else
    {
        #if 0
        DEBUG_PRINT("Waiting for async unordered tasks\n");
        hsa_signal_t signal = stream_obj->task_count;
        if(signal.handle != (uint64_t)-1) {
            #if 1
            hsa_signal_wait_acquire(signal, HSA_SIGNAL_CONDITION_EQ, 0, UINT64_MAX, ATMI_WAIT_STATE);
            #else
            // debugging atmi dp doorbell signaling. remove once that issue is resolved.
            hsa_signal_value_t val;
            do {
                val = hsa_signal_load_relaxed(signal);
                if(val) printf("Signal Value: %lu\n", val);
                /*hsa_queue_t *this_Q = GPU_CommandQ[0]->getQueue();
                const uint32_t queueMask = this_Q->size - 1;
                printf("Headers: (%lu %lu) ", hsa_queue_load_read_index_relaxed(this_Q), hsa_queue_load_write_index_relaxed(this_Q));
                for(uint64_t index = 0; index < 20; index++) {
                    hsa_kernel_dispatch_packet_t* this_aql = &(((hsa_kernel_dispatch_packet_t*)(this_Q->base_address))[index&queueMask]);
                    printf("%d ", (int)(0x00FF & this_aql->header));
                }
                printf("\n");*/
                sleep(1);
            } while (val != 0);
            #endif
        }
        else {
            fprintf(stderr, "Waiting for invalid task group signal!\n");
        }
/*
        atl_task_list_t *task_head = stream_obj->tasks;
        if(stream_obj->tasks == NULL ) return;
        while(task_head) {
            atl_task_wait(task_head->task);
            set_task_metrics(task_head->task);
            task_head = task_head->next;
        } */
        #else
        while(stream_obj->task_count.load() != 0) {}
        #endif
    }
    if(stream_obj->ordered == ATMI_TRUE) {
        atl_task_wait(stream_obj->last_task);
        lock(&(stream_obj->group_mutex));
        stream_obj->last_task = NULL;
        unlock(&(stream_obj->group_mutex));
    }
    clear_saved_tasks(stream_obj);
}

atmi_status_t atmi_task_group_sync(atmi_task_group_t *stream) {
    atmi_task_group_t *str = (stream == NULL) ? &atl_default_stream_obj : stream;
    atmi_task_group_table_t *stream_obj = StreamTable[str->id];
    TaskWaitTimer.Start();
    if(stream_obj) atl_stream_sync(stream_obj);
    else DEBUG_PRINT("Waiting for invalid task group signal!\n");
    TaskWaitTimer.Stop();
    return ATMI_STATUS_SUCCESS;
}

hsa_queue_t *acquire_and_set_next_cpu_queue(atmi_task_group_table_t *stream_obj, atmi_place_t place) {
    hsa_queue_t *queue = NULL;
    atmi_scheduler_t sched = stream_obj->ordered ? ATMI_SCHED_NONE : ATMI_SCHED_RR;
    ATLCPUProcessor &proc = get_processor<ATLCPUProcessor>(place);
    if(stream_obj->ordered) {
        if(stream_obj->cpu_queue == NULL) {
            stream_obj->cpu_queue = proc.getQueue(stream_obj->group_queue_id);
        }
        queue = stream_obj->cpu_queue;
    }
    else {
        queue = proc.getBestQueue(sched);
    }
    DEBUG_PRINT("Returned Queue: %p\n", queue);
    return queue;
}

std::vector<hsa_queue_t *> get_cpu_queues(atmi_place_t place) {
    ATLCPUProcessor &proc = get_processor<ATLCPUProcessor>(place);
    return proc.getQueues();
}

hsa_queue_t *acquire_and_set_next_gpu_queue(atmi_task_group_table_t *stream_obj, atmi_place_t place) {
    hsa_queue_t *queue = NULL;
    atmi_scheduler_t sched = stream_obj->ordered ? ATMI_SCHED_NONE : ATMI_SCHED_RR;
    ATLGPUProcessor &proc = get_processor<ATLGPUProcessor>(place);
    if(stream_obj->ordered) {
        if(stream_obj->gpu_queue == NULL) {
            stream_obj->gpu_queue = proc.getQueue(stream_obj->group_queue_id);
        }
        queue = stream_obj->gpu_queue;
    }
    else {
        queue = proc.getBestQueue(sched);
    }
    DEBUG_PRINT("Returned Queue: %p\n", queue);
    return queue;
}

atmi_status_t clear_saved_tasks(atmi_task_group_table_t *stream_obj) {
    atmi_status_t err = ATMI_STATUS_SUCCESS;
    lock(&(stream_obj->group_mutex));
    stream_obj->running_ordered_tasks.clear();
    stream_obj->running_groupable_tasks.clear();
    unlock(&(stream_obj->group_mutex));
    return err;
}

atmi_status_t check_change_in_device_type(atl_task_t *task, atmi_task_group_table_t *stream_obj, hsa_queue_t *queue, atmi_devtype_t new_task_device_type) {
    if(stream_obj->ordered != ATMI_ORDERED) return ATMI_STATUS_SUCCESS;

    if(stream_obj->last_device_type != new_task_device_type) {
        if(stream_obj->last_task != NULL) {
            DEBUG_PRINT("Devtype: %d waiting for task %p\n", new_task_device_type, stream_obj->last_task);
            /* device changed. introduce a dependency here for ordered streams */
            int num_required = 1;
            atl_task_t *requires = stream_obj->last_task;

            if(new_task_device_type == ATMI_DEVTYPE_GPU) {
                if(queue) {
                    enqueue_barrier(task, queue, num_required, &requires, SNK_NOWAIT, SNK_AND, ATMI_DEVTYPE_GPU);
                }
            }
            else {
                if(queue) {
                    enqueue_barrier(task, queue, num_required, &requires, SNK_NOWAIT, SNK_AND, ATMI_DEVTYPE_CPU);
                }
            }
        }
    }
    return ATMI_STATUS_SUCCESS;
}

atmi_status_t register_task(atmi_task_group_table_t *stream_obj, atl_task_t *task) {

    #if 0
    atl_task_list_t *node = (atl_task_list_t *)malloc(sizeof(atl_task_list_t));
    node->task = task;
    node->profilable = task->profilable;
    node->next = NULL;
    node->devtype = task->devtype;
    if(stream_obj->tasks == NULL) {
        stream_obj->tasks = node;
    } else {
        atl_task_list_t *cur = stream_obj->tasks;
        stream_obj->tasks = node;
        node->next = cur;
    }
    #endif
    if(task->groupable)
        stream_obj->running_groupable_tasks.push_back(task);
    stream_obj->last_device_type = task->devtype;
    //DEBUG_PRINT("Registering %s task %p Profilable? %s\n",
    //            (devtype == ATMI_DEVTYPE_GPU) ? "GPU" : "CPU",
    //            task, (profilable == ATMI_TRUE) ? "Yes" : "No");
    return ATMI_STATUS_SUCCESS;
}

atmi_status_t get_stream_signal(atmi_task_group_table_t *stream_obj, hsa_signal_t *signal) {
    *signal = stream_obj->group_signal;
    return ATMI_STATUS_SUCCESS;
}

atmi_status_t set_stream_signal(atmi_task_group_table_t *stream_obj, hsa_signal_t signal) {
    stream_obj->group_signal = signal;
    return ATMI_STATUS_SUCCESS;
}

/*
 * do not use the below because they will not work if we want to sort mutexes
atmi_status_t get_stream_mutex(atmi_task_group_table_t *stream_obj, pthread_mutex_t *m) {
    *m = stream_obj->group_mutex;
    return ATMI_STATUS_SUCCESS;
}

atmi_status_t set_stream_mutex(atmi_task_group_table_t *stream_obj, pthread_mutex_t *m) {
    stream_obj->group_mutex = *m;
    return ATMI_STATUS_SUCCESS;
}
*/
atmi_status_t register_stream(atmi_task_group_t *stream) {
    /* Check if the stream exists in the stream table.
     * If no, then add this stream to the stream table.
     */
    DEBUG_PRINT("Stream %p registered\n", stream);
    if(StreamTable.find(stream->id) == StreamTable.end()) {
        atmi_task_group_table_t *stream_entry = new atmi_task_group_table_t;
        stream_entry->last_task = NULL;
        stream_entry->running_groupable_tasks.clear();
        stream_entry->and_successors.clear();
        stream_entry->cpu_queue = NULL;
        stream_entry->gpu_queue = NULL;
        assert(StreamCommonSignalIdx < ATMI_MAX_STREAMS);
        stream_entry->group_queue_id = StreamCommonSignalIdx++;
        // get the next free signal from the stream common signal pool
        stream_entry->group_signal = StreamCommonSignalPool[stream_entry->group_queue_id];
        stream_entry->task_count.store(0);
        stream_entry->callback_started.clear();
        pthread_mutex_init(&(stream_entry->group_mutex), NULL);
        StreamTable[stream->id] = stream_entry;
    }
    lock(&(StreamTable[stream->id]->group_mutex));
    StreamTable[stream->id]->ordered = stream->ordered;
    unlock(&(StreamTable[stream->id]->group_mutex));

    return ATMI_STATUS_SUCCESS;
}

/*
   All global values are defined here in two data structures.

1  atmi_context is all information we expose externally.
   The structure atmi_context_t is defined in atmi.h.
   Most references will use pointer prefix atmi_context->
   The value atmi_context_data. is equivalent to atmi_context->

2  atlc is all internal global values.
   The structure atl_context_t is defined in atl_rt.h
   Most references will use the global structure prefix atlc.
   However the pointer value atlc_p-> is equivalent to atlc.

*/


atmi_context_t atmi_context_data;
atmi_context_t * atmi_context = NULL;
atl_context_t atlc = { .struct_initialized=0 };
atl_context_t * atlc_p = NULL;

void init_tasks() {
    if(atlc.g_tasks_initialized != 0) return;
    hsa_status_t err;
    int task_num;
    std::vector<hsa_agent_t> gpu_agents;
    int gpu_count = g_atl_machine.getProcessorCount<ATLGPUProcessor>();
    for(int gpu = 0; gpu < gpu_count; gpu++) {
        atmi_place_t place = ATMI_PLACE_GPU(0, gpu);
        ATLGPUProcessor &proc = get_processor<ATLGPUProcessor>(place);
        gpu_agents.push_back(proc.getAgent());
    }
/* Initialize all preallocated tasks and signals */
    for ( task_num = 0 ; task_num < ATMI_MAX_STREAMS; task_num++){
       hsa_signal_t new_signal;
       err=hsa_signal_create(0, 0, NULL, &new_signal);
       ErrorCheck(Creating a HSA signal, err);
       StreamCommonSignalPool[task_num] = new_signal;
    }
    for ( task_num = 0 ; task_num < g_max_signals; task_num++){
       hsa_signal_t new_signal;
       err=hsa_signal_create(0, 0, NULL, &new_signal);
       //err=hsa_signal_create(0, 1, &gpu_agents[0], &new_signal);
       ErrorCheck(Creating a HSA signal, err);
       FreeSignalPool.push(new_signal);
    }
    err=hsa_signal_create(1, 0, NULL, &IdentityORSignal);
    ErrorCheck(Creating a HSA signal, err);
    err=hsa_signal_create(0, 0, NULL, &IdentityANDSignal);
    ErrorCheck(Creating a HSA signal, err);
    err=hsa_signal_create(0, 0, NULL, &IdentityCopySignal);
    ErrorCheck(Creating a HSA signal, err);
    DEBUG_PRINT("Signal Pool Size: %lu\n", FreeSignalPool.size());
    atlc.g_tasks_initialized = 1;
}

void atmi_init_context_structs() {
    atmi_context = &atmi_context_data;
    atlc_p = &atlc;
    atlc.struct_initialized = 1; /* This only gets called one time */
    atlc.g_cpu_initialized = 0;
    atlc.g_hsa_initialized = 0;
    atlc.g_gpu_initialized = 0;
    atlc.g_tasks_initialized = 0;
}

atmi_status_t atl_init_context() {
    atl_init_gpu_context();
    atl_init_cpu_context();

    return ATMI_STATUS_SUCCESS;
}

void atl_set_atmi_initialized() {
    // FIXME: thread safe? locks?
    g_atmi_initialized = true;
}

void atl_reset_atmi_initialized() {
    // FIXME: thread safe? locks?
    g_atmi_initialized = false;
}

bool atl_is_atmi_initialized() {
    return g_atmi_initialized;
}

atmi_status_t atmi_ke_init() {
    // create and fill in the global structure needed for device enqueue
    // fill in gpu queues
    std::vector<hsa_queue_t *> gpu_queues;
    int gpu_count = g_atl_machine.getProcessorCount<ATLGPUProcessor>();
    for(int gpu = 0; gpu < gpu_count; gpu++) {
        int num_queues = 0;
        atmi_place_t place = ATMI_PLACE_GPU(0, gpu);
        ATLGPUProcessor &proc = get_processor<ATLGPUProcessor>(place);
        std::vector<hsa_queue_t *> qs = proc.getQueues();
        num_queues = qs.size();
        gpu_queues.insert(gpu_queues.end(), qs.begin(), qs.end());
        // TODO: how to handle queues from multiple devices? keep them separate?
		// Currently, first N queues correspond to GPU0, next N queues map to GPU1
		// and so on.
    }
    g_ke_args.num_gpu_queues = gpu_queues.size();
    void *gpu_queue_ptr;
    hsa_status_t err = hsa_amd_memory_pool_allocate(atl_gpu_kernarg_pool,
            sizeof(hsa_queue_t *) * g_ke_args.num_gpu_queues,
            0,
            &gpu_queue_ptr);
    ErrorCheck(Allocating GPU queue pointers, err);
    allow_access_to_all_gpu_agents(gpu_queue_ptr);
    for(int gpuq = 0; gpuq < gpu_queues.size(); gpuq++) {
        ((hsa_queue_t **)gpu_queue_ptr)[gpuq] = gpu_queues[gpuq];
    }
    g_ke_args.gpu_queue_ptr = gpu_queue_ptr;

    // fill in cpu queues
    std::vector<hsa_queue_t *> cpu_queues;
    int cpu_count = g_atl_machine.getProcessorCount<ATLCPUProcessor>();
    for(int cpu = 0; cpu < cpu_count; cpu++) {
        int num_queues = 0;
        atmi_place_t place = ATMI_PLACE_CPU(0, cpu);
        ATLCPUProcessor &proc = get_processor<ATLCPUProcessor>(place);
        std::vector<hsa_queue_t *> qs = proc.getQueues();
        num_queues = qs.size();
        cpu_queues.insert(cpu_queues.end(), qs.begin(), qs.end());
        // TODO: how to handle queues from multiple devices? keep them separate?
		// Currently, first N queues correspond to CPU0, next N queues map to CPU1
		// and so on.
    }
    g_ke_args.num_cpu_queues = cpu_queues.size();
    void *cpu_queue_ptr;
    err = hsa_amd_memory_pool_allocate(atl_gpu_kernarg_pool,
            sizeof(hsa_queue_t *) * g_ke_args.num_cpu_queues,
            0,
            &cpu_queue_ptr);
    ErrorCheck(Allocating CPU queue pointers, err);
    allow_access_to_all_gpu_agents(cpu_queue_ptr);
    for(int cpuq = 0; cpuq < cpu_queues.size(); cpuq++) {
        ((hsa_queue_t **)cpu_queue_ptr)[cpuq] = cpu_queues[cpuq];
    }
    g_ke_args.cpu_queue_ptr = cpu_queue_ptr;

    void *cpu_worker_signals;
    err = hsa_amd_memory_pool_allocate(atl_gpu_kernarg_pool,
            sizeof(hsa_signal_t) * g_ke_args.num_cpu_queues,
            0,
            &cpu_worker_signals);
    ErrorCheck(Allocating CPU queue iworker signals, err);
    allow_access_to_all_gpu_agents(cpu_worker_signals);
    for(int cpuq = 0; cpuq < cpu_queues.size(); cpuq++) {
        ((hsa_signal_t *)cpu_worker_signals)[cpuq] = *(get_worker_sig(cpu_queues[cpuq]));
    }
    g_ke_args.cpu_worker_signals = cpu_worker_signals;


    void *kernarg_template_ptr;
    // Allocate template space for shader kernels
    err = hsa_amd_memory_pool_allocate(atl_gpu_kernarg_pool,
            sizeof(atmi_kernel_enqueue_template_t) * MAX_NUM_KERNEL_TYPES,
            0,
            &kernarg_template_ptr);
    ErrorCheck(Allocating kernel argument template pointer, err);
    allow_access_to_all_gpu_agents(kernarg_template_ptr);
    g_ke_args.kernarg_template_ptr = kernarg_template_ptr;
    g_ke_args.kernel_counter = 0;
    return ATMI_STATUS_SUCCESS;
}

atmi_status_t atmi_init(atmi_devtype_t devtype) {
    atmi_status_t status = ATMI_STATUS_SUCCESS;
    if(atl_is_atmi_initialized()) return ATMI_STATUS_SUCCESS;

    task_process_init_buffer = NULL;
    task_process_fini_buffer = NULL;

    if(devtype == ATMI_DEVTYPE_ALL || devtype & ATMI_DEVTYPE_GPU)
        status = atl_init_gpu_context();

    if(devtype == ATMI_DEVTYPE_ALL || devtype & ATMI_DEVTYPE_CPU)
        status = atl_init_cpu_context();

    status = atmi_ke_init();

    if(status == ATMI_STATUS_SUCCESS) atl_set_atmi_initialized();
    return status;
}

atmi_status_t atmi_finalize() {
    // TODO: Finalize all processors, queues, signals, kernarg memory regions
    hsa_status_t err;
    finalize_hsa();
    // free up the kernel enqueue related data
    for(int i = 0; i < g_ke_args.kernel_counter; i++) {
        atmi_kernel_enqueue_template_t *ke_template = &((atmi_kernel_enqueue_template_t *)g_ke_args.kernarg_template_ptr)[i];
        hsa_memory_free(ke_template->kernarg_regions);
    }
    hsa_memory_free(g_ke_args.kernarg_template_ptr);
    hsa_memory_free(g_ke_args.cpu_queue_ptr);
    hsa_memory_free(g_ke_args.cpu_worker_signals);
    hsa_memory_free(g_ke_args.gpu_queue_ptr);
    for(int i = 0; i < g_executables.size(); i++) {
        err = hsa_executable_destroy(g_executables[i]);
        ErrorCheck(Destroying executable, err);
    }
    if(atlc.g_cpu_initialized == 1) {
        agent_fini();
        atlc.g_cpu_initialized = 0;
    }

    for(int i = 0; i < SymbolInfoTable.size(); i++) {
        SymbolInfoTable[i].clear();
    }
    SymbolInfoTable.clear();
    for(int i = 0; i < KernelInfoTable.size(); i++) {
        KernelInfoTable[i].clear();
    }
    KernelInfoTable.clear();

    atl_reset_atmi_initialized();
    err = hsa_shut_down();
    ErrorCheck(Shutting down HSA, err);
    std::cout << ParamsInitTimer;
    std::cout << ParamsInitTimer;
    std::cout << TryLaunchTimer;
    std::cout << TryLaunchInitTimer;
    std::cout << ShouldDispatchTimer;
    std::cout << TryDispatchTimer;
    std::cout << TaskWaitTimer;
    std::cout << LockTimer;
    std::cout << HandleSignalTimer;
    std::cout << RegisterCallbackTimer;
#if 0
    std::cout << HandleSignalInvokeTimer;
    std::cout << SignalAddTimer;
    std::cout << "Max Ready Queue Size: " << max_ready_queue_sz << std::endl;
    std::cout << "Waiting Tasks: " << waiting_count << std::endl;
    std::cout << "Direct Dispatch Tasks: " << direct_dispatch << std::endl;
    std::cout << "Callback Dispatch Tasks: " << callback_dispatch << std::endl;
    std::cout << "SYNC_TASK(" << ret->id.lo << ");" << std::endl;
#endif
    ParamsInitTimer.Reset();
    TryLaunchTimer.Reset();
    TryLaunchInitTimer.Reset();
    ShouldDispatchTimer.Reset();
    HandleSignalTimer.Reset();
    HandleSignalInvokeTimer.Reset();
    TryDispatchTimer.Reset();
    LockTimer.Reset();
    RegisterCallbackTimer.Reset();
    max_ready_queue_sz = 0;
    waiting_count = 0;
    direct_dispatch = 0;
    callback_dispatch = 0;

    return ATMI_STATUS_SUCCESS;
}

hsa_status_t init_comute_and_memory() {
    hsa_status_t err;
#ifdef MEMORY_REGION
    err = hsa_iterate_agents(get_gpu_agent, &atl_gpu_agent);
    if(err == HSA_STATUS_INFO_BREAK) { err = HSA_STATUS_SUCCESS; }
    ErrorCheck(Getting a gpu agent, err);

    /* Query the name of the agent.  */
    //char name[64] = { 0 };
    //err = hsa_agent_get_info(atl_gpu_agent, HSA_AGENT_INFO_NAME, name);
    //ErrorCheck(Querying the agent name, err);
    /* printf("The agent name is %s.\n", name); */
    err = hsa_agent_get_info(atl_gpu_agent, HSA_AGENT_INFO_PROFILE, &atl_gpu_agent_profile);
    ErrorCheck(Query the agent profile, err);
    DEBUG_PRINT("Agent Profile: %d\n", atl_gpu_agent_profile);

    /* Find a memory region that supports kernel arguments.  */
    atl_gpu_kernarg_region.handle=(uint64_t)-1;
    hsa_agent_iterate_regions(atl_gpu_agent, get_kernarg_memory_region, &atl_gpu_kernarg_region);
    err = (atl_gpu_kernarg_region.handle == (uint64_t)-1) ? HSA_STATUS_ERROR : HSA_STATUS_SUCCESS;
    ErrorCheck(Finding a kernarg memory region, err);

    err = hsa_iterate_agents(get_cpu_agent, &atl_cpu_agent);
    if(err == HSA_STATUS_INFO_BREAK) { err = HSA_STATUS_SUCCESS; }
    ErrorCheck(Getting a gpu agent, err);

    atl_cpu_kernarg_region.handle=(uint64_t)-1;
    err = hsa_agent_iterate_regions(atl_cpu_agent, get_fine_grained_region, &atl_cpu_kernarg_region);
    if(err == HSA_STATUS_INFO_BREAK) { err = HSA_STATUS_SUCCESS; }
    err = (atl_cpu_kernarg_region.handle == (uint64_t)-1) ? HSA_STATUS_ERROR : HSA_STATUS_SUCCESS;
    ErrorCheck(Finding a CPU kernarg memory region handle, err);
#else
    /* Iterate over the agents and pick the gpu agent */
    err = hsa_iterate_agents(get_agent_info, NULL);
    if(err == HSA_STATUS_INFO_BREAK) { err = HSA_STATUS_SUCCESS; }
    ErrorCheck(Getting a gpu agent, err);
    if(err != HSA_STATUS_SUCCESS) return err;

    /* Init all devices or individual device types? */
    std::vector<ATLCPUProcessor> &cpu_procs = g_atl_machine.getProcessors<ATLCPUProcessor>();
    std::vector<ATLGPUProcessor> &gpu_procs = g_atl_machine.getProcessors<ATLGPUProcessor>();
    std::vector<ATLDSPProcessor> &dsp_procs = g_atl_machine.getProcessors<ATLDSPProcessor>();
    /* For CPU memory pools, add other devices that can access them directly
     * or indirectly */
    for(std::vector<ATLCPUProcessor>::iterator cpu_it = cpu_procs.begin();
            cpu_it != cpu_procs.end(); cpu_it++) {
        std::vector<ATLMemory> &cpu_mems = cpu_it->getMemories();
        for(std::vector<ATLMemory>::iterator cpu_mem_it = cpu_mems.begin();
                cpu_mem_it != cpu_mems.end(); cpu_mem_it++) {
            hsa_amd_memory_pool_t pool = cpu_mem_it->getMemory();
            for(std::vector<ATLGPUProcessor>::iterator gpu_it = gpu_procs.begin();
                    gpu_it != gpu_procs.end(); gpu_it++) {
                hsa_agent_t agent = gpu_it->getAgent();
                hsa_amd_memory_pool_access_t access;
                hsa_amd_agent_memory_pool_get_info(agent,
                        pool,
                        HSA_AMD_AGENT_MEMORY_POOL_INFO_ACCESS,
                        &access);
                if(access != 0) {
                    // this means not NEVER, but could be YES or NO
                    // add this memory pool to the proc
                    gpu_it->addMemory(*cpu_mem_it);
                }
            }
            for(std::vector<ATLDSPProcessor>::iterator dsp_it = dsp_procs.begin();
                    dsp_it != dsp_procs.end(); dsp_it++) {
                hsa_agent_t agent = dsp_it->getAgent();
                hsa_amd_memory_pool_access_t access;
                hsa_amd_agent_memory_pool_get_info(agent,
                        pool,
                        HSA_AMD_AGENT_MEMORY_POOL_INFO_ACCESS,
                        &access);
                if(access != 0) {
                    // this means not NEVER, but could be YES or NO
                    // add this memory pool to the proc
                    dsp_it->addMemory(*cpu_mem_it);
                }
            }
        }
    }

    /* FIXME: are the below combinations of procs and memory pools needed?
     * all to all compare procs with their memory pools and add those memory
     * pools that are accessible by the target procs */
    for(std::vector<ATLGPUProcessor>::iterator gpu_it = gpu_procs.begin();
            gpu_it != gpu_procs.end(); gpu_it++) {
        std::vector<ATLMemory> &gpu_mems = gpu_it->getMemories();
        for(std::vector<ATLMemory>::iterator gpu_mem_it = gpu_mems.begin();
                gpu_mem_it != gpu_mems.end(); gpu_mem_it++) {
            hsa_amd_memory_pool_t pool = gpu_mem_it->getMemory();
            for(std::vector<ATLDSPProcessor>::iterator dsp_it = dsp_procs.begin();
                    dsp_it != dsp_procs.end(); dsp_it++) {
                hsa_agent_t agent = dsp_it->getAgent();
                hsa_amd_memory_pool_access_t access;
                hsa_amd_agent_memory_pool_get_info(agent,
                        pool,
                        HSA_AMD_AGENT_MEMORY_POOL_INFO_ACCESS,
                        &access);
                if(access != 0) {
                    // this means not NEVER, but could be YES or NO
                    // add this memory pool to the proc
                    dsp_it->addMemory(*gpu_mem_it);
                }
            }

            for(std::vector<ATLCPUProcessor>::iterator cpu_it = cpu_procs.begin();
                    cpu_it != cpu_procs.end(); cpu_it++) {
                hsa_agent_t agent = cpu_it->getAgent();
                hsa_amd_memory_pool_access_t access;
                hsa_amd_agent_memory_pool_get_info(agent,
                        pool,
                        HSA_AMD_AGENT_MEMORY_POOL_INFO_ACCESS,
                        &access);
                if(access != 0) {
                    // this means not NEVER, but could be YES or NO
                    // add this memory pool to the proc
                    cpu_it->addMemory(*gpu_mem_it);
                }
            }
        }
    }

    for(std::vector<ATLDSPProcessor>::iterator dsp_it = dsp_procs.begin();
            dsp_it != dsp_procs.end(); dsp_it++) {
        std::vector<ATLMemory> &dsp_mems = dsp_it->getMemories();
        for(std::vector<ATLMemory>::iterator dsp_mem_it = dsp_mems.begin();
                dsp_mem_it != dsp_mems.end(); dsp_mem_it++) {
            hsa_amd_memory_pool_t pool = dsp_mem_it->getMemory();
            for(std::vector<ATLGPUProcessor>::iterator gpu_it = gpu_procs.begin();
                    gpu_it != gpu_procs.end(); gpu_it++) {
                hsa_agent_t agent = gpu_it->getAgent();
                hsa_amd_memory_pool_access_t access;
                hsa_amd_agent_memory_pool_get_info(agent,
                        pool,
                        HSA_AMD_AGENT_MEMORY_POOL_INFO_ACCESS,
                        &access);
                if(access != 0) {
                    // this means not NEVER, but could be YES or NO
                    // add this memory pool to the proc
                    gpu_it->addMemory(*dsp_mem_it);
                }
            }

            for(std::vector<ATLCPUProcessor>::iterator cpu_it = cpu_procs.begin();
                    cpu_it != cpu_procs.end(); cpu_it++) {
                hsa_agent_t agent = cpu_it->getAgent();
                hsa_amd_memory_pool_access_t access;
                hsa_amd_agent_memory_pool_get_info(agent,
                        pool,
                        HSA_AMD_AGENT_MEMORY_POOL_INFO_ACCESS,
                        &access);
                if(access != 0) {
                    // this means not NEVER, but could be YES or NO
                    // add this memory pool to the proc
                    cpu_it->addMemory(*dsp_mem_it);
                }
            }
        }
    }

    g_atmi_machine.device_count_by_type[ATMI_DEVTYPE_CPU] = cpu_procs.size();
    g_atmi_machine.device_count_by_type[ATMI_DEVTYPE_GPU] = gpu_procs.size();
    g_atmi_machine.device_count_by_type[ATMI_DEVTYPE_DSP] = dsp_procs.size();
    size_t num_procs = cpu_procs.size() + gpu_procs.size() + dsp_procs.size();
    //g_atmi_machine.devices = (atmi_device_t *)malloc(num_procs * sizeof(atmi_device_t));
    atmi_device_t *all_devices = (atmi_device_t *)malloc(num_procs * sizeof(atmi_device_t));
    int num_iGPUs = 0;
    int num_dGPUs = 0;
    for(int i = 0; i < gpu_procs.size(); i++) {
        if(gpu_procs[i].getType() == ATMI_DEVTYPE_iGPU)
            num_iGPUs++;
        else
            num_dGPUs++;
    }
    assert(num_iGPUs + num_dGPUs == gpu_procs.size() && "Number of dGPUs and iGPUs do not add up");
    DEBUG_PRINT("CPU Agents: %lu\n", cpu_procs.size());
    DEBUG_PRINT("iGPU Agents: %d\n", num_iGPUs);
    DEBUG_PRINT("dGPU Agents: %d\n", num_dGPUs);
    DEBUG_PRINT("GPU Agents: %lu\n", gpu_procs.size());
    DEBUG_PRINT("DSP Agents: %lu\n", dsp_procs.size());
    g_atmi_machine.device_count_by_type[ATMI_DEVTYPE_iGPU] = num_iGPUs;
    g_atmi_machine.device_count_by_type[ATMI_DEVTYPE_dGPU] = num_dGPUs;

    int cpus_begin = 0;
    int cpus_end = cpu_procs.size();
    int gpus_begin = cpu_procs.size();
    int gpus_end = cpu_procs.size() + gpu_procs.size();
    g_atmi_machine.devices_by_type[ATMI_DEVTYPE_CPU] = &all_devices[cpus_begin];
    g_atmi_machine.devices_by_type[ATMI_DEVTYPE_GPU] = &all_devices[gpus_begin];
    g_atmi_machine.devices_by_type[ATMI_DEVTYPE_iGPU] = &all_devices[gpus_begin];
    g_atmi_machine.devices_by_type[ATMI_DEVTYPE_dGPU] = &all_devices[gpus_begin];
    int proc_index = 0;
    for(int i = cpus_begin; i < cpus_end; i++) {
        all_devices[i].type = cpu_procs[proc_index].getType();
        all_devices[i].core_count = cpu_procs[proc_index].getNumCUs();

        std::vector<ATLMemory> memories = cpu_procs[proc_index].getMemories();
        int fine_memories_size = 0;
        int coarse_memories_size = 0;
        DEBUG_PRINT("CPU memory types:\t");
        for(std::vector<ATLMemory>::iterator it = memories.begin();
                            it != memories.end(); it++) {
            atmi_memtype_t type = it->getType();
            if(type == ATMI_MEMTYPE_FINE_GRAINED) {
                fine_memories_size++;
                DEBUG_PRINT("Fine\t");
            }
            else {
                coarse_memories_size++;
                DEBUG_PRINT("Coarse\t");
            }
        }
        DEBUG_PRINT("\nFine Memories : %d", fine_memories_size);
        DEBUG_PRINT("\tCoarse Memories : %d\n", coarse_memories_size);
        all_devices[i].memory_count = memories.size();
        proc_index++;
    }
    proc_index = 0;
    for(int i = gpus_begin; i < gpus_end; i++) {
        all_devices[i].type = gpu_procs[proc_index].getType();
        all_devices[i].core_count = gpu_procs[proc_index].getNumCUs();

        std::vector<ATLMemory> memories = gpu_procs[proc_index].getMemories();
        int fine_memories_size = 0;
        int coarse_memories_size = 0;
        DEBUG_PRINT("GPU memory types:\t");
        for(std::vector<ATLMemory>::iterator it = memories.begin();
                            it != memories.end(); it++) {
            atmi_memtype_t type = it->getType();
            if(type == ATMI_MEMTYPE_FINE_GRAINED) {
                fine_memories_size++;
                DEBUG_PRINT("Fine\t");
            }
            else {
                coarse_memories_size++;
                DEBUG_PRINT("Coarse\t");
            }
        }
        DEBUG_PRINT("\nFine Memories : %d", fine_memories_size);
        DEBUG_PRINT("\tCoarse Memories : %d\n", coarse_memories_size);
        all_devices[i].memory_count = memories.size();
        proc_index++;
    }
    proc_index = 0;
    atl_cpu_kernarg_region.handle=(uint64_t)-1;
    if(cpu_procs.size() > 0) {
        err = hsa_agent_iterate_regions(cpu_procs[0].getAgent(), get_fine_grained_region, &atl_cpu_kernarg_region);
        if(err == HSA_STATUS_INFO_BREAK) { err = HSA_STATUS_SUCCESS; }
        err = (atl_cpu_kernarg_region.handle == (uint64_t)-1) ? HSA_STATUS_ERROR : HSA_STATUS_SUCCESS;
        ErrorCheck(Finding a CPU kernarg memory region handle, err);
    }
    /* Find a memory region that supports kernel arguments.  */
    atl_gpu_kernarg_region.handle=(uint64_t)-1;
    if(gpu_procs.size() > 0) {
        hsa_agent_iterate_regions(gpu_procs[0].getAgent(), get_kernarg_memory_region, &atl_gpu_kernarg_region);
        err = (atl_gpu_kernarg_region.handle == (uint64_t)-1) ? HSA_STATUS_ERROR : HSA_STATUS_SUCCESS;
        ErrorCheck(Finding a kernarg memory region, err);
    }
    if(num_procs > 0)
        return HSA_STATUS_SUCCESS;
    else
        return HSA_STATUS_ERROR_NOT_INITIALIZED;
#endif
}

hsa_status_t init_hsa() {
    if(atlc.g_hsa_initialized == 0) {
        DEBUG_PRINT("Initializing HSA...");
        hsa_status_t err = hsa_init();
        ErrorCheck(Initializing the hsa runtime, err);
        if(err != HSA_STATUS_SUCCESS) return err;
        char * dep_sync_type = getenv("ATMI_DEPENDENCY_SYNC_TYPE");
        if(dep_sync_type == NULL || strcmp(dep_sync_type, "ATMI_SYNC_CALLBACK") == 0) {
            g_dep_sync_type = ATL_SYNC_CALLBACK;
        }
        else if(strcmp(dep_sync_type, "ATMI_SYNC_BARRIER_PKT") == 0) {
            g_dep_sync_type = ATL_SYNC_BARRIER_PKT;
        }
        char * max_signals = getenv("ATMI_MAX_HSA_SIGNALS");
        g_max_signals = 24;
        if(max_signals != NULL)
            g_max_signals = atoi(max_signals);
        err = init_comute_and_memory();
        if(err != HSA_STATUS_SUCCESS) return err;
        ErrorCheck(After initializing compute and memory, err);
        init_dag_scheduler();
        atlc.g_hsa_initialized = 1;
        DEBUG_PRINT("done\n");
    }
    return HSA_STATUS_SUCCESS;
}

hsa_status_t finalize_hsa() {
    return HSA_STATUS_SUCCESS;
}

atmi_status_t atl_init_gpu_context() {

    if(atlc.struct_initialized == 0) atmi_init_context_structs();
    if(atlc.g_gpu_initialized != 0) return ATMI_STATUS_SUCCESS;

    hsa_status_t err;
    err = init_hsa();
    if(err != HSA_STATUS_SUCCESS) return ATMI_STATUS_ERROR;
    int num_queues = -1;
    /* TODO: If we get a good use case for device-specific worker count, we
     * should explore it, but let us keep the worker count uniform for all
     * devices of a type until that time
     */
    char *num_gpu_workers = getenv("ATMI_DEVICE_GPU_WORKERS");
    if(num_gpu_workers) num_queues = atoi(num_gpu_workers);

    int gpu_count = g_atl_machine.getProcessorCount<ATLGPUProcessor>();
    for(int gpu = 0; gpu < gpu_count; gpu++) {
        atmi_place_t place = ATMI_PLACE_GPU(0, gpu);
        ATLGPUProcessor &proc = get_processor<ATLGPUProcessor>(place);
        if(num_queues == -1) {
            num_queues = proc.getNumCUs();
            num_queues = (num_queues > 8) ? 8 : num_queues;
        }
        proc.createQueues(num_queues);
    }

    if(context_init_time_init == 0) {
        clock_gettime(CLOCK_MONOTONIC_RAW,&context_init_time);
        context_init_time_init = 1;
    }

    init_tasks();
    atlc.g_gpu_initialized = 1;
    return ATMI_STATUS_SUCCESS;
}

atmi_status_t atl_init_cpu_context() {

    if(atlc.struct_initialized == 0) atmi_init_context_structs();

    if(atlc.g_cpu_initialized != 0) return ATMI_STATUS_SUCCESS;

    hsa_status_t err;
    err = init_hsa();
    if(err != HSA_STATUS_SUCCESS) return ATMI_STATUS_ERROR;
    int num_queues = -1;
    /* TODO: If we get a good use case for device-specific worker count, we
     * should explore it, but let us keep the worker count uniform for all
     * devices of a type until that time
     */
    char *num_cpu_workers = getenv("ATMI_DEVICE_CPU_WORKERS");
    if(num_cpu_workers) num_queues = atoi(num_cpu_workers);

    // FIXME: For some reason, if CPU context is initialized before, the GPU queues dont get
    // created. They exit with 0x1008 out of resources. HACK!!!
    atl_init_gpu_context();
    /* Get a CPU agent, create a pthread to handle packets*/
    /* Iterate over the agents and pick the cpu agent */
#if defined (ATMI_HAVE_PROFILE)
    atmi_profiling_init();
#endif /*ATMI_HAVE_PROFILE */
    int cpu_count = g_atl_machine.getProcessorCount<ATLCPUProcessor>();
    for(int cpu = 0; cpu < cpu_count; cpu++) {
        atmi_place_t place = ATMI_PLACE_CPU(0, cpu);
        ATLCPUProcessor &proc = get_processor<ATLCPUProcessor>(place);
        // FIXME: We are creating as many CPU queues as there are cores
        // But, this will share CPU worker threads with main host thread
        // and the HSA callback thread. Is there any real benefit from
        // restricting the number of queues to num_cus - 2?
        if(num_queues == -1) {
            num_queues = proc.getNumCUs();
        }
        cpu_agent_init(cpu, num_queues);
    }


    init_tasks();
    atlc.g_cpu_initialized = 1;
    return ATMI_STATUS_SUCCESS;
}

void *atl_read_binary_from_file(const char *module, size_t *module_size) {
    // Open file.
    std::ifstream file(module, std::ios::in | std::ios::binary);
    if(!(file.is_open() && file.good())) {
        fprintf(stderr, "File %s not found\n", module);
        return NULL;
    }

    // Find out file size.
    file.seekg(0, file.end);
    size_t size = file.tellg();
    file.seekg(0, file.beg);

    // Allocate memory for raw code object.
    void *raw_code_object = malloc(size);
    assert(raw_code_object);

    // Read file contents.
    file.read((char*)raw_code_object, size);

    // Close file.
    file.close();
    *module_size = size;
    return raw_code_object;
}

atmi_status_t atl_gpu_create_program() {
    hsa_status_t err;
    /* Create hsa program.  */
    memset(&atl_hsa_program,0,sizeof(hsa_ext_program_t));
    err = hsa_ext_program_create(HSA_MACHINE_MODEL_LARGE, atl_gpu_agent_profile, HSA_DEFAULT_FLOAT_ROUNDING_MODE_DEFAULT, NULL, &atl_hsa_program);
    ErrorCheck(Create the program, err);
    return ATMI_STATUS_SUCCESS;
}

atmi_status_t atl_gpu_add_brig_module(char _CN__HSA_BrigMem[]) {
    hsa_status_t err;
    /* Add the BRIG module to hsa program.  */
    err = hsa_ext_program_add_module(atl_hsa_program, (hsa_ext_module_t)_CN__HSA_BrigMem);
    ErrorCheck(Adding the brig module to the program, err);
    return ATMI_STATUS_SUCCESS;
}

atmi_status_t atl_gpu_create_executable(hsa_executable_t *executable) {
    /* Create the empty executable.  */
    hsa_status_t err = hsa_executable_create(atl_gpu_agent_profile, HSA_EXECUTABLE_STATE_UNFROZEN, "", executable);
    ErrorCheck(Create the executable, err);
    return ATMI_STATUS_SUCCESS;
}

atmi_status_t atl_gpu_freeze_executable(hsa_executable_t *executable) {
    /* Freeze the executable; it can now be queried for symbols.  */
    hsa_status_t err = hsa_executable_freeze(*executable, "");
    ErrorCheck(Freeze the executable, err);
    return ATMI_STATUS_SUCCESS;
}

atmi_status_t atl_gpu_add_finalized_module(hsa_executable_t *executable, char *module, const size_t module_sz) {
#if 0
    // Open file.
    std::ifstream file(module, std::ios::in | std::ios::binary);
    assert(file.is_open() && file.good());

    // Find out file size.
    file.seekg(0, file.end);
    size_t size = file.tellg();
    file.seekg(0, file.beg);

    // Allocate memory for raw code object.
    void *raw_code_object = malloc(size);
    assert(raw_code_object);

    // Read file contents.
    file.read((char*)raw_code_object, size);

    // Close file.
    file.close();

    // Deserialize code object.
    hsa_code_object_t code_object = {0};
    hsa_status_t err = hsa_code_object_deserialize(raw_code_object, size, NULL, &code_object);
    ErrorCheck(Code Object Deserialization, err);
    assert(0 != code_object.handle);

    // Free raw code object memory.
    free(raw_code_object);
#else
    // Deserialize code object.
    hsa_code_object_t code_object = {0};
    hsa_status_t err = hsa_code_object_deserialize(module, module_sz, NULL, &code_object);
    ErrorCheck(Code Object Deserialization, err);
    assert(0 != code_object.handle);

#endif
    /* Load the code object.  */
    err = hsa_executable_load_code_object(*executable, atl_gpu_agent, code_object, "");
    ErrorCheck(Loading the code object, err);
    return ATMI_STATUS_SUCCESS;
}

atmi_status_t atl_gpu_build_executable(hsa_executable_t *executable) {
    hsa_status_t err;
    /* Determine the agents ISA.  */
    hsa_isa_t isa;
    err = hsa_agent_get_info(atl_gpu_agent, HSA_AGENT_INFO_ISA, &isa);
    ErrorCheck(Query the agents isa, err);

    /* * Finalize the program and extract the code object.  */
    hsa_ext_control_directives_t control_directives;
    memset(&control_directives, 0, sizeof(hsa_ext_control_directives_t));
    hsa_code_object_t code_object;
    err = hsa_ext_program_finalize(atl_hsa_program, isa, 0, control_directives, "-O2", HSA_CODE_OBJECT_TYPE_PROGRAM, &code_object);
    ErrorCheck(Finalizing the program, err);

    /* Destroy the program, it is no longer needed.  */
    err=hsa_ext_program_destroy(atl_hsa_program);
    ErrorCheck(Destroying the program, err);

    /* Create the empty executable.  */
    err = hsa_executable_create(atl_gpu_agent_profile, HSA_EXECUTABLE_STATE_UNFROZEN, "", executable);
    ErrorCheck(Create the executable, err);

    /* Load the code object.  */
    err = hsa_executable_load_code_object(*executable, atl_gpu_agent, code_object, "");
    ErrorCheck(Loading the code object, err);

    /* Freeze the executable; it can now be queried for symbols.  */
    err = hsa_executable_freeze(*executable, "");
    ErrorCheck(Freeze the executable, err);

    return ATMI_STATUS_SUCCESS;
}

hsa_status_t validate_code_object(hsa_code_object_t code_object, hsa_code_symbol_t symbol, void *data) {
    hsa_status_t retVal = HSA_STATUS_SUCCESS;
    int gpu = *(int *)data;
    hsa_symbol_kind_t type;

    uint32_t name_length;
    hsa_status_t err;
    err = hsa_code_symbol_get_info(symbol, HSA_CODE_SYMBOL_INFO_TYPE, &type);
    ErrorCheck(Symbol info extraction, err);
    DEBUG_PRINT("Exec Symbol type: %d\n", type);

   if(type == HSA_SYMBOL_KIND_VARIABLE) {
        err = hsa_code_symbol_get_info(symbol, HSA_CODE_SYMBOL_INFO_NAME_LENGTH, &name_length);
        ErrorCheck(Symbol info extraction, err);
        char *name = (char *)malloc(name_length + 1);
        err = hsa_code_symbol_get_info(symbol, HSA_CODE_SYMBOL_INFO_NAME, name);
        ErrorCheck(Symbol info extraction, err);
        name[name_length] = 0;

        if(SymbolSet.find(std::string(name)) != SymbolSet.end()) {
            // Symbol already found. Return Error
            DEBUG_PRINT("Symbol %s already found!\n", name);
            retVal = HSA_STATUS_ERROR_VARIABLE_ALREADY_DEFINED;
        } else {
            SymbolSet.insert(std::string(name));
        }

        free(name);
    }
    else {
        DEBUG_PRINT("Symbol is an indirect function\n");
    }
    return retVal;

}

hsa_status_t create_kernarg_memory(hsa_executable_t executable, hsa_executable_symbol_t symbol, void *data) {
    int gpu = *(int *)data;
    hsa_symbol_kind_t type;

    uint32_t name_length;
    hsa_status_t err;
    err = hsa_executable_symbol_get_info(symbol, HSA_EXECUTABLE_SYMBOL_INFO_TYPE, &type);
    ErrorCheck(Symbol info extraction, err);
    DEBUG_PRINT("Exec Symbol type: %d\n", type);
    if(type == HSA_SYMBOL_KIND_KERNEL) {
        err = hsa_executable_symbol_get_info(symbol, HSA_EXECUTABLE_SYMBOL_INFO_NAME_LENGTH, &name_length);
        ErrorCheck(Symbol info extraction, err);
        char *name = (char *)malloc(name_length + 1);
        err = hsa_executable_symbol_get_info(symbol, HSA_EXECUTABLE_SYMBOL_INFO_NAME, name);
        ErrorCheck(Symbol info extraction, err);
        name[name_length] = 0;
        atl_kernel_info_t info;
        /* Extract dispatch information from the symbol */
        err = hsa_executable_symbol_get_info(symbol, HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_OBJECT, &(info.kernel_object));
        ErrorCheck(Extracting the symbol from the executable, err);
        err = hsa_executable_symbol_get_info(symbol, HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_GROUP_SEGMENT_SIZE, &(info.group_segment_size));
        ErrorCheck(Extracting the group segment size from the executable, err);
        err = hsa_executable_symbol_get_info(symbol, HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_PRIVATE_SEGMENT_SIZE, &(info.private_segment_size));
        ErrorCheck(Extracting the private segment from the executable, err);

        /* Extract dispatch information from the symbol */
        err = hsa_executable_symbol_get_info(symbol, HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_KERNARG_SEGMENT_SIZE, &(info.kernel_segment_size));
        ErrorCheck(Extracting the kernarg segment size from the executable, err);

        // add size of implicit args, e.g.: offset x, y and z and pipe pointer
        info.kernel_segment_size += sizeof(atmi_implicit_args_t);

        DEBUG_PRINT("Kernel %s --> %lx symbol %u group segsize %u pvt segsize %u bytes kernarg\n", name,
            info.kernel_object,
            info.group_segment_size,
            info.private_segment_size,
            info.kernel_segment_size);
        KernelInfoTable[gpu][std::string(name)] = info;
        free(name);

        /*
        void *thisKernargAddress = NULL;
        // create a memory segment for this kernel's arguments
        err = hsa_memory_allocate(atl_gpu_kernarg_region, info.kernel_segment_size * MAX_NUM_KERNELS, &thisKernargAddress);
        ErrorCheck(Allocating memory for the executable-kernel, err);
        */
    }
    else if(type == HSA_SYMBOL_KIND_VARIABLE) {
        err = hsa_executable_symbol_get_info(symbol, HSA_EXECUTABLE_SYMBOL_INFO_NAME_LENGTH, &name_length);
        ErrorCheck(Symbol info extraction, err);
        char *name = (char *)malloc(name_length + 1);
        err = hsa_executable_symbol_get_info(symbol, HSA_EXECUTABLE_SYMBOL_INFO_NAME, name);
        ErrorCheck(Symbol info extraction, err);
        name[name_length] = 0;

        atl_symbol_info_t info;

        err = hsa_executable_symbol_get_info(symbol, HSA_EXECUTABLE_SYMBOL_INFO_VARIABLE_ADDRESS, &(info.addr));
        ErrorCheck(Symbol info address extraction, err);

        err = hsa_executable_symbol_get_info(symbol, HSA_EXECUTABLE_SYMBOL_INFO_VARIABLE_SIZE, &(info.size));
        ErrorCheck(Symbol info size extraction, err);

        atmi_mem_place_t place = ATMI_MEM_PLACE(ATMI_DEVTYPE_GPU, gpu, 0);
        register_allocation((void *)info.addr, (size_t)info.size, place);
        SymbolInfoTable[gpu][std::string(name)] = info;
        DEBUG_PRINT("Symbol %s = %p (%u bytes)\n", name, (void *)info.addr, info.size);
        free(name);
    }
    else {
        DEBUG_PRINT("Symbol is an indirect function\n");
    }
    return HSA_STATUS_SUCCESS;
}

// function pointers
task_process_init_buffer_t task_process_init_buffer;
task_process_fini_buffer_t task_process_fini_buffer;

atmi_status_t atmi_register_task_init_buffer(task_process_init_buffer_t fp)
{
  //printf("register function pointer \n");
  task_process_init_buffer = fp;

  return ATMI_STATUS_SUCCESS;
}

atmi_status_t atmi_register_task_fini_buffer(task_process_fini_buffer_t fp)
{
  //printf("register function pointer \n");
  task_process_fini_buffer = fp;

  return ATMI_STATUS_SUCCESS;
}

// register kernel data, including both metadata and gpu location
typedef struct metadata_per_gpu_s {
  atl_kernel_metadata_t md;
  int gpu;
} metadata_per_gpu_t;

atmi_status_t atmi_module_register_from_memory(void **modules, size_t *module_sizes, atmi_platform_type_t *types, const int num_modules) {
    hsa_status_t err;
    int some_success = 0;
    std::vector<std::string> modules_str;
    for(int i = 0; i < num_modules; i++) {
        modules_str.push_back(std::string((char *)modules[i]));
    }

    int gpu_count = g_atl_machine.getProcessorCount<ATLGPUProcessor>();

    KernelInfoTable.resize(gpu_count);
    SymbolInfoTable.resize(gpu_count);

    for(int gpu = 0; gpu < gpu_count; gpu++)
    {
        DEBUG_PRINT("Trying to load module to GPU-%d\n", gpu);
        atmi_place_t place = ATMI_PLACE_GPU(0, gpu);
        ATLGPUProcessor &proc = get_processor<ATLGPUProcessor>(place);
        hsa_agent_t agent = proc.getAgent();
        hsa_executable_t executable = {0};
        hsa_profile_t agent_profile;

        err = hsa_agent_get_info(agent, HSA_AGENT_INFO_PROFILE, &agent_profile);
        ErrorCheckAndContinue(Query the agent profile, err);
        // FIXME: Assume that every profile is FULL until we understand how to build BRIG with base profile
        agent_profile = HSA_PROFILE_FULL;
        /* Create the empty executable.  */
        err = hsa_executable_create(agent_profile, HSA_EXECUTABLE_STATE_UNFROZEN, "", &executable);
        ErrorCheckAndContinue(Create the executable, err);

        // clear symbol set for every executable
        SymbolSet.clear();
        int module_load_success = 0;
        for(int i = 0; i < num_modules; i++) {
            void *module_bytes = modules[i];
            size_t module_size = module_sizes[i];
            if(types[i] == BRIG) {
                hsa_ext_module_t module = (hsa_ext_module_t)module_bytes;

                hsa_ext_program_t program;
                /* Create hsa program.  */
                memset(&program,0,sizeof(hsa_ext_program_t));
                err = hsa_ext_program_create(HSA_MACHINE_MODEL_LARGE, agent_profile, HSA_DEFAULT_FLOAT_ROUNDING_MODE_DEFAULT, NULL, &program);
                ErrorCheckAndContinue(Create the program, err);

                /* Add the BRIG module to hsa program.  */
                err = hsa_ext_program_add_module(program, module);
                ErrorCheckAndContinue(Adding the brig module to the program, err);
                /* Determine the agents ISA.  */
                hsa_isa_t isa;
                err = hsa_agent_get_info(agent, HSA_AGENT_INFO_ISA, &isa);
                ErrorCheckAndContinue(Query the agents isa, err);

                /* * Finalize the program and extract the code object.  */
                hsa_ext_control_directives_t control_directives;
                memset(&control_directives, 0, sizeof(hsa_ext_control_directives_t));
                hsa_code_object_t code_object;
                err = hsa_ext_program_finalize(program, isa, 0, control_directives, "-O2", HSA_CODE_OBJECT_TYPE_PROGRAM, &code_object);
                ErrorCheckAndContinue(Finalizing the program, err);

                /* Destroy the program, it is no longer needed.  */
                err=hsa_ext_program_destroy(program);
                ErrorCheckAndContinue(Destroying the program, err);

                /* Load the code object.  */
                err = hsa_executable_load_code_object(executable, agent, code_object, "");
                ErrorCheckAndContinue(Loading the code object, err);
            }
            else if (types[i] == AMDGCN) {
                // Deserialize code object.
                hsa_code_object_t code_object = {0};
                err = hsa_code_object_deserialize(module_bytes, module_size, NULL, &code_object);
                ErrorCheckAndContinue(Code Object Deserialization, err);
                assert(0 != code_object.handle);

                err = hsa_code_object_iterate_symbols(code_object, validate_code_object, &gpu);
                ErrorCheckAndContinue(Iterating over symbols for execuatable, err);

                /* Load the code object.  */
                err = hsa_executable_load_code_object(executable, agent, code_object, NULL);
                ErrorCheckAndContinue(Loading the code object, err);


            }
            module_load_success = 1;
        }
        if(!module_load_success) continue;

        /* Freeze the executable; it can now be queried for symbols.  */
        err = hsa_executable_freeze(executable, "");
        ErrorCheckAndContinue(Freeze the executable, err);

        err = hsa_executable_iterate_symbols(executable, create_kernarg_memory, &gpu);
        ErrorCheckAndContinue(Iterating over symbols for execuatable, err);

        //err = hsa_executable_iterate_program_symbols(executable, iterate_program_symbols, &gpu);
        //ErrorCheckAndContinue(Iterating over symbols for execuatable, err);

        //err = hsa_executable_iterate_agent_symbols(executable, iterate_agent_symbols, &gpu);
        //ErrorCheckAndContinue(Iterating over symbols for execuatable, err);

        // save the executable and destroy during finalize
        g_executables.push_back(executable);
        some_success = 1;
    }
    DEBUG_PRINT("Modules loaded successful? %d\n", some_success);
    //ModuleMap[executable.handle] = modules_str;
    return (some_success) ? ATMI_STATUS_SUCCESS : ATMI_STATUS_ERROR;
}

atmi_status_t atmi_module_register(const char **filenames, atmi_platform_type_t *types, const int num_modules) {
    std::vector<void *> modules;
    std::vector<size_t> module_sizes;
    for(int i = 0; i < num_modules; i++) {
        size_t module_size;
        void *module_bytes = atl_read_binary_from_file(filenames[i], &module_size);
        if(!module_bytes) return ATMI_STATUS_ERROR;
        modules.push_back(module_bytes);
        module_sizes.push_back(module_size);
    }

    atmi_status_t status = atmi_module_register_from_memory(&modules[0], &module_sizes[0], types, num_modules);

    // memory space got by
    // void *raw_code_object = malloc(size);
    for(int i = 0; i < num_modules; i++) {
        free(modules[i]);
    }

    return status;
}


pthread_mutex_t sort_mutex_2(pthread_mutex_t *addr1, pthread_mutex_t *addr2, pthread_mutex_t **addr_low, pthread_mutex_t **addr_hi) {
    if((uint64_t)addr1 < (uint64_t)addr2) {
        *addr_low = addr1;
        *addr_hi = addr2;
    }
    else {
        *addr_hi = addr1;
        *addr_low = addr2;
    }
}

void lock(pthread_mutex_t *m) {
    //printf("Locked Mutex: %p\n", m);
    //LockTimer.Start();
    pthread_mutex_lock(m);
    //LockTimer.Stop();
}

void unlock(pthread_mutex_t *m) {
    //printf("Unlocked Mutex: %p\n", m);
    //LockTimer.Start();
    pthread_mutex_unlock(m);
    //LockTimer.Stop();
}

void print_rset(std::set<pthread_mutex_t *> &mutexes) {
    printf("[ ");
    for(std::set<pthread_mutex_t *>::reverse_iterator it = mutexes.rbegin();
                    it != mutexes.rend(); it++) {
        printf("%p ", *it);
    }
    printf("]\n");
}

void print_set(std::set<pthread_mutex_t *> &mutexes) {
    printf("[ ");
    for(std::set<pthread_mutex_t *>::iterator it = mutexes.begin();
                    it != mutexes.end(); it++) {
        printf("%p ", *it);
    }
    printf("]\n");
}

void print_sorted_vec(std::vector<pthread_mutex_t *> &mutexes) {
    std::sort(mutexes.begin(), mutexes.end());
    printf("[ ");
    for(size_t i = 0; i < mutexes.size(); i++) {
        printf("%p ", mutexes[i]);
    }
    printf("]\n");
}


void lock_set(std::set<pthread_mutex_t *> &mutexes) {
    DEBUG_PRINT("Locking [ ");
    for(std::set<pthread_mutex_t *>::iterator it = mutexes.begin();
                    it != mutexes.end(); it++) {
        DEBUG_PRINT("%p ", *it);
        lock((pthread_mutex_t *)(*it));
    }
    DEBUG_PRINT("]\n");
}

void unlock_set(std::set<pthread_mutex_t *> &mutexes) {
    DEBUG_PRINT("Unlocking [ ");
    for(std::set<pthread_mutex_t *>::reverse_iterator it = mutexes.rbegin();
                    it != mutexes.rend(); it++) {
        DEBUG_PRINT("%p ", *it);
        unlock((pthread_mutex_t *)(*it));
    }
    DEBUG_PRINT("]\n");
}

#if 0
void lock_vec(std::vector<pthread_mutex_t *> &mutexes) {
    std::sort(mutexes.begin(), mutexes.end());
    for(size_t i = 0; i < mutexes.size(); i++) {
        lock((pthread_mutex_t *)mutexes[i]);
    }
}

void unlock_vec(std::vector<pthread_mutex_t *> &mutexes) {
    for(size_t i = mutexes.size(); i-- > 0; ) {
        unlock((pthread_mutex_t *)mutexes[i]);
    }
}
#endif

template<typename T>
void clear_container(T &q)
{
   T empty;
   std::swap(q, empty);
}

atmi_status_t atmi_kernel_create_empty(atmi_kernel_t *atmi_kernel, const int num_args,
                                    const size_t *arg_sizes) {
    static uint64_t counter = 0;
    uint64_t pif_id = ++counter;
    atmi_kernel->handle = (uint64_t)pif_id;

    atl_kernel_t *kernel = new atl_kernel_t;
    kernel->id_map.clear();
    kernel->num_args = num_args;
    for(int i = 0; i < num_args; i++) {
        kernel->arg_sizes.push_back(arg_sizes[i]);
    }
    clear_container(kernel->impls);
    kernel->pif_id = pif_id;
    KernelImplMap[pif_id] = kernel;
    return ATMI_STATUS_SUCCESS;
}

atmi_status_t atmi_kernel_release(atmi_kernel_t atmi_kernel) {
    uint64_t pif_id = atmi_kernel.handle;

    atl_kernel_t *kernel = KernelImplMap[pif_id];
    //kernel->id_map.clear();
    clear_container(kernel->arg_sizes);
    for(std::vector<atl_kernel_impl_t *>::iterator it = kernel->impls.begin();
                        it != kernel->impls.end(); it++) {
        lock(&((*it)->mutex));
        if((*it)->devtype == ATMI_DEVTYPE_GPU) {
            // free the pipe_ptrs data
            // We create the pipe_ptrs region for all kernel instances
            // combined, and each instance of the kernel
            // invocation takes a piece of it. So, the first kernel instance
            // (k=0) will have the pointer to the entire pipe region itself.
            atmi_implicit_args_t *impl_args = (atmi_implicit_args_t *)(((char *)(*it)->kernarg_region) + (*it)->kernarg_segment_size - sizeof(atmi_implicit_args_t));
            void *pipe_ptrs = (void *)impl_args->pipe_ptr;
            DEBUG_PRINT("Freeing pipe ptr: %p\n", pipe_ptrs);
            hsa_memory_free(pipe_ptrs);
            hsa_memory_free((*it)->kernarg_region);
            free((*it)->kernel_objects);
            free((*it)->group_segment_sizes);
            free((*it)->private_segment_sizes);
        }
        else if((*it)->devtype == ATMI_DEVTYPE_CPU) {
            free((*it)->kernarg_region);
        }
        clear_container((*it)->free_kernarg_segments);
        unlock(&((*it)->mutex));
        delete *it;
    }
    clear_container(kernel->impls);
    delete kernel;

    KernelImplMap.erase(pif_id);
    atmi_kernel.handle = 0ull;
    return ATMI_STATUS_SUCCESS;
}

atmi_status_t atmi_kernel_create(atmi_kernel_t *atmi_kernel, const int num_args,
        const size_t *arg_sizes, const int num_impls, ...) {
    atmi_status_t status;
    hsa_status_t err;
    if(!atl_is_atmi_initialized()) return ATMI_STATUS_ERROR;
    status = atmi_kernel_create_empty(atmi_kernel, num_args, arg_sizes);
    ATMIErrorCheck(Creating kernel object, status);

    static int counter = 0;
    bool has_gpu_impl = false;
    uint64_t pif_id = atmi_kernel->handle;
    atl_kernel_t *kernel = KernelImplMap[pif_id];
    size_t max_kernarg_segment_size = 0;
    va_list arguments;
    va_start(arguments, num_impls);
    for(int impl_id = 0; impl_id < num_impls; impl_id++) {
        atmi_devtype_t devtype = (atmi_devtype_t)va_arg(arguments, int);
        if(devtype == ATMI_DEVTYPE_GPU) {
            const char *impl = va_arg(arguments, const char *);
            status = atmi_kernel_add_gpu_impl(*atmi_kernel, impl, impl_id);
            ATMIErrorCheck(Adding GPU kernel implementation, status);
            DEBUG_PRINT("GPU kernel %s added [%u]\n", impl, impl_id);
            has_gpu_impl = true;
        }
        else if(devtype == ATMI_DEVTYPE_CPU) {
            atmi_generic_fp impl = va_arg(arguments, atmi_generic_fp);
            status = atmi_kernel_add_cpu_impl(*atmi_kernel, impl, impl_id);
            ATMIErrorCheck(Adding CPU kernel implementation, status);
            DEBUG_PRINT("CPU kernel %p added [%u]\n", impl, impl_id);
        }
        else {
            fprintf(stderr, "Unsupported device type: %d\n", devtype);
            return ATMI_STATUS_ERROR;
        }
        size_t this_kernarg_segment_size = kernel->impls[impl_id]->kernarg_segment_size;
        if(this_kernarg_segment_size > max_kernarg_segment_size)
            max_kernarg_segment_size = this_kernarg_segment_size;
        ATMIErrorCheck(Creating kernel implementations, status);
        // rest of kernel impl fields will be populated at first kernel launch
    }
    va_end(arguments);
    //// FIXME EEEEEE: for EVERY GPU impl, add all CPU/GPU implementations in
    //their templates!!!
    if(has_gpu_impl) {
        // populate the AQL packet template for GPU kernel impls
        void *ke_kernarg_region;
        // first 4 bytes store the current index of the kernel arg region
        err = hsa_amd_memory_pool_allocate(atl_gpu_kernarg_pool,
                sizeof(int) + max_kernarg_segment_size * MAX_NUM_KERNELS,
                0,
                &ke_kernarg_region);
        ErrorCheck(Allocating memory for the executable-kernel, err);
        allow_access_to_all_gpu_agents(ke_kernarg_region);
        *(int *)ke_kernarg_region = 0;
        char *ke_kernargs = (char *)ke_kernarg_region + sizeof(int);

        // fill in the kernel template AQL packets
        int cur_kernel = g_ke_args.kernel_counter++;
        assert(cur_kernel < MAX_NUM_KERNEL_TYPES);
        if(cur_kernel >= MAX_NUM_KERNEL_TYPES) return ATMI_STATUS_ERROR;
        atmi_kernel_enqueue_template_t *ke_template = &((atmi_kernel_enqueue_template_t *)g_ke_args.kernarg_template_ptr)[cur_kernel];
        ke_template->kernel_handle = atmi_kernel->handle; // To be used by device code to pick a task template

        // fill in the kernel arg regions
        ke_template->kernarg_segment_size = max_kernarg_segment_size;
        ke_template->kernarg_regions = ke_kernarg_region;

        std::vector<atl_kernel_impl_t *>::iterator impl_it;
        int this_impl_id = 0;
        for(impl_it = kernel->impls.begin(); impl_it != kernel->impls.end(); impl_it++) {
            atl_kernel_impl_t *this_impl = *impl_it;
            if(this_impl->devtype == ATMI_DEVTYPE_GPU) {
                // fill in the GPU AQL template
                hsa_kernel_dispatch_packet_t *k_packet = &(ke_template->k_packet);
                k_packet->header = 0; //ATMI_DEVTYPE_GPU;
                k_packet->kernarg_address = NULL;
                k_packet->kernel_object = this_impl->kernel_objects[0];
                k_packet->private_segment_size = this_impl->private_segment_sizes[0];
                k_packet->group_segment_size = this_impl->group_segment_sizes[0];
            }
            else if(this_impl->devtype == ATMI_DEVTYPE_CPU) {

                // fill in the CPU AQL template
                hsa_agent_dispatch_packet_t *a_packet = &(ke_template->a_packet);
                a_packet->header = 0; //ATMI_DEVTYPE_CPU;
                a_packet->type = (uint16_t) this_impl_id;
                /* FIXME: We are considering only void return types for now.*/
                //a_packet->return_address = NULL;
                /* Set function args */
                a_packet->arg[0] = (uint64_t) ATMI_NULL_TASK_HANDLE;
                a_packet->arg[1] = (uint64_t) NULL;
                a_packet->arg[2] = (uint64_t) kernel; // pass task handle to fill in metrics
                a_packet->arg[3] = 0; // tasks can query for current task ID
            }
            this_impl_id++;
        }
        for(impl_it = kernel->impls.begin(); impl_it != kernel->impls.end(); impl_it++) {
            atl_kernel_impl_t *this_impl = *impl_it;
            if(this_impl->devtype == ATMI_DEVTYPE_GPU) {
                for(int k = 0; k < MAX_NUM_KERNELS; k++) {
                    atmi_implicit_args_t *impl_args = (atmi_implicit_args_t *)((char *)this_impl->kernarg_region + (((k + 1) * this_impl->kernarg_segment_size) - sizeof(atmi_implicit_args_t)));
                    // fill in the queue
                    impl_args->num_gpu_queues = g_ke_args.num_gpu_queues;
                    impl_args->gpu_queue_ptr = (uint64_t) g_ke_args.gpu_queue_ptr;
                    impl_args->num_cpu_queues = g_ke_args.num_cpu_queues;
                    impl_args->cpu_queue_ptr = (uint64_t) g_ke_args.cpu_queue_ptr;
                    impl_args->cpu_worker_signals = (uint64_t) g_ke_args.cpu_worker_signals;

                    // fill in the signals?
                    impl_args->kernarg_template_ptr = (uint64_t)g_ke_args.kernarg_template_ptr;

                    // *** fill in implicit args for kernel enqueue ***
                    atmi_implicit_args_t *ke_impl_args = (atmi_implicit_args_t *)((char *)ke_kernargs + (((k + 1) * this_impl->kernarg_segment_size) - sizeof(atmi_implicit_args_t)));
                    // SHARE the same pipe for printf etc
                    *ke_impl_args = *impl_args;
                }
            }
            // CPU impls dont use implicit args for now
        }
    }
    return ATMI_STATUS_SUCCESS;
}

atmi_status_t atmi_kernel_add_gpu_impl(atmi_kernel_t atmi_kernel, const char *impl, const unsigned int ID) {
    if(!atl_is_atmi_initialized()) return ATMI_STATUS_ERROR;
    uint64_t pif_id = atmi_kernel.handle;
    atl_kernel_t *kernel = KernelImplMap[pif_id];
    if(kernel->id_map.find(ID) != kernel->id_map.end()) {
        fprintf(stderr, "Kernel ID %d already found\n", ID);
        return ATMI_STATUS_ERROR;
    }
    std::string hsaco_name = std::string(impl );
    std::string brig_name = std::string("&__OpenCL_");
    brig_name += std::string(impl );
    brig_name += std::string("_kernel");

    atl_kernel_impl_t *kernel_impl = new atl_kernel_impl_t;
    kernel_impl->kernel_id = ID;
    unsigned int kernel_mapped_id = kernel->impls.size();
    kernel_impl->devtype = ATMI_DEVTYPE_GPU;

    std::vector<ATLGPUProcessor> &gpu_procs = g_atl_machine.getProcessors<ATLGPUProcessor>();
    int gpu_count = gpu_procs.size();
    kernel_impl->kernel_objects = (uint64_t *)malloc(sizeof(uint64_t) * gpu_count);
    kernel_impl->group_segment_sizes = (uint32_t *)malloc(sizeof(uint32_t) * gpu_count);
    kernel_impl->private_segment_sizes = (uint32_t *)malloc(sizeof(uint32_t) * gpu_count);
    int max_kernarg_segment_size = 0;
    std::string kernel_name;
    atmi_platform_type_t kernel_type;
    bool some_success = false;
    for(int gpu = 0; gpu < gpu_count; gpu++) {
        if(KernelInfoTable[gpu].find(hsaco_name) != KernelInfoTable[gpu].end()) {
			DEBUG_PRINT("Found kernel %s for GPU %d\n", hsaco_name.c_str(), gpu);
            kernel_name = hsaco_name;
            kernel_type = AMDGCN;
            some_success = true;
        }
        else if(KernelInfoTable[gpu].find(brig_name) != KernelInfoTable[gpu].end()) {
            kernel_name = brig_name;
            kernel_type = BRIG;
            some_success = true;
        }
        else {
            DEBUG_PRINT("Did NOT find kernel %s or %s for GPU %d\n",
                    hsaco_name.c_str(),
                    brig_name.c_str(),
                    gpu);
            continue;
        }
        atl_kernel_info_t info = KernelInfoTable[gpu][kernel_name];
        kernel_impl->kernel_objects[gpu] = info.kernel_object;
        kernel_impl->group_segment_sizes[gpu] = info.group_segment_size;
        kernel_impl->private_segment_sizes[gpu] = info.private_segment_size;
        if(max_kernarg_segment_size < info.kernel_segment_size)
            max_kernarg_segment_size = info.kernel_segment_size;
    }
    if(!some_success) return ATMI_STATUS_ERROR;
    kernel_impl->kernel_name = kernel_name;
    kernel_impl->kernel_type = kernel_type;
    kernel_impl->kernarg_segment_size = max_kernarg_segment_size;
    /* create kernarg memory */
    kernel_impl->kernarg_region = NULL;
#ifdef MEMORY_REGION
    hsa_status_t err = hsa_memory_allocate(atl_gpu_kernarg_region,
            kernel_impl->kernarg_segment_size * MAX_NUM_KERNELS,
            &(kernel_impl->kernarg_region));
    ErrorCheck(Allocating memory for the executable-kernel, err);
#else
    if(kernel_impl->kernarg_segment_size > 0) {
        DEBUG_PRINT("New kernarg segment size: %u\n", kernel_impl->kernarg_segment_size);
        hsa_status_t err = hsa_amd_memory_pool_allocate(atl_gpu_kernarg_pool,
                kernel_impl->kernarg_segment_size * MAX_NUM_KERNELS,
                0,
                &(kernel_impl->kernarg_region));
        ErrorCheck(Allocating memory for the executable-kernel, err);
        allow_access_to_all_gpu_agents(kernel_impl->kernarg_region);

        void *pipe_ptrs;
        // allocate pipe memory in the kernarg memory pool
        // TODO: may be possible to allocate this on device specific
        // memory but data movement will have to be done later by
        // post-processing kernel on destination agent.
        err = hsa_amd_memory_pool_allocate(atl_gpu_kernarg_pool,
                MAX_PIPE_SIZE * MAX_NUM_KERNELS,
                0,
                &pipe_ptrs);
        ErrorCheck(Allocating pipe memory region, err);
        DEBUG_PRINT("Allocating pipe ptr: %p\n", pipe_ptrs);
        allow_access_to_all_gpu_agents(pipe_ptrs);

        for(int k = 0; k < MAX_NUM_KERNELS; k++) {
            atmi_implicit_args_t *impl_args = (atmi_implicit_args_t *)((char *)kernel_impl->kernarg_region + (((k + 1) * kernel_impl->kernarg_segment_size) - sizeof(atmi_implicit_args_t)));
            impl_args->pipe_ptr = (uint64_t)((char *)pipe_ptrs + (k * MAX_PIPE_SIZE));
            impl_args->offset_x = 0;
            impl_args->offset_y = 0;
            impl_args->offset_z = 0;
        }
    }

#endif
    for(int i = 0; i < MAX_NUM_KERNELS; i++) {
        kernel_impl->free_kernarg_segments.push(i);
    }
    pthread_mutex_init(&(kernel_impl->mutex), NULL);

    kernel->id_map[ID] = kernel_mapped_id;

    kernel->impls.push_back(kernel_impl);
    // rest of kernel impl fields will be populated at first kernel launch
    return ATMI_STATUS_SUCCESS;
}

atmi_status_t atmi_kernel_add_cpu_impl(atmi_kernel_t atmi_kernel, atmi_generic_fp impl, const unsigned int ID) {
    if(!atl_is_atmi_initialized()) return ATMI_STATUS_ERROR;
    static int counter = 0;
    uint64_t pif_id = atmi_kernel.handle;
    std::string cl_pif_name("_x86_");
    cl_pif_name += std::to_string(counter);
    cl_pif_name += std::string("_");
    cl_pif_name += std::to_string(pif_id);
    counter++;

    atl_kernel_impl_t *kernel_impl = new atl_kernel_impl_t;
    kernel_impl->kernel_id = ID;
    kernel_impl->kernel_name = cl_pif_name;
    kernel_impl->devtype = ATMI_DEVTYPE_CPU;
    kernel_impl->function = impl;

    atl_kernel_t *kernel = KernelImplMap[pif_id];
    if(kernel->id_map.find(ID) != kernel->id_map.end()) {
        fprintf(stderr, "Kernel ID %d already found\n", ID);
        return ATMI_STATUS_ERROR;
    }
    kernel->id_map[ID] = kernel->impls.size();
    /* create kernarg memory */
    uint32_t kernarg_size = 0;
    for(int i = 0; i < kernel->num_args; i++){
        kernarg_size += kernel->arg_sizes[i];
    }
    kernel_impl->kernarg_segment_size = kernarg_size;
    kernel_impl->kernarg_region = NULL;
    if(kernarg_size)
        kernel_impl->kernarg_region = malloc(kernel_impl->kernarg_segment_size * MAX_NUM_KERNELS);
    for(int i = 0; i < MAX_NUM_KERNELS; i++) {
        kernel_impl->free_kernarg_segments.push(i);
    }

    pthread_mutex_init(&(kernel_impl->mutex), NULL);
    kernel->impls.push_back(kernel_impl);
    // rest of kernel impl fields will be populated at first kernel launch
    return ATMI_STATUS_SUCCESS;
}

bool is_valid_kernel_id(atl_kernel_t *kernel, unsigned int kernel_id) {
    std::map<unsigned int, unsigned int>::iterator it = kernel->id_map.find(kernel_id);
    if(it == kernel->id_map.end()) {
        fprintf(stderr, "ERROR: Kernel not found\n");
        return false;
    }
    int idx = it->second;
    if(idx >= kernel->impls.size()) {
        fprintf(stderr, "Kernel ID %d out of bounds (%lu)\n", kernel_id, kernel->impls.size());
        return false;
    }
    return true;
}

int get_kernel_index(atl_kernel_t *kernel, unsigned int kernel_id) {
    if(!is_valid_kernel_id(kernel, kernel_id)) {
        return -1;
    }
    return kernel->id_map[kernel_id];
}

atl_kernel_impl_t *get_kernel_impl(atl_kernel_t *kernel, unsigned int kernel_id) {
    int idx = get_kernel_index(kernel, kernel_id);
    if(idx < 0) {
        fprintf(stderr, "Incorrect Kernel ID %d\n", kernel_id);
        return NULL;
    }

    return kernel->impls[idx];
}

void dispatch_all_tasks(std::vector<atl_task_t*>::iterator &start,
        std::vector<atl_task_t*>::iterator &end) {
    for(std::vector<atl_task_t*>::iterator it = start; it != end; it++) {
        dispatch_task(*it);
    }
}

void handle_signal_callback(atl_task_t *task) {
    // tasks without atmi_task handle should not be added to callbacks anyway
    assert(task->groupable != ATMI_TRUE);

    lock(&(task->mutex));
    set_task_state(task, ATMI_EXECUTED);
    unlock(&(task->mutex));
    // after predecessor is done, decrement all successor's dependency count.
    // If count reaches zero, then add them to a 'ready' task list. Next,
    // dispatch all ready tasks in a round-robin manner to the available
    // GPU/CPU queues.
    // decrement reference count of its dependencies; add those with ref count = 0 to a
    // “ready” list
    atl_task_vector_t &deps = task->and_successors;
    DEBUG_PRINT("Deps list of %lu [%lu]: ", task->id, deps.size());
    atl_task_vector_t temp_list;
    for(atl_task_vector_t::iterator it = deps.begin();
            it != deps.end(); it++) {
        // FIXME: should we be grabbing a lock on each successor before
        // decrementing their predecessor count? Currently, it may not be
        // required because there is only one callback thread, but what if there
        // were more?
        lock(&((*it)->mutex));
        DEBUG_PRINT(" %lu(%d) ", (*it)->id, (*it)->num_predecessors);
        (*it)->num_predecessors--;
        if((*it)->num_predecessors == 0) {
            // add to ready list
            temp_list.push_back(*it);
        }
        unlock(&((*it)->mutex));
    }
    std::set<pthread_mutex_t *> mutexes;
    // release the kernarg segment back to the kernarg pool
    atl_kernel_t *kernel = task->kernel;
    atl_kernel_impl_t *kernel_impl = NULL;
    if(kernel) {
        kernel_impl = get_kernel_impl(kernel, task->kernel_id);
        mutexes.insert(&(kernel_impl->mutex));
    }
    mutexes.insert(&mutex_readyq_);
    lock_set(mutexes);
    DEBUG_PRINT("\n");
    for(atl_task_vector_t::iterator it = temp_list.begin();
            it != temp_list.end(); it++) {
        // FIXME: if groupable task, push it in the right stream
        // else push it to readyqueue
        ReadyTaskQueue.push(*it);
    }
    // release your own signal to the pool
    FreeSignalPool.push(task->signal);
    DEBUG_PRINT("Freeing Kernarg Segment Id: %d\n", task->kernarg_region_index);
    // release your kernarg region to the pool
    if(kernel) kernel_impl->free_kernarg_segments.push(task->kernarg_region_index);
    unlock_set(mutexes);
    DEBUG_PRINT("[Handle Signal %lu ] Free Signal Pool Size: %lu; Ready Task Queue Size: %lu\n", task->id, FreeSignalPool.size(), ReadyTaskQueue.size());
    // dispatch from ready queue if any task exists
    lock(&(task->mutex));
    set_task_metrics(task);
    set_task_state(task, ATMI_COMPLETED);
    unlock(&(task->mutex));
    do_progress(task->stream_obj);
    //dispatch_ready_task_for_free_signal();
    //dispatch_ready_task_or_release_signal(task);
}

void handle_signal_barrier_pkt(atl_task_t *task) {
    // tasks without atmi_task handle should not be added to callbacks anyway
    assert(task->groupable != ATMI_TRUE);
    lock(&(task->mutex));
    set_task_state(task, ATMI_EXECUTED);
    unlock(&(task->mutex));
    std::set<pthread_mutex_t *> mutexes;
    // release the kernarg segment back to the kernarg pool
    atl_kernel_t *kernel = task->kernel;
    atl_kernel_impl_t *kernel_impl = NULL;
    if(kernel) {
        kernel_impl = get_kernel_impl(kernel, task->kernel_id);
        mutexes.insert(&(kernel_impl->mutex));
    }
    mutexes.insert(&(task->mutex));
    mutexes.insert(&mutex_readyq_);
    atl_task_vector_t &requires = task->and_predecessors;
    for(int idx = 0; idx < requires.size(); idx++) {
        mutexes.insert(&(requires[idx]->mutex));
    }

    lock_set(mutexes);

    DEBUG_PRINT("Freeing Kernarg Segment Id: %d\n", task->kernarg_region_index);
    if(kernel) kernel_impl->free_kernarg_segments.push(task->kernarg_region_index);

    DEBUG_PRINT("Task %lu completed\n", task->id);

    // decrement each predecessor's num_successors
    DEBUG_PRINT("Requires list of %lu [%lu]: ", task->id, requires.size());
    std::vector<hsa_signal_t> temp_list;
    temp_list.clear();
    // if num_successors == 0 then we can reuse their signal.
    for(atl_task_vector_t::iterator it = requires.begin();
            it != requires.end(); it++) {
        assert((*it)->state >= ATMI_DISPATCHED);
        (*it)->num_successors--;
        if((*it)->state >= ATMI_EXECUTED && (*it)->num_successors == 0) {
            // release signal because this predecessor is done waiting for
            temp_list.push_back((*it)->signal);
        }
    }
    if(task->groupable != ATMI_TRUE) {
        if(task->num_successors == 0) {
            temp_list.push_back(task->signal);
        }
    }
    for(std::vector<hsa_signal_t>::iterator it = task->barrier_signals.begin();
            it != task->barrier_signals.end(); it++) {
        //hsa_signal_store_relaxed((*it), 1);
        FreeSignalPool.push(*it);
        DEBUG_PRINT("[Handle Barrier_Signal %lu ] Free Signal Pool Size: %lu; Ready Task Queue Size: %lu\n", task->id, FreeSignalPool.size(), ReadyTaskQueue.size());
    }
    clear_container(task->barrier_signals);
    for(std::vector<hsa_signal_t>::iterator it = temp_list.begin();
            it != temp_list.end(); it++) {
        //hsa_signal_store_relaxed((*it), 1);
        FreeSignalPool.push(*it);
        DEBUG_PRINT("[Handle Signal %lu ] Free Signal Pool Size: %lu; Ready Task Queue Size: %lu\n", task->id, FreeSignalPool.size(), ReadyTaskQueue.size());
    }
    set_task_metrics(task);
    set_task_state(task, ATMI_COMPLETED);
    unlock_set(mutexes);

    do_progress(task->stream_obj);
    //dispatch_ready_task_for_free_signal();
}

bool handle_signal(hsa_signal_value_t value, void *arg) {
    //HandleSignalInvokeTimer.Stop();
    #if 1
    static bool is_called = false;
    if(!is_called) {
        set_thread_affinity(1);
        /*
        int policy;
        struct sched_param param;
        pthread_getschedparam(pthread_self(), &policy, &param);
        param.sched_priority = sched_get_priority_max(SCHED_RR);
        printf("Setting Priority Policy for %d: %d\n", SCHED_RR, param.sched_priority);
        pthread_setschedparam(pthread_self(), SCHED_RR, &param);
        */
        is_called = true;
    }
    #endif
    HandleSignalTimer.Start();
    atl_task_t *task = (atl_task_t *)arg;
    DEBUG_PRINT("Handle signal from task %lu\n", task->id);

    // process printf buffer
    {
      atl_kernel_impl_t *kernel_impl = NULL;
      if (task_process_fini_buffer) {
        if(task->kernel) {
          kernel_impl = get_kernel_impl(task->kernel, task->kernel_id);
          //printf("Task Id: %lu, kernel name: %s\n", task->id, kernel_impl->kernel_name.c_str());
          char *kargs = (char *)(task->kernarg_region);
          if(task->type == ATL_KERNEL_EXECUTION &&
              task->devtype == ATMI_DEVTYPE_GPU &&
              kernel_impl->kernel_type == AMDGCN) {
            atmi_implicit_args_t *impl_args = (atmi_implicit_args_t *)(kargs + (task->kernarg_region_size - sizeof(atmi_implicit_args_t)));

            (*task_process_fini_buffer)((void *)impl_args->pipe_ptr, MAX_PIPE_SIZE);

          }
        }
      }
    }

    //static int counter = 0;
    if(task->stream_obj->ordered) {
        lock(&(task->stream_obj->group_mutex));
        // does it matter which task was enqueued first, as long as we are
        // popping one task for every push?
        if(!task->stream_obj->running_ordered_tasks.empty())
            task->stream_obj->running_ordered_tasks.pop_front();
        unlock(&(task->stream_obj->group_mutex));
    }
    if(g_dep_sync_type == ATL_SYNC_CALLBACK) {
        handle_signal_callback(task);
    }
    else if(g_dep_sync_type == ATL_SYNC_BARRIER_PKT) {
        handle_signal_barrier_pkt(task);
    }
    HandleSignalTimer.Stop();
    //HandleSignalInvokeTimer.Start();
    return false;
}

#if 0
void dispatch_ready_task_for_free_signal() {
    atl_task_t *ready_task = NULL;
    hsa_signal_t free_signal;
    bool should_dispatch = false;
    bool should_register_callback = true;

    // if there is no deadlock then at least one task in ready
    // queue should be available for dispatch
    size_t queue_sz = 0;
    lock(&mutex_readyq_);
    queue_sz = ReadyTaskQueue.size();
    unlock(&mutex_readyq_);
    size_t iterations = 0;
    if(queue_sz > 0) {
        do {
            ready_task = NULL;
            lock(&mutex_readyq_);
            if(!ReadyTaskQueue.empty()) {
                ready_task = ReadyTaskQueue.front();
                ReadyTaskQueue.pop();
            }
            unlock(&mutex_readyq_);

            if(ready_task)
                should_dispatch = try_dispatch(ready_task, NULL, ATMI_FALSE);
            #if 0
            if(ready_task) {
                should_dispatch = try_dispatch_barrier_pkt(ready_task, NULL);
                /* if cannot be dispatched the task will be automatically be added to
                 * the ready queue */
                should_register_callback = ((ready_task->groupable != ATMI_TRUE) ||
                        (ready_task->groupable == ATMI_TRUE && !(ready_task->and_predecessors.empty())));
            }
            if(should_dispatch) {
                if(ready_task->atmi_task) {
                    // FIXME: set a lookup table for dynamic parallelism kernels
                    // perhaps?
                    ready_task->atmi_task->handle = (void *)(&(ready_task->signal));
                }
                DEBUG_PRINT("Callback dispatching next task %p (%lu)\n", ready_task, ready_task->id);
                callback_dispatch++;
                direct_dispatch++;
                dispatch_task(ready_task);
                if(should_register_callback) {
                    hsa_status_t err = hsa_amd_signal_async_handler(ready_task->signal,
                            HSA_SIGNAL_CONDITION_EQ, 0,
                            handle_signal, (void *)ready_task);
                    ErrorCheck(Creating signal handler, err);
                }
            }
            #endif
            iterations++;
        } while(!should_dispatch && iterations < queue_sz);
    }
}
#endif

#if 0
void dispatch_ready_task_or_release_signal(atl_task_t *task) {
    hsa_signal_t signal = task->signal;
    bool should_dispatch = false;
    bool should_register_callback = false;
    atl_task_vector_t ready_tasks;
    clear_container(ready_tasks);
    lock(&mutex_readyq_);
    // take *any* task with ref count = 0, which means it is ready to be dispatched
    while(!ReadyTaskQueue.empty()) {
        atl_task_t *ready_task = ReadyTaskQueue.front();
        ready_tasks.push_back(ready_task);
        ReadyTaskQueue.pop();
        if(ready_task->groupable != ATMI_TRUE)
            break;
    }
    unlock(&mutex_readyq_);

    DEBUG_PRINT("[Handle Signal2] Free Signal Pool Size: %lu; Ready Task Queue Size: %lu\n", FreeSignalPool.size(), ReadyTaskQueue.size());
    // set signal to wait for 1 ready task
    //hsa_signal_store_relaxed(signal, 1);
    for(atl_task_vector_t::iterator it = ready_tasks.begin();
            it!= ready_tasks.end(); it++) {
        atl_task_t *ready_task = *it;
        should_dispatch = try_dispatch_callback(ready_task, NULL);
        should_register_callback = (ready_task->groupable != ATMI_TRUE);
        if(should_dispatch) {
            if(ready_task->atmi_task) {
                // FIXME: set a lookup table for dynamic parallelism kernels
                // perhaps?
                ready_task->atmi_task->handle = (void *)(&(ready_task->signal));
            }
            callback_dispatch++;
            dispatch_task(ready_task);
            if(should_register_callback) {
                hsa_status_t err = hsa_amd_signal_async_handler(ready_task->signal,
                        HSA_SIGNAL_CONDITION_EQ, 0,
                        handle_signal, (void *)ready_task);
                ErrorCheck(Creating signal handler, err);
            }
        }
    }
}
#endif

bool handle_group_signal(hsa_signal_value_t value, void *arg) {
    //HandleSignalInvokeTimer.Stop();
#if 1
    static bool is_called = false;
    if(!is_called) {
        set_thread_affinity(1);
        /*
           int policy;
           struct sched_param param;
           pthread_getschedparam(pthread_self(), &policy, &param);
           param.sched_priority = sched_get_priority_max(SCHED_RR);
           printf("Setting Priority Policy for %d: %d\n", SCHED_RR, param.sched_priority);
           pthread_setschedparam(pthread_self(), SCHED_RR, &param);
           */
        is_called = true;
    }
#endif
    HandleSignalTimer.Start();
    atmi_task_group_table_t *task_group = (atmi_task_group_table_t *)arg;
    // TODO: what if the group task has dependents? this is not clear as of now.
    // we need to define dependencies between tasks and task groups better...

    // release resources
    lock(&(task_group->group_mutex));
    hsa_signal_wait_acquire(task_group->group_signal, HSA_SIGNAL_CONDITION_EQ, 0, UINT64_MAX, ATMI_WAIT_STATE);

    std::vector<atl_task_t *> group_tasks = task_group->running_groupable_tasks;
    task_group->running_groupable_tasks.clear();
    if(task_group->ordered) {
        DEBUG_PRINT("Handling group signal callback for task group: %p size: %lu\n", task_group, group_tasks.size());
        for(int t = 0; t < group_tasks.size(); t++) {
            // does it matter which task was enqueued first, as long as we are
            // popping one task for every push?
            if(!task_group->running_ordered_tasks.empty()) {
                DEBUG_PRINT("Removing task %lu with state: %d\n",
                        task_group->running_ordered_tasks.front()->id,
                        task_group->running_ordered_tasks.front()->state.load()
                        );
                task_group->running_ordered_tasks.pop_front();
            }
        }
    }
    task_group->callback_started.clear();
    unlock(&(task_group->group_mutex));

    std::set<pthread_mutex_t *> mutexes;
    for(std::vector<atl_task_t *>::iterator it = group_tasks.begin();
            it != group_tasks.end(); it++) {
        atl_task_t *task = *it;
        mutexes.insert(&(task->mutex));
        atl_kernel_t *kernel = task->kernel;
        atl_kernel_impl_t *kernel_impl = NULL;
        if(kernel) {
            kernel_impl = get_kernel_impl(kernel, task->kernel_id);
            mutexes.insert(&(kernel_impl->mutex));

            // process printf buffer
            {
              atl_kernel_impl_t *kernel_impl = NULL;
              if (task_process_fini_buffer) {
                if(task->kernel) {
                  kernel_impl = get_kernel_impl(task->kernel, task->kernel_id);
                  //printf("Task Id: %lu, kernel name: %s\n", task->id, kernel_impl->kernel_name.c_str());
                  char *kargs = (char *)(task->kernarg_region);
                  if(task->type == ATL_KERNEL_EXECUTION &&
                      task->devtype == ATMI_DEVTYPE_GPU &&
                      kernel_impl->kernel_type == AMDGCN) {
                    atmi_implicit_args_t *impl_args = (atmi_implicit_args_t *)(kargs + (task->kernarg_region_size - sizeof(atmi_implicit_args_t)));
                    (*task_process_fini_buffer)((void *)impl_args->pipe_ptr, MAX_PIPE_SIZE);
                  }
                }
              }
            }

        }
        if(g_dep_sync_type == ATL_SYNC_BARRIER_PKT) {
            atl_task_vector_t &requires = task->and_predecessors;
            for(int idx = 0; idx < requires.size(); idx++) {
                mutexes.insert(&(requires[idx]->mutex));
            }
        }
    }
    mutexes.insert(&mutex_readyq_);
    lock_set(mutexes);
    for(std::vector<atl_task_t *>::iterator itg = group_tasks.begin();
            itg != group_tasks.end(); itg++) {
        atl_task_t *task = *itg;
        atl_kernel_t *kernel = task->kernel;
        atl_kernel_impl_t *kernel_impl = NULL;
        if(kernel) {
            kernel_impl = get_kernel_impl(kernel, task->kernel_id);
            kernel_impl->free_kernarg_segments.push(task->kernarg_region_index);
        }
        if(g_dep_sync_type == ATL_SYNC_BARRIER_PKT) {
            std::vector<hsa_signal_t> temp_list;
            temp_list.clear();
            // if num_successors == 0 then we can reuse their signal.
            atl_task_vector_t &requires = task->and_predecessors;
            for(atl_task_vector_t::iterator it = requires.begin();
                    it != requires.end(); it++) {
                assert((*it)->state >= ATMI_DISPATCHED);
                (*it)->num_successors--;
                if((*it)->state >= ATMI_EXECUTED && (*it)->num_successors == 0) {
                    // release signal because this predecessor is done waiting for
                    temp_list.push_back((*it)->signal);
                }
            }
            for(std::vector<hsa_signal_t>::iterator it = task->barrier_signals.begin();
                    it != task->barrier_signals.end(); it++) {
                //hsa_signal_store_relaxed((*it), 1);
                FreeSignalPool.push(*it);
                DEBUG_PRINT("[Handle Barrier_Signal %lu ] Free Signal Pool Size: %lu; Ready Task Queue Size: %lu\n", task->id, FreeSignalPool.size(), ReadyTaskQueue.size());
            }
            clear_container(task->barrier_signals);
            for(std::vector<hsa_signal_t>::iterator it = temp_list.begin();
                    it != temp_list.end(); it++) {
                //hsa_signal_store_relaxed((*it), 1);
                FreeSignalPool.push(*it);
                DEBUG_PRINT("[Handle Signal %lu ] Free Signal Pool Size: %lu; Ready Task Queue Size: %lu\n", task->id, FreeSignalPool.size(), ReadyTaskQueue.size());
            }
        }
        else if(g_dep_sync_type == ATL_SYNC_CALLBACK) {
            // TODO: we dont know how to specify dependencies between task groups and
            // individual tasks yet. once we figure that out, we need to include
            // logic here to push the successor tasks to the ready task queue.
        }
        DEBUG_PRINT("Completed task %lu in group\n", task->id);
        set_task_metrics(task);
        set_task_state(task, ATMI_COMPLETED);
    }
    unlock_set(mutexes);

    // make progress on all other ready tasks
    do_progress(task_group, group_tasks.size());
    DEBUG_PRINT("Releasing %lu tasks from %p task group\n", group_tasks.size(), task_group);
    task_group->task_count -= group_tasks.size();
    if(g_dep_sync_type == ATL_SYNC_CALLBACK) {
        if(!task_group->task_count.load()) {
            // after predecessor is done, decrement all successor's dependency count.
            // If count reaches zero, then add them to a 'ready' task list. Next,
            // dispatch all ready tasks in a round-robin manner to the available
            // GPU/CPU queues.
            // decrement reference count of its dependencies; add those with ref count = 0 to a
            // “ready” list
            lock(&(task_group->group_mutex));
            atl_task_vector_t deps = task_group->and_successors;
            task_group->and_successors.clear();
            unlock(&(task_group->group_mutex));
            DEBUG_PRINT("Deps list of %p [%lu]: ", task_group, deps.size());
            atl_task_vector_t temp_list;
            for(atl_task_vector_t::iterator it = deps.begin();
                    it != deps.end(); it++) {
                // FIXME: should we be grabbing a lock on each successor before
                // decrementing their predecessor count? Currently, it may not be
                // required because there is only one callback thread, but what if there
                // were more?
                lock(&((*it)->mutex));
                DEBUG_PRINT(" %lu(%d) ", (*it)->id, (*it)->num_predecessors);
                (*it)->num_predecessors--;
                if((*it)->num_predecessors == 0) {
                    // add to ready list
                    temp_list.push_back(*it);
                }
                unlock(&((*it)->mutex));
            }
            lock(&mutex_readyq_);
            for(atl_task_vector_t::iterator it = temp_list.begin();
                    it != temp_list.end(); it++) {
                // FIXME: if groupable task, push it in the right stream
                // else push it to readyqueue
                ReadyTaskQueue.push(*it);
            }
            unlock(&mutex_readyq_);

            if(!temp_list.empty()) do_progress(task_group);
        }
    }
    HandleSignalTimer.Stop();
    //HandleSignalInvokeTimer.Start();
    // return true because we want the callback function to always be
    // 'registered' for any more incoming tasks belonging to this task group
    DEBUG_PRINT("Done handle_group_signal\n");
    return false;
}

void do_progress_on_task_group(atmi_task_group_table_t *task_group) {
    bool should_dispatch = false;
    atl_task_t *ready_task = NULL;
    do {
        ready_task = NULL;
        should_dispatch = false;
        lock(&(task_group->group_mutex));
        //navigate through the queue to find the 1st non-ready task and try
        //dispatching it
        for(int i = 0; i < task_group->running_ordered_tasks.size(); i++) {
            //if(task_group->running_ordered_tasks[i]->state == ATMI_COMPLETED) {
            //    task_group->running_ordered_tasks.erase(task_group->running_ordered_tasks.begin() + i);
            //}
            if(task_group->running_ordered_tasks[i]->state >= ATMI_READY) {
                // some other thread is looking at dispatching a task from
                // this ordered queue, so give up doing progress on this
                // thread because tasks have to be processed in order by definition
                ready_task = NULL;
                break;
            }
            if(task_group->running_ordered_tasks[i]->state < ATMI_READY) {
                ready_task = task_group->running_ordered_tasks[i];
                break;
            }
        }
        unlock(&(task_group->group_mutex));
        if(ready_task) {
            should_dispatch = try_dispatch(ready_task, NULL, ATMI_FALSE);
        }
    } while(should_dispatch);
}

void do_progress(atmi_task_group_table_t *task_group, int progress_count) {
    bool should_dispatch = false;
    bool should_register_callback = false;
#if 1
    #if 0
    atl_task_vector_t ready_tasks;
    clear_container(ready_tasks);
    lock(&mutex_readyq_);
    // take *any* task with ref count = 0, which means it is ready to be dispatched
    while(!ReadyTaskQueue.empty()) {
        atl_task_t *ready_task = ReadyTaskQueue.front();
        ready_tasks.push_back(ready_task);
        ReadyTaskQueue.pop();
        //if(ready_task->groupable != ATMI_TRUE)
        //    break;
    }
    unlock(&mutex_readyq_);

    DEBUG_PRINT("[Handle Signal2] Free Signal Pool Size: %lu; Ready Task Queue Size: %lu\n", FreeSignalPool.size(), ReadyTaskQueue.size());

    for(atl_task_vector_t::iterator it = ready_tasks.begin();
            it!= ready_tasks.end(); it++) {
        try_dispatch(*it, NULL, ATMI_FALSE);
    }
    #else
    if(task_group && task_group->ordered) {
        do_progress_on_task_group(task_group);
    }
    else {
        size_t queue_sz = 0;
        lock(&mutex_readyq_);
        queue_sz = ReadyTaskQueue.size();
        unlock(&mutex_readyq_);
        int dispatched_tasks = 0;
        for(int i = 0; i < queue_sz; i++) {
            atl_task_t *ready_task = NULL;
            lock(&mutex_readyq_);
            if(!ReadyTaskQueue.empty()) {
                ready_task = ReadyTaskQueue.front();
                ReadyTaskQueue.pop();
            }
            unlock(&mutex_readyq_);

            if(ready_task) {
                should_dispatch = try_dispatch(ready_task, NULL, ATMI_FALSE);
                dispatched_tasks++;
            }
            if(should_dispatch && dispatched_tasks >= progress_count) break;
        }
    }
    #endif
#else
    atl_task_t *ready_task = NULL;
    lock(&mutex_readyq_);
    if(!ReadyTaskQueue.empty()) {
        ready_task = ReadyTaskQueue.front();
        ReadyTaskQueue.pop();
    }
    printf("[Handle Signal2] Free Signal Pool Size: %lu; Ready Task Queue Size: %lu\n", FreeSignalPool.size(), ReadyTaskQueue.size());
    unlock(&mutex_readyq_);
    if(ready_task) try_dispatch(ready_task, NULL, ATMI_FALSE);
#endif
}

void *acquire_kernarg_segment(atl_kernel_impl_t *impl, int *segment_id) {
    uint32_t kernel_segment_size = impl->kernarg_segment_size;
    void * ret_address = NULL;
    int free_idx = -1;
    lock(&(impl->mutex));
    if(!(impl->free_kernarg_segments.empty())) {
        free_idx = impl->free_kernarg_segments.front();
        DEBUG_PRINT("Acquiring Kernarg Segment Id: %d\n", free_idx);
        ret_address = (void *)((char *)impl->kernarg_region +
                            (free_idx * kernel_segment_size));
        impl->free_kernarg_segments.pop();
    }
    else {
        fprintf(stderr, "No free kernarg segments. Increase MAX_NUM_KERNELS and recompile.\n");
    }
    unlock(&(impl->mutex));
    *segment_id = free_idx;
    return ret_address;
    // check if pool is empty by comparing the read and write indexes
    // if pool is empty
    //      extend existing pool
    //      if pool size is beyond a threshold max throw error
    // fi
    // get a free pool object and return
    //
    //
    // on handle signal, just return the free pool object?
}

atmi_status_t dispatch_task(atl_task_t *task) {
    //DEBUG_PRINT("GPU Place Info: %d, %lx %lx\n", lparm->place.node_id, lparm->place.cpu_set, lparm->place.gpu_set);

    static std::atomic<int> counter(0);
    if(task->type == ATL_DATA_MOVEMENT) {
        return dispatch_data_movement(task, task->data_dest_ptr, task->data_src_ptr,
                                      task->data_size);
    }
    else {
        TryDispatchTimer.Start();
        atmi_task_group_table_t *stream_obj = task->stream_obj;
        /* get this stream's HSA queue (could be dynamically mapped or round robin
         * if it is an unordered stream */
        // FIXME: round robin for now, but may use some other load balancing algo
        // enqueue task's packet to that queue

        int proc_id = task->place.device_id;
        if(proc_id == -1) {
            // user is asking runtime to pick a device
            // TODO: best device of this type? pick 0 for now
            proc_id = 0;
        }
        hsa_queue_t* this_Q = NULL;
        if(task->devtype == ATMI_DEVTYPE_GPU)
            this_Q = acquire_and_set_next_gpu_queue(stream_obj, task->place);
        else if(task->devtype == ATMI_DEVTYPE_CPU)
            this_Q = acquire_and_set_next_cpu_queue(stream_obj, task->place);
        if(!this_Q) return ATMI_STATUS_ERROR;

        /* if stream is ordered and the devtype changed for this task,
         * enqueue a barrier to wait for previous device to complete */
        //check_change_in_device_type(task, stream_obj, this_Q, task->devtype);

        if(g_dep_sync_type == ATL_SYNC_BARRIER_PKT) {
            /* For dependent child tasks, add dependent parent kernels to barriers.  */
            //DEBUG_PRINT("Pif requires %d tasks\n", lparm->predecessor.size());
            if ( task->and_predecessors.size() > 0) {
                int val = 0;
                DEBUG_PRINT("(");
                for(size_t count = 0; count < task->and_predecessors.size(); count++) {
                    if(task->and_predecessors[count]->state < ATMI_DISPATCHED) val++;
                    assert(task->and_predecessors[count]->state >= ATMI_DISPATCHED);
                    DEBUG_PRINT("%lu ", task->and_predecessors[count]->id);
                }
                DEBUG_PRINT(")\n");
                if(val > 0) DEBUG_PRINT("Task[%lu] has %d not-dispatched predecessor tasks\n", task->id, val);
                enqueue_barrier(task, this_Q, task->and_predecessors.size(), &(task->and_predecessors[0]), SNK_NOWAIT, SNK_AND, task->devtype);
            }
            DEBUG_PRINT("%lu\n", task->id);
            /*else if ( lparm->num_needs_any > 0) {
              std::vector<atl_task_t *> needs_any(lparm->num_needs_any);
              for(int idx = 0; idx < lparm->num_needs_any; idx++) {
              needs_any[idx] = PublicTaskMap[lparm->needs_any[idx]];
              }
              enqueue_barrier(task, this_Q, lparm->num_needs_any, &needs_any[0], SNK_NOWAIT, SNK_OR, task->devtype);
              }*/
        }
        int ndim = -1;
        if(task->gridDim[2] > 1)
            ndim = 3;
        else if(task->gridDim[1] > 1)
            ndim = 2;
        else
            ndim = 1;
        if(task->devtype == ATMI_DEVTYPE_GPU) {
            /*  Obtain the current queue write index. increases with each call to kernel  */
            //uint64_t index = hsa_queue_add_write_index_relaxed(this_Q, 1);
            // Atomically request a new packet ID.
            uint64_t index = hsa_queue_add_write_index_relaxed(this_Q, 1);
            // Wait until the queue is not full before writing the packet
            DEBUG_PRINT("Queue %p ordered? %d index: %lu\n", this_Q, stream_obj->ordered, index);
            while(index - hsa_queue_load_read_index_acquire(this_Q) >= this_Q->size);
            atl_kernel_impl_t *kernel_impl = get_kernel_impl(task->kernel, task->kernel_id);
            /* Find the queue index address to write the packet info into.  */
            const uint32_t queueMask = this_Q->size - 1;
            hsa_kernel_dispatch_packet_t* this_aql = &(((hsa_kernel_dispatch_packet_t*)(this_Q->base_address))[index&queueMask]);
            //this_aql->header = create_header(HSA_PACKET_TYPE_INVALID, ATMI_FALSE);
            //memset(this_aql, 0, sizeof(hsa_kernel_dispatch_packet_t));
            /*  FIXME: We need to check for queue overflow here. */
            //SignalAddTimer.Start();
            if(task->groupable == ATMI_TRUE) {
                lock(&(stream_obj->group_mutex));
                hsa_signal_add_acq_rel(task->signal, 1);
                stream_obj->running_groupable_tasks.push_back(task);
                unlock(&(stream_obj->group_mutex));
            }
            else
                hsa_signal_add_acq_rel(task->signal, 1);
            //hsa_signal_store_relaxed(task->signal, 1);
            //SignalAddTimer.Stop();
            this_aql->completion_signal = task->signal;

            /* pass this task handle to the kernel as an argument */
#if 0
            struct kernel_args_struct {
                /*uint64_t arg0;
                  uint64_t arg1;
                  uint64_t arg2;
                  uint64_t arg3;
                  uint64_t arg4;
                  uint64_t arg5;*/
                /* other fields no needed to set task handle? */
            } __attribute__((aligned(16)));
            struct kernel_args_struct *kargs = (struct kernel_args_struct *)(task->kernarg_region);
#endif
            char *kargs = (char *)(task->kernarg_region);
            if(kernel_impl->kernel_type == BRIG) {
                const int dummy_arg_count = 6;
                kargs += (dummy_arg_count * sizeof(uint64_t));
            }
            else if(kernel_impl->kernel_type == AMDGCN) {
                atmi_implicit_args_t *impl_args = (atmi_implicit_args_t *)(kargs + (task->kernarg_region_size - sizeof(atmi_implicit_args_t)));
                impl_args->offset_x = 0;
                impl_args->offset_y = 0;
                impl_args->offset_z = 0;
                // char *pipe_ptr = impl_args->pipe_ptr;
            }

            // initialize printf buffer
            {
              atl_kernel_impl_t *kernel_impl = NULL;
              if (task_process_init_buffer) {
                if(task->kernel) {
                  kernel_impl = get_kernel_impl(task->kernel, task->kernel_id);
                  //printf("Task Id: %lu, kernel name: %s\n", task->id, kernel_impl->kernel_name.c_str());
                  char *kargs = (char *)(task->kernarg_region);
                  if(task->type == ATL_KERNEL_EXECUTION &&
                      task->devtype == ATMI_DEVTYPE_GPU &&
                      kernel_impl->kernel_type == AMDGCN) {
                    atmi_implicit_args_t *impl_args = (atmi_implicit_args_t *)(kargs + (task->kernarg_region_size - sizeof(atmi_implicit_args_t)));

                    // initalize printf buffer
                    (*task_process_init_buffer)((void *)impl_args->pipe_ptr, MAX_PIPE_SIZE);
                  }
                }
              }
            }

            /*  Process task values */
            /*  this_aql.dimensions=(uint16_t) ndim; */
            this_aql->setup  |= (uint16_t) ndim << HSA_KERNEL_DISPATCH_PACKET_SETUP_DIMENSIONS;
            this_aql->grid_size_x=task->gridDim[0];
            this_aql->workgroup_size_x=task->groupDim[0];
            if (ndim>1) {
                this_aql->grid_size_y=task->gridDim[1];
                this_aql->workgroup_size_y=task->groupDim[1];
            } else {
                this_aql->grid_size_y=1;
                this_aql->workgroup_size_y=1;
            }
            if (ndim>2) {
                this_aql->grid_size_z=task->gridDim[2];
                this_aql->workgroup_size_z=task->groupDim[2];
            } else {
                this_aql->grid_size_z=1;
                this_aql->workgroup_size_z=1;
            }

            /*  Bind kernel argument buffer to the aql packet.  */
            this_aql->kernarg_address = task->kernarg_region;
            this_aql->kernel_object = kernel_impl->kernel_objects[proc_id];
            this_aql->private_segment_size = kernel_impl->private_segment_sizes[proc_id];
            this_aql->group_segment_size = kernel_impl->group_segment_sizes[proc_id];

            this_aql->reserved2 = task->id;
            set_task_state(task, ATMI_DISPATCHED);
            /*  Prepare and set the packet header */
            this_aql->header = create_header(HSA_PACKET_TYPE_KERNEL_DISPATCH, stream_obj->ordered);
            /* Increment write index and ring doorbell to dispatch the kernel.  */
            //hsa_queue_store_write_index_relaxed(this_Q, index+1);
            hsa_signal_store_relaxed(this_Q->doorbell_signal, index);
        }
        else if(task->devtype == ATMI_DEVTYPE_CPU) {
            std::vector<hsa_queue_t *> this_queues = get_cpu_queues(task->place);
            int q_count = this_queues.size();
            int thread_count = task->gridDim[0] * task->gridDim[1] * task->gridDim[2];
            if(thread_count == 0) {
                fprintf(stderr, "WARNING: one of the dimensionsions is set to 0 threads. \
                         Choosing 1 thread by default. \n");
                thread_count = 1;
            }
            if(thread_count == 1) {
                int q;
                for(q = 0; q < q_count; q++) {
                    if(this_queues[q] == this_Q)
                        break;
                }
                hsa_queue_t *tmp = this_queues[0];
                this_queues[0] = this_queues[q];
                this_queues[q] = tmp;
            }
            struct timespec dispatch_time;
            clock_gettime(CLOCK_MONOTONIC_RAW,&dispatch_time);
            /* this "virtual" task encompasses thread_count number of ATMI CPU tasks performing
             * data parallel SPMD style of processing
             */
            if(task->groupable == ATMI_TRUE) {
                lock(&(stream_obj->group_mutex));
                hsa_signal_add_acq_rel(task->signal, thread_count);
                stream_obj->running_groupable_tasks.push_back(task);
                unlock(&(stream_obj->group_mutex));
            }
            else
                hsa_signal_add_acq_rel(task->signal, thread_count);
            for(int tid = 0; tid < thread_count; tid++) {
                hsa_queue_t *this_queue = this_queues[tid % q_count];
                /*  Obtain the current queue write index. increases with each call to kernel  */
                uint64_t index = hsa_queue_add_write_index_relaxed(this_queue, 1);
                while(index - hsa_queue_load_read_index_acquire(this_queue) >= this_queue->size);
                /* Find the queue index address to write the packet info into.  */
                const uint32_t queueMask = this_queue->size - 1;
                hsa_agent_dispatch_packet_t* this_aql = &(((hsa_agent_dispatch_packet_t*)(this_queue->base_address))[index&queueMask]);
                memset(this_aql, 0, sizeof(hsa_agent_dispatch_packet_t));
                /*  FIXME: We need to check for queue overflow here. Do we need
                 *  to do this for CPU agents too? */
                //SignalAddTimer.Start();
                //hsa_signal_store_relaxed(task->signal, 1);
                //SignalAddTimer.Stop();
                this_aql->completion_signal = task->signal;

                /* Set the type and return args.*/
                // FIXME FIXME FIXME: Use the hierarchical pif-kernel table to
                // choose the best kernel. Don't use a flat table structure
                this_aql->type = (uint16_t)get_kernel_index(task->kernel, task->kernel_id);
                /* FIXME: We are considering only void return types for now.*/
                //this_aql->return_address = NULL;
                /* Set function args */
                this_aql->arg[0] = (uint64_t) task->id;
                this_aql->arg[1] = (uint64_t) task->kernarg_region;
                this_aql->arg[2] = (uint64_t) task->kernel; // pass task handle to fill in metrics
                this_aql->arg[3] = tid; // tasks can query for current task ID

                /*  Prepare and set the packet header */
                /* FIXME: CPU tasks ignore barrier bit as of now. Change
                 * implementation? I think it doesn't matter because we are
                 * executing the subroutines one-by-one, so barrier bit is
                 * inconsequential.
                 */
                this_aql->header = create_header(HSA_PACKET_TYPE_AGENT_DISPATCH, stream_obj->ordered);

            }
            set_task_state(task, ATMI_DISPATCHED);
            /* Store dispatched time */
            if(task->profilable == ATMI_TRUE && task->atmi_task)
                task->atmi_task->profile.dispatch_time = get_nanosecs(context_init_time, dispatch_time);
            // FIXME: in the current logic, multiple CPU threads are round-robin
            // scheduled across queues from Q0. So, just ring the doorbell on only
            // the queues that are touched and leave the other queues alone
            int doorbell_count = (q_count < thread_count) ? q_count : thread_count;
            for(int q = 0; q < doorbell_count; q++) {
                /* fetch write index and ring doorbell on one lesser value
                 * (because the add index call will have incremented it already by
                 * 1 and dispatch the kernel.  */
                uint64_t index = hsa_queue_load_write_index_acquire(this_queues[q]) - 1;
                hsa_signal_store_relaxed(this_queues[q]->doorbell_signal, index);
                signal_worker(this_queues[q], PROCESS_PKT);
            }
        }
        counter++;
        TryDispatchTimer.Stop();
        DEBUG_PRINT("Task %lu (%d) Dispatched\n", task->id, task->devtype);
        return ATMI_STATUS_SUCCESS;
    }
}

void *try_grab_kernarg_region(atl_task_t *ret, int *free_idx) {
    atl_kernel_impl_t *kernel_impl = get_kernel_impl(ret->kernel, ret->kernel_id);
    uint32_t kernarg_segment_size = kernel_impl->kernarg_segment_size;
    /* acquire_kernarg_segment and copy args here before dispatch */
    void * ret_address = NULL;
    if(!(kernel_impl->free_kernarg_segments.empty())) {
        *free_idx = kernel_impl->free_kernarg_segments.front();
        DEBUG_PRINT("Acquiring Kernarg Segment Id: %p\n", free_idx);
        ret_address = (void *)((char *)kernel_impl->kernarg_region +
                ((*free_idx) * kernarg_segment_size));
        kernel_impl->free_kernarg_segments.pop();
    }
    return ret_address;
}

void set_kernarg_region(atl_task_t *ret, void **args) {
    char *thisKernargAddress = (char *)(ret->kernarg_region);
    if(ret->kernel->num_args && thisKernargAddress == NULL) {
        fprintf(stderr, "Unable to allocate/find free kernarg segment\n");
    }
    atl_kernel_impl_t *kernel_impl = get_kernel_impl(ret->kernel, ret->kernel_id);
    if(kernel_impl->kernel_type == BRIG) {
        if(ret->devtype == ATMI_DEVTYPE_GPU) {
            /* zero out all the dummy arguments */
            for(int dummy_idx = 0; dummy_idx < 3; dummy_idx++) {
                *(uint64_t *)thisKernargAddress = 0;
                thisKernargAddress += sizeof(uint64_t);
            }
            //*(uint64_t *)thisKernargAddress = (uint64_t)PifKlistMap[ret->kernel->pif_id];
            thisKernargAddress += sizeof(uint64_t);
            for(int dummy_idx = 0; dummy_idx < 2; dummy_idx++) {
                *(uint64_t *)thisKernargAddress = 0;
                thisKernargAddress += sizeof(uint64_t);
            }
        }
    }
    // Argument references will be copied to a contiguous memory region here
    // TODO: resolve all data affinities before copying, depending on
    // atmi_data_affinity_policy_t: ATMI_COPY, ATMI_NOCOPY
    atmi_place_t place = ret->place;
    for(int i = 0; i < ret->kernel->num_args; i++) {
        void *thisArgAddress = args[i];
#if 0
        void *thisArg = *(void **)thisArgAddress;
        atl_ptr_info_t info = get_ptr_info(thisArg);
        if(info.is_managed) {
            if(ret->affinity_policy == ATMI_DATA_COPY) {
                atmi_copy
            }
            else if(ret->affinity_policy == ATMI_DATA_NOCOPY) {
            }
        }
#endif
        memcpy(thisKernargAddress, thisArgAddress, ret->kernel->arg_sizes[i]);
        //hsa_memory_register(thisKernargAddress, ???
        DEBUG_PRINT("Arg[%d] = %p\n", i, *(void **)thisKernargAddress);
        thisKernargAddress += ret->kernel->arg_sizes[i];
    }
}

bool try_dispatch_barrier_pkt(atl_task_t *ret, void **args) {
    bool resources_available = true;
    bool should_dispatch = true;
    hsa_signal_t new_signal;

    std::set<pthread_mutex_t *> req_mutexes;
    std::vector<atl_task_t *> &temp_vecs = ret->predecessors;
    req_mutexes.clear();
    for(int idx = 0; idx < ret->predecessors.size(); idx++) {
        atl_task_t *pred_task = temp_vecs[idx];
        req_mutexes.insert((pthread_mutex_t *)&(pred_task->mutex));
    }
    req_mutexes.insert((pthread_mutex_t *)&(ret->mutex));
    req_mutexes.insert(&mutex_readyq_);
    if(ret->prev_ordered_task) req_mutexes.insert(&(ret->prev_ordered_task->mutex));
    atl_kernel_impl_t *kernel_impl = NULL;
    if(ret->kernel) {
        kernel_impl = get_kernel_impl(ret->kernel, ret->kernel_id);
        if(kernel_impl) req_mutexes.insert(&(kernel_impl->mutex));
    }
    atmi_task_group_table_t *stream_obj = ret->stream_obj;

    lock_set(req_mutexes);
    //std::cout << "[" << ret->id << "]Signals Before: " << FreeSignalPool.size() << std::endl;

    if(ret->state >= ATMI_READY) {
        // If someone else is trying to dispatch this task, give up
        unlock_set(req_mutexes);
        return false;
    }
    int required_tasks = 0;
    if(should_dispatch) {
        // find if all dependencies are satisfied
        int dep_count = 0;
        for(int idx = 0; idx < ret->predecessors.size(); idx++) {
            atl_task_t *pred_task = temp_vecs[idx];
            DEBUG_PRINT("Task %lu depends on %lu as %d th predecessor ",
                    ret->id, pred_task->id, idx);
            if(pred_task->state < ATMI_DISPATCHED) {
                /* still in ready queue and not received a signal */
                should_dispatch = false;
                DEBUG_PRINT("(waiting)\n");
                waiting_count++;
            }
            else {
                DEBUG_PRINT("(dispatched)\n");
            }
        }
        if(ret->prev_ordered_task) {
            DEBUG_PRINT("Task %lu depends on %lu as ordered predecessor ",
                    ret->id, ret->prev_ordered_task->id);
            if(ret->prev_ordered_task->state < ATMI_DISPATCHED) {
                should_dispatch = false;
                DEBUG_PRINT("(waiting)\n");
                waiting_count++;
            }
            else {
                DEBUG_PRINT("(dispatched)\n");
            }
        }
        for(int idx = 0; idx < ret->predecessors.size(); idx++) {
            atl_task_t *pred_task = temp_vecs[idx];
            if(pred_task->state /*.load(std::memory_order_seq_cst)*/ < ATMI_EXECUTED) {
                required_tasks++;
            }
        }
    }
    const int HSA_BARRIER_MAX_DEPENDENT_TASKS = 4;
    /* round up */
    int barrier_pkt_count = (required_tasks + HSA_BARRIER_MAX_DEPENDENT_TASKS - 1) / HSA_BARRIER_MAX_DEPENDENT_TASKS;

    if(should_dispatch) {
        if((ret->groupable != ATMI_TRUE && FreeSignalPool.size() < barrier_pkt_count + 1)
             || (ret->groupable == ATMI_TRUE && FreeSignalPool.size() < barrier_pkt_count)
             || (kernel_impl && kernel_impl->free_kernarg_segments.empty())) {
            should_dispatch = false;
            resources_available = false;
            ret->and_predecessors.clear();
            DEBUG_PRINT("Do not dispatch because (signals: %lu, kernels: %lu, ready tasks: %lu)\n", FreeSignalPool.size(),
                                                         kernel_impl->free_kernarg_segments.size(),
                                                         ReadyTaskQueue.size());
        }
    }

    if(should_dispatch) {
        if(ret->groupable != ATMI_TRUE) {
            // this is a task that uses individual signals (not stream-signals)
            // and we did not find a free signal, so just enqueue
            // for a later dispatch
            new_signal = FreeSignalPool.front();
            FreeSignalPool.pop();
            ret->signal = new_signal;
            for(int barrier_id = 0; barrier_id < barrier_pkt_count; barrier_id++) {
                new_signal = FreeSignalPool.front();
                FreeSignalPool.pop();
                ret->barrier_signals.push_back(new_signal);
            }
        }
        else {
            hsa_signal_t signal;
            get_stream_signal(stream_obj, &signal);
            ret->signal = signal;
            for(int barrier_id = 0; barrier_id < barrier_pkt_count; barrier_id++) {
                new_signal = FreeSignalPool.front();
                FreeSignalPool.pop();
                ret->barrier_signals.push_back(new_signal);
            }
        }
        if(ret->kernel) {
            // get kernarg resource
            uint32_t kernarg_segment_size = kernel_impl->kernarg_segment_size;
            int free_idx = kernel_impl->free_kernarg_segments.front();
            ret->kernarg_region_index = free_idx;
            DEBUG_PRINT("Acquiring Kernarg Segment Id: %d\n", free_idx);
            void *addr = (void *)((char *)kernel_impl->kernarg_region +
                    (free_idx * kernarg_segment_size));
            kernel_impl->free_kernarg_segments.pop();
            if(ret->kernarg_region != NULL) {
                // we had already created a memory region using malloc. Copy it
                // to the newly availed space
                size_t size_to_copy = ret->kernarg_region_size;
                if(ret->devtype == ATMI_DEVTYPE_GPU && kernel_impl->kernel_type == AMDGCN) {
                    // do not copy the implicit args from saved region
                    // they are to be set/reset during task dispatch
                    size_to_copy -= sizeof(atmi_implicit_args_t);
                }
                memcpy(addr, ret->kernarg_region, size_to_copy);
                // free existing region
                free(ret->kernarg_region);
                ret->kernarg_region = addr;
            }
            else {
                // first time allocation/assignment
                ret->kernarg_region = addr;
                set_kernarg_region(ret, args);
            }
        }

        for(int idx = 0; idx < ret->predecessors.size(); idx++) {
            atl_task_t *pred_task = temp_vecs[idx];
            if(pred_task->state /*.load(std::memory_order_seq_cst)*/ < ATMI_EXECUTED) {
                pred_task->num_successors++;
                DEBUG_PRINT("Task %p (%lu) adding %p (%lu) as successor\n",
                        pred_task, pred_task->id, ret, ret->id);
                ret->and_predecessors.push_back(pred_task);
            }
        }
        set_task_state(ret, ATMI_READY);
    }
    else {
        if(ret->kernel && ret->kernarg_region == NULL) {
            // first time allocation/assignment
            ret->kernarg_region = malloc(ret->kernarg_region_size);
            //ret->kernarg_region_copied = true;
            set_kernarg_region(ret, args);
        }
        //printf("[Not dispatching %lu] Free Signal Pool Size: %lu; Ready Task Queue Size: %lu\n", ret->id, FreeSignalPool.size(), ReadyTaskQueue.size());
        if(ret->stream_obj->ordered == false)
            ReadyTaskQueue.push(ret);
        max_ready_queue_sz++;
        set_task_state(ret, ATMI_INITIALIZED);
    }
    //std::cout << "[" << ret->id << "]Signals After (" << should_dispatch << "): " << FreeSignalPool.size() << std::endl;
    DEBUG_PRINT("[Try Dispatch] Free Signal Pool Size: %lu; Ready Task Queue Size: %lu\n", FreeSignalPool.size(), ReadyTaskQueue.size());
    unlock_set(req_mutexes);
    // try to dispatch if
    // a) you are using callbacks to resolve dependencies and all
    // your predecessors are done executing, OR
    // b) you are using barrier packets, in which case always try
    // to launch if you have a free signal at hand
    return should_dispatch;
}

bool try_dispatch_callback(atl_task_t *ret, void **args) {
    bool should_try_dispatch = true;
    bool resources_available = true;
    bool predecessors_complete = true;
    bool should_dispatch = false;

    std::set<pthread_mutex_t *> req_mutexes;
    std::vector<atl_task_t *> &temp_vecs = ret->predecessors;
    req_mutexes.clear();
    for(int idx = 0; idx < ret->predecessors.size(); idx++) {
        atl_task_t *pred_task = ret->predecessors[idx];
        req_mutexes.insert((pthread_mutex_t *)&(pred_task->mutex));
    }
    req_mutexes.insert((pthread_mutex_t *)&(ret->mutex));
    req_mutexes.insert(&mutex_readyq_);
    if(ret->prev_ordered_task) req_mutexes.insert(&(ret->prev_ordered_task->mutex));
    atmi_task_group_table_t *stream_obj = ret->stream_obj;
    req_mutexes.insert(&(stream_obj->group_mutex));
    atl_kernel_impl_t *kernel_impl = NULL;
    if(ret->kernel) {
        kernel_impl = get_kernel_impl(ret->kernel, ret->kernel_id);
        req_mutexes.insert(&(kernel_impl->mutex));
    }
    lock_set(req_mutexes);

    if(ret->state >= ATMI_READY) {
        // If someone else is trying to dispatch this task, give up
        unlock_set(req_mutexes);
        return false;
    }
    // do not add predecessor-successor link if task is already initialized using
    // create-activate pattern
    if(ret->state < ATMI_INITIALIZED && should_try_dispatch) {
        if(ret->predecessors.size() > 0) {
            // add to its predecessor's dependents list and return
            for(int idx = 0; idx < ret->predecessors.size(); idx++) {
                atl_task_t *pred_task = ret->predecessors[idx];
                DEBUG_PRINT("Task %p depends on %p as predecessor ",
                        ret, pred_task);
                if(pred_task->state /*.load(std::memory_order_seq_cst)*/ < ATMI_EXECUTED) {
                    should_try_dispatch = false;
                    predecessors_complete = false;
                    pred_task->and_successors.push_back(ret);
                    ret->num_predecessors++;
                    DEBUG_PRINT("(waiting)\n");
                    waiting_count++;
                }
                else {
                    DEBUG_PRINT("(completed)\n");
                }
            }
        }
        if(ret->pred_stream_objs.size() > 0) {
            // add to its predecessor's dependents list and return
            DEBUG_PRINT("Task %lu has %lu predecessor task groups\n", ret->id, ret->pred_stream_objs.size());
            for(int idx = 0; idx < ret->pred_stream_objs.size(); idx++) {
                atmi_task_group_table_t *pred_tg = ret->pred_stream_objs[idx];
                DEBUG_PRINT("Task %p depends on %p as predecessor task group ",
                        ret, pred_tg);
                if(pred_tg && pred_tg->task_count.load() > 0) {
                    // predecessor task group is still running, so add yourself to its successor list
                    should_try_dispatch = false;
                    predecessors_complete = false;
                    pred_tg->and_successors.push_back(ret);
                    ret->num_predecessors++;
                    DEBUG_PRINT("(waiting)\n");
                }
                else {
                    DEBUG_PRINT("(completed)\n");
                }
            }
        }
    }

    if(ret->state < ATMI_INITIALIZED && should_try_dispatch) {
        if(ret->prev_ordered_task) {
            DEBUG_PRINT("Task %lu depends on %lu as ordered predecessor ",
                    ret->id, ret->prev_ordered_task->id);
            if(ret->prev_ordered_task->state < ATMI_DISPATCHED) {
                should_try_dispatch = false;
                predecessors_complete = false;
                DEBUG_PRINT("(waiting)\n");
                waiting_count++;
            }
            else {
                DEBUG_PRINT("(dispatched)\n");
            }
        }

    }

    if(should_try_dispatch) {
        if((kernel_impl && kernel_impl->free_kernarg_segments.empty()) ||
            (ret->groupable == ATMI_FALSE && FreeSignalPool.empty())) {
            should_try_dispatch = false;
            resources_available = false;
        }
    }

    if(should_try_dispatch) {
        // try to dispatch if
        // a) you are using callbacks to resolve dependencies and all
        // your predecessors are done executing, OR
        // b) you are using barrier packets, in which case always try
        // to launch if you have a free signal at hand
        if(ret->groupable == ATMI_TRUE) {
            hsa_signal_t signal;
            get_stream_signal(stream_obj, &signal);
            ret->signal = signal;
        }
        else {
            // get a free signal
            hsa_signal_t new_signal = FreeSignalPool.front();
            ret->signal = new_signal;
            DEBUG_PRINT("Before pop Signal handle: %" PRIu64 " Signal value:%ld (Signal pool sz: %lu)\n", ret->signal.handle, hsa_signal_load_relaxed(ret->signal),
                    FreeSignalPool.size());
            FreeSignalPool.pop();
            DEBUG_PRINT("[Try Dispatch] Free Signal Pool Size: %lu; Ready Task Queue Size: %lu\n", FreeSignalPool.size(), ReadyTaskQueue.size());
            //unlock(&mutex_readyq_);
        }
        if(ret->kernel) {
            // get kernarg resource
            uint32_t kernarg_segment_size = kernel_impl->kernarg_segment_size;
            int free_idx = kernel_impl->free_kernarg_segments.front();
            DEBUG_PRINT("Acquiring Kernarg Segment Id: %d\n", free_idx);
            ret->kernarg_region_index = free_idx;
            void *addr = (void *)((char *)kernel_impl->kernarg_region +
                    (free_idx * kernarg_segment_size));
            kernel_impl->free_kernarg_segments.pop();
            assert(!(ret->kernel->num_args) || (ret->kernel->num_args && (ret->kernarg_region || args)));
            if(ret->kernarg_region != NULL) {
                // we had already created a memory region using malloc. Copy it
                // to the newly availed space
                size_t size_to_copy = ret->kernarg_region_size;
                if(ret->devtype == ATMI_DEVTYPE_GPU && kernel_impl->kernel_type == AMDGCN) {
                    // do not copy the implicit args from saved region
                    // they are to be set/reset during task dispatch
                    size_to_copy -= sizeof(atmi_implicit_args_t);
                }
                if(size_to_copy) memcpy(addr, ret->kernarg_region, size_to_copy);
                // free existing region
                free(ret->kernarg_region);
                ret->kernarg_region = addr;
            }
            else {
                // first time allocation/assignment
                ret->kernarg_region = addr;
                set_kernarg_region(ret, args);
            }
        }
        set_task_state(ret, ATMI_READY);
    }
    else {
        if(ret->kernel && ret->kernarg_region == NULL) {
            // first time allocation/assignment
            ret->kernarg_region = malloc(ret->kernarg_region_size);
            //ret->kernarg_region_copied = true;
            set_kernarg_region(ret, args);
        }
        set_task_state(ret, ATMI_INITIALIZED);
    }
    if(predecessors_complete == true &&
       resources_available == false  &&
       ret->stream_obj->ordered == false) {
        // Ready task but no resources available. So, we push it to a
        // ready queue
        ReadyTaskQueue.push(ret);
        max_ready_queue_sz++;
    }

    unlock_set(req_mutexes);
    return should_try_dispatch;
}

atl_task_t *get_new_task() {
    atl_task_t *ret = new atl_task_t;
    memset(ret, 0, sizeof(atl_task_t));
    //ret->is_continuation = false;
    lock(&mutex_all_tasks_);
    AllTasks.push_back(ret);
    set_task_state(ret, ATMI_UNINITIALIZED);
    atmi_task_handle_t new_id;
    //new_id.node = 0;
    set_task_handle_ID(&new_id, AllTasks.size() - 1);
    //new_id.lo = AllTasks.size() - 1;
    //PublicTaskMap[new_id] = ret;
    unlock(&mutex_all_tasks_);
    ret->id = new_id;
    ret->and_successors.clear();
    ret->and_predecessors.clear();
    ret->predecessors.clear();
    ret->continuation_task = NULL;
    ret->pred_stream_objs.clear();
    pthread_mutex_init(&(ret->mutex), NULL);
    return ret;
}

bool try_dispatch(atl_task_t *ret, void **args, boolean synchronous) {
    ShouldDispatchTimer.Start();
    bool should_dispatch = true;
    bool register_task_callback = (ret->groupable != ATMI_TRUE);
    if(g_dep_sync_type == ATL_SYNC_CALLBACK) {
        should_dispatch = try_dispatch_callback(ret, args);
    }
    else if(g_dep_sync_type == ATL_SYNC_BARRIER_PKT) {
        should_dispatch = try_dispatch_barrier_pkt(ret, args);
    }
    ShouldDispatchTimer.Stop();

    if(should_dispatch) {
        if(ret->atmi_task) {
            // FIXME: set a lookup table for dynamic parallelism kernels
            // perhaps?
            ret->atmi_task->handle = (void *)(&(ret->signal));
        }
        // acquire_kernel_impl_segment
        // update kernarg ptrs
        //direct_dispatch++;
        dispatch_task(ret);


        RegisterCallbackTimer.Start();
        if(register_task_callback) {
            DEBUG_PRINT("Registering callback for task %lu\n", ret->id);
            hsa_status_t err = hsa_amd_signal_async_handler(ret->signal,
                    HSA_SIGNAL_CONDITION_EQ, 0,
                    handle_signal, (void *)ret);
            ErrorCheck(Creating signal handler, err);
        }
        else {
            if(!ret->stream_obj->callback_started.test_and_set()) {
                DEBUG_PRINT("Registering callback for task groups\n");
                hsa_status_t err = hsa_amd_signal_async_handler(ret->signal,
                        HSA_SIGNAL_CONDITION_EQ, 0,
                        handle_group_signal, (void *)(ret->stream_obj));
                ErrorCheck(Creating signal handler, err);
            }
        }
        RegisterCallbackTimer.Stop();
    }
    if ( synchronous == ATMI_TRUE ) { /*  Sychronous execution */
        /* For default synchrnous execution, wait til kernel is finished.  */
    //    TaskWaitTimer.Start();
        if(ret->groupable != ATMI_TRUE) {
            atl_task_wait(ret);
        }
        else {
            atl_stream_sync(ret->stream_obj);
        }
        set_task_metrics(ret);
        set_task_state(ret, ATMI_COMPLETED);
      //  TaskWaitTimer.Stop();
        //std::cout << "Task Wait Interim Timer " << TaskWaitTimer << std::endl;
        //std::cout << "Launch Time: " << TryLaunchTimer << std::endl;
    }
    else {
        /* add task to the corresponding row in the stream table */
        if(ret->groupable != ATMI_TRUE)
            register_task(ret->stream_obj, ret);
    }
    return should_dispatch;
}

atl_task_t *atl_trycreate_task(atl_kernel_t *kernel) {
    atl_task_t *ret = get_new_task();
    ret->kernel = kernel;
    return ret;
}

void set_task_params(atl_task_t *task_obj,
                 const atmi_lparm_t *lparm,
                 unsigned int kernel_id,
                 void **args) {
    TryLaunchInitTimer.Start();
    if(!task_obj) return;
    DEBUG_PRINT("GPU Place Info: %d, %d, %d : %lx\n", lparm->place.node_id, lparm->place.type, lparm->place.device_id, lparm->place.cu_mask);
#if 1
    static bool is_called = false;
    if(!is_called) {
        set_thread_affinity(0);

        /* int policy;
           struct sched_param param;
           pthread_getschedparam(pthread_self(), &policy, &param);
           param.sched_priority = sched_get_priority_min(policy);
           printf("Setting Priority Policy for %d: %d\n", policy, param.sched_priority);
           pthread_setschedparam(pthread_self(), policy, &param);
           */
        is_called = true;
    }
#endif
    atmi_task_group_t *stream = NULL;
    if(lparm->group == NULL) {
        stream = &atl_default_stream_obj;
    } else {
        stream = lparm->group;
    }
    /* Add row to stream table for purposes of future synchronizations */
    register_stream(stream);
    atmi_task_group_table_t *stream_obj = StreamTable[stream->id];

    uint16_t i;
    atl_task_t *ret = task_obj;
    atl_task_t *continuation_task = NULL;
    bool has_continuation = false;
#if 0
    if(lparm->task) {
        has_continuation = (lparm->task->continuation != NULL);
        if(PublicTaskMap.find(lparm->task) != PublicTaskMap.end()) {
            // task is already found, so use it and dont create a new one
            ret = PublicTaskMap[lparm->task];
            DEBUG_PRINT("Task %p found\n", lparm->task);
        }
    }
    if(ret == NULL || ret->is_continuation == false) {
        /* create anyway if not part of a continuation? */
        ret = new atl_task_t;
        memset(ret, 0, sizeof(atl_task_t));
        ret->is_continuation = false;
        lock(&mutex_all_tasks_);
        AllTasks.push_back(ret);
        //AllTasks.push_back(atl_task_t());
        //AllTasks.emplace_back();
        //ret = AllTasks[AllTasks.size() - 1];
        atmi_task_handle_t new_id;
        new_id.node = 0;
        new_id.hi = 0;
        new_id.lo = AllTasks.size() - 1;
        ret->id = new_id;
        ret->and_successors.clear();
        ret->and_predecessors.clear();
        ret->predecessors.clear();
        //AllTasks.push_back(task);
        //atl_task_t *ret = task; //AllTasks[AllTasks.size() - 1];
        unlock(&mutex_all_tasks_);
        if(lparm->task) {
            PublicTaskMap[lparm->task] = ret;
            DEBUG_PRINT("Task Map[%p] = %p (%s)\n", lparm->task, PublicTaskMap[lparm->task], kernel_name);
        }
        pthread_mutex_init(&(ret->mutex), NULL);
    }
    if(has_continuation) {
        continuation_task = new atl_task_t;
        memset(continuation_task, 0, sizeof(atl_task_t));
        continuation_task->is_continuation = true;
        lock(&mutex_all_tasks_);
        AllTasks.push_back(continuation_task);
        atmi_task_handle_t new_id;
        new_id.node = 0;
        new_id.hi = 0;
        new_id.lo = AllTasks.size() - 1;
        continuation_task->id = new_id;
        ret->id.hi = AllTasks.size();
        unlock(&mutex_all_tasks_);
        PublicTaskMap[lparm->task->continuation] = continuation_task;
        DEBUG_PRINT("Continuation Map[%p] = %p (%s)\n", lparm->task->continuation, PublicTaskMap[lparm->task->continuation], kernel_name);
        pthread_mutex_init(&(continuation_task->mutex), NULL);
    }
#else
#endif
    atl_kernel_t *kernel = ret->kernel;
    ret->kernel_id = kernel_id;
    atl_kernel_impl_t *kernel_impl = get_kernel_impl(kernel, ret->kernel_id);
    ret->kernarg_region = NULL;
    ret->kernarg_region_size = kernel_impl->kernarg_segment_size;
    ret->devtype = kernel_impl->devtype;
    ret->profilable = lparm->profilable;
    ret->groupable = lparm->groupable;
    ret->atmi_task = lparm->task_info;

    // fill in from lparm
    for(int i = 0; i < 3 /* 3dims */; i++) {
        ret->gridDim[i] = lparm->gridDim[i];
        ret->groupDim[i] = lparm->groupDim[i];
    }
    //DEBUG_PRINT("Requires LHS: %p and RHS: %p\n", ret->lparm.requires, lparm->requires);
    //DEBUG_PRINT("Requires ThisTask: %p and ThisTask: %p\n", ret->lparm.task_info, lparm->task_info);

    ret->group = *stream;
    ret->stream_obj = stream_obj;
    ret->place = lparm->place;
    ret->synchronous = lparm->synchronous;
    //DEBUG_PRINT("Stream LHS: %p and RHS: %p\n", ret->lparm.group, lparm->group);
    ret->num_predecessors = 0;
    ret->num_successors = 0;

    /* For dependent child tasks, add dependent parent kernels to barriers.  */
    DEBUG_PRINT("Pif %s requires %d task\n", kernel_impl->kernel_name.c_str(), lparm->num_required);

    ret->predecessors.clear();
    //ret->predecessors.resize(lparm->num_required);
    for(int idx = 0; idx < lparm->num_required; idx++) {
        atl_task_t *pred_task = get_task(lparm->requires[idx]);
        //assert(pred_task != NULL);
        if(pred_task) {
            DEBUG_PRINT("Task %lu adding %lu task as predecessor\n", ret->id, pred_task->id);
            ret->predecessors.push_back(pred_task);
        }
    }
    ret->pred_stream_objs.clear();
    ret->pred_stream_objs.resize(lparm->num_required_groups);
    for(int idx = 0; idx < lparm->num_required_groups; idx++) {
        std::map<int, atmi_task_group_table_t *>::iterator map_it = StreamTable.find(lparm->required_groups[idx]->id);
        if(map_it != StreamTable.end()) {
            atmi_task_group_table_t *pred_tg = map_it->second;
            assert(pred_tg != NULL);
            DEBUG_PRINT("Task %p adding %p task group as predecessor\n", ret, pred_tg);
            ret->pred_stream_objs[idx] = pred_tg;
        }
    }

    ret->type = ATL_KERNEL_EXECUTION;
    ret->data_src_ptr = NULL;
    ret->data_dest_ptr = NULL;
    ret->data_size = 0;

    if(ret->stream_obj->ordered) {
        lock(&(ret->stream_obj->group_mutex));
        ret->stream_obj->running_ordered_tasks.push_back(ret);
        ret->prev_ordered_task = ret->stream_obj->last_task;
        ret->stream_obj->last_task = ret;
        unlock(&(ret->stream_obj->group_mutex));
    }
    if(ret->groupable) {
        DEBUG_PRINT("Add ref_cnt 1 to task group %p\n", ret->stream_obj);
        (ret->stream_obj->task_count)++;
    }
    TryLaunchInitTimer.Stop();
}

atmi_task_handle_t atl_trylaunch_kernel(const atmi_lparm_t *lparm,
                 atl_task_t *task_obj,
                 unsigned int kernel_id,
                 void **args) {
    atmi_task_handle_t ret_handle = ATMI_NULL_TASK_HANDLE;
    atl_task_t *ret = task_obj;
    if(ret) {
        set_task_params(ret, lparm, kernel_id, args);
        try_dispatch(ret, args, lparm->synchronous);
        ret_handle = ret->id;
    }
    return ret_handle;
}

atl_kernel_t *get_kernel_obj(atmi_kernel_t atmi_kernel) {
    uint64_t pif_id = atmi_kernel.handle;
    std::map<uint64_t, atl_kernel_t *>::iterator map_iter;
    map_iter = KernelImplMap.find(pif_id);
    if(map_iter == KernelImplMap.end()) {
        DEBUG_PRINT("ERROR: Kernel/PIF %lu not found\n", pif_id);
        return NULL;
    }
    atl_kernel_t *kernel = map_iter->second;
    return kernel;
 }

atmi_task_handle_t atmi_task_template_create(atmi_kernel_t atmi_kernel) {
    atmi_task_handle_t ret = ATMI_NULL_TASK_HANDLE;
    atl_kernel_t *kernel = get_kernel_obj(atmi_kernel);
    if(kernel) {
        atl_task_t *ret_obj = atl_trycreate_task(kernel);
        if(ret_obj) ret = ret_obj->id;
    }
    return ret;
}

int get_kernel_id(atmi_lparm_t *lparm, atl_kernel_t *kernel) {
    int num_args = kernel->num_args;
    int kernel_id = lparm->kernel_id;
    if(kernel_id == -1) {
        // choose the first available kernel for the given devtype
        atmi_devtype_t type = lparm->place.type;
        for(std::vector<atl_kernel_impl_t *>::iterator it = kernel->impls.begin();
            it != kernel->impls.end(); it++) {
            if((*it)->devtype == type) {
                kernel_id = (*it)->kernel_id;
                break;
            }
        }
        if(kernel_id == -1) {
            fprintf(stderr, "ERROR: Kernel/PIF %lu doesn't have any implementations\n",
                    kernel->pif_id);
            return -1;
        }
    }
    else {
        if(!is_valid_kernel_id(kernel, kernel_id)) {
            DEBUG_PRINT("ERROR: Kernel ID %d not found\n", kernel_id);
            return -1;
        }
    }
    atl_kernel_impl_t *kernel_impl = get_kernel_impl(kernel, kernel_id);
    if(kernel->num_args && kernel_impl->kernarg_region == NULL) {
        fprintf(stderr, "ERROR: Kernel Arguments not initialized for Kernel %s\n",
                            kernel_impl->kernel_name.c_str());
        return -1;
    }

    return kernel_id;
}

atmi_task_handle_t atmi_task_template_activate(atmi_task_handle_t task, atmi_lparm_t *lparm,
                                    void **args) {
    atmi_task_handle_t ret = ATMI_NULL_TASK_HANDLE;
    atl_task_t *task_obj = get_task(task);
    if(!task_obj) return ret;

    /*if(lparm == NULL && args == NULL) {
        DEBUG_PRINT("Signaling the completed task\n");
        set_task_state(task_obj, ATMI_COMPLETED);
        return task;
    }*/

    atl_kernel_t *kernel = task_obj->kernel;
    int kernel_id = get_kernel_id(lparm, kernel);
    if(kernel_id == -1) return ret;

    ret = atl_trylaunch_kernel(lparm, task_obj, kernel_id, args);
    DEBUG_PRINT("[Returned Task: %lu]\n", ret);
    return ret;
}

atmi_task_handle_t atmi_task_create(atmi_lparm_t *lparm,
                                    atmi_kernel_t atmi_kernel,
                                    void **args) {

    atmi_task_handle_t ret = ATMI_NULL_TASK_HANDLE;
    atl_kernel_t *kernel = get_kernel_obj(atmi_kernel);
    if(kernel) {
        atl_task_t *ret_obj = atl_trycreate_task(kernel);
        if(ret_obj) {
            int kernel_id = get_kernel_id(lparm, kernel);
            if(kernel_id == -1) return ret;
            set_task_params(ret_obj, lparm, kernel_id, args);

            std::set<pthread_mutex_t *> req_mutexes;
            req_mutexes.clear();
            req_mutexes.insert((pthread_mutex_t *)&(ret_obj->mutex));
            std::vector<atl_task_t *> &temp_vecs = ret_obj->predecessors;
            for(int idx = 0; idx < ret_obj->predecessors.size(); idx++) {
                atl_task_t *pred_task = ret_obj->predecessors[idx];
                req_mutexes.insert((pthread_mutex_t *)&(pred_task->mutex));
            }
#if 0
            // do we care about locking for ordered and task group mutexes?
            if(ret_obj->prev_ordered_task)
                req_mutexes.insert(&(ret_obj->prev_ordered_task->mutex));
            atmi_task_group_table_t *stream_obj = ret_obj->stream_obj;
            req_mutexes.insert(&(stream_obj->group_mutex));
#endif
            lock_set(req_mutexes);
            // populate its predecessors' successor list
            // similar to a double linked list
            if(ret_obj->predecessors.size() > 0) {
                // add to its predecessor's dependents list and ret_objurn
                for(int idx = 0; idx < ret_obj->predecessors.size(); idx++) {
                    atl_task_t *pred_task = ret_obj->predecessors[idx];
                    DEBUG_PRINT("Task %p depends on %p as predecessor ",
                            ret_obj, pred_task);
                    if(pred_task->state /*.load(std::memory_order_seq_cst)*/ < ATMI_EXECUTED) {
                        //should_try_dispatch = false;
                        //predecessors_complete = false;
                        pred_task->and_successors.push_back(ret_obj);
                        ret_obj->num_predecessors++;
                        DEBUG_PRINT("(waiting)\n");
                        //waiting_count++;
                    }
                    else {
                        DEBUG_PRINT("(completed)\n");
                    }
                }
            }
            if(ret_obj->pred_stream_objs.size() > 0) {
                // add to its predecessor's dependents list and ret_objurn
                DEBUG_PRINT("Task %lu has %lu predecessor task groups\n", ret_obj->id, ret_obj->pred_stream_objs.size());
                for(int idx = 0; idx < ret_obj->pred_stream_objs.size(); idx++) {
                    atmi_task_group_table_t *pred_tg = ret_obj->pred_stream_objs[idx];
                    DEBUG_PRINT("Task %p depends on %p as predecessor task group ",
                            ret_obj, pred_tg);
                    if(pred_tg && pred_tg->task_count.load() > 0) {
                        // predecessor task group is still running, so add yourself to its successor list
                        //should_try_dispatch = false;
                        //predecessors_complete = false;
                        pred_tg->and_successors.push_back(ret_obj);
                        ret_obj->num_predecessors++;
                        DEBUG_PRINT("(waiting)\n");
                    }
                    else {
                        DEBUG_PRINT("(completed)\n");
                    }
                }
            }

            // Save kernel args because it will be activated
            // later. This way, the user will be able to
            // reuse their kernarg region that was created
            // in the application space.
            if(ret_obj->kernel && ret_obj->kernarg_region == NULL) {
                // first time allocation/assignment
                ret_obj->kernarg_region = malloc(ret_obj->kernarg_region_size);
                //ret->kernarg_region_copied = true;
                set_kernarg_region(ret_obj, args);
            }
            set_task_state(ret_obj, ATMI_INITIALIZED);

            unlock_set(req_mutexes);
            ret = ret_obj->id;
        }
    }
    return ret;
}

atmi_task_handle_t atmi_task_activate(atmi_task_handle_t task) {
    atmi_task_handle_t ret = ATMI_NULL_TASK_HANDLE;
    atl_task_t *ret_obj = get_task(task);
    if(!ret_obj) return ret;
    ret = ret_obj->id;

    // If the task has predecessors then you cannot activate this task. Task
    // activation is supported only for tasks without predecessors.
    if(ret_obj->predecessors.size() <= 0)
        try_dispatch(ret_obj, NULL, ret_obj->synchronous);
    DEBUG_PRINT("[Returned Task: %lu]\n", ret);
    return ret;
}

atmi_task_handle_t atmi_task_launch(atmi_lparm_t *lparm, atmi_kernel_t atmi_kernel,
                                    void **args/*, more params for place info? */) {
    ParamsInitTimer.Start();
    atmi_task_handle_t ret = ATMI_NULL_TASK_HANDLE;
    atl_kernel_t *kernel = get_kernel_obj(atmi_kernel);
    if(!kernel) return ret;

    int kernel_id = get_kernel_id(lparm, kernel);
    if(kernel_id == -1) return ret;
    /*lock(&(kernel_impl->mutex));
    if(kernel_impl->free_kernarg_segments.empty()) {
        // no free kernarg segments -- allocate some more?
        // FIXME: realloc instead? HSA realloc?
    }
    unlock(&(kernel_impl->mutex));
    */
    ParamsInitTimer.Stop();
    TryLaunchTimer.Start();
    atl_task_t *ret_obj = atl_trycreate_task(kernel);
    ret = atl_trylaunch_kernel(lparm, ret_obj, kernel_id, args);
    TryLaunchTimer.Stop();
    DEBUG_PRINT("[Returned Task: %lu]\n", ret);
    return ret;
}

/* Machine Info */
atmi_machine_t *atmi_machine_get_info() {
    if(!atlc.g_hsa_initialized) return NULL;
    return &g_atmi_machine;
}


