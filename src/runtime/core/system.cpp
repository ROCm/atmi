/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/
#include "atl_internal.h"
#include "rt.h"
#include "ATLMachine.h"
#include <cstdarg>
#include <cassert>
#include <iostream>
#include <fstream>
#include <set>
#include <string>
#include <iomanip>

#include "amd_comgr.h"
#include <gelf.h>
#include <libelf.h>

typedef unsigned char* address;
/*
 * Note descriptors.
 */
typedef struct {
  uint32_t    n_namesz;    /* Length of note's name. */
  uint32_t    n_descsz;    /* Length of note's value. */
  uint32_t    n_type;      /* Type of note. */
} Elf_Note;

#include "llvm/Support/AMDGPUMetadata.h"

typedef llvm::AMDGPU::HSAMD::Metadata CodeObjectMD;
typedef llvm::AMDGPU::HSAMD::Kernel::Metadata KernelMD;
typedef llvm::AMDGPU::HSAMD::Kernel::Arg::Metadata KernelArgMD;

using llvm::AMDGPU::HSAMD::AccessQualifier;
using llvm::AMDGPU::HSAMD::AddressSpaceQualifier;
using llvm::AMDGPU::HSAMD::ValueKind;
using llvm::AMDGPU::HSAMD::ValueType;

enum class ArgField : uint8_t {
  Name          = 0,
  TypeName      = 1,
  Size          = 2,
  Align         = 3,
  ValueKind     = 4,
  ValueType     = 5,
  PointeeAlign  = 6,
  AddrSpaceQual = 7,
  AccQual       = 8,
  ActualAccQual = 9,
  IsConst       = 10,
  IsRestrict    = 11,
  IsVolatile    = 12,
  IsPipe        = 13,
  Offset        = 14
};

static const std::map<std::string,ArgField> ArgFieldMap =
{
  // v2
  {"Name",          ArgField::Name},
  {"TypeName",      ArgField::TypeName},
  {"Size",          ArgField::Size},
  {"Align",         ArgField::Align},
  {"ValueKind",     ArgField::ValueKind},
  {"ValueType",     ArgField::ValueType},
  {"PointeeAlign",  ArgField::PointeeAlign},
  {"AddrSpaceQual", ArgField::AddrSpaceQual},
  {"AccQual",       ArgField::AccQual},
  {"ActualAccQual", ArgField::ActualAccQual},
  {"IsConst",       ArgField::IsConst},
  {"IsRestrict",    ArgField::IsRestrict},
  {"IsVolatile",    ArgField::IsVolatile},
  {"IsPipe",        ArgField::IsPipe},
  // v3
  {".type_name",    ArgField::TypeName},
  {".value_kind",   ArgField::ValueKind},
  {".address_space",ArgField::AddrSpaceQual},
  {".is_const",     ArgField::IsConst},
  {".offset",       ArgField::Offset},
  {".size",         ArgField::Size},
  {".value_type",   ArgField::ValueType},
  {".name",         ArgField::Name}
};

static const std::map<std::string,ValueKind> ArgValueKind =
{
  {"ByValue",                 ValueKind::ByValue},
  {"GlobalBuffer",            ValueKind::GlobalBuffer},
  {"DynamicSharedPointer",    ValueKind::DynamicSharedPointer},
  {"Sampler",                 ValueKind::Sampler},
  {"Image",                   ValueKind::Image},
  {"Pipe",                    ValueKind::Pipe},
  {"Queue",                   ValueKind::Queue},
  {"HiddenGlobalOffsetX",     ValueKind::HiddenGlobalOffsetX},
  {"HiddenGlobalOffsetY",     ValueKind::HiddenGlobalOffsetY},
  {"HiddenGlobalOffsetZ",     ValueKind::HiddenGlobalOffsetZ},
  {"HiddenNone",              ValueKind::HiddenNone},
  {"HiddenPrintfBuffer",      ValueKind::HiddenPrintfBuffer},
  {"HiddenDefaultQueue",      ValueKind::HiddenDefaultQueue},
  {"HiddenCompletionAction",  ValueKind::HiddenCompletionAction},
  // {"HiddenMultiGridSyncArg",  ValueKind::HiddenMultiGridSyncArg},
  // v3
  {"by_value",                  ValueKind::ByValue},
  {"global_buffer",             ValueKind::GlobalBuffer},
  {"dynamic_shared_pointer",    ValueKind::DynamicSharedPointer},
  {"sampler",                   ValueKind::Sampler},
  {"image",                     ValueKind::Image},
  {"pipe",                      ValueKind::Pipe},
  {"queue",                     ValueKind::Queue},
  {"hidden_global_offset_x",    ValueKind::HiddenGlobalOffsetX},
  {"hidden_global_offset_y",    ValueKind::HiddenGlobalOffsetY},
  {"hidden_global_offset_z",    ValueKind::HiddenGlobalOffsetZ},
  {"hidden_none",               ValueKind::HiddenNone},
  {"hidden_printf_buffer",      ValueKind::HiddenPrintfBuffer},
  {"hidden_default_queue",      ValueKind::HiddenDefaultQueue},
  {"hidden_completion_action",  ValueKind::HiddenCompletionAction}
  //,{"hidden_multigrid_sync_arg", ValueKind::HiddenMultiGridSyncArg}
};

enum class CodePropField : uint8_t {
  KernargSegmentSize      = 0,
  GroupSegmentFixedSize   = 1,
  PrivateSegmentFixedSize = 2,
  KernargSegmentAlign     = 3,
  WavefrontSize           = 4,
  NumSGPRs                = 5,
  NumVGPRs                = 6,
  MaxFlatWorkGroupSize    = 7,
  IsDynamicCallStack      = 8,
  IsXNACKEnabled          = 9,
  NumSpilledSGPRs         = 10,
  NumSpilledVGPRs         = 11
};

static const std::map<std::string,CodePropField> CodePropFieldMap =
{
  {"KernargSegmentSize",      CodePropField::KernargSegmentSize},
  {"GroupSegmentFixedSize",   CodePropField::GroupSegmentFixedSize},
  {"PrivateSegmentFixedSize", CodePropField::PrivateSegmentFixedSize},
  {"KernargSegmentAlign",     CodePropField::KernargSegmentAlign},
  {"WavefrontSize",           CodePropField::WavefrontSize},
  {"NumSGPRs",                CodePropField::NumSGPRs},
  {"NumVGPRs",                CodePropField::NumVGPRs},
  {"MaxFlatWorkGroupSize",    CodePropField::MaxFlatWorkGroupSize},
  {"IsDynamicCallStack",      CodePropField::IsDynamicCallStack},
  {"IsXNACKEnabled",          CodePropField::IsXNACKEnabled},
  {"NumSpilledSGPRs",         CodePropField::NumSpilledSGPRs},
  {"NumSpilledVGPRs",         CodePropField::NumSpilledVGPRs}
};
#include "RealTimerClass.h"
using namespace Global;

// public variables -- TODO move these to a runtime object?
atmi_machine_t g_atmi_machine;
ATLMachine g_atl_machine;

hsa_agent_t atl_cpu_agent;
hsa_ext_program_t atl_hsa_program;
hsa_region_t atl_hsa_primary_region;
hsa_region_t atl_gpu_kernarg_region;
hsa_amd_memory_pool_t atl_gpu_kernarg_pool;
hsa_region_t atl_cpu_kernarg_region;
hsa_agent_t atl_gpu_agent;
hsa_profile_t atl_gpu_agent_profile;

atl_kernel_enqueue_args_t g_ke_args;
static std::vector<hsa_executable_t> g_executables;

std::map<std::string, std::string> KernelNameMap;
std::vector<std::map<std::string, atl_kernel_info_t> > KernelInfoTable;
std::vector<std::map<std::string, atl_symbol_info_t> > SymbolInfoTable;
std::set<std::string> SymbolSet;

static atl_dep_sync_t g_dep_sync_type = (atl_dep_sync_t)core::Runtime::getInstance().getDepSyncType();

RealTimer SignalAddTimer("Signal Time");
RealTimer HandleSignalTimer("Handle Signal Time");;
RealTimer HandleSignalInvokeTimer("Handle Signal Invoke Time");
RealTimer TaskWaitTimer("Task Wait Time");
RealTimer TryLaunchTimer("Launch Time");
RealTimer ParamsInitTimer("Params Init Time");
RealTimer TryLaunchInitTimer("Launch Init Time");
RealTimer ShouldDispatchTimer("Dispatch Eval Time");
RealTimer RegisterCallbackTimer("Register Callback Time");
RealTimer LockTimer("Lock/Unlock Time");
RealTimer TryDispatchTimer("Dispatch Time");
size_t max_ready_queue_sz = 0;
size_t waiting_count = 0;
size_t direct_dispatch = 0;
size_t callback_dispatch = 0;

bool g_atmi_initialized = false;

struct timespec context_init_time;
int context_init_time_init = 0;

/*
   All global values are defined here in two data structures.

   1  atmi_context is all information we expose externally.
   The structure atmi_context_t is defined in atmi.h.
   Most references will use pointer prefix atmi_context->
   The value atmi_context_data. is equivalent to atmi_context->

   2  atlc is all internal global values.
   The structure atl_context_t is defined in atl_internal.h
   Most references will use the global structure prefix atlc.
   However the pointer value atlc_p-> is equivalent to atlc.

*/


atmi_context_t atmi_context_data;
atmi_context_t * atmi_context = NULL;
atl_context_t atlc = { .struct_initialized=0 };
atl_context_t * atlc_p = NULL;

hsa_signal_t IdentityORSignal;
hsa_signal_t IdentityANDSignal;
hsa_signal_t IdentityCopySignal;

namespace core {
  /* Machine Info */
  atmi_machine_t* Runtime::GetMachineInfo() {
    if(!atlc.g_hsa_initialized) return NULL;
    return &g_atmi_machine;
  }

  void atl_set_atmi_initialized() {
    // FIXME: thread safe? locks?
    g_atmi_initialized = true;
  }

  void atl_reset_atmi_initialized() {
    // FIXME: thread safe? locks?
    g_atmi_initialized = false;
  }

  bool atl_is_atmi_initialized() {
    return g_atmi_initialized;
  }

  void allow_access_to_all_gpu_agents(void *ptr) {
    hsa_status_t err;
    std::vector<ATLGPUProcessor> &gpu_procs = g_atl_machine.getProcessors<ATLGPUProcessor>();
    std::vector<hsa_agent_t> agents;
    for(int i = 0; i < gpu_procs.size(); i++) {
      agents.push_back(gpu_procs[i].getAgent());
    }
    err = hsa_amd_agents_allow_access(agents.size(), &agents[0], NULL, ptr);
    ErrorCheck(Allow agents ptr access, err);
  }

  atmi_status_t atmi_ke_init() {
    // create and fill in the global structure needed for device enqueue
    // fill in gpu queues
    hsa_status_t err;
    std::vector<hsa_queue_t *> gpu_queues;
    int gpu_count = g_atl_machine.getProcessorCount<ATLGPUProcessor>();
    for(int gpu = 0; gpu < gpu_count; gpu++) {
      int num_queues = 0;
      atmi_place_t place = ATMI_PLACE_GPU(0, gpu);
      ATLGPUProcessor &proc = get_processor<ATLGPUProcessor>(place);
      std::vector<hsa_queue_t *> qs = proc.getQueues();
      num_queues = qs.size();
      gpu_queues.insert(gpu_queues.end(), qs.begin(), qs.end());
      // TODO: how to handle queues from multiple devices? keep them separate?
      // Currently, first N queues correspond to GPU0, next N queues map to GPU1
      // and so on.
    }
    g_ke_args.num_gpu_queues = gpu_queues.size();
    void *gpu_queue_ptr = NULL;
    if(g_ke_args.num_gpu_queues > 0) {
      err = hsa_amd_memory_pool_allocate(atl_gpu_kernarg_pool,
          sizeof(hsa_queue_t *) * g_ke_args.num_gpu_queues,
          0,
          &gpu_queue_ptr);
      ErrorCheck(Allocating GPU queue pointers, err);
      allow_access_to_all_gpu_agents(gpu_queue_ptr);
      for(int gpuq = 0; gpuq < gpu_queues.size(); gpuq++) {
        ((hsa_queue_t **)gpu_queue_ptr)[gpuq] = gpu_queues[gpuq];
      }
    }
    g_ke_args.gpu_queue_ptr = gpu_queue_ptr;

    // fill in cpu queues
    std::vector<hsa_queue_t *> cpu_queues;
    int cpu_count = g_atl_machine.getProcessorCount<ATLCPUProcessor>();
    for(int cpu = 0; cpu < cpu_count; cpu++) {
      int num_queues = 0;
      atmi_place_t place = ATMI_PLACE_CPU(0, cpu);
      ATLCPUProcessor &proc = get_processor<ATLCPUProcessor>(place);
      std::vector<hsa_queue_t *> qs = proc.getQueues();
      num_queues = qs.size();
      cpu_queues.insert(cpu_queues.end(), qs.begin(), qs.end());
      // TODO: how to handle queues from multiple devices? keep them separate?
      // Currently, first N queues correspond to CPU0, next N queues map to CPU1
      // and so on.
    }
    g_ke_args.num_cpu_queues = cpu_queues.size();
    void *cpu_queue_ptr = NULL;
    if(g_ke_args.num_cpu_queues > 0) {
      err = hsa_amd_memory_pool_allocate(atl_gpu_kernarg_pool,
          sizeof(hsa_queue_t *) * g_ke_args.num_cpu_queues,
          0,
          &cpu_queue_ptr);
      ErrorCheck(Allocating CPU queue pointers, err);
      allow_access_to_all_gpu_agents(cpu_queue_ptr);
      for(int cpuq = 0; cpuq < cpu_queues.size(); cpuq++) {
        ((hsa_queue_t **)cpu_queue_ptr)[cpuq] = cpu_queues[cpuq];
      }
    }
    g_ke_args.cpu_queue_ptr = cpu_queue_ptr;

    void *cpu_worker_signals = NULL;
    if(g_ke_args.num_cpu_queues > 0) {
      err = hsa_amd_memory_pool_allocate(atl_gpu_kernarg_pool,
          sizeof(hsa_signal_t) * g_ke_args.num_cpu_queues,
          0,
          &cpu_worker_signals);
      ErrorCheck(Allocating CPU queue iworker signals, err);
      allow_access_to_all_gpu_agents(cpu_worker_signals);
      for(int cpuq = 0; cpuq < cpu_queues.size(); cpuq++) {
        ((hsa_signal_t *)cpu_worker_signals)[cpuq] = *(get_worker_sig(cpu_queues[cpuq]));
      }
    }
    g_ke_args.cpu_worker_signals = cpu_worker_signals;


    void *kernarg_template_ptr = NULL;
    int max_kernel_types = core::Runtime::getInstance().getMaxKernelTypes();
    if (max_kernel_types > 0) {
      // Allocate template space for shader kernels
      err = hsa_amd_memory_pool_allocate(atl_gpu_kernarg_pool,
          sizeof(atmi_kernel_enqueue_template_t) *
          max_kernel_types,
          0, &kernarg_template_ptr);
      ErrorCheck(Allocating kernel argument template pointer, err);
      allow_access_to_all_gpu_agents(kernarg_template_ptr);
    }
    g_ke_args.kernarg_template_ptr = kernarg_template_ptr;
    g_ke_args.kernel_counter = 0;
    return ATMI_STATUS_SUCCESS;
  }

