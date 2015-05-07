#!/bin/bash
#
#  snk_genw.sh: Part of snack that generates the user callable wrapper functions.
#
#  Written by Greg Rodgers  Gregory.Rodgers@amd.com
#  Maintained by Shreyas Ramalingam Shreyas.Ramalingam@amd.com
#
# Copyright (c) 2015 ADVANCED MICRO DEVICES, INC.  
# 
# AMD is granting you permission to use this software and documentation (if any) (collectively, the 
# Materials) pursuant to the terms and conditions of the Software License Agreement included with the 
# Materials.  If you do not have a copy of the Software License Agreement, contact your AMD 
# representative for a copy.
# 
# You agree that you will not reverse engineer or decompile the Materials, in whole or in part, except for 
# example code which is provided in source code form and as allowed by applicable law.
# 
# WARRANTY DISCLAIMER: THE SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY 
# KIND.  AMD DISCLAIMS ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING BUT NOT 
# LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
# PURPOSE, TITLE, NON-INFRINGEMENT, THAT THE SOFTWARE WILL RUN UNINTERRUPTED OR ERROR-
# FREE OR WARRANTIES ARISING FROM CUSTOM OF TRADE OR COURSE OF USAGE.  THE ENTIRE RISK 
# ASSOCIATED WITH THE USE OF THE SOFTWARE IS ASSUMED BY YOU.  Some jurisdictions do not 
# allow the exclusion of implied warranties, so the above exclusion may not apply to You. 
# 
# LIMITATION OF LIABILITY AND INDEMNIFICATION:  AMD AND ITS LICENSORS WILL NOT, 
# UNDER ANY CIRCUMSTANCES BE LIABLE TO YOU FOR ANY PUNITIVE, DIRECT, INCIDENTAL, 
# INDIRECT, SPECIAL OR CONSEQUENTIAL DAMAGES ARISING FROM USE OF THE SOFTWARE OR THIS 
# AGREEMENT EVEN IF AMD AND ITS LICENSORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH 
# DAMAGES.  In no event shall AMD's total liability to You for all damages, losses, and 
# causes of action (whether in contract, tort (including negligence) or otherwise) 
# exceed the amount of $100 USD.  You agree to defend, indemnify and hold harmless 
# AMD and its licensors, and any of their directors, officers, employees, affiliates or 
# agents from and against any and all loss, damage, liability and other expenses 
# (including reasonable attorneys' fees), resulting from Your use of the Software or 
# violation of the terms and conditions of this Agreement.  
# 
# U.S. GOVERNMENT RESTRICTED RIGHTS: The Materials are provided with "RESTRICTED RIGHTS." 
# Use, duplication, or disclosure by the Government is subject to the restrictions as set 
# forth in FAR 52.227-14 and DFAR252.227-7013, et seq., or its successor.  Use of the 
# Materials by the Government constitutes acknowledgement of AMD's proprietary rights in them.
# 
# EXPORT RESTRICTIONS: The Materials may be subject to export restrictions as stated in the 
# Software License Agreement.
# 

function write_copyright_template(){
/bin/cat  <<"EOF"
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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "hsa.h"
#include "hsa_ext_finalize.h"

/*  set NOTCOHERENT needs this include
#include "hsa_ext_amd.h"
*/

typedef enum status_t status_t;
enum status_t {
    STATUS_SUCCESS=0,
    STATUS_UNKNOWN=1
};
EOF
}

