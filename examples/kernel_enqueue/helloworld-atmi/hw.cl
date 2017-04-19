/*
MIT License

Copyright © 2016 Advanced Micro Devices, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software
without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "atmi.h"
#include "ockl/inc/ockl_hsa.h"
#include "irif/inc/irif.h"
#include "atmi_kl.h"

uint16_t create_header(hsa_packet_type_t type, int barrier) {
   uint16_t header = type << HSA_PACKET_HEADER_TYPE;
   header |= barrier << HSA_PACKET_HEADER_BARRIER;
   header |= HSA_FENCE_SCOPE_SYSTEM << HSA_PACKET_HEADER_ACQUIRE_FENCE_SCOPE;
   header |= HSA_FENCE_SCOPE_SYSTEM << HSA_PACKET_HEADER_RELEASE_FENCE_SCOPE;
   return header;
}

void kernel_dispatch(atmi_lparm_t *lparm, global hsa_queue_t *this_Q, 
                     void *kernarg_region, hsa_kernel_dispatch_packet_t *kernel_packet) { 
 
    // *** this task ID ***
    __constant hsa_kernel_dispatch_packet_t *p = __llvm_amdgcn_dispatch_ptr();

    /* Find the queue index address to write the packet info into.  */ 
    const uint32_t queueMask = this_Q->size - 1; 
    uint64_t index = __ockl_hsa_queue_add_write_index(this_Q, 1, __ockl_memory_order_relaxed); 
    hsa_kernel_dispatch_packet_t *this_aql = 
            &(((hsa_kernel_dispatch_packet_t *)(uintptr_t)(this_Q->base_address))[index&queueMask]); 
 

    int ndim = -1;
    if(lparm->gridDim[2] > 1)
        ndim = 3;
    else if(lparm->gridDim[1] > 1)
        ndim = 2;
    else
        ndim = 1;

    /*  Process launch parameter values */ 
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
    } 
    else 
    { 
        this_aql->grid_size_z=1; 
        this_aql->workgroup_size_z=1; 
    } 
    
    this_aql->reserved2 = p->reserved2;
 
    /* thisKernargAddress has already been set up in the beginning of this routine */ 
    /*  Bind kernel argument buffer to the aql packet.  */ 
    this_aql->kernarg_address = (__constant void *)(uintptr_t)kernarg_region; 
    this_aql->kernel_object = kernel_packet->kernel_object; 
    this_aql->private_segment_size = kernel_packet->private_segment_size; 
    this_aql->group_segment_size = kernel_packet->group_segment_size; 

    // *** get signal ***
    __ockl_hsa_signal_add(p->completion_signal, 1, __ockl_memory_order_release); 
    this_aql->completion_signal = p->completion_signal; 
 
    /* Prepare and set the packet header */  
    /* Only set barrier bit if asynchronous execution */ 
    this_aql->header = create_header(HSA_PACKET_TYPE_KERNEL_DISPATCH, ATMI_FALSE); 
 
    /* Increment write index and ring doorbell to dispatch the kernel.  */ 
    __ockl_hsa_signal_store(this_Q->doorbell_signal, index, __ockl_memory_order_release); 
    //__ockl_hsa_signal_add(this_Q->doorbell_signal, 1, __ockl_memory_order_release); 
}


void agent_dispatch(atmi_lparm_t *lparm, global hsa_queue_t *this_Q, void *kernarg_region, 
                     hsa_agent_dispatch_packet_t *kernel_packet, hsa_signal_t *worker_sig) { 
 
    // *** this task ID ***
    __constant hsa_kernel_dispatch_packet_t *p = __llvm_amdgcn_dispatch_ptr();
    atmi_task_handle_t thisTask = p->reserved2;

    /* Find the queue index address to write the packet info into.  */ 
    const uint32_t queueMask = this_Q->size - 1; 
    uint64_t index = __ockl_hsa_queue_add_write_index(this_Q, 1, __ockl_memory_order_relaxed); 
    hsa_agent_dispatch_packet_t *this_aql = 
        &(((hsa_agent_dispatch_packet_t *)(uintptr_t)(this_Q->base_address))[index&queueMask]); 
    
    this_aql->type   = kernel_packet->type;  // this should be fixed when we merge atmi_kernel_create_kernel to 1 function
    this_aql->arg[0] = thisTask;
    this_aql->arg[1] = (uint64_t) kernarg_region;
    this_aql->arg[2] = kernel_packet->arg[2]; 
    this_aql->arg[3] = kernel_packet->arg[3]; 
    // *** get signal ***
    __ockl_hsa_signal_add(p->completion_signal, 1, __ockl_memory_order_release); 
    this_aql->completion_signal = p->completion_signal; 

 
    /* Prepare and set the packet header */  
    /* Only set barrier bit if asynchronous execution */ 
    this_aql->header = create_header(HSA_PACKET_TYPE_AGENT_DISPATCH, ATMI_FALSE); 
 
    /* Increment write index and ring doorbell to dispatch the kernel.  */ 
    __ockl_hsa_signal_store(this_Q->doorbell_signal, index, __ockl_memory_order_release); 
    //__ockl_hsa_signal_add(this_Q->doorbell_signal, 1, __ockl_memory_order_release); 
 
    __ockl_hsa_signal_store(*worker_sig, 0, __ockl_memory_order_release); 
}