  atmi_status_t Runtime::Initialize(atmi_devtype_t devtype) {
    if(atl_is_atmi_initialized()) return ATMI_STATUS_SUCCESS;

    task_process_init_buffer = NULL;
    task_process_fini_buffer = NULL;

    if(devtype == ATMI_DEVTYPE_ALL || devtype & ATMI_DEVTYPE_GPU)
      ATMIErrorCheck(GPU context init, atl_init_gpu_context());

    if(devtype == ATMI_DEVTYPE_ALL || devtype & ATMI_DEVTYPE_CPU)
      ATMIErrorCheck(CPU context init, atl_init_cpu_context());

    // create default taskgroup obj
    atmi_taskgroup_handle_t tghandle;
    ATMIErrorCheck(Create default taskgroup, TaskGroupCreate(&tghandle));

    ATMIErrorCheck(Device enqueue init, atmi_ke_init());

    atl_set_atmi_initialized();
    return ATMI_STATUS_SUCCESS;
  }

  atmi_status_t Runtime::Finalize() {
    // TODO: Finalize all processors, queues, signals, kernarg memory regions
    hsa_status_t err;
    finalize_hsa();
    // free up the kernel enqueue related data
    for(int i = 0; i < g_ke_args.kernel_counter; i++) {
      atmi_kernel_enqueue_template_t *ke_template = &((atmi_kernel_enqueue_template_t *)g_ke_args.kernarg_template_ptr)[i];
      hsa_memory_free(ke_template->kernarg_regions);
    }
    hsa_memory_free(g_ke_args.kernarg_template_ptr);
    hsa_memory_free(g_ke_args.cpu_queue_ptr);
    hsa_memory_free(g_ke_args.cpu_worker_signals);
    hsa_memory_free(g_ke_args.gpu_queue_ptr);
    for(int i = 0; i < g_executables.size(); i++) {
      err = hsa_executable_destroy(g_executables[i]);
      ErrorCheck(Destroying executable, err);
    }
    if(atlc.g_cpu_initialized == 1) {
      agent_fini();
      atlc.g_cpu_initialized = 0;
    }

    for(int i = 0; i < SymbolInfoTable.size(); i++) {
      SymbolInfoTable[i].clear();
    }
    SymbolInfoTable.clear();
    for(int i = 0; i < KernelInfoTable.size(); i++) {
      KernelInfoTable[i].clear();
    }
    KernelInfoTable.clear();

    atl_reset_atmi_initialized();
    err = hsa_shut_down();
    ErrorCheck(Shutting down HSA, err);
    std::cout << ParamsInitTimer;
    std::cout << ParamsInitTimer;
    std::cout << TryLaunchTimer;
    std::cout << TryLaunchInitTimer;
    std::cout << ShouldDispatchTimer;
    std::cout << TryDispatchTimer;
    std::cout << TaskWaitTimer;
    std::cout << LockTimer;
    std::cout << HandleSignalTimer;
    std::cout << RegisterCallbackTimer;

    ParamsInitTimer.Reset();
    TryLaunchTimer.Reset();
    TryLaunchInitTimer.Reset();
    ShouldDispatchTimer.Reset();
    HandleSignalTimer.Reset();
    HandleSignalInvokeTimer.Reset();
    TryDispatchTimer.Reset();
    LockTimer.Reset();
    RegisterCallbackTimer.Reset();
    max_ready_queue_sz = 0;
    waiting_count = 0;
    direct_dispatch = 0;
    callback_dispatch = 0;

    return ATMI_STATUS_SUCCESS;
  }

  void atmi_init_context_structs() {
    atmi_context = &atmi_context_data;
    atlc_p = &atlc;
    atlc.struct_initialized = 1; /* This only gets called one time */
    atlc.g_cpu_initialized = 0;
    atlc.g_hsa_initialized = 0;
    atlc.g_gpu_initialized = 0;
    atlc.g_tasks_initialized = 0;
  }

  atmi_status_t atl_init_context() {
    atl_init_gpu_context();
    atl_init_cpu_context();

    return ATMI_STATUS_SUCCESS;
  }

  // Implement memory_pool iteration function
  static hsa_status_t get_memory_pool_info(hsa_amd_memory_pool_t memory_pool, void* data)
  {
    ATLProcessor* proc = reinterpret_cast<ATLProcessor*>(data);
    hsa_status_t err = HSA_STATUS_SUCCESS;
    // Check if the memory_pool is allowed to allocate, i.e. do not return group
    // memory
    bool alloc_allowed = false;
    err = hsa_amd_memory_pool_get_info(memory_pool, HSA_AMD_MEMORY_POOL_INFO_RUNTIME_ALLOC_ALLOWED, &alloc_allowed);
    ErrorCheck(Alloc allowed in memory pool check, err);
    if(alloc_allowed) {
      uint32_t global_flag = 0;
      err = hsa_amd_memory_pool_get_info(memory_pool, HSA_AMD_MEMORY_POOL_INFO_GLOBAL_FLAGS, &global_flag);
      ErrorCheck(Get memory pool info, err);
      if(HSA_AMD_MEMORY_POOL_GLOBAL_FLAG_FINE_GRAINED & global_flag) {
        ATLMemory new_mem(memory_pool, *proc, ATMI_MEMTYPE_FINE_GRAINED);
        proc->addMemory(new_mem);
        if(HSA_AMD_MEMORY_POOL_GLOBAL_FLAG_KERNARG_INIT & global_flag) {
          DEBUG_PRINT("GPU kernel args pool handle: %lu\n", memory_pool.handle);
          atl_gpu_kernarg_pool = memory_pool;
        }
      }
      else {
        ATLMemory new_mem(memory_pool, *proc, ATMI_MEMTYPE_COARSE_GRAINED);
        proc->addMemory(new_mem);
      }
    }

    return err;
  }

  static hsa_status_t get_agent_info(hsa_agent_t agent, void *data) {
    hsa_status_t err = HSA_STATUS_SUCCESS;
    hsa_device_type_t device_type;
    err = hsa_agent_get_info(agent, HSA_AGENT_INFO_DEVICE, &device_type);
    ErrorCheck(Get device type info, err);
    switch(device_type) {
      case HSA_DEVICE_TYPE_CPU:
        {
          ;
          ATLCPUProcessor new_proc(agent);
          err = hsa_amd_agent_iterate_memory_pools(agent, get_memory_pool_info, &new_proc);
          ErrorCheck(Iterate all memory pools, err);
          g_atl_machine.addProcessor(new_proc);
        }
        break;
      case HSA_DEVICE_TYPE_GPU:
        {
          ;
          hsa_profile_t profile;
          err = hsa_agent_get_info(agent, HSA_AGENT_INFO_PROFILE, &profile);
          ErrorCheck(Query the agent profile, err);
          atmi_devtype_t gpu_type;
          gpu_type = (profile == HSA_PROFILE_FULL) ? ATMI_DEVTYPE_iGPU : ATMI_DEVTYPE_dGPU;
          ATLGPUProcessor new_proc(agent, gpu_type);
          err = hsa_amd_agent_iterate_memory_pools(agent, get_memory_pool_info, &new_proc);
          ErrorCheck(Iterate all memory pools, err);
          g_atl_machine.addProcessor(new_proc);
        }
        break;
      case HSA_DEVICE_TYPE_DSP:
        {
          ;
          ATLDSPProcessor new_proc(agent);
          err = hsa_amd_agent_iterate_memory_pools(agent, get_memory_pool_info, &new_proc);
          ErrorCheck(Iterate all memory pools, err);
          g_atl_machine.addProcessor(new_proc);
        }
        break;
    }

    return err;
  }
  //#else
  /* Determines if the given agent is of type HSA_DEVICE_TYPE_GPU
     and sets the value of data to the agent handle if it is.
     */
  static hsa_status_t get_gpu_agent(hsa_agent_t agent, void *data) {
    hsa_status_t status;
    hsa_device_type_t device_type;
    status = hsa_agent_get_info(agent, HSA_AGENT_INFO_DEVICE, &device_type);
    DEBUG_PRINT("Device Type = %d\n", device_type);
    if (HSA_STATUS_SUCCESS == status && HSA_DEVICE_TYPE_GPU == device_type) {
      uint32_t max_queues;
      status = hsa_agent_get_info(agent, HSA_AGENT_INFO_QUEUES_MAX, &max_queues);
      DEBUG_PRINT("GPU has max queues = %" PRIu32 "\n", max_queues);
      hsa_agent_t* ret = (hsa_agent_t*)data;
      *ret = agent;
      return HSA_STATUS_INFO_BREAK;
    }
    return HSA_STATUS_SUCCESS;
  }

  /* Determines if the given agent is of type HSA_DEVICE_TYPE_CPU
     and sets the value of data to the agent handle if it is.
     */
  static hsa_status_t get_cpu_agent(hsa_agent_t agent, void *data) {
    hsa_status_t status;
    hsa_device_type_t device_type;
    status = hsa_agent_get_info(agent, HSA_AGENT_INFO_DEVICE, &device_type);
    if (HSA_STATUS_SUCCESS == status && HSA_DEVICE_TYPE_CPU == device_type) {
      uint32_t max_queues;
      status = hsa_agent_get_info(agent, HSA_AGENT_INFO_QUEUES_MAX, &max_queues);
      DEBUG_PRINT("CPU has max queues = %" PRIu32 "\n", max_queues);
      hsa_agent_t* ret = (hsa_agent_t*)data;
      *ret = agent;
      return HSA_STATUS_INFO_BREAK;
    }
    return HSA_STATUS_SUCCESS;
  }

  hsa_status_t get_fine_grained_region(hsa_region_t region, void* data) {
    hsa_region_segment_t segment;
    hsa_region_get_info(region, HSA_REGION_INFO_SEGMENT, &segment);
    if (segment != HSA_REGION_SEGMENT_GLOBAL) {
      return HSA_STATUS_SUCCESS;
    }
    hsa_region_global_flag_t flags;
    hsa_region_get_info(region, HSA_REGION_INFO_GLOBAL_FLAGS, &flags);
    if (flags & HSA_REGION_GLOBAL_FLAG_FINE_GRAINED) {
      hsa_region_t* ret = (hsa_region_t*) data;
      *ret = region;
      return HSA_STATUS_INFO_BREAK;
    }
    return HSA_STATUS_SUCCESS;
  }

  /* Determines if a memory region can be used for kernarg allocations.  */
  static hsa_status_t get_kernarg_memory_region(hsa_region_t region, void* data) {
    hsa_region_segment_t segment;
    hsa_region_get_info(region, HSA_REGION_INFO_SEGMENT, &segment);
    if (HSA_REGION_SEGMENT_GLOBAL != segment) {
      return HSA_STATUS_SUCCESS;
    }

    hsa_region_global_flag_t flags;
    hsa_region_get_info(region, HSA_REGION_INFO_GLOBAL_FLAGS, &flags);
    if (flags & HSA_REGION_GLOBAL_FLAG_KERNARG) {
      hsa_region_t* ret = (hsa_region_t*) data;
      *ret = region;
      return HSA_STATUS_INFO_BREAK;
    }

    return HSA_STATUS_SUCCESS;
  }