function write_header_template(){
/bin/cat  <<"EOF"
#ifdef __cplusplus
#define _CPPSTRING_ "C" 
#endif
#ifndef __cplusplus
#define _CPPSTRING_ 
#endif
#ifndef __SNK_DEFS
#define SNK_MAX_STREAMS 8 
#define SNK_MAX_TASKS 100001
extern _CPPSTRING_ void stream_sync(const int stream_num);

#define SNK_ORDERED 1
#define SNK_UNORDERED 0

#include <stdint.h>
#ifndef HSA_RUNTIME_INC_HSA_H_
typedef struct hsa_signal_s { uint64_t handle; } hsa_signal_t;
#endif

typedef struct snk_task_s snk_task_t;
struct snk_task_s { 
   hsa_signal_t signal ; 
   snk_task_t* next;
};

typedef struct snk_lparm_s snk_lparm_t;
struct snk_lparm_s { 
   int ndim;                  /* default = 1 */
   size_t gdims[3];           /* NUMBER OF THREADS TO EXECUTE MUST BE SPECIFIED */ 
   size_t ldims[3];           /* Default = {64} , e.g. 1 of 8 CU on Kaveri */
   int stream;                /* default = -1 , synchrnous */
   int barrier;               /* default = SNK_UNORDERED */
   int acquire_fence_scope;   /* default = 2 */
   int release_fence_scope;   /* default = 2 */
   snk_task_t *requires ;     /* Linked list of required parent tasks, default = NULL  */
   snk_task_t *needs ;        /* Linked list of parent tasks where only one must complete, default=NULL */
} ;

/* This string macro is used to declare launch parameters set default values  */
#define SNK_INIT_LPARM(X,Y) snk_lparm_t * X ; snk_lparm_t  _ ## X ={.ndim=1,.gdims={Y},.ldims={64},.stream=-1,.barrier=SNK_UNORDERED,.acquire_fence_scope=2,.release_fence_scope=2,.requires=NULL,.needs=NULL} ; X = &_ ## X ;
 
/* Equivalent host data types for kernel data types */
typedef struct snk_image3d_s snk_image3d_t;
struct snk_image3d_s { 
   unsigned int channel_order; 
   unsigned int channel_data_type; 
   size_t width, height, depth;
   size_t row_pitch, slice_pitch;
   size_t element_size;
   void *data;
};

#define __SNK_DEFS
#endif
EOF
}
function write_global_functions_template(){
/bin/cat  <<"EOF"

void packet_store_release(uint32_t* packet, uint16_t header, uint16_t rest){
  __atomic_store_n(packet,header|(rest<<16),__ATOMIC_RELEASE);
}

uint16_t header(hsa_packet_type_t type) {
   uint16_t header = type << HSA_PACKET_HEADER_TYPE;
   header |= HSA_FENCE_SCOPE_SYSTEM << HSA_PACKET_HEADER_ACQUIRE_FENCE_SCOPE;
   header |= HSA_FENCE_SCOPE_SYSTEM << HSA_PACKET_HEADER_RELEASE_FENCE_SCOPE;
   return header;
}

void barrier_sync(int stream_num, snk_task_t *dep_task_list) {
    /* This routine will wait for all dependent packets to complete
       irrespective of their queue number. This will put a barrier packet in the
       stream belonging to the current packet. 
     */

    if(stream_num < 0 || dep_task_list == NULL) return; 

    hsa_queue_t *queue = Stream_CommandQ[stream_num];
    int dep_task_count = 0;
    snk_task_t *head = dep_task_list;
    while(head != NULL) {
        dep_task_count++;
        head = head->next;
    }

    /* Keep adding barrier packets in multiples of 5 because that is the maximum signals that 
       the HSA barrier packet can support today
     */
    snk_task_t *tasks = dep_task_list;
    hsa_signal_t signal;
    hsa_signal_create(1, 0, NULL, &signal);
    const int HSA_BARRIER_MAX_DEPENDENT_TASKS = 5;
    /* round up */
    int barrier_pkt_count = (dep_task_count + HSA_BARRIER_MAX_DEPENDENT_TASKS - 1) / HSA_BARRIER_MAX_DEPENDENT_TASKS;
    int barrier_pkt_id = 0;
    for(barrier_pkt_id = 0; barrier_pkt_id < barrier_pkt_count; barrier_pkt_id++) {
        /* Obtain the write index for the command queue for this stream.  */
        uint64_t index = hsa_queue_load_write_index_relaxed(queue);
        const uint32_t queueMask = queue->size - 1;

        /* Define the barrier packet to be at the calculated queue index address.  */
        hsa_barrier_and_packet_t* barrier = &(((hsa_barrier_and_packet_t*)(queue->base_address))[index&queueMask]);
        memset(barrier, 0, sizeof(hsa_barrier_and_packet_t));
        barrier->header |= HSA_FENCE_SCOPE_SYSTEM << HSA_PACKET_HEADER_ACQUIRE_FENCE_SCOPE;
        barrier->header |= HSA_FENCE_SCOPE_SYSTEM << HSA_PACKET_HEADER_RELEASE_FENCE_SCOPE;
        barrier->header |= 0 << HSA_PACKET_HEADER_BARRIER;
        barrier->header |= HSA_PACKET_TYPE_BARRIER_AND << HSA_PACKET_HEADER_TYPE; 

        /* populate all dep_signals */
        int dep_signal_id = 0;
        for(dep_signal_id = 0; dep_signal_id < HSA_BARRIER_MAX_DEPENDENT_TASKS; dep_signal_id++) {
            if(tasks != NULL) {
                /* fill out the barrier packet and ring doorbell */
                barrier->dep_signal[dep_signal_id] = tasks->signal; 
                tasks = tasks->next;
            }
        }
        if(tasks == NULL) { 
            /* reached the end of task list */
            barrier->header |= 1 << HSA_PACKET_HEADER_BARRIER;
            barrier->completion_signal = signal;
        }
        /* Increment write index and ring doorbell to dispatch the kernel.  */
        hsa_queue_store_write_index_relaxed(queue, index+1);
        hsa_signal_store_relaxed(queue->doorbell_signal, index);
        //printf("barrier pkt submitted: %d\n", barrier_pkt_id);
    }

    /* Wait on completion signal til kernel is finished.  */
    hsa_signal_wait_acquire(signal, HSA_SIGNAL_CONDITION_LT, 1, UINT64_MAX, HSA_WAIT_STATE_BLOCKED);

    hsa_signal_destroy(signal);
}

extern void stream_sync(int stream_num) {
    /* This is a user-callable function that puts a barrier packet into a queue where
       all former dispatch packets were put on the queue for asynchronous asynchrnous 
       executions. This routine will wait for all packets to complete on this queue.
    */

    hsa_queue_t *queue = Stream_CommandQ[stream_num];

    hsa_signal_t signal;
    hsa_signal_create(1, 0, NULL, &signal);
  
    /* Obtain the write index for the command queue for this stream.  */
    uint64_t index = hsa_queue_load_write_index_relaxed(queue);
    const uint32_t queueMask = queue->size - 1;

    /* Define the barrier packet to be at the calculated queue index address.  */
    hsa_barrier_or_packet_t* barrier = &(((hsa_barrier_or_packet_t*)(queue->base_address))[index&queueMask]);
    memset(barrier, 0, sizeof(hsa_barrier_or_packet_t));
    barrier->header |= HSA_FENCE_SCOPE_SYSTEM << HSA_PACKET_HEADER_ACQUIRE_FENCE_SCOPE;
    barrier->header |= HSA_FENCE_SCOPE_SYSTEM << HSA_PACKET_HEADER_RELEASE_FENCE_SCOPE;
    barrier->header |= 1 << HSA_PACKET_HEADER_BARRIER;
    barrier->header |= HSA_PACKET_TYPE_BARRIER_AND << HSA_PACKET_HEADER_TYPE; 
    barrier->completion_signal = signal;

    /* Increment write index and ring doorbell to dispatch the kernel.  */
    hsa_queue_store_write_index_relaxed(queue, index+1);
    hsa_signal_store_relaxed(queue->doorbell_signal, index);

    /* Wait on completion signal til kernel is finished.  */
    hsa_signal_wait_acquire(signal, HSA_SIGNAL_CONDITION_LT, 1, UINT64_MAX, HSA_WAIT_STATE_BLOCKED);

    hsa_signal_destroy(signal);

}  /* End of generated global functions */
EOF
} # end of bash function write_global_functions_template() 

