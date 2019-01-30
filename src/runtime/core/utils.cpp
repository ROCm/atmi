/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/
#include "rt.h"
#include "atl_internal.h"

/*
 * Helper functions
 */
const char *get_atmi_error_string(atmi_status_t err) {
  switch(err) {
    case ATMI_STATUS_SUCCESS: return "ATMI_STATUS_SUCCESS";
    case ATMI_STATUS_ERROR: return "ATMI_STATUS_ERROR";
    default: return "";
  }
}

const char *get_error_string(hsa_status_t err) {
  switch(err) {
    case HSA_STATUS_SUCCESS: return "HSA_STATUS_SUCCESS";
    case HSA_STATUS_INFO_BREAK: return "HSA_STATUS_INFO_BREAK";
    case HSA_STATUS_ERROR: return "HSA_STATUS_ERROR";
    case HSA_STATUS_ERROR_INVALID_ARGUMENT: return "HSA_STATUS_ERROR_INVALID_ARGUMENT";
    case HSA_STATUS_ERROR_INVALID_QUEUE_CREATION: return "HSA_STATUS_ERROR_INVALID_QUEUE_CREATION";
    case HSA_STATUS_ERROR_INVALID_ALLOCATION: return "HSA_STATUS_ERROR_INVALID_ALLOCATION";
    case HSA_STATUS_ERROR_INVALID_AGENT: return "HSA_STATUS_ERROR_INVALID_AGENT";
    case HSA_STATUS_ERROR_INVALID_REGION: return "HSA_STATUS_ERROR_INVALID_REGION";
    case HSA_STATUS_ERROR_INVALID_SIGNAL: return "HSA_STATUS_ERROR_INVALID_SIGNAL";
    case HSA_STATUS_ERROR_INVALID_QUEUE: return "HSA_STATUS_ERROR_INVALID_QUEUE";
    case HSA_STATUS_ERROR_OUT_OF_RESOURCES: return "HSA_STATUS_ERROR_OUT_OF_RESOURCES";
    case HSA_STATUS_ERROR_INVALID_PACKET_FORMAT: return "HSA_STATUS_ERROR_INVALID_PACKET_FORMAT";
    case HSA_STATUS_ERROR_RESOURCE_FREE: return "HSA_STATUS_ERROR_RESOURCE_FREE";
    case HSA_STATUS_ERROR_NOT_INITIALIZED: return "HSA_STATUS_ERROR_NOT_INITIALIZED";
    case HSA_STATUS_ERROR_REFCOUNT_OVERFLOW: return "HSA_STATUS_ERROR_REFCOUNT_OVERFLOW";
    case HSA_STATUS_ERROR_INCOMPATIBLE_ARGUMENTS: return "HSA_STATUS_ERROR_INCOMPATIBLE_ARGUMENTS";
    case HSA_STATUS_ERROR_INVALID_INDEX: return "HSA_STATUS_ERROR_INVALID_INDEX";
    case HSA_STATUS_ERROR_INVALID_ISA: return "HSA_STATUS_ERROR_INVALID_ISA";
    case HSA_STATUS_ERROR_INVALID_ISA_NAME: return "HSA_STATUS_ERROR_INVALID_ISA_NAME";
    case HSA_STATUS_ERROR_INVALID_CODE_OBJECT: return "HSA_STATUS_ERROR_INVALID_CODE_OBJECT";
    case HSA_STATUS_ERROR_INVALID_EXECUTABLE: return "HSA_STATUS_ERROR_INVALID_EXECUTABLE";
    case HSA_STATUS_ERROR_FROZEN_EXECUTABLE: return "HSA_STATUS_ERROR_FROZEN_EXECUTABLE";
    case HSA_STATUS_ERROR_INVALID_SYMBOL_NAME: return "HSA_STATUS_ERROR_INVALID_SYMBOL_NAME";
    case HSA_STATUS_ERROR_VARIABLE_ALREADY_DEFINED: return "HSA_STATUS_ERROR_VARIABLE_ALREADY_DEFINED";
    case HSA_STATUS_ERROR_VARIABLE_UNDEFINED: return "HSA_STATUS_ERROR_VARIABLE_UNDEFINED";
    case HSA_STATUS_ERROR_EXCEPTION: return "HSA_STATUS_ERROR_EXCEPTION";
  }
}

namespace core {
  /*
   * Environment variables
   */
  void Environment::GetEnvAll() {
    std::string var = GetEnv("ATMI_DEPENDENCY_SYNC_TYPE");
    // default dependency type: callback; switch the if-else checks to change the defaults
    if(var.empty() || var == "ATMI_SYNC_CALLBACK") {
      dep_sync_type_ = ATL_SYNC_CALLBACK;
    }
    else if(var == "ATMI_SYNC_BARRIER_PKT") {
      dep_sync_type_ = ATL_SYNC_BARRIER_PKT;
    }

    var = GetEnv("ATMI_MAX_HSA_SIGNALS");
    if(!var.empty()) max_signals_ = std::stoi(var);

    /* TODO: If we get a good use case for device-specific worker count, we
     * should explore it, but let us keep the worker count uniform for all
     * devices of a type until that time
     */
    var = GetEnv("ATMI_DEVICE_GPU_WORKERS");
    if(!var.empty()) num_gpu_queues_ = std::stoi(var);

    /* TODO: If we get a good use case for device-specific worker count, we
     * should explore it, but let us keep the worker count uniform for all
     * devices of a type until that time
     */
    var = GetEnv("ATMI_DEVICE_CPU_WORKERS");
    if(!var.empty()) num_cpu_queues_ = std::stoi(var);

    var = GetEnv("ATMI_DEBUG");
    if(!var.empty()) debug_mode_ = std::stoi(var);

    var = GetEnv("ATMI_PROFILE");
    if(!var.empty()) profile_mode_ = std::stoi(var);
  }
} // namespace core
