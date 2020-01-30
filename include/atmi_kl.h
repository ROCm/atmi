/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/
#ifndef INCLUDE_ATMI_KL_H_
#define INCLUDE_ATMI_KL_H_

#include <atmi.h>
#ifdef __OPENCL_C_VERSION__
#include "ockl_hsa.h"
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

extern void atmid_task_launch(atmi_lparm_t *lp, unsigned long kernel_id,
                              void *args_region, size_t args_region_size);

#endif  // INCLUDE_ATMI_KL_H_