function write_context_template(){
/bin/cat  <<"EOF"

#define ErrorCheck(msg, status) \
if (status != HSA_STATUS_SUCCESS) { \
    printf("%s failed.\n", #msg); \
    exit(1); \
} else { \
 /*  printf("%s succeeded.\n", #msg);*/ \
}

/* Determines if the given agent is of type HSA_DEVICE_TYPE_GPU
   and sets the value of data to the agent handle if it is.
*/
static hsa_status_t get_gpu_agent(hsa_agent_t agent, void *data) {
    hsa_status_t status;
    hsa_device_type_t device_type;
    status = hsa_agent_get_info(agent, HSA_AGENT_INFO_DEVICE, &device_type);
    if (HSA_STATUS_SUCCESS == status && HSA_DEVICE_TYPE_GPU == device_type) {
        hsa_agent_t* ret = (hsa_agent_t*)data;
        *ret = agent;
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

/* Stream specific globals */
hsa_queue_t* Stream_CommandQ[SNK_MAX_STREAMS];
snk_task_t   SNK_Tasks[SNK_MAX_TASKS];
int          SNK_NextTaskId = 0 ;

/* Context(cl file) specific globals */
hsa_ext_module_t*                _CN__BrigModule;
hsa_agent_t                      _CN__Agent;
hsa_ext_program_t                _CN__HsaProgram;
hsa_executable_t                 _CN__Executable;
hsa_region_t                     _CN__KernargRegion;
int                              _CN__FC = 0; 

/* Global variables */
hsa_queue_t*                     Sync_CommandQ;
hsa_signal_t                     Sync_Signal; 
#include "_CN__brig.h" 

status_t _CN__InitContext(){

    hsa_status_t err;

    err = hsa_init();
    ErrorCheck(Initializing the hsa runtime, err);

    /* Iterate over the agents and pick the gpu agent */
    err = hsa_iterate_agents(get_gpu_agent, &_CN__Agent);
    if(err == HSA_STATUS_INFO_BREAK) { err = HSA_STATUS_SUCCESS; }
    ErrorCheck(Getting a gpu agent, err);

    /* Query the name of the agent.  */
    char name[64] = { 0 };
    err = hsa_agent_get_info(_CN__Agent, HSA_AGENT_INFO_NAME, name);
    ErrorCheck(Querying the agent name, err);
    /* printf("The agent name is %s.\n", name); */

    /* Query the maximum size of the queue.  */
    uint32_t queue_size = 0;
    err = hsa_agent_get_info(_CN__Agent, HSA_AGENT_INFO_QUEUE_MAX_SIZE, &queue_size);
    ErrorCheck(Querying the agent maximum queue size, err);
    /* printf("The maximum queue size is %u.\n", (unsigned int) queue_size); */

    /* Load the BRIG binary.  */
    _CN__BrigModule = (hsa_ext_module_t*) &HSA_BrigMem;

    /* Create hsa program.  */
    memset(&_CN__HsaProgram,0,sizeof(hsa_ext_program_t));
    err = hsa_ext_program_create(HSA_MACHINE_MODEL_LARGE, HSA_PROFILE_FULL, HSA_DEFAULT_FLOAT_ROUNDING_MODE_DEFAULT, NULL, &_CN__HsaProgram);
    ErrorCheck(Create the program, err);

    /* Add the BRIG module to hsa program.  */
    err = hsa_ext_program_add_module(_CN__HsaProgram, _CN__BrigModule);
    ErrorCheck(Adding the brig module to the program, err);

    /* Determine the agents ISA.  */
    hsa_isa_t isa;
    err = hsa_agent_get_info(_CN__Agent, HSA_AGENT_INFO_ISA, &isa);
    ErrorCheck(Query the agents isa, err);

    /* * Finalize the program and extract the code object.  */
    hsa_ext_control_directives_t control_directives;
    memset(&control_directives, 0, sizeof(hsa_ext_control_directives_t));
    hsa_code_object_t code_object;
    err = hsa_ext_program_finalize(_CN__HsaProgram, isa, 0, control_directives, "", HSA_CODE_OBJECT_TYPE_PROGRAM, &code_object);
    ErrorCheck(Finalizing the program, err);

    /* Destroy the program, it is no longer needed.  */
    err=hsa_ext_program_destroy(_CN__HsaProgram);
    ErrorCheck(Destroying the program, err);

    /* Create the empty executable.  */
    err = hsa_executable_create(HSA_PROFILE_FULL, HSA_EXECUTABLE_STATE_UNFROZEN, "", &_CN__Executable);
    ErrorCheck(Create the executable, err);

    /* Load the code object.  */
    err = hsa_executable_load_code_object(_CN__Executable, _CN__Agent, code_object, "");
    ErrorCheck(Loading the code object, err);

    /* Freeze the executable; it can now be queried for symbols.  */
    err = hsa_executable_freeze(_CN__Executable, "");
    ErrorCheck(Freeze the executable, err);

    /* Find a memory region that supports kernel arguments.  */
    _CN__KernargRegion.handle=(uint64_t)-1;
    hsa_agent_iterate_regions(_CN__Agent, get_kernarg_memory_region, &_CN__KernargRegion);
    err = (_CN__KernargRegion.handle == (uint64_t)-1) ? HSA_STATUS_ERROR : HSA_STATUS_SUCCESS;
    ErrorCheck(Finding a kernarg memory region, err);

    /*  Create a queue using the maximum size.  */
    err = hsa_queue_create(_CN__Agent, queue_size, HSA_QUEUE_TYPE_SINGLE, NULL, NULL, UINT32_MAX, UINT32_MAX, &Sync_CommandQ);
    ErrorCheck(Creating the queue, err);

    /*  Create signal to wait for the dispatch to finish. this Signal is only used for synchronous execution  */ 
    err=hsa_signal_create(1, 0, NULL, &Sync_Signal);
    ErrorCheck(Creating a HSA signal, err);

    int task_num;
    /* Initialize all preallocated tasks and signals */
    for ( task_num = 0 ; task_num < SNK_MAX_TASKS; task_num++){
       SNK_Tasks[task_num].next = NULL;
       err=hsa_signal_create(1, 0, NULL, &(SNK_Tasks[task_num].signal));
       ErrorCheck(Creating a HSA signal, err);
    }

    /* Create queues and signals for each stream. */
    int stream_num;
    for ( stream_num = 0 ; stream_num < SNK_MAX_STREAMS ; stream_num++){
       /* printf("calling queue create for stream %d\n",stream_num); */
       err=hsa_queue_create(_CN__Agent, queue_size, HSA_QUEUE_TYPE_SINGLE, NULL, NULL, UINT32_MAX, UINT32_MAX, &Stream_CommandQ[stream_num]);
       ErrorCheck(Creating the Stream Command Q, err);
    }

    return STATUS_SUCCESS;
} /* end of __CN__InitContext */

EOF
}

function write_KernelStatics_template(){
/bin/cat <<"EOF"

/* Kernel specific globals, one set for each kernel  */
hsa_executable_symbol_t          _KN__Symbol;
int                              _KN__FK = 0 ; 
status_t                         _KN__init();
uint64_t                         _KN__Kernel_Object;
uint32_t                         _KN__Kernarg_Segment_Size; /* May not need to be global */
uint32_t                         _KN__Group_Segment_Size;
uint32_t                         _KN__Private_Segment_Size;

EOF
}

function write_InitKernel_template(){
/bin/cat <<"EOF"
extern status_t _KN__init(){

    if (_CN__FC == 0 ) {
       status_t status = _CN__InitContext();
       if ( status  != STATUS_SUCCESS ) return; 
       _CN__FC = 1;
    }
   
    hsa_status_t err;

    /* Extract the symbol from the executable.  */
    /* printf("Kernel name _KN__: Looking for symbol %s\n","__OpenCL__KN__kernel"); */
    err = hsa_executable_get_symbol(_CN__Executable, NULL, "&__OpenCL__KN__kernel", _CN__Agent , 0, &_KN__Symbol);
    ErrorCheck(Extract the symbol from the executable, err);

    /* Extract dispatch information from the symbol */
    err = hsa_executable_symbol_get_info(_KN__Symbol, HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_OBJECT, &_KN__Kernel_Object);
    ErrorCheck(Extracting the symbol from the executable, err);
    err = hsa_executable_symbol_get_info(_KN__Symbol, HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_KERNARG_SEGMENT_SIZE, &_KN__Kernarg_Segment_Size);
    ErrorCheck(Extracting the kernarg segment size from the executable, err);
    err = hsa_executable_symbol_get_info(_KN__Symbol, HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_GROUP_SEGMENT_SIZE, &_KN__Group_Segment_Size);
    ErrorCheck(Extracting the group segment size from the executable, err);
    err = hsa_executable_symbol_get_info(_KN__Symbol, HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_PRIVATE_SEGMENT_SIZE, &_KN__Private_Segment_Size);
    ErrorCheck(Extracting the private segment from the executable, err);


    return STATUS_SUCCESS;

} /* end of _KN__init */

EOF
}

function write_kernel_template(){
/bin/cat <<"EOF"

    /*  Get stream number from launch parameters.       */
    /*  This must be less than SNK_MAX_STREAMS.         */
    /*  If negative, then function call is synchrnous.  */
    int stream_num = lparm->stream;
    if ( stream_num >= SNK_MAX_STREAMS )  {
       printf(" ERROR Stream number %d specified, must be less than %d \n", stream_num, SNK_MAX_STREAMS);
       return; 
    }

    hsa_queue_t* this_Q ;
    if ( stream_num < 0 ) { /*  Sychronous execution */
       this_Q = Sync_CommandQ;
    } else { /* Asynchrnous execution uses one command Q per stream */
       this_Q = Stream_CommandQ[stream_num];
    }

    if ( lparm->requires != NULL) {
       /* For dependent child tasks, wait till all parent kernels are finished.  */
	   /* FIXME: To use multiple barrier AND packets or individual waiting for better performance? 
	      KPS benchmark showed that barrier AND was a lot slower, but will keep both implementations
		  for future use */
	   #if 1
	   #if 0
	   barrier_sync(stream_num, lparm->requires);
	   #else
	   snk_task_t *p = lparm->requires;
	   while(p != NULL) {
       	hsa_signal_value_t value = hsa_signal_wait_acquire(p->signal, HSA_SIGNAL_CONDITION_LT, 1, UINT64_MAX, HSA_WAIT_STATE_BLOCKED);
       	// HSA manual uses a while loop. Why? 
		// while(hsa_signal_wait_acquire(p->signal, HSA_SIGNAL_CONDITION_EQ, 0, UINT64_MAX, HSA_WAIT_STATE_ACTIVE) != 0);
		p = p->next;
	   }
	   #endif
	   #endif
    }
    if ( lparm->needs != NULL) {
       printf("\n THIS TASK NEEDS ONE OF A LIST OF PARENTS TO COMPLETE BEFORE THIS TASK STARTS \n\n");
    }

    /*  Obtain the current queue write index. increases with each call to kernel  */
    uint64_t index = hsa_queue_load_write_index_relaxed(this_Q);
    /* printf("DEBUG:Call #%d to kernel \"%s\" \n",(int) index,"_KN_");  */

    /* Find the queue index address to write the packet info into.  */
    const uint32_t queueMask = this_Q->size - 1;
    hsa_kernel_dispatch_packet_t* this_aql = &(((hsa_kernel_dispatch_packet_t*)(this_Q->base_address))[index&queueMask]);

    /*  FIXME: We need to check for queue overflow here. */

    /* If this kernel was declared as snk_task_t*, then use preallocated signal */
    if ( needs_return_task == 1) {
        if ( SNK_NextTaskId == SNK_MAX_TASKS ) {
           printf("ERROR:  Too many parent tasks, increase SNK_MAX_TASKS =%d\n",SNK_MAX_TASKS);
           return ;
        }
        /* hsa_signal_store_relaxed(SNK_Tasks[SNK_NextTaskId].signal,1); */
        this_aql->completion_signal = SNK_Tasks[SNK_NextTaskId].signal;
    }

    if ( stream_num < 0 ) {
       /* Use the global synchrnous signal Sync_Signal */
       this_aql->completion_signal=Sync_Signal;
       hsa_signal_store_relaxed(Sync_Signal,1);
    }

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
    /* Only set barrier bit if asynchrnous execution */
    if ( stream_num >= 0 )  this_aql->header |= lparm->barrier << HSA_PACKET_HEADER_BARRIER; 
    this_aql->header |= lparm->acquire_fence_scope << HSA_PACKET_HEADER_ACQUIRE_FENCE_SCOPE;
    this_aql->header |= lparm->release_fence_scope  << HSA_PACKET_HEADER_RELEASE_FENCE_SCOPE;
    __atomic_store_n((uint8_t*)(&this_aql->header), (uint8_t)HSA_PACKET_TYPE_KERNEL_DISPATCH, __ATOMIC_RELEASE);

    /* Increment write index and ring doorbell to dispatch the kernel.  */
    hsa_queue_store_write_index_relaxed(this_Q, index+1);
    hsa_signal_store_relaxed(this_Q->doorbell_signal, index);

    if ( stream_num < 0 ) {
       /* For default synchrnous execution, wait til kernel is finished.  */
       hsa_signal_value_t value = hsa_signal_wait_acquire(Sync_Signal, HSA_SIGNAL_CONDITION_LT, 1, UINT64_MAX, HSA_WAIT_STATE_BLOCKED);
    }

    /*  *** END OF KERNEL LAUNCH TEMPLATE ***  */
EOF
}

function write_fortran_lparm_t(){
if [ -f launch_params.f ] ; then 
   echo
   echo "WARNING: The file launch_params.f already exists.   "
   echo "         snack will not overwrite this file.  "
   echo
else
/bin/cat >launch_params.f <<"EOF"
C     INCLUDE launch_params.f in your FORTRAN source so you can set dimensions.
      use, intrinsic :: ISO_C_BINDING
      type, BIND(C) :: snk_lparm_t
          integer (C_INT) :: ndim = 1
          integer (C_SIZE_T) :: gdims(3) = (/ 1 , 0, 0 /)
          integer (C_SIZE_T) :: ldims(3) = (/ 64, 0, 0 /)
          integer (C_INT) :: stream = -1 
          integer (C_INT) :: barrier = 1
          integer (C_INT) :: acquire_fence_scope = 2
          integer (C_INT) :: release_fence_scope = 2
      end type snk_lparm_t
      type (snk_lparm_t) lparm
C  
C     Set default values
C     lparm%ndim=1 
C     lparm%gdims(1)=1
C     lparm%ldims(1)=64
C     lparm%stream=-1 
C     lparm%barrier=1
C  
C  
EOF
fi
}


function is_scalar() {
    scalartypes="int,float,char,double,void,size_t,image3d_t"
    local stype
    IFS=","
    for stype in $scalartypes ; do 
       if [ "$stype" == "$1" ] ; then 
          return 1
       fi
   done
   return 0
}

function parse_arg() {
   arg_name=`echo $1 | awk '{print $NF}'`
   arg_type=`echo $1 | awk '{$NF=""}1' | sed 's/ *$//'`
   if [ "${arg_type:0:7}" == "__local" ] ; then   
      is_local=1
#     arg_type=${arg_type:8}
      arg_type="size_t"
      arg_name="${arg_name}_size"
   else
      is_local=0
   fi
   if [ "${arg_type:0:4}" == "int3" ] ; then   
      arg_type="int*"
   fi
   simple_arg_type=`echo $arg_type | awk '{print $NF}' | sed 's/\*//'`
#  Drop keyword restrict from argument in host callable c function
   if [ "${simple_arg_type}" == "restrict" ] ; then 
      arg_type=${arg_type%%restrict*}
      simple_arg_type=`echo $arg_type | awk '{print $NF}' | sed 's/\*//'`
   fi
   last_char="${arg_type: $((${#arg_type}-1)):1}"
   if [ "$last_char" == "*" ] ; then 
      is_pointer=1
      local __lc='*'
   else
      is_pointer=0
      local __lc=""
      last_char=" " 
   fi
#  Convert CL types to c types.  A lot of work is needed here.
   if [ "$simple_arg_type" == "uint" ] ; then 
      simple_arg_type="int"
      arg_type="unsigned int${__lc}"
   elif [ "$simple_arg_type" == "uchar" ] ; then 
      simple_arg_type="char"
      arg_type="unsigned char${__lc}"
   elif [ "$simple_arg_type" == "uchar16" ] ; then 
      simple_arg_type="int"
      arg_type="unsigned short int${__lc}"
   fi
#   echo "arg_name:$arg_name arg_type:$arg_type  simple_arg_type:$simple_arg_type"
}

#  snk_genw starts here
   
#  Inputs 
__SN=$1
__CLF=$2
__PROGV=$3
#  Work space
__TMPD=$4

#  Outputs: cwrapper, header file, and updated CL 
__CWRAP=$5
__HDRF=$6
__UPDATED_CL=$7

# If snack call snk_genw with -fort option
__IS_FORTRAN=$8

# If snack was called with -noglobs
__NO_GLOB_FUNS=$9

# Intermediate files.
__EXTRACL=${__TMPD}/extra.cl
__KARGLIST=${__TMPD}/klist
__ARGL=""

__WRAPPRE="_"
__SEDCMD=" "

#   if [ $GENW_ADD_DUMMY ] ; then 
#      echo
#      echo "WARNING:  DUMMY ARGS ARE ADDED FOR STABLE COMPILER "
#      echo
#   fi

#  Read the CLF and build a list of kernels and args, one kernel and set of args per line of KARGLIST file
   cpp $__CLF | sed -e '/__kernel/,/)/!d' |  sed -e ':a;$!N;s/\n/ /;ta;P;D' | sed -e 's/__kernel/\n__kernel/g'  | grep "__kernel" | \
   sed -e "s/__kernel//;s/__global//g;s/{//g;s/ \*/\*/g"  | cut -d\) -f1 | sed -e "s/\*/\* /g;s/__restrict__//g" >$__KARGLIST

#  The header and extra-cl files must start empty because lines are incrementally added to end of file
   if [ -f $__EXTRACL ] ; then rm -f $__EXTRACL ; fi
   touch $__EXTRACL

#  Create header file for c and c++ with extra lparm arg (global and local dimensions)
   echo "/* HEADER FILE GENERATED BY snack VERSION $__PROGV */" >$__HDRF
   echo "/* THIS FILE:  $__HDRF  */" >>$__HDRF
   echo "/* INPUT FILE: $__CLF  */" >>$__HDRF
   write_header_template >>$__HDRF

#  Write comments at the beginning of the c wrapper, include copyright notice
   echo "/* THIS TEMPORARY c SOURCE FILE WAS GENERATED BY snack version $__PROGV */" >$__CWRAP
   echo "/* THIS FILE : $__CWRAP  */" >>$__CWRAP
   echo "/* INPUT FILE: $__CLF  */" >>$__CWRAP
   echo "/* UPDATED CL: $__UPDATED_CL  */" >>$__CWRAP
   echo "/*                               */ " >>$__CWRAP
   echo "    " >>$__CWRAP

   write_copyright_template >>$__CWRAP
   write_header_template >>$__CWRAP
   write_context_template | sed -e "s/_CN_/${__SN}/g"  >>$__CWRAP

   if [ "$__NO_GLOB_FUNS" == "0" ] ; then 
      write_global_functions_template >>$__CWRAP
   fi

#  Add includes from CL to the generated C wrapper.
   grep "^#include " $__CLF >> $__CWRAP

#  Process each cl __kernel and its arguments stored as one line in the KARGLIST file
#  We need to process list of args 3 times in this loop.  
#      1) SNACK function declaration
#      2) Build structure for kernel arguments 
#      3) Write values to kernel argument structure

   sed_sepchar=""
   while read line ; do 

#     parse the kernel name __KN and the native argument list __ARGL
      TYPE_NAME=`echo ${line%(*}`
      __KN=`echo $TYPE_NAME | awk '{print $2}'`
      __KT=`echo $TYPE_NAME | awk '{print $1}'`
      __ARGL=${line#*(}
#     force it to return pointer to snk_task_t
      if [ "$__KT" == "snk_task_t" ] ; then  
         __KT="snk_task_t*" 
      fi
         

#     Add the kernel initialization routine to the c wrapper
      write_KernelStatics_template | sed -e "s/_CN_/${__SN}/g;s/_KN_/${__KN}/g" >>$__CWRAP

#     Build a corrected argument list , change CL types to c types as necessary, see parse_arg
      __CFN_ARGL=""
      __PROTO_ARGL=""
      sepchar=""
      IFS=","
      for _val in $__ARGL ; do 
         parse_arg $_val
         __CFN_ARGL="${__CFN_ARGL}${sepchar}${simple_arg_type}${last_char} ${arg_name}"
         __PROTO_ARGL="${__PROTO_ARGL}${sepchar}${arg_type} ${arg_name}"
         sepchar=","
      done

#     Write start of the SNACK function
      echo "/* ------  Start of SNACK function ${__KN} ------ */ " >> $__CWRAP 
      if [ "$__IS_FORTRAN" == "1" ] ; then 
#        Add underscore to kernel name and resolve lparm pointer 
         echo "extern ${__KT} ${__KN}_($__CFN_ARGL, const snk_lparm_t * lparm) {" >>$__CWRAP
      else  
         if [ "$__CFN_ARGL" == "" ] ; then 
            echo "extern ${__KT} $__KN(const snk_lparm_t * lparm) {" >>$__CWRAP
         else
            echo "extern ${__KT} $__KN($__CFN_ARGL, const snk_lparm_t * lparm) {" >>$__CWRAP
         fi
      fi
 
	  echo "   /* Kernel initialization has to be done before kernel arguments are set/inspected */ " >> $__CWRAP
      echo "   if (${__KN}_FK == 0 ) { " >> $__CWRAP
      echo "     status_t status = ${__KN}_init(); " >> $__CWRAP
      echo "     if ( status  != STATUS_SUCCESS ) return; " >> $__CWRAP
      echo "     ${__KN}_FK = 1; " >> $__CWRAP
      echo "   } " >> $__CWRAP
#     Write the structure definition for the kernel arguments.
#     Consider eliminating global _KN__args and memcopy and write directly to thisKernargAddress.
#     by writing these statements here:
      echo "   /* Allocate the kernel argument buffer from the correct region. */ " >> $__CWRAP
      echo "   void* thisKernargAddress; " >> $__CWRAP
      echo "   /* HSA 1.0F has a bug that serializes all queue operations when hsa_memory_allocate is used. " >> $__CWRAP
	  echo "	  Revert back to hsa_memory_allocate once bug is fixed. */ " >> $__CWRAP
	  echo "   thisKernargAddress = malloc(${__KN}_Kernarg_Segment_Size); " >> $__CWRAP
	  #echo "   hsa_memory_allocate(${__SN}_KernargRegion, ${__KN}_Kernarg_Segment_Size, &thisKernargAddress); " >> $__CWRAP
#     How to map a structure into an malloced memory area?
      echo "   struct ${__KN}_args_struct {" >> $__CWRAP
      NEXTI=0
      if [ $GENW_ADD_DUMMY ] ; then 
         echo "      uint64_t arg0;"  >> $__CWRAP
         echo "      uint64_t arg1;"  >> $__CWRAP
         echo "      uint64_t arg2;"  >> $__CWRAP
         echo "      uint64_t arg3;"  >> $__CWRAP
         echo "      uint64_t arg4;"  >> $__CWRAP
         echo "      uint64_t arg5;"  >> $__CWRAP
         NEXTI=6
      fi
      IFS=","
      for _val in $__ARGL ; do 
         parse_arg $_val
         if [ "$last_char" == "*" ] ; then 
            echo "      ${simple_arg_type}* arg${NEXTI};"  >> $__CWRAP
         else
            is_scalar $simple_arg_type
            if [ $? == 1 ] ; then 
               echo "      ${simple_arg_type} arg${NEXTI};"  >> $__CWRAP
            else
               echo "      ${simple_arg_type}* arg${NEXTI};"  >> $__CWRAP
            fi
         fi
         NEXTI=$(( NEXTI + 1 ))
      done
      echo "   } __attribute__ ((aligned (16))) ; "  >> $__CWRAP
      echo "   struct ${__KN}_args_struct* ${__KN}_args ; "  >> $__CWRAP
	  echo "   /* Setup kernel args */ " >> $__CWRAP
	  echo "   ${__KN}_args = (struct ${__KN}_args_struct*) thisKernargAddress; " >> $__CWRAP

#     Write statements to fill in the argument structure and 
#     keep track of updated CL arg list and new call list 
#     in case we have to create a wrapper CL function.
#     to call the real kernel CL function. 
      NEXTI=0
      if [ $GENW_ADD_DUMMY ] ; then 
         echo "   ${__KN}_args->arg0=0 ; "  >> $__CWRAP
         echo "   ${__KN}_args->arg1=0 ; "  >> $__CWRAP
         echo "   ${__KN}_args->arg2=0 ; "  >> $__CWRAP
         echo "   ${__KN}_args->arg3=0 ; "  >> $__CWRAP
         echo "   ${__KN}_args->arg4=0 ; "  >> $__CWRAP
         echo "   ${__KN}_args->arg5=0 ; "  >> $__CWRAP
         NEXTI=6
      fi
      KERN_NEEDS_CL_WRAPPER="FALSE"
      arglistw=""
      calllist=""
      sepchar=""
      IFS=","
      for _val in $__ARGL ; do 
         parse_arg $_val
#        These echo statments help debug a lot
#        echo "simple_arg_type=|${simple_arg_type}|" 
#        echo "arg_type=|${arg_type}|" 
         if [ "$last_char" == "*" ] ; then 
            arglistw="${arglistw}${sepchar}${arg_type} ${arg_name}"
            calllist="${calllist}${sepchar}${arg_name}"
            echo "   ${__KN}_args->arg${NEXTI} = $arg_name ; "  >> $__CWRAP
         else
            is_scalar $simple_arg_type
            if [ $? == 1 ] ; then 
               arglistw="$arglistw${sepchar}${arg_type} $arg_name"
               calllist="${calllist}${sepchar}${arg_name}"
               echo "   ${__KN}_args->arg${NEXTI} = $arg_name ; "  >> $__CWRAP
            else
               KERN_NEEDS_CL_WRAPPER="TRUE"
               arglistw="$arglistw${sepchar}${arg_type}* $arg_name"
               calllist="${calllist}${sepchar}${arg_name}[0]"
               echo "   ${__KN}_args->arg${NEXTI} = &$arg_name ; "  >> $__CWRAP
            fi
         fi 
         sepchar=","
         NEXTI=$(( NEXTI + 1 ))
      done
      
#     Write the extra CL if we found call-by-value structs and write the extra CL needed
      if [ "$KERN_NEEDS_CL_WRAPPER" == "TRUE" ] ; then 
         echo "__kernel void ${__WRAPPRE}$__KN($arglistw){ $__KN($calllist) ; } " >> $__EXTRACL
         __FN="\&__OpenCL_${__WRAPPRE}${__KN}_kernel"
#        change the original __kernel (external callable) to internal callable
         __SEDCMD="${__SEDCMD}${sed_sepchar}s/__kernel void $__KN /void $__KN/;s/__kernel void $__KN(/void $__KN(/"
         sed_sepchar=";"
      else
         __FN="\&__OpenCL_${__KN}_kernel"
      fi

#     Write the prototype to the header file
      if [ "$__IS_FORTRAN" == "1" ] ; then 
#        don't use headers for fortran but it is a good reference for how to call from fortran
         echo "extern _CPPSTRING_ $__KT ${__KN}_($__PROTO_ARGL, const snk_lparm_t * lparm_p);" >>$__HDRF
      else
         if [ "$__PROTO_ARGL" == "" ] ; then 
            echo "extern _CPPSTRING_ $__KT ${__KN}(const snk_lparm_t * lparm);" >>$__HDRF
         else
            echo "extern _CPPSTRING_ $__KT ${__KN}($__PROTO_ARGL, const snk_lparm_t * lparm);" >>$__HDRF
         fi
      fi

#     Make sure template knows when to allocate and bind a global signal for this function
      if [ $__KT == "snk_task_t*" ] ; then 
         echo "   int needs_return_task = 1;" >>$__CWRAP
      else
         echo "   int needs_return_task = 0;" >>$__CWRAP
      fi

#     Now add the kernel template to wrapper and change all three strings
#     1) Context Name _CN_ 2) Kerneel name _KN_ and 3) Funtion name _FN_
      write_kernel_template | sed -e "s/_CN_/${__SN}/g;s/_KN_/${__KN}/g;s/_FN_/${__FN}/g" >>$__CWRAP

#     if kernel is type snk_task_t*, then return &parentTask else return void 
# FIXME: Need to rotate and reuse the task array!
      if [ $__KT == "snk_task_t*" ] ; then 
         echo "    return (snk_task_t*) &(SNK_Tasks[SNK_NextTaskId++]);" >> $__CWRAP 
      else
         echo "    return;" >> $__CWRAP 
      fi 
      echo "} " >> $__CWRAP 
      echo "/* ------  End of SNACK function ${__KN} ------ */ " >> $__CWRAP 

#     Add the kernel initialization routine to the c wrapper
      write_InitKernel_template | sed -e "s/_CN_/${__SN}/g;s/_KN_/${__KN}/g;s/_FN_/${__FN}/g" >>$__CWRAP

#  END OF WHILE LOOP TO PROCESS EACH KERNEL IN THE CL FILE
   done < $__KARGLIST


   if [ "$__IS_FORTRAN" == "1" ] ; then 
      write_fortran_lparm_t
   fi

#  Write the updated CL
   if [ "$__SEDCMD" != " " ] ; then 
#     Remove extra spaces, then change "__kernel void" to "void" if they have call-by-value structs
#     Still could fail if __kernel void _FN_ split across multple lines, FIX THIS
      awk '$1=$1'  $__CLF | sed -e "$__SEDCMD" > $__UPDATED_CL
      cat $__EXTRACL | sed -e "s/ snk_task_t/ void/g" >> $__UPDATED_CL
   else 
#  No changes to the CL file are needed, so just make a copy
      cat $__CLF | sed -e "s/ snk_task_t/ void/g" > $__UPDATED_CL
   fi

   rm $__KARGLIST
   rm $__EXTRACL 