void atmi_task_launch(atmi_lparm_t *lp, ulong kernel_id, void *args_region, size_t args_region_size) {
    constant atmi_implicit_args_t *impl_args = (constant atmi_implicit_args_t *)__llvm_amdgcn_implicitarg_ptr();

    // *** get kernel arg region ***
    atmi_kernel_enqueue_template_t *ke_template = (atmi_kernel_enqueue_template_t *)impl_args->kernarg_template_ptr; 
    // *** get kernel template for kernel_id
    atmi_kernel_enqueue_template_t *this_ke_template = ke_template + kernel_id;
    void *kernarg_region = this_ke_template->kernarg_regions;
    atomic_int *kernarg_index = (atomic_int *) kernarg_region;
    int kernarg_offset = atomic_fetch_add_explicit(kernarg_index, 1, memory_order_relaxed, memory_scope_all_svm_devices);
    kernarg_region = (void *)((char *)kernarg_region + sizeof(int) + (kernarg_offset * ke_template->kernarg_segment_size));

    // *** set args ***
    for(size_t i = 0; i < args_region_size; i++) {
        ((char *)kernarg_region)[i] = ((char *)args_region)[i];
    }
    
    if(lp->place.type == ATMI_DEVTYPE_CPU) {
        // *** get queue ***
        int qid = get_global_id(0) & 1;
        global hsa_queue_t *queue = ((global hsa_queue_t **)(impl_args->cpu_queue_ptr))[qid];
        hsa_signal_t worker_sig = ((hsa_signal_t *)(impl_args->cpu_worker_signals))[qid];
    
        // *** agent_dispatch
        agent_dispatch(lp, queue, kernarg_region, &(this_ke_template->a_packet), &worker_sig);
    }
    else if(lp->place.type == ATMI_DEVTYPE_GPU) {
        // *** get queue ***
        int qid = get_global_id(0) & 1;
        global hsa_queue_t *queue = ((global hsa_queue_t **)(impl_args->gpu_queue_ptr))[qid];
    
        // *** kernel_dispatch
        kernel_dispatch(lp, queue, kernarg_region, &(this_ke_template->k_packet));
    }
    return;
}

enum { 
    K_ID_mainTask_gpu = 0, 
    K_ID_print_taskId_cpu = 1,
    K_ID_subTask_gpu = 2,
};

typedef struct args_s {
    int arg1;
} args_t;

void subTask_gpu(int taskId) {
    ATMI_LPARM_1D(lparm, 1);
    lparm->place = (atmi_place_t)ATMI_PLACE_CPU(0, 0);
    // default case for kernel enqueue: lparm->groupable = ATMI_TRUE;
    args_t args;
    args.arg1 = taskId;
    
    atmi_task_launch(lparm, K_ID_print_taskId_cpu, (void *)&args, sizeof(args_t));
}

__kernel void mainTask_gpu(__global ulong *arr, long int numTasks) {
	int gid = get_global_id(0);
    subTask_gpu(gid);
#if 0
    ATMI_LPARM_1D(lparm, 1);
    lparm->place = (atmi_place_t)ATMI_PLACE_GPU(0, 0);
    // default case for kernel enqueue: lparm->groupable = ATMI_TRUE;
    args_t args;
    args.arg1 = gid;

    if(gid < numTasks) {
        atmi_task_launch(lparm, K_ID_subTask_gpu, (void *)&args, sizeof(args_t));
    }
#endif
}

__kernel void print_taskId_gpu(int taskId) {
}

