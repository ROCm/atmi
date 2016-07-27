#ifndef HSA_RUNTIME_CORE_ISA_HPP_
#define HSA_RUNTIME_CORE_ISA_HPP_

#include <cstdint>
#include <ostream>
#include <string>
#include "compute_capability.hpp"
#include "hsa_internal.h"

#define ISA_NAME_AMD_VENDOR      "AMD"
#define ISA_NAME_AMD_DEVICE      "AMDGPU"
#define ISA_NAME_AMD_TOKEN_COUNT 5

namespace core {

//===----------------------------------------------------------------------===//
// Isa.                                                                       //
//===----------------------------------------------------------------------===//

class Isa final {
public:
  const std::string& full_name() const {
    return full_name_;
  }
  const std::string& vendor() const {
    return vendor_;
  }
  const std::string& device() const {
    return device_;
  }
  const ComputeCapability& compute_capability() const {
    return compute_capability_;
  }

  static hsa_status_t Create(
    const hsa_agent_t &in_agent,
    hsa_isa_t *out_isa_handle
  );

  static hsa_status_t Create(
    const char *in_isa_name,
    hsa_isa_t *out_isa_handle
  );

  static hsa_isa_t Handle(const Isa *in_isa_object);

  static Isa* Object(const hsa_isa_t &in_isa_handle);

  Isa():
    full_name_(""),
    vendor_(""),
    device_("") {}

  ~Isa() {}

  hsa_status_t Initialize(const hsa_agent_t &in_agent);

  hsa_status_t Initialize(const char *in_isa_name);

  void Reset();

  hsa_status_t GetInfo(
    const hsa_isa_info_t &in_isa_attribute,
    const uint32_t &in_call_convention_index,
    void *out_value
  ) const;

  hsa_status_t IsCompatible(
    const Isa &in_isa_object,
    bool *out_result
  ) const;

  bool IsValid();

  friend std::ostream& operator<<(
    std::ostream &out_stream,
    const Isa &in_isa
  );

private:
  std::string full_name_;
  std::string vendor_;
  std::string device_;
  ComputeCapability compute_capability_;
}; // class Isa

} // namespace core

#endif // HSA_RUNTIME_CORE_ISA_HPP_
