/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/
#ifndef SRC_DEVICE_RUNTIME_DEVICE_RT_H_
#define SRC_DEVICE_RUNTIME_DEVICE_RT_H_

#include "rt.h"

namespace core {

class DeviceRuntime : public Runtime {
 public:
  static DeviceRuntime &getInstance() {
    static DeviceRuntime instance;
    return instance;
  }

  // init/finalize
  virtual atmi_status_t Initialize(atmi_devtype_t);
  virtual atmi_status_t Finalize();
  // kernels
  virtual atmi_status_t CreateKernel(atmi_kernel_t *, const int, const size_t *,
                                     const int, va_list);
  virtual atmi_status_t ReleaseKernel(atmi_kernel_t);

  // bool initialized() const { return initialized_; }
  // void set_initialized(const bool val) { initialized_ = val; }
 private:
  DeviceRuntime() = default;
  ~DeviceRuntime() = default;
  DeviceRuntime(const DeviceRuntime &) = delete;
  DeviceRuntime &operator=(const DeviceRuntime &) = delete;
  // bool initialized_;
};

}  // namespace core

#endif  // SRC_DEVICE_RUNTIME_DEVICE_RT_H_
