/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/

#include "kernel.h"
#include "ATLMachine.h"
#include "atl_internal.h"

extern hsa_amd_memory_pool_t atl_gpu_kernarg_pool;
extern atl_kernel_enqueue_args_t g_ke_args;
extern std::map<uint64_t, core::Kernel *> KernelImplMap;

namespace core {
void allow_access_to_all_gpu_agents(void *ptr);
atmi_status_t Runtime::CreateEmptyKernel(atmi_kernel_t *atmi_kernel,
                                         const int num_args,
                                         const size_t *arg_sizes) {
  static uint64_t counter = 0;
  uint64_t k_id = ++counter;
  atmi_kernel->handle = (uint64_t)k_id;
  Kernel *kernel = new Kernel(k_id, num_args, arg_sizes);
  KernelImplMap[k_id] = kernel;
  return ATMI_STATUS_SUCCESS;
}

atmi_status_t Runtime::ReleaseKernel(atmi_kernel_t atmi_kernel) {
  uint64_t k_id = atmi_kernel.handle;
  delete KernelImplMap[k_id];
  KernelImplMap.erase(k_id);
  return ATMI_STATUS_SUCCESS;
}

atmi_status_t Runtime::CreateKernel(atmi_kernel_t *atmi_kernel,
                                    const int num_args, const size_t *arg_sizes,
                                    const int num_impls, va_list arguments) {
  atmi_status_t status;
  hsa_status_t err;
  if (!atl_is_atmi_initialized()) return ATMI_STATUS_ERROR;
  status = atmi_kernel_create_empty(atmi_kernel, num_args, arg_sizes);
  ATMIErrorCheck(Creating kernel object, status);

  static int counter = 0;
  bool has_gpu_impl = false;
  uint64_t k_id = atmi_kernel->handle;
  Kernel *kernel = KernelImplMap[k_id];
  size_t max_kernarg_segment_size = 0;
  // va_list arguments;
  // va_start(arguments, num_impls);
  for (int impl_id = 0; impl_id < num_impls; impl_id++) {
    atmi_devtype_t devtype = (atmi_devtype_t)va_arg(arguments, int);
    if (devtype == ATMI_DEVTYPE_GPU) {
      const char *impl = va_arg(arguments, const char *);
      status = atmi_kernel_add_gpu_impl(*atmi_kernel, impl, impl_id);
      ATMIErrorCheck(Adding GPU kernel implementation, status);
      DEBUG_PRINT("GPU kernel %s added [%u]\n", impl, impl_id);
      has_gpu_impl = true;
    } else if (devtype == ATMI_DEVTYPE_CPU) {
      atmi_generic_fp impl = va_arg(arguments, atmi_generic_fp);
      status = atmi_kernel_add_cpu_impl(*atmi_kernel, impl, impl_id);
      ATMIErrorCheck(Adding CPU kernel implementation, status);
      DEBUG_PRINT("CPU kernel %p added [%u]\n", impl, impl_id);
    } else {
      fprintf(stderr, "Unsupported device type: %d\n", devtype);
      return ATMI_STATUS_ERROR;
    }
    size_t this_kernarg_segment_size =
        kernel->impls()[impl_id]->kernarg_segment_size();
    if (this_kernarg_segment_size > max_kernarg_segment_size)
      max_kernarg_segment_size = this_kernarg_segment_size;
    ATMIErrorCheck(Creating kernel implementations, status);
    // rest of kernel impl fields will be populated at first kernel launch
  }
  // va_end(arguments);
  //// FIXME EEEEEE: for EVERY GPU impl, add all CPU/GPU implementations in
  // their templates!!!
  if (has_gpu_impl) {
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

      // fill in the kernel template AQL packets
      int cur_kernel = g_ke_args.kernel_counter++;
      // FIXME: reformulate this for debug mode
      // assert(cur_kernel < MAX_NUM_KERNEL_TYPES);
      if (cur_kernel >= MAX_NUM_KERNEL_TYPES) return ATMI_STATUS_ERROR;
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

atmi_status_t Runtime::AddGPUKernelImpl(atmi_kernel_t atmi_kernel,
                                        const char *impl,
                                        const unsigned int ID) {
  if (!atl_is_atmi_initialized() || KernelInfoTable.empty())
    return ATMI_STATUS_ERROR;
  uint64_t k_id = atmi_kernel.handle;
  Kernel *kernel = KernelImplMap[k_id];
  if (kernel->id_map().find(ID) != kernel->id_map().end()) {
    fprintf(stderr, "Kernel ID %d already found\n", ID);
    return ATMI_STATUS_ERROR;
  }
  std::vector<ATLGPUProcessor> &gpu_procs =
      g_atl_machine.processors<ATLGPUProcessor>();
  int gpu_count = gpu_procs.size();

  std::string hsaco_name = std::string(impl);
  std::string kernel_name;
  atmi_platform_type_t kernel_type;
  bool some_success = false;
  for (int gpu = 0; gpu < gpu_count; gpu++) {
    if (KernelInfoTable[gpu].find(hsaco_name) != KernelInfoTable[gpu].end()) {
      DEBUG_PRINT("Found kernel %s for GPU %d\n", hsaco_name.c_str(), gpu);
      kernel_name = hsaco_name;
      kernel_type = AMDGCN;
      some_success = true;
    } else {
      DEBUG_PRINT("Did NOT find kernel %s for GPU %d\n", hsaco_name.c_str(),
                  gpu);
      continue;
    }
  }
  if (!some_success) return ATMI_STATUS_ERROR;

  KernelImpl *kernel_impl =
      new GPUKernelImpl(ID, kernel_name, kernel_type, *kernel);
  // KernelImpl* kernel_impl = kernel->createGPUKernelImpl(ID, kernel_name,
  // kernel_type);

  kernel->id_map()[ID] = kernel->impls().size();

  kernel->impls().push_back(kernel_impl);
  // rest of kernel impl fields will be populated at first kernel launch
  return ATMI_STATUS_SUCCESS;
}

atmi_status_t Runtime::AddCPUKernelImpl(atmi_kernel_t atmi_kernel,
                                        atmi_generic_fp impl,
                                        const unsigned int ID) {
  if (!atl_is_atmi_initialized()) return ATMI_STATUS_ERROR;
  static int counter = 0;
  uint64_t k_id = atmi_kernel.handle;
  std::string fn_name("_x86_");
  fn_name += std::to_string(counter);
  fn_name += std::string("_");
  fn_name += std::to_string(k_id);
  counter++;

  Kernel *kernel = KernelImplMap[k_id];
  if (kernel->id_map().find(ID) != kernel->id_map().end()) {
    fprintf(stderr, "Kernel ID %d already found\n", ID);
    return ATMI_STATUS_ERROR;
  }

  KernelImpl *kernel_impl = new CPUKernelImpl(ID, fn_name, X86, impl, *kernel);
  // KernelImpl* kernel_impl = kernel->createCPUKernelImpl(ID, kernel_name,
  // kernel_type);

  kernel->id_map()[ID] = kernel->impls().size();
  kernel->impls().push_back(kernel_impl);
  // rest of kernel impl fields will be populated at first kernel launch
  return ATMI_STATUS_SUCCESS;
}

KernelImpl::KernelImpl(unsigned int id, const std::string &name,
                       atmi_platform_type_t platform_type,
                       const Kernel &kernel)
    : id_(id), name_(name), platform_type_(platform_type), kernel_(kernel) {}

GPUKernelImpl::GPUKernelImpl(unsigned int id, const std::string &name,
                             atmi_platform_type_t platform_type,
                             const Kernel &kernel)
    : KernelImpl(id, name, platform_type, kernel) {
  std::vector<ATLGPUProcessor> &gpu_procs =
      g_atl_machine.processors<ATLGPUProcessor>();
  int gpu_count = gpu_procs.size();

  kernel_objects_.reserve(gpu_count);
  group_segment_sizes_.reserve(gpu_count);
  private_segment_sizes_.reserve(gpu_count);
  int max_kernarg_segment_size = 0;
  for (int gpu = 0; gpu < gpu_count; gpu++) {
    atl_kernel_info_t info = KernelInfoTable[gpu][name_];
    kernel_objects_[gpu] = info.kernel_object;
    group_segment_sizes_[gpu] = info.group_segment_size;
    private_segment_sizes_[gpu] = info.private_segment_size;
    if (max_kernarg_segment_size < info.kernel_segment_size)
      max_kernarg_segment_size = info.kernel_segment_size;
  }
  kernarg_segment_size_ = max_kernarg_segment_size;

  arg_offsets_.reserve(kernel.num_args());
  atl_kernel_info_t any_gpu_info = KernelInfoTable[0][name_];
  for (int i = 0; i < kernel.num_args(); i++) {
    arg_offsets_[i] = any_gpu_info.arg_offsets[i];
  }

  /* create kernarg memory */
  kernarg_region_ = NULL;
#ifdef MEMORY_REGION
  hsa_status_t err = hsa_memory_allocate(
      atl_gpu_kernarg_region, kernarg_segment_size_ * MAX_NUM_KERNELS,
      &kernarg_region_);
    ErrorCheck(Allocating memory for the executable-kernel, err);
#else
  if (kernarg_segment_size_ > 0) {
    DEBUG_PRINT("New kernarg segment size: %u\n", kernarg_segment_size_);
    hsa_status_t err = hsa_amd_memory_pool_allocate(
        atl_gpu_kernarg_pool, kernarg_segment_size_ * MAX_NUM_KERNELS, 0,
        &kernarg_region_);
      ErrorCheck(Allocating memory for the executable-kernel, err);
      allow_access_to_all_gpu_agents(kernarg_region_);

      void *pipe_ptrs;
      // allocate pipe memory in the kernarg memory pool
      // TODO(ashwinma): may be possible to allocate this on device specific
      // memory but data movement will have to be done later by
      // post-processing kernel on destination agent.
      err = hsa_amd_memory_pool_allocate(
          atl_gpu_kernarg_pool, MAX_PIPE_SIZE * MAX_NUM_KERNELS, 0, &pipe_ptrs);
      ErrorCheck(Allocating pipe memory region, err);
      DEBUG_PRINT("Allocating pipe ptr: %p\n", pipe_ptrs);
      allow_access_to_all_gpu_agents(pipe_ptrs);

      for (int k = 0; k < MAX_NUM_KERNELS; k++) {
        atmi_implicit_args_t *impl_args =
            reinterpret_cast<atmi_implicit_args_t *>(
                reinterpret_cast<char *>(kernarg_region_) +
                (((k + 1) * kernarg_segment_size_) -
                 sizeof(atmi_implicit_args_t)));
        impl_args->pipe_ptr = (uint64_t)(reinterpret_cast<char *>(pipe_ptrs) +
                                         (k * MAX_PIPE_SIZE));
        impl_args->offset_x = 0;
        impl_args->offset_y = 0;
        impl_args->offset_z = 0;
      }
  }

#endif
    for (int i = 0; i < MAX_NUM_KERNELS; i++) {
      free_kernarg_segments_.push(i);
    }
    pthread_mutex_init(&mutex_, NULL);
}

CPUKernelImpl::CPUKernelImpl(unsigned int id, const std::string &name,
                             atmi_platform_type_t platform_type,
                             atmi_generic_fp function, const Kernel &kernel)
    : KernelImpl(id, name, platform_type, kernel), function_(function) {
  /* create kernarg memory */
  uint32_t kernarg_size = 0;
  // extract arg offsets out and pass as arg to KernelImpl constructor or
  // builder fn?
  for (int i = 0; i < kernel.num_args(); i++) {
    arg_offsets_.push_back(kernarg_size);
    kernarg_size += kernel.arg_sizes()[i];
  }
  kernarg_segment_size_ = kernarg_size;
  kernarg_region_ = NULL;
  if (kernarg_size)
    kernarg_region_ = malloc(kernarg_segment_size_ * MAX_NUM_KERNELS);
  for (int i = 0; i < MAX_NUM_KERNELS; i++) {
    free_kernarg_segments_.push(i);
  }

  pthread_mutex_init(&mutex_, NULL);
}

KernelImpl::~KernelImpl() {
  arg_offsets_.clear();
  clear_container(&free_kernarg_segments_);
}

GPUKernelImpl::~GPUKernelImpl() {
  lock(&mutex_);
  // free the pipe_ptrs data
  // We create the pipe_ptrs region for all kernel instances
  // combined, and each instance of the kernel
  // invocation takes a piece of it. So, the first kernel instance
  // (k=0) will have the pointer to the entire pipe region itself.
  atmi_implicit_args_t *impl_args = reinterpret_cast<atmi_implicit_args_t *>(
      reinterpret_cast<char *>(kernarg_region_) + kernarg_segment_size_ -
      sizeof(atmi_implicit_args_t));
  void *pipe_ptrs = reinterpret_cast<void *>(impl_args->pipe_ptr);
  DEBUG_PRINT("Freeing pipe ptr: %p\n", pipe_ptrs);
  hsa_memory_free(pipe_ptrs);
  hsa_memory_free(kernarg_region_);
  kernel_objects_.clear();
  group_segment_sizes_.clear();
  private_segment_sizes_.clear();
  unlock(&mutex_);
}

CPUKernelImpl::~CPUKernelImpl() {
  lock(&mutex_);
  free(kernarg_region_);
  unlock(&mutex_);
}

bool Kernel::isValidId(unsigned int kernel_id) {
  std::map<unsigned int, unsigned int>::iterator it = id_map_.find(kernel_id);
  if (it == id_map_.end()) {
    fprintf(stderr, "ERROR: Kernel not found\n");
    return false;
  }
  int idx = it->second;
  if (idx >= impls_.size()) {
    fprintf(stderr, "Kernel ID %d out of bounds (%lu)\n", kernel_id,
            impls_.size());
    return false;
  }
  return true;
}

int Kernel::getKernelIdMapIndex(unsigned int kernel_id) {
  if (!isValidId(kernel_id)) {
    return -1;
  }
  return id_map_[kernel_id];
}

KernelImpl *Kernel::getKernelImpl(unsigned int kernel_id) {
  int idx = getKernelIdMapIndex(kernel_id);
  if (idx < 0) {
    fprintf(stderr, "Incorrect Kernel ID %d\n", kernel_id);
    return NULL;
  }
  return impls_[idx];
}

Kernel::Kernel(uint64_t id, const int num_args, const size_t *arg_sizes)
    : id_(id), num_args_(num_args) {
  id_map_.clear();
  arg_sizes_.clear();
  impls_.clear();
  for (int i = 0; i < num_args; i++) {
    arg_sizes_.push_back(arg_sizes[i]);
  }
}

Kernel::~Kernel() {
  for (auto kernel_impl : impls_) {
    delete kernel_impl;
  }
  impls_.clear();
  arg_sizes_.clear();
  id_map_.clear();
}
}  // namespace core
