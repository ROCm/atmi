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

/* This file is the SNACK library. Idea is to move as much of code as possible
 * from the snk_genw.sh script to a library
 */
#include "snk_internal.h"
#include <time.h>
#include <assert.h>

#define NSECPERSEC 1000000000L

/*  set NOTCOHERENT needs this include
#include "hsa_ext_amd.h"
*/

/* -------------- Helper functions -------------------------- */
#define ErrorCheck(msg, status) \
if (status != HSA_STATUS_SUCCESS) { \
    printf("%s failed. %x\n", #msg, status); \
    /*exit(1); */\
} else { \
 /*  printf("%s succeeded.\n", #msg);*/ \
}

/* Stream table to hold the runtime state of the 
 * stream and its tasks. Which was the latest 
 * device used, latest queue used and also a 
 * pool of tasks for synchronization if need be */
atmi_stream_table_t StreamTable[ATMI_MAX_STREAMS];
//atmi_task_table_t TaskTable[SNK_MAX_TASKS];

/* Stream specific globals */
hsa_agent_t* snk_gpu_agent;
hsa_agent_t* snk_cpu_agent;
hsa_queue_t* GPU_CommandQ[SNK_MAX_GPU_QUEUES];
atmi_task_t   SNK_Tasks[SNK_MAX_TASKS];
hsa_signal_t  SNK_Signals[SNK_MAX_TASKS];
int          SNK_NextTaskId = 0 ;
atmi_stream_t snk_default_stream_obj = {ATMI_ORDERED};
int          SNK_NextGPUQueueID[ATMI_MAX_STREAMS];
int          SNK_NextCPUQueueID[ATMI_MAX_STREAMS];

struct timespec context_init_time;
static int context_init_time_init = 0;
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

hsa_signal_t enqueue_barrier(hsa_queue_t *queue, const int dep_task_count, atmi_task_t **dep_task_list) {
    /* This routine will enqueue a barrier packet for all dependent packets to complete
       irrespective of their stream
     */

    long t_barrier_wait = 0L;
    long t_barrier_dispatch = 0L;
    /* Keep adding barrier packets in multiples of 5 because that is the maximum signals that 
       the HSA barrier packet can support today
     */
    hsa_signal_t last_signal;
    if(queue == NULL || dep_task_list == NULL || dep_task_count <= 0) return last_signal;
    hsa_signal_create(0, 0, NULL, &last_signal);
    atmi_task_t **tasks = dep_task_list;
    int tasks_remaining = dep_task_count;
    const int HSA_BARRIER_MAX_DEPENDENT_TASKS = 4;
    /* round up */
    int barrier_pkt_count = (dep_task_count + HSA_BARRIER_MAX_DEPENDENT_TASKS - 1) / HSA_BARRIER_MAX_DEPENDENT_TASKS;
    int barrier_pkt_id = 0;

    for(barrier_pkt_id = 0; barrier_pkt_id < barrier_pkt_count; barrier_pkt_id++) {
        hsa_signal_t signal;
        hsa_signal_create(1, 0, NULL, &signal);
        /* Obtain the write index for the command queue for this stream.  */
        uint64_t index = hsa_queue_load_write_index_relaxed(queue);
        const uint32_t queueMask = queue->size - 1;
        /* Define the barrier packet to be at the calculated queue index address.  */
        hsa_barrier_and_packet_t* barrier = &(((hsa_barrier_and_packet_t*)(queue->base_address))[index&queueMask]);
        memset(barrier, 0, sizeof(hsa_barrier_and_packet_t));
        barrier->header = create_header(HSA_PACKET_TYPE_BARRIER_AND, 0);

        /* populate all dep_signals */
        int dep_signal_id = 0;
        int iter = 0;
        for(dep_signal_id = 0; dep_signal_id < HSA_BARRIER_MAX_DEPENDENT_TASKS; dep_signal_id++) {
            if(*tasks != NULL && tasks_remaining > 0) {
                DEBUG_PRINT("Barrier Packet %d\n", iter);
                iter++;
                /* fill out the barrier packet and ring doorbell */
                barrier->dep_signal[dep_signal_id] = *((hsa_signal_t *)((*tasks)->handle)); 
                DEBUG_PRINT("Enqueue wait for signal handle: %" PRIu64 "\n", barrier->dep_signal[dep_signal_id].handle);
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

void enqueue_barrier_gpu(hsa_queue_t *queue, const int dep_task_count, atmi_task_t **dep_task_list, int wait_flag) {
    hsa_signal_t last_signal = enqueue_barrier(queue, dep_task_count, dep_task_list);
    /* Wait on completion signal if blockine */
    if(wait_flag == SNK_WAIT) {
        hsa_signal_wait_acquire(last_signal, HSA_SIGNAL_CONDITION_EQ, 0, UINT64_MAX, HSA_WAIT_STATE_BLOCKED);
        hsa_signal_destroy(last_signal);
    }
}

void enqueue_barrier_cpu(hsa_queue_t *queue, const int dep_task_count, atmi_task_t **dep_task_list, int wait_flag) {
    hsa_signal_t last_signal = enqueue_barrier(queue, dep_task_count, dep_task_list);
    signal_worker(queue, PROCESS_PKT);
    /* Wait on completion signal if blockine */
    if(wait_flag == SNK_WAIT) {
        hsa_signal_wait_acquire(last_signal, HSA_SIGNAL_CONDITION_EQ, 0, UINT64_MAX, HSA_WAIT_STATE_BLOCKED);
        hsa_signal_destroy(last_signal);
    }
}

extern void snk_task_wait(atmi_task_t *task) {
    if(task != NULL) {
        DEBUG_PRINT("Signal Value: %" PRIu64 "\n", ((hsa_signal_t *)(task->handle))->handle);
        hsa_signal_wait_acquire(*((hsa_signal_t *)(task->handle)), HSA_SIGNAL_CONDITION_EQ, 0, UINT64_MAX, HSA_WAIT_STATE_BLOCKED);
        /* Flag this task as completed */
        /* FIXME: How can HSA tell us if and when a task has failed? */
        task->state = ATMI_COMPLETED;
    }

    return;// STATUS_SUCCESS;
}

extern void atmi_task_wait(atmi_task_t *task) {
    snk_task_wait(task);
}

status_t queue_sync(hsa_queue_t *queue) {
    if(queue == NULL) return STATUS_SUCCESS;
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
    hsa_signal_wait_acquire(signal, HSA_SIGNAL_CONDITION_EQ, 0, UINT64_MAX, HSA_WAIT_STATE_BLOCKED);
    hsa_signal_destroy(signal);

    return STATUS_SUCCESS;
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

int get_stream_id(atmi_stream_t *stream) {
    int stream_id;
    int ret_stream_id = -1;
    for(stream_id = 0; stream_id < ATMI_MAX_STREAMS; stream_id++) {
        if(StreamTable[stream_id].stream != NULL) {
            if(StreamTable[stream_id].stream == stream) {
                /* stream found */
                ret_stream_id = stream_id;
                break;
            }
        }
    }
    return ret_stream_id;
}

extern void snk_stream_sync(atmi_stream_t *stream) {
    int stream_num = get_stream_id(stream); 
    if(stream_num == -1) {
        /* simply return because this is as good as a no-op */
        return;// STATUS_SUCCESS;
    }
    if(StreamTable[stream_num].tasks == NULL ) return;
    if(stream->ordered == ATMI_TRUE) {
        /* just insert a barrier packet to the CPU and GPU queues and wait */
        DEBUG_PRINT("Waiting for GPU Q\n");
        queue_sync(StreamTable[stream_num].gpu_queue);
        DEBUG_PRINT("Waiting for CPU Q\n");
        queue_sync(StreamTable[stream_num].cpu_queue);
        if(StreamTable[stream_num].cpu_queue) 
            signal_worker(StreamTable[stream_num].cpu_queue, PROCESS_PKT);
    }
    else {
        /* wait on each one of the tasks in the task bag */
        #if 1
        snk_task_list_t *task_head = StreamTable[stream_num].tasks;
        printf("Waiting for async unordered tasks\n");
        while(task_head) {
            snk_task_wait(task_head->task);
            task_head = task_head->next;
        }
        #else
        int num_tasks = 0;
        snk_task_list_t *task_head = StreamTable[stream_num].tasks;
        while(task_head) {
            num_tasks++;
            task_head = task_head->next;
        }
        if(num_tasks > 0) {
            atmi_task_t **tasks = (atmi_task_t **)malloc(sizeof(atmi_task_t *) * num_tasks);
            int task_id = 0;
            task_head = StreamTable[stream_num].tasks;
            for(task_id = 0; task_id < num_tasks; task_id++) {
                tasks[task_id] = task_head->task;
                task_head = task_head->next;
            }
            
            if(StreamTable[stream_num].gpu_queue != NULL) 
                enqueue_barrier_gpu(StreamTable[stream_num].gpu_queue, num_tasks, tasks, SNK_WAIT);
            else if(StreamTable[stream_num].cpu_queue != NULL) 
                enqueue_barrier_cpu(StreamTable[stream_num].cpu_queue, num_tasks, tasks, SNK_WAIT);
            else {
                int idx; 
                for(idx = 0; idx < num_tasks; idx++) {
                    snk_task_wait(tasks[idx]);
                }
            }
            free(tasks);
        }
        #endif
    }
    clear_saved_tasks(stream);
}

extern void atmi_stream_sync(atmi_stream_t *stream) {
    atmi_stream_t *str = (stream == NULL) ? &snk_default_stream_obj : stream;
    snk_stream_sync(str);
}

hsa_queue_t *acquire_and_set_next_cpu_queue(atmi_stream_t *stream) {
    int stream_num = get_stream_id(stream); 
    if(stream_num == -1) {
        DEBUG_PRINT("Stream unregistered\n");
        return NULL;
    }
    int ret_queue_id = SNK_NextCPUQueueID[stream_num];
    /* use the same queue if the stream is ordered */
    /* otherwise, round robin the queue ID for unordered streams */
    if(stream->ordered == ATMI_FALSE) {
        SNK_NextCPUQueueID[stream_num] = (ret_queue_id + 1) % SNK_MAX_CPU_QUEUES;
    }
    hsa_queue_t *queue = get_cpu_queue(ret_queue_id);
    StreamTable[stream_num].cpu_queue = queue;
    return queue;
}

hsa_queue_t *acquire_and_set_next_gpu_queue(atmi_stream_t *stream) {
    int stream_num = get_stream_id(stream); 
    if(stream_num == -1) {
        DEBUG_PRINT("Stream unregistered\n");
        return NULL;
    }
    int ret_queue_id = SNK_NextGPUQueueID[stream_num];
    /* use the same queue if the stream is ordered */
    /* otherwise, round robin the queue ID for unordered streams */
    if(stream->ordered == ATMI_FALSE) {
        SNK_NextGPUQueueID[stream_num] = (ret_queue_id + 1) % SNK_MAX_GPU_QUEUES;
    }
    hsa_queue_t *queue = GPU_CommandQ[ret_queue_id];
    StreamTable[stream_num].gpu_queue = queue;
    return queue;
}

status_t clear_saved_tasks(atmi_stream_t *stream) {
    int stream_num = get_stream_id(stream); 
    if(stream_num == -1) {
        DEBUG_PRINT("Stream unregistered\n");
        return STATUS_ERROR;
    }
   
    hsa_status_t err;
    snk_task_list_t *cur = StreamTable[stream_num].tasks;
    snk_task_list_t *prev = cur;
    while(cur != NULL ){
        atmi_task_t* task = cur->task;
        if(task->profile != NULL) {
            hsa_signal_t signal = *(hsa_signal_t *)(task->handle);
            hsa_amd_dispatch_time_t metrics;
            if(cur->devtype == ATMI_DEVTYPE_GPU) {
                err = hsa_ext_get_dispatch_times(*snk_gpu_agent, 
                        signal, &metrics); 
                ErrorCheck(Profiling GPU dispatch, err);
                task->profile->start_time = metrics.start;
                task->profile->end_time = metrics.end;
            }
            else {
                /* metrics for CPU tasks will be populated in the 
                 * worker pthread itself. No special function call */
            }
        }
        cur = cur->next;
        free(prev);
        prev = cur;
    }

    StreamTable[stream_num].tasks = NULL;

    return STATUS_SUCCESS;
}

status_t check_change_in_device_type(atmi_stream_t *stream, hsa_queue_t *queue, atmi_devtype_t new_task_device_type) {
    if(stream->ordered != ATMI_ORDERED) return STATUS_SUCCESS;

    int stream_num = get_stream_id(stream); 
    if(stream_num == -1) {
        DEBUG_PRINT("Stream unregistered\n");
        return STATUS_ERROR;
    }

    if(StreamTable[stream_num].tasks != NULL) {
        if(StreamTable[stream_num].last_device_type != new_task_device_type) {
            DEBUG_PRINT("Devtype: %d waiting for task %p\n", new_task_device_type, StreamTable[stream_num].tasks->task);
            /* device changed. introduce a dependency here for ordered streams */
            int num_required = 1;
            atmi_task_t *requires = StreamTable[stream_num].tasks->task;

            if(new_task_device_type == ATMI_DEVTYPE_GPU) {
                if(queue) {
                    enqueue_barrier_gpu(queue, num_required, &requires, SNK_NOWAIT);
                }
            }
            else {
                if(queue) {
                    enqueue_barrier_cpu(queue, num_required, &requires, SNK_NOWAIT);
                }
            }
        }
    }
}

status_t register_task(atmi_stream_t *stream, atmi_task_t *task, atmi_devtype_t devtype) {
    int stream_num = get_stream_id(stream); 
    if(stream_num == -1) {
        DEBUG_PRINT("Stream unregistered\n");
        return STATUS_ERROR;
    }

    snk_task_list_t *node = (snk_task_list_t *)malloc(sizeof(snk_task_list_t));
    node->task = task;
    node->next = NULL;
    node->devtype = devtype;
    if(StreamTable[stream_num].tasks == NULL) {
        StreamTable[stream_num].tasks = node;
    } else {
        snk_task_list_t *cur = StreamTable[stream_num].tasks;
        StreamTable[stream_num].tasks = node;
        node->next = cur;
    }
    StreamTable[stream_num].last_device_type = devtype;
    DEBUG_PRINT("Registering %s task %p\n", 
                (devtype == ATMI_DEVTYPE_GPU) ? "GPU" : "CPU",
                task);
    return STATUS_SUCCESS;
}

status_t register_stream(atmi_stream_t *stream) {
    /* Check if the stream exists in the stream table. 
     * If no, then add this stream to the stream table.
     */
    int stream_id;
    int stream_found = 0;
    for(stream_id = 0; stream_id < ATMI_MAX_STREAMS; stream_id++) {
        if(StreamTable[stream_id].stream != NULL) {
            if(StreamTable[stream_id].stream == stream) {
                /* stream found */
                stream_found = 1;
                break;
            }
        }
        else {
            /* insert stream table entry at the first NULL row */
            break;
        }
    }
    if(stream_id >= ATMI_MAX_STREAMS) {
       printf(" ERROR! Too many streams created! Stream count must be less than %d.\n", ATMI_MAX_STREAMS);
       return STATUS_ERROR;
    }
    if(stream_found == 0) {
       /* insert stream table entry at index = last_stream_id */
       StreamTable[stream_id].stream = stream;
    }

    return STATUS_SUCCESS;
}

/* -------------- SNACK launch functions -------------------------- */
status_t snk_init_context(
                        hsa_agent_t *_CN__Agent, 
                        hsa_ext_module_t **_CN__BrigModule,
                        hsa_ext_program_t *_CN__HsaProgram,
                        hsa_executable_t *_CN__Executable,
                        hsa_region_t *_CN__KernargRegion,
                        hsa_agent_t *_CN__CPU_Agent,
                        hsa_region_t *_CN__CPU_KernargRegion
                        ) {

    hsa_status_t err;

    err = hsa_init();
    ErrorCheck(Initializing the hsa runtime, err);

    /* Get a CPU agent, create a pthread to handle packets*/
    /* Iterate over the agents and pick the cpu agent */
    err = hsa_iterate_agents(get_cpu_agent, _CN__CPU_Agent);
    if(err == HSA_STATUS_INFO_BREAK) { err = HSA_STATUS_SUCCESS; }
    ErrorCheck(Getting a gpu agent, err);

    _CN__CPU_KernargRegion->handle=(uint64_t)-1;
    err = hsa_agent_iterate_regions(*_CN__CPU_Agent, get_fine_grained_region, _CN__CPU_KernargRegion);
    if(err == HSA_STATUS_INFO_BREAK) { err = HSA_STATUS_SUCCESS; }
    err = (_CN__CPU_KernargRegion->handle == (uint64_t)-1) ? HSA_STATUS_ERROR : HSA_STATUS_SUCCESS;
    ErrorCheck(Finding a CPU kernarg memory region handle, err);

    int num_queues = SNK_MAX_CPU_QUEUES;
    int queue_capacity = 32768;
    cpu_agent_init(*_CN__CPU_Agent, *_CN__CPU_KernargRegion, num_queues, queue_capacity);

    /* Iterate over the agents and pick the gpu agent */
    err = hsa_iterate_agents(get_gpu_agent, _CN__Agent);
    if(err == HSA_STATUS_INFO_BREAK) { err = HSA_STATUS_SUCCESS; }
    ErrorCheck(Getting a gpu agent, err);
    
    snk_gpu_agent = _CN__Agent;
    snk_cpu_agent = _CN__CPU_Agent;
    /* Query the name of the agent.  */
    char name[64] = { 0 };
    err = hsa_agent_get_info(*_CN__Agent, HSA_AGENT_INFO_NAME, name);
    ErrorCheck(Querying the agent name, err);
    /* printf("The agent name is %s.\n", name); */

    /* Query the maximum size of the queue.  */
    uint32_t queue_size = 0;
    err = hsa_agent_get_info(*_CN__Agent, HSA_AGENT_INFO_QUEUE_MAX_SIZE, &queue_size);
    ErrorCheck(Querying the agent maximum queue size, err);
    /* printf("The maximum queue size is %u.\n", (unsigned int) queue_size); */

    /* Load the BRIG binary.  */
    //*_CN__BrigModule = (hsa_ext_module_t*) HSA_BrigMem;

    /* Create hsa program.  */
    memset(_CN__HsaProgram,0,sizeof(hsa_ext_program_t));
    err = hsa_ext_program_create(HSA_MACHINE_MODEL_LARGE, HSA_PROFILE_FULL, HSA_DEFAULT_FLOAT_ROUNDING_MODE_DEFAULT, NULL, _CN__HsaProgram);
    ErrorCheck(Create the program, err);

    /* Add the BRIG module to hsa program.  */
    err = hsa_ext_program_add_module(*_CN__HsaProgram, *_CN__BrigModule);
    ErrorCheck(Adding the brig module to the program, err);

    /* Determine the agents ISA.  */
    hsa_isa_t isa;
    err = hsa_agent_get_info(*_CN__Agent, HSA_AGENT_INFO_ISA, &isa);
    ErrorCheck(Query the agents isa, err);

    /* * Finalize the program and extract the code object.  */
    hsa_ext_control_directives_t control_directives;
    memset(&control_directives, 0, sizeof(hsa_ext_control_directives_t));
    hsa_code_object_t code_object;
    err = hsa_ext_program_finalize(*_CN__HsaProgram, isa, 0, control_directives, "", HSA_CODE_OBJECT_TYPE_PROGRAM, &code_object);
    ErrorCheck(Finalizing the program, err);

    /* Destroy the program, it is no longer needed.  */
    err=hsa_ext_program_destroy(*_CN__HsaProgram);
    ErrorCheck(Destroying the program, err);

    /* Create the empty executable.  */
    err = hsa_executable_create(HSA_PROFILE_FULL, HSA_EXECUTABLE_STATE_UNFROZEN, "", _CN__Executable);
    ErrorCheck(Create the executable, err);

    /* Load the code object.  */
    err = hsa_executable_load_code_object(*_CN__Executable, *_CN__Agent, code_object, "");
    ErrorCheck(Loading the code object, err);

    /* Freeze the executable; it can now be queried for symbols.  */
    err = hsa_executable_freeze(*_CN__Executable, "");
    ErrorCheck(Freeze the executable, err);

    /* Find a memory region that supports kernel arguments.  */
    _CN__KernargRegion->handle=(uint64_t)-1;
    hsa_agent_iterate_regions(*_CN__Agent, get_kernarg_memory_region, _CN__KernargRegion);
    err = (_CN__KernargRegion->handle == (uint64_t)-1) ? HSA_STATUS_ERROR : HSA_STATUS_SUCCESS;
    ErrorCheck(Finding a kernarg memory region, err);

    int task_num;
    /* Initialize all preallocated tasks and signals */
    for ( task_num = 0 ; task_num < SNK_MAX_TASKS; task_num++){
       //SNK_Tasks[task_num].next = NULL;
       //err=hsa_signal_create(1, 0, NULL, &(TaskTable[task_num].handle));
       //TaskTable[task_num].task = &(SNK_Tasks[task_num]);
       err=hsa_signal_create(1, 0, NULL, &SNK_Signals[task_num]);
       SNK_Tasks[task_num].handle = (void *)(&SNK_Signals[task_num]);
       ErrorCheck(Creating a HSA signal, err);
    }

    /* Create queues and signals for each stream. */
    int stream_num;
    for ( stream_num = 0 ; stream_num < SNK_MAX_GPU_QUEUES ; stream_num++){
        /* printf("calling queue create for stream %d\n",stream_num); */
       err=hsa_queue_create(*_CN__Agent, queue_size, HSA_QUEUE_TYPE_SINGLE, NULL, NULL, UINT32_MAX, UINT32_MAX, &GPU_CommandQ[stream_num]);
       ErrorCheck(Creating the Stream Command Q, err);
       err = hsa_ext_set_profiling( GPU_CommandQ[stream_num], 1); 
       ErrorCheck(Enabling profiling support, err); 
    }

    for(stream_num = 0; stream_num < ATMI_MAX_STREAMS; stream_num++) {
        /* round robin streams to queues */
        SNK_NextGPUQueueID[stream_num] = stream_num % SNK_MAX_GPU_QUEUES;
        SNK_NextCPUQueueID[stream_num] = stream_num % SNK_MAX_CPU_QUEUES;
    }

    if(context_init_time_init == 0) {
        clock_gettime(CLOCK_MONOTONIC_RAW,&context_init_time);
        context_init_time_init = 1;
    }
    return STATUS_SUCCESS;
}

status_t snk_init_cpu_kernel() {
    return STATUS_SUCCESS;
}

status_t snk_init_gpu_kernel(hsa_executable_symbol_t          *_KN__Symbol,
                            const char *kernel_symbol_name,
                            uint64_t                         *_KN__Kernel_Object,
                            uint32_t                         *_KN__Kernarg_Segment_Size, /* May not need to be global */
                            uint32_t                         *_KN__Group_Segment_Size,
                            uint32_t                         *_KN__Private_Segment_Size,
                            hsa_agent_t _CN__Agent, 
                            hsa_executable_t _CN__Executable
                            ) {

    hsa_status_t err;

    /* Extract the symbol from the executable.  */
    /* printf("Kernel name _KN__: Looking for symbol %s\n","__OpenCL__KN__kernel"); */
    err = hsa_executable_get_symbol(_CN__Executable, "", kernel_symbol_name, _CN__Agent , 0, _KN__Symbol);
    ErrorCheck(Extract the symbol from the executable, err);

    /* Extract dispatch information from the symbol */
    err = hsa_executable_symbol_get_info(*_KN__Symbol, HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_OBJECT, _KN__Kernel_Object);
    ErrorCheck(Extracting the symbol from the executable, err);
    err = hsa_executable_symbol_get_info(*_KN__Symbol, HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_KERNARG_SEGMENT_SIZE, _KN__Kernarg_Segment_Size);
    ErrorCheck(Extracting the kernarg segment size from the executable, err);
    err = hsa_executable_symbol_get_info(*_KN__Symbol, HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_GROUP_SEGMENT_SIZE, _KN__Group_Segment_Size);
    ErrorCheck(Extracting the group segment size from the executable, err);
    err = hsa_executable_symbol_get_info(*_KN__Symbol, HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_PRIVATE_SEGMENT_SIZE, _KN__Private_Segment_Size);
    ErrorCheck(Extracting the private segment from the executable, err);


    return STATUS_SUCCESS;

}                    

atmi_task_t *snk_cpu_kernel(const atmi_lparm_t *lparm, 
                 const cpu_kernel_table_t *_CN__CPU_kernels,
                 const char *kernel_name,
                 const uint32_t _KN__cpu_task_num_args,
                 const snk_kernel_args_t *kernel_args) {
    atmi_stream_t *stream = NULL;
    struct timespec dispatch_time;
    clock_gettime(CLOCK_MONOTONIC_RAW,&dispatch_time);
    if(lparm->stream == NULL) {
        stream = &snk_default_stream_obj;
    } else {
        stream = lparm->stream;
    }
    /* Add row to stream table for purposes of future synchronizations */
    register_stream(stream);

    /* get this stream's HSA soft queue (could be dynamically mapped or round robin
     * if it is an unordered stream */
    hsa_queue_t* this_Q = acquire_and_set_next_cpu_queue(stream);
    if(!this_Q) return NULL;

    /* if stream is ordered and the devtype changed for this task, 
     * enqueue a barrier to wait for previous device to complete */
    check_change_in_device_type(stream, this_Q, ATMI_DEVTYPE_CPU);

    /* For dependent child tasks, wait till all parent kernels are finished.  */
    if ( lparm->num_required > 0) {
        enqueue_barrier_cpu(this_Q, lparm->num_required, lparm->requires, SNK_NOWAIT);
    }

    /* Iterate over function table and retrieve the ID for kernel_name */
    uint16_t i;
    /* Store/save CPU kernel table that is generated in the SNACK wrapper */
    set_cpu_kernel_table(_CN__CPU_kernels);
    atmi_task_t *ret = NULL;
    for(i = 0; i < SNK_MAX_CPU_FUNCTIONS; i++) {
        //DEBUG_PRINT("Comparing kernels %s %s\n", _CN__CPU_kernels[i].name, kernel_name);
        if(_CN__CPU_kernels[i].name && kernel_name) {
            if(strcmp(_CN__CPU_kernels[i].name, kernel_name) == 0) {
                /* FIXME: REUSE SIGNALS!! */
                if ( SNK_NextTaskId == SNK_MAX_TASKS ) {
                    printf("ERROR:  Too many parent tasks, increase SNK_MAX_TASKS =%d\n",SNK_MAX_TASKS);
                    return ;
                }
                ret = (atmi_task_t*) &(SNK_Tasks[SNK_NextTaskId]);
                
                /* Get profiling object. Can be NULL? */
                ret->profile = lparm->profile;
                /* ID i is the kernel. Enqueue the function to the soft queue */
                //DEBUG_PRINT("CPU Function [%d]: %s has %" PRIu32 " args\n", i, kernel_name, _KN__cpu_task_num_args);
                /*  Obtain the current queue write index. increases with each call to kernel  */
                uint64_t index = hsa_queue_load_write_index_relaxed(this_Q);

                DEBUG_PRINT("Enqueueing Queue Idx: %" PRIu64 "\n", index);

                /* Find the queue index address to write the packet info into.  */
                const uint32_t queueMask = this_Q->size - 1;
                hsa_agent_dispatch_packet_t* this_aql = &(((hsa_agent_dispatch_packet_t*)(this_Q->base_address))[index&queueMask]);
                memset(this_aql, 0, sizeof(hsa_agent_dispatch_packet_t));
                /*  FIXME: We need to check for queue overflow here. Do we need
                 *  to do this for CPU agents too? */

                /* Set the type and return args.*/
                this_aql->type = (uint16_t)i;
                /* FIXME: We are considering only void return types for now.*/
                //this_aql->return_address = NULL;
                /* Set function args */
                this_aql->arg[0] = _KN__cpu_task_num_args;
                this_aql->arg[1] = (uint64_t) kernel_args;
                this_aql->arg[2] = (uint64_t) ret; // pass task handle to fill in metrics
                this_aql->arg[3] = UINT64_MAX;

                this_aql->completion_signal = *((hsa_signal_t *)(ret->handle));

                /*  Prepare and set the packet header */
                /* FIXME: CPU tasks ignore barrier bit as of now. Change
                 * implementation? I think it doesn't matter because we are
                 * executing the subroutines one-by-one, so barrier bit is
                 * inconsequential.
                 */
                if(stream->ordered == ATMI_TRUE)
                    this_aql->header = create_header(HSA_PACKET_TYPE_AGENT_DISPATCH, lparm->synchronous);
                else
                    /* If stream is unordered, sync ONLY this packet */
                    this_aql->header = create_header(HSA_PACKET_TYPE_AGENT_DISPATCH, ATMI_FALSE);

                /* Increment write index and ring doorbell to dispatch the kernel.  */
                hsa_queue_store_write_index_relaxed(this_Q, index+1);
                hsa_signal_store_relaxed(this_Q->doorbell_signal, index);
                /* Store dispatched time */
                if(ret->profile) ret->profile->dispatch_time = get_nanosecs(context_init_time, dispatch_time);
                ret->state = ATMI_DISPATCHED;
                signal_worker(this_Q, PROCESS_PKT);
                if ( lparm->synchronous == ATMI_TRUE ) { /*  Sychronous execution */
                    /* For default synchrnous execution, wait til kernel is finished.  */
                    hsa_signal_value_t value = hsa_signal_wait_acquire(this_aql->completion_signal, HSA_SIGNAL_CONDITION_EQ, 0, UINT64_MAX, HSA_WAIT_STATE_BLOCKED);
                    ret->state = ATMI_COMPLETED;
                    if(lparm->num_required == 0) { 
                        // Greg's logic is to flush the entire stream if the task is sync and has no dependencies */
                        // FIXME: This has to change for unordered streams. Why should we flush the entire stream 
                        // for every sync kernel in an unordered stream?
                        snk_stream_sync(stream);
                    }
                }
                else {
                    /* add task to the corresponding row in the stream table */
                    register_task(stream, ret, ATMI_DEVTYPE_CPU);
                }

                //SNK_NextTaskId = (SNK_NextTaskId + 1) % SNK_MAX_TASKS;
                SNK_NextTaskId++;
            }
        }
    }
    return ret;
}

atmi_task_t *snk_gpu_kernel(const atmi_lparm_t *lparm, 
                 uint64_t                         _KN__Kernel_Object,
                 uint32_t                         _KN__Group_Segment_Size,
                 uint32_t                         _KN__Private_Segment_Size,
                 void *thisKernargAddress) {
    atmi_stream_t *stream = NULL;
    if(lparm->stream == NULL) {
        stream = &snk_default_stream_obj;
    } else {
        stream = lparm->stream;
    }
    /* Add row to stream table for purposes of future synchronizations */
    register_stream(stream);
    
    /* get this stream's HSA queue (could be dynamically mapped or round robin
     * if it is an unordered stream */
    hsa_queue_t* this_Q = acquire_and_set_next_gpu_queue(stream);
    if(!this_Q) return NULL;

    /* if stream is ordered and the devtype changed for this task, 
     * enqueue a barrier to wait for previous device to complete */
    check_change_in_device_type(stream, this_Q, ATMI_DEVTYPE_GPU);

    /* For dependent child tasks, add dependent parent kernels to barriers.  */
    if ( lparm->num_required > 0) {
        enqueue_barrier_gpu(this_Q, lparm->num_required, lparm->requires, SNK_NOWAIT);
    }

    /* FIXME: REUSE SIGNALS!! */
    if ( SNK_NextTaskId == SNK_MAX_TASKS ) {
       printf("ERROR:  Too many parent tasks, increase SNK_MAX_TASKS =%d\n",SNK_MAX_TASKS);
       return ;
    }
    atmi_task_t *ret = (atmi_task_t*) &(SNK_Tasks[SNK_NextTaskId]);

    /* Get profiling object. Can be NULL? */
    ret->profile = lparm->profile;

    /*  Obtain the current queue write index. increases with each call to kernel  */
    uint64_t index = hsa_queue_load_write_index_relaxed(this_Q);

    /* Find the queue index address to write the packet info into.  */
    const uint32_t queueMask = this_Q->size - 1;
    hsa_kernel_dispatch_packet_t* this_aql = &(((hsa_kernel_dispatch_packet_t*)(this_Q->base_address))[index&queueMask]);

    /*  FIXME: We need to check for queue overflow here. */

    this_aql->completion_signal = *((hsa_signal_t*)(ret->handle));

    /*  Process lparm values */
    /*  this_aql.dimensions=(uint16_t) lparm->ndim; */
    this_aql->setup  |= (uint16_t) lparm->ndim << HSA_KERNEL_DISPATCH_PACKET_SETUP_DIMENSIONS;
    this_aql->grid_size_x=lparm->gdims[0];
    this_aql->workgroup_size_x=lparm->ldims[0];
    if (lparm->ndim>1) {
       this_aql->grid_size_y=lparm->gdims[1];
       this_aql->workgroup_size_y=lparm->ldims[1];
    } else {
       this_aql->grid_size_y=1;
       this_aql->workgroup_size_y=1;
    }
    if (lparm->ndim>2) {
       this_aql->grid_size_z=lparm->gdims[2];
       this_aql->workgroup_size_z=lparm->ldims[2];
    } else {
       this_aql->grid_size_z=1;
       this_aql->workgroup_size_z=1;
    }

	/* thisKernargAddress has already been set up in the beginning of this routine */
    /*  Bind kernel argument buffer to the aql packet.  */
    this_aql->kernarg_address = (void*) thisKernargAddress;
    this_aql->kernel_object = _KN__Kernel_Object;
    this_aql->private_segment_size = _KN__Private_Segment_Size;
    this_aql->group_segment_size = _KN__Group_Segment_Size;

    /*  Prepare and set the packet header */ 
    if(stream->ordered == ATMI_TRUE)
        this_aql->header = create_header(HSA_PACKET_TYPE_KERNEL_DISPATCH, lparm->synchronous);
    else
        /* If stream is unordered, sync ONLY this packet */
        this_aql->header = create_header(HSA_PACKET_TYPE_KERNEL_DISPATCH, ATMI_FALSE);

    /* Increment write index and ring doorbell to dispatch the kernel.  */
    hsa_queue_store_write_index_relaxed(this_Q, index+1);
    hsa_signal_store_relaxed(this_Q->doorbell_signal, index);
    ret->state = ATMI_DISPATCHED;

    if ( lparm->synchronous == ATMI_TRUE ) { /*  Sychronous execution */
       /* For default synchrnous execution, wait til kernel is finished.  */
       hsa_signal_value_t value = hsa_signal_wait_acquire(this_aql->completion_signal, HSA_SIGNAL_CONDITION_EQ, 0, UINT64_MAX, HSA_WAIT_STATE_BLOCKED);
       ret->state = ATMI_COMPLETED;
       if(lparm->num_required == 0) { 
           // Greg's logic is to flush the entire stream if the task is sync and has no dependencies */
           // FIXME: This has to change for unordered streams. Why should we flush the entire stream 
           // for every sync kernel in an unordered stream?
           snk_stream_sync(stream);
       }
    }
    else {
       /* add task to the corresponding row in the stream table */
       register_task(stream, ret, ATMI_DEVTYPE_GPU);
    }
    //SNK_NextTaskId = (SNK_NextTaskId + 1) % SNK_MAX_TASKS;
    SNK_NextTaskId++;

    return ret;
}




