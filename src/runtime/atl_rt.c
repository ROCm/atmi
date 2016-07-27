/*

  Copyright (c) 2015 ADVANCED MICRO DEVICES, INC.  

  AMD is granting you permission to use this software and documentation (if any) (collectively, the 
  Materials) pursuant to the terms and conditions of the Software License Agreement included with the 
  Materials.  If you do not have a copy of the Software License Agreement, contact your AMD 
  representative for a copy.

  You agree that you will not reverse engineer or decompile the Materials, in whole or in part, except for 
  example code which is provided in source code form and as allowed by applicable law.

  WARRANTY DISCLAIMER: THE SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY 
  KIND.  AMD DISCLAIMS ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING BUT NOT 
  LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
  PURPOSE, TITLE, NON-INFRINGEMENT, THAT THE SOFTWARE WILL RUN UNINTERRUPTED OR ERROR-
  FREE OR WARRANTIES ARISING FROM CUSTOM OF TRADE OR COURSE OF USAGE.  THE ENTIRE RISK 
  ASSOCIATED WITH THE USE OF THE SOFTWARE IS ASSUMED BY YOU.  Some jurisdictions do not 
  allow the exclusion of implied warranties, so the above exclusion may not apply to You. 

  LIMITATION OF LIABILITY AND INDEMNIFICATION:  AMD AND ITS LICENSORS WILL NOT, 
  UNDER ANY CIRCUMSTANCES BE LIABLE TO YOU FOR ANY PUNITIVE, DIRECT, INCIDENTAL, 
  INDIRECT, SPECIAL OR CONSEQUENTIAL DAMAGES ARISING FROM USE OF THE SOFTWARE OR THIS 
  AGREEMENT EVEN IF AMD AND ITS LICENSORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH 
  DAMAGES.  In no event shall AMD's total liability to You for all damages, losses, and 
  causes of action (whether in contract, tort (including negligence) or otherwise) 
  exceed the amount of $100 USD.  You agree to defend, indemnify and hold harmless 
  AMD and its licensors, and any of their directors, officers, employees, affiliates or 
  agents from and against any and all loss, damage, liability and other expenses 
  (including reasonable attorneys' fees), resulting from Your use of the Software or 
  violation of the terms and conditions of this Agreement.  

  U.S. GOVERNMENT RESTRICTED RIGHTS: The Materials are provided with "RESTRICTED RIGHTS." 
  Use, duplication, or disclosure by the Government is subject to the restrictions as set 
  forth in FAR 52.227-14 and DFAR252.227-7013, et seq., or its successor.  Use of the 
  Materials by the Government constitutes acknowledgement of AMD's proprietary rights in them.

  EXPORT RESTRICTIONS: The Materials may be subject to export restrictions as stated in the 
  Software License Agreement.

*/ 

/* This file is the ATMI library.  */
#include "atl_internal.h"
#include "atl_profile.h"
#include "ATLQueue.h"
#include "ATLMachine.h"
#include <pthread.h>
#include <time.h>
#include <assert.h>
#include <fstream>
#include <iostream>
#include <map>
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
RealTimer SignalAddTimer;
RealTimer HandleSignalTimer;
RealTimer HandleSignalInvokeTimer;
RealTimer TaskWaitTimer;
RealTimer TryLaunchTimer;
RealTimer ParamsInitTimer;
RealTimer TryLaunchInitTimer;
RealTimer ShouldDispatchTimer;
RealTimer RegisterCallbackTimer;
RealTimer TryDispatchTimer;
static size_t max_ready_queue_sz = 0;
static size_t waiting_count = 0;
static size_t direct_dispatch = 0;
static size_t callback_dispatch = 0;
atl_task_t ***GlobalTaskPtr;
#define NSECPERSEC 1000000000L

//  set NOTCOHERENT needs this include
#include "hsa_ext_amd.h"

/* -------------- Helper functions -------------------------- */
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

extern bool handle_signal(hsa_signal_value_t value, void *arg);

void print_atl_kernel(const char * str, const int i);
/* Stream table to hold the runtime state of the 
 * stream and its tasks. Which was the latest 
 * device used, latest queue used and also a 
 * pool of tasks for synchronization if need be */
std::map<int, atmi_task_group_table_t *> StreamTable;

std::vector<atl_task_t *> AllTasks;
std::queue<atl_task_t *> ReadyTaskQueue;
std::queue<hsa_signal_t> FreeSignalPool;

std::vector<std::map<std::string, atl_kernel_info_t> > KernelInfoTable;
std::map<std::string, atl_kernel_t *> KernelImplMap;
std::map<std::string, atmi_klist_t *> PifKlistMap;
//std::map<uint64_t, std::vector<std::string> > ModuleMap;
hsa_signal_t StreamCommonSignalPool[ATMI_MAX_STREAMS];
hsa_signal_t IdentityORSignal;
hsa_signal_t IdentityANDSignal;
hsa_signal_t IdentityCopySignal; 
static int StreamCommonSignalIdx = 0;
std::queue<atl_task_t *> DispatchedTasks;

atmi_machine_t g_atmi_machine;
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

atmi_task_handle_t NULL_TASK = ATMI_TASK_HANDLE(0);

#define handle_error_en(en, msg) \
    do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

atmi_status_t set_thread_affinity(int id) {
    int s, j;
    cpu_set_t cpuset;
    pthread_t thread;

    thread = pthread_self();

    /* Set affinity mask to include CPUs 0 to 7 */

    CPU_ZERO(&cpuset);
    CPU_SET(id, &cpuset);

    s = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    if (s != 0)
        handle_error_en(s, "pthread_setaffinity_np");

    /* Check the actual affinity mask
     * assigned to the thread */
    s = pthread_getaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    if (s != 0)
        handle_error_en(s, "pthread_getaffinity_np");

    /*printf("Set returned by pthread_getaffinity_np() contained:\n");
    for (j = 0; j < CPU_SETSIZE; j++)
        if (CPU_ISSET(j, &cpuset))
            printf("    CPU %d\n", j);
    */
    return ATMI_STATUS_SUCCESS; 
}

long int get_nanosecs( struct timespec start_time, struct timespec end_time) {
    long int nanosecs;
    if ((end_time.tv_nsec-start_time.tv_nsec)<0) nanosecs =
        ((((long int) end_time.tv_sec- (long int) start_time.tv_sec )-1)*NSECPERSEC ) +
            ( NSECPERSEC + (long int) end_time.tv_nsec - (long int) start_time.tv_nsec) ;
    else nanosecs =
        (((long int) end_time.tv_sec- (long int) start_time.tv_sec )*NSECPERSEC ) +
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
    task_handle |= 0xFFFFFFFF;
    task_handle &= ID;
    *t = task_handle;
    #else
    t->lo = ID;
    #endif
}

atl_task_t *get_task(atmi_task_handle_t t) {
    /* FIXME: node 0 only for now */
    return AllTasks[get_task_handle_ID(t)];
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
                DEBUG_PRINT("Enqueue wait for task %d signal handle: %" PRIu64 "\n", (*tasks)->id, barrier->dep_signal[dep_signal_id].handle);
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
    if(task != NULL) {
        if(task->state < ATMI_DISPATCHED) {
            while(true) {
                int value = task->state;//.load(std::memory_order_seq_cst);
                if(value != ATMI_COMPLETED) {
                    //DEBUG_PRINT("Signal Value: %" PRIu64 "\n", task->signal.handle);
                    DEBUG_PRINT("Task (%d) state: %d\n", task->id, value);
                }
                else {
                    //printf("Task[%d] Completed!\n", task->id);
                    break;
                }
            }
        } 
        else {
            DEBUG_PRINT("Signal handle: %" PRIu64 " Signal value:%ld\n", task->signal.handle, hsa_signal_load_relaxed(task->signal));
            hsa_signal_wait_acquire(task->signal, HSA_SIGNAL_CONDITION_EQ, 0, UINT64_MAX, ATMI_WAIT_STATE);
        }
        /* Flag this task as completed */
        /* FIXME: How can HSA tell us if and when a task has failed? */
        set_task_state(task, ATMI_COMPLETED);
#if 0
        std::cout << "Params Init Timer: " << ParamsInitTimer << std::endl;
        std::cout << "Launch Time: " << TryLaunchTimer << std::endl;
#if 0
        std::cout << "Handle Signal Timer: " << HandleSignalTimer << std::endl;
        std::cout << "Handle Signal Invoke Timer: " << HandleSignalInvokeTimer << std::endl;
        std::cout << "Launch Init Timer: " << TryLaunchInitTimer << std::endl;
        std::cout << "Dispatch Eval Timer: " << ShouldDispatchTimer << std::endl;
        std::cout << "Dispatch Timer: " << TryDispatchTimer << std::endl;
        std::cout << "Register Callback Timer: " << RegisterCallbackTimer << std::endl;

        std::cout << "Signal Timer: " << SignalAddTimer << std::endl;
        std::cout << "Task Wait Time: " << TaskWaitTimer << std::endl;
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
        RegisterCallbackTimer.Reset();
        max_ready_queue_sz = 0;
        waiting_count = 0;
        direct_dispatch = 0;
        callback_dispatch = 0;
#endif
    }

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
    hsa_status_t err;
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
            ATLFineMemory new_mem(memory_pool, *proc);
            proc->addMemory(new_mem);
            if(HSA_AMD_MEMORY_POOL_GLOBAL_FLAG_KERNARG_INIT & global_flag) {
                DEBUG_PRINT("GPU kernel args pool handle: %lu\n", memory_pool.handle);
                atl_gpu_kernarg_pool = memory_pool;
            }
        }
        else {
            ATLCoarseMemory new_mem(memory_pool, *proc);
            proc->addMemory(new_mem);
        }
    }
	return HSA_STATUS_SUCCESS;
}


