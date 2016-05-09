#include "ATLMachine.h"
#include <stdio.h>
#include <stdlib.h>

#define ErrorCheck(msg, status) \
if (status != HSA_STATUS_SUCCESS) { \
    printf("%s failed: %d\n", #msg, status); \
    exit(1); \
} else { \
 /*  printf("%s succeeded.\n", #msg);*/ \
}

void *ATLMemory::alloc(size_t sz) {
    void *ret;
    hsa_status_t err = hsa_amd_memory_pool_allocate(_memory_pool, sz, 0, &ret);
    ErrorCheck(Allocate from memory pool, err);
    return ret;
}

void ATLMemory::free(void *ptr) {
    hsa_status_t err = hsa_amd_memory_pool_free(ptr);
    ErrorCheck(Allocate from memory pool, err);
}

/*atmi_task_handle_t ATLMemory::copy(void *dest, void *ATLMemory &m, bool async) {
    atmi_task_handle_t ret_task;

    if(async) atmi_task_wait(ret_task);
    return ret_task;
}*/


template<> void ATLProcessor::addMemory(ATLFineMemory &mem) {
    _dram_memories.push_back(mem);
}

template<> void ATLProcessor::addMemory(ATLCoarseMemory &mem) {
    _gddr_memories.push_back(mem);
}

template<> std::vector<ATLFineMemory> &ATLProcessor::getMemories() {
    return _dram_memories;
}

template<> std::vector<ATLCoarseMemory> &ATLProcessor::getMemories() {
    return _gddr_memories;
}

template <> std::vector<ATLCPUProcessor> &ATLMachine::getProcessors() { 
    return _cpu_processors; 
}

template <> std::vector<ATLGPUProcessor> &ATLMachine::getProcessors() { 
    return _gpu_processors; 
}

template <> std::vector<ATLDSPProcessor> &ATLMachine::getProcessors() { 
    return _dsp_processors; 
}

template <> void ATLMachine::addProcessor(ATLCPUProcessor &p) {
    _cpu_processors.push_back(p);
}

template <> void ATLMachine::addProcessor(ATLGPUProcessor &p) {
    _gpu_processors.push_back(p);
}

template <> void ATLMachine::addProcessor(ATLDSPProcessor &p) {
    _dsp_processors.push_back(p);
}
