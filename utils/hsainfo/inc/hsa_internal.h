////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2014 ADVANCED MICRO DEVICES, INC.
//
// AMD is granting you permission to use this software and documentation(if any)
// (collectively, the "Materials") pursuant to the terms and conditions of the
// Software License Agreement included with the Materials.If you do not have a
// copy of the Software License Agreement, contact your AMD representative for a
// copy.
//
// You agree that you will not reverse engineer or decompile the Materials, in
// whole or in part, except as allowed by applicable law.
//
// WARRANTY DISCLAIMER : THE SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND.AMD DISCLAIMS ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY,
// INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE, NON - INFRINGEMENT, THAT THE
// SOFTWARE WILL RUN UNINTERRUPTED OR ERROR - FREE OR WARRANTIES ARISING FROM
// CUSTOM OF TRADE OR COURSE OF USAGE.THE ENTIRE RISK ASSOCIATED WITH THE USE OF
// THE SOFTWARE IS ASSUMED BY YOU.Some jurisdictions do not allow the exclusion
// of implied warranties, so the above exclusion may not apply to You.
//
// LIMITATION OF LIABILITY AND INDEMNIFICATION : AMD AND ITS LICENSORS WILL NOT,
// UNDER ANY CIRCUMSTANCES BE LIABLE TO YOU FOR ANY PUNITIVE, DIRECT,
// INCIDENTAL, INDIRECT, SPECIAL OR CONSEQUENTIAL DAMAGES ARISING FROM USE OF
// THE SOFTWARE OR THIS AGREEMENT EVEN IF AMD AND ITS LICENSORS HAVE BEEN
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.In no event shall AMD's total
// liability to You for all damages, losses, and causes of action (whether in
// contract, tort (including negligence) or otherwise) exceed the amount of $100
// USD.  You agree to defend, indemnify and hold harmless AMD and its licensors,
// and any of their directors, officers, employees, affiliates or agents from
// and against any and all loss, damage, liability and other expenses (including
// reasonable attorneys' fees), resulting from Your use of the Software or
// violation of the terms and conditions of this Agreement.
//
// U.S.GOVERNMENT RESTRICTED RIGHTS : The Materials are provided with
// "RESTRICTED RIGHTS." Use, duplication, or disclosure by the Government is
// subject to the restrictions as set forth in FAR 52.227 - 14 and DFAR252.227 -
// 7013, et seq., or its successor.Use of the Materials by the Government
// constitutes acknowledgement of AMD's proprietary rights in them.
//
// EXPORT RESTRICTIONS: The Materials may be subject to export restrictions as
//                      stated in the Software License Agreement.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef HSA_RUNTIME_CORE_INC_HSA_INTERNAL_H
#define HSA_RUNTIME_CORE_INC_HSA_INTERNAL_H

#include "hsa.h"

namespace HSA
{