  hsa_status_t init_comute_and_memory() {
    hsa_status_t err;
#ifdef MEMORY_REGION
    err = hsa_iterate_agents(get_gpu_agent, &atl_gpu_agent);
    if(err == HSA_STATUS_INFO_BREAK) { err = HSA_STATUS_SUCCESS; }
    ErrorCheck(Getting a gpu agent, err);

    /* Query the name of the agent.  */
    //char name[64] = { 0 };
    //err = hsa_agent_get_info(atl_gpu_agent, HSA_AGENT_INFO_NAME, name);
    //ErrorCheck(Querying the agent name, err);
    /* printf("The agent name is %s.\n", name); */
    err = hsa_agent_get_info(atl_gpu_agent, HSA_AGENT_INFO_PROFILE, &atl_gpu_agent_profile);
    ErrorCheck(Query the agent profile, err);
    DEBUG_PRINT("Agent Profile: %d\n", atl_gpu_agent_profile);

    /* Find a memory region that supports kernel arguments.  */
    atl_gpu_kernarg_region.handle=(uint64_t)-1;
    hsa_agent_iterate_regions(atl_gpu_agent, get_kernarg_memory_region, &atl_gpu_kernarg_region);
    err = (atl_gpu_kernarg_region.handle == (uint64_t)-1) ? HSA_STATUS_ERROR : HSA_STATUS_SUCCESS;
    ErrorCheck(Finding a kernarg memory region, err);

    err = hsa_iterate_agents(get_cpu_agent, &atl_cpu_agent);
    if(err == HSA_STATUS_INFO_BREAK) { err = HSA_STATUS_SUCCESS; }
    ErrorCheck(Getting a gpu agent, err);

    atl_cpu_kernarg_region.handle=(uint64_t)-1;
    err = hsa_agent_iterate_regions(atl_cpu_agent, get_fine_grained_region, &atl_cpu_kernarg_region);
    if(err == HSA_STATUS_INFO_BREAK) { err = HSA_STATUS_SUCCESS; }
    err = (atl_cpu_kernarg_region.handle == (uint64_t)-1) ? HSA_STATUS_ERROR : HSA_STATUS_SUCCESS;
    ErrorCheck(Finding a CPU kernarg memory region handle, err);
#else
    /* Iterate over the agents and pick the gpu agent */
    err = hsa_iterate_agents(get_agent_info, NULL);
    if(err == HSA_STATUS_INFO_BREAK) { err = HSA_STATUS_SUCCESS; }
    ErrorCheck(Getting a gpu agent, err);
    if(err != HSA_STATUS_SUCCESS) return err;

    /* Init all devices or individual device types? */
    std::vector<ATLCPUProcessor> &cpu_procs = g_atl_machine.getProcessors<ATLCPUProcessor>();
    std::vector<ATLGPUProcessor> &gpu_procs = g_atl_machine.getProcessors<ATLGPUProcessor>();
    std::vector<ATLDSPProcessor> &dsp_procs = g_atl_machine.getProcessors<ATLDSPProcessor>();
    /* For CPU memory pools, add other devices that can access them directly
     * or indirectly */
    for(std::vector<ATLCPUProcessor>::iterator cpu_it = cpu_procs.begin();
        cpu_it != cpu_procs.end(); cpu_it++) {
      std::vector<ATLMemory> &cpu_mems = cpu_it->getMemories();
      for(std::vector<ATLMemory>::iterator cpu_mem_it = cpu_mems.begin();
          cpu_mem_it != cpu_mems.end(); cpu_mem_it++) {
        hsa_amd_memory_pool_t pool = cpu_mem_it->getMemory();
        for(std::vector<ATLGPUProcessor>::iterator gpu_it = gpu_procs.begin();
            gpu_it != gpu_procs.end(); gpu_it++) {
          hsa_agent_t agent = gpu_it->getAgent();
          hsa_amd_memory_pool_access_t access;
          hsa_amd_agent_memory_pool_get_info(agent,
              pool,
              HSA_AMD_AGENT_MEMORY_POOL_INFO_ACCESS,
              &access);
          if(access != 0) {
            // this means not NEVER, but could be YES or NO
            // add this memory pool to the proc
            gpu_it->addMemory(*cpu_mem_it);
          }
        }
        for(std::vector<ATLDSPProcessor>::iterator dsp_it = dsp_procs.begin();
            dsp_it != dsp_procs.end(); dsp_it++) {
          hsa_agent_t agent = dsp_it->getAgent();
          hsa_amd_memory_pool_access_t access;
          hsa_amd_agent_memory_pool_get_info(agent,
              pool,
              HSA_AMD_AGENT_MEMORY_POOL_INFO_ACCESS,
              &access);
          if(access != 0) {
            // this means not NEVER, but could be YES or NO
            // add this memory pool to the proc
            dsp_it->addMemory(*cpu_mem_it);
          }
        }
      }
    }

    /* FIXME: are the below combinations of procs and memory pools needed?
     * all to all compare procs with their memory pools and add those memory
     * pools that are accessible by the target procs */
    for(std::vector<ATLGPUProcessor>::iterator gpu_it = gpu_procs.begin();
        gpu_it != gpu_procs.end(); gpu_it++) {
      std::vector<ATLMemory> &gpu_mems = gpu_it->getMemories();
      for(std::vector<ATLMemory>::iterator gpu_mem_it = gpu_mems.begin();
          gpu_mem_it != gpu_mems.end(); gpu_mem_it++) {
        hsa_amd_memory_pool_t pool = gpu_mem_it->getMemory();
        for(std::vector<ATLDSPProcessor>::iterator dsp_it = dsp_procs.begin();
            dsp_it != dsp_procs.end(); dsp_it++) {
          hsa_agent_t agent = dsp_it->getAgent();
          hsa_amd_memory_pool_access_t access;
          hsa_amd_agent_memory_pool_get_info(agent,
              pool,
              HSA_AMD_AGENT_MEMORY_POOL_INFO_ACCESS,
              &access);
          if(access != 0) {
            // this means not NEVER, but could be YES or NO
            // add this memory pool to the proc
            dsp_it->addMemory(*gpu_mem_it);
          }
        }

        for(std::vector<ATLCPUProcessor>::iterator cpu_it = cpu_procs.begin();
            cpu_it != cpu_procs.end(); cpu_it++) {
          hsa_agent_t agent = cpu_it->getAgent();
          hsa_amd_memory_pool_access_t access;
          hsa_amd_agent_memory_pool_get_info(agent,
              pool,
              HSA_AMD_AGENT_MEMORY_POOL_INFO_ACCESS,
              &access);
          if(access != 0) {
            // this means not NEVER, but could be YES or NO
            // add this memory pool to the proc
            cpu_it->addMemory(*gpu_mem_it);
          }
        }
      }
    }

    for(std::vector<ATLDSPProcessor>::iterator dsp_it = dsp_procs.begin();
        dsp_it != dsp_procs.end(); dsp_it++) {
      std::vector<ATLMemory> &dsp_mems = dsp_it->getMemories();
      for(std::vector<ATLMemory>::iterator dsp_mem_it = dsp_mems.begin();
          dsp_mem_it != dsp_mems.end(); dsp_mem_it++) {
        hsa_amd_memory_pool_t pool = dsp_mem_it->getMemory();
        for(std::vector<ATLGPUProcessor>::iterator gpu_it = gpu_procs.begin();
            gpu_it != gpu_procs.end(); gpu_it++) {
          hsa_agent_t agent = gpu_it->getAgent();
          hsa_amd_memory_pool_access_t access;
          hsa_amd_agent_memory_pool_get_info(agent,
              pool,
              HSA_AMD_AGENT_MEMORY_POOL_INFO_ACCESS,
              &access);
          if(access != 0) {
            // this means not NEVER, but could be YES or NO
            // add this memory pool to the proc
            gpu_it->addMemory(*dsp_mem_it);
          }
        }

        for(std::vector<ATLCPUProcessor>::iterator cpu_it = cpu_procs.begin();
            cpu_it != cpu_procs.end(); cpu_it++) {
          hsa_agent_t agent = cpu_it->getAgent();
          hsa_amd_memory_pool_access_t access;
          hsa_amd_agent_memory_pool_get_info(agent,
              pool,
              HSA_AMD_AGENT_MEMORY_POOL_INFO_ACCESS,
              &access);
          if(access != 0) {
            // this means not NEVER, but could be YES or NO
            // add this memory pool to the proc
            cpu_it->addMemory(*dsp_mem_it);
          }
        }
      }
    }

    g_atmi_machine.device_count_by_type[ATMI_DEVTYPE_CPU] = cpu_procs.size();
    g_atmi_machine.device_count_by_type[ATMI_DEVTYPE_GPU] = gpu_procs.size();
    g_atmi_machine.device_count_by_type[ATMI_DEVTYPE_DSP] = dsp_procs.size();
    size_t num_procs = cpu_procs.size() + gpu_procs.size() + dsp_procs.size();
    //g_atmi_machine.devices = (atmi_device_t *)malloc(num_procs * sizeof(atmi_device_t));
    atmi_device_t *all_devices = (atmi_device_t *)malloc(num_procs * sizeof(atmi_device_t));
    int num_iGPUs = 0;
    int num_dGPUs = 0;
    for(int i = 0; i < gpu_procs.size(); i++) {
      if(gpu_procs[i].getType() == ATMI_DEVTYPE_iGPU)
        num_iGPUs++;
      else
        num_dGPUs++;
    }
    assert(num_iGPUs + num_dGPUs == gpu_procs.size() && "Number of dGPUs and iGPUs do not add up");
    DEBUG_PRINT("CPU Agents: %lu\n", cpu_procs.size());
    DEBUG_PRINT("iGPU Agents: %d\n", num_iGPUs);
    DEBUG_PRINT("dGPU Agents: %d\n", num_dGPUs);
    DEBUG_PRINT("GPU Agents: %lu\n", gpu_procs.size());
    DEBUG_PRINT("DSP Agents: %lu\n", dsp_procs.size());
    g_atmi_machine.device_count_by_type[ATMI_DEVTYPE_iGPU] = num_iGPUs;
    g_atmi_machine.device_count_by_type[ATMI_DEVTYPE_dGPU] = num_dGPUs;

    int cpus_begin = 0;
    int cpus_end = cpu_procs.size();
    int gpus_begin = cpu_procs.size();
    int gpus_end = cpu_procs.size() + gpu_procs.size();
    g_atmi_machine.devices_by_type[ATMI_DEVTYPE_CPU] = &all_devices[cpus_begin];
    g_atmi_machine.devices_by_type[ATMI_DEVTYPE_GPU] = &all_devices[gpus_begin];
    g_atmi_machine.devices_by_type[ATMI_DEVTYPE_iGPU] = &all_devices[gpus_begin];
    g_atmi_machine.devices_by_type[ATMI_DEVTYPE_dGPU] = &all_devices[gpus_begin];
    int proc_index = 0;
    for(int i = cpus_begin; i < cpus_end; i++) {
      all_devices[i].type = cpu_procs[proc_index].getType();
      all_devices[i].core_count = cpu_procs[proc_index].getNumCUs();

      std::vector<ATLMemory> memories = cpu_procs[proc_index].getMemories();
      int fine_memories_size = 0;
      int coarse_memories_size = 0;
      DEBUG_PRINT("CPU memory types:\t");
      for(std::vector<ATLMemory>::iterator it = memories.begin();
          it != memories.end(); it++) {
        atmi_memtype_t type = it->getType();
        if(type == ATMI_MEMTYPE_FINE_GRAINED) {
          fine_memories_size++;
          DEBUG_PRINT("Fine\t");
        }
        else {
          coarse_memories_size++;
          DEBUG_PRINT("Coarse\t");
        }
      }
      DEBUG_PRINT("\nFine Memories : %d", fine_memories_size);
      DEBUG_PRINT("\tCoarse Memories : %d\n", coarse_memories_size);
      all_devices[i].memory_count = memories.size();
      proc_index++;
    }
    proc_index = 0;
    for(int i = gpus_begin; i < gpus_end; i++) {
      all_devices[i].type = gpu_procs[proc_index].getType();
      all_devices[i].core_count = gpu_procs[proc_index].getNumCUs();

      std::vector<ATLMemory> memories = gpu_procs[proc_index].getMemories();
      int fine_memories_size = 0;
      int coarse_memories_size = 0;
      DEBUG_PRINT("GPU memory types:\t");
      for(std::vector<ATLMemory>::iterator it = memories.begin();
          it != memories.end(); it++) {
        atmi_memtype_t type = it->getType();
        if(type == ATMI_MEMTYPE_FINE_GRAINED) {
          fine_memories_size++;
          DEBUG_PRINT("Fine\t");
        }
        else {
          coarse_memories_size++;
          DEBUG_PRINT("Coarse\t");
        }
      }
      DEBUG_PRINT("\nFine Memories : %d", fine_memories_size);
      DEBUG_PRINT("\tCoarse Memories : %d\n", coarse_memories_size);
      all_devices[i].memory_count = memories.size();
      proc_index++;
    }
    proc_index = 0;
    atl_cpu_kernarg_region.handle=(uint64_t)-1;
    if(cpu_procs.size() > 0) {
      err = hsa_agent_iterate_regions(cpu_procs[0].getAgent(), get_fine_grained_region, &atl_cpu_kernarg_region);
      if(err == HSA_STATUS_INFO_BREAK) { err = HSA_STATUS_SUCCESS; }
      err = (atl_cpu_kernarg_region.handle == (uint64_t)-1) ? HSA_STATUS_ERROR : HSA_STATUS_SUCCESS;
      ErrorCheck(Finding a CPU kernarg memory region handle, err);
    }
    /* Find a memory region that supports kernel arguments.  */
    atl_gpu_kernarg_region.handle=(uint64_t)-1;
    if(gpu_procs.size() > 0) {
      hsa_agent_iterate_regions(gpu_procs[0].getAgent(), get_kernarg_memory_region, &atl_gpu_kernarg_region);
      err = (atl_gpu_kernarg_region.handle == (uint64_t)-1) ? HSA_STATUS_ERROR : HSA_STATUS_SUCCESS;
      ErrorCheck(Finding a kernarg memory region, err);
    }
    if(num_procs > 0)
      return HSA_STATUS_SUCCESS;
    else
      return HSA_STATUS_ERROR_NOT_INITIALIZED;
#endif
  }

