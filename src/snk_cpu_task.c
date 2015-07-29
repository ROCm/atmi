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
/* This file contains logic for CPU tasking in SNACK */
#include "snk_internal.h"
#include "bindthread.h"
#include "profiling.h"

#include <assert.h>
agent_t agent[SNK_MAX_CPU_QUEUES];

hsa_signal_t worker_sig[SNK_MAX_CPU_QUEUES];

pthread_t agent_threads[SNK_MAX_CPU_QUEUES];

snk_pif_kernel_table_t snk_kernels[SNK_MAX_FUNCTIONS];

size_t numWorkers;

extern struct timespec context_init_time;

hsa_queue_t* get_cpu_queue(int id) {
    return agent[id].queue;
}

agent_t get_cpu_q_agent(int id) {
    return agent[id];
}

void signal_worker(hsa_queue_t *queue, int signal) {
    DEBUG_PRINT("Signaling work %d\n", signal);
    int id;
    for(id = 0; id < SNK_MAX_CPU_QUEUES; id++) {
        if(agent[id].queue == queue) break;
    }
    hsa_signal_store_release(worker_sig[id], signal);
}

void signal_worker_id(int id, int signal) {
    DEBUG_PRINT("Signaling work %d\n", signal);
    hsa_signal_store_release(worker_sig[id], signal);
}

int is_barrier(uint16_t header) {
    return (header & (1 << HSA_PACKET_HEADER_BARRIER)) ? 1 : 0;
}

uint8_t get_packet_type(uint16_t header) {
    // FIXME: The width of packet type is 8 bits. Change to below line if width
    // changes
    //return (header >> HSA_PACKET_HEADER_TYPE) & ((1 << HSA_PACKET_HEADER_WIDTH_TYPE) - 1);
    return (header >> HSA_PACKET_HEADER_TYPE) & 0xFF;
}

