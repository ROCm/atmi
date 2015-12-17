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
#include "RealTimerClass.h"
using namespace Global;
pthread_mutex_t mutex_all_tasks_;
pthread_mutex_t mutex_readyq_;
RealTimer SignalAddTimer;
RealTimer HandleSignalTimer;
RealTimer HandleSignalInvokeTimer;
RealTimer TaskWaitTimer;
RealTimer TryLaunchTimer;
RealTimer TryDispatchTimer;
static size_t max_ready_queue_sz = 0;
static size_t waiting_count = 0;
static size_t direct_dispatch = 0;
static size_t callback_dispatch = 0;
#define NSECPERSEC 1000000000L

//  set NOTCOHERENT needs this include
#include "hsa_ext_amd.h"

/* -------------- Helper functions -------------------------- */
#define ErrorCheck(msg, status) \
if (status != HSA_STATUS_SUCCESS) { \
    printf("%s failed. 0x%x\n", #msg, status); \
    /*exit(1); */\
} else { \
 /*  printf("%s succeeded.\n", #msg);*/ \
}

extern bool handle_signal(hsa_signal_value_t value, void *arg);

void print_atl_kernel(const char * str, const int i);
/* Stream table to hold the runtime state of the 
 * stream and its tasks. Which was the latest 
 * device used, latest queue used and also a 
 * pool of tasks for synchronization if need be */
std::map<atmi_stream_t *, atmi_stream_table_t *> StreamTable;
//atmi_task_table_t TaskTable[SNK_MAX_TASKS];

std::vector<atl_task_t *> AllTasks;
std::queue<atl_task_t *> ReadyTaskQueue;
std::queue<hsa_signal_t> FreeSignalPool;
std::map<atmi_task_t *, atl_task_t *> PublicTaskMap;
std::map<std::string, atl_kernel_info_t> KernelInfoTable;
std::map<uint64_t, std::vector<std::string> > ModuleMap;
hsa_signal_t StreamCommonSignalPool[ATMI_MAX_STREAMS];
hsa_signal_t IdentityORSignal;
hsa_signal_t IdentityANDSignal;
static int StreamCommonSignalIdx = 0;
std::queue<atl_task_t *> DispatchedTasks;

static atl_dep_sync_t g_dep_sync_type;
static int g_max_signals;
/* Stream specific globals */
hsa_agent_t atl_cpu_agent;
hsa_ext_program_t atl_hsa_program;
//hsa_executable_t atl_executable;
hsa_region_t atl_gpu_KernargRegion;
hsa_region_t atl_cpu_KernargRegion;
hsa_agent_t atl_gpu_agent;
hsa_queue_t* GPU_CommandQ[SNK_MAX_GPU_QUEUES];
int          SNK_NextTaskId = 0 ;
atmi_stream_t atl_default_stream_obj = {ATMI_FALSE};
int          SNK_NextGPUQueueID[ATMI_MAX_STREAMS];
int          SNK_NextCPUQueueID[ATMI_MAX_STREAMS];

extern atl_pif_kernel_table_t atl_kernels[SNK_MAX_FUNCTIONS];

struct timespec context_init_time;
static int context_init_time_init = 0;

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
            //DEBUG_PRINT("Signal handle: %" PRIu64 " Signal value:%ld\n", task->signal.handle, hsa_signal_load_relaxed(task->signal));
            hsa_signal_wait_acquire(task->signal, HSA_SIGNAL_CONDITION_EQ, 0, UINT64_MAX, ATMI_WAIT_STATE);
        }
        /* Flag this task as completed */
        /* FIXME: How can HSA tell us if and when a task has failed? */
        set_task_state(task, ATMI_COMPLETED);
    }

    return;// ATMI_STATUS_SUCCESS;
}