  hsa_status_t init_hsa() {
    if(atlc.g_hsa_initialized == 0) {
      DEBUG_PRINT("Initializing HSA...");
      hsa_status_t err = hsa_init();
      ErrorCheck(Initializing the hsa runtime, err);
      if(err != HSA_STATUS_SUCCESS) return err;

      err = init_comute_and_memory();
      if(err != HSA_STATUS_SUCCESS) return err;
      ErrorCheck(After initializing compute and memory, err);
      init_dag_scheduler();
      for(int i = 0; i < SymbolInfoTable.size(); i++)
        SymbolInfoTable[i].clear();
      SymbolInfoTable.clear();
      for(int i = 0; i < KernelInfoTable.size(); i++)
        KernelInfoTable[i].clear();
      KernelInfoTable.clear();
      atlc.g_hsa_initialized = 1;
      DEBUG_PRINT("done\n");
    }
    return HSA_STATUS_SUCCESS;
  }

  hsa_status_t finalize_hsa() {
    return HSA_STATUS_SUCCESS;
  }

  void init_tasks() {
    if(atlc.g_tasks_initialized != 0) return;
    hsa_status_t err;
    int task_num;
    std::vector<hsa_agent_t> gpu_agents;
    int gpu_count = g_atl_machine.getProcessorCount<ATLGPUProcessor>();
    for(int gpu = 0; gpu < gpu_count; gpu++) {
      atmi_place_t place = ATMI_PLACE_GPU(0, gpu);
      ATLGPUProcessor &proc = get_processor<ATLGPUProcessor>(place);
      gpu_agents.push_back(proc.getAgent());
    }
    int max_signals = core::Runtime::getInstance().getMaxSignals();
    for ( task_num = 0 ; task_num < max_signals; task_num++){
      hsa_signal_t new_signal;
      // For ATL_SYNC_CALLBACK, we need host to be interrupted
      // upon task completion to resolve dependencies on the host.
      // For ATL_SYNC_BARRIER_PKT, since barrier packets resolve
      // dependencies within the GPU, they can be just agent signals
      // without host interrupts.
      // TODO: for barrier packet with host tasks, should we create
      // a separate list of free signals?
      if(g_dep_sync_type == ATL_SYNC_CALLBACK)
        err=hsa_signal_create(0, 0, NULL, &new_signal);
      else
        err=hsa_signal_create(0, gpu_count, &gpu_agents[0], &new_signal);
      ErrorCheck(Creating a HSA signal, err);
      FreeSignalPool.push(new_signal);
    }
    err=hsa_signal_create(1, 0, NULL, &IdentityORSignal);
    ErrorCheck(Creating a HSA signal, err);
    err=hsa_signal_create(0, 0, NULL, &IdentityANDSignal);
    ErrorCheck(Creating a HSA signal, err);
    err=hsa_signal_create(0, 0, NULL, &IdentityCopySignal);
    ErrorCheck(Creating a HSA signal, err);
    DEBUG_PRINT("Signal Pool Size: %lu\n", FreeSignalPool.size());
    atlc.g_tasks_initialized = 1;
  }

