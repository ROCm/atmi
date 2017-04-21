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
#ifndef __ATMI_KL_H__
#define __ATMI_KL_H__

#define MAX_NUM_KERNELS (1024)
#define MAX_NUM_KERNEL_TYPES (8)
/*typedef struct atmi_task_impl_s {
    unsigned long int signal;
    unsigned char reserved[376];
} atmi_task_impl_t;
*/
typedef struct atmi_implicit_args_s {
    unsigned long    offset_x;
    unsigned long    offset_y;
    unsigned long    offset_z;
    unsigned long    pipe_ptr;
    char             num_gpu_queues;
    unsigned long    gpu_queue_ptr;
    char             num_cpu_queues;
    unsigned long    cpu_worker_signals;
    unsigned long    cpu_queue_ptr;
    unsigned long    kernarg_template_ptr;
//  possible TODO: send signal pool to be used by DAGs on GPU
//  uint8_t     num_signals;
//  unsigned long    signal_ptr;
} atmi_implicit_args_t;

typedef struct atmi_kernel_enqueue_template_s {
    unsigned long kernel_handle;
    hsa_kernel_dispatch_packet_t k_packet;
    hsa_agent_dispatch_packet_t a_packet;
    unsigned long    kernarg_segment_size;
    void *kernarg_regions;
} atmi_kernel_enqueue_template_t;

#endif