static hsa_status_t get_agent_info(hsa_agent_t agent, void *data) {
    hsa_status_t err;
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

    return HSA_STATUS_SUCCESS;
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
        for(int i = 0; i < KernelInfoTable.size(); i++) 
            KernelInfoTable[i].clear();
        KernelInfoTable.clear();
        NULL_TASK = ATMI_TASK_HANDLE(0);
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
    hsa_status_t err;
    //if(task->profile != NULL) {
    if(task->profilable == ATMI_TRUE) {
        hsa_signal_t signal = task->signal;
        hsa_amd_profiling_dispatch_time_t metrics;
        if(task->devtype == ATMI_DEVTYPE_GPU) {
            err = hsa_amd_profiling_get_dispatch_time(atl_gpu_agent, 
                    signal, &metrics); 
            ErrorCheck(Profiling GPU dispatch, err);
            if(task->atmi_task) {
                task->atmi_task->profile.start_time = metrics.start;
                task->atmi_task->profile.end_time = metrics.end;
                task->atmi_task->profile.dispatch_time = metrics.start;
                task->atmi_task->profile.ready_time = metrics.start;
            }
        }
        else {
            /* metrics for CPU tasks will be populated in the 
             * worker pthread itself. No special function call */
        }
    }
}

extern void atl_stream_sync(atmi_task_group_table_t *stream_obj) {
    if(stream_obj->ordered == ATMI_TRUE) {
        /* just insert a barrier packet to the CPU and GPU queues and wait */
        DEBUG_PRINT("Waiting for GPU Q\n");
        queue_sync(stream_obj->gpu_queue);
        if(stream_obj->cpu_queue) 
            signal_worker(stream_obj->cpu_queue, PROCESS_PKT);
        DEBUG_PRINT("Waiting for CPU Q\n");
        queue_sync(stream_obj->cpu_queue);
        for(std::vector<atl_task_t *>::iterator it = stream_obj->running_groupable_tasks.begin();
                    it != stream_obj->running_groupable_tasks.end(); it++) {
            set_task_state(*it, ATMI_COMPLETED);
            set_task_metrics(*it);
        }
        /*atl_task_list_t *task_head = stream_obj->tasks;
        DEBUG_PRINT("Waiting for async ordered tasks\n");
        while(task_head) {
            set_task_state(task_head->task, ATMI_COMPLETED);
            set_task_metrics(task_head->task);
            task_head = task_head->next;
        }*/
    }
    else {
        /* wait on each one of the tasks in the task bag */
        #if 1
        DEBUG_PRINT("Waiting for async unordered tasks\n");
        hsa_signal_t signal = stream_obj->common_signal;
        if(signal.handle != (uint64_t)-1) {
            #if 1
            hsa_signal_wait_acquire(signal, HSA_SIGNAL_CONDITION_EQ, 0, UINT64_MAX, ATMI_WAIT_STATE);
            #else
            // debugging atmi dp doorbell signaling. remove once that issue is resolved.
            hsa_signal_value_t val;
            do {
                val = hsa_signal_load_relaxed(signal);
                printf("Signal Value: %lu %lu\n", val, hsa_signal_load_relaxed(GPU_CommandQ[0]->getQueue()->doorbell_signal));
                hsa_queue_t *this_Q = GPU_CommandQ[0]->getQueue();
                const uint32_t queueMask = this_Q->size - 1;
                printf("Headers: (%lu %lu) ", hsa_queue_load_read_index_relaxed(this_Q), hsa_queue_load_write_index_relaxed(this_Q));
                for(uint64_t index = 0; index < 20; index++) {
                    hsa_kernel_dispatch_packet_t* this_aql = &(((hsa_kernel_dispatch_packet_t*)(this_Q->base_address))[index&queueMask]);
                    printf("%d ", (int)(0x00FF & this_aql->header)); 
                }
                printf("\n");
                sleep(1);
            } while (val != 0);
            #endif
        }
        for(std::vector<atl_task_t *>::iterator it = stream_obj->running_groupable_tasks.begin();
                    it != stream_obj->running_groupable_tasks.end(); it++) {
            // atl_task_wait(task_head->task); 
            set_task_state(*it, ATMI_COMPLETED);
            set_task_metrics(*it);
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
        int num_tasks = 0;
        atl_task_list_t *task_head = stream_obj->tasks;
        while(task_head) {
            num_tasks++;
            task_head = task_head->next;
        }
        if(num_tasks > 0) {
            atmi_task_t **tasks = (atmi_task_t **)malloc(sizeof(atmi_task_t *) * num_tasks);
            int task_id = 0;
            task_head = stream_obj->tasks;
            for(task_id = 0; task_id < num_tasks; task_id++) {
                tasks[task_id] = task_head->task;
                task_head = task_head->next;
            }
            
            if(stream_obj->gpu_queue != NULL) 
                enqueue_barrier_gpu(stream_obj->gpu_queue, num_tasks, tasks, SNK_WAIT);
            else if(stream_obj->cpu_queue != NULL) 
                enqueue_barrier_cpu(stream_obj->cpu_queue, num_tasks, tasks, SNK_WAIT);
            else {
                int idx; 
                for(idx = 0; idx < num_tasks; idx++) {
                    atl_task_wait(tasks[idx]);
                }
            }
            free(tasks);
        }
        #endif
    }
    clear_saved_tasks(stream_obj);
}

atmi_status_t atmi_task_group_sync(atmi_task_group_t *stream) {
    atmi_task_group_t *str = (stream == NULL) ? &atl_default_stream_obj : stream;
    atmi_task_group_table_t *stream_obj = StreamTable[str->id];
    if(stream_obj) atl_stream_sync(stream_obj);

    return ATMI_STATUS_SUCCESS;
}

hsa_queue_t *acquire_and_set_next_cpu_queue(atmi_task_group_table_t *stream_obj, atmi_place_t place) {
    ATLCPUProcessor &proc = get_processor<ATLCPUProcessor>(place);
    atmi_scheduler_t sched = stream_obj->ordered ? ATMI_SCHED_NONE : ATMI_SCHED_RR;
    hsa_queue_t *queue = proc.getBestQueue(sched);
    stream_obj->cpu_queue = queue;
    return queue;
}

std::vector<hsa_queue_t *> get_cpu_queues(atmi_place_t place) {
    ATLCPUProcessor &proc = get_processor<ATLCPUProcessor>(place);
    return proc.getQueues();
}

hsa_queue_t *acquire_and_set_next_gpu_queue(atmi_task_group_table_t *stream_obj, atmi_place_t place) {
    ATLGPUProcessor &proc = get_processor<ATLGPUProcessor>(place);
    atmi_scheduler_t sched = stream_obj->ordered ? ATMI_SCHED_NONE : ATMI_SCHED_RR;
    hsa_queue_t *queue = proc.getBestQueue(sched);
    stream_obj->gpu_queue = queue;
    DEBUG_PRINT("Returned Queue: %p\n", queue);
    return queue;
}

atmi_status_t clear_saved_tasks(atmi_task_group_table_t *stream_obj) {
    hsa_status_t err;
    atl_task_list_t *cur = stream_obj->tasks;
    /*atl_task_list_t *prev = cur;
    while(cur != NULL ){
        cur = cur->next;
        free(prev);
        prev = cur;
    }
    */
    stream_obj->tasks = NULL;
    stream_obj->running_groupable_tasks.clear();
    return ATMI_STATUS_SUCCESS;
}

atmi_status_t check_change_in_device_type(atl_task_t *task, atmi_task_group_table_t *stream_obj, hsa_queue_t *queue, atmi_devtype_t new_task_device_type) {
    if(stream_obj->ordered != ATMI_ORDERED) return ATMI_STATUS_SUCCESS;

    if(stream_obj->tasks != NULL) {
        if(stream_obj->last_device_type != new_task_device_type) {
            DEBUG_PRINT("Devtype: %d waiting for task %p\n", new_task_device_type, stream_obj->tasks->task);
            /* device changed. introduce a dependency here for ordered streams */
            int num_required = 1;
            atl_task_t *requires = stream_obj->tasks->task;

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
    *signal = stream_obj->common_signal;
    return ATMI_STATUS_SUCCESS;
}

atmi_status_t set_stream_signal(atmi_task_group_table_t *stream_obj, hsa_signal_t signal) {
    stream_obj->common_signal = signal;
    return ATMI_STATUS_SUCCESS;
}

atmi_status_t get_stream_mutex(atmi_task_group_table_t *stream_obj, pthread_mutex_t *m) {
    *m = stream_obj->mutex;
    return ATMI_STATUS_SUCCESS;
}

atmi_status_t set_stream_mutex(atmi_task_group_table_t *stream_obj, pthread_mutex_t *m) {
    stream_obj->mutex = *m;
    return ATMI_STATUS_SUCCESS;
}

atmi_status_t register_stream(atmi_task_group_t *stream) {
    /* Check if the stream exists in the stream table. 
     * If no, then add this stream to the stream table.
     */
    DEBUG_PRINT("Stream %p registered\n", stream);
    if(StreamTable.find(stream->id) == StreamTable.end()) {
        atmi_task_group_table_t *stream_entry = new atmi_task_group_table_t;
        stream_entry->tasks = NULL;
        stream_entry->running_groupable_tasks.clear();
        stream_entry->cpu_queue = NULL;
        stream_entry->gpu_queue = NULL;
        stream_entry->common_signal.handle = (uint64_t)-1;
        pthread_mutex_init(&(stream_entry->mutex), NULL);
        StreamTable[stream->id] = stream_entry;
    }
    StreamTable[stream->id]->ordered = stream->ordered;

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
    //GlobalTaskPtr = (atl_task_t ***)malloc(sizeof(atl_task_t**));
    //int val = posix_memalign((void **)&GlobalTaskPtr, 4096, sizeof(atl_task_t **));
#ifdef MEMORY_REGION
    err = hsa_memory_allocate(atl_gpu_kernarg_region, 
            sizeof(atl_task_t **),
            (void **)&GlobalTaskPtr);
#else    
    err = hsa_amd_memory_pool_allocate(atl_gpu_kernarg_pool, 
            sizeof(atl_task_t **),
            0,
            (void **)&GlobalTaskPtr);
    allow_access_to_all_gpu_agents(GlobalTaskPtr);
#endif
    ErrorCheck(Creating the global task ptr, err);
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

atmi_status_t atmi_init(atmi_devtype_t devtype) {
    if(devtype == ATMI_DEVTYPE_ALL || devtype & ATMI_DEVTYPE_GPU) 
        atl_init_gpu_context();

    if(devtype == ATMI_DEVTYPE_ALL || devtype & ATMI_DEVTYPE_CPU) 
        atl_init_cpu_context();

    return ATMI_STATUS_SUCCESS;
}

atmi_status_t atmi_finalize() {
    // TODO: Finalize all processors, queues, signals, kernarg memory regions
    hsa_status_t err;
    for(int i = 0; i < g_executables.size(); i++) {
        err = hsa_executable_destroy(g_executables[i]);
        ErrorCheck(Destroying executable, err);
    }
    return ATMI_STATUS_SUCCESS;
}

void init_comute_and_memory() {
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

    /* Init all devices or individual device types? */
    std::vector<ATLCPUProcessor> &cpu_procs = g_atl_machine.getProcessors<ATLCPUProcessor>(); 
    std::vector<ATLGPUProcessor> &gpu_procs = g_atl_machine.getProcessors<ATLGPUProcessor>(); 
    std::vector<ATLDSPProcessor> &dsp_procs = g_atl_machine.getProcessors<ATLDSPProcessor>(); 
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
    DEBUG_PRINT("iGPU Agents: %lu\n", num_iGPUs);
    DEBUG_PRINT("dGPU Agents: %lu\n", num_dGPUs);
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
        
        std::vector<ATLFineMemory> fine_memories = cpu_procs[proc_index].getMemories<ATLFineMemory>();
        std::vector<ATLCoarseMemory> coarse_memories = cpu_procs[proc_index].getMemories<ATLCoarseMemory>();
        DEBUG_PRINT("CPU\tFine Memories : %lu\n", fine_memories.size());
        DEBUG_PRINT("\tCoarse Memories : %lu\n", coarse_memories.size());
        all_devices[i].memory_pool_count = fine_memories.size() + coarse_memories.size();
        proc_index++;
    }
    proc_index = 0;
    for(int i = gpus_begin; i < gpus_end; i++) {
        all_devices[i].type = gpu_procs[proc_index].getType();
        
        std::vector<ATLFineMemory> fine_memories = gpu_procs[proc_index].getMemories<ATLFineMemory>();
        std::vector<ATLCoarseMemory> coarse_memories = gpu_procs[proc_index].getMemories<ATLCoarseMemory>();
        DEBUG_PRINT("GPU\tFine Memories : %lu\n", fine_memories.size());
        DEBUG_PRINT("\tCoarse Memories : %lu\n", coarse_memories.size());
        all_devices[i].memory_pool_count = fine_memories.size() + coarse_memories.size();
        proc_index++;
    }
    proc_index = 0;
    atl_cpu_kernarg_region.handle=(uint64_t)-1;
    err = hsa_agent_iterate_regions(cpu_procs[0].getAgent(), get_fine_grained_region, &atl_cpu_kernarg_region);
    if(err == HSA_STATUS_INFO_BREAK) { err = HSA_STATUS_SUCCESS; }
    err = (atl_cpu_kernarg_region.handle == (uint64_t)-1) ? HSA_STATUS_ERROR : HSA_STATUS_SUCCESS;
    ErrorCheck(Finding a CPU kernarg memory region handle, err);

    /* Find a memory region that supports kernel arguments.  */
    atl_gpu_kernarg_region.handle=(uint64_t)-1;
    hsa_agent_iterate_regions(gpu_procs[0].getAgent(), get_kernarg_memory_region, &atl_gpu_kernarg_region);
    err = (atl_gpu_kernarg_region.handle == (uint64_t)-1) ? HSA_STATUS_ERROR : HSA_STATUS_SUCCESS;
    ErrorCheck(Finding a kernarg memory region, err);
#endif 
}

void init_hsa() {
    if(atlc.g_hsa_initialized == 0) {
        DEBUG_PRINT("Initializing HSA...");
        hsa_status_t err = hsa_init();
        ErrorCheck(Initializing the hsa runtime, err);
        char * dep_sync_type = getenv("ATMI_DEPENDENCY_SYNC_TYPE");
        if(dep_sync_type == NULL || strcmp(dep_sync_type, "ATMI_SYNC_BARRIER_PKT") == 0) {
            g_dep_sync_type = ATL_SYNC_BARRIER_PKT;
        }
        else if(strcmp(dep_sync_type, "ATMI_SYNC_CALLBACK") == 0) {
            g_dep_sync_type = ATL_SYNC_CALLBACK;
        }
        char * max_signals = getenv("ATMI_MAX_HSA_SIGNALS");
        g_max_signals = 24;
        if(max_signals != NULL)
            g_max_signals = atoi(max_signals);
        init_comute_and_memory();
        init_dag_scheduler();
        atlc.g_hsa_initialized = 1;
        DEBUG_PRINT("done\n");
    }
}

atmi_status_t atl_init_gpu_context() {

    if(atlc.struct_initialized == 0) atmi_init_context_structs();
    if(atlc.g_gpu_initialized != 0) return ATMI_STATUS_SUCCESS;
    
    init_hsa();
    hsa_status_t err;

    int gpu_count = g_atl_machine.getProcessorCount<ATLGPUProcessor>();
    for(int gpu = 0; gpu < gpu_count; gpu++) {
        atmi_place_t place = ATMI_PLACE_GPU(0, gpu);
        ATLGPUProcessor &proc = get_processor<ATLGPUProcessor>(place);
        proc.createQueues(proc.getNumCUs());
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
    init_hsa();
    
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
        int num_queues = proc.getNumCUs();
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

hsa_status_t create_kernarg_memory(hsa_executable_t executable, hsa_executable_symbol_t symbol, void *data) {
    int gpu = *(int *)data;
    hsa_symbol_kind_t type;

    uint32_t name_length;
    hsa_status_t err;
    err = hsa_executable_symbol_get_info(symbol, HSA_EXECUTABLE_SYMBOL_INFO_TYPE, &type); 
    ErrorCheck(Symbol info extraction, err);
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
    else {
        err = hsa_executable_symbol_get_info(symbol, HSA_EXECUTABLE_SYMBOL_INFO_NAME_LENGTH, &name_length); 
        ErrorCheck(Symbol info extraction, err);
        char *name = (char *)malloc(name_length + 1);
        err = hsa_executable_symbol_get_info(symbol, HSA_EXECUTABLE_SYMBOL_INFO_NAME, name); 
        ErrorCheck(Symbol info extraction, err);
        name[name_length] = 0;

        DEBUG_PRINT("Symbol name: %s\n", name);
    }
    return HSA_STATUS_SUCCESS;
}

atmi_status_t atmi_module_register_from_memory(void **modules, size_t *module_sizes, atmi_platform_type_t *types, const int num_modules) {
    std::vector<std::string> modules_str;
    for(int i = 0; i < num_modules; i++) {
        modules_str.push_back(std::string((char *)modules[i]));
    }
    
    int gpu_count = g_atl_machine.getProcessorCount<ATLGPUProcessor>();
    KernelInfoTable.resize(gpu_count);
    for(int gpu = 0; gpu < gpu_count; gpu++) 
    {
        atmi_place_t place = ATMI_PLACE_GPU(0, gpu);
        ATLGPUProcessor &proc = get_processor<ATLGPUProcessor>(place);
        hsa_agent_t agent = proc.getAgent();
        hsa_executable_t executable = {0}; 
        hsa_status_t err;
        hsa_profile_t agent_profile;

        DEBUG_PRINT("Registering module...");
        err = hsa_agent_get_info(agent, HSA_AGENT_INFO_PROFILE, &agent_profile);
        ErrorCheck(Query the agent profile, err);
        // FIXME: Assume that every profile is FULL until we understand how to build BRIG with base profile
        agent_profile = HSA_PROFILE_FULL;
        /* Create the empty executable.  */
        err = hsa_executable_create(agent_profile, HSA_EXECUTABLE_STATE_UNFROZEN, "", &executable);
        ErrorCheck(Create the executable, err);

        for(int i = 0; i < num_modules; i++) {
            void *module_bytes = modules[i];
            size_t module_size = module_sizes[i];
            if(types[i] == BRIG) {
                hsa_ext_module_t module = (hsa_ext_module_t)module_bytes;

                hsa_ext_program_t program;
                /* Create hsa program.  */
                memset(&program,0,sizeof(hsa_ext_program_t));
                err = hsa_ext_program_create(HSA_MACHINE_MODEL_LARGE, agent_profile, HSA_DEFAULT_FLOAT_ROUNDING_MODE_DEFAULT, NULL, &program);
                ErrorCheck(Create the program, err);

                /* Add the BRIG module to hsa program.  */
                err = hsa_ext_program_add_module(program, module);
                ErrorCheck(Adding the brig module to the program, err);
                /* Determine the agents ISA.  */
                hsa_isa_t isa;
                err = hsa_agent_get_info(agent, HSA_AGENT_INFO_ISA, &isa);
                ErrorCheck(Query the agents isa, err);

                /* * Finalize the program and extract the code object.  */
                hsa_ext_control_directives_t control_directives;
                memset(&control_directives, 0, sizeof(hsa_ext_control_directives_t));
                hsa_code_object_t code_object;
                err = hsa_ext_program_finalize(program, isa, 0, control_directives, "-O2", HSA_CODE_OBJECT_TYPE_PROGRAM, &code_object);
                ErrorCheck(Finalizing the program, err);

                /* Destroy the program, it is no longer needed.  */
                err=hsa_ext_program_destroy(program);
                ErrorCheck(Destroying the program, err);

                /* Load the code object.  */
                err = hsa_executable_load_code_object(executable, agent, code_object, "");
                ErrorCheck(Loading the code object, err);
            }
            else if (types[i] == AMDGCN) {
                // Deserialize code object.
                hsa_code_object_t code_object = {0};
                err = hsa_code_object_deserialize(module_bytes, module_size, NULL, &code_object);
                ErrorCheck(Code Object Deserialization, err);
                assert(0 != code_object.handle);

                /* Load the code object.  */
                err = hsa_executable_load_code_object(executable, agent, code_object, NULL);
                ErrorCheck(Loading the code object, err);
            }
        }

        /* Freeze the executable; it can now be queried for symbols.  */
        err = hsa_executable_freeze(executable, "");
        ErrorCheck(Freeze the executable, err);

        err = hsa_executable_iterate_symbols(executable, create_kernarg_memory, &gpu); 
        ErrorCheck(Iterating over symbols for execuatable, err);

        // save the executable and destroy during finalize
        g_executables.push_back(executable);
    }
    DEBUG_PRINT("done\n");
    //ModuleMap[executable.handle] = modules_str;
    return ATMI_STATUS_SUCCESS;
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

    return atmi_module_register_from_memory(&modules[0], &module_sizes[0], types, num_modules);
    for(int i = 0; i < num_modules; i++) {
        free(modules[i]);
    }
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
    pthread_mutex_lock(m);
}

void unlock(pthread_mutex_t *m) {
    //printf("Unlocked Mutex: %p\n", m);
    pthread_mutex_unlock(m);
}

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

template<typename T>
void clear_container(T &q)
{
   T empty;
   std::swap(q, empty);
}
      
atmi_status_t atmi_kernel_create_empty(atmi_kernel_t *atmi_kernel, const int num_args,
                                    const size_t *arg_sizes) {
    static int counter = 0;
    char *pif = (char *)malloc(256);
    memset(pif, 0, 256);
    sprintf(pif, "num%d", counter);
    atmi_kernel->handle = (uint64_t)pif;
    std::string pif_name_str = std::string((const char *)(atmi_kernel->handle));
    counter++;
    
    atl_kernel_t *kernel = new atl_kernel_t; 
    kernel->id_map.clear();
    kernel->num_args = num_args;
    for(int i = 0; i < num_args; i++) {
        kernel->arg_sizes.push_back(arg_sizes[i]);
    }
    clear_container(kernel->impls);
    KernelImplMap[pif_name_str] = kernel;
    return ATMI_STATUS_SUCCESS;
}

atmi_status_t atmi_kernel_release(atmi_kernel_t atmi_kernel) {
    char *pif = (char *)(atmi_kernel.handle);
    
    atl_kernel_t *kernel = KernelImplMap[std::string(pif)];
    //kernel->id_map.clear();
    clear_container(kernel->arg_sizes);
    for(std::vector<atl_kernel_impl_t *>::iterator it = kernel->impls.begin(); 
                        it != kernel->impls.end(); it++) {
        lock(&((*it)->mutex));
        if((*it)->devtype == ATMI_DEVTYPE_GPU) {
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

    KernelImplMap.erase(std::string(pif));
    free(pif);
    atmi_kernel.handle = 0ull;
    return ATMI_STATUS_SUCCESS;
}

atmi_status_t atmi_kernel_add_gpu_impl(atmi_kernel_t atmi_kernel, const char *impl, const unsigned int ID) {
    const char *pif_name = (const char *)(atmi_kernel.handle);
    atl_kernel_t *kernel = KernelImplMap[std::string(pif_name)];
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
    kernel_impl->devtype = ATMI_DEVTYPE_GPU;
   
    std::vector<ATLGPUProcessor> &gpu_procs = g_atl_machine.getProcessors<ATLGPUProcessor>(); 
    int gpu_count = gpu_procs.size();
    kernel_impl->kernel_objects = (uint64_t *)malloc(sizeof(uint64_t) * gpu_count); 
    kernel_impl->group_segment_sizes = (uint32_t *)malloc(sizeof(uint32_t) * gpu_count); 
    kernel_impl->private_segment_sizes = (uint32_t *)malloc(sizeof(uint32_t) * gpu_count); 
    int max_kernarg_segment_size = 0;
    std::string kernel_name; 
    atmi_platform_type_t kernel_type;
    for(int gpu = 0; gpu < gpu_count; gpu++) {
        if(KernelInfoTable[gpu].find(hsaco_name) != KernelInfoTable[gpu].end()) {
            kernel_name = hsaco_name;
            kernel_type = AMDGCN;
        }
        else if(KernelInfoTable[gpu].find(brig_name) != KernelInfoTable[gpu].end()) {
            kernel_name = brig_name;
            kernel_type = BRIG;
        }
        else {
            printf("Did NOT find kernel %s or %s for GPU %d\n", 
                    hsaco_name.c_str(),
                    brig_name.c_str(),
                    gpu);
            return ATMI_STATUS_ERROR;
        }
        atl_kernel_info_t info = KernelInfoTable[gpu][kernel_name];
        kernel_impl->kernel_objects[gpu] = info.kernel_object;
        kernel_impl->group_segment_sizes[gpu] = info.group_segment_size;
        kernel_impl->private_segment_sizes[gpu] = info.private_segment_size;
        if(max_kernarg_segment_size < info.kernel_segment_size)
            max_kernarg_segment_size = info.kernel_segment_size;
    }
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
    hsa_status_t err = hsa_amd_memory_pool_allocate(atl_gpu_kernarg_pool, 
            kernel_impl->kernarg_segment_size * MAX_NUM_KERNELS, 
            0,
            &(kernel_impl->kernarg_region));
    ErrorCheck(Allocating memory for the executable-kernel, err);
    allow_access_to_all_gpu_agents(kernel_impl->kernarg_region);

#endif
    for(int i = 0; i < MAX_NUM_KERNELS; i++) {
        kernel_impl->free_kernarg_segments.push(i);
    }
    pthread_mutex_init(&(kernel_impl->mutex), NULL);
    
    kernel->id_map[ID] = kernel->impls.size();

    kernel->impls.push_back(kernel_impl);
    // rest of kernel impl fields will be populated at first kernel launch
}

atmi_status_t atmi_kernel_add_cpu_impl(atmi_kernel_t atmi_kernel, atmi_generic_fp impl, const unsigned int ID) {
    static int counter = 0;
    const char *pif_name = (const char *)(atmi_kernel.handle);
    std::string cl_pif_name("_x86_");
    cl_pif_name += std::to_string(counter);
    cl_pif_name += std::string("_");
    cl_pif_name += std::string(pif_name);
    counter++;

    atl_kernel_impl_t *kernel_impl = new atl_kernel_impl_t;
    kernel_impl->kernel_id = ID;
    kernel_impl->kernel_name = cl_pif_name;
    kernel_impl->devtype = ATMI_DEVTYPE_CPU;
    kernel_impl->function = impl;
    
    atl_kernel_t *kernel = KernelImplMap[std::string(pif_name)];
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
}

bool is_valid_kernel_id(atl_kernel_t *kernel, unsigned int kernel_id) {
    std::map<unsigned int, unsigned int>::iterator it = kernel->id_map.find(kernel_id);
    if(it == kernel->id_map.end()) {
        fprintf(stderr, "Kernel ID %d not found\n", kernel_id);
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
    set_task_state(task, ATMI_COMPLETED);
    set_task_metrics(task);
    unlock(&(task->mutex));

    // after predecessor is done, decrement all successor's dependency count. 
    // If count reaches zero, then add them to a 'ready' task list. Next, 
    // dispatch all ready tasks in a round-robin manner to the available 
    // GPU/CPU queues. 
    // decrement reference count of its dependencies; add those with ref count = 0 to a
    // “ready” list
    atl_task_vector_t &deps = task->and_successors;
    DEBUG_PRINT("Deps list of %d [%d]: ", task->id, deps.size());
    atl_task_vector_t temp_list;
    for(atl_task_vector_t::iterator it = deps.begin();
            it != deps.end(); it++) {
        // FIXME: should we be grabbing a lock on each successor before
        // decrementing their predecessor count? Currently, it may not be
        // required because there is only one callback thread, but what if there
        // were more? 
        lock(&((*it)->mutex));
        DEBUG_PRINT(" %d(%d) ", (*it)->id, (*it)->num_predecessors);
        (*it)->num_predecessors--;
        if((*it)->num_predecessors == 0) {
            // add to ready list
            temp_list.push_back(*it);
        }
        unlock(&((*it)->mutex));
    }
    std::vector<pthread_mutex_t *> mutexes;
    // release the kernarg segment back to the kernarg pool
    atl_kernel_t *kernel = task->kernel;
    atl_kernel_impl_t *kernel_impl = NULL;
    if(kernel) {
        kernel_impl = get_kernel_impl(kernel, task->kernel_id);
        mutexes.push_back(&(kernel_impl->mutex));
    }
    mutexes.push_back(&mutex_readyq_);
    lock_vec(mutexes);
    DEBUG_PRINT("\n");
    for(atl_task_vector_t::iterator it = temp_list.begin();
            it != temp_list.end(); it++) {
        ReadyTaskQueue.push(*it);
    }
    // do not release Stream signals into the pool
    // but stream signals should not be in the handlers
    // anyway
    if(task->groupable != ATMI_TRUE)
        FreeSignalPool.push(task->signal);
    DEBUG_PRINT("Freeing Kernarg Segment Id: %d\n", task->kernarg_region_index);
    if(kernel) kernel_impl->free_kernarg_segments.push(task->kernarg_region_index);
    unlock_vec(mutexes);
    DEBUG_PRINT("[Handle Signal %d ] Free Signal Pool Size: %lu; Ready Task Queue Size: %lu\n", task->id, FreeSignalPool.size(), ReadyTaskQueue.size());
    // dispatch from ready queue if any task exists
    dispatch_ready_task_or_release_signal(task);
}

void handle_signal_barrier_pkt(atl_task_t *task) {
    // tasks without atmi_task handle should not be added to callbacks anyway
    assert(task->groupable != ATMI_TRUE);
    std::vector<pthread_mutex_t *> mutexes;
    // release the kernarg segment back to the kernarg pool
    atl_kernel_t *kernel = task->kernel;
    atl_kernel_impl_t *kernel_impl = NULL; 
    if(kernel) {
        kernel_impl = get_kernel_impl(kernel, task->kernel_id);
        mutexes.push_back(&(kernel_impl->mutex));
    }
    mutexes.push_back(&(task->mutex));
    mutexes.push_back(&mutex_readyq_);
    atl_task_vector_t &requires = task->and_predecessors;
    for(int idx = 0; idx < requires.size(); idx++) {
        mutexes.push_back(&(requires[idx]->mutex));
    }
    
    lock_vec(mutexes);

    DEBUG_PRINT("Freeing Kernarg Segment Id: %d\n", task->kernarg_region_index);
    if(kernel) kernel_impl->free_kernarg_segments.push(task->kernarg_region_index);

    DEBUG_PRINT("{%d}\n", task->id);
    set_task_state(task, ATMI_COMPLETED);
    set_task_metrics(task);

    // decrement each predecessor's num_successors
    DEBUG_PRINT("Requires list of %d [%d]: ", task->id, requires.size());
    std::vector<hsa_signal_t> temp_list;
    temp_list.clear();
    // if num_successors == 0 then we can reuse their signal. 
    for(atl_task_vector_t::iterator it = requires.begin();
            it != requires.end(); it++) {
        assert((*it)->state >= ATMI_DISPATCHED);
        (*it)->num_successors--;
        if((*it)->state == ATMI_COMPLETED && (*it)->num_successors == 0) {
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
        DEBUG_PRINT("[Handle Barrier_Signal %d ] Free Signal Pool Size: %lu; Ready Task Queue Size: %lu\n", task->id, FreeSignalPool.size(), ReadyTaskQueue.size());
    }
    clear_container(task->barrier_signals);
    for(std::vector<hsa_signal_t>::iterator it = temp_list.begin();
            it != temp_list.end(); it++) {
        //hsa_signal_store_relaxed((*it), 1);
        FreeSignalPool.push(*it);
        DEBUG_PRINT("[Handle Signal %d ] Free Signal Pool Size: %lu; Ready Task Queue Size: %lu\n", task->id, FreeSignalPool.size(), ReadyTaskQueue.size());
    }
    unlock_vec(mutexes);

    dispatch_ready_task_for_free_signal(); 
}

bool handle_signal(hsa_signal_value_t value, void *arg) {
    HandleSignalInvokeTimer.Stop();
    #if 1
    static bool is_called = false;
    if(!is_called) {
        set_thread_affinity(2);
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
    //static int counter = 0;
    if(g_dep_sync_type == ATL_SYNC_CALLBACK) {
        handle_signal_callback(task); 
    }
    else if(g_dep_sync_type == ATL_SYNC_BARRIER_PKT) {
        handle_signal_barrier_pkt(task); 
    }
    HandleSignalTimer.Stop();
    HandleSignalInvokeTimer.Start();
    return false; 
}

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
                DEBUG_PRINT("Callback dispatching next task %p (%d)\n", ready_task, ready_task->id);
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
            iterations++;
        } while((!should_dispatch || (should_dispatch && !should_register_callback)) && iterations < queue_sz);
    }
}

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
            DEBUG_PRINT("Pif requires %d tasks\n", lparm->predecessor.size());
            if ( task->and_predecessors.size() > 0) {
                int val = 0;
                DEBUG_PRINT("(");
                for(size_t count = 0; count < task->and_predecessors.size(); count++) {
                    if(task->and_predecessors[count]->state < ATMI_DISPATCHED) val++;
                    assert(task->and_predecessors[count]->state >= ATMI_DISPATCHED);
                    DEBUG_PRINT("%d ", task->and_predecessors[count]->id);
                }
                DEBUG_PRINT(")\n");
                if(val > 0) DEBUG_PRINT("Task[%d] has %d not-dispatched predecessor tasks\n", task->id, val);
                enqueue_barrier(task, this_Q, task->and_predecessors.size(), &(task->and_predecessors[0]), SNK_NOWAIT, SNK_AND, task->devtype);
            }
            DEBUG_PRINT("%d\n", task->id);
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
            uint64_t index = hsa_queue_add_write_index_relaxed(this_Q, 1);
            atl_kernel_impl_t *kernel_impl = get_kernel_impl(task->kernel, task->kernel_id);
            /* Find the queue index address to write the packet info into.  */
            const uint32_t queueMask = this_Q->size - 1;
            hsa_kernel_dispatch_packet_t* this_aql = &(((hsa_kernel_dispatch_packet_t*)(this_Q->base_address))[index&queueMask]);
            memset(this_aql, 0, sizeof(hsa_kernel_dispatch_packet_t));
            /*  FIXME: We need to check for queue overflow here. */
            //SignalAddTimer.Start();
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
            this_aql->header = create_header(HSA_PACKET_TYPE_KERNEL_DISPATCH, ATMI_FALSE);
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
            hsa_signal_add_acq_rel(task->signal, thread_count);
            for(int tid = 0; tid < thread_count; tid++) {
                hsa_queue_t *this_queue = this_queues[tid % q_count];
                /*  Obtain the current queue write index. increases with each call to kernel  */
                uint64_t index = hsa_queue_add_write_index_relaxed(this_queue, 1);
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
                this_aql->arg[0] = (uint64_t) task;
                this_aql->arg[1] = (uint64_t) task->kernarg_region;
                this_aql->arg[2] = (uint64_t) task->kernel; // pass task handle to fill in metrics
                this_aql->arg[3] = tid; // tasks can query for current task ID

                /*  Prepare and set the packet header */
                /* FIXME: CPU tasks ignore barrier bit as of now. Change
                 * implementation? I think it doesn't matter because we are
                 * executing the subroutines one-by-one, so barrier bit is
                 * inconsequential.
                 */
                this_aql->header = create_header(HSA_PACKET_TYPE_AGENT_DISPATCH, ATMI_FALSE);

            }
            set_task_state(task, ATMI_DISPATCHED);
            /* Store dispatched time */
            if(task->profilable == ATMI_TRUE && task->atmi_task) 
                task->atmi_task->profile.dispatch_time = get_nanosecs(context_init_time, dispatch_time);
            for(int q = 0; q < q_count; q++) {
                /* Increment write index and ring doorbell to dispatch the kernel.  */
                //hsa_queue_store_write_index_relaxed(this_Q, index+1);
                uint64_t index = hsa_queue_load_write_index_acquire(this_queues[q]);
                hsa_signal_store_relaxed(this_queues[q]->doorbell_signal, index);
                signal_worker(this_queues[q], PROCESS_PKT);
            }
        }
        TryDispatchTimer.Stop();
        DEBUG_PRINT("Task %d (%d) Dispatched\n", task->id, task->devtype);
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
        DEBUG_PRINT("Acquiring Kernarg Segment Id: %d\n", free_idx);
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
            *(uint64_t *)thisKernargAddress = (uint64_t)PifKlistMap[ret->kernel->pif_name];
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

    std::vector<pthread_mutex_t *> req_mutexes;
    std::vector<atl_task_t *> &temp_vecs = ret->predecessors;
    req_mutexes.clear();
    for(int idx = 0; idx < ret->predecessors.size(); idx++) {
        atl_task_t *pred_task = temp_vecs[idx];
        req_mutexes.push_back((pthread_mutex_t *)&(pred_task->mutex));
    }
    req_mutexes.push_back((pthread_mutex_t *)&(ret->mutex));
    req_mutexes.push_back(&mutex_readyq_);
    atl_kernel_impl_t *kernel_impl = NULL;
    if(ret->kernel) {
        kernel_impl = get_kernel_impl(ret->kernel, ret->kernel_id);
        if(kernel_impl) req_mutexes.push_back(&(kernel_impl->mutex));
    }
    pthread_mutex_t stream_mutex;
    atmi_task_group_table_t *stream_obj = ret->stream_obj;
    get_stream_mutex(stream_obj, &stream_mutex);
    req_mutexes.push_back(&stream_mutex);
    lock_vec(req_mutexes);
    //std::cout << "[" << ret->id << "]Signals Before: " << FreeSignalPool.size() << std::endl;
    int dep_count = 0;
    for(int idx = 0; idx < ret->predecessors.size(); idx++) {
        atl_task_t *pred_task = temp_vecs[idx]; 
        DEBUG_PRINT("Task %d depends on %d as %d th predecessor ",
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
    int required_tasks = 0;
    for(int idx = 0; idx < ret->predecessors.size(); idx++) {
        atl_task_t *pred_task = temp_vecs[idx]; 
        if(pred_task->state /*.load(std::memory_order_seq_cst)*/ != ATMI_COMPLETED) {
            required_tasks++;
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
            if(signal.handle != (uint64_t)-1) {
                ret->signal = signal;
            }
            else {
                assert(StreamCommonSignalIdx < ATMI_MAX_STREAMS);
                // get the next free signal from the stream common signal pool
                signal = StreamCommonSignalPool[StreamCommonSignalIdx++];
                ret->signal = signal;
                set_stream_signal(stream_obj, signal);
            }
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
            DEBUG_PRINT("Acquiring Kernarg Segment Id: %d\n", free_idx);
            void *addr = (void *)((char *)kernel_impl->kernarg_region + 
                    (free_idx * kernarg_segment_size));
            kernel_impl->free_kernarg_segments.pop();
            if(ret->kernarg_region != NULL) {
                // we had already created a memory region using malloc. Copy it
                // to the newly availed space
                memcpy(addr, ret->kernarg_region, ret->kernarg_region_size);
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
            if(pred_task->state /*.load(std::memory_order_seq_cst)*/ != ATMI_COMPLETED) {
                pred_task->num_successors++;
                DEBUG_PRINT("Task %p (%d) adding %p (%d) as successor\n",
                        pred_task, pred_task->id, ret, ret->id);
                ret->and_predecessors.push_back(pred_task);
            }
        }
    }
    else {
        if(ret->kernel && ret->kernarg_region == NULL) {
            // first time allocation/assignment
            ret->kernarg_region = malloc(ret->kernarg_region_size);
            //ret->kernarg_region_copied = true;
            set_kernarg_region(ret, args);
        }
        ReadyTaskQueue.push(ret);
        max_ready_queue_sz++;
    }
    //std::cout << "[" << ret->id << "]Signals After (" << should_dispatch << "): " << FreeSignalPool.size() << std::endl;
    unlock_vec(req_mutexes);
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

    std::vector<pthread_mutex_t *> req_mutexes;
    std::vector<atl_task_t *> &temp_vecs = ret->predecessors;
    req_mutexes.clear();
    for(int idx = 0; idx < ret->predecessors.size(); idx++) {
        atl_task_t *pred_task = ret->predecessors[idx];
        req_mutexes.push_back((pthread_mutex_t *)&(pred_task->mutex));
    }
    req_mutexes.push_back((pthread_mutex_t *)&(ret->mutex));
    req_mutexes.push_back(&mutex_readyq_);
    pthread_mutex_t stream_mutex;
    atmi_task_group_table_t *stream_obj = ret->stream_obj;
    get_stream_mutex(stream_obj, &stream_mutex);
    req_mutexes.push_back(&stream_mutex);
    atl_kernel_impl_t *kernel_impl = NULL;
    if(ret->kernel) {
        kernel_impl = get_kernel_impl(ret->kernel, ret->kernel_id);
        req_mutexes.push_back(&(kernel_impl->mutex));
    }
    lock_vec(req_mutexes);
   
    if(ret->predecessors.size() > 0) {
        // add to its predecessor's dependents list and return 
/*        std::vector<pthread_mutex_t *> req_mutexes;
        req_mutexes.clear();
        for(int idx = 0; idx < ret->predecessors.size(); idx++) {
            atl_task_t *pred_task = ret->predecessors[idx];
            req_mutexes.push_back((pthread_mutex_t *)&(pred_task->mutex));
        }
        req_mutexes.push_back((pthread_mutex_t *)&(ret->mutex));
        lock_vec(req_mutexes);
*/
        for(int idx = 0; idx < ret->predecessors.size(); idx++) {
            atl_task_t *pred_task = ret->predecessors[idx];
            DEBUG_PRINT("Task %p depends on %p as predecessor ",
                    ret, pred_task);
            if(pred_task->state /*.load(std::memory_order_seq_cst)*/ != ATMI_COMPLETED) {
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
        //unlock_vec(req_mutexes);
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
            /*pthread_mutex_t stream_mutex;
              atmi_task_group_table_t *stream_obj = ret->stream_obj;
              get_stream_mutex(stream_obj, &stream_mutex);
              lock(&stream_mutex);*/
            hsa_signal_t signal;
            get_stream_signal(stream_obj, &signal);
            if(signal.handle != (uint64_t)-1) {
                ret->signal = signal;
            }
            else {
                assert(StreamCommonSignalIdx < ATMI_MAX_STREAMS);
                // get the next free signal from the stream common signal pool
                signal = StreamCommonSignalPool[StreamCommonSignalIdx++];
                ret->signal = signal;
                set_stream_signal(stream_obj, signal);
            }
            //unlock(&stream_mutex);
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
            void *addr = (void *)((char *)kernel_impl->kernarg_region + 
                    (free_idx * kernarg_segment_size));
            kernel_impl->free_kernarg_segments.pop();
            if(ret->kernarg_region != NULL) {
                // we had already created a memory region using malloc. Copy it
                // to the newly availed space
                memcpy(addr, ret->kernarg_region, ret->kernarg_region_size);
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
    }
    else {
        if(ret->kernel && ret->kernarg_region == NULL) {
            // first time allocation/assignment
            ret->kernarg_region = malloc(ret->kernarg_region_size);
            //ret->kernarg_region_copied = true;
            set_kernarg_region(ret, args);
        }
    }
    if(predecessors_complete == true && resources_available == false) {
        // Ready task but no resources available. So, we push it to a
        // ready queue
        ReadyTaskQueue.push(ret);
        max_ready_queue_sz++;
    }

    unlock_vec(req_mutexes);
    return should_try_dispatch;
}

atl_task_t *get_new_task() {
    atl_task_t *ret = new atl_task_t;
    memset(ret, 0, sizeof(atl_task_t));
    //ret->is_continuation = false;
    lock(&mutex_all_tasks_);
    AllTasks.push_back(ret);
    set_task_state(ret, ATMI_INITIALIZED);
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
    pthread_mutex_init(&(ret->mutex), NULL);
    return ret;
}

void try_dispatch(atl_task_t *ret, void **args, bool synchronous) {
    bool should_dispatch = true;
    bool should_register_callback = true;
    if(g_dep_sync_type == ATL_SYNC_CALLBACK) {
        should_dispatch = try_dispatch_callback(ret, args);
        should_register_callback = (ret->groupable != ATMI_TRUE);
    }
    else if(g_dep_sync_type == ATL_SYNC_BARRIER_PKT) {
        should_dispatch = try_dispatch_barrier_pkt(ret, args);
        should_register_callback = ((ret->groupable != ATMI_TRUE) ||  
                (ret->groupable == ATMI_TRUE && !(ret->and_predecessors.empty())));
    }
    
    if(should_dispatch) {
        if(ret->atmi_task) {
            // FIXME: set a lookup table for dynamic parallelism kernels
            // perhaps?
            ret->atmi_task->handle = (void *)(&(ret->signal));
        }
        // acquire_kernel_impl_segment
        // update kernarg ptrs
        //direct_dispatch++;
        if(ret->type == ATL_KERNEL_EXECUTION) 
            dispatch_task(ret);
        else 
            dispatch_data_movement(ret, ret->data_dest_ptr, ret->data_src_ptr, ret->data_size);

        if(should_register_callback) {
        RegisterCallbackTimer.Start();
            hsa_status_t err = hsa_amd_signal_async_handler(ret->signal,
                    HSA_SIGNAL_CONDITION_EQ, 0,
                    handle_signal, (void *)ret);
            ErrorCheck(Creating signal handler, err);
        RegisterCallbackTimer.Stop();
        }
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
        set_task_state(ret, ATMI_COMPLETED);
        set_task_metrics(ret);
      //  TaskWaitTimer.Stop();
        //std::cout << "Task Wait Interim Timer " << TaskWaitTimer << std::endl;
        //std::cout << "Launch Time: " << TryLaunchTimer << std::endl;
    }
    else {
        /* add task to the corresponding row in the stream table */
        if(ret->groupable != ATMI_TRUE)
            register_task(ret->stream_obj, ret);
    }
}

atmi_task_handle_t atl_trylaunch_kernel(const atmi_lparm_t *lparm,
                 atl_kernel_t *kernel,
                 unsigned int kernel_id,
                 void **args) {
    TryLaunchInitTimer.Start();
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
    atl_task_t *ret = NULL;
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
    ret = get_new_task();
#endif
    TryLaunchInitTimer.Stop();
    ret->kernel = kernel;
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
    DEBUG_PRINT("Requires LHS: %p and RHS: %p\n", ret->lparm.requires, lparm->requires);
    DEBUG_PRINT("Requires ThisTask: %p and ThisTask: %p\n", ret->lparm.task_info, lparm->task_info);

    ret->group = stream;
    ret->stream_obj = stream_obj;
    ret->place = lparm->place;
    DEBUG_PRINT("Stream LHS: %p and RHS: %p\n", ret->lparm.group, lparm->group);
    ret->num_predecessors = 0;
    ret->num_successors = 0;

    /* For dependent child tasks, add dependent parent kernels to barriers.  */
    DEBUG_PRINT("Pif %s requires %d task\n", kernel_impl->kernel_name.c_str(), lparm->num_required);

    ShouldDispatchTimer.Start();
    ret->predecessors.clear();
    ret->predecessors.resize(lparm->num_required);
    ShouldDispatchTimer.Stop();
    for(int idx = 0; idx < lparm->num_required; idx++) {
        atl_task_t *pred_task = get_task(lparm->requires[idx]);
        assert(pred_task != NULL);
        ret->predecessors[idx] = pred_task;
    }

    ret->type = ATL_KERNEL_EXECUTION;
    ret->data_src_ptr = NULL;
    ret->data_dest_ptr = NULL;
    ret->data_size = 0;

    try_dispatch(ret, args, lparm->synchronous);

    return ret->id;
}

atmi_task_handle_t atmi_task_launch(atmi_lparm_t *lparm, atmi_kernel_t atmi_kernel,  
                                    void **args/*, more params for place info? */) {
    ParamsInitTimer.Start();
    atmi_task_handle_t ret = NULL_TASK;
    const char *pif_name = (const char *)(atmi_kernel.handle);
    std::string pif_name_str = std::string(pif_name);
    std::map<std::string, atl_kernel_t *>::iterator map_iter;
    map_iter = KernelImplMap.find(pif_name_str);
    if(map_iter == KernelImplMap.end()) {
        fprintf(stderr, "ERROR: Kernel/PIF %s not found\n", pif_name);
        return NULL_TASK;
    }
    atl_kernel_t *kernel = map_iter->second;
    kernel->pif_name = pif_name_str;
    int this_kernel_iter = 0;
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
            fprintf(stderr, "ERROR: Kernel/PIF %s doesn't have any implementations\n", 
                    pif_name);
            return NULL_TASK;
        }
    }
    else {
        if(!is_valid_kernel_id(kernel, kernel_id)) {
            fprintf(stderr, "ERROR: Kernel/PIF %s doesn't have %d implementations\n", 
                    pif_name, kernel_id + 1);
            return NULL_TASK;
        }
    }
    atl_kernel_impl_t *kernel_impl = get_kernel_impl(kernel, kernel_id);
    if(kernel->num_args && kernel_impl->kernarg_region == NULL) {
        fprintf(stderr, "ERROR: Kernel Arguments not initialized for Kernel %s\n", 
                            kernel_impl->kernel_name.c_str());
        return NULL_TASK;
    }
    atmi_devtype_t devtype = kernel_impl->devtype;
    /*lock(&(kernel_impl->mutex));
    if(kernel_impl->free_kernarg_segments.empty()) {
        // no free kernarg segments -- allocate some more? 
        // FIXME: realloc instead? HSA realloc?
    }
    unlock(&(kernel_impl->mutex));
    */
    ParamsInitTimer.Stop();
    TryLaunchTimer.Start();
    ret = atl_trylaunch_kernel(lparm, kernel, kernel_id, args);
    TryLaunchTimer.Stop();
    DEBUG_PRINT("[Returned Task: %lu]\n", ret);
    return ret;
}

enum queue_type{device_queue = 0, soft_queue}; 
void atl_kl_init(atmi_klist_t *atmi_klist,
        atmi_kernel_t atmi_kernel,
        const int pif_id) {
    const char *pif_name = (const char *)(atmi_kernel.handle);
    atmi_task_group_t *stream  = &atl_default_stream_obj;

    atl_kernel_t *kernel = KernelImplMap[std::string(pif_name)];
    /* Add row to stream table for purposes of future synchronizations */
    register_stream(stream);
    atmi_task_group_table_t *stream_obj = StreamTable[stream->id];

    atmi_klist_t *atmi_klist_curr = atmi_klist + pif_id; 

    atl_task_t **task_ptr = &AllTasks[0];
    memcpy(GlobalTaskPtr, &task_ptr, sizeof(atl_task_t **));
    atmi_klist_curr->tasks = (void *)(*GlobalTaskPtr);
    /* get this stream's HSA queue (could be dynamically mapped or round robin
     * if it is an unordered stream */
    int gpu = 0;
    int cpu = 0;
    #if 1
    hsa_queue_t* this_devQ = acquire_and_set_next_gpu_queue(stream_obj, ATMI_PLACE_GPU(0, gpu));
    if(!this_devQ) return;

    hsa_queue_t* this_softQ = acquire_and_set_next_cpu_queue(stream_obj, ATMI_PLACE_CPU(0, cpu));
    if(!this_softQ) return;

    atmi_klist_curr->queues[device_queue] = (uint64_t)this_devQ; 
    atmi_klist_curr->queues[soft_queue] = (uint64_t)this_softQ; 
    atmi_klist_curr->worker_sig = (uint64_t)get_worker_sig(this_softQ); 
    #else
    atmi_klist_curr->cpu_queue_offset = 0;
    atmi_klist_curr->gpu_queue_offset = 0;
    atmi_klist_curr->num_cpu_queues = SNK_MAX_CPU_QUEUES;
    atmi_klist_curr->num_gpu_queues = SNK_MAX_GPU_QUEUES;
    atmi_klist_curr->cpu_queues = (uint64_t *)malloc(sizeof(uint64_t) * atmi_klist_curr->num_cpu_queues);
    atmi_klist_curr->gpu_queues = (uint64_t *)malloc(sizeof(uint64_t) * atmi_klist_curr->num_gpu_queues);

    for(int qid = 0; qid < SNK_MAX_CPU_QUEUES; qid++) {
        atmi_klist_curr->cpu_queues[qid] = (long unsigned int)get_cpu_queue(qid); 
    }
    for(int qid = 0; qid < SNK_MAX_GPU_QUEUES; qid++) {
        atmi_klist_curr->gpu_queues[qid] = (long unsigned int)GPU_CommandQ[qid]; 
    }
    #endif

    uint64_t _KN__Kernel_Object;
    uint32_t _KN__Group_Segment_Size;
    uint32_t _KN__Private_Segment_Size;
    uint32_t _KN__Kernarg_Size;

    /* Allocate the kernel argument buffer from the correct region. */
    //void* thisKernargAddress;
    //atl_gpu_memory_allocate(lparm, g_executable, pif_name, &thisKernargAddress);

    uint16_t i = 0;
    for(std::vector<atl_kernel_impl_t *>::iterator it = kernel->impls.begin();
                                                 it != kernel->impls.end(); it++) {
        if((*it)->devtype == ATMI_DEVTYPE_GPU) {
            if(PifKlistMap.find(std::string(pif_name)) == PifKlistMap.end()) {
                PifKlistMap[std::string(pif_name)] = atmi_klist;
            }

            // FIXME: change this to specific GPU ids for DP from different GPUs
            _KN__Kernel_Object = (*it)->kernel_objects[gpu];
            _KN__Group_Segment_Size = (*it)->group_segment_sizes[gpu];
            _KN__Private_Segment_Size = (*it)->private_segment_sizes[gpu];
            _KN__Kernarg_Size = (*it)->kernarg_segment_size;
            DEBUG_PRINT("Kernel GPU memalloc. Kernel %s needs %" PRIu32" bytes for kernargs\n", (*it)->kernel_name.c_str(), (*it)->kernarg_segment_size); 

            atmi_klist_curr->num_kernel_packets++;
            #if 0
            if(atmi_klist_curr->kernel_packets) {
                atmi_free(atmi_klist_curr->kernel_packets);
            }
            atmi_malloc((void **)&(atmi_klist_curr->kernel_packets), 0, 
                    sizeof(atmi_kernel_packet_t) * 
                    atmi_klist_curr->num_kernel_packets);
            #else
                atmi_klist_curr->kernel_packets = 
                (atmi_kernel_packet_t *)realloc(
                        atmi_klist_curr->kernel_packets, 
                        sizeof(atmi_kernel_packet_t) * 
                        atmi_klist_curr->num_kernel_packets);
            #endif
            hsa_kernel_dispatch_packet_t *this_aql = 
                (hsa_kernel_dispatch_packet_t *)(atmi_klist_curr
                        ->kernel_packets +
                        atmi_klist_curr->num_kernel_packets - 1);


            /* thisKernargAddress has already been set up in the beginning of this routine */
            /*  Bind kernel argument buffer to the aql packet.  */
            this_aql->header = 0;
            //this_aql->kernarg_address = (void*) thisKernargAddress;
            this_aql->kernel_object = _KN__Kernel_Object;
            this_aql->private_segment_size = _KN__Private_Segment_Size;
            this_aql->group_segment_size = _KN__Group_Segment_Size;

        }
        else if((*it)->devtype == ATMI_DEVTYPE_CPU){
            atmi_klist_curr->num_kernel_packets++;
            #if 0
            if(atmi_klist_curr->kernel_packets) {
                atmi_free(atmi_klist_curr->kernel_packets);
            }
            atmi_malloc((void **)&(atmi_klist_curr->kernel_packets), 0, 
                    sizeof(atmi_kernel_packet_t) * 
                    atmi_klist_curr->num_kernel_packets);
            #else
                atmi_klist_curr->kernel_packets = 
                (atmi_kernel_packet_t *)realloc(
                        atmi_klist_curr->kernel_packets, 
                        sizeof(atmi_kernel_packet_t) * 
                        atmi_klist_curr->num_kernel_packets);
            #endif
            hsa_agent_dispatch_packet_t *this_aql = 
                (hsa_agent_dispatch_packet_t *)(atmi_klist_curr
                        ->kernel_packets +
                        atmi_klist_curr->num_kernel_packets - 1);
            this_aql->header = 1;
            this_aql->type = (uint16_t)i;
            //const uint32_t num_params = kernel->num_args;
            //this_aql->arg[0] = (uint64_t) task;
            //this_aql->arg[1] = (uint64_t) task->kernarg_region;
            this_aql->arg[2] = (uint64_t) kernel;
            this_aql->arg[3] = UINT64_MAX; 
        }
        i++;
    }
}

/* Machine Info */
atmi_machine_t *atmi_machine_get_info() {
    if(!atlc.g_hsa_initialized) return NULL;
    return &g_atmi_machine;
}