int process_packet(hsa_queue_t *queue, int id)
{
    DEBUG_PRINT("Processing Packet from CPU Queue\n");

    uint64_t start_time_ns; 
    uint64_t end_time_ns; 
    uint64_t read_index = hsa_queue_load_read_index_acquire(queue);
    assert(read_index == 0);
    hsa_signal_t doorbell = queue->doorbell_signal;
    /* FIXME: Handle queue overflows */
    while (read_index < queue->size) {
        DEBUG_PRINT("Read Index: %" PRIu64 " Queue Size: %" PRIu32 "\n", read_index, queue->size);
        hsa_signal_value_t doorbell_value = SNK_MAX_TASKS;
        while ( (doorbell_value = hsa_signal_wait_acquire(doorbell, HSA_SIGNAL_CONDITION_GTE, read_index, UINT64_MAX,
                    HSA_WAIT_STATE_BLOCKED)) < (hsa_signal_value_t) read_index );
        //fprintf(stderr, "doorbell: %ld read_index: %ld\n", doorbell_value, read_index);
        if (doorbell_value == SNK_MAX_TASKS) break;
        atmi_task_t *this_task = NULL; // will be assigned to collect metrics
        char *kernel_name = NULL;
#if defined (ATMI_HAVE_PROFILE)
        struct timespec start_time, end_time;
        clock_gettime(CLOCK_MONOTONIC_RAW,&start_time);
        start_time_ns = get_nanosecs(context_init_time, start_time);
#endif /* ATMI_HAVE_PROFILE */
        hsa_agent_dispatch_packet_t* packets = (hsa_agent_dispatch_packet_t*) queue->base_address;
        hsa_agent_dispatch_packet_t* packet = packets + read_index % queue->size;
        int i;
        DEBUG_PRINT("Processing CPU task with header: %d\n", get_packet_type(packet->header));
        //fprintf(stderr, "Processing CPU task with header: %d\n", get_packet_type(packet->header));
        switch (get_packet_type(packet->header)) {
            case HSA_PACKET_TYPE_BARRIER_OR: 
                ;
                hsa_barrier_or_packet_t *barrier_or = (hsa_barrier_or_packet_t *)packet; 
                DEBUG_PRINT("Executing OR barrier\n");
                for (i = 0; i < 5; ++i) {
                    if (barrier_or->dep_signal[i].handle != 0) {
                        hsa_signal_wait_acquire(barrier_or->dep_signal[i], 
                                HSA_SIGNAL_CONDITION_EQ,
                                0, UINT64_MAX,
                                HSA_WAIT_STATE_BLOCKED);
                        break;
                    }
                }
                packet_store_release((uint32_t*) barrier_or, create_header(HSA_PACKET_TYPE_INVALID, 0), HSA_PACKET_TYPE_BARRIER_OR);
                break;
            case HSA_PACKET_TYPE_BARRIER_AND: 
                ;
                hsa_barrier_and_packet_t *barrier = (hsa_barrier_and_packet_t *)packet; 
                DEBUG_PRINT("Executing AND barrier\n");
                for (i = 0; i < 5; ++i) {
                    if (barrier->dep_signal[i].handle != 0) {
                        DEBUG_PRINT("Waiting for signal handle: %" PRIu64 "\n", barrier->dep_signal[i].handle);
                        hsa_signal_wait_acquire(barrier->dep_signal[i], 
                                HSA_SIGNAL_CONDITION_EQ,
                                0, UINT64_MAX,
                                HSA_WAIT_STATE_BLOCKED);
                    }
                }
                packet_store_release((uint32_t*) barrier, create_header(HSA_PACKET_TYPE_INVALID, 0), HSA_PACKET_TYPE_BARRIER_AND);
                break;
            case HSA_PACKET_TYPE_AGENT_DISPATCH: 
                ;
                kernel_name = (char *)snk_kernels[packet->type].cpu_kernel.kernel_name;
                uint64_t num_params = packet->arg[0];
                // typecast to char * to be able to do ptr arithmetic
                char *kernel_args_ptr = (char *)(packet->arg[1]);
                if(num_params > 0) assert(kernel_args_ptr != NULL);
                this_task = (atmi_task_t *)(packet->arg[2]);
                assert(this_task != NULL);
                DEBUG_PRINT("Invoking function %s with %" PRIu64 " args, thisTask: %p\n", kernel_name, num_params, this_task);
                char **kernel_args = (char **)malloc(sizeof(char *) * num_params);
                int kernarg_id = 0;
                size_t kernarg_offset = 0;
                //fprintf(stderr, "kernel_args_ptr: %p\n", kernel_args_ptr);
                // unpack kernel_args to num_params * (void *) args and invoke function
                for(kernarg_id = 0; kernarg_id < num_params; kernarg_id++) {
                    size_t kernarg_size = *(size_t *)(kernel_args_ptr + kernarg_offset);
                    kernarg_offset += sizeof(size_t); 
                    kernel_args[kernarg_id] = *(char **)(kernel_args_ptr + kernarg_offset);
                    kernarg_offset += kernarg_size; 
                }
                // pass task handle to kernel_args first param
                kernel_args[0] = (char *)&this_task;
                switch(num_params) {
                    case 0: 
                        ;
                        void (*function0) (void) =
                            (void (*)(void)) snk_kernels[packet->type].cpu_kernel.function;
                        DEBUG_PRINT("Func Ptr: %p Args: NONE\n", 
                                function0
                                );
                        function0(
                                );
                        break;
                    case 1: 
                        ;
                        void (*function1) (ARG_TYPE) =
                            (void (*)(ARG_TYPE)) snk_kernels[packet->type].cpu_kernel.function;
                        DEBUG_PRINT("Args: %p\n", 
                                kernel_args[0]
                                );
                        function1(
                                kernel_args[0]
                                );
                        break;
                    case 2: 
                        ;
                        void (*function2) (ARG_TYPE, ARG_TYPE) =
                            (void (*)(ARG_TYPE, ARG_TYPE)) snk_kernels[packet->type].cpu_kernel.function;
                        DEBUG_PRINT("Args: %p %p\n", 
                                kernel_args[0],
                                kernel_args[1]
                                );
                        function2(
                                kernel_args[0],
                                kernel_args[1]
                                );
                        break;
                    case 3: 
                        ;
                        void (*function3) (ARG_TYPE REPEAT2(ARG_TYPE)) =
                            (void (*)(ARG_TYPE REPEAT2(ARG_TYPE))) snk_kernels[packet->type].cpu_kernel.function;
                        function3(
                                kernel_args[0],
                                kernel_args[1],
                                kernel_args[2]
                                );
                        break;
                    case 4: 
                        ;
                        void (*function4) (ARG_TYPE REPEAT2(ARG_TYPE) REPEAT(ARG_TYPE)) =
                            (void (*)(ARG_TYPE REPEAT2(ARG_TYPE) REPEAT(ARG_TYPE))) snk_kernels[packet->type].cpu_kernel.function;
                        function4(
                                kernel_args[0],
                                kernel_args[1],
                                kernel_args[2],
                                kernel_args[3]
                                );
                        break;
                    case 5: 
                        ;
                        void (*function5) (ARG_TYPE REPEAT4(ARG_TYPE)) = 
                            (void (*)(ARG_TYPE REPEAT4(ARG_TYPE))) snk_kernels[packet->type].cpu_kernel.function;
                        function5(
                                kernel_args[0],
                                kernel_args[1],
                                kernel_args[2],
                                kernel_args[3],
                                kernel_args[4]
                                );
                        break;
                    case 6: 
                        ;
                        void (*function6) (ARG_TYPE REPEAT4(ARG_TYPE) REPEAT(ARG_TYPE)) = 
                            (void (*)(ARG_TYPE REPEAT4(ARG_TYPE) REPEAT(ARG_TYPE))) snk_kernels[packet->type].cpu_kernel.function;
                        function6(
                                kernel_args[0]
                                ,kernel_args[1]
                                ,kernel_args[2]
                                ,kernel_args[3]
                                ,kernel_args[4]
                                ,kernel_args[5]
                                );
                        break;
                    case 7: 
                        ;
                        void (*function7) (ARG_TYPE REPEAT4(ARG_TYPE) REPEAT2(ARG_TYPE)) = 
                            (void (*)(ARG_TYPE REPEAT4(ARG_TYPE) REPEAT2(ARG_TYPE))) snk_kernels[packet->type].cpu_kernel.function;
                        function7(
                                kernel_args[0]
                                ,kernel_args[1]
                                ,kernel_args[2]
                                ,kernel_args[3]
                                ,kernel_args[4]
                                ,kernel_args[5]
                                ,kernel_args[6]
                                );
                        break;
                    case 8: 
                        ;
                        void (*function8) (ARG_TYPE REPEAT4(ARG_TYPE) REPEAT2(ARG_TYPE) REPEAT(ARG_TYPE)) = 
                            (void (*)(ARG_TYPE REPEAT4(ARG_TYPE) REPEAT2(ARG_TYPE) REPEAT(ARG_TYPE))) snk_kernels[packet->type].cpu_kernel.function;
                        function8(
                                kernel_args[0]
                                ,kernel_args[1]
                                ,kernel_args[2]
                                ,kernel_args[3]
                                ,kernel_args[4]
                                ,kernel_args[5]
                                ,kernel_args[6]
                                ,kernel_args[7]
                                );
                        break;
                    case 9: 
                        ;
                        void (*function9) (ARG_TYPE REPEAT8(ARG_TYPE)) = 
                            (void (*)(ARG_TYPE REPEAT8(ARG_TYPE))) snk_kernels[packet->type].cpu_kernel.function;
                        function9(
                                kernel_args[0]
                                ,kernel_args[1]
                                ,kernel_args[2]
                                ,kernel_args[3]
                                ,kernel_args[4]
                                ,kernel_args[5]
                                ,kernel_args[6]
                                ,kernel_args[7]
                                ,kernel_args[8]
                                );
                        break;
                    case 10: 
                        ;
                        void (*function10) (ARG_TYPE REPEAT8(ARG_TYPE) REPEAT(ARG_TYPE)) = 
                            (void (*)(ARG_TYPE REPEAT8(ARG_TYPE) REPEAT(ARG_TYPE))) snk_kernels[packet->type].cpu_kernel.function;
                        function10(
                                kernel_args[0]
                                ,kernel_args[1]
                                ,kernel_args[2]
                                ,kernel_args[3]
                                ,kernel_args[4]
                                ,kernel_args[5]
                                ,kernel_args[6]
                                ,kernel_args[7]
                                ,kernel_args[8]
                                ,kernel_args[9]
                                );
                        break;
                    case 11: 
                        ;
                        void (*function11) (ARG_TYPE REPEAT8(ARG_TYPE) REPEAT2(ARG_TYPE)) = 
                            (void (*)(ARG_TYPE REPEAT8(ARG_TYPE) REPEAT2(ARG_TYPE))) snk_kernels[packet->type].cpu_kernel.function;
                        function11(
                                kernel_args[0]
                                ,kernel_args[1]
                                ,kernel_args[2]
                                ,kernel_args[3]
                                ,kernel_args[4]
                                ,kernel_args[5]
                                ,kernel_args[6]
                                ,kernel_args[7]
                                ,kernel_args[8]
                                ,kernel_args[9]
                                ,kernel_args[10]
                                );
                        break;
                    case 12: 
                        ;
                        void (*function12) (ARG_TYPE REPEAT8(ARG_TYPE) REPEAT2(ARG_TYPE) REPEAT(ARG_TYPE)) = 
                            (void (*)(ARG_TYPE REPEAT8(ARG_TYPE) REPEAT2(ARG_TYPE) REPEAT(ARG_TYPE))) snk_kernels[packet->type].cpu_kernel.function;
                        function12(
                                kernel_args[0]
                                ,kernel_args[1]
                                ,kernel_args[2]
                                ,kernel_args[3]
                                ,kernel_args[4]
                                ,kernel_args[5]
                                ,kernel_args[6]
                                ,kernel_args[7]
                                ,kernel_args[8]
                                ,kernel_args[9]
                                ,kernel_args[10]
                                ,kernel_args[11]
                                );
                        break;
                    case 13: 
                        ;
                        void (*function13) (ARG_TYPE REPEAT8(ARG_TYPE) REPEAT4(ARG_TYPE)) = 
                            (void (*)(ARG_TYPE REPEAT8(ARG_TYPE) REPEAT4(ARG_TYPE))) snk_kernels[packet->type].cpu_kernel.function;
                        function13(
                                kernel_args[0]
                                ,kernel_args[1]
                                ,kernel_args[2]
                                ,kernel_args[3]
                                ,kernel_args[4]
                                ,kernel_args[5]
                                ,kernel_args[6]
                                ,kernel_args[7]
                                ,kernel_args[8]
                                ,kernel_args[9]
                                ,kernel_args[10]
                                ,kernel_args[11]
                                ,kernel_args[12]
                                );
                        break;
                    case 14: 
                        ;
                        void (*function14) (ARG_TYPE REPEAT8(ARG_TYPE) REPEAT4(ARG_TYPE) REPEAT(ARG_TYPE)) = 
                            (void (*)(ARG_TYPE REPEAT8(ARG_TYPE) REPEAT4(ARG_TYPE) REPEAT(ARG_TYPE))) snk_kernels[packet->type].cpu_kernel.function;
                        function14(
                                kernel_args[0]
                                ,kernel_args[1]
                                ,kernel_args[2]
                                ,kernel_args[3]
                                ,kernel_args[4]
                                ,kernel_args[5]
                                ,kernel_args[6]
                                ,kernel_args[7]
                                ,kernel_args[8]
                                ,kernel_args[9]
                                ,kernel_args[10]
                                ,kernel_args[11]
                                ,kernel_args[12]
                                ,kernel_args[13]
                                );
                        break;
                    case 15: 
                        ;
                        void (*function15) (ARG_TYPE REPEAT8(ARG_TYPE) REPEAT4(ARG_TYPE) REPEAT2(ARG_TYPE)) = 
                            (void (*)(ARG_TYPE REPEAT8(ARG_TYPE) REPEAT4(ARG_TYPE) REPEAT2(ARG_TYPE))) snk_kernels[packet->type].cpu_kernel.function;
                        function15(
                                kernel_args[0]
                                ,kernel_args[1]
                                ,kernel_args[2]
                                ,kernel_args[3]
                                ,kernel_args[4]
                                ,kernel_args[5]
                                ,kernel_args[6]
                                ,kernel_args[7]
                                ,kernel_args[8]
                                ,kernel_args[9]
                                ,kernel_args[10]
                                ,kernel_args[11]
                                ,kernel_args[12]
                                ,kernel_args[13]
                                ,kernel_args[14]
                                );
                        break;
                    case 16: 
                        ;
                        void (*function16) (ARG_TYPE REPEAT8(ARG_TYPE) REPEAT4(ARG_TYPE) REPEAT2(ARG_TYPE) REPEAT(ARG_TYPE)) = 
                            (void (*)(ARG_TYPE REPEAT8(ARG_TYPE) REPEAT4(ARG_TYPE) REPEAT2(ARG_TYPE) REPEAT(ARG_TYPE))) snk_kernels[packet->type].cpu_kernel.function;
                        function16(
                                kernel_args[0]
                                ,kernel_args[1]
                                ,kernel_args[2]
                                ,kernel_args[3]
                                ,kernel_args[4]
                                ,kernel_args[5]
                                ,kernel_args[6]
                                ,kernel_args[7]
                                ,kernel_args[8]
                                ,kernel_args[9]
                                ,kernel_args[10]
                                ,kernel_args[11]
                                ,kernel_args[12]
                                ,kernel_args[13]
                                ,kernel_args[14]
                                ,kernel_args[15]
                                );
                        break;
                    case 17: 
                        ;
                        void (*function17) (ARG_TYPE REPEAT16(ARG_TYPE)) = 
                            (void (*)(ARG_TYPE REPEAT16(ARG_TYPE))) snk_kernels[packet->type].cpu_kernel.function;
                        function17(
                                kernel_args[0]
                                ,kernel_args[1]
                                ,kernel_args[2]
                                ,kernel_args[3]
                                ,kernel_args[4]
                                ,kernel_args[5]
                                ,kernel_args[6]
                                ,kernel_args[7]
                                ,kernel_args[8]
                                ,kernel_args[9]
                                ,kernel_args[10]
                                ,kernel_args[11]
                                ,kernel_args[12]
                                ,kernel_args[13]
                                ,kernel_args[14]
                                ,kernel_args[15]
                                ,kernel_args[16]
                                );
                        break;
                    case 18: 
                        ;
                        void (*function18) (ARG_TYPE REPEAT16(ARG_TYPE) REPEAT(ARG_TYPE)) = 
                            (void (*)(ARG_TYPE REPEAT16(ARG_TYPE) REPEAT(ARG_TYPE))) snk_kernels[packet->type].cpu_kernel.function;
                        function18(
                                kernel_args[0]
                                ,kernel_args[1]
                                ,kernel_args[2]
                                ,kernel_args[3]
                                ,kernel_args[4]
                                ,kernel_args[5]
                                ,kernel_args[6]
                                ,kernel_args[7]
                                ,kernel_args[8]
                                ,kernel_args[9]
                                ,kernel_args[10]
                                ,kernel_args[11]
                                ,kernel_args[12]
                                ,kernel_args[13]
                                ,kernel_args[14]
                                ,kernel_args[15]
                                ,kernel_args[16]
                                ,kernel_args[17]
                                );
                        break;
                    case 19: 
                        ;
                        void (*function19) (ARG_TYPE REPEAT16(ARG_TYPE) REPEAT2(ARG_TYPE)) = 
                            (void (*)(ARG_TYPE REPEAT16(ARG_TYPE) REPEAT2(ARG_TYPE))) snk_kernels[packet->type].cpu_kernel.function;
                        function19(
                                kernel_args[0]
                                ,kernel_args[1]
                                ,kernel_args[2]
                                ,kernel_args[3]
                                ,kernel_args[4]
                                ,kernel_args[5]
                                ,kernel_args[6]
                                ,kernel_args[7]
                                ,kernel_args[8]
                                ,kernel_args[9]
                                ,kernel_args[10]
                                ,kernel_args[11]
                                ,kernel_args[12]
                                ,kernel_args[13]
                                ,kernel_args[14]
                                ,kernel_args[15]
                                ,kernel_args[16]
                                ,kernel_args[17]
                                ,kernel_args[18]
                                );
                        break;
                    case 20: 
                        ;
                        void (*function20) (ARG_TYPE REPEAT16(ARG_TYPE) REPEAT2(ARG_TYPE) REPEAT(ARG_TYPE)) = 
                            (void (*)(ARG_TYPE REPEAT16(ARG_TYPE) REPEAT2(ARG_TYPE) REPEAT(ARG_TYPE))) snk_kernels[packet->type].cpu_kernel.function;
                        function20(
                                kernel_args[0]
                                ,kernel_args[1]
                                ,kernel_args[2]
                                ,kernel_args[3]
                                ,kernel_args[4]
                                ,kernel_args[5]
                                ,kernel_args[6]
                                ,kernel_args[7]
                                ,kernel_args[8]
                                ,kernel_args[9]
                                ,kernel_args[10]
                                ,kernel_args[11]
                                ,kernel_args[12]
                                ,kernel_args[13]
                                ,kernel_args[14]
                                ,kernel_args[15]
                                ,kernel_args[16]
                                ,kernel_args[17]
                                ,kernel_args[18]
                                ,kernel_args[19]
                                );
                        break;
                    default: 

                        DEBUG_PRINT("Too many function arguments: %"  PRIu64 "\n", num_params);
                             check(Too many function arguments, HSA_STATUS_ERROR_INCOMPATIBLE_ARGUMENTS);
                             break;
                }
                DEBUG_PRINT("Signaling from CPU task: %" PRIu64 "\n", packet->completion_signal.handle);
                //fprintf(stderr, "Signaling from CPU task: %" PRIu64 "\n", packet->completion_signal.handle);
                packet_store_release((uint32_t*) packet, create_header(HSA_PACKET_TYPE_INVALID, 0), packet->type);
                for(kernarg_id = 0; kernarg_id < num_params; kernarg_id++) {
                    if(kernarg_id == 0) continue; // task handle should be managed elsewhere in the runtime
                    free(kernel_args[kernarg_id]);
                }
                free(kernel_args);
                //free(kernel_args_ptr);
                break;
        }
        if (packet->completion_signal.handle != 0) {
            hsa_signal_subtract_release(packet->completion_signal, 1);
        }
#if defined (ATMI_HAVE_PROFILE)
        clock_gettime(CLOCK_MONOTONIC_RAW,&end_time);
        end_time_ns = get_nanosecs(context_init_time, end_time);
        if(this_task != NULL) {
            //if(this_task->profile != NULL) 
            if (kernel_name != NULL)
            {
                this_task->profile.end_time = end_time_ns;
                this_task->profile.start_time = start_time_ns;
                this_task->profile.ready_time = start_time_ns;
                DEBUG_PRINT("Task %p timing info (%" PRIu64", %" PRIu64")\n", 
                        this_task, start_time_ns, end_time_ns);
                atmi_profiling_record(id, &(this_task->profile), kernel_name);
            }
        }
#endif /* ATMI_HAVE_PROFILE */
        read_index++;
        hsa_queue_store_read_index_release(queue, read_index);
    }

    DEBUG_PRINT("Finished executing agent dispatch\n");

    // Finishing this task may free up more tasks, so issue the wakeup command
    //DEBUG_PRINT("Signaling more work\n");
    //hsa_signal_store_release(worker_sig[id], PROCESS_PKT);
    return 0;
}
#if 0
typedef struct thread_args_s {
    int tid;
    size_t num_queues;
    size_t capacity;
    hsa_agent_t cpu_agent;
    hsa_region_t cpu_region;
} thread_args_t;