extern atmi_status_t atmi_task_wait(atmi_task_t *task) {
    atl_task_wait(PublicTaskMap[task]);
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

void init_dag_scheduler() {
    if(atlc.g_mutex_dag_initialized == 0) {
        pthread_mutex_init(&mutex_all_tasks_, NULL);
        pthread_mutex_init(&mutex_readyq_, NULL);
        AllTasks.clear();
        AllTasks.reserve(500000);
        PublicTaskMap.clear();
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

void set_task_metrics(atl_task_t *task, atmi_devtype_t devtype, boolean profilable) {
    hsa_status_t err;
    //if(task->profile != NULL) {
    if(profilable == ATMI_TRUE) {
        hsa_signal_t signal = task->signal;
        hsa_amd_profiling_dispatch_time_t metrics;
        if(devtype == ATMI_DEVTYPE_GPU) {
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

extern void atl_stream_sync(atmi_stream_table_t *stream_obj) {
    if(stream_obj->ordered == ATMI_TRUE) {
        /* just insert a barrier packet to the CPU and GPU queues and wait */
        DEBUG_PRINT("Waiting for GPU Q\n");
        queue_sync(stream_obj->gpu_queue);
        DEBUG_PRINT("Waiting for CPU Q\n");
        queue_sync(stream_obj->cpu_queue);
        if(stream_obj->cpu_queue) 
            signal_worker(stream_obj->cpu_queue, PROCESS_PKT);
        atl_task_list_t *task_head = stream_obj->tasks;
        DEBUG_PRINT("Waiting for async ordered tasks\n");
        if(stream_obj->tasks == NULL ) return;
        while(task_head) {
            set_task_state(task_head->task, ATMI_COMPLETED);
            set_task_metrics(task_head->task, task_head->devtype, task_head->profilable);
            task_head = task_head->next;
        }
    }
    else {
        /* wait on each one of the tasks in the task bag */
        #if 1
        DEBUG_PRINT("Waiting for async unordered tasks\n");
        hsa_signal_t signal = stream_obj->stream_common_signal;
        if(signal.handle != (uint64_t)-1) hsa_signal_wait_acquire(signal, HSA_SIGNAL_CONDITION_EQ, 0, UINT64_MAX, ATMI_WAIT_STATE);
/*
        atl_task_list_t *task_head = stream_obj->tasks;
        if(stream_obj->tasks == NULL ) return;
        while(task_head) {
            atl_task_wait(task_head->task);
            set_task_metrics(task_head->task, task_head->devtype, task_head->profilable);
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

extern void atmi_stream_sync(atmi_stream_t *stream) {
    atmi_stream_t *str = (stream == NULL) ? &atl_default_stream_obj : stream;
    atmi_stream_table_t *stream_obj = StreamTable[stream];
    atl_stream_sync(stream_obj);
}

hsa_queue_t *acquire_and_set_next_cpu_queue(atmi_stream_table_t *stream_obj) {
    int ret_queue_id = stream_obj->next_cpu_qid;
    DEBUG_PRINT("Q ID: %d\n", ret_queue_id);
    /* use the same queue if the stream is ordered */
    /* otherwise, round robin the queue ID for unordered streams */
    if(stream_obj->ordered == ATMI_FALSE) {
        stream_obj->next_cpu_qid = (ret_queue_id + 1) % SNK_MAX_CPU_QUEUES;
    }
    hsa_queue_t *queue = get_cpu_queue(ret_queue_id);
    stream_obj->cpu_queue = queue;
    return queue;
}

hsa_queue_t *acquire_and_set_next_gpu_queue(atmi_stream_table_t *stream_obj) {
    int ret_queue_id = stream_obj->next_gpu_qid;
    /* use the same queue if the stream is ordered */
    /* otherwise, round robin the queue ID for unordered streams */
    if(stream_obj->ordered == ATMI_FALSE) {
        stream_obj->next_gpu_qid = (ret_queue_id + 1) % SNK_MAX_GPU_QUEUES;
    }
    hsa_queue_t *queue = GPU_CommandQ[ret_queue_id];
    stream_obj->gpu_queue = queue;
    return queue;
}

atmi_status_t clear_saved_tasks(atmi_stream_table_t *stream_obj) {
    hsa_status_t err;
    atl_task_list_t *cur = stream_obj->tasks;
    atl_task_list_t *prev = cur;
    while(cur != NULL ){
        cur = cur->next;
        free(prev);
        prev = cur;
    }

    stream_obj->tasks = NULL;

    return ATMI_STATUS_SUCCESS;
}

atmi_status_t check_change_in_device_type(atl_task_t *task, atmi_stream_table_t *stream_obj, hsa_queue_t *queue, atmi_devtype_t new_task_device_type) {
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

atmi_status_t register_task(atmi_stream_table_t *stream_obj, atl_task_t *task, atmi_devtype_t devtype, boolean profilable) {

    atl_task_list_t *node = (atl_task_list_t *)malloc(sizeof(atl_task_list_t));
    node->task = task;
    node->profilable = profilable;
    node->next = NULL;
    node->devtype = devtype;
    if(stream_obj->tasks == NULL) {
        stream_obj->tasks = node;
    } else {
        atl_task_list_t *cur = stream_obj->tasks;
        stream_obj->tasks = node;
        node->next = cur;
    }
    stream_obj->last_device_type = devtype;
    //DEBUG_PRINT("Registering %s task %p Profilable? %s\n", 
    //            (devtype == ATMI_DEVTYPE_GPU) ? "GPU" : "CPU",
    //            task, (profilable == ATMI_TRUE) ? "Yes" : "No");
    return ATMI_STATUS_SUCCESS;
}

atmi_status_t get_stream_signal(atmi_stream_table_t *stream_obj, hsa_signal_t *signal) {
    *signal = stream_obj->stream_common_signal;
    return ATMI_STATUS_SUCCESS;
}

atmi_status_t set_stream_signal(atmi_stream_table_t *stream_obj, hsa_signal_t signal) {
    stream_obj->stream_common_signal = signal;
    return ATMI_STATUS_SUCCESS;
}

atmi_status_t get_stream_mutex(atmi_stream_table_t *stream_obj, pthread_mutex_t *m) {
    *m = stream_obj->mutex;
    return ATMI_STATUS_SUCCESS;
}

atmi_status_t set_stream_mutex(atmi_stream_table_t *stream_obj, pthread_mutex_t *m) {
    stream_obj->mutex = *m;
    return ATMI_STATUS_SUCCESS;
}

atmi_status_t register_stream(atmi_stream_t *stream) {
    /* Check if the stream exists in the stream table. 
     * If no, then add this stream to the stream table.
     */
    DEBUG_PRINT("Stream %p registered\n", stream);
    if(StreamTable.find(stream) == StreamTable.end()) {
        atmi_stream_table_t *stream_entry = new atmi_stream_table_t;
        stream_entry->tasks = NULL;
        stream_entry->cpu_queue = NULL;
        stream_entry->gpu_queue = NULL;
        int stream_num = StreamTable.size();
        stream_entry->next_cpu_qid = stream_num % SNK_MAX_CPU_QUEUES;
        stream_entry->next_gpu_qid = stream_num % SNK_MAX_CPU_QUEUES;
        stream_entry->stream_common_signal.handle = (uint64_t)-1;
        pthread_mutex_init(&(stream_entry->mutex), NULL);
        StreamTable[stream] = stream_entry;
    }
    StreamTable[stream]->ordered = stream->ordered;

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

atmi_status_t atmi_init(int devtype) {
    if(devtype & ATMI_DEVTYPE_GPU) 
        atl_init_gpu_context();

    if(devtype & ATMI_DEVTYPE_CPU) 
        atl_init_cpu_context();

    return ATMI_STATUS_SUCCESS;
}

void init_hsa() {
    if(atlc.g_hsa_initialized == 0) {
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
        init_dag_scheduler();
        atlc.g_hsa_initialized = 1;
    }
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
    err = hsa_iterate_agents(get_cpu_agent, &atl_cpu_agent);
    if(err == HSA_STATUS_INFO_BREAK) { err = HSA_STATUS_SUCCESS; }
    ErrorCheck(Getting a gpu agent, err);

    atl_cpu_KernargRegion.handle=(uint64_t)-1;
    err = hsa_agent_iterate_regions(atl_cpu_agent, get_fine_grained_region, &atl_cpu_KernargRegion);
    if(err == HSA_STATUS_INFO_BREAK) { err = HSA_STATUS_SUCCESS; }
    err = (atl_cpu_KernargRegion.handle == (uint64_t)-1) ? HSA_STATUS_ERROR : HSA_STATUS_SUCCESS;
    ErrorCheck(Finding a CPU kernarg memory region handle, err);

    int num_queues = SNK_MAX_CPU_QUEUES;
    int queue_capacity = (1024 * 1024);
    cpu_agent_init(atl_cpu_agent, atl_cpu_KernargRegion, num_queues, queue_capacity);

    int stream_num;
    for(stream_num = 0; stream_num < ATMI_MAX_STREAMS; stream_num++) {
        /* round robin streams to queues */
        SNK_NextCPUQueueID[stream_num] = stream_num % SNK_MAX_CPU_QUEUES;
    }

    init_tasks();
    atlc.g_cpu_initialized = 1;
    return ATMI_STATUS_SUCCESS;
}

void *atl_read_binary_from_file(const char *module, size_t *module_size) {
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
    *module_size = size;
    return raw_code_object;
}

atmi_status_t atl_gpu_create_program() {
    hsa_status_t err;
    /* Create hsa program.  */
    memset(&atl_hsa_program,0,sizeof(hsa_ext_program_t));
    err = hsa_ext_program_create(HSA_MACHINE_MODEL_LARGE, HSA_PROFILE_FULL, HSA_DEFAULT_FLOAT_ROUNDING_MODE_DEFAULT, NULL, &atl_hsa_program);
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
    hsa_status_t err = hsa_executable_create(HSA_PROFILE_FULL, HSA_EXECUTABLE_STATE_UNFROZEN, "", executable);
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
    err = hsa_executable_create(HSA_PROFILE_FULL, HSA_EXECUTABLE_STATE_UNFROZEN, "", executable);
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

        KernelInfoTable[std::string(name)] = info;
        free(name);

        /*
        void *thisKernargAddress = NULL;
        // create a memory segment for this kernel's arguments
        err = hsa_memory_allocate(atl_gpu_KernargRegion, info.kernel_segment_size * MAX_NUM_KERNELS, &thisKernargAddress);
        ErrorCheck(Allocating memory for the executable-kernel, err);
        */
    }
    return HSA_STATUS_SUCCESS;
}

atmi_status_t atmi_module_register(const char **filenames, atmi_platform_type_t *types, const int num_modules) {
    hsa_executable_t executable; 
    hsa_status_t err;

    std::vector<std::string> filenames_str;
    /* Create the empty executable.  */
    err = hsa_executable_create(HSA_PROFILE_FULL, HSA_EXECUTABLE_STATE_UNFROZEN, "", &executable);
    ErrorCheck(Create the executable, err);
    
    for(int i = 0; i < num_modules; i++) {
        filenames_str.push_back(std::string(filenames[i]));
        size_t module_size;
        void *module_bytes = atl_read_binary_from_file(filenames[i], &module_size); 
        if(types[i] == BRIG) {
            hsa_ext_module_t module = (hsa_ext_module_t)module_bytes;

            /* Create hsa program.  */
            memset(&atl_hsa_program,0,sizeof(hsa_ext_program_t));
            err = hsa_ext_program_create(HSA_MACHINE_MODEL_LARGE, HSA_PROFILE_FULL, HSA_DEFAULT_FLOAT_ROUNDING_MODE_DEFAULT, NULL, &atl_hsa_program);
            ErrorCheck(Create the program, err);

            /* Add the BRIG module to hsa program.  */
            err = hsa_ext_program_add_module(atl_hsa_program, module);
            ErrorCheck(Adding the brig module to the program, err);
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

            /* Load the code object.  */
            err = hsa_executable_load_code_object(executable, atl_gpu_agent, code_object, "");
            ErrorCheck(Loading the code object, err);
        }
        else if (types[i] == BRIG_FINAL) {
            // Deserialize code object.
            hsa_code_object_t code_object = {0};
            hsa_status_t err = hsa_code_object_deserialize(module_bytes, module_size, NULL, &code_object);
            ErrorCheck(Code Object Deserialization, err);
            assert(0 != code_object.handle);

            /* Load the code object.  */
            err = hsa_executable_load_code_object(executable, atl_gpu_agent, code_object, "");
            ErrorCheck(Loading the code object, err);
        }
        free(module_bytes);
    }

    /* Freeze the executable; it can now be queried for symbols.  */
    err = hsa_executable_freeze(executable, "");
    ErrorCheck(Freeze the executable, err);
    
    err = hsa_executable_iterate_symbols(executable, create_kernarg_memory, NULL); 
    ErrorCheck(Iterating over symbols for execuatable, err);
    
    ModuleMap[executable.handle] = filenames_str;
    return ATMI_STATUS_SUCCESS;
}

atmi_status_t atl_init_gpu_context() {

    if(atlc.struct_initialized == 0) atmi_init_context_structs();
    if(atlc.g_gpu_initialized != 0) return ATMI_STATUS_SUCCESS;
    
    hsa_status_t err;

    init_hsa();
    /* Iterate over the agents and pick the gpu agent */
    err = hsa_iterate_agents(get_gpu_agent, &atl_gpu_agent);
    if(err == HSA_STATUS_INFO_BREAK) { err = HSA_STATUS_SUCCESS; }
    ErrorCheck(Getting a gpu agent, err);
    
    /* Query the name of the agent.  */
    char name[64] = { 0 };
    err = hsa_agent_get_info(atl_gpu_agent, HSA_AGENT_INFO_NAME, name);
    ErrorCheck(Querying the agent name, err);
    /* printf("The agent name is %s.\n", name); */

    /* Find a memory region that supports kernel arguments.  */
    atl_gpu_KernargRegion.handle=(uint64_t)-1;
    hsa_agent_iterate_regions(atl_gpu_agent, get_kernarg_memory_region, &atl_gpu_KernargRegion);
    err = (atl_gpu_KernargRegion.handle == (uint64_t)-1) ? HSA_STATUS_ERROR : HSA_STATUS_SUCCESS;
    ErrorCheck(Finding a kernarg memory region, err);
    
    /* Query the maximum size of the queue.  */
    uint32_t queue_size = 0;
    err = hsa_agent_get_info(atl_gpu_agent, HSA_AGENT_INFO_QUEUE_MAX_SIZE, &queue_size);
    ErrorCheck(Querying the agent maximum queue size, err);
    /* printf("The maximum queue size is %u.\n", (unsigned int) queue_size); */

    /* Create queues and signals for each stream. */
    int stream_num;
    for ( stream_num = 0 ; stream_num < SNK_MAX_GPU_QUEUES ; stream_num++){
       err=hsa_queue_create(atl_gpu_agent, queue_size, HSA_QUEUE_TYPE_SINGLE, NULL, NULL, UINT32_MAX, UINT32_MAX, &GPU_CommandQ[stream_num]);
       ErrorCheck(Creating the Stream Command Q, err);
       err = hsa_amd_profiling_set_profiler_enabled( GPU_CommandQ[stream_num], 1); 
       ErrorCheck(Enabling profiling support, err); 
    }

    for(stream_num = 0; stream_num < ATMI_MAX_STREAMS; stream_num++) {
        /* round robin streams to queues */
        SNK_NextGPUQueueID[stream_num] = stream_num % SNK_MAX_GPU_QUEUES;
    }

    if(context_init_time_init == 0) {
        clock_gettime(CLOCK_MONOTONIC_RAW,&context_init_time);
        context_init_time_init = 1;
    }

    init_tasks();
    atlc.g_gpu_initialized = 1;
    return ATMI_STATUS_SUCCESS;
}

void print_atl_kernel(const char * str, const int i) {

    printf("[%s] PIF Table[%d]: %s -- %d -- %d -- %p(%s) -- %p -- %p(%s)\n",
            str, i,
            atl_kernels[i].pif_name, 
            atl_kernels[i].devtype, 
            atl_kernels[i].num_params,
            atl_kernels[i].cpu_kernel.kernel_name, 
            atl_kernels[i].cpu_kernel.kernel_name, 
            atl_kernels[i].cpu_kernel.function,
            atl_kernels[i].gpu_kernel.kernel_name,
            atl_kernels[i].gpu_kernel.kernel_name
          );

}

atmi_status_t atl_init_kernel(
                             const char *pif_name, 
                             const atmi_devtype_t devtype,
                             const int num_params, 
                             const char *cpu_kernel_name, 
                             atmi_generic_fp fn_ptr,
                             const char *gpu_kernel_name) {
    static int atl_kernel_counter = 0;
    if(atl_kernel_counter < 0 || atl_kernel_counter >= SNK_MAX_FUNCTIONS) {
        DEBUG_PRINT("Too many kernel functions. Increase SNK_MAX_FUNCTIONS value.\n");
        return ATMI_STATUS_ERROR;
    }
    DEBUG_PRINT("PIF[%d] Entry Name: %s, Num args: %d CPU Kernel: %s, GPU Kernel: %p\n", atl_kernel_counter, pif_name, num_params, cpu_kernel_name, gpu_kernel_name);
    atl_kernels[atl_kernel_counter].pif_name = pif_name;
    atl_kernels[atl_kernel_counter].devtype = devtype;
    atl_kernels[atl_kernel_counter].num_params = num_params;
    //atl_kernels[atl_kernel_counter].cpu_kernel.kernel_name = cpu_kernel_name;
    if(cpu_kernel_name) strcpy(atl_kernels[atl_kernel_counter].cpu_kernel.kernel_name, cpu_kernel_name);
    atl_kernels[atl_kernel_counter].cpu_kernel.function = fn_ptr; 
    if(gpu_kernel_name) strcpy(atl_kernels[atl_kernel_counter].gpu_kernel.kernel_name, gpu_kernel_name);
    //atl_kernels[atl_kernel_counter].gpu_kernel.kernel_name[strlen(gpu_kernel_name)] = 0;
    //atl_kernels[atl_kernel_counter].gpu_kernel.kernel_name = gpu_kernel_name;
    atl_kernel_counter++;
    return ATMI_STATUS_SUCCESS;
}

atmi_status_t atl_pif_init(atl_pif_kernel_table_t pif_fn_table[], const int sz) {
    int i;
    // FIXME: Move away from a flat table structure to a hierarchical table where
    // each PIF has a table of potential kernels!
    DEBUG_PRINT("Number of kernels for pif: %lu / %lu\n", sizeof(pif_fn_table), sizeof(pif_fn_table[0]));
    for (i = 0; i < sz; i++) {
       atl_init_kernel(
                           pif_fn_table[i].pif_name, 
                           pif_fn_table[i].devtype, 
                           pif_fn_table[i].num_params,
                           pif_fn_table[i].cpu_kernel.kernel_name, 
                           pif_fn_table[i].cpu_kernel.function,
                           pif_fn_table[i].gpu_kernel.kernel_name
                           );
    }
    return ATMI_STATUS_SUCCESS;
}

atmi_status_t atmi_kernel_create_empty(atmi_kernel_t *kernel, const char *pif_str) {
    static int counter = 0;
    char *pif = (char *)malloc(256);
    memset(pif, 0, 256);
    if(pif_str == NULL) {
        sprintf(pif, "num%d", counter);
    }
    else {
        memcpy(pif, pif_str, strlen(pif_str));
    }
    kernel->handle = (uint64_t)pif;
    counter++;
    return ATMI_STATUS_SUCCESS;
}

atmi_status_t atmi_kernel_release(atmi_kernel_t kernel) {
    char *pif = (char *)(kernel.handle);
    free(pif);
    return ATMI_STATUS_SUCCESS;
}

atmi_status_t atmi_kernel_add_gpu_impl(atmi_kernel_t kernel, const char *impl) {
    const unsigned int num_params = 0;
    const char *pif_name = (const char *)(kernel.handle);
    std::string cl_pif_name("&__OpenCL_");
    cl_pif_name += std::string(impl );
    cl_pif_name += std::string("_kernel");
    atl_init_kernel(
            pif_name, 
            ATMI_DEVTYPE_GPU, 
            num_params,
            NULL, (atmi_generic_fp)NULL, cl_pif_name.c_str());
}

atmi_status_t atmi_kernel_add_cpu_impl(atmi_kernel_t kernel, atmi_generic_fp impl) {
    const unsigned int num_params = 0;
    const char *pif_name = (const char *)(kernel.handle);
    atl_init_kernel(
            pif_name, 
            ATMI_DEVTYPE_CPU, 
            num_params,
            NULL, impl, NULL);
}

atmi_status_t atl_get_gpu_kernel_info(
                            hsa_executable_t executable,
                            const char *kernel_symbol_name,
                            uint64_t                         *_KN__Kernel_Object,
                            uint32_t                         *_KN__Group_Segment_Size,
                            uint32_t                         *_KN__Private_Segment_Size,
                            uint32_t                         *_KN__Kernarg_Size
                            ) {

    hsa_status_t err;
    atl_kernel_info_t info;
    if(KernelInfoTable.find(std::string(kernel_symbol_name)) == KernelInfoTable.end()) {

        hsa_executable_symbol_t symbol;
        /* Extract the symbol from the executable.  */
        //DEBUG_PRINT("Kernel name _KN__: Looking for symbol %s\n", kernel_symbol_name); 
        err = hsa_executable_get_symbol(executable, NULL, kernel_symbol_name, atl_gpu_agent, 0, &symbol);
        ErrorCheck(Extract the symbol from the executable, err);

        /* Extract dispatch information from the symbol */
        err = hsa_executable_symbol_get_info(symbol, HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_OBJECT, &(info.kernel_object));
        ErrorCheck(Extracting the symbol from the executable, err);
        err = hsa_executable_symbol_get_info(symbol, HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_GROUP_SEGMENT_SIZE, &(info.group_segment_size));
        ErrorCheck(Extracting the group segment size from the executable, err);
        err = hsa_executable_symbol_get_info(symbol, HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_PRIVATE_SEGMENT_SIZE, &(info.private_segment_size));
        ErrorCheck(Extracting the private segment from the executable, err);

        KernelInfoTable[std::string(kernel_symbol_name)] = info;
    }
    else {
        info = KernelInfoTable[std::string(kernel_symbol_name)];
    }
    *_KN__Kernel_Object = info.kernel_object;
    *_KN__Group_Segment_Size = info.group_segment_size;
    *_KN__Private_Segment_Size = info.private_segment_size;
    *_KN__Kernarg_Size = info.kernel_segment_size;

    return ATMI_STATUS_SUCCESS;

}                    

atmi_status_t atl_gpu_memory_allocate(const atmi_lparm_t *lparm,
                 hsa_executable_t executable,
                 const char *pif_name,
                 void **thisKernargAddress) {
    hsa_status_t err;
    uint16_t i;
    int this_kernel_iter = 0;
    for(i = 0; i < SNK_MAX_FUNCTIONS; i++) {
        //DEBUG_PRINT("Comparing kernels %s %s\n", atl_kernels[i].pif_name, pif_name);
        if(atl_kernels[i].pif_name && pif_name) {
            if(strcmp(atl_kernels[i].pif_name, pif_name) == 0) {// && atl_kernels[i].gpu_kernel.kernel_name != NULL) {
                if(this_kernel_iter != lparm->kernel_id) {
                    this_kernel_iter++;
                    continue;
                }
                if(atl_kernels[i].devtype != ATMI_DEVTYPE_GPU) {
                    fprintf(stderr, "ERROR:  Bad GPU kernel_id %d for PIF %s\n", this_kernel_iter, pif_name);
                    return ATMI_STATUS_ERROR;
                }
                const char *kernel_name = atl_kernels[i].gpu_kernel.kernel_name;
                hsa_executable_symbol_t symbol;
                /* Extract the symbol from the executable.  */
                err = hsa_executable_get_symbol(executable, NULL, kernel_name, atl_gpu_agent, 0, &symbol);
                ErrorCheck(Extract the symbol from the executable, err);

                /* Extract dispatch information from the symbol */
                uint32_t kernel_segment_size;
                err = hsa_executable_symbol_get_info(symbol, HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_KERNARG_SEGMENT_SIZE, &kernel_segment_size);
                ErrorCheck(Extracting the kernarg segment size from the executable, err);
                DEBUG_PRINT("Kernel GPU memalloc. Kernel %s needs %" PRIu32" bytes for kernargs\n", kernel_name, kernel_segment_size); 

#if 1
                *thisKernargAddress = malloc(kernel_segment_size);
                //posix_memalign(thisKernargAddress, 16, kernel_segment_size);
#else
                /* FIXME: HSA 1.0F may have a bug that serializes all queue
                 * operations when hsa_memory_allocate is used.
                 * Investigate more and revert back to
                 * hsa_memory_allocate once bug is fixed. */
                err = hsa_memory_allocate(atl_gpu_KernargRegion, kernel_segment_size, thisKernargAddress);
                ErrorCheck(Allocating memory for the executable-kernel, err);
#endif
                break;
            }
        }
    }
}

void dispatch_all_tasks(std::vector<atl_task_t*>::iterator &start, 
        std::vector<atl_task_t*>::iterator &end) {
    for(std::vector<atl_task_t*>::iterator it = start; it != end; it++) {
        dispatch_task(*it);
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

void handle_signal_callback(atl_task_t *task) {
    atmi_lparm_t *l = NULL;
    // tasks without atmi_task handle should not be added to callbacks anyway
    assert(task->atmi_task != NULL);
    lock(&(task->mutex));
    l = &(task->lparm);
    set_task_state(task, ATMI_COMPLETED);
    set_task_metrics(task, task->devtype, task->profilable);
    unlock(&(task->mutex));

    // after predecessor is done, decrement all successor's dependency count. 
    // If count reaches zero, then add them to a 'ready' task list. Next, 
    // dispatch all ready tasks in a round-robin manner to the available 
    // GPU/CPU queues. 
    // decrement reference count of its dependencies; add those with ref count = 0 to a
    // “ready” list
    atl_task_vector_t deps = task->and_successors;
    //printf("Deps list of %d [%d]: ", task->id, deps.size());
    atl_task_vector_t temp_list;
    for(atl_task_vector_t::iterator it = deps.begin();
            it != deps.end(); it++) {
        // FIXME: should we be grabbing a lock on each successor before
        // decrementing their predecessor count? Currently, it may not be
        // required because there is only one callback thread, but what if there
        // were more? 
        lock(&((*it)->mutex));
        (*it)->num_predecessors--;
        if((*it)->num_predecessors == 0) {
            // add to ready list
            temp_list.push_back(*it);
        }
        unlock(&((*it)->mutex));
    }

    for(atl_task_vector_t::iterator it = temp_list.begin();
            it != temp_list.end(); it++) {
        lock(&mutex_readyq_);
        ReadyTaskQueue.push(*it);
        unlock(&mutex_readyq_);
    }
    DEBUG_PRINT("[Handle Signal %d ] Free Signal Pool Size: %lu; Ready Task Queue Size: %lu\n", task->id, FreeSignalPool.size(), ReadyTaskQueue.size());
    // dispatch from ready queue if any task exists
    dispatch_ready_task_or_release_signal(task);
}

void handle_signal_barrier_pkt(atl_task_t *task) {
    atmi_lparm_t *l = &(task->lparm);
    std::vector<pthread_mutex_t *> mutexes;
    mutexes.push_back(&(task->mutex));
    mutexes.push_back(&mutex_readyq_);
    atl_task_vector_t &requires = task->and_predecessors;
    for(int idx = 0; idx < requires.size(); idx++) {
        mutexes.push_back(&(requires[idx]->mutex));
    }
    
    lock_vec(mutexes);

    DEBUG_PRINT("{%d}\n", task->id);
    l = &(task->lparm);
    set_task_state(task, ATMI_COMPLETED);
    set_task_metrics(task, task->devtype, task->profilable);

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
    if(task->atmi_task != NULL) {
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
    task->barrier_signals.clear();
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
        set_thread_affinity(3);
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
    //HandleSignalTimer.Start();
    atl_task_t *task = (atl_task_t *)arg;
    //static int counter = 0;
    if(g_dep_sync_type == ATL_SYNC_CALLBACK) {
        handle_signal_callback(task); 
    }
    else if(g_dep_sync_type == ATL_SYNC_BARRIER_PKT) {
        handle_signal_barrier_pkt(task); 
    }
    //HandleSignalTimer.Stop();
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
                should_dispatch = try_dispatch_barrier_pkt(ready_task);
                /* if cannot be dispatched the task will be automatically be added to
                 * the ready queue */
                should_register_callback = ((ready_task->atmi_task != NULL) ||  
                        (ready_task->atmi_task == NULL && !(ready_task->and_predecessors.empty())));
            }
            if(should_dispatch) {
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
    atl_task_vector_t ready_tasks;
    ready_tasks.clear();
    lock(&mutex_readyq_);
    // take *any* task with ref count = 0, which means it is ready to be dispatched
    while(!ReadyTaskQueue.empty()) {
        atl_task_t *ready_task = ReadyTaskQueue.front(); 
        ready_tasks.push_back(ready_task); 
        ReadyTaskQueue.pop();
        if(ready_task->atmi_task != NULL) 
            break;
    }

    if(ready_tasks.empty()) {
        if(task->atmi_task != NULL)
            FreeSignalPool.push(signal);
        // do not release Stream signals into the pool
        // but stream signals should not be in the handlers
        // anyway
    }
    unlock(&mutex_readyq_);
    DEBUG_PRINT("[Handle Signal2] Free Signal Pool Size: %lu; Ready Task Queue Size: %lu\n", FreeSignalPool.size(), ReadyTaskQueue.size());
    // set signal to wait for 1 ready task
    //hsa_signal_store_relaxed(signal, 1);
    for(atl_task_vector_t::iterator it = ready_tasks.begin(); 
                                  it!= ready_tasks.end(); it++) {
        atl_task_t *ready_task = *it;
        if(ready_task->atmi_task != NULL) {        
            ready_task->signal = signal;
            DEBUG_PRINT("Callback dispatching next task %p (%d)\n", ready_task, ready_task->id);
            //DEBUG_PRINT("tid = %lu\n", syscall(SYS_gettid));
        }
        else {
            pthread_mutex_t stream_mutex;
            atmi_stream_table_t *stream_obj = ready_task->stream_obj;
            get_stream_mutex(stream_obj, &stream_mutex);
            lock(&stream_mutex);
            hsa_signal_t stream_signal;
            get_stream_signal(stream_obj, &stream_signal);
            if(stream_signal.handle != (uint64_t)-1) {
                ready_task->signal = stream_signal;
            }
            else {
                assert(StreamCommonSignalIdx < ATMI_MAX_STREAMS);
                // get the next free signal from the stream common signal pool
                stream_signal = StreamCommonSignalPool[StreamCommonSignalIdx++];
                ready_task->signal = stream_signal;
                set_stream_signal(stream_obj, stream_signal);
            }
            unlock(&stream_mutex);
        }
        callback_dispatch++;
        dispatch_task(ready_task);
        if(ready_task->atmi_task != NULL) {    
            hsa_status_t err = hsa_amd_signal_async_handler(signal,
                    HSA_SIGNAL_CONDITION_EQ, 0,
                    handle_signal, (void *)ready_task);
            ErrorCheck(Creating signal handler, err);
        }
    }
}

atmi_status_t dispatch_task(atl_task_t *task) {
    atmi_lparm_t *lparm = &(task->lparm);
    //DEBUG_PRINT("GPU Place Info: %d, %lx %lx\n", lparm->place.node_id, lparm->place.cpu_set, lparm->place.gpu_set);

    //TryDispatchTimer.Start();
    atmi_stream_table_t *stream_obj = task->stream_obj;
    /* get this stream's HSA queue (could be dynamically mapped or round robin
     * if it is an unordered stream */
    // FIXME: round robin for now, but may use some other load balancing algo
    // enqueue task's packet to that queue

    hsa_queue_t* this_Q = NULL;
    if(task->devtype == ATMI_DEVTYPE_GPU)
        this_Q = acquire_and_set_next_gpu_queue(stream_obj);
    else if(task->devtype == ATMI_DEVTYPE_CPU)
        this_Q = acquire_and_set_next_cpu_queue(stream_obj);
    if(!this_Q) return ATMI_STATUS_ERROR;

    /* if stream is ordered and the devtype changed for this task, 
     * enqueue a barrier to wait for previous device to complete */
    //check_change_in_device_type(task, stream_obj, this_Q, task->devtype);

    if(g_dep_sync_type == ATL_SYNC_BARRIER_PKT) {
        /* For dependent child tasks, add dependent parent kernels to barriers.  */
        DEBUG_PRINT("Pif requires %d tasks\n", lparm->num_required);
        if ( task->and_predecessors.size() > 0) {
            int val = 0;
            DEBUG_PRINT("(");
            for(size_t count = 0; count < task->and_predecessors.size(); count++) {
                if(task->and_predecessors[count]->state < ATMI_DISPATCHED) val++;
                assert(task->and_predecessors[count]->state >= ATMI_DISPATCHED);
                DEBUG_PRINT("%d ", task->and_predecessors[count]->id);
            }
            DEBUG_PRINT(")\n");
            if(val > 0) printf("Task[%d] has %d not-dispatched predecessor tasks\n", task->id, val);
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
    /*  Obtain the current queue write index. increases with each call to kernel  */
    uint64_t index = hsa_queue_load_write_index_relaxed(this_Q);

    /* Find the queue index address to write the packet info into.  */
    const uint32_t queueMask = this_Q->size - 1;
    if(task->devtype == ATMI_DEVTYPE_GPU) {
        hsa_kernel_dispatch_packet_t* this_aql = &(((hsa_kernel_dispatch_packet_t*)(this_Q->base_address))[index&queueMask]);
        memset(this_aql, 0, sizeof(hsa_kernel_dispatch_packet_t));
        /*  FIXME: We need to check for queue overflow here. */
        //SignalAddTimer.Start();
        hsa_signal_add_relaxed(task->signal, 1);
        //hsa_signal_store_relaxed(task->signal, 1);
        //SignalAddTimer.Stop();
        this_aql->completion_signal = task->signal;
        int ndim = -1; 
        if(lparm->gridDim[2] > 1) 
            ndim = 3;
        else if(lparm->gridDim[1] > 1) 
            ndim = 2;
        else
            ndim = 1;

        /* pass this task handle to the kernel as an argument */
        struct kernel_args_struct {
            uint64_t arg0;
            uint64_t arg1;
            uint64_t arg2;
            uint64_t arg3;
            uint64_t arg4;
            uint64_t arg5;
            atmi_task_t* arg6;
            /* other fields no needed to set task handle? */
        } __attribute__((aligned(16)));
        struct kernel_args_struct *kargs = (struct kernel_args_struct *)task->gpu_kernargptr;
        kargs->arg6 = task->atmi_task;

        /*  Process lparm values */
        /*  this_aql.dimensions=(uint16_t) ndim; */
        this_aql->setup  |= (uint16_t) ndim << HSA_KERNEL_DISPATCH_PACKET_SETUP_DIMENSIONS;
        this_aql->grid_size_x=lparm->gridDim[0];
        this_aql->workgroup_size_x=lparm->groupDim[0];
        if (ndim>1) {
            this_aql->grid_size_y=lparm->gridDim[1];
            this_aql->workgroup_size_y=lparm->groupDim[1];
        } else {
            this_aql->grid_size_y=1;
            this_aql->workgroup_size_y=1;
        }
        if (ndim>2) {
            this_aql->grid_size_z=lparm->gridDim[2];
            this_aql->workgroup_size_z=lparm->groupDim[2];
        } else {
            this_aql->grid_size_z=1;
            this_aql->workgroup_size_z=1;
        }

        /*  Bind kernel argument buffer to the aql packet.  */
        this_aql->kernarg_address = task->gpu_kernargptr;
        this_aql->kernel_object = task->kernel_object;
        this_aql->private_segment_size = task->private_segment_size;
        this_aql->group_segment_size = task->group_segment_size;

        set_task_state(task, ATMI_DISPATCHED);
        /*  Prepare and set the packet header */ 
        this_aql->header = create_header(HSA_PACKET_TYPE_KERNEL_DISPATCH, ATMI_FALSE);
        /* Increment write index and ring doorbell to dispatch the kernel.  */
        hsa_queue_store_write_index_relaxed(this_Q, index+1);
        hsa_signal_store_relaxed(this_Q->doorbell_signal, index);
    } 
    else if(task->devtype == ATMI_DEVTYPE_CPU) {
        struct timespec dispatch_time;
        clock_gettime(CLOCK_MONOTONIC_RAW,&dispatch_time);
        hsa_agent_dispatch_packet_t* this_aql = &(((hsa_agent_dispatch_packet_t*)(this_Q->base_address))[index&queueMask]);
        memset(this_aql, 0, sizeof(hsa_agent_dispatch_packet_t));
        /*  FIXME: We need to check for queue overflow here. Do we need
         *  to do this for CPU agents too? */
        //SignalAddTimer.Start();
        hsa_signal_add_acq_rel(task->signal, 1);
        //hsa_signal_store_relaxed(task->signal, 1);
        //SignalAddTimer.Stop();
        this_aql->completion_signal = task->signal;

        /* Set the type and return args.*/
        // FIXME FIXME FIXME: Use the hierarchical pif-kernel table to
        // choose the best kernel. Don't use a flat table structure
        this_aql->type = (uint16_t)task->cpu_kernelid;
        /* FIXME: We are considering only void return types for now.*/
        //this_aql->return_address = NULL;
        /* Set function args */
        this_aql->arg[0] = task->num_params;
        this_aql->arg[1] = (uint64_t) task->cpu_kernelargs;
        this_aql->arg[2] = (uint64_t) task->atmi_task; // pass task handle to fill in metrics
        this_aql->arg[3] = UINT64_MAX;

        /*  Prepare and set the packet header */
        /* FIXME: CPU tasks ignore barrier bit as of now. Change
         * implementation? I think it doesn't matter because we are
         * executing the subroutines one-by-one, so barrier bit is
         * inconsequential.
         */
        set_task_state(task, ATMI_DISPATCHED);
        this_aql->header = create_header(HSA_PACKET_TYPE_AGENT_DISPATCH, ATMI_FALSE);

        /* Store dispatched time */
        if(task->profilable == ATMI_TRUE && task->atmi_task) 
            task->atmi_task->profile.dispatch_time = get_nanosecs(context_init_time, dispatch_time);
        /* Increment write index and ring doorbell to dispatch the kernel.  */
        hsa_queue_store_write_index_relaxed(this_Q, index+1);
        hsa_signal_store_relaxed(this_Q->doorbell_signal, index);
        signal_worker(this_Q, PROCESS_PKT);
    }
    //TryDispatchTimer.Stop();
    DEBUG_PRINT("Task %d (%d) Dispatched\n", task->id, task->devtype);
    return ATMI_STATUS_SUCCESS;
}

bool try_dispatch_barrier_pkt(atl_task_t *ret) {
    bool should_dispatch = true;
    atmi_lparm_t *lparm = &(ret->lparm);
    hsa_signal_t new_signal;
    // add to its predecessor's dependents list and return 
    std::vector<pthread_mutex_t *> req_mutexes;
    std::vector<atl_task_t *> &temp_vecs = ret->predecessors;
    req_mutexes.clear();
    for(int idx = 0; idx < lparm->num_required; idx++) {
        atl_task_t *pred_task = temp_vecs[idx];
        req_mutexes.push_back((pthread_mutex_t *)&(pred_task->mutex));
    }
    req_mutexes.push_back((pthread_mutex_t *)&(ret->mutex));
    req_mutexes.push_back(&mutex_readyq_);
    pthread_mutex_t stream_mutex;
    atmi_stream_table_t *stream_obj = ret->stream_obj;
    get_stream_mutex(stream_obj, &stream_mutex);
    req_mutexes.push_back(&stream_mutex);
    lock_vec(req_mutexes);
    //std::cout << "[" << ret->id << "]Signals Before: " << FreeSignalPool.size() << std::endl;
    int dep_count = 0;
    for(int idx = 0; idx < lparm->num_required; idx++) {
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
    for(int idx = 0; idx < lparm->num_required; idx++) {
        atl_task_t *pred_task = temp_vecs[idx]; 
        if(pred_task->state /*.load(std::memory_order_seq_cst)*/ != ATMI_COMPLETED) {
            required_tasks++;
        }
    }
    const int HSA_BARRIER_MAX_DEPENDENT_TASKS = 4;
    /* round up */
    int barrier_pkt_count = (required_tasks + HSA_BARRIER_MAX_DEPENDENT_TASKS - 1) / HSA_BARRIER_MAX_DEPENDENT_TASKS;
    if(should_dispatch) {
        if(ret->atmi_task != NULL) {
            // this is a task that uses individual signals (not stream-signals)
            // and we did not find a free signal, so just enqueue  
            // for a later dispatch
            if(FreeSignalPool.size() < barrier_pkt_count + 1) {
                should_dispatch = false;
                ret->and_predecessors.clear();
            }
            else {
                new_signal = FreeSignalPool.front();
                FreeSignalPool.pop();
                ret->signal = new_signal;
                for(int barrier_id = 0; barrier_id < barrier_pkt_count; barrier_id++) { 
                    new_signal = FreeSignalPool.front();
                    FreeSignalPool.pop();
                    ret->barrier_signals.push_back(new_signal);
                }
            }
        }
        else {
            if(FreeSignalPool.size() < barrier_pkt_count) {
                should_dispatch = false;
                ret->and_predecessors.clear();
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
        }
    }
    if(should_dispatch) {
        for(int idx = 0; idx < lparm->num_required; idx++) {
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

bool try_dispatch_callback(atl_task_t *ret) {
    bool should_try_dispatch = true;
    bool should_dispatch = false;
    atmi_lparm_t *lparm = &(ret->lparm);
    
    if(lparm->num_required > 0) {
        // add to its predecessor's dependents list and return 
        std::vector<pthread_mutex_t *> req_mutexes;
        req_mutexes.clear();
        for(int idx = 0; idx < lparm->num_required; idx++) {
            atl_task_t *pred_task = ret->predecessors[idx];
            req_mutexes.push_back((pthread_mutex_t *)&(pred_task->mutex));
        }
        req_mutexes.push_back((pthread_mutex_t *)&(ret->mutex));
        std::sort(req_mutexes.begin(), req_mutexes.end());
        for(size_t i = 0; i < req_mutexes.size(); i++) {
            lock((pthread_mutex_t *)req_mutexes[i]);
        }        
        for(int idx = 0; idx < lparm->num_required; idx++) {
            atl_task_t *pred_task = ret->predecessors[idx];
            DEBUG_PRINT("Task %d depends on %d as predecessor ",
                    ret->id, pred_task->id);
            if(pred_task->state /*.load(std::memory_order_seq_cst)*/ != ATMI_COMPLETED) {
                should_try_dispatch = false;
                pred_task->and_successors.push_back(ret);
                ret->num_predecessors++;
                DEBUG_PRINT("(waiting)\n");
                waiting_count++;
            }
            else {
                DEBUG_PRINT("(completed)\n");
            }
        }
        for(size_t i = req_mutexes.size(); i-- > 0; ) {
            unlock((pthread_mutex_t *)req_mutexes[i]);
        }        
    }

    if(should_try_dispatch) {
        // try to dispatch if 
        // a) you are using callbacks to resolve dependencies and all
        // your predecessors are done executing, OR
        // b) you are using barrier packets, in which case always try
        // to launch if you have a free signal at hand

        if(lparm->task == NULL) {
            pthread_mutex_t stream_mutex;
            atmi_stream_table_t *stream_obj = ret->stream_obj;
            get_stream_mutex(stream_obj, &stream_mutex);
            lock(&stream_mutex);
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
            should_dispatch = true;
            unlock(&stream_mutex);
        }
        else {
            // get a free signal
            lock(&mutex_readyq_);
            if(!FreeSignalPool.empty()) {
                hsa_signal_t new_signal = FreeSignalPool.front();
                ret->signal = new_signal;
                DEBUG_PRINT("Before pop Signal handle: %" PRIu64 " Signal value:%ld (Signal pool sz: %lu)\n", ret->signal.handle, hsa_signal_load_relaxed(ret->signal),
                        FreeSignalPool.size());
                FreeSignalPool.pop(); 
                should_dispatch = true;
            }
            else {
                // add to ready queue
                DEBUG_PRINT("Before add task %p (%d) to ready queue (Sz: %lu)\n", ret, ret->id, ReadyTaskQueue.size());
                ReadyTaskQueue.push(ret);
                max_ready_queue_sz++;
            }
            DEBUG_PRINT("[Try Dispatch] Free Signal Pool Size: %lu; Ready Task Queue Size: %lu\n", FreeSignalPool.size(), ReadyTaskQueue.size());
            unlock(&mutex_readyq_);
        }
    }
    return should_dispatch;
}

atmi_task_t *atl_trylaunch_kernel(const int kernel_id,
                 const atmi_lparm_t *lparm,
                 const char *kernel_name,
                 void *thisKernargAddress) {
    DEBUG_PRINT("GPU Place Info: %d, %lx %lx\n", lparm->place.node_id, lparm->place.cpu_set, lparm->place.gpu_set);
#if 1
    static bool is_called = false;
    if(!is_called) {
        set_thread_affinity(2);

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
    atmi_stream_t *stream = NULL;
    if(lparm->stream == NULL) {
        stream = &atl_default_stream_obj;
    } else {
        stream = lparm->stream;
    }
    /* Add row to stream table for purposes of future synchronizations */
    register_stream(stream);
    atmi_stream_table_t *stream_obj = StreamTable[stream];

    if(lparm->synchronous == ATMI_TRUE && lparm->num_required == 0 && lparm->num_needs_any == 0
            //&& stream->ordered == ATMI_TRUE
      ) { 
        // Greg's logic is to flush the entire stream if the task is sync and has no dependencies */
        // FIXME: This has to change for unordered streams. Why should we flush the entire stream 
        // for every sync kernel in an unordered stream?
        atl_stream_sync(stream_obj);
    }

    //TryLaunchTimer.Start();
    uint16_t i;
    atl_task_t *ret = NULL;
    atl_task_t *continuation_task = NULL;
    bool has_continuation = false;
    if(lparm->task) {
        has_continuation = (lparm->task->continuation != NULL);
        if(PublicTaskMap.find(lparm->task) != PublicTaskMap.end()) {
            // task is already found, so use it and dont create a new one
            ret = PublicTaskMap[lparm->task];
        }
    }
    if(ret == NULL) {
        ret = new atl_task_t;
        memset(ret, 0, sizeof(atl_task_t));
        lock(&mutex_all_tasks_);
        AllTasks.push_back(ret);
        //AllTasks.push_back(atl_task_t());
        //AllTasks.emplace_back();
        //ret = AllTasks[AllTasks.size() - 1];
        ret->id = AllTasks.size();
        //AllTasks.push_back(task); 
        //atl_task_t *ret = task; //AllTasks[AllTasks.size() - 1];
        unlock(&mutex_all_tasks_);
        PublicTaskMap[lparm->task] = ret;
        DEBUG_PRINT("Task Map[%p] = %p (%s)\n", lparm->task, PublicTaskMap[lparm->task], kernel_name);
        pthread_mutex_init(&(ret->mutex), NULL);
    }
    if(has_continuation) {
        continuation_task = new atl_task_t;
        memset(continuation_task, 0, sizeof(atl_task_t));
        lock(&mutex_all_tasks_);
        AllTasks.push_back(continuation_task);
        continuation_task->id = AllTasks.size();
        unlock(&mutex_all_tasks_);
        PublicTaskMap[lparm->task->continuation] = continuation_task;
        DEBUG_PRINT("Continuation Map[%p] = %p (%s)\n", lparm->task->continuation, PublicTaskMap[lparm->task->continuation], kernel_name);
        pthread_mutex_init(&(continuation_task->mutex), NULL);
    }

    if(atl_kernels[kernel_id].devtype == ATMI_DEVTYPE_GPU) {
        atl_kernel_info_t info = KernelInfoTable[std::string(atl_kernels[kernel_id].gpu_kernel.kernel_name)];
        ret->gpu_kernargptr = (void*) thisKernargAddress;
        ret->kernel_object = info.kernel_object; 
        ret->private_segment_size = info.private_segment_size;
        ret->group_segment_size = info.group_segment_size;
    } 
    else if(atl_kernels[kernel_id].devtype == ATMI_DEVTYPE_CPU) {
        ret->cpu_kernelargs = thisKernargAddress;
        ret->cpu_kernelid = kernel_id;
    }
    ret->num_params = atl_kernels[kernel_id].num_params;
    ret->devtype = atl_kernels[kernel_id].devtype;
    ret->profilable = lparm->profilable;
    ret->atmi_task = lparm->task;
    //memcpy(&(ret->lparm), lparm, sizeof(atmi_lparm_t));
    ret->lparm = *lparm;
    DEBUG_PRINT("Requires LHS: %p and RHS: %p\n", ret->lparm.requires, lparm->requires);
    DEBUG_PRINT("Requires ThisTask: %p and ThisTask: %p\n", ret->lparm.task, lparm->task);

    ret->lparm.stream = stream;
    ret->stream_obj = stream_obj;
    DEBUG_PRINT("Stream LHS: %p and RHS: %p\n", ret->lparm.stream, lparm->stream);
    ret->num_predecessors = 0;
    ret->num_successors = 0;

    /* For dependent child tasks, add dependent parent kernels to barriers.  */
    DEBUG_PRINT("Pif %s requires %d task\n", kernel_name, lparm->num_required);

    ret->predecessors.clear();
    for(int idx = 0; idx < lparm->num_required; idx++) {
        atl_task_t *pred_task = PublicTaskMap[lparm->requires[idx]];
        assert(pred_task != NULL);
        ret->predecessors.push_back(pred_task);
    }

    bool should_dispatch = true;
    bool should_register_callback = true;
    if(g_dep_sync_type == ATL_SYNC_CALLBACK) {
        should_dispatch = try_dispatch_callback(ret);
        should_register_callback = (ret->atmi_task != NULL);
    }
    else if(g_dep_sync_type == ATL_SYNC_BARRIER_PKT) {
        should_dispatch = try_dispatch_barrier_pkt(ret);
        should_register_callback = ((ret->atmi_task != NULL) ||  
                (ret->atmi_task == NULL && !(ret->and_predecessors.empty())));
    }

   // TryLaunchTimer.Stop();
    if(should_dispatch) {
        if(ret->atmi_task) {
            ret->atmi_task->handle = (void *)(&(ret->signal));
        }
        direct_dispatch++;
        dispatch_task(ret);
        if(should_register_callback) {
            hsa_status_t err = hsa_amd_signal_async_handler(ret->signal,
                    HSA_SIGNAL_CONDITION_EQ, 0,
                    handle_signal, (void *)ret);
            ErrorCheck(Creating signal handler, err);
        }
    }

    if ( lparm->synchronous == ATMI_TRUE ) { /*  Sychronous execution */
        /* For default synchrnous execution, wait til kernel is finished.  */
    //    TaskWaitTimer.Start();
        atl_task_wait(ret);
        set_task_state(ret, ATMI_COMPLETED);
        set_task_metrics(ret, atl_kernels[kernel_id].devtype, lparm->profilable);
      //  TaskWaitTimer.Stop();
        //std::cout << "Task Wait Interim Timer " << TaskWaitTimer << std::endl;
    }
    else {
        /* add task to the corresponding row in the stream table */
        if(ret->atmi_task)
            register_task(stream_obj, ret, atl_kernels[kernel_id].devtype, lparm->profilable);
    }
    #if 0
    if(strcmp(pif_name, "__sync_kernel_pif") == 0) {
        /*std::cout << "Launch Time: " << TryLaunchTimer << std::endl;
        std::cout << "Try Dispatch Timer: " << TryDispatchTimer << std::endl;
        std::cout << "Signal Timer: " << SignalAddTimer << std::endl;
        std::cout << "Task Wait Time: " << TaskWaitTimer << std::endl;
        std::cout << "Handle Signal Timer: " << HandleSignalTimer << std::endl;
        std::cout << "Handle Signal Invoke Timer: " << HandleSignalInvokeTimer << std::endl;
        std::cout << "Max Ready Queue Size: " << max_ready_queue_sz << std::endl;
        std::cout << "Waiting Tasks: " << waiting_count << std::endl;
        std::cout << "Direct Dispatch Tasks: " << direct_dispatch << std::endl;
        std::cout << "Callback Dispatch Tasks: " << callback_dispatch << std::endl;
        */
        std::cout << "SYNC_TASK(" << ret->id << ");" << std::endl;
        max_ready_queue_sz = 0;
        waiting_count = 0;
        direct_dispatch = 0;
        callback_dispatch = 0;
    }
    #endif
    return ret->atmi_task;
}

atmi_task_t *atl_trylaunch_pif(const atmi_lparm_t *lparm,
                 hsa_executable_t *p_executable,
                 const char *pif_name,
                 void *thisKernargAddress) {
    DEBUG_PRINT("GPU Place Info: %d, %lx %lx\n", lparm->place.node_id, lparm->place.cpu_set, lparm->place.gpu_set);
    hsa_executable_t executable = *p_executable;
#if 1
    static bool is_called = false;
    if(!is_called) {
        set_thread_affinity(2);

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
    atmi_stream_t *stream = NULL;
    if(lparm->stream == NULL) {
        stream = &atl_default_stream_obj;
    } else {
        stream = lparm->stream;
    }
    /* Add row to stream table for purposes of future synchronizations */
    register_stream(stream);
    atmi_stream_table_t *stream_obj = StreamTable[stream];

    if(lparm->synchronous == ATMI_TRUE && lparm->num_required == 0 && lparm->num_needs_any == 0
            //&& stream->ordered == ATMI_TRUE
      ) { 
        // Greg's logic is to flush the entire stream if the task is sync and has no dependencies */
        // FIXME: This has to change for unordered streams. Why should we flush the entire stream 
        // for every sync kernel in an unordered stream?
        atl_stream_sync(stream_obj);
    }

    //TryLaunchTimer.Start();
    uint16_t i;
    atl_task_t *ret = NULL;
    atl_task_t *continuation_task = NULL;
    bool has_continuation = false;
    if(lparm->task) {
        has_continuation = (lparm->task->continuation != NULL);
        if(PublicTaskMap.find(lparm->task) != PublicTaskMap.end()) {
            // task is already found, so use it and dont create a new one
            ret = PublicTaskMap[lparm->task];
        }
    }
    if(ret == NULL) {
        ret = new atl_task_t;
        memset(ret, 0, sizeof(atl_task_t));
        lock(&mutex_all_tasks_);
        AllTasks.push_back(ret);
        //AllTasks.push_back(atl_task_t());
        //AllTasks.emplace_back();
        //ret = AllTasks[AllTasks.size() - 1];
        ret->id = AllTasks.size();
        //AllTasks.push_back(task); 
        //atl_task_t *ret = task; //AllTasks[AllTasks.size() - 1];
        unlock(&mutex_all_tasks_);
        PublicTaskMap[lparm->task] = ret;
        DEBUG_PRINT("Task Map[%p] = %p (%s)\n", lparm->task, PublicTaskMap[lparm->task], pif_name);
        pthread_mutex_init(&(ret->mutex), NULL);
    }
    if(has_continuation) {
        continuation_task = new atl_task_t;
        memset(continuation_task, 0, sizeof(atl_task_t));
        lock(&mutex_all_tasks_);
        AllTasks.push_back(continuation_task);
        continuation_task->id = AllTasks.size();
        unlock(&mutex_all_tasks_);
        PublicTaskMap[lparm->task->continuation] = continuation_task;
        DEBUG_PRINT("Continuation Map[%p] = %p (%s)\n", lparm->task->continuation, PublicTaskMap[lparm->task->continuation], pif_name);
        pthread_mutex_init(&(continuation_task->mutex), NULL);
    }

    int this_kernel_iter = 0;
    const char *pif_found_name = NULL;
    const char *gpu_kernel_name = NULL;
    const char *cpu_kernel_name = NULL;
    atmi_devtype_t devtype;
    int num_params;
    int kernel_id;
    for(i = 0; i < SNK_MAX_FUNCTIONS; i++) {
        //DEBUG_PRINT("Comparing kernels %s %s\n", atl_kernels[i].pif_name, pif_name);
        if(atl_kernels[i].pif_name && pif_name) {
            if(strcmp(atl_kernels[i].pif_name, pif_name) == 0) // && atl_kernels[i].gpu_kernel.kernel_name != NULL) 
            {
                if(this_kernel_iter != lparm->kernel_id) {
                    this_kernel_iter++;
                }
                else {
                    devtype = atl_kernels[i].devtype;
                    pif_found_name = atl_kernels[i].pif_name;
                    gpu_kernel_name = atl_kernels[i].gpu_kernel.kernel_name;
                    cpu_kernel_name = atl_kernels[i].cpu_kernel.kernel_name;
                    num_params = atl_kernels[i].num_params;
                    kernel_id = i;
                    break;
                }
            }
        }
    }
    if(pif_found_name == NULL) {
        fprintf(stderr, "ERROR: Kernel/PIF %s not found\n", pif_name);
        return NULL;
    }

    if(devtype == ATMI_DEVTYPE_GPU) {
        uint64_t _KN__Kernel_Object;
        uint32_t _KN__Group_Segment_Size;
        uint32_t _KN__Private_Segment_Size;
        uint32_t _KN__Kernarg_Size;

        atl_get_gpu_kernel_info(executable, gpu_kernel_name, &_KN__Kernel_Object, 
                &_KN__Group_Segment_Size, &_KN__Private_Segment_Size,
                &_KN__Kernarg_Size);
        /* thisKernargAddress has already been set up in the beginning of this routine */
        /*  Bind kernel argument buffer to the aql packet.  */
        ret->gpu_kernargptr = (void*) thisKernargAddress;
        atl_kernel_info_t info = KernelInfoTable[std::string(gpu_kernel_name)];
        ret->kernel_object = info.kernel_object; 
        ret->private_segment_size = info.private_segment_size;
        ret->group_segment_size = info.group_segment_size;
    } 
    else if(devtype == ATMI_DEVTYPE_CPU) {
        ret->cpu_kernelargs = thisKernargAddress;
        ret->cpu_kernelid = kernel_id;
    }
    ret->num_params = num_params;
    ret->devtype = devtype;
    ret->profilable = lparm->profilable;
    ret->atmi_task = lparm->task;
    //memcpy(&(ret->lparm), lparm, sizeof(atmi_lparm_t));
    ret->lparm = *lparm;
    DEBUG_PRINT("Requires LHS: %p and RHS: %p\n", ret->lparm.requires, lparm->requires);
    DEBUG_PRINT("Requires ThisTask: %p and ThisTask: %p\n", ret->lparm.task, lparm->task);

    ret->lparm.stream = stream;
    ret->stream_obj = stream_obj;
    DEBUG_PRINT("Stream LHS: %p and RHS: %p\n", ret->lparm.stream, lparm->stream);
    ret->num_predecessors = 0;
    ret->num_successors = 0;

    /* For dependent child tasks, add dependent parent kernels to barriers.  */
    DEBUG_PRINT("Pif %s requires %d task\n", pif_name, lparm->num_required);

    ret->predecessors.clear();
    for(int idx = 0; idx < lparm->num_required; idx++) {
        atl_task_t *pred_task = PublicTaskMap[lparm->requires[idx]];
        assert(pred_task != NULL);
        ret->predecessors.push_back(pred_task);
    }

    bool should_dispatch = true;
    bool should_register_callback = true;
    if(g_dep_sync_type == ATL_SYNC_CALLBACK) {
        should_dispatch = try_dispatch_callback(ret);
        should_register_callback = (ret->atmi_task != NULL);
    }
    else if(g_dep_sync_type == ATL_SYNC_BARRIER_PKT) {
        should_dispatch = try_dispatch_barrier_pkt(ret);
        should_register_callback = ((ret->atmi_task != NULL) ||  
                (ret->atmi_task == NULL && !(ret->and_predecessors.empty())));
    }

   // TryLaunchTimer.Stop();
    if(should_dispatch) {
        if(ret->atmi_task) {
            ret->atmi_task->handle = (void *)(&(ret->signal));
        }
        direct_dispatch++;
        dispatch_task(ret);
        if(should_register_callback) {
            hsa_status_t err = hsa_amd_signal_async_handler(ret->signal,
                    HSA_SIGNAL_CONDITION_EQ, 0,
                    handle_signal, (void *)ret);
            ErrorCheck(Creating signal handler, err);
        }
    }

    if ( lparm->synchronous == ATMI_TRUE ) { /*  Sychronous execution */
        /* For default synchrnous execution, wait til kernel is finished.  */
    //    TaskWaitTimer.Start();
        atl_task_wait(ret);
        set_task_state(ret, ATMI_COMPLETED);
        set_task_metrics(ret, devtype, lparm->profilable);
      //  TaskWaitTimer.Stop();
        //std::cout << "Task Wait Interim Timer " << TaskWaitTimer << std::endl;
    }
    else {
        /* add task to the corresponding row in the stream table */
        if(ret->atmi_task)
            register_task(stream_obj, ret, devtype, lparm->profilable);
    }
    #if 0
    if(strcmp(pif_name, "__sync_kernel_pif") == 0) {
        /*std::cout << "Launch Time: " << TryLaunchTimer << std::endl;
        std::cout << "Try Dispatch Timer: " << TryDispatchTimer << std::endl;
        std::cout << "Signal Timer: " << SignalAddTimer << std::endl;
        std::cout << "Task Wait Time: " << TaskWaitTimer << std::endl;
        std::cout << "Handle Signal Timer: " << HandleSignalTimer << std::endl;
        std::cout << "Handle Signal Invoke Timer: " << HandleSignalInvokeTimer << std::endl;
        std::cout << "Max Ready Queue Size: " << max_ready_queue_sz << std::endl;
        std::cout << "Waiting Tasks: " << waiting_count << std::endl;
        std::cout << "Direct Dispatch Tasks: " << direct_dispatch << std::endl;
        std::cout << "Callback Dispatch Tasks: " << callback_dispatch << std::endl;
        */
        std::cout << "SYNC_TASK(" << ret->id << ");" << std::endl;
        max_ready_queue_sz = 0;
        waiting_count = 0;
        direct_dispatch = 0;
        callback_dispatch = 0;
    }
    #endif
    return ret->atmi_task;
}

// below exploration for nested tasks
atmi_task_t *atl_launch_cpu_kernel(const atmi_lparm_t *lparm, 
                 const char *kernel_name,
                 void *kernel_args) {
    atmi_task_t *ret = NULL;
    /*if(lparm->nested == ATMI_TRUE) {
        ret = atl_create_cpu_kernel(lparm, kernel_name,
                 kernel_args);
    }
    else {
        ret = atl_cpu_kernel(lparm, kernel_name,
                 kernel_args);
    }
    */
    return ret;
}

atmi_task_t *atmi_task_launch(atmi_kernel_t kernel, atmi_lparm_t *lparm, void **args, size_t *arg_sizes, const int num_args) {
    const char *pif_name = (const char *)(kernel.handle);
    int this_kernel_iter = 0;
    const char *pif_found_name = NULL;
    const char *gpu_kernel_name = NULL;
    const char *cpu_kernel_name = NULL;
    atmi_devtype_t devtype;
    int actual_num_args = num_args + 1; // to include atmi_task_t *
    int kernel_id;
    for(int i = 0; i < SNK_MAX_FUNCTIONS; i++) {
        DEBUG_PRINT("Comparing kernels[%d] %s %s\n", i, atl_kernels[i].pif_name, pif_name);
        if(atl_kernels[i].pif_name && pif_name) {
            if(strcmp(atl_kernels[i].pif_name, pif_name) == 0) // && atl_kernels[i].gpu_kernel.kernel_name != NULL) 
            {
                if(this_kernel_iter != lparm->kernel_id) {
                    this_kernel_iter++;
                }
                else {
                    devtype = atl_kernels[i].devtype;
                    pif_found_name = atl_kernels[i].pif_name;
                    gpu_kernel_name = atl_kernels[i].gpu_kernel.kernel_name;
                    cpu_kernel_name = atl_kernels[i].cpu_kernel.kernel_name;
                    //num_params = atl_kernels[i].num_params;
                    atl_kernels[i].num_params = actual_num_args;
                    kernel_id = i;
                    break;
                }
            }
        }
    }
    if(pif_found_name == NULL) {
        fprintf(stderr, "ERROR: Kernel/PIF %s not found\n", pif_name);
        return NULL;
    }
    char *thisKernargAddress;
    if(devtype == ATMI_DEVTYPE_CPU) {
        thisKernargAddress = (char *)malloc(actual_num_args * sizeof(char *));
        char *kernargPtr = (char *)thisKernargAddress;
        thisKernargAddress += sizeof(char *);
        for(int i = 0; i < num_args; i++) {
            void *arg_ptr = malloc(arg_sizes[i]);
            memcpy(arg_ptr, args[i], arg_sizes[i]);
            *(char **)thisKernargAddress = (char *)arg_ptr;
            thisKernargAddress += sizeof(char *);
        }
        
        return atl_trylaunch_kernel(kernel_id, lparm, cpu_kernel_name, kernargPtr);
    }
    else if(devtype == ATMI_DEVTYPE_GPU) {
        atl_kernel_info_t info = KernelInfoTable[std::string(gpu_kernel_name)];
        uint32_t kernel_segment_size = info.kernel_segment_size;
        hsa_status_t err = hsa_memory_allocate(atl_gpu_KernargRegion, kernel_segment_size, (void **)&thisKernargAddress);
        ErrorCheck(Allocating memory for the executable-kernel, err);
        // skip over the six special HSA kernel arguments
        char *kernargPtr = thisKernargAddress;
        thisKernargAddress += (6 * sizeof(uint64_t));
        thisKernargAddress += sizeof(char *);
        for(int i = 0; i < num_args; i++) {
            memcpy(thisKernargAddress, args[i], arg_sizes[i]);
            thisKernargAddress += arg_sizes[i];
        }

        return atl_trylaunch_kernel(kernel_id, lparm, gpu_kernel_name, kernargPtr);
    }
}

enum queue_type{device_queue = 0, soft_queue}; 
void atl_kl_init(atmi_klist_t *atmi_klist,
        const char *pif_name,
        const int pif_id) {

    atmi_stream_t *stream  = &atl_default_stream_obj;

    /* Add row to stream table for purposes of future synchronizations */
    register_stream(stream);
    atmi_stream_table_t *stream_obj = StreamTable[stream];

    atmi_klist_t *atmi_klist_curr = atmi_klist + pif_id; 

    /* get this stream's HSA queue (could be dynamically mapped or round robin
     * if it is an unordered stream */
    hsa_queue_t* this_devQ = acquire_and_set_next_gpu_queue(stream_obj);
    if(!this_devQ) return;

    hsa_queue_t* this_softQ = acquire_and_set_next_cpu_queue(stream_obj);
    if(!this_softQ) return;

    atmi_klist_curr->num_queues = 2;
    //atmi_klist_curr->queues = (uint64_t *)malloc(sizeof(uint64_t) * atmi_klist_curr->num_queues);

    atmi_klist_curr->queues[device_queue] = (uint64_t)this_devQ; 
    atmi_klist_curr->queues[soft_queue] = (uint64_t)this_softQ; 
    atmi_klist_curr->worker_sig = (uint64_t)get_worker_sig(this_softQ); 

    uint64_t _KN__Kernel_Object;
    uint32_t _KN__Group_Segment_Size;
    uint32_t _KN__Private_Segment_Size;
    uint32_t _KN__Kernarg_Size;

    /* Allocate the kernel argument buffer from the correct region. */
    //void* thisKernargAddress;
    //atl_gpu_memory_allocate(lparm, g_executable, pif_name, &thisKernargAddress);

    uint16_t i;
    atmi_task_t *ret = NULL;
    for(i = 0; i < SNK_MAX_FUNCTIONS; i++) {
        if(atl_kernels[i].pif_name && pif_name) {
            if(strcmp(atl_kernels[i].pif_name, pif_name) == 0) { 
                if(atl_kernels[i].devtype == ATMI_DEVTYPE_GPU) {

                    std::string cl_pif_name("&__OpenCL_");
                    cl_pif_name += std::string(atl_kernels[i].gpu_kernel.kernel_name);
                    cl_pif_name += std::string("_kernel");
                    atl_kernel_info_t info = KernelInfoTable[cl_pif_name];
                    _KN__Kernel_Object = info.kernel_object;
                    _KN__Group_Segment_Size = info.group_segment_size;
                    _KN__Private_Segment_Size = info.private_segment_size;
                    _KN__Kernarg_Size = info.kernel_segment_size;

                    atmi_klist_curr->num_kernel_packets++;
                    atmi_klist_curr->kernel_packets = 
                        (atmi_kernel_packet_t *)realloc(
                                atmi_klist_curr->kernel_packets, 
                                sizeof(atmi_kernel_packet_t) * 
                                atmi_klist_curr->num_kernel_packets);

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
                else if(atl_kernels[i].devtype == ATMI_DEVTYPE_CPU){
                    atmi_klist_curr->num_kernel_packets++;
                    atmi_klist_curr->kernel_packets = 
                        (atmi_kernel_packet_t *)realloc(
                                atmi_klist_curr->kernel_packets, 
                                sizeof(atmi_kernel_packet_t) * 
                                atmi_klist_curr->num_kernel_packets);

                    hsa_agent_dispatch_packet_t *this_aql = 
                        (hsa_agent_dispatch_packet_t *)(atmi_klist_curr
                                ->kernel_packets +
                                atmi_klist_curr->num_kernel_packets - 1);
                    this_aql->header = 1;
                    this_aql->type = (uint16_t)i;
                    const uint32_t num_params = atl_kernels[i].num_params;
                    //ret = (atmi_task_t*) &(SNK_Tasks[SNK_NextTaskId]);
                    this_aql->arg[0] = num_params;
                    //this_aql->arg[1] = (uint64_t) cpu_kernel_args;
                    //this_aql->arg[2] = (uint64_t) ret; 
                    this_aql->arg[3] = UINT64_MAX; 
                    //SNK_NextTaskId++;
                }
            }
        }
    }
}
