#include "ATLData.h"
#include <stdio.h>
#include <string.h>
#include <hsa.h>
#include <hsa_ext_amd.h>
#include "ATLMachine.h"
#include <vector>
#include "atmi_runtime.h"
#include "atl_internal.h"

extern ATLMachine g_atl_machine;
extern hsa_signal_t IdentityCopySignal;

std::map<void *, ATLData *> MemoryMap;

#define ErrorCheck(msg, status) \
if (status != HSA_STATUS_SUCCESS) { \
    printf("%s failed: 0x%x\n", #msg, status); \
    exit(1); \
} else { \
 /*  printf("%s succeeded.\n", #msg);*/ \
}

using namespace std;

hsa_amd_memory_pool_t get_memory_pool(ATLProcessor &proc, const int mem_id) {
    hsa_amd_memory_pool_t pool;
    vector<ATLFineMemory> f_mems = proc.getMemories<ATLFineMemory>();
    vector<ATLCoarseMemory> c_mems = proc.getMemories<ATLCoarseMemory>();
    if(mem_id < f_mems.size()) {
        pool = f_mems[mem_id].getMemory(); 
    }
    else {
        pool = c_mems[mem_id - f_mems.size()].getMemory(); 
    }
    return pool;
}

ATLProcessor get_processor(atmi_mem_place_t place) {
    int dev_id = place.dev_id;
    switch(place.dev_type) {
        case ATMI_DEVTYPE_CPU:
            return g_atl_machine.getProcessors<ATLCPUProcessor>()[dev_id];
        case ATMI_DEVTYPE_GPU: 
            return g_atl_machine.getProcessors<ATLGPUProcessor>()[dev_id];
        case ATMI_DEVTYPE_DSP: 
            return g_atl_machine.getProcessors<ATLDSPProcessor>()[dev_id];
    }
}

hsa_agent_t get_agent(atmi_mem_place_t place) {
    return get_processor(place).getAgent();
}

hsa_amd_memory_pool_t get_memory_pool(atmi_mem_place_t place) {
    ATLProcessor proc = get_processor(place);
    return get_memory_pool(proc, place.mem_id);
}
#if 0
atmi_status_t atmi_data_map_sync(void *ptr, size_t size, atmi_mem_place_t place, atmi_arg_type_t arg_type, void **mapped_ptr) {
    if(!mapped_ptr || !ptr) return ATMI_STATUS_ERROR; 
    hsa_status_t err;
    #if 1
    hsa_amd_memory_pool_t dev_pool = get_memory_pool(place);
    err = hsa_amd_memory_pool_allocate(dev_pool, size, 0, mapped_ptr);
    ErrorCheck(Host staging buffer alloc, err);

    if(arg_type != ATMI_OUT) {
        atmi_mem_place_t cpu_place = {0, ATMI_DEVTYPE_CPU, 0, 0}; 
        void *host_ptr;
        hsa_amd_memory_pool_t host_pool = get_memory_pool(cpu_place);
        err = hsa_amd_memory_pool_allocate(host_pool, size, 0, &host_ptr);
        ErrorCheck(Host staging buffer alloc, err);
        memcpy(host_ptr, ptr, size);

        hsa_signal_add_acq_rel(IdentityCopySignal, 1);
        err = hsa_amd_memory_async_copy(*mapped_ptr, get_agent(place),
                host_ptr, get_agent(cpu_place),
                size, 
                0, NULL, IdentityCopySignal);
        ErrorCheck(Copy async between memory pools, err);
        hsa_signal_wait_acquire(IdentityCopySignal, HSA_SIGNAL_CONDITION_EQ, 0, UINT64_MAX, ATMI_WAIT_STATE);

        err = hsa_amd_memory_pool_free(host_ptr);
        ErrorCheck(atmi_data_create_sync, err);
    }
    #else
    // 1) Lock ptr to a host mem pool -- get a staging buffer
    // 2a) allow access to dest mem pool? 
    // 2b) Copy it to dest mem pool
    // 3) delete staging buffer? 
    //
    // OR
    
    void *agent_ptr; 
    hsa_agent_t dest_agent = get_agent(place);
    // 1) Lock ptr to dest mem pool (poorer performance via PCIe)
    err = hsa_amd_memory_lock(ptr, data->size, &dest_agent, 1, &agent_ptr);
    ErrorCheck(Locking the host ptr, err);
    // 2) Copy to dest mem pool
    // 3) unlock ptr? 
    data->ptr = agent_ptr;
    #endif
    // TODO: register ptr in a pointer map 
    ATLData *m = new ATLData(*mapped_ptr, ptr, size, place, arg_type);
    MemoryMap[*mapped_ptr] = m;

}

atmi_status_t atmi_data_unmap_sync(void *ptr, void *mapped_ptr) {
    if(!mapped_ptr || !ptr) return ATMI_STATUS_ERROR; 
    ATLData *m = MemoryMap[mapped_ptr];
    if(m->getHostAliasPtr() != ptr) return ATMI_STATUS_ERROR;
    if(m->getSize() == 0) return ATMI_STATUS_ERROR;

    hsa_status_t err;
    if(m->getArgType() != ATMI_IN) {
        atmi_mem_place_t place = m->getPlace();
        // 1) copy data to staging buffer of ptr
        // 2) unlock staging buffer
        // OR
        //
        void *host_ptr;
        atmi_mem_place_t cpu_place = {0, ATMI_DEVTYPE_CPU, 0, 0}; 
        hsa_amd_memory_pool_t host_pool = get_memory_pool(cpu_place);
        err = hsa_amd_memory_pool_allocate(host_pool, m->getSize(), 0, &host_ptr);
        ErrorCheck(Host staging buffer alloc, err);

        hsa_signal_add_acq_rel(IdentityCopySignal, 1);
        err = hsa_amd_memory_async_copy(host_ptr, get_agent(cpu_place),
                mapped_ptr, get_agent(m->getPlace()),
                m->getSize(), 
                0, NULL, IdentityCopySignal);
        ErrorCheck(Copy async between memory pools, err);
        hsa_signal_wait_acquire(IdentityCopySignal, HSA_SIGNAL_CONDITION_EQ, 0, UINT64_MAX, ATMI_WAIT_STATE);

        memcpy(ptr, host_ptr, m->getSize());

        err = hsa_amd_memory_pool_free(host_ptr);
        ErrorCheck(atmi_data_create_sync, err);
    }
    err = hsa_amd_memory_pool_free(mapped_ptr);
    ErrorCheck(atmi_data_create_sync, err);

// 1) if directly locked, then simply unlock
    delete m;
    MemoryMap.erase(mapped_ptr);

    return ATMI_STATUS_SUCCESS;
}

atmi_status_t atmi_data_copy_sync(atmi_data_t *dest, const atmi_data_t *src) {
    if(!dest || !src) return ATMI_STATUS_ERROR;
    if(dest->size != src->size) return ATMI_STATUS_ERROR;

    atmi_mem_place_t dest_place = dest->place;
    atmi_mem_place_t src_place = src->place;
    hsa_status_t err;
    hsa_signal_add_acq_rel(IdentityCopySignal, 1);
    err = hsa_amd_memory_async_copy(dest->ptr, get_agent(dest_place),
                              src->ptr, get_agent(src_place),
                              src->size, 
                              0, NULL, IdentityCopySignal);
	ErrorCheck(Copy async between memory pools, err);
    hsa_signal_wait_acquire(IdentityCopySignal, HSA_SIGNAL_CONDITION_EQ, 0, UINT64_MAX, ATMI_WAIT_STATE);
    return ATMI_STATUS_SUCCESS;
}                             

atmi_status_t atmi_data_copy_h2d_sync(atmi_data_t *dest, void *src, size_t size) {
    void *agent_ptr; 
    hsa_status_t err;
    atmi_status_t ret = ATMI_STATUS_SUCCESS;
    hsa_agent_t dest_agent = get_agent(dest->place);
    err = hsa_amd_memory_lock(src, data->size, &dest_agent, 1, &agent_ptr);
    ErrorCheck(Locking the host ptr, err);

    hsa_signal_add_acq_rel(IdentityCopySignal, 1);
    err = hsa_amd_memory_async_copy(dest->ptr, dest_agent,
                              agent_ptr, dest_agent,
                              size, 
                              0, NULL, IdentityCopySignal);
	ErrorCheck(Copy async between memory pools, err);
    hsa_signal_wait_acquire(IdentityCopySignal, HSA_SIGNAL_CONDITION_EQ, 0, UINT64_MAX, ATMI_WAIT_STATE);

    if(err != HSA_STATUS_SUCCESS || ret != ATMI_STATUS_SUCCESS) ret = ATMI_STATUS_ERROR;
    return ret;
}

atmi_status_t atmi_data_create_sync(atmi_data_t **data, void *host_ptr, size_t size, atmi_mem_place_t place, atmi_arg_type_t arg_type = ATMI_IN_OUT) {
    if(!data) return ATMI_STATUS_ERROR;
    if(size <= 0) return ATMI_STATUS_ERROR;

    atmi_status_t ret = ATMI_STATUS_SUCCESS;
    void *d_ptr;
    hsa_amd_memory_pool_t pool = get_memory_pool(place);
    hsa_status_t err = hsa_amd_memory_pool_allocate(pool, size, 0, &d_ptr);
    ErrorCheck(atmi_data_create_sync, err);
   
    ret = atmi_data_copy_h2d_sync(*data, host_ptr, size);
    *data = new atmi_data_t;
    *data->ptr = d_ptr;
    *data->size = size;
    *data->place = place;
    ATLData *m = new ATLData(*data->ptr, NULL, *data->size, *data->place, arg_type);
    MemoryMap[*data] = m;

    if(err != HSA_STATUS_SUCCESS || ret != ATMI_STATUS_SUCCESS) ret = ATMI_STATUS_ERROR;
    return ret;
}

atmi_status_t atmi_data_destroy_sync(atmi_data_t *data) {
    if(!data) return ATMI_STATUS_ERROR;
    if(MemoryMap.find(data) == MemoryMap.end()) return ATMI_STATUS_ERROR;
    atmi_status_t ret = ATMI_STATUS_SUCCESS;

    ATLData *m = MemoryMap[data];
    delete m;
    MemoryMap.erase(data);

    hsa_status_t err = hsa_amd_memory_pool_free(data->ptr);
    ErrorCheck(atmi_data_create_sync, err);
    delete data;

    if(err != HSA_STATUS_SUCCESS) ret = ATMI_STATUS_ERROR;
    return ret;

}
#endif 

atmi_status_t atmi_malloc(void **ptr, size_t size, atmi_mem_place_t place) {
    atmi_status_t ret = ATMI_STATUS_SUCCESS;
    hsa_amd_memory_pool_t pool = get_memory_pool(place);
    hsa_status_t err = hsa_amd_memory_pool_allocate(pool, size, 0, ptr);
    ErrorCheck(atmi_malloc, err);
    
    if(err != HSA_STATUS_SUCCESS) ret = ATMI_STATUS_ERROR;
    return ret;
}

atmi_status_t atmi_free(void *ptr) {
    atmi_status_t ret = ATMI_STATUS_SUCCESS;
    hsa_status_t err = hsa_amd_memory_pool_free(ptr);
    ErrorCheck(atmi_free, err);
    
    if(err != HSA_STATUS_SUCCESS) ret = ATMI_STATUS_ERROR;
    return ret;
}

atmi_status_t atmi_copy_d2h(void *dest, const void *src, size_t size, atmi_mem_place_t place) {
    void *agent_ptr; 
    hsa_status_t err;
    atmi_status_t ret = ATMI_STATUS_SUCCESS;
    hsa_agent_t src_agent = get_agent(place);
    err = hsa_amd_memory_lock(dest, size, &src_agent, 1, &agent_ptr);
    ErrorCheck(Locking the host ptr, err);
    hsa_signal_add_acq_rel(IdentityCopySignal, 1);
    err = hsa_amd_memory_async_copy(
                              agent_ptr, src_agent,
                              src, src_agent,
                              size, 
                              0, NULL, IdentityCopySignal);
	ErrorCheck(Copy async between memory pools, err);
    hsa_signal_wait_acquire(IdentityCopySignal, HSA_SIGNAL_CONDITION_EQ, 0, UINT64_MAX, ATMI_WAIT_STATE);

    err = hsa_amd_memory_unlock(dest);
    ErrorCheck(Unlocking the host ptr, err);
    if(err != HSA_STATUS_SUCCESS || ret != ATMI_STATUS_SUCCESS) ret = ATMI_STATUS_ERROR;
    return ret;
}

atmi_status_t atmi_copy_h2d(void *dest, const void *src, size_t size, atmi_mem_place_t place) {
    void *agent_ptr; 
    hsa_status_t err;
    atmi_status_t ret = ATMI_STATUS_SUCCESS;
    hsa_agent_t dest_agent = get_agent(place);
    hsa_amd_memory_pool_t pool = get_memory_pool(place);
    #if 1
    ret = atmi_malloc(&agent_ptr, size, ATMI_MEM_PLACE_CPU_MEM(0, 0, 0));
    memcpy(agent_ptr, src, size);

    // FIXME: ideally lock would be the better approach, but we need to try to
    // understand why the h2d copy segfaults if we dont have the below lines
    err = hsa_amd_agents_allow_access(1, &dest_agent, NULL, agent_ptr);
    ErrorCheck(Allow access to ptr, err);
    #else
    /*void *foo_ptr;
    ret = atmi_malloc(&foo_ptr, 1, ATMI_MEM_PLACE_CPU_MEM(0, 0, 0));
    err = hsa_amd_agents_allow_access(1, &dest_agent, NULL, foo_ptr);
    atmi_free(foo_ptr);
    */
    err = hsa_amd_memory_lock((void *)src, size, &dest_agent, 1, &agent_ptr);
    ErrorCheck(Locking the host ptr, err);
    #endif
    hsa_signal_add_acq_rel(IdentityCopySignal, 1);
    err = hsa_amd_memory_async_copy(dest, dest_agent,
                              agent_ptr, dest_agent,
                              size, 
                              0, NULL, IdentityCopySignal);
	ErrorCheck(Copy async between memory pools, err);
    hsa_signal_wait_acquire(IdentityCopySignal, HSA_SIGNAL_CONDITION_EQ, 0, UINT64_MAX, ATMI_WAIT_STATE);

    #if 1
    atmi_free(agent_ptr);
    #else
    err = hsa_amd_memory_unlock((void *)src);
    ErrorCheck(Unlocking the host ptr, err);
    #endif
    if(err != HSA_STATUS_SUCCESS || ret != ATMI_STATUS_SUCCESS) ret = ATMI_STATUS_ERROR;
    return ret;
}


