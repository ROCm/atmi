#include "ATLData.h"
#include <stdio.h>
#include <string.h>
#include <hsa.h>
#include <hsa_ext_amd.h>
#include "ATLMachine.h"
#include <vector>
#include <iostream>
#include "atmi_runtime.h"
#include "atl_internal.h"

using namespace std;

extern ATLMachine g_atl_machine;
extern hsa_signal_t IdentityCopySignal;

void allow_access_to_all_gpu_agents(void *ptr);
ATLPointerTracker g_data_map;  // Track all am pointer allocations.
//std::map<void *, ATLData *> MemoryMap;

const char *getPlaceStr(atmi_devtype_t type) {
    switch(type) {
        case ATMI_DEVTYPE_CPU: return "CPU";
        case ATMI_DEVTYPE_GPU: return "GPU";
        case ATMI_DEVTYPE_DSP: return "DSP";
        default: return NULL;
    }
}

std::ostream &operator<<(std::ostream &os, const ATLData *ap)
{
    atmi_mem_place_t place = ap->getPlace();
    os << "hostPointer:" << ap->getHostAliasPtr() << " devicePointer:"<< ap->getPtr() << " sizeBytes:" << ap->getSize()
       << " place:(" << getPlaceStr(place.dev_type) << ", " << place.dev_id 
       << ", " << place.mem_id << ")"; 
    return os;
}

void ATLPointerTracker::insert (void *pointer, ATLData *p)
{
    std::lock_guard<std::mutex> l (_mutex);

    DEBUG_PRINT ("insert: %p + %zu\n", pointer, p->getSize());
    _tracker.insert(std::make_pair(ATLMemoryRange(pointer, p->getSize()), p));
}

void ATLPointerTracker::remove (void *pointer)
{
    std::lock_guard<std::mutex> l (_mutex);
    DEBUG_PRINT ("remove: %p\n", pointer);
    _tracker.erase(ATLMemoryRange(pointer,1));
}

ATLData *ATLPointerTracker::find (const void *pointer)
{
    std::lock_guard<std::mutex> l (_mutex);
    ATLData *ret = NULL;
    auto iter = _tracker.find(ATLMemoryRange(pointer,1));
    DEBUG_PRINT ("find: %p\n", pointer);
    if(iter != _tracker.end()) // found
        ret = iter->second;
    return ret;
}

ATLProcessor &get_processor_by_mem_place(atmi_mem_place_t place) {
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
    return get_processor_by_mem_place(place).getAgent();
}

