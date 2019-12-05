/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/
#ifndef SRC_RUNTIME_INCLUDE_RT_H_
#define SRC_RUNTIME_INCLUDE_RT_H_

#include <atmi_runtime.h>
#include <hsa.h>
#include <cstdarg>
#include <string>

namespace core {

class Environment {
 public:
  Environment()
      : max_signals_(24),
        max_kernel_types_(32),
        num_gpu_queues_(-1),
        num_cpu_queues_(-1),
        debug_mode_(0),
        profile_mode_(0) {
    GetEnvAll();
  }

  virtual ~Environment() {}

  void GetEnvAll();

  int getDepSyncType() const { return dep_sync_type_; }
  int getMaxSignals() const { return max_signals_; }
  int getMaxKernelTypes() const { return max_kernel_types_; }
  int getNumGPUQueues() const { return num_gpu_queues_; }
  int getNumCPUQueues() const { return num_cpu_queues_; }
  // TODO(ashwinma): int may change to enum if we have more debug modes
  int getDebugMode() const { return debug_mode_; }
  // TODO(ashwinma): int may change to enum if we have more profile modes
  int getProfileMode() const { return profile_mode_; }

 private:
  std::string GetEnv(const char *name) {
    char *env = getenv(name);
    std::string ret;
    if (env) {
      ret = env;
    }
    return ret;
  }

  int dep_sync_type_;
  int max_signals_;
  int max_kernel_types_;
  int num_gpu_queues_;
  int num_cpu_queues_;
  int debug_mode_;
  int profile_mode_;
};

class Runtime {
 public:
  static Runtime &getInstance() {
    static Runtime instance;
    return instance;
  }

  // init/finalize
  virtual atmi_status_t Initialize(atmi_devtype_t);
  virtual atmi_status_t Finalize();
  // machine info
  atmi_machine_t *GetMachineInfo();
  // modules
  atmi_status_t RegisterModuleFromMemory(void **, size_t *,
                                         atmi_platform_type_t *, const int);
  atmi_status_t RegisterModule(const char **, atmi_platform_type_t *,
                               const int);
  // kernels
  virtual atmi_status_t CreateKernel(atmi_kernel_t *, const int, const size_t *,
                             const int, va_list);
  virtual atmi_status_t ReleaseKernel(atmi_kernel_t);
  atmi_status_t CreateEmptyKernel(atmi_kernel_t *, const int, const size_t *);
  atmi_status_t AddGPUKernelImpl(atmi_kernel_t, const char *,
                                 const unsigned int);
  atmi_status_t AddCPUKernelImpl(atmi_kernel_t, atmi_generic_fp,
                                 const unsigned int);
  // sync
  atmi_status_t TaskGroupSync(atmi_taskgroup_handle_t);
  atmi_status_t TaskWait(atmi_task_handle_t);
  // print buffers/pipes
  atmi_status_t RegisterTaskInitBuffer(task_process_init_buffer_t);
  atmi_status_t RegisterTaskFiniBuffer(task_process_fini_buffer_t);
  // tasks
  atmi_task_handle_t CreateTaskTemplate(atmi_kernel_t);
  atmi_task_handle_t ActivateTaskTemplate(atmi_task_handle_t, atmi_lparm_t *,
                                          void **);
  atmi_task_handle_t CreateTask(atmi_lparm_t *, atmi_kernel_t, void **);
  atmi_task_handle_t ActivateTask(atmi_task_handle_t);
  atmi_task_handle_t LaunchTask(atmi_lparm_t *, atmi_kernel_t, void **);
  // taskgroups
  atmi_status_t TaskGroupCreate(atmi_taskgroup_handle_t *, bool ordered = false,
                                atmi_place_t place = ATMI_DEFAULT_PLACE);
  atmi_status_t TaskGroupRelease(atmi_taskgroup_handle_t);
  // data
  atmi_status_t Memcpy(void *, const void *, size_t);
  atmi_task_handle_t MemcpyAsync(atmi_cparm_t *, void *, const void *, size_t);
  atmi_status_t Memfree(void *);
  atmi_status_t Malloc(void **, size_t, atmi_mem_place_t);

  // environment variables
  const Environment &getEnvironment() const { return env_; }
  int getDepSyncType() const { return env_.getDepSyncType(); }
  int getMaxSignals() const { return env_.getMaxSignals(); }
  int getMaxKernelTypes() const { return env_.getMaxKernelTypes(); }
  int getNumGPUQueues() const { return env_.getNumGPUQueues(); }
  int getNumCPUQueues() const { return env_.getNumCPUQueues(); }
  // TODO(ashwinma): int may change to enum if we have more debug modes
  int getDebugMode() const { return env_.getDebugMode(); }
  // TODO(ashwinma): int may change to enum if we have more profile modes
  int getProfileMode() const { return env_.getProfileMode(); }

  #if 0
  bool initialized() const { return initialized_; }
  void set_initialized(const bool val) { initialized_ = val; }

  bool hsa_initialized() const { return hsa_initialized_; }
  void set_hsa_initialized(const bool val) { hsa_initialized_ = val; }

  bool cpu_initialized() const { return cpu_initialized_; }
  void set_cpu_initialized(const bool val) { cpu_initialized_ = val; }

  bool gpu_initialized() const { return gpu_initialized_; }
  void set_gpu_initialized(const bool val) { gpu_initialized_ = val; }

  bool tasks_initialized() const { return tasks_initialized_; }
  void set_tasks_initialized(const bool val) { tasks_initialized_ = val; }
#endif
 protected:
  Runtime() = default;
  ~Runtime() = default;
  Runtime(const Runtime &) = delete;
  Runtime &operator=(const Runtime &) = delete;
  //bool initialized_;
  //bool hsa_initialized_;
  //bool cpu_initialized_;
  //bool gpu_initialized_;
  //bool tasks_initialized_;

 protected:
  // variable to track environment variables
  Environment env_;
};

}  // namespace core

#endif  // SRC_RUNTIME_INCLUDE_RT_H_