  // Define core namespace interfaces - copy of function declarations in hsa.h
  hsa_status_t HSA_API hsa_init();
  hsa_status_t HSA_API hsa_shut_down();
  hsa_status_t HSA_API
    hsa_system_get_info(hsa_system_info_t attribute, void *value);
  hsa_status_t HSA_API
    hsa_system_extension_supported(uint16_t extension, uint16_t version_major,
    uint16_t version_minor, bool *result);
  hsa_status_t HSA_API
    hsa_system_get_extension_table(uint16_t extension, uint16_t version_major,
    uint16_t version_minor, void *table);
  hsa_status_t HSA_API
    hsa_iterate_agents(hsa_status_t (*callback)(hsa_agent_t agent, void *data),
    void *data);
  hsa_status_t HSA_API hsa_agent_get_info(hsa_agent_t agent,
    hsa_agent_info_t attribute,
    void *value);
  hsa_status_t HSA_API hsa_agent_get_exception_policies(hsa_agent_t agent,
    hsa_profile_t profile,
    uint16_t *mask);
  hsa_status_t HSA_API
    hsa_agent_extension_supported(uint16_t extension, hsa_agent_t agent,
    uint16_t version_major,
    uint16_t version_minor, bool *result);
  hsa_status_t HSA_API
    hsa_queue_create(hsa_agent_t agent, uint32_t size, hsa_queue_type_t type,
    void (*callback)(hsa_status_t status, hsa_queue_t *source,
    void *data),
    void *data, uint32_t private_segment_size,
    uint32_t group_segment_size, hsa_queue_t **queue);
  hsa_status_t HSA_API
    hsa_soft_queue_create(hsa_region_t region, uint32_t size,
    hsa_queue_type_t type, uint32_t features,
    hsa_signal_t completion_signal, hsa_queue_t **queue);
  hsa_status_t HSA_API hsa_queue_destroy(hsa_queue_t *queue);
  hsa_status_t HSA_API hsa_queue_inactivate(hsa_queue_t *queue);
  uint64_t HSA_API hsa_queue_load_read_index_acquire(const hsa_queue_t *queue);
  uint64_t HSA_API hsa_queue_load_read_index_relaxed(const hsa_queue_t *queue);
  uint64_t HSA_API hsa_queue_load_write_index_acquire(const hsa_queue_t *queue);
  uint64_t HSA_API hsa_queue_load_write_index_relaxed(const hsa_queue_t *queue);
  void HSA_API hsa_queue_store_write_index_relaxed(const hsa_queue_t *queue,
    uint64_t value);
  void HSA_API hsa_queue_store_write_index_release(const hsa_queue_t *queue,
    uint64_t value);
  uint64_t HSA_API hsa_queue_cas_write_index_acq_rel(const hsa_queue_t *queue,
    uint64_t expected,
    uint64_t value);
  uint64_t HSA_API hsa_queue_cas_write_index_acquire(const hsa_queue_t *queue,
    uint64_t expected,
    uint64_t value);
  uint64_t HSA_API hsa_queue_cas_write_index_relaxed(const hsa_queue_t *queue,
    uint64_t expected,
    uint64_t value);
  uint64_t HSA_API hsa_queue_cas_write_index_release(const hsa_queue_t *queue,
    uint64_t expected,
    uint64_t value);
  uint64_t HSA_API
    hsa_queue_add_write_index_acq_rel(const hsa_queue_t *queue, uint64_t value);
  uint64_t HSA_API
    hsa_queue_add_write_index_acquire(const hsa_queue_t *queue, uint64_t value);
  uint64_t HSA_API
    hsa_queue_add_write_index_relaxed(const hsa_queue_t *queue, uint64_t value);
  uint64_t HSA_API
    hsa_queue_add_write_index_release(const hsa_queue_t *queue, uint64_t value);
  void HSA_API hsa_queue_store_read_index_relaxed(const hsa_queue_t *queue,
    uint64_t value);
  void HSA_API hsa_queue_store_read_index_release(const hsa_queue_t *queue,
    uint64_t value);
  hsa_status_t HSA_API hsa_agent_iterate_regions(
    hsa_agent_t agent,
    hsa_status_t (*callback)(hsa_region_t region, void *data), void *data);
  hsa_status_t HSA_API hsa_region_get_info(hsa_region_t region,
    hsa_region_info_t attribute,
    void *value);
  hsa_status_t HSA_API hsa_memory_register(void *address, size_t size);
  hsa_status_t HSA_API hsa_memory_deregister(void *address, size_t size);
  hsa_status_t HSA_API
    hsa_memory_allocate(hsa_region_t region, size_t size, void **ptr);
  hsa_status_t HSA_API hsa_memory_free(void *ptr);
  hsa_status_t HSA_API hsa_memory_copy(void *dst, const void *src, size_t size);
  hsa_status_t HSA_API hsa_memory_assign_agent(void *ptr, hsa_agent_t agent,
    hsa_access_permission_t access);
  hsa_status_t HSA_API
    hsa_signal_create(hsa_signal_value_t initial_value, uint32_t num_consumers,
    const hsa_agent_t *consumers, hsa_signal_t *signal);
  hsa_status_t HSA_API hsa_signal_destroy(hsa_signal_t signal);
  hsa_signal_value_t HSA_API hsa_signal_load_relaxed(hsa_signal_t signal);
  hsa_signal_value_t HSA_API hsa_signal_load_acquire(hsa_signal_t signal);
  void HSA_API
    hsa_signal_store_relaxed(hsa_signal_t signal, hsa_signal_value_t value);
  void HSA_API
    hsa_signal_store_release(hsa_signal_t signal, hsa_signal_value_t value);
  hsa_signal_value_t HSA_API
    hsa_signal_wait_relaxed(hsa_signal_t signal,
    hsa_signal_condition_t condition,
    hsa_signal_value_t compare_value,
    uint64_t timeout_hint,
    hsa_wait_state_t wait_expectancy_hint);
  hsa_signal_value_t HSA_API
    hsa_signal_wait_acquire(hsa_signal_t signal,
    hsa_signal_condition_t condition,
    hsa_signal_value_t compare_value,
    uint64_t timeout_hint,
    hsa_wait_state_t wait_expectancy_hint);
  void HSA_API
    hsa_signal_and_relaxed(hsa_signal_t signal, hsa_signal_value_t value);
  void HSA_API
    hsa_signal_and_acquire(hsa_signal_t signal, hsa_signal_value_t value);
  void HSA_API
    hsa_signal_and_release(hsa_signal_t signal, hsa_signal_value_t value);
  void HSA_API
    hsa_signal_and_acq_rel(hsa_signal_t signal, hsa_signal_value_t value);
  void HSA_API
    hsa_signal_or_relaxed(hsa_signal_t signal, hsa_signal_value_t value);
  void HSA_API
    hsa_signal_or_acquire(hsa_signal_t signal, hsa_signal_value_t value);
  void HSA_API
    hsa_signal_or_release(hsa_signal_t signal, hsa_signal_value_t value);
  void HSA_API
    hsa_signal_or_acq_rel(hsa_signal_t signal, hsa_signal_value_t value);
  void HSA_API
    hsa_signal_xor_relaxed(hsa_signal_t signal, hsa_signal_value_t value);
  void HSA_API
    hsa_signal_xor_acquire(hsa_signal_t signal, hsa_signal_value_t value);
  void HSA_API
    hsa_signal_xor_release(hsa_signal_t signal, hsa_signal_value_t value);
  void HSA_API
    hsa_signal_xor_acq_rel(hsa_signal_t signal, hsa_signal_value_t value);
  void HSA_API
    hsa_signal_add_relaxed(hsa_signal_t signal, hsa_signal_value_t value);
  void HSA_API
    hsa_signal_add_acquire(hsa_signal_t signal, hsa_signal_value_t value);
  void HSA_API
    hsa_signal_add_release(hsa_signal_t signal, hsa_signal_value_t value);
  void HSA_API
    hsa_signal_add_acq_rel(hsa_signal_t signal, hsa_signal_value_t value);
  void HSA_API
    hsa_signal_subtract_relaxed(hsa_signal_t signal, hsa_signal_value_t value);
  void HSA_API
    hsa_signal_subtract_acquire(hsa_signal_t signal, hsa_signal_value_t value);
  void HSA_API
    hsa_signal_subtract_release(hsa_signal_t signal, hsa_signal_value_t value);
  void HSA_API
    hsa_signal_subtract_acq_rel(hsa_signal_t signal, hsa_signal_value_t value);
  hsa_signal_value_t HSA_API
    hsa_signal_exchange_relaxed(hsa_signal_t signal, hsa_signal_value_t value);
  hsa_signal_value_t HSA_API
    hsa_signal_exchange_acquire(hsa_signal_t signal, hsa_signal_value_t value);
  hsa_signal_value_t HSA_API
    hsa_signal_exchange_release(hsa_signal_t signal, hsa_signal_value_t value);
  hsa_signal_value_t HSA_API
    hsa_signal_exchange_acq_rel(hsa_signal_t signal, hsa_signal_value_t value);
  hsa_signal_value_t HSA_API hsa_signal_cas_relaxed(hsa_signal_t signal,
    hsa_signal_value_t expected,
    hsa_signal_value_t value);
  hsa_signal_value_t HSA_API hsa_signal_cas_acquire(hsa_signal_t signal,
    hsa_signal_value_t expected,
    hsa_signal_value_t value);
  hsa_signal_value_t HSA_API hsa_signal_cas_release(hsa_signal_t signal,
    hsa_signal_value_t expected,
    hsa_signal_value_t value);
  hsa_signal_value_t HSA_API hsa_signal_cas_acq_rel(hsa_signal_t signal,
    hsa_signal_value_t expected,
    hsa_signal_value_t value);
  hsa_status_t hsa_isa_from_name(
    const char *name,
    hsa_isa_t *isa
    );
  hsa_status_t HSA_API hsa_isa_get_info(
    hsa_isa_t isa,
    hsa_isa_info_t attribute,
    uint32_t index,
    void *value
    );
  hsa_status_t hsa_isa_compatible(
    hsa_isa_t code_object_isa,
    hsa_isa_t agent_isa,
    bool *result
    );
  hsa_status_t HSA_API hsa_code_object_serialize(
    hsa_code_object_t code_object,
    hsa_status_t (*alloc_callback)(
    size_t size, hsa_callback_data_t data, void **address
    ),
    hsa_callback_data_t callback_data,
    const char *options,
    void **serialized_code_object,
    size_t *serialized_code_object_size
    );
  hsa_status_t HSA_API hsa_code_object_deserialize(
    void *serialized_code_object,
    size_t serialized_code_object_size,
    const char *options,
    hsa_code_object_t *code_object
    );
  hsa_status_t HSA_API hsa_code_object_destroy(
    hsa_code_object_t code_object
    );
  hsa_status_t HSA_API hsa_code_object_get_info(
    hsa_code_object_t code_object,
    hsa_code_object_info_t attribute,
    void *value
    );
  hsa_status_t HSA_API hsa_code_object_get_symbol(
    hsa_code_object_t code_object,
    const char *symbol_name,
    hsa_code_symbol_t *symbol
    );
  hsa_status_t HSA_API hsa_code_symbol_get_info(
    hsa_code_symbol_t code_symbol,
    hsa_code_symbol_info_t attribute,
    void *value
    );
  hsa_status_t HSA_API hsa_code_object_iterate_symbols(
    hsa_code_object_t code_object,
    hsa_status_t (*callback)(
    hsa_code_object_t code_object, hsa_code_symbol_t symbol, void *data
    ),
    void *data
    );
  hsa_status_t HSA_API hsa_executable_create(
    hsa_profile_t profile,
    hsa_executable_state_t executable_state,
    const char *options,
    hsa_executable_t *executable
    );
  hsa_status_t HSA_API hsa_executable_destroy(
    hsa_executable_t executable
    );
  hsa_status_t HSA_API hsa_executable_load_code_object(
    hsa_executable_t executable,
    hsa_agent_t agent,
    hsa_code_object_t code_object,
    const char *options
    );
  hsa_status_t HSA_API hsa_executable_freeze(
    hsa_executable_t executable,
    const char *options
    );
  hsa_status_t HSA_API hsa_executable_get_info(
    hsa_executable_t executable,
    hsa_executable_info_t attribute,
    void *value
    );
  hsa_status_t HSA_API hsa_executable_global_variable_define(
    hsa_executable_t executable,
    const char *variable_name,
    void *address
    );
  hsa_status_t HSA_API hsa_executable_agent_global_variable_define(
    hsa_executable_t executable,
    hsa_agent_t agent,
    const char *variable_name,
    void *address
    );
  hsa_status_t HSA_API hsa_executable_readonly_variable_define(
    hsa_executable_t executable,
    hsa_agent_t agent,
    const char *variable_name,
    void *address
    );
  hsa_status_t HSA_API hsa_executable_validate(
    hsa_executable_t executable,
    uint32_t *result
    );
  hsa_status_t HSA_API hsa_executable_get_symbol(
    hsa_executable_t executable,
    const char *module_name,
    const char *symbol_name,
    hsa_agent_t agent,
    int32_t call_convention,
    hsa_executable_symbol_t *symbol
    );
  hsa_status_t HSA_API hsa_executable_symbol_get_info(
    hsa_executable_symbol_t executable_symbol,
    hsa_executable_symbol_info_t attribute,
    void *value
    );
  hsa_status_t HSA_API hsa_executable_iterate_symbols(
    hsa_executable_t executable,
    hsa_status_t (*callback)(
    hsa_executable_t executable, hsa_executable_symbol_t symbol, void *data
    ),
    void *data
    );
  hsa_status_t HSA_API
    hsa_status_string(hsa_status_t status, const char **status_string);

}

#ifdef BUILDING_HSA_CORE_RUNTIME
//This using declaration is deliberate!
//We want unqualified name resolution to fail when building the runtime.  This is a guard against accidental use of the intercept layer in the runtime.
using namespace HSA;
#endif

#endif