void *agent_worker(void *agent_args) {
    /* TODO: Investigate more if we really need the inter-thread worker signal. 
     * Can we just do the below without hanging? */
    thread_args_t *args = (thread_args_t *) agent_args; 
    int tid = args->tid;
    agent[tid].num_queues = args->num_queues;
    agent[tid].id = tid;

    hsa_signal_t db_signal;
    hsa_status_t err;
    err = hsa_signal_create(1, 0, NULL, &db_signal);
    check(Creating a HSA signal for agent dispatch db signal, err);

    hsa_queue_t *queue = NULL;
    err = hsa_soft_queue_create(args->cpu_region, args->capacity, HSA_QUEUE_TYPE_SINGLE,
            HSA_QUEUE_FEATURE_AGENT_DISPATCH, db_signal, &queue);
    check(Creating an agent queue, err);

    /* FIXME: Looks like a nasty HSA bug. The doorbell signal that we pass to the 
     * soft queue creation API never seems to be set. Workaround is to 
     * manually set it again like below.
     */
    queue->doorbell_signal = db_signal;

    process_packet(queue, tid);
}
#else
void *agent_worker(void *agent_args) {
    agent_t *agent = (agent_t *) agent_args;

    atmi_cpu_bindthread(agent->id); 
#if defined (ATMI_HAVE_PROFILE)
    atmi_profiling_agent_init(agent->id);
#endif /* ATMI_HAVE_PROFILE */ 

    hsa_signal_value_t sig_value = IDLE;
    while (sig_value == IDLE) {
        DEBUG_PRINT("Worker thread sleeping\n");
        sig_value = hsa_signal_wait_acquire(worker_sig[agent->id], HSA_SIGNAL_CONDITION_LT, IDLE,
                UINT64_MAX, HSA_WAIT_STATE_BLOCKED);
        DEBUG_PRINT("Worker thread waking up\n");

        if (sig_value == FINISH) {
            DEBUG_PRINT("Worker thread received the EXIT SIGNAL\n");
            break;
        }

        if (PROCESS_PKT == hsa_signal_cas_acq_rel(worker_sig[agent->id],
                    PROCESS_PKT, IDLE) ) {
            hsa_queue_t *queue = agent->queue;
            if (!process_packet(queue, agent->id)) continue;
        }
        sig_value = IDLE;
    }

#if defined (ATMI_HAVE_PROFILE)
    atmi_profiling_output(agent->id);
    atmi_profiling_agent_fini(agent->id);
#endif /*ATMI_HAVE_PROFILE */
    return NULL;
}
#endif
void
cpu_agent_init(hsa_agent_t cpu_agent, hsa_region_t cpu_region, 
                const size_t num_queues, const size_t capacity
                ) {
    hsa_status_t err;
    uint32_t i;
    #if 1
    for (i = 0; i < num_queues; i++) {
        agent[i].num_queues = num_queues;
        agent[i].id = i;
        // signal between the host thread and the CPU tasking queue thread
        err = hsa_signal_create(IDLE, 0, NULL, &worker_sig[i]);
        check(Creating a HSA signal for agent dispatch worker threads, err);

        hsa_signal_t db_signal;
        err = hsa_signal_create(1, 0, NULL, &db_signal);
        check(Creating a HSA signal for agent dispatch db signal, err);

        err = hsa_soft_queue_create(cpu_region, capacity, HSA_QUEUE_TYPE_SINGLE,
                HSA_QUEUE_FEATURE_AGENT_DISPATCH, db_signal, &(agent[i].queue));
        check(Creating an agent queue, err);

        hsa_queue_t *q = agent[i].queue;
        //err = hsa_ext_set_profiling( q, 1); 
        //check(Enabling CPU profiling support, err); 
        //profiling does not work for CPU queues
        /* FIXME: Looks like a nasty HSA bug. The doorbell signal that we pass to the 
         * soft queue creation API never seems to be set. Workaround is to 
         * manually set it again like below.
         */
        q->doorbell_signal = db_signal;
    }
    #endif
    numWorkers = num_queues;
    DEBUG_PRINT("Spawning %zu CPU execution threads\n",
                 numWorkers);

    for (i = 0; i < numWorkers; i++) {
#if 1
        pthread_create(&agent_threads[i], NULL, agent_worker, (void *)&(agent[i]));
#else
        thread_args_t args;
        args.tid = i;
        args.num_queues = num_queues;
        args.capacity = capacity;
        args.cpu_agent = cpu_agent;
        args.cpu_region = cpu_region;
        pthread_create(&agent_threads[i], NULL, agent_worker, (void *)&args);
#endif
    }
} 

/* FIXME: When and who should call this cleanup funtion? */
void
agent_fini()
{
    DEBUG_PRINT("SIGNALING EXIT\n");

    /* wait for the other threads */
    uint32_t i;
    for (i = 0; i < numWorkers; i++) {
        hsa_signal_store_release(agent[i].queue->doorbell_signal, SNK_MAX_TASKS);
        hsa_signal_store_release(worker_sig[i], FINISH);
        pthread_join(agent_threads[i], NULL);
        hsa_queue_destroy(agent[i].queue);
    }

    DEBUG_PRINT("agent_fini completed\n");
}

hsa_signal_t *get_worker_sig(hsa_queue_t *queue) {
    DEBUG_PRINT("Signaling work %d\n", signal);
    int id;
    for(id = 0; id < SNK_MAX_CPU_QUEUES; id++) {
        if(agent[id].queue == queue) break;

    }
    return &(worker_sig[id]);

}
