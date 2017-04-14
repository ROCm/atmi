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

#pragma OPENCL EXTENSION cl_khr_int64_base_atomics : enable
#pragma OPENCL EXTENSION cl_khr_int64_extended_atomics : enable

#define ATTR __attribute__((always_inline))
static ATTR void
update_mbox(const __global amd_signal_t *sig)
{
    __global atomic_ulong *mb = (__global atomic_ulong *)sig->event_mailbox_ptr;
    if (mb) {
        uint id = sig->event_id;
        atomic_store_explicit(mb, id, memory_order_release, memory_scope_all_svm_devices);
        __llvm_amdgcn_s_sendmsg(1 | (0 << 4), __llvm_amdgcn_readfirstlane(id) & 0xff);
    }
}

uint16_t create_header(hsa_packet_type_t type, int barrier) {
   uint16_t header = type << HSA_PACKET_HEADER_TYPE;
   header |= barrier << HSA_PACKET_HEADER_BARRIER;
   header |= HSA_FENCE_SCOPE_SYSTEM << HSA_PACKET_HEADER_ACQUIRE_FENCE_SCOPE;
   header |= HSA_FENCE_SCOPE_SYSTEM << HSA_PACKET_HEADER_RELEASE_FENCE_SCOPE;
   return header;
}
#if 0
void kernel_dispatch(const atmi_klparm_t *lparm, hsa_kernel_dispatch_packet_t *kernel_packet, const int pif_id) { 
 
    atmi_klist_t *atmi_klist = (atmi_klist_t *)get_atmi_context();
    //hsa_kernel_dispatch_packet_t *kernel_packet = (hsa_kernel_dispatch_packet_t *)(atmi_klist[pif_id].kernel_packets + k_id); 
 
    //int q_offset = atomic_fetch_add_explicit((__global int *)(&(atmi_klist[pif_id].gpu_queue_offset)), 1, memory_order_relaxed, memory_scope_all_svm_devices); 
    //hsa_queue_t* this_Q = (hsa_queue_t *)atmi_klist[pif_id].gpu_queues[q_offset % atmi_klist[pif_id].num_gpu_queues]; 
    hsa_queue_t* this_Q = (hsa_queue_t *)atmi_klist[pif_id].queues[device_queue]; 
 
    /* Find the queue index address to write the packet info into.  */ 
    const uint32_t queueMask = this_Q->size - 1; 
    uint64_t index = __ockl_hsa_queue_add_write_index(this_Q, 1, __ockl_memory_order_relaxed); 
    hsa_kernel_dispatch_packet_t *this_aql = &(((hsa_kernel_dispatch_packet_t *)(this_Q->base_address))[index&queueMask]); 
 

    int ndim = -1;
    if(lparm->gridDim[2] > 1)
        ndim = 3;
    else if(lparm->gridDim[1] > 1)
        ndim = 2;
    else
        ndim = 1;

    /*  Process lparm values */ 
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
 
    /* thisKernargAddress has already been set up in the beginning of this routine */ 
    /*  Bind kernel argument buffer to the aql packet.  */ 
    this_aql->kernarg_address = kernel_packet->kernarg_address; 
    this_aql->kernel_object = kernel_packet->kernel_object; 
    this_aql->private_segment_size = kernel_packet->private_segment_size; 
    this_aql->group_segment_size = kernel_packet->group_segment_size; 
    this_aql->completion_signal = kernel_packet->completion_signal; 

    __ockl_hsa_signal_add(this_aql->completion_signal, 1, __ockl_memory_order_relaxed); 
 
    /*  Prepare and set the packet header */  
    /* Only set barrier bit if asynchrnous execution */ 
    this_aql->header = create_header(HSA_PACKET_TYPE_KERNEL_DISPATCH, ATMI_FALSE); 
 
    /* Increment write index and ring doorbell to dispatch the kernel.  */ 
    __ockl_hsa_signal_store(this_Q->doorbell_signal, atmi_klist[pif_id].gpu_kernarg_offset, __ockl_memory_order_relaxed); 
    //__ockl_hsa_signal_add(this_Q->doorbell_signal, 1); 
}
#endif


void agent_dispatch(global hsa_queue_t *this_Q, void *kernarg_region, 
                     hsa_agent_dispatch_packet_t *kernel_packet, hsa_signal_t *worker_sig,
                     global ulong *arr) { 
 
    // *** this task ID ***
    __constant hsa_kernel_dispatch_packet_t *p = __llvm_amdgcn_dispatch_ptr();
    atmi_task_handle_t thisTask = p->reserved2;

    /* Find the queue index address to write the packet info into.  */ 
    const uint32_t queueMask = this_Q->size - 1; 
    uint64_t index = __ockl_hsa_queue_add_write_index(this_Q, 1, __ockl_memory_order_relaxed); 
    hsa_agent_dispatch_packet_t *this_aql = 
        &(((hsa_agent_dispatch_packet_t *)(uintptr_t)(this_Q->base_address))[index&queueMask]); 
    
    this_aql->type   = 1;//kernel_packet->type;  // this should be fixed when we merge atmi_kernel_create_kernel to 1 function
    this_aql->arg[0] = thisTask;
    this_aql->arg[1] = (uint64_t) kernarg_region;
    this_aql->arg[2] = kernel_packet->arg[2]; 
    this_aql->arg[3] = kernel_packet->arg[3]; 
    // *** get signal ***
    __ockl_hsa_signal_add(p->completion_signal, 1, __ockl_memory_order_release); 
    //__ockl_hsa_signal_store(p->completion_signal, 65, __ockl_memory_order_release); 
    this_aql->completion_signal = p->completion_signal; 
    //*arr = __ockl_hsa_signal_load(p->completion_signal, __ockl_memory_order_release);
    //return;

 
    /*  Prepare and set the packet header */  
    /* Only set barrier bit if asynchrnous execution */ 
    this_aql->header = create_header(HSA_PACKET_TYPE_AGENT_DISPATCH, ATMI_FALSE); 
 
    /* Increment write index and ring doorbell to dispatch the kernel.  */ 
    __ockl_hsa_signal_store(this_Q->doorbell_signal, index, __ockl_memory_order_release); 
    //__ockl_hsa_signal_add(this_Q->doorbell_signal, 1, __ockl_memory_order_release); 
 
    __ockl_hsa_signal_store(*worker_sig, 0, __ockl_memory_order_release); 
}