  hsa_status_t callbackEvent(const hsa_amd_event_t *event, void *data) {
#if (ROCM_VERSION_MAJOR >= 3) || (ROCM_VERSION_MAJOR >= 2 && ROCM_VERSION_MINOR >= 3)
    if(event->event_type == HSA_AMD_GPU_MEMORY_FAULT_EVENT) {
#else
    if(event->event_type == GPU_MEMORY_FAULT_EVENT) {
#endif
      hsa_amd_gpu_memory_fault_info_t memory_fault = event->memory_fault;
      // memory_fault.agent
      // memory_fault.virtual_address
      // memory_fault.fault_reason_mask
      //fprintf("[GPU Error at %p: Reason is ", memory_fault.virtual_address);
      std::stringstream stream;
      stream << std::hex << (uintptr_t)memory_fault.virtual_address;
      std::string addr("0x" + stream.str());

      std::string err_string = "[GPU Memory Error] Addr: " + addr;
      err_string += " Reason: ";
      if(!(memory_fault.fault_reason_mask & 0x00111111))
        err_string += "No Idea! ";
      else {
        if(memory_fault.fault_reason_mask & 0x00000001)
          err_string += "Page not present or supervisor privilege. ";
        if(memory_fault.fault_reason_mask & 0x00000010)
          err_string += "Write access to a read-only page. ";
        if(memory_fault.fault_reason_mask & 0x00000100)
          err_string += "Execute access to a page marked NX. ";
        if(memory_fault.fault_reason_mask & 0x00001000)
          err_string += "Host access only. ";
        if(memory_fault.fault_reason_mask & 0x00010000)
          err_string += "ECC failure (if supported by HW). ";
        if(memory_fault.fault_reason_mask & 0x00100000)
          err_string += "Can't determine the exact fault address. ";
      }
      fprintf(stderr, "%s\n", err_string.c_str());
      return HSA_STATUS_ERROR;
    }
    return HSA_STATUS_SUCCESS;
  }

  atmi_status_t atl_init_gpu_context() {

    if(atlc.struct_initialized == 0) atmi_init_context_structs();
    if(atlc.g_gpu_initialized != 0) return ATMI_STATUS_SUCCESS;

    hsa_status_t err;
    err = init_hsa();
    if(err != HSA_STATUS_SUCCESS) return ATMI_STATUS_ERROR;

    int gpu_count = g_atl_machine.getProcessorCount<ATLGPUProcessor>();
    for(int gpu = 0; gpu < gpu_count; gpu++) {
      atmi_place_t place = ATMI_PLACE_GPU(0, gpu);
      ATLGPUProcessor &proc = get_processor<ATLGPUProcessor>(place);
      int num_gpu_queues = core::Runtime::getInstance().getNumGPUQueues();
      if(num_gpu_queues == -1) {
        num_gpu_queues = proc.getNumCUs();
        num_gpu_queues = (num_gpu_queues > 8) ? 8 : num_gpu_queues;
      }
      proc.createQueues(num_gpu_queues);
    }

    if(context_init_time_init == 0) {
      clock_gettime(CLOCK_MONOTONIC_RAW,&context_init_time);
      context_init_time_init = 1;
    }

    err = hsa_amd_register_system_event_handler(callbackEvent, NULL);
    ErrorCheck(Registering the system for memory faults, err);

    init_tasks();
    atlc.g_gpu_initialized = 1;
    return ATMI_STATUS_SUCCESS;
  }

  atmi_status_t atl_init_cpu_context() {

    if(atlc.struct_initialized == 0) atmi_init_context_structs();

    if(atlc.g_cpu_initialized != 0) return ATMI_STATUS_SUCCESS;

    hsa_status_t err;
    err = init_hsa();
    if(err != HSA_STATUS_SUCCESS) return ATMI_STATUS_ERROR;

    /* Get a CPU agent, create a pthread to handle packets*/
    /* Iterate over the agents and pick the cpu agent */
#if defined (ATMI_HAVE_PROFILE)
    atmi_profiling_init();
#endif /*ATMI_HAVE_PROFILE */
    int cpu_count = g_atl_machine.getProcessorCount<ATLCPUProcessor>();
    for(int cpu = 0; cpu < cpu_count; cpu++) {
      atmi_place_t place = ATMI_PLACE_CPU(0, cpu);
      ATLCPUProcessor &proc = get_processor<ATLCPUProcessor>(place);
      // FIXME: We are creating as many CPU queues as there are cores
      // But, this will share CPU worker threads with main host thread
      // and the HSA callback thread. Is there any real benefit from
      // restricting the number of queues to num_cus - 2?
      int num_cpu_queues = core::Runtime::getInstance().getNumCPUQueues();
      if(num_cpu_queues == -1) {
        num_cpu_queues = proc.getNumCUs();
      }
      cpu_agent_init(cpu, num_cpu_queues);
    }

    init_tasks();
    atlc.g_cpu_initialized = 1;
    return ATMI_STATUS_SUCCESS;
  }

  void *atl_read_binary_from_file(const char *module, size_t *module_size) {
    // Open file.
    std::ifstream file(module, std::ios::in | std::ios::binary);
    if(!(file.is_open() && file.good())) {
      fprintf(stderr, "File %s not found\n", module);
      return NULL;
    }

    // Find out file size.
    file.seekg(0, file.end);
    size_t size = file.tellg();
    file.seekg(0, file.beg);

    // Allocate memory for raw code object.
    void *raw_code_object = malloc(size);
    assert(raw_code_object);

    // Read file contents.
    file.read((char*)raw_code_object, size);

    // Close file.
    file.close();
    *module_size = size;
    return raw_code_object;
  }

  bool isImplicit(ValueKind value_kind) {
    switch(value_kind) {
      case ValueKind::HiddenGlobalOffsetX:
      case ValueKind::HiddenGlobalOffsetY:
      case ValueKind::HiddenGlobalOffsetZ:
      case ValueKind::HiddenNone:
      case ValueKind::HiddenPrintfBuffer:
      case ValueKind::HiddenDefaultQueue:
      case ValueKind::HiddenCompletionAction:
      //case ValueKind::HiddenMultiGridSyncArg:
        return true;
      default:
        return false;
    }
  }

  hsa_status_t validate_code_object(hsa_code_object_t code_object, hsa_code_symbol_t symbol, void *data) {
    hsa_status_t retVal = HSA_STATUS_SUCCESS;
    int gpu = *(int *)data;
    hsa_symbol_kind_t type;

    uint32_t name_length;
    hsa_status_t err;
    err = hsa_code_symbol_get_info(symbol, HSA_CODE_SYMBOL_INFO_TYPE, &type);
    ErrorCheck(Symbol info extraction, err);
    DEBUG_PRINT("Exec Symbol type: %d\n", type);

    if(type == HSA_SYMBOL_KIND_VARIABLE) {
      err = hsa_code_symbol_get_info(symbol, HSA_CODE_SYMBOL_INFO_NAME_LENGTH, &name_length);
      ErrorCheck(Symbol info extraction, err);
      char *name = (char *)malloc(name_length + 1);
      err = hsa_code_symbol_get_info(symbol, HSA_CODE_SYMBOL_INFO_NAME, name);
      ErrorCheck(Symbol info extraction, err);
      name[name_length] = 0;

      if(SymbolSet.find(std::string(name)) != SymbolSet.end()) {
        // Symbol already found. Return Error
        DEBUG_PRINT("Symbol %s already found!\n", name);
        retVal = HSA_STATUS_ERROR_VARIABLE_ALREADY_DEFINED;
      } else {
        SymbolSet.insert(std::string(name));
      }

      free(name);
    }
    else {
      DEBUG_PRINT("Symbol is an indirect function\n");
    }
    return retVal;

  }

  static amd_comgr_status_t getMetaBuf(const amd_comgr_metadata_node_t meta,
      std::string* str) {
    size_t size = 0;
    amd_comgr_status_t status = amd_comgr_get_metadata_string(meta, &size, NULL);

    if (status == AMD_COMGR_STATUS_SUCCESS) {
      str->resize(size-1);    // minus one to discount the null character
      status = amd_comgr_get_metadata_string(meta, &size, &((*str)[0]));
    }

    return status;
  }

  static amd_comgr_status_t populateArgs(const amd_comgr_metadata_node_t key,
      const amd_comgr_metadata_node_t value,
      void *data) {
    amd_comgr_status_t status;
    amd_comgr_metadata_kind_t kind;
    std::string buf;

    // get the key of the argument field
    size_t size = 0;
    status = amd_comgr_get_metadata_kind(key, &kind);
    if (kind == AMD_COMGR_METADATA_KIND_STRING && status == AMD_COMGR_STATUS_SUCCESS) {
      status = getMetaBuf(key, &buf);
    }
    //std::cout << "Trying to get argField: " << buf << std::endl;
    if (status != AMD_COMGR_STATUS_SUCCESS) {
      return AMD_COMGR_STATUS_ERROR;
    }

    auto itArgField = ArgFieldMap.find(buf);
    if (itArgField == ArgFieldMap.end()) {
      return AMD_COMGR_STATUS_ERROR;
    }

    // get the value of the argument field
    status = getMetaBuf(value, &buf);

    KernelArgMD* lcArg = static_cast<KernelArgMD*>(data);

    switch (itArgField->second) {
      case ArgField::Name:
        lcArg->mName = buf;
        break;
      case ArgField::TypeName:
        lcArg->mTypeName = buf;
        break;
      case ArgField::Size:
        lcArg->mSize = atoi(buf.c_str());
        break;
      case ArgField::Align:
        lcArg->mAlign = atoi(buf.c_str());
        break;
      case ArgField::Offset:
        // FIXME: KernelArgMD does not have the mOffset field; until that changes, we use
        // mAlign to store the offset; minor "workaround"
        lcArg->mAlign = atoi(buf.c_str());
        break;
      case ArgField::ValueKind:
        {
          auto itValueKind = ArgValueKind.find(buf);
          if (itValueKind == ArgValueKind.end()) {
            return AMD_COMGR_STATUS_ERROR;
          }
          lcArg->mValueKind = itValueKind->second;
        }
        break;
      // TODO: If we are interested in parsing other fields, then uncomment them from below
#if 0
      case ArgField::ValueType:
        {
          auto itValueType = ArgValueType.find(buf);
          if (itValueType == ArgValueType.end()) {
            return AMD_COMGR_STATUS_ERROR;
          }
          lcArg->mValueType = itValueType->second;
        }
        break;
      case ArgField::PointeeAlign:
        lcArg->mPointeeAlign = atoi(buf.c_str());
        break;
      case ArgField::AddrSpaceQual:
        {
          auto itAddrSpaceQual = ArgAddrSpaceQual.find(buf);
          if (itAddrSpaceQual == ArgAddrSpaceQual.end()) {
            return AMD_COMGR_STATUS_ERROR;
          }
          lcArg->mAddrSpaceQual = itAddrSpaceQual->second;
        }
        break;
      case ArgField::AccQual:
        {
          auto itAccQual = ArgAccQual.find(buf);
          if (itAccQual == ArgAccQual.end()) {
            return AMD_COMGR_STATUS_ERROR;
          }
          lcArg->mAccQual = itAccQual->second;
        }
        break;
      case ArgField::ActualAccQual:
        {
          auto itAccQual = ArgAccQual.find(buf);
          if (itAccQual == ArgAccQual.end()) {
            return AMD_COMGR_STATUS_ERROR;
          }
          lcArg->mActualAccQual = itAccQual->second;
        }
        break;
      case ArgField::IsConst:
        lcArg->mIsConst = (buf.compare("true") == 0);
        break;
      case ArgField::IsRestrict:
        lcArg->mIsRestrict = (buf.compare("true") == 0);
        break;
      case ArgField::IsVolatile:
        lcArg->mIsVolatile = (buf.compare("true") == 0);
        break;
      case ArgField::IsPipe:
        lcArg->mIsPipe = (buf.compare("true") == 0);
        break;
#endif
      default:
        return AMD_COMGR_STATUS_SUCCESS;
        //return AMD_COMGR_STATUS_ERROR;
    }
    return AMD_COMGR_STATUS_SUCCESS;
  }

  static amd_comgr_status_t populateCodeProps(const amd_comgr_metadata_node_t key,
                                            const amd_comgr_metadata_node_t value,
                                            void *data) {
    amd_comgr_status_t status;
    amd_comgr_metadata_kind_t kind;
    std::string buf;

    // get the key of the argument field
    status = amd_comgr_get_metadata_kind(key, &kind);
    if (kind == AMD_COMGR_METADATA_KIND_STRING && status == AMD_COMGR_STATUS_SUCCESS) {
      status = getMetaBuf(key, &buf);
    }

    if (status != AMD_COMGR_STATUS_SUCCESS) {
      return AMD_COMGR_STATUS_ERROR;
    }

    auto itCodePropField = CodePropFieldMap.find(buf);
    if (itCodePropField == CodePropFieldMap.end()) {
      return AMD_COMGR_STATUS_ERROR;
    }

    // get the value of the argument field
    if (status == AMD_COMGR_STATUS_SUCCESS) {
      status = getMetaBuf(value, &buf);
    }

    KernelMD*  kernelMD = static_cast<KernelMD*>(data);
    switch (itCodePropField->second) {
      case CodePropField::KernargSegmentSize:
        kernelMD->mCodeProps.mKernargSegmentSize = atoi(buf.c_str());
        break;
      // TODO: If we are interested in parsing other fields, then uncomment them from below
#if 0
      case CodePropField::GroupSegmentFixedSize:
        kernelMD->mCodeProps.mGroupSegmentFixedSize = atoi(buf.c_str());
        break;
      case CodePropField::PrivateSegmentFixedSize:
        kernelMD->mCodeProps.mPrivateSegmentFixedSize = atoi(buf.c_str());
        break;
      case CodePropField::KernargSegmentAlign:
        kernelMD->mCodeProps.mKernargSegmentAlign = atoi(buf.c_str());
        break;
      case CodePropField::WavefrontSize:
        kernelMD->mCodeProps.mWavefrontSize = atoi(buf.c_str());
        break;
      case CodePropField::NumSGPRs:
        kernelMD->mCodeProps.mNumSGPRs = atoi(buf.c_str());
        break;
      case CodePropField::NumVGPRs:
        kernelMD->mCodeProps.mNumVGPRs = atoi(buf.c_str());
        break;
      case CodePropField::MaxFlatWorkGroupSize:
        kernelMD->mCodeProps.mMaxFlatWorkGroupSize = atoi(buf.c_str());
        break;
      case CodePropField::IsDynamicCallStack:
        kernelMD->mCodeProps.mIsDynamicCallStack = (buf.compare("true") == 0);
        break;
      case CodePropField::IsXNACKEnabled:
        kernelMD->mCodeProps.mIsXNACKEnabled = (buf.compare("true") == 0);
        break;
      case CodePropField::NumSpilledSGPRs:
        kernelMD->mCodeProps.mNumSpilledSGPRs = atoi(buf.c_str());
        break;
      case CodePropField::NumSpilledVGPRs:
        kernelMD->mCodeProps.mNumSpilledVGPRs = atoi(buf.c_str());
        break;
#endif
      default:
        return AMD_COMGR_STATUS_SUCCESS;
    }
    return AMD_COMGR_STATUS_SUCCESS;
  }

  hsa_status_t get_code_object_custom_metadata_v2(atmi_platform_type_t platform,
                                                  void *binary, size_t binSize, int gpu) {
    amd_comgr_metadata_node_t programMD;
    amd_comgr_status_t status;
    amd_comgr_data_t binaryData;

    status  = amd_comgr_create_data(AMD_COMGR_DATA_KIND_EXECUTABLE, &binaryData);
    comgrErrorCheck(COMGR create binary data, status);

    status = amd_comgr_set_data(binaryData, binSize,
        reinterpret_cast<const char*>(binary));
    comgrErrorCheck(COMGR set binary data, status);

    status = amd_comgr_get_data_metadata(binaryData, &programMD);
    comgrErrorCheck(COMGR get program metadata, status);

    amd_comgr_release_data(binaryData);

    amd_comgr_metadata_node_t kernelsMD;
    size_t kernelsSize = 0;

    status = amd_comgr_metadata_lookup(programMD, "Kernels", &kernelsMD);
    comgrErrorCheck(COMGR kernels lookup in program metadata, status);

    status = amd_comgr_get_metadata_list_size(kernelsMD, &kernelsSize);
    comgrErrorCheck(COMGR kernels size lookup in kernels metadata, status);

    for (size_t i = 0; i < kernelsSize; i++) {
      std::string kernelName;
      std::string languageName;
      amd_comgr_metadata_node_t nameMeta;
      amd_comgr_metadata_node_t kernelMD;
      amd_comgr_metadata_node_t argsMeta;
      amd_comgr_metadata_node_t codePropsMeta;
      amd_comgr_metadata_node_t languageMeta;
      KernelMD kernelObj;

      status = amd_comgr_index_list_metadata(kernelsMD, i, &kernelMD);
      comgrErrorCheck(COMGR ith kernel in kernels metadata, status);

      status = amd_comgr_metadata_lookup(kernelMD, "Name", &nameMeta);
      comgrErrorCheck(COMGR kernel name metadata lookup in kernel metadata, status);

      status = amd_comgr_metadata_lookup(kernelMD, "Language", &languageMeta);
      comgrErrorCheck(COMGR kernel language metadata lookup in kernel metadata, status);

      status  = getMetaBuf(languageMeta, &languageName);
      comgrErrorCheck(COMGR kernel language name lookup in language metadata, status);

      status  = getMetaBuf(nameMeta, &kernelName);
      comgrErrorCheck(COMGR kernel name lookup in name metadata, status);

      // For v3, the kernel symbol name is different from the kernel name itself, so there
      // is a need for a kernel symbol->name map. However, for v2, the symbol and kernel
      // names are the same. But, to be consistent with v3's implementation, we still use
      // the kernel symbol->name map, but the key for v2 will be the kernel name itself.
      KernelNameMap[kernelName] = kernelName;

      atl_kernel_info_t info;
      size_t kernel_segment_size = 0;
      size_t kernel_explicit_args_size = 0;

      // extract the code properties metadata
      status = amd_comgr_metadata_lookup(kernelMD, "CodeProps", &codePropsMeta);
      comgrErrorCheck(COMGR code props lookup, status);

      status = amd_comgr_iterate_map_metadata(codePropsMeta, populateCodeProps,
          static_cast<void*>(&kernelObj));
      comgrErrorCheck(COMGR code props iterate, status);
      kernel_segment_size = kernelObj.mCodeProps.mKernargSegmentSize;

      bool hasHiddenArgs = false;
      if(kernel_segment_size > 0) {
        // this kernel has some arguments
        size_t offset = 0;
        size_t argsSize;

        status =  amd_comgr_metadata_lookup(kernelMD, "Args", &argsMeta);
        comgrErrorCheck(COMGR kernel args metadata lookup in kernel metadata, status);

        status = amd_comgr_get_metadata_list_size(argsMeta, &argsSize);
        comgrErrorCheck(COMGR kernel args size lookup in kernel args metadata, status);

        info.num_args = argsSize;

        for (size_t i = 0; i < argsSize; ++i) {
          KernelArgMD  lcArg;

          amd_comgr_metadata_node_t argsNode;
          amd_comgr_metadata_kind_t kind;

          status = amd_comgr_index_list_metadata(argsMeta, i, &argsNode);
          comgrErrorCheck(COMGR list args node in kernel args metadata, status);

          status = amd_comgr_get_metadata_kind(argsNode, &kind);
          comgrErrorCheck(COMGR args kind in kernel args metadata, status);

          if (kind != AMD_COMGR_METADATA_KIND_MAP) {
            status = AMD_COMGR_STATUS_ERROR;
          }
          comgrErrorCheck(COMGR check args kind in kernel args metadata, status);

          status = amd_comgr_iterate_map_metadata(argsNode, populateArgs, static_cast<void*>(&lcArg));
          comgrErrorCheck(COMGR iterate args map in kernel args metadata, status);

          amd_comgr_destroy_metadata(argsNode);

          if (status != AMD_COMGR_STATUS_SUCCESS) {
            amd_comgr_destroy_metadata(argsMeta);
            return HSA_STATUS_ERROR_INVALID_CODE_OBJECT;
          }

          // TODO: should the below population actions be done only for 
          // non-implicit args?
          // populate info with num args, their sizes and alignments
          info.arg_sizes.push_back(lcArg.mSize);
          info.arg_alignments.push_back(lcArg.mAlign);
          //  use offset with/instead of alignment
          size_t new_offset = core::alignUp(offset, lcArg.mAlign);
          size_t padding = new_offset - offset;
          offset = new_offset;
          info.arg_offsets.push_back(offset);
          DEBUG_PRINT("[%s:%lu] \"%s\" (%u, %lu)\n", kernelName.c_str(), i, lcArg.mName.c_str(), lcArg.mSize, offset);
          offset += lcArg.mSize;

          // check if the arg is a hidden/implicit arg
          // this logic assumes that all hidden args are 8-byte aligned
          if(!isImplicit(lcArg.mValueKind)) {
            kernel_explicit_args_size += lcArg.mSize;
          }
          else {
            hasHiddenArgs = true;
          }
          kernel_explicit_args_size += padding;
        }
        amd_comgr_destroy_metadata(argsMeta);
      }

      // add size of implicit args, e.g.: offset x, y and z and pipe pointer, but
      // in ATMI, do not count the compiler set implicit args, but set your own
      // implicit args by discounting the compiler set implicit args
      info.kernel_segment_size = (hasHiddenArgs ? kernel_explicit_args_size : kernelObj.mCodeProps.mKernargSegmentSize)
                                 + sizeof(atmi_implicit_args_t);
      DEBUG_PRINT("[%s: kernarg seg size] (%lu --> %u)\n", kernelName.c_str(), kernelObj.mCodeProps.mKernargSegmentSize, info.kernel_segment_size);

      // kernel received, now add it to the kernel info table
      KernelInfoTable[gpu][kernelName] = info;

      amd_comgr_destroy_metadata(codePropsMeta);
      amd_comgr_destroy_metadata(nameMeta);
      amd_comgr_destroy_metadata(kernelMD);
    }
    amd_comgr_destroy_metadata(kernelsMD);

    return HSA_STATUS_SUCCESS;
  }

  hsa_status_t get_code_object_custom_metadata_v3(atmi_platform_type_t platform,
                                                  void *binary, size_t binSize, int gpu) {
    // parse code object with different keys from v2
    // also, the kernel name is not the same as the symbol name -- so a symbol->name map is needed
    amd_comgr_metadata_node_t programMD;
    amd_comgr_status_t status;
    amd_comgr_data_t binaryData;

    status  = amd_comgr_create_data(AMD_COMGR_DATA_KIND_EXECUTABLE, &binaryData);
    comgrErrorCheck(COMGR create binary data, status);

    status = amd_comgr_set_data(binaryData, binSize,
        reinterpret_cast<const char*>(binary));
    comgrErrorCheck(COMGR set binary data, status);

    status = amd_comgr_get_data_metadata(binaryData, &programMD);
    comgrErrorCheck(COMGR get program metadata, status);

    amd_comgr_release_data(binaryData);

    amd_comgr_metadata_node_t kernelsMD;
    size_t kernelsSize = 0;

    status = amd_comgr_metadata_lookup(programMD, "amdhsa.kernels", &kernelsMD);
    comgrErrorCheck(COMGR kernels lookup in program metadata, status);

    status = amd_comgr_get_metadata_list_size(kernelsMD, &kernelsSize);
    comgrErrorCheck(COMGR kernels size lookup in kernels metadata, status);

    for (size_t i = 0; i < kernelsSize; i++) {
      std::string kernelName;
      std::string languageName;
      std::string symbolName;
      std::string kernargSegSize;
      amd_comgr_metadata_node_t symbolMeta;
      amd_comgr_metadata_node_t nameMeta;
      amd_comgr_metadata_node_t languageMeta;
      amd_comgr_metadata_node_t kernelMD;
      amd_comgr_metadata_node_t argsMeta;
      amd_comgr_metadata_node_t kernargSegSizeMeta;

      status = amd_comgr_index_list_metadata(kernelsMD, i, &kernelMD);
      comgrErrorCheck(COMGR ith kernel in kernels metadata, status);

      status = amd_comgr_metadata_lookup(kernelMD, ".name", &nameMeta);
      comgrErrorCheck(COMGR kernel name metadata lookup in kernel metadata, status);

      status  = getMetaBuf(nameMeta, &kernelName);
      comgrErrorCheck(COMGR kernel name lookup in name metadata, status);

      status = amd_comgr_metadata_lookup(kernelMD, ".language", &languageMeta);
      comgrErrorCheck(COMGR kernel language metadata lookup in kernel metadata, status);

      status  = getMetaBuf(languageMeta, &languageName);
      comgrErrorCheck(COMGR kernel language name lookup in language metadata, status);

      status = amd_comgr_metadata_lookup(kernelMD, ".symbol", &symbolMeta);
      comgrErrorCheck(COMGR kernel symbol metadata lookup in kernel metadata, status);

      status  = getMetaBuf(symbolMeta, &symbolName);
      comgrErrorCheck(COMGR kernel symbol lookup in name metadata, status);

      status = amd_comgr_metadata_lookup(kernelMD, ".kernarg_segment_size", &kernargSegSizeMeta);
      comgrErrorCheck(COMGR kernarg segment size metadata lookup in kernel metadata, status);

      status  = getMetaBuf(kernargSegSizeMeta, &kernargSegSize);
      comgrErrorCheck(COMGR kernarg segment size lookup in kernarg seg size metadata, status);

      // create a map from symbol to name
      DEBUG_PRINT("Kernel symbol %s; Name: %s; Size: %s\n",
                  symbolName.c_str(), kernelName.c_str(), kernargSegSize.c_str());
      KernelNameMap[symbolName] = kernelName;

      atl_kernel_info_t info;
      size_t kernel_explicit_args_size = 0;
      size_t kernel_segment_size = std::stoi(kernargSegSize);

      bool hasHiddenArgs = false;
      if(kernel_segment_size > 0) {
        size_t argsSize;
        size_t offset = 0;

        status =  amd_comgr_metadata_lookup(kernelMD, ".args", &argsMeta);
        comgrErrorCheck(COMGR kernel args metadata lookup in kernel metadata, status);

        status = amd_comgr_get_metadata_list_size(argsMeta, &argsSize);
        comgrErrorCheck(COMGR kernel args size lookup in kernel args metadata, status);

        info.num_args = argsSize;

        for (size_t i = 0; i < argsSize; ++i) {
          KernelArgMD  lcArg;

          amd_comgr_metadata_node_t argsNode;
          amd_comgr_metadata_kind_t kind;

          status = amd_comgr_index_list_metadata(argsMeta, i, &argsNode);
          comgrErrorCheck(COMGR list args node in kernel args metadata, status);

          status = amd_comgr_get_metadata_kind(argsNode, &kind);
          comgrErrorCheck(COMGR args kind in kernel args metadata, status);

          if (kind != AMD_COMGR_METADATA_KIND_MAP) {
            status = AMD_COMGR_STATUS_ERROR;
          }
          comgrErrorCheck(COMGR check args kind in kernel args metadata, status);

          status = amd_comgr_iterate_map_metadata(argsNode, populateArgs, static_cast<void*>(&lcArg));
          comgrErrorCheck(COMGR iterate args map in kernel args metadata, status);

          amd_comgr_destroy_metadata(argsNode);

          if (status != AMD_COMGR_STATUS_SUCCESS) {
            amd_comgr_destroy_metadata(argsMeta);
            return HSA_STATUS_ERROR_INVALID_CODE_OBJECT;
          }

          // TODO: should the below population actions be done only for 
          // non-implicit args?
          // populate info with sizes and offsets
          info.arg_sizes.push_back(lcArg.mSize);
          // FIXME: KernelArgMD does not have the mOffset field; until that changes, we use
          // mAlign to store the offset; minor "workaround"
          //info.arg_alignments.push_back(lcArg.mAlign);
          //  use offset with/instead of alignment
          // offset = lcArg.mAlign;
          size_t new_offset = lcArg.mAlign;
          size_t padding = new_offset - offset;
          offset = new_offset;
          info.arg_offsets.push_back(lcArg.mAlign);
          DEBUG_PRINT("Arg[%lu] \"%s\" (%u, %u)\n",
              i, lcArg.mName.c_str(), lcArg.mSize, lcArg.mAlign);
          offset += lcArg.mSize;

          // check if the arg is a hidden/implicit arg
          // this logic assumes that all hidden args are 8-byte aligned
          if(!isImplicit(lcArg.mValueKind)) {
            kernel_explicit_args_size += lcArg.mSize;
          }
          else {
            hasHiddenArgs = true;
          }
          kernel_explicit_args_size += padding;
        }
        amd_comgr_destroy_metadata(argsMeta);
      }

      // add size of implicit args, e.g.: offset x, y and z and pipe pointer, but
      // in ATMI, do not count the compiler set implicit args, but set your own
      // implicit args by discounting the compiler set implicit args
      info.kernel_segment_size = (hasHiddenArgs ? kernel_explicit_args_size : kernel_segment_size)
                                 + sizeof(atmi_implicit_args_t);
      DEBUG_PRINT("[%s: kernarg seg size] (%lu --> %u)\n", kernelName.c_str(), kernel_segment_size, info.kernel_segment_size);

      // kernel received, now add it to the kernel info table
      KernelInfoTable[gpu][kernelName] = info;

      amd_comgr_destroy_metadata(nameMeta);
      amd_comgr_destroy_metadata(kernelMD);
    }
    amd_comgr_destroy_metadata(kernelsMD);

    return HSA_STATUS_SUCCESS;
  }

  hsa_status_t get_code_object_custom_metadata(atmi_platform_type_t platform,
                                               void *binary, size_t binSize, int gpu) {
    int code_object_ver = 0;
    // Get the code object version by looking int the runtime metadata note
    // Begin the Elf image from memory
    Elf* e = elf_memory((char*)binary, binSize);
    if (elf_kind(e) != ELF_K_ELF) {
      ELFErrorReturn(Error while reading the ELF program binary,
          HSA_STATUS_ERROR_INVALID_CODE_OBJECT);
    }

    size_t numpHdrs;
    if (elf_getphdrnum(e, &numpHdrs) != 0) {
      ELFErrorReturn(Error while reading the ELF program binary,
          HSA_STATUS_ERROR_INVALID_CODE_OBJECT);
    }

    for (size_t i = 0; i < numpHdrs; ++i) {
      GElf_Phdr pHdr;
      if (gelf_getphdr(e, i, &pHdr) != &pHdr) {
        continue;
      }
      // Look for the runtime metadata note
      if (pHdr.p_type == PT_NOTE && pHdr.p_align >= sizeof(int)) {
        // Iterate over the notes in this segment
        address ptr = (address)binary + pHdr.p_offset;
        address segmentEnd = ptr + pHdr.p_filesz;

        while (ptr < segmentEnd) {
          Elf_Note* note = (Elf_Note*)ptr;
          address name = (address)&note[1];
          address desc = name + core::alignUp(note->n_namesz, sizeof(int));

          if (note->n_type == 7 ||
              note->n_type == 8) {
            ELFErrorReturn(Error: object code with old metadata is not supported,
                HSA_STATUS_ERROR_INVALID_CODE_OBJECT);
          }
          else if (note->n_type == 10 /* NT_AMD_AMDGPU_HSA_METADATA */ &&
              note->n_namesz == sizeof "AMD" &&
              !memcmp(name, "AMD", note->n_namesz)) {
            // code object v2
            code_object_ver = 2;
            // We've found the code object version, exit the
            // note record loop now.
            break;
          }
          else if (note->n_type == 32 /* NT_AMDGPU_METADATA */ &&
              note->n_namesz == sizeof "AMDGPU" &&
              !memcmp(name, "AMDGPU", note->n_namesz)) {
            // code object v3
            code_object_ver = 3;
            // We've found the code object version, exit the
            // note record loop now.
            break;
          }
          ptr += sizeof(*note) + core::alignUp(note->n_namesz, sizeof(int)) +
            core::alignUp(note->n_descsz, sizeof(int));
        }
      }
    }

    if(code_object_ver == 2)
      return get_code_object_custom_metadata_v2(platform, binary, binSize, gpu);
    else if(code_object_ver == 3)
      return get_code_object_custom_metadata_v3(platform, binary, binSize, gpu);
    else
      ELFErrorReturn(Error: Code object version invalid,
          HSA_STATUS_ERROR_INVALID_CODE_OBJECT);
  }

  hsa_status_t create_kernarg_memory(hsa_executable_t executable, hsa_executable_symbol_t symbol, void *data) {
    int gpu = *(int *)data;
    hsa_symbol_kind_t type;

    uint32_t name_length;
    hsa_status_t err;
    err = hsa_executable_symbol_get_info(symbol, HSA_EXECUTABLE_SYMBOL_INFO_TYPE, &type);
    ErrorCheck(Symbol info extraction, err);
    DEBUG_PRINT("Exec Symbol type: %d\n", type);
    if(type == HSA_SYMBOL_KIND_KERNEL) {
      err = hsa_executable_symbol_get_info(symbol, HSA_EXECUTABLE_SYMBOL_INFO_NAME_LENGTH, &name_length);
      ErrorCheck(Symbol info extraction, err);
      char *name = (char *)malloc(name_length + 1);
      err = hsa_executable_symbol_get_info(symbol, HSA_EXECUTABLE_SYMBOL_INFO_NAME, name);
      ErrorCheck(Symbol info extraction, err);
      name[name_length] = 0;

      if(KernelNameMap.find(std::string(name)) == KernelNameMap.end()) {
        // did not find kernel name in the kernel map; this can happen only
        // if the ROCr API for getting symbol info (name) is different from
        // the comgr method of getting symbol info
        ErrorCheck(Invalid kernel name, HSA_STATUS_ERROR_INVALID_CODE_OBJECT);
      }
      atl_kernel_info_t info;
      std::string kernelName = KernelNameMap[std::string(name)];
      // by now, the kernel info table should already have an entry
      // because the non-ROCr custom code object parsing is called before
      // iterating over the code object symbols using ROCr
      if(KernelInfoTable[gpu].find(kernelName) == KernelInfoTable[gpu].end()) {
        ErrorCheck(Finding the entry kernel info table, HSA_STATUS_ERROR_INVALID_CODE_OBJECT);
      }
      // found, so assign and update
      info = KernelInfoTable[gpu][kernelName];

      /* Extract dispatch information from the symbol */
      err = hsa_executable_symbol_get_info(symbol, HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_OBJECT, &(info.kernel_object));
      ErrorCheck(Extracting the symbol from the executable, err);
      err = hsa_executable_symbol_get_info(symbol, HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_GROUP_SEGMENT_SIZE, &(info.group_segment_size));
      ErrorCheck(Extracting the group segment size from the executable, err);
      err = hsa_executable_symbol_get_info(symbol, HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_PRIVATE_SEGMENT_SIZE, &(info.private_segment_size));
      ErrorCheck(Extracting the private segment from the executable, err);

      DEBUG_PRINT("Kernel %s --> %lx symbol %u group segsize %u pvt segsize %u bytes kernarg\n",
          kernelName.c_str(),
          info.kernel_object,
          info.group_segment_size,
          info.private_segment_size,
          info.kernel_segment_size);

      // assign it back to the kernel info table
      KernelInfoTable[gpu][kernelName] = info;
      free(name);

      /*
         void *thisKernargAddress = NULL;
      // create a memory segment for this kernel's arguments
      err = hsa_memory_allocate(atl_gpu_kernarg_region, info.kernel_segment_size * MAX_NUM_KERNELS, &thisKernargAddress);
      ErrorCheck(Allocating memory for the executable-kernel, err);
      */
    }
    else if(type == HSA_SYMBOL_KIND_VARIABLE) {
      err = hsa_executable_symbol_get_info(symbol, HSA_EXECUTABLE_SYMBOL_INFO_NAME_LENGTH, &name_length);
      ErrorCheck(Symbol info extraction, err);
      char *name = (char *)malloc(name_length + 1);
      err = hsa_executable_symbol_get_info(symbol, HSA_EXECUTABLE_SYMBOL_INFO_NAME, name);
      ErrorCheck(Symbol info extraction, err);
      name[name_length] = 0;

      atl_symbol_info_t info;

      err = hsa_executable_symbol_get_info(symbol, HSA_EXECUTABLE_SYMBOL_INFO_VARIABLE_ADDRESS, &(info.addr));
      ErrorCheck(Symbol info address extraction, err);

      err = hsa_executable_symbol_get_info(symbol, HSA_EXECUTABLE_SYMBOL_INFO_VARIABLE_SIZE, &(info.size));
      ErrorCheck(Symbol info size extraction, err);

      atmi_mem_place_t place = ATMI_MEM_PLACE(ATMI_DEVTYPE_GPU, gpu, 0);
      DEBUG_PRINT("Symbol %s = %p (%u bytes)\n", name, (void *)info.addr, info.size);
      register_allocation((void *)info.addr, (size_t)info.size, place);
      SymbolInfoTable[gpu][std::string(name)] = info;
      free(name);
    }
    else {
      DEBUG_PRINT("Symbol is an indirect function\n");
    }
    return HSA_STATUS_SUCCESS;
  }

  hsa_status_t create_kernarg_memory_agent(hsa_executable_t executable,
      hsa_agent_t agent,
      hsa_executable_symbol_t symbol, void *data) {
    return create_kernarg_memory(executable, symbol, data);
  }

  atmi_status_t Runtime::RegisterModuleFromMemory(void **modules, size_t *module_sizes, atmi_platform_type_t *types, const int num_modules) {
    hsa_status_t err;
    int some_success = 0;
    std::vector<std::string> modules_str;
    for(int i = 0; i < num_modules; i++) {
      modules_str.push_back(std::string((char *)modules[i]));
    }

    int gpu_count = g_atl_machine.getProcessorCount<ATLGPUProcessor>();

    KernelInfoTable.resize(gpu_count);
    SymbolInfoTable.resize(gpu_count);

    for(int gpu = 0; gpu < gpu_count; gpu++)
    {
      DEBUG_PRINT("Trying to load module to GPU-%d\n", gpu);
      atmi_place_t place = ATMI_PLACE_GPU(0, gpu);
      ATLGPUProcessor &proc = get_processor<ATLGPUProcessor>(place);
      hsa_agent_t agent = proc.getAgent();
      hsa_executable_t executable = {0};
      hsa_profile_t agent_profile;

      err = hsa_agent_get_info(agent, HSA_AGENT_INFO_PROFILE, &agent_profile);
      ErrorCheckAndContinue(Query the agent profile, err);
      // FIXME: Assume that every profile is FULL until we understand how to build GCN with base profile
      agent_profile = HSA_PROFILE_FULL;
      /* Create the empty executable.  */
      err = hsa_executable_create(agent_profile, HSA_EXECUTABLE_STATE_UNFROZEN, "", &executable);
      ErrorCheckAndContinue(Create the executable, err);

      // clear symbol set for every executable
      SymbolSet.clear();
      int module_load_success = 0;
      for(int i = 0; i < num_modules; i++) {
        void *module_bytes = modules[i];
        size_t module_size = module_sizes[i];
        if (types[i] == AMDGCN) {
          // Some metadata info is not available through ROCr API, so use custom
          // code object metadata parsing to collect such metadata info
          void *tmp_module = malloc(module_size);
          memcpy(tmp_module, module_bytes, module_size);
          err = get_code_object_custom_metadata(types[i], tmp_module, module_size, gpu);
          ErrorCheckAndContinue(Getting custom code object metadata, err);
          free(tmp_module);

          // Deserialize code object.
          hsa_code_object_t code_object = {0};
          err = hsa_code_object_deserialize(module_bytes, module_size, NULL, &code_object);
          ErrorCheckAndContinue(Code Object Deserialization, err);
          assert(0 != code_object.handle);

          err = hsa_code_object_iterate_symbols(code_object, validate_code_object, &gpu);
          ErrorCheckAndContinue(Iterating over symbols for execuatable, err);

          /* Load the code object.  */
          err = hsa_executable_load_code_object(executable, agent, code_object, NULL);
          ErrorCheckAndContinue(Loading the code object, err);

          //cannot iterate over symbols until executable is frozen
          //err = hsa_executable_iterate_agent_symbols(executable, agent,
          //    create_kernarg_memory_agent, &gpu);
          //ErrorCheckAndContinue(Iterating over symbols for execuatable, err);

        }
        else {
          ErrorCheckAndContinue(Loading non-AMDGCN code object, HSA_STATUS_ERROR_INVALID_CODE_OBJECT);
        }
        module_load_success = 1;
      }
      if(!module_load_success) continue;

      /* Freeze the executable; it can now be queried for symbols.  */
      err = hsa_executable_freeze(executable, "");
      ErrorCheckAndContinue(Freeze the executable, err);

      err = hsa_executable_iterate_symbols(executable, create_kernarg_memory, &gpu);
      ErrorCheckAndContinue(Iterating over symbols for execuatable, err);

      //err = hsa_executable_iterate_program_symbols(executable, iterate_program_symbols, &gpu);
      //ErrorCheckAndContinue(Iterating over symbols for execuatable, err);

      //err = hsa_executable_iterate_agent_symbols(executable, iterate_agent_symbols, &gpu);
      //ErrorCheckAndContinue(Iterating over symbols for execuatable, err);

      // save the executable and destroy during finalize
      g_executables.push_back(executable);
      some_success = 1;
    }
    DEBUG_PRINT("Modules loaded successful? %d\n", some_success);
    //ModuleMap[executable.handle] = modules_str;
    return (some_success) ? ATMI_STATUS_SUCCESS : ATMI_STATUS_ERROR;
  }

  atmi_status_t Runtime::RegisterModule(const char **filenames, atmi_platform_type_t *types, const int num_modules) {
    std::vector<void *> modules;
    std::vector<size_t> module_sizes;
    for(int i = 0; i < num_modules; i++) {
      size_t module_size;
      void *module_bytes = atl_read_binary_from_file(filenames[i], &module_size);
      if(!module_bytes) return ATMI_STATUS_ERROR;
      modules.push_back(module_bytes);
      module_sizes.push_back(module_size);
    }

    atmi_status_t status = atmi_module_register_from_memory(&modules[0], &module_sizes[0], types, num_modules);

    // memory space got by
    // void *raw_code_object = malloc(size);
    for(int i = 0; i < num_modules; i++) {
      free(modules[i]);
    }

    return status;
  }

  atmi_status_t Runtime::CreateEmptyKernel(atmi_kernel_t *atmi_kernel, const int num_args,
      const size_t *arg_sizes) {
    static uint64_t counter = 0;
    uint64_t pif_id = ++counter;
    atmi_kernel->handle = (uint64_t)pif_id;

    atl_kernel_t *kernel = new atl_kernel_t;
    kernel->id_map.clear();
    kernel->num_args = num_args;
    for(int i = 0; i < num_args; i++) {
      kernel->arg_sizes.push_back(arg_sizes[i]);
    }
    clear_container(kernel->impls);
    kernel->pif_id = pif_id;
    KernelImplMap[pif_id] = kernel;
    return ATMI_STATUS_SUCCESS;
  }

  atmi_status_t Runtime::ReleaseKernel(atmi_kernel_t atmi_kernel) {
    uint64_t pif_id = atmi_kernel.handle;

    atl_kernel_t *kernel = KernelImplMap[pif_id];
    //kernel->id_map.clear();
    clear_container(kernel->arg_sizes);
    for(std::vector<atl_kernel_impl_t *>::iterator it = kernel->impls.begin();
        it != kernel->impls.end(); it++) {
      lock(&((*it)->mutex));
      if((*it)->devtype == ATMI_DEVTYPE_GPU) {
        // free the pipe_ptrs data
        // We create the pipe_ptrs region for all kernel instances
        // combined, and each instance of the kernel
        // invocation takes a piece of it. So, the first kernel instance
        // (k=0) will have the pointer to the entire pipe region itself.
        atmi_implicit_args_t *impl_args = (atmi_implicit_args_t *)(((char *)(*it)->kernarg_region) + (*it)->kernarg_segment_size - sizeof(atmi_implicit_args_t));
        void *pipe_ptrs = (void *)impl_args->pipe_ptr;
        DEBUG_PRINT("Freeing pipe ptr: %p\n", pipe_ptrs);
        hsa_memory_free(pipe_ptrs);
        hsa_memory_free((*it)->kernarg_region);
        free((*it)->kernel_objects);
        free((*it)->group_segment_sizes);
        free((*it)->private_segment_sizes);
      }
      else if((*it)->devtype == ATMI_DEVTYPE_CPU) {
        free((*it)->kernarg_region);
      }
      clear_container((*it)->free_kernarg_segments);
      unlock(&((*it)->mutex));
      delete *it;
    }
    clear_container(kernel->impls);
    delete kernel;

    KernelImplMap.erase(pif_id);
    atmi_kernel.handle = 0ull;
    return ATMI_STATUS_SUCCESS;
  }

  atmi_status_t Runtime::CreateKernel(atmi_kernel_t *atmi_kernel, const int num_args,
      const size_t *arg_sizes, const int num_impls, va_list arguments) {
    atmi_status_t status;
    hsa_status_t err;
    if(!atl_is_atmi_initialized()) return ATMI_STATUS_ERROR;
    status = atmi_kernel_create_empty(atmi_kernel, num_args, arg_sizes);
    ATMIErrorCheck(Creating kernel object, status);

    static int counter = 0;
    bool has_gpu_impl = false;
    uint64_t pif_id = atmi_kernel->handle;
    atl_kernel_t *kernel = KernelImplMap[pif_id];
    size_t max_kernarg_segment_size = 0;
    //va_list arguments;
    //va_start(arguments, num_impls);
    for(int impl_id = 0; impl_id < num_impls; impl_id++) {
      atmi_devtype_t devtype = (atmi_devtype_t)va_arg(arguments, int);
      if(devtype == ATMI_DEVTYPE_GPU) {
        const char *impl = va_arg(arguments, const char *);
        status = atmi_kernel_add_gpu_impl(*atmi_kernel, impl, impl_id);
        ATMIErrorCheck(Adding GPU kernel implementation, status);
        DEBUG_PRINT("GPU kernel %s added [%u]\n", impl, impl_id);
        has_gpu_impl = true;
      }
      else if(devtype == ATMI_DEVTYPE_CPU) {
        atmi_generic_fp impl = va_arg(arguments, atmi_generic_fp);
        status = atmi_kernel_add_cpu_impl(*atmi_kernel, impl, impl_id);
        ATMIErrorCheck(Adding CPU kernel implementation, status);
        DEBUG_PRINT("CPU kernel %p added [%u]\n", impl, impl_id);
      }
      else {
        fprintf(stderr, "Unsupported device type: %d\n", devtype);
        return ATMI_STATUS_ERROR;
      }
      size_t this_kernarg_segment_size = kernel->impls[impl_id]->kernarg_segment_size;
      if(this_kernarg_segment_size > max_kernarg_segment_size)
        max_kernarg_segment_size = this_kernarg_segment_size;
      ATMIErrorCheck(Creating kernel implementations, status);
      // rest of kernel impl fields will be populated at first kernel launch
    }
    //va_end(arguments);
    //// FIXME EEEEEE: for EVERY GPU impl, add all CPU/GPU implementations in
    //their templates!!!
    if(has_gpu_impl) {
      // fill in the kernel template AQL packets
      int cur_kernel = g_ke_args.kernel_counter++;
      // FIXME: reformulate this for debug mode
      // assert(cur_kernel < core::Runtime::getInstance().getMaxKernelTypes());
      int max_kernel_types = core::Runtime::getInstance().getMaxKernelTypes();
      if (cur_kernel >= max_kernel_types) {
        fprintf(stderr, "Creating more than %d task types. Set "
            "ATMI_MAX_KERNEL_TYPES environment variable to a larger "
            "number and try again.\n",
            max_kernel_types);
        return ATMI_STATUS_KERNELCOUNT_OVERFLOW;
      }
      // populate the AQL packet template for GPU kernel impls
      void *ke_kernarg_region;
      // first 4 bytes store the current index of the kernel arg region
      err = hsa_amd_memory_pool_allocate(atl_gpu_kernarg_pool,
          sizeof(int) + max_kernarg_segment_size * MAX_NUM_KERNELS,
          0,
          &ke_kernarg_region);
      ErrorCheck(Allocating memory for the executable-kernel, err);
      allow_access_to_all_gpu_agents(ke_kernarg_region);
      *(int *)ke_kernarg_region = 0;
      char *ke_kernargs = (char *)ke_kernarg_region + sizeof(int);

      atmi_kernel_enqueue_template_t *ke_template = &((atmi_kernel_enqueue_template_t *)g_ke_args.kernarg_template_ptr)[cur_kernel];
      ke_template->kernel_handle = atmi_kernel->handle; // To be used by device code to pick a task template

      // fill in the kernel arg regions
      ke_template->kernarg_segment_size = max_kernarg_segment_size;
      ke_template->kernarg_regions = ke_kernarg_region;

      std::vector<atl_kernel_impl_t *>::iterator impl_it;
      int this_impl_id = 0;
      for(impl_it = kernel->impls.begin(); impl_it != kernel->impls.end(); impl_it++) {
        atl_kernel_impl_t *this_impl = *impl_it;
        if(this_impl->devtype == ATMI_DEVTYPE_GPU) {
          // fill in the GPU AQL template
          hsa_kernel_dispatch_packet_t *k_packet = &(ke_template->k_packet);
          k_packet->header = 0; //ATMI_DEVTYPE_GPU;
          k_packet->kernarg_address = NULL;
          k_packet->kernel_object = this_impl->kernel_objects[0];
          k_packet->private_segment_size = this_impl->private_segment_sizes[0];
          k_packet->group_segment_size = this_impl->group_segment_sizes[0];
        }
        else if(this_impl->devtype == ATMI_DEVTYPE_CPU) {

          // fill in the CPU AQL template
          hsa_agent_dispatch_packet_t *a_packet = &(ke_template->a_packet);
          a_packet->header = 0; //ATMI_DEVTYPE_CPU;
          a_packet->type = (uint16_t) this_impl_id;
          /* FIXME: We are considering only void return types for now.*/
          //a_packet->return_address = NULL;
          /* Set function args */
          a_packet->arg[0] = (uint64_t) ATMI_NULL_TASK_HANDLE;
          a_packet->arg[1] = (uint64_t) NULL;
          a_packet->arg[2] = (uint64_t) kernel; // pass task handle to fill in metrics
          a_packet->arg[3] = 0; // tasks can query for current task ID
        }
        this_impl_id++;
      }
      for(impl_it = kernel->impls.begin(); impl_it != kernel->impls.end(); impl_it++) {
        atl_kernel_impl_t *this_impl = *impl_it;
        if(this_impl->devtype == ATMI_DEVTYPE_GPU) {
          for(int k = 0; k < MAX_NUM_KERNELS; k++) {
            atmi_implicit_args_t *impl_args = (atmi_implicit_args_t *)((char *)this_impl->kernarg_region + (((k + 1) * this_impl->kernarg_segment_size) - sizeof(atmi_implicit_args_t)));
            // fill in the queue
            impl_args->num_gpu_queues = g_ke_args.num_gpu_queues;
            impl_args->gpu_queue_ptr = (uint64_t) g_ke_args.gpu_queue_ptr;
            impl_args->num_cpu_queues = g_ke_args.num_cpu_queues;
            impl_args->cpu_queue_ptr = (uint64_t) g_ke_args.cpu_queue_ptr;
            impl_args->cpu_worker_signals = (uint64_t) g_ke_args.cpu_worker_signals;

            // fill in the signals?
            impl_args->kernarg_template_ptr = (uint64_t)g_ke_args.kernarg_template_ptr;

            // *** fill in implicit args for kernel enqueue ***
            atmi_implicit_args_t *ke_impl_args = (atmi_implicit_args_t *)((char *)ke_kernargs + (((k + 1) * this_impl->kernarg_segment_size) - sizeof(atmi_implicit_args_t)));
            // SHARE the same pipe for printf etc
            *ke_impl_args = *impl_args;
          }
        }
        // CPU impls dont use implicit args for now
      }
    }
    return ATMI_STATUS_SUCCESS;
  }

  atmi_status_t Runtime::AddGPUKernelImpl(atmi_kernel_t atmi_kernel, const char *impl, const unsigned int ID) {
    if(!atl_is_atmi_initialized() || KernelInfoTable.empty()) return ATMI_STATUS_ERROR;
    uint64_t pif_id = atmi_kernel.handle;
    atl_kernel_t *kernel = KernelImplMap[pif_id];
    if(kernel->id_map.find(ID) != kernel->id_map.end()) {
      fprintf(stderr, "Kernel ID %d already found\n", ID);
      return ATMI_STATUS_ERROR;
    }
    std::string hsaco_name = std::string(impl );

    atl_kernel_impl_t *kernel_impl = new atl_kernel_impl_t;
    kernel_impl->kernel_id = ID;
    unsigned int kernel_mapped_id = kernel->impls.size();
    kernel_impl->devtype = ATMI_DEVTYPE_GPU;

    std::vector<ATLGPUProcessor> &gpu_procs = g_atl_machine.getProcessors<ATLGPUProcessor>();
    int gpu_count = gpu_procs.size();
    kernel_impl->kernel_objects = (uint64_t *)malloc(sizeof(uint64_t) * gpu_count);
    kernel_impl->group_segment_sizes = (uint32_t *)malloc(sizeof(uint32_t) * gpu_count);
    kernel_impl->private_segment_sizes = (uint32_t *)malloc(sizeof(uint32_t) * gpu_count);
    int max_kernarg_segment_size = 0;
    std::string kernel_name;
    atmi_platform_type_t kernel_type;
    bool some_success = false;
    for(int gpu = 0; gpu < gpu_count; gpu++) {
      if(KernelInfoTable[gpu].find(hsaco_name) != KernelInfoTable[gpu].end()) {
        DEBUG_PRINT("Found kernel %s for GPU %d\n", hsaco_name.c_str(), gpu);
        kernel_name = hsaco_name;
        kernel_type = AMDGCN;
        some_success = true;
      }
      else {
        DEBUG_PRINT("Did NOT find kernel %s for GPU %d\n",
            hsaco_name.c_str(),
            gpu);
        continue;
      }
      atl_kernel_info_t info = KernelInfoTable[gpu][kernel_name];
      kernel_impl->kernel_objects[gpu] = info.kernel_object;
      kernel_impl->group_segment_sizes[gpu] = info.group_segment_size;
      kernel_impl->private_segment_sizes[gpu] = info.private_segment_size;
      if(max_kernarg_segment_size < info.kernel_segment_size)
        max_kernarg_segment_size = info.kernel_segment_size;
    }
    if(!some_success) return ATMI_STATUS_ERROR;
    kernel_impl->kernel_name = kernel_name;
    kernel_impl->kernel_type = kernel_type;
    kernel_impl->kernarg_segment_size = max_kernarg_segment_size;
    atl_kernel_info_t any_gpu_info = KernelInfoTable[0][kernel_name];
    for(int i = 0; i < kernel->num_args; i++) {
      kernel_impl->arg_offsets.push_back(any_gpu_info.arg_offsets[i]);
    }
    /* create kernarg memory */
    kernel_impl->kernarg_region = NULL;
#ifdef MEMORY_REGION
    hsa_status_t err = hsa_memory_allocate(atl_gpu_kernarg_region,
        kernel_impl->kernarg_segment_size * MAX_NUM_KERNELS,
        &(kernel_impl->kernarg_region));
    ErrorCheck(Allocating memory for the executable-kernel, err);
#else
    if(kernel_impl->kernarg_segment_size > 0) {
      DEBUG_PRINT("New kernarg segment size: %u\n", kernel_impl->kernarg_segment_size);
      hsa_status_t err = hsa_amd_memory_pool_allocate(atl_gpu_kernarg_pool,
          kernel_impl->kernarg_segment_size * MAX_NUM_KERNELS,
          0,
          &(kernel_impl->kernarg_region));
      ErrorCheck(Allocating memory for the executable-kernel, err);
      allow_access_to_all_gpu_agents(kernel_impl->kernarg_region);

      void *pipe_ptrs;
      // allocate pipe memory in the kernarg memory pool
      // TODO: may be possible to allocate this on device specific
      // memory but data movement will have to be done later by
      // post-processing kernel on destination agent.
      err = hsa_amd_memory_pool_allocate(atl_gpu_kernarg_pool,
          MAX_PIPE_SIZE * MAX_NUM_KERNELS,
          0,
          &pipe_ptrs);
      ErrorCheck(Allocating pipe memory region, err);
      DEBUG_PRINT("Allocating pipe ptr: %p\n", pipe_ptrs);
      allow_access_to_all_gpu_agents(pipe_ptrs);

      for(int k = 0; k < MAX_NUM_KERNELS; k++) {
        atmi_implicit_args_t *impl_args = (atmi_implicit_args_t *)((char *)kernel_impl->kernarg_region + (((k + 1) * kernel_impl->kernarg_segment_size) - sizeof(atmi_implicit_args_t)));
        impl_args->pipe_ptr = (uint64_t)((char *)pipe_ptrs + (k * MAX_PIPE_SIZE));
        impl_args->offset_x = 0;
        impl_args->offset_y = 0;
        impl_args->offset_z = 0;
      }
    }

#endif
    for(int i = 0; i < MAX_NUM_KERNELS; i++) {
      kernel_impl->free_kernarg_segments.push(i);
    }
    pthread_mutex_init(&(kernel_impl->mutex), NULL);

    kernel->id_map[ID] = kernel_mapped_id;

    kernel->impls.push_back(kernel_impl);
    // rest of kernel impl fields will be populated at first kernel launch
    return ATMI_STATUS_SUCCESS;
  }

  atmi_status_t Runtime::AddCPUKernelImpl(atmi_kernel_t atmi_kernel, atmi_generic_fp impl, const unsigned int ID) {
    if(!atl_is_atmi_initialized()) return ATMI_STATUS_ERROR;
    static int counter = 0;
    uint64_t pif_id = atmi_kernel.handle;
    std::string cl_pif_name("_x86_");
    cl_pif_name += std::to_string(counter);
    cl_pif_name += std::string("_");
    cl_pif_name += std::to_string(pif_id);
    counter++;

    atl_kernel_impl_t *kernel_impl = new atl_kernel_impl_t;
    kernel_impl->kernel_id = ID;
    kernel_impl->kernel_name = cl_pif_name;
    kernel_impl->devtype = ATMI_DEVTYPE_CPU;
    kernel_impl->function = impl;

    atl_kernel_t *kernel = KernelImplMap[pif_id];
    if(kernel->id_map.find(ID) != kernel->id_map.end()) {
      fprintf(stderr, "Kernel ID %d already found\n", ID);
      return ATMI_STATUS_ERROR;
    }
    kernel->id_map[ID] = kernel->impls.size();
    /* create kernarg memory */
    uint32_t kernarg_size = 0;
    for(int i = 0; i < kernel->num_args; i++){
      kernel_impl->arg_offsets.push_back(kernarg_size);
      kernarg_size += kernel->arg_sizes[i];
    }
    kernel_impl->kernarg_segment_size = kernarg_size;
    kernel_impl->kernarg_region = NULL;
    if(kernarg_size)
      kernel_impl->kernarg_region = malloc(kernel_impl->kernarg_segment_size * MAX_NUM_KERNELS);
    for(int i = 0; i < MAX_NUM_KERNELS; i++) {
      kernel_impl->free_kernarg_segments.push(i);
    }

    pthread_mutex_init(&(kernel_impl->mutex), NULL);
    kernel->impls.push_back(kernel_impl);
    // rest of kernel impl fields will be populated at first kernel launch
    return ATMI_STATUS_SUCCESS;
  }

  bool is_valid_kernel_id(atl_kernel_t *kernel, unsigned int kernel_id) {
    std::map<unsigned int, unsigned int>::iterator it = kernel->id_map.find(kernel_id);
    if(it == kernel->id_map.end()) {
      fprintf(stderr, "ERROR: Kernel not found\n");
      return false;
    }
    int idx = it->second;
    if(idx >= kernel->impls.size()) {
      fprintf(stderr, "Kernel ID %d out of bounds (%lu)\n", kernel_id, kernel->impls.size());
      return false;
    }
    return true;
  }

  int get_kernel_index(atl_kernel_t *kernel, unsigned int kernel_id) {
    if(!is_valid_kernel_id(kernel, kernel_id)) {
      return -1;
    }
    return kernel->id_map[kernel_id];
  }

  atl_kernel_impl_t *get_kernel_impl(atl_kernel_t *kernel, unsigned int kernel_id) {
    int idx = get_kernel_index(kernel, kernel_id);
    if(idx < 0) {
      fprintf(stderr, "Incorrect Kernel ID %d\n", kernel_id);
      return NULL;
    }

    return kernel->impls[idx];
  }



} // namespace core