hsa_amd_memory_pool_t get_memory_pool_by_mem_place(atmi_mem_place_t place) {
    ATLProcessor &proc = get_processor_by_mem_place(place);
    return get_memory_pool(proc, place.mem_id);
}
#if 0
atmi_status_t atmi_data_map_sync(void *ptr, size_t size, atmi_mem_place_t place, atmi_arg_type_t arg_type, void **mapped_ptr) {
    if(!mapped_ptr || !ptr) return ATMI_STATUS_ERROR; 
    hsa_status_t err;
    #if 1
    hsa_amd_memory_pool_t dev_pool = get_memory_pool_by_mem_place(place);
    err = hsa_amd_memory_pool_allocate(dev_pool, size, 0, mapped_ptr);
    ErrorCheck(Host staging buffer alloc, err);

    if(arg_type != ATMI_OUT) {
        atmi_mem_place_t cpu_place = {0, ATMI_DEVTYPE_CPU, 0, 0}; 
        void *host_ptr;
        hsa_amd_memory_pool_t host_pool = get_memory_pool_by_mem_place(cpu_place);
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
        hsa_amd_memory_pool_t host_pool = get_memory_pool_by_mem_place(cpu_place);
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
    hsa_amd_memory_pool_t pool = get_memory_pool_by_mem_place(place);
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
    hsa_amd_memory_pool_t pool = get_memory_pool_by_mem_place(place);
    hsa_status_t err = hsa_amd_memory_pool_allocate(pool, size, 0, ptr);
    ErrorCheck(atmi_malloc, err);
    DEBUG_PRINT("Malloced [%s %d] %p\n", place.dev_type == ATMI_DEVTYPE_CPU ? "CPU":"GPU", place.dev_id, *ptr); 
    if(err != HSA_STATUS_SUCCESS) ret = ATMI_STATUS_ERROR;

    ATLData *data = new ATLData(*ptr, NULL, size, place, ATMI_IN_OUT); 
    g_data_map.insert(*ptr, data);

    if(place.dev_type == ATMI_DEVTYPE_CPU) allow_access_to_all_gpu_agents(*ptr);

    return ret;
}

atmi_status_t atmi_free(void *ptr) {
    atmi_status_t ret = ATMI_STATUS_SUCCESS;
    hsa_status_t err = hsa_amd_memory_pool_free(ptr);
    ErrorCheck(atmi_free, err);
    DEBUG_PRINT("Freed %p\n", ptr); 
    
    ATLData *data = g_data_map.find(ptr);
    if(data) {
        g_data_map.remove(ptr);
        delete data;
    }
    if(err != HSA_STATUS_SUCCESS || !data) ret = ATMI_STATUS_ERROR;
    return ret;
}

atmi_status_t atmi_memcpy(void *dest, const void *src, size_t size) {
    atmi_status_t ret;
    hsa_status_t err; 

    ATLData * volatile src_data = g_data_map.find(src);
    ATLData * volatile dest_data = g_data_map.find(dest);
    atmi_mem_place_t cpu = ATMI_MEM_PLACE_CPU_MEM(0, 0, 0);
    hsa_agent_t cpu_agent = get_agent(cpu);
    hsa_agent_t src_agent;
    hsa_agent_t dest_agent;
    void *temp_host_ptr; 
    const void *src_ptr = src;
    void *dest_ptr = dest;
    volatile unsigned type;
    if(src_data && !dest_data) {
        type = ATMI_D2H;
        src_agent = get_agent(src_data->getPlace());
        dest_agent = src_agent; 
        //dest_agent = cpu_agent; // FIXME: can the two agents be the GPU agent itself?
        ret = atmi_malloc(&temp_host_ptr, size, cpu);
        //err = hsa_amd_agents_allow_access(1, &src_agent, NULL, temp_host_ptr);
        //ErrorCheck(Allow access to ptr, err);
        src_ptr = src;
        dest_ptr = temp_host_ptr;
    }
    else if(!src_data && dest_data) {
        type = ATMI_H2D;
        dest_agent = get_agent(dest_data->getPlace());
        //src_agent = cpu_agent; // FIXME: can the two agents be the GPU agent itself?
        src_agent = dest_agent;
        ret = atmi_malloc(&temp_host_ptr, size, cpu);
        memcpy(temp_host_ptr, src, size);
        // FIXME: ideally lock would be the better approach, but we need to try to
        // understand why the h2d copy segfaults if we dont have the below lines
        //err = hsa_amd_agents_allow_access(1, &dest_agent, NULL, temp_host_ptr);
        //ErrorCheck(Allow access to ptr, err);
        src_ptr = (const void *)temp_host_ptr;
        dest_ptr = dest;
    }
    else if(!src_data && !dest_data) {
        type = ATMI_H2H;
        src_agent = cpu_agent;
        dest_agent = cpu_agent; 
        src_ptr = src;
        dest_ptr = dest;
    }
    else {
        type = ATMI_D2D;
        src_agent = get_agent(src_data->getPlace());
        dest_agent = get_agent(dest_data->getPlace());
        src_ptr = src;
        dest_ptr = dest;
    }
    DEBUG_PRINT("Memcpy source agent: %lu\n", src_agent.handle);
    DEBUG_PRINT("Memcpy dest agent: %lu\n", dest_agent.handle);
    hsa_signal_store_release(IdentityCopySignal, 1);
    //hsa_signal_add_acq_rel(IdentityCopySignal, 1);
    err = hsa_amd_memory_async_copy(
                              dest_ptr, dest_agent,
                              src_ptr, src_agent,
                              size, 
                              0, NULL, IdentityCopySignal);
	ErrorCheck(Copy async between memory pools, err);
    hsa_signal_wait_acquire(IdentityCopySignal, HSA_SIGNAL_CONDITION_EQ, 0, UINT64_MAX, ATMI_WAIT_STATE);
    
    // cleanup for D2H and H2D
    if(type == ATMI_D2H) {
        memcpy(dest, temp_host_ptr, size);
        ret = atmi_free(temp_host_ptr);
    }
    else if(type == ATMI_H2D) {
        ret = atmi_free(temp_host_ptr);
    }
    if(err != HSA_STATUS_SUCCESS || ret != ATMI_STATUS_SUCCESS) ret = ATMI_STATUS_ERROR;
    return ret;
}

atmi_status_t atmi_copy_d2h(void *dest, const void *src, size_t size, atmi_mem_place_t place) {
    void *agent_ptr; 
    hsa_status_t err;
    atmi_status_t ret = ATMI_STATUS_SUCCESS;
    hsa_agent_t src_agent = get_agent(place);
    #if 0
    err = hsa_amd_memory_lock(dest, size, &src_agent, 1, &agent_ptr);
    ErrorCheck(Locking the host ptr, err);
    #else
    ret = atmi_malloc(&agent_ptr, size, ATMI_MEM_PLACE_CPU_MEM(0, 0, 0));
    err = hsa_amd_agents_allow_access(1, &src_agent, NULL, agent_ptr);
    ErrorCheck(Allow access to ptr, err);
    #endif
    //hsa_signal_store_release(IdentityCopySignal, 1);
    hsa_signal_add_acq_rel(IdentityCopySignal, 1);
    DEBUG_PRINT("D2H %p --> %p (%lu)\n", src, agent_ptr, size);
    err = hsa_amd_memory_async_copy(
                              agent_ptr, src_agent,
                              src, src_agent,
                              size, 
                              0, NULL, IdentityCopySignal);
	ErrorCheck(Copy async between memory pools, err);
    hsa_signal_wait_acquire(IdentityCopySignal, HSA_SIGNAL_CONDITION_EQ, 0, UINT64_MAX, ATMI_WAIT_STATE);
    DEBUG_PRINT("Done D2H %p --> %p (%lu)\n", src, agent_ptr, size);

    #if 0
    err = hsa_amd_memory_unlock(dest);
    ErrorCheck(Unlocking the host ptr, err);
    #else
    memcpy(dest, agent_ptr, size);
    ret = atmi_free(agent_ptr);
    #endif
    if(err != HSA_STATUS_SUCCESS || ret != ATMI_STATUS_SUCCESS) ret = ATMI_STATUS_ERROR;
    return ret;
}

atmi_status_t atmi_copy_h2d(void *dest, const void *src, size_t size, atmi_mem_place_t place) {
   void *agent_ptr; 
    hsa_status_t err;
    atmi_status_t ret = ATMI_STATUS_SUCCESS;
    hsa_agent_t dest_agent = get_agent(place);
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
    DEBUG_PRINT("H2D %p --> %p (%lu)\n", agent_ptr, dest, size);
    err = hsa_amd_memory_async_copy(dest, dest_agent,
                              agent_ptr, dest_agent,
                              size, 
                              0, NULL, IdentityCopySignal);
	ErrorCheck(Copy async between memory pools, err);
    hsa_signal_wait_acquire(IdentityCopySignal, HSA_SIGNAL_CONDITION_EQ, 0, UINT64_MAX, ATMI_WAIT_STATE);

    #if 1
    ret = atmi_free(agent_ptr);
    #else
    err = hsa_amd_memory_unlock((void *)src);
    ErrorCheck(Unlocking the host ptr, err);
    #endif
    if(err != HSA_STATUS_SUCCESS || ret != ATMI_STATUS_SUCCESS) ret = ATMI_STATUS_ERROR;
    return ret;
}


