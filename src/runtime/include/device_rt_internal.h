/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/
#ifndef SRC_RUNTIME_INCLUDE_DEVICE_RT_INTERNAL_H_
#define SRC_RUNTIME_INCLUDE_DEVICE_RT_INTERNAL_H_

#ifdef __OPENCL_C_VERSION__
#include "device_amd_hsa.h"
#else
#include <hsa/hsa.h>
#endif

#define MAX_NUM_KERNELS (1024 * 16)
/*typedef struct atmi_task_impl_s {
    unsigned long int signal;
    unsigned char reserved[376];
} atmi_task_impl_t;
*/
typedef struct atmi_implicit_args_s {
  unsigned long offset_x;
  unsigned long offset_y;
  unsigned long offset_z;
  unsigned long hostcall_ptr;
  char num_gpu_queues;
  unsigned long gpu_queue_ptr;
  char num_cpu_queues;
  unsigned long cpu_worker_signals;
  unsigned long cpu_queue_ptr;
  unsigned long kernarg_template_ptr;
  //  possible TODO: send signal pool to be used by DAGs on GPU
  //  uint8_t     num_signals;
  //  unsigned long    signal_ptr;
} atmi_implicit_args_t;

typedef struct atmi_kernel_enqueue_template_s {
  unsigned long kernel_handle;
  hsa_kernel_dispatch_packet_t k_packet;
  hsa_agent_dispatch_packet_t a_packet;
  unsigned long kernarg_segment_size;
  void *kernarg_regions;
} atmi_kernel_enqueue_template_t;

#endif  // SRC_RUNTIME_INCLUDE_DEVICE_RT_INTERNAL_H_
