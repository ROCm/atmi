/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/
#include "device_rt.h"
#include <map>
#include <vector>
#include "ATLMachine.h"
#include "atl_internal.h"
#include "atmi_kl.h"
#include "kernel.h"

extern ATLMachine g_atl_machine;
extern std::map<uint64_t, core::Kernel *> KernelImplMap;
atl_kernel_enqueue_args_t g_ke_args;
namespace core {

bool g_atmi_devrt_initialized = false;
void atl_set_atmi_devrt_initialized() {
  // FIXME: thread safe? locks?
  g_atmi_devrt_initialized = true;
}

void atl_reset_atmi_devrt_initialized() {
  // FIXME: thread safe? locks?
  g_atmi_devrt_initialized = false;
}

bool atl_is_atmi_devrt_initialized() { return g_atmi_devrt_initialized; }

atmi_status_t atmi_ke_init() {
  // create and fill in the global structure needed for device enqueue
  // fill in gpu queues
  hsa_status_t err;
  std::vector<hsa_queue_t *> gpu_queues;
  int gpu_count = g_atl_machine.processorCount<ATLGPUProcessor>();
  for (int gpu = 0; gpu < gpu_count; gpu++) {
    int num_queues = 0;
    atmi_place_t place = ATMI_PLACE_GPU(0, gpu);
    ATLGPUProcessor &proc = get_processor<ATLGPUProcessor>(place);
    std::vector<hsa_queue_t *> qs = proc.queues();
    num_queues = qs.size();
    gpu_queues.insert(gpu_queues.end(), qs.begin(), qs.end());
    // TODO(ashwinma): how to handle queues from multiple devices? keep them
    // separate?
    // Currently, first N queues correspond to GPU0, next N queues map to GPU1
    // and so on.
  }
  g_ke_args.num_gpu_queues = gpu_queues.size();
  void *gpu_queue_ptr = NULL;
  if (g_ke_args.num_gpu_queues > 0) {
    err = hsa_amd_memory_pool_allocate(
        atl_gpu_kernarg_pool, sizeof(hsa_queue_t *) * g_ke_args.num_gpu_queues,
        0, &gpu_queue_ptr);
    ErrorCheck(Allocating GPU queue pointers, err);
    allow_access_to_all_gpu_agents(gpu_queue_ptr);
    for (int gpuq = 0; gpuq < gpu_queues.size(); gpuq++) {
      (reinterpret_cast<hsa_queue_t **>(gpu_queue_ptr))[gpuq] =
          gpu_queues[gpuq];
    }
  }
  g_ke_args.gpu_queue_ptr = gpu_queue_ptr;

  // fill in cpu queues
  std::vector<hsa_queue_t *> cpu_queues;
  int cpu_count = g_atl_machine.processorCount<ATLCPUProcessor>();
  for (int cpu = 0; cpu < cpu_count; cpu++) {
    int num_queues = 0;
    atmi_place_t place = ATMI_PLACE_CPU(0, cpu);
    ATLCPUProcessor &proc = get_processor<ATLCPUProcessor>(place);
    std::vector<hsa_queue_t *> qs = proc.queues();
    num_queues = qs.size();
    cpu_queues.insert(cpu_queues.end(), qs.begin(), qs.end());
    // TODO(ashwinma): how to handle queues from multiple devices? keep them
    // separate?
    // Currently, first N queues correspond to CPU0, next N queues map to CPU1
    // and so on.
  }
  g_ke_args.num_cpu_queues = cpu_queues.size();
  void *cpu_queue_ptr = NULL;
  if (g_ke_args.num_cpu_queues > 0) {
    err = hsa_amd_memory_pool_allocate(
        atl_gpu_kernarg_pool, sizeof(hsa_queue_t *) * g_ke_args.num_cpu_queues,
        0, &cpu_queue_ptr);
    ErrorCheck(Allocating CPU queue pointers, err);
    allow_access_to_all_gpu_agents(cpu_queue_ptr);
    for (int cpuq = 0; cpuq < cpu_queues.size(); cpuq++) {
      (reinterpret_cast<hsa_queue_t **>(cpu_queue_ptr))[cpuq] =
          cpu_queues[cpuq];
    }
  }
  g_ke_args.cpu_queue_ptr = cpu_queue_ptr;

  void *cpu_worker_signals = NULL;
  if (g_ke_args.num_cpu_queues > 0) {
    err = hsa_amd_memory_pool_allocate(
        atl_gpu_kernarg_pool, sizeof(hsa_signal_t) * g_ke_args.num_cpu_queues,
        0, &cpu_worker_signals);
    ErrorCheck(Allocating CPU queue iworker signals, err);
    allow_access_to_all_gpu_agents(cpu_worker_signals);
    for (int cpuq = 0; cpuq < cpu_queues.size(); cpuq++) {
      (reinterpret_cast<hsa_signal_t *>(cpu_worker_signals))[cpuq] =
          *(get_worker_sig(cpu_queues[cpuq]));
    }
  }
  g_ke_args.cpu_worker_signals = cpu_worker_signals;

  void *kernarg_template_ptr = NULL;
  int max_kernel_types = core::Runtime::getInstance().getMaxKernelTypes();
  if (max_kernel_types > 0) {
    // Allocate template space for shader kernels
    err = hsa_amd_memory_pool_allocate(
        atl_gpu_kernarg_pool,
        sizeof(atmi_kernel_enqueue_template_t) * max_kernel_types, 0,
        &kernarg_template_ptr);
    ErrorCheck(Allocating kernel argument template pointer, err);
    allow_access_to_all_gpu_agents(kernarg_template_ptr);
  }
  g_ke_args.kernarg_template_ptr = kernarg_template_ptr;
  g_ke_args.kernel_counter = 0;
  return ATMI_STATUS_SUCCESS;
}

atmi_status_t DeviceRuntime::Initialize(atmi_devtype_t devtype) {
  Runtime::Initialize(devtype);
  if (atl_is_atmi_devrt_initialized()) return ATMI_STATUS_SUCCESS;
  ATMIErrorCheck(Device enqueue init, atmi_ke_init());
  atl_set_atmi_devrt_initialized();
  return ATMI_STATUS_SUCCESS;
}

atmi_status_t DeviceRuntime::Finalize() {
  // free up the kernel enqueue related data
  for (int i = 0; i < g_ke_args.kernel_counter; i++) {
    atmi_kernel_enqueue_template_t *ke_template =
        &(reinterpret_cast<atmi_kernel_enqueue_template_t *>(
            g_ke_args.kernarg_template_ptr))[i];
    ErrorCheck(Memory pool free,
               hsa_amd_memory_pool_free(ke_template->kernarg_regions));
  }
  ErrorCheck(Memory pool free,
             hsa_amd_memory_pool_free(g_ke_args.kernarg_template_ptr));
  ErrorCheck(Memory pool free,
             hsa_amd_memory_pool_free(g_ke_args.cpu_queue_ptr));
  ErrorCheck(Memory pool free,
             hsa_amd_memory_pool_free(g_ke_args.cpu_worker_signals));
  ErrorCheck(Memory pool free,
             hsa_amd_memory_pool_free(g_ke_args.gpu_queue_ptr));
  Runtime::Finalize();
  return ATMI_STATUS_SUCCESS;
}

atmi_status_t DeviceRuntime::CreateKernel(atmi_kernel_t *atmi_kernel,
                                          const int num_args,
                                          const size_t *arg_sizes,
                                          const int num_impls,
                                          va_list arguments) {
  va_list dup_arguments;
  va_copy(dup_arguments, arguments);
  atmi_status_t status = Runtime::CreateKernel(atmi_kernel, num_args, arg_sizes,
                                               num_impls, dup_arguments);
  ATMIErrorCheck(Creating base kernel object, status);
  hsa_status_t err;

  bool has_gpu_impl = false;
  uint64_t k_id = atmi_kernel->handle;
  Kernel *kernel = KernelImplMap[k_id];
  size_t max_kernarg_segment_size = 0;
  // va_list arguments;
  // va_start(arguments, num_impls);
  for (int impl_id = 0; impl_id < num_impls; impl_id++) {
    atmi_devtype_t devtype = (atmi_devtype_t)va_arg(arguments, int);
    if (devtype == ATMI_DEVTYPE_GPU) {
      has_gpu_impl = true;
    }
    size_t this_kernarg_segment_size =
        kernel->impls()[impl_id]->kernarg_segment_size();
    if (this_kernarg_segment_size > max_kernarg_segment_size)
      max_kernarg_segment_size = this_kernarg_segment_size;
    ATMIErrorCheck(Creating kernel implementations, status);
  }
  // va_end(arguments);
  //// FIXME EEEEEE: for EVERY GPU impl, add all CPU/GPU implementations in
  // their templates!!!
  if (has_gpu_impl) {
    // fill in the kernel template AQL packets
    // FIXME: reformulate this for debug mode
    // assert(cur_kernel < core::Runtime::getInstance().getMaxKernelTypes());
    int max_kernel_types = core::Runtime::getInstance().getMaxKernelTypes();
    if (g_ke_args.kernel_counter + 1 >= max_kernel_types) {
      fprintf(stderr,
              "Creating more than %d task types. Set "
              "ATMI_MAX_KERNEL_TYPES environment variable to a larger "
              "number and try again.\n",
              max_kernel_types);
      return ATMI_STATUS_KERNELCOUNT_OVERFLOW;
    }
    int cur_kernel = g_ke_args.kernel_counter++;
    // populate the AQL packet template for GPU kernel impls
    void *ke_kernarg_region;
    // first 4 bytes store the current index of the kernel arg region
    err = hsa_amd_memory_pool_allocate(
        atl_gpu_kernarg_pool,
        sizeof(int) + max_kernarg_segment_size * MAX_NUM_KERNELS, 0,
        &ke_kernarg_region);
      ErrorCheck(Allocating memory for the executable-kernel, err);
      allow_access_to_all_gpu_agents(ke_kernarg_region);
      *reinterpret_cast<int *>(ke_kernarg_region) = 0;
      char *ke_kernargs =
          reinterpret_cast<char *>(ke_kernarg_region) + sizeof(int);

      atmi_kernel_enqueue_template_t *ke_template =
          &(reinterpret_cast<atmi_kernel_enqueue_template_t *>(
              g_ke_args.kernarg_template_ptr))[cur_kernel];
      // To be used by device code to pick a task template
      ke_template->kernel_handle = atmi_kernel->handle;

      // fill in the kernel arg regions
      ke_template->kernarg_segment_size = max_kernarg_segment_size;
      ke_template->kernarg_regions = ke_kernarg_region;

      int kernel_impl_id = 0;
      for (auto kernel_impl : kernel->impls()) {
        if (kernel_impl->devtype() == ATMI_DEVTYPE_GPU) {
          // fill in the GPU AQL template
          hsa_kernel_dispatch_packet_t *k_packet = &(ke_template->k_packet);
          k_packet->header = 0;  // ATMI_DEVTYPE_GPU;
          k_packet->kernarg_address = NULL;
          k_packet->kernel_object =
              dynamic_cast<GPUKernelImpl *>(kernel_impl)->kernel_objects_[0];
          k_packet->private_segment_size =
              dynamic_cast<GPUKernelImpl *>(kernel_impl)
                  ->private_segment_sizes_[0];
          k_packet->group_segment_size =
              dynamic_cast<GPUKernelImpl *>(kernel_impl)
                  ->group_segment_sizes_[0];
          for (int k = 0; k < MAX_NUM_KERNELS; k++) {
            atmi_implicit_args_t *impl_args =
                reinterpret_cast<atmi_implicit_args_t *>(
                    reinterpret_cast<char *>(kernel_impl->kernarg_region()) +
                    (((k + 1) * kernel_impl->kernarg_segment_size()) -
                     sizeof(atmi_implicit_args_t)));
            // fill in the queue
            impl_args->num_gpu_queues = g_ke_args.num_gpu_queues;
            impl_args->gpu_queue_ptr = (uint64_t)g_ke_args.gpu_queue_ptr;
            impl_args->num_cpu_queues = g_ke_args.num_cpu_queues;
            impl_args->cpu_queue_ptr = (uint64_t)g_ke_args.cpu_queue_ptr;
            impl_args->cpu_worker_signals =
                (uint64_t)g_ke_args.cpu_worker_signals;

            // fill in the signals?
            impl_args->kernarg_template_ptr =
                (uint64_t)g_ke_args.kernarg_template_ptr;

            // *** fill in implicit args for kernel enqueue ***
            atmi_implicit_args_t *ke_impl_args =
                reinterpret_cast<atmi_implicit_args_t *>(
                    reinterpret_cast<char *>(ke_kernargs) +
                    (((k + 1) * kernel_impl->kernarg_segment_size()) -
                     sizeof(atmi_implicit_args_t)));
            // SHARE the same pipe for printf etc
            *ke_impl_args = *impl_args;
          }
        } else if (kernel_impl->devtype() == ATMI_DEVTYPE_CPU) {
          // fill in the CPU AQL template
          hsa_agent_dispatch_packet_t *a_packet = &(ke_template->a_packet);
          a_packet->header = 0;  // ATMI_DEVTYPE_CPU;
          a_packet->type = (uint16_t)kernel_impl_id;
          /* FIXME: We are considering only void return types for now.*/
          // a_packet->return_address = NULL;
          /* Set function args */
          a_packet->arg[0] = (uint64_t)ATMI_NULL_TASK_HANDLE;
          a_packet->arg[1] = (uint64_t)NULL;
          a_packet->arg[2] =
              (uint64_t)kernel;  // pass task handle to fill in metrics
          a_packet->arg[3] = 0;  // tasks can query for current task ID
          // CPU impls dont use implicit args for now
        }
        kernel_impl_id++;
      }
  }
  return ATMI_STATUS_SUCCESS;
}

atmi_status_t DeviceRuntime::ReleaseKernel(atmi_kernel_t atmi_kernel) {
  return Runtime::ReleaseKernel(atmi_kernel);
}

}  // namespace core