enum mainTask_kid_klist{K_ID_mainTask_gpu = 42, };
enum print_kid_klist{K_ID_print_taskId_cpu = 42, };
enum subTask_kid_klist{K_ID_subTask_gpu = 42, };

void atmi_task_launch_print(global ulong *arr, atmi_lparm_t *lp, long int taskId) {
    // *** agent_dispatch
    constant atmi_implicit_args_t *impl_args = (constant atmi_implicit_args_t *)__llvm_amdgcn_implicitarg_ptr();

    // *** get kernel arg region ***
    atmi_kernel_enqueue_template_t *ke_template = (atmi_kernel_enqueue_template_t *)impl_args->kernarg_template_ptr; 
    // assume print = 2;
    atmi_kernel_enqueue_template_t *this_ke_template = ke_template + 2;
    void *kernarg_region = this_ke_template->kernarg_regions;
    atomic_int *kernarg_index = (atomic_int *) kernarg_region;
    //int kernarg_offset = atomic_fetch_add(kernarg_index, 1);
    int kernarg_offset = atomic_fetch_add_explicit(kernarg_index, 1, memory_order_relaxed, memory_scope_all_svm_devices);
    kernarg_region = (void *)((char *)kernarg_region + sizeof(int) + (kernarg_offset * impl_args->kernarg_segment_size));

    // *** set args ***
    *(int *)kernarg_region = taskId;
    
    // *** get queue ***
    int qid = 0;//taskId & 1;
    global hsa_queue_t *queue = ((global hsa_queue_t **)(impl_args->cpu_queue_ptr))[qid];
    hsa_signal_t worker_sig = ((hsa_signal_t *)(impl_args->cpu_worker_signals))[qid];
#if 0
    __ockl_hsa_signal_store(worker_sig, 42, __ockl_memory_order_seq_cst); 
    //ulong val = __ockl_hsa_signal_exchange(worker_sig, 42, __ockl_memory_order_release); 
    volatile ulong foo;
#if 0
    __global amd_signal_t *s = (__global amd_signal_t *)worker_sig.handle;
    foo = atomic_fetch_add_explicit((__global atomic_long *)&s->value, 3, 
                                              memory_order_seq_cst, memory_scope_all_svm_devices);

    update_mbox(s);
    *arr = s->value;
    //const __global amd_signal_t *s2 = (const __global amd_signal_t *)worker_sig.handle;
    //*arr = atomic_load_explicit((__global atomic_long *)&s2->value, memory_order_seq_cst, memory_scope_all_svm_devices);
    //*arr = s2->value;
#else
    __ockl_hsa_signal_add(worker_sig, 3, __ockl_memory_order_release); 
    *arr = __ockl_hsa_signal_load(worker_sig, __ockl_memory_order_acquire);
    //*arr = foo;
#endif
    //__ockl_hsa_signal_store(worker_sig, 65, __ockl_memory_order_release);                                                  
    
return;
    __constant hsa_kernel_dispatch_packet_t *p = __llvm_amdgcn_dispatch_ptr();
//    *arr = __ockl_hsa_signal_load(p->completion_signal, __ockl_memory_order_seq_cst);
#endif
    agent_dispatch(queue, kernarg_region, &(this_ke_template->a_packet), &worker_sig, arr);
    return;
}

__kernel void mainTask_gpu(__global ulong *arr, long int numTasks) {
//__kernel void mainTask_gpu(long int numTasks) {
	int gid = get_global_id(0);
    constant atmi_implicit_args_t *impl_args = (constant atmi_implicit_args_t *)__llvm_amdgcn_implicitarg_ptr();
    
    constant hsa_kernel_dispatch_packet_t *p = __llvm_amdgcn_dispatch_ptr();
    if(gid < numTasks) {
        atmi_task_launch_print(arr, NULL, gid);
        //ATMI_KLPARM_1D(klparm, 1, thisTask);
        //klparm->kernel_id = K_ID_subTask_gpu; //tell decode to use decode_gpu kernel
        //subTask(klparm, gid);
        //klparm->kernel_id = K_ID_print_taskId_cpu; //tell print_taskId to use print_taskId_cpu
        //print(klparm, gid);
    }
}

__kernel void print_taskId_gpu(int taskId) {
}

__kernel void subTask_gpu(int taskId) {
}

