/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/
#ifndef __RT_H__
#define __RT_H__

#include <hsa.h>
#include <atmi_runtime.h>
#include <cstdarg>
#include <string>

namespace core {
  class Environment {
    public:
      explicit Environment() :
        max_signals_(24),
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
      int getNumGPUQueues() const { return num_gpu_queues_; }
      int getNumCPUQueues() const { return num_cpu_queues_; }
      // TODO: int may change to enum if we have more debug modes
      int getDebugMode() const { return debug_mode_; }
      // TODO: int may change to enum if we have more profile modes
      int getProfileMode() const { return profile_mode_; }

    private:
      std::string GetEnv(const char *name) {
        char *env = getenv(name);
        std::string ret;
        if(env) {
          ret = env;
        }
        return ret;
      }

      int dep_sync_type_;
      int max_signals_;
      int num_gpu_queues_;
      int num_cpu_queues_;
      int debug_mode_;
      int profile_mode_;
  };

  class Runtime{
    public:
      static Runtime& getInstance(){
        static Runtime instance;
        return instance;
      }

      // init/finalize
      atmi_status_t Initialize(atmi_devtype_t);
      atmi_status_t Finalize();
      // machine info
      atmi_machine_t* GetMachineInfo();
      // modules
      atmi_status_t RegisterModuleFromMemory(void**, size_t*, atmi_platform_type_t*, const int);
      atmi_status_t RegisterModule(const char**, atmi_platform_type_t*, const int);
      // kernels
      atmi_status_t CreateKernel(atmi_kernel_t*, const int, const size_t*, const int, va_list);
      atmi_status_t ReleaseKernel(atmi_kernel_t);
      atmi_status_t CreateEmptyKernel(atmi_kernel_t*, const int, const size_t*);
      atmi_status_t AddGPUKernelImpl(atmi_kernel_t, const char*, const unsigned int);
      atmi_status_t AddCPUKernelImpl(atmi_kernel_t, atmi_generic_fp, const unsigned int);
      // sync
      atmi_status_t TaskGroupSync(atmi_taskgroup_handle_t);
      atmi_status_t TaskWait(atmi_task_handle_t);
      // print buffers/pipes
      atmi_status_t RegisterTaskInitBuffer(task_process_init_buffer_t);
      atmi_status_t RegisterTaskFiniBuffer(task_process_fini_buffer_t);
      // tasks
      atmi_task_handle_t CreateTaskTemplate(atmi_kernel_t);
      atmi_task_handle_t ActivateTaskTemplate(atmi_task_handle_t, atmi_lparm_t*, void**);
      atmi_task_handle_t CreateTask(atmi_lparm_t*, atmi_kernel_t, void**);
      atmi_task_handle_t ActivateTask(atmi_task_handle_t);
      atmi_task_handle_t LaunchTask(atmi_lparm_t*, atmi_kernel_t, void**);
      // taskgroups
      atmi_status_t TaskGroupCreate(atmi_taskgroup_handle_t *,
                                    bool ordered = false,
                                    atmi_place_t place = ATMI_DEFAULT_PLACE);
      atmi_status_t TaskGroupRelease(atmi_taskgroup_handle_t);
      // data
      atmi_status_t Memcpy(void *, const void *, size_t);
      atmi_task_handle_t MemcpyAsync(atmi_cparm_t *, void *, const void *, size_t);
      atmi_status_t Memfree(void *);
      atmi_status_t Malloc(void**, size_t, atmi_mem_place_t);

      // environment variables
      const Environment& getEnvironment() const { return env_; }
      int getDepSyncType() const { return env_.getDepSyncType(); }
      int getMaxSignals() const { return env_.getMaxSignals(); }
      int getNumGPUQueues() const { return env_.getNumGPUQueues(); }
      int getNumCPUQueues() const { return env_.getNumCPUQueues(); }
      // TODO: int may change to enum if we have more debug modes
      int getDebugMode() const { return env_.getDebugMode(); }
      // TODO: int may change to enum if we have more profile modes
      int getProfileMode() const { return env_.getProfileMode(); }

    private:
      Runtime()= default;
      ~Runtime()= default;
      Runtime(const Runtime&)= delete;
      Runtime& operator=(const Runtime&)= delete;

      // variable to track environment variables
      Environment env_;
  };
} // namespace core

#endif // __RT_H__
