/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/
#include "ATLData.h"
#include <stdio.h>
#include <string.h>
#include <hsa.h>
#include <hsa_ext_amd.h>
#include "ATLMachine.h"
#include <vector>
#include <iostream>
#include "atmi_runtime.h"
#include "rt.h"
#include <cassert>
#include <thread>
#include "atl_internal.h"
#include "taskgroup.h"

using namespace std;

extern ATLMachine g_atl_machine;
extern hsa_signal_t IdentityCopySignal;
extern std::deque<atl_task_t *> TaskList;

namespace core {
#ifndef USE_ROCR_PTR_INFO
ATLPointerTracker g_data_map;  // Track all am pointer allocations.
#endif
void allow_access_to_all_gpu_agents(void *ptr);
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

#ifndef USE_ROCR_PTR_INFO
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
#endif

ATLProcessor &get_processor_by_compute_place(atmi_place_t place) {
    int dev_id = place.device_id;
    switch(place.type) {
        case ATMI_DEVTYPE_CPU:
            return g_atl_machine.getProcessors<ATLCPUProcessor>()[dev_id];
        case ATMI_DEVTYPE_GPU:
            return g_atl_machine.getProcessors<ATLGPUProcessor>()[dev_id];
        case ATMI_DEVTYPE_DSP:
            return g_atl_machine.getProcessors<ATLDSPProcessor>()[dev_id];
    }
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

hsa_agent_t get_compute_agent(atmi_place_t place) {
    return get_processor_by_compute_place(place).getAgent();
}

hsa_agent_t get_mem_agent(atmi_mem_place_t place) {
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
        err = hsa_amd_memory_async_copy(*mapped_ptr, get_mem_agent(place),
                host_ptr, get_mem_agent(cpu_place),
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
    hsa_agent_t dest_agent = get_mem_agent(place);
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
        err = hsa_amd_memory_async_copy(host_ptr, get_mem_agent(cpu_place),
                mapped_ptr, get_mem_agent(m->getPlace()),
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
    err = hsa_amd_memory_async_copy(dest->ptr, get_mem_agent(dest_place),
                              src->ptr, get_mem_agent(src_place),
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
    hsa_agent_t dest_agent = get_mem_agent(dest->place);
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

void register_allocation(void *ptr, size_t size, atmi_mem_place_t place) {
#ifndef USE_ROCR_PTR_INFO
    ATLData *data = new ATLData(ptr, NULL, size, place, ATMI_IN_OUT);
    g_data_map.insert(ptr, data);
#else
    ATLData *data = new ATLData(ptr, NULL, size, place, ATMI_IN_OUT);

    hsa_status_t err = hsa_amd_pointer_info_set_userdata(ptr, data);
    ErrorCheck(Setting pointer info with user data, err);
#endif
    if(place.dev_type == ATMI_DEVTYPE_CPU) allow_access_to_all_gpu_agents(ptr);
    // TODO: what if one GPU wants to access another GPU?
}

atmi_status_t Runtime::Malloc(void **ptr, size_t size, atmi_mem_place_t place) {
    atmi_status_t ret = ATMI_STATUS_SUCCESS;
    hsa_amd_memory_pool_t pool = get_memory_pool_by_mem_place(place);
    hsa_status_t err = hsa_amd_memory_pool_allocate(pool, size, 0, ptr);
    ErrorCheck(atmi_malloc, err);
    DEBUG_PRINT("Malloced [%s %d] %p\n", place.dev_type == ATMI_DEVTYPE_CPU ? "CPU":"GPU", place.dev_id, *ptr);
    if(err != HSA_STATUS_SUCCESS) ret = ATMI_STATUS_ERROR;

    register_allocation(*ptr, size, place);

    return ret;
}

atmi_status_t Runtime::Memfree(void *ptr) {
    atmi_status_t ret = ATMI_STATUS_SUCCESS;
    hsa_status_t err;
#ifndef USE_ROCR_PTR_INFO
    ATLData *data = g_data_map.find(ptr);
#else
    hsa_amd_pointer_info_t ptr_info;
    ptr_info.size = sizeof(hsa_amd_pointer_info_t);
    err = hsa_amd_pointer_info((void *)ptr, &ptr_info,
                               NULL, /* alloc fn ptr */
                               NULL, /* num_agents_accessible */
                               NULL);/* accessible agents */
    ErrorCheck(Checking pointer info, err);

    ATLData *data = (ATLData *)ptr_info.userData;
#endif
    if(!data)
      ErrorCheck(Checking pointer info userData, HSA_STATUS_ERROR_INVALID_ALLOCATION);

#ifndef USE_ROCR_PTR_INFO
    g_data_map.remove(ptr);
#else
    // is there a way to unset a userdata with AMD pointer before deleting 'data'?
#endif
    delete data;

    err = hsa_amd_memory_pool_free(ptr);
    ErrorCheck(atmi_free, err);
    DEBUG_PRINT("Freed %p\n", ptr);

    if(err != HSA_STATUS_SUCCESS || !data) ret = ATMI_STATUS_ERROR;
    return ret;
}

atmi_task_handle_t Runtime::MemcpyAsync(atmi_cparm_t *lparm, void *dest, const void *src, size_t size) {
    // TODO: Reuse code in atl_rt for setting up default task params

    atl_task_t *ret = get_new_task();

    /* Add row to taskgroup table for purposes of future synchronizations */
    ret->taskgroup = lparm->group;
    ret->taskgroup_obj = get_taskgroup_impl(ret->taskgroup);

    ret->data_dest_ptr = dest;
    ret->data_src_ptr = (void *)src;
    ret->data_size = size;

    ret->profilable = lparm->profilable;
    ret->groupable = lparm->groupable;
    ret->atmi_task = lparm->task_info;
    ret->type = ATL_DATA_MOVEMENT;

    // FIXME: assign the memory scope; change this if it makes sense to have an API for
    // doing data copies in non-system scope
    ret->acquire_scope = ATMI_FENCE_SCOPE_SYSTEM;
    ret->release_scope = ATMI_FENCE_SCOPE_SYSTEM;

    // TODO: performance fix if there are more CPU agents to improve locality
    ret->place = ATMI_PLACE_GPU(0, 0);

    ret->num_predecessors = 0;
    ret->num_successors = 0;

    ret->devtype = ATMI_DEVTYPE_GPU;
    ret->kernel = NULL;
    ret->kernarg_region = NULL;
    ret->kernarg_region_size = 0;

    ret->predecessors.clear();
    ret->predecessors.resize(lparm->num_required);
    for(int idx = 0; idx < lparm->num_required; idx++) {
        atl_task_t *pred_task = get_task(lparm->requires[idx]);
        assert(pred_task != NULL);
        ret->predecessors[idx] = pred_task;
    }
    ret->pred_taskgroup_objs.clear();
    ret->pred_taskgroup_objs.resize(lparm->num_required_groups);
    for(int idx = 0; idx < lparm->num_required_groups; idx++) {
        ret->pred_taskgroup_objs[idx] = get_taskgroup_impl(lparm->required_groups[idx]);
    }

    lock(&(ret->taskgroup_obj->_group_mutex));
    if(ret->taskgroup_obj->_ordered) {
        ret->taskgroup_obj->_running_ordered_tasks.push_back(ret);
        ret->prev_ordered_task = ret->taskgroup_obj->_last_task;
        ret->taskgroup_obj->_last_task = ret;
    }
    else {
        ret->taskgroup_obj->_running_default_tasks.push_back(ret);
    }
    unlock(&(ret->taskgroup_obj->_group_mutex));
    if(ret->groupable) {
      DEBUG_PRINT("Add ref_cnt 1 to task group %p\n", ret->taskgroup_obj);
      (ret->taskgroup_obj->_task_count)++;
    }
    atl_dep_sync_t dep_sync_type = (atl_dep_sync_t)core::Runtime::getInstance().getDepSyncType();
    if(dep_sync_type == ATL_SYNC_BARRIER_PKT) {
      lock(&mutex_readyq_);
      TaskList.push_back(ret);
      unlock(&mutex_readyq_);
    }
    try_dispatch(ret, NULL, lparm->synchronous);

    return ret->id;
}

atmi_status_t dispatch_data_movement(atl_task_t *task, void *dest,
        const void *src, const size_t size) {

    atmi_status_t ret;
    hsa_status_t err;

    TaskgroupImpl *taskgroup_obj = task->taskgroup_obj;
    atl_dep_sync_t dep_sync_type = (atl_dep_sync_t)core::Runtime::getInstance().getDepSyncType();
    std::vector<hsa_signal_t> dep_signals;
    int val = 0;
    DEBUG_PRINT("(");
    for(atl_task_vector_t::iterator it = task->and_predecessors.begin();
        it != task->and_predecessors.end(); it++) {
        dep_signals.push_back((*it)->signal);
        if((*it)->state < ATMI_DISPATCHED) val++;
        assert((*it)->state >= ATMI_DISPATCHED);
        DEBUG_PRINT("%lu ", (*it)->id);
    }
    DEBUG_PRINT(")\n");
    if(val > 0) DEBUG_PRINT("Task[%lu] has %d not-dispatched predecessor tasks\n", task->id, val);

#ifndef USE_ROCR_PTR_INFO
    ATLData * volatile src_data = g_data_map.find(src);
    ATLData * volatile dest_data = g_data_map.find(dest);
#else
    hsa_amd_pointer_info_t src_ptr_info;
    hsa_amd_pointer_info_t dest_ptr_info;
    src_ptr_info.size = sizeof(hsa_amd_pointer_info_t);
    dest_ptr_info.size = sizeof(hsa_amd_pointer_info_t);
    err = hsa_amd_pointer_info((void *)src, &src_ptr_info,
                               NULL, /* alloc fn ptr */
                               NULL, /* num_agents_accessible */
                               NULL);  /* accessible agents */
    ErrorCheck(Checking src pointer info, err);
    err = hsa_amd_pointer_info((void *)dest, &dest_ptr_info,
                               NULL, /* alloc fn ptr */
                               NULL, /* num_agents_accessible */
                               NULL);  /* accessible agents */
    ErrorCheck(Checking dest pointer info, err);
    ATLData * volatile src_data = (ATLData *)src_ptr_info.userData;
    ATLData * volatile dest_data = (ATLData *)dest_ptr_info.userData;
#endif
    bool is_src_host = (!src_data || src_data->getPlace().dev_type == ATMI_DEVTYPE_CPU);
    bool is_dest_host = (!dest_data || dest_data->getPlace().dev_type == ATMI_DEVTYPE_CPU);
    atmi_mem_place_t cpu = ATMI_MEM_PLACE_CPU_MEM(0, 0, 0);
    hsa_agent_t cpu_agent = get_mem_agent(cpu);
    hsa_agent_t src_agent;
    hsa_agent_t dest_agent;
    void *temp_host_ptr;
    const void *src_ptr = src;
    void *dest_ptr = dest;
    volatile unsigned type;
    if(is_src_host && is_dest_host) {
        type = ATMI_H2H;
        src_agent = cpu_agent;
        dest_agent = cpu_agent;
        src_ptr = src;
        dest_ptr = dest;
    }
    else if(src_data && !dest_data) {
        type = ATMI_D2H;
        src_agent = get_mem_agent(src_data->getPlace());
        dest_agent = src_agent;
        src_ptr = src;
        dest_ptr = dest;
    }
    else if(!src_data && dest_data) {
        type = ATMI_H2D;
        dest_agent = get_mem_agent(dest_data->getPlace());
        src_agent = dest_agent;
        src_ptr = src;
        dest_ptr = dest;
    }
    else {
        type = ATMI_D2D;
        src_agent = get_mem_agent(src_data->getPlace());
        dest_agent = get_mem_agent(dest_data->getPlace());
        src_ptr = src;
        dest_ptr = dest;
    }
    DEBUG_PRINT("Memcpy source agent: %lu\n", src_agent.handle);
    DEBUG_PRINT("Memcpy dest agent: %lu\n", dest_agent.handle);

    if(type == ATMI_H2D || type == ATMI_D2H) {
      if(task->groupable == ATMI_TRUE) {
        lock(&(taskgroup_obj->_group_mutex));
        // barrier pkt already sets the signal values when the signal resource
        // is available
        taskgroup_obj->_running_groupable_tasks.push_back(task);
        unlock(&(taskgroup_obj->_group_mutex));
      }
      // For malloc'ed buffers, additional atmi_malloc/memcpy/free
      // steps are needed. So, fire and forget a copy thread with
      // signal count = 2 (one for actual host-device copy and another
      // for H2H copy to setup the device copy.
      std::thread([](void* dst, const void* src, size_t size, hsa_agent_t agent,
            unsigned type, atmi_mem_place_t cpu, hsa_signal_t signal,
            std::vector<hsa_signal_t> dep_signals, atl_task_t *task) {
          atmi_status_t ret;
          hsa_status_t err;
          atl_dep_sync_t dep_sync_type = (atl_dep_sync_t)core::Runtime::getInstance().getDepSyncType();
          void *temp_host_ptr;
          const void *src_ptr = src;
          void *dest_ptr = dst;
          ret = atmi_malloc(&temp_host_ptr, size, cpu);
          if(type == ATMI_H2D) {
          memcpy(temp_host_ptr, src, size);
          src_ptr = (const void *)temp_host_ptr;
          dest_ptr = dst;
          }
          else {
          src_ptr = src;
          dest_ptr = temp_host_ptr;
          }

          if(dep_sync_type == ATL_SYNC_BARRIER_PKT && !dep_signals.empty()) {
            DEBUG_PRINT("SDMA-host for %p (%lu) with %lu dependencies\n", task, task->id, dep_signals.size());
            err = hsa_amd_memory_async_copy(
                dest_ptr, agent,
                src_ptr, agent,
                size,
                dep_signals.size(), &(dep_signals[0]), signal);
            ErrorCheck(Copy async between memory pools, err);
          } else {
            DEBUG_PRINT("SDMA-host for %p (%lu)\n", task, task->id);
            err = hsa_amd_memory_async_copy(
                dest_ptr, agent,
                src_ptr, agent,
                size,
                0, NULL, signal);
            ErrorCheck(Copy async between memory pools, err);
          }
          set_task_state(task, ATMI_DISPATCHED);
          hsa_signal_wait_acquire(signal, HSA_SIGNAL_CONDITION_EQ, 1, UINT64_MAX, ATMI_WAIT_STATE);
          // cleanup for D2H and H2D
          if(type == ATMI_D2H) {
            memcpy(dst, temp_host_ptr, size);
          }
          atmi_free(temp_host_ptr);
          hsa_signal_subtract_acq_rel(signal, 1);
      },
            dest, src, size, src_agent, type, cpu, task->signal, dep_signals, task).detach();
    }
    else {
      if(task->groupable == ATMI_TRUE) {
        lock(&(taskgroup_obj->_group_mutex));
        // barrier pkt already sets the signal values when the signal resource
        // is available
        taskgroup_obj->_running_groupable_tasks.push_back(task);
        unlock(&(taskgroup_obj->_group_mutex));
      }

      // set task state to dispatched; then dispatch
      set_task_state(task, ATMI_DISPATCHED);

      if(dep_sync_type == ATL_SYNC_BARRIER_PKT && !dep_signals.empty()) {
        DEBUG_PRINT("SDMA for %p (%lu) with %lu dependencies\n", task, task->id, dep_signals.size());
        err = hsa_amd_memory_async_copy(
            dest_ptr, dest_agent,
            src_ptr, src_agent,
            size,
            dep_signals.size(), &(dep_signals[0]), task->signal);
        ErrorCheck(Copy async between memory pools, err);
      } else {
        DEBUG_PRINT("SDMA for %p (%lu)\n", task, task->id);
        err = hsa_amd_memory_async_copy(
            dest_ptr, dest_agent,
            src_ptr, src_agent,
            size,
            0, NULL, task->signal);
        ErrorCheck(Copy async between memory pools, err);
      }
    }
    return ATMI_STATUS_SUCCESS;
}

atmi_status_t Runtime::Memcpy(void *dest, const void *src, size_t size) {
    atmi_status_t ret;
    hsa_status_t err;

#ifndef USE_ROCR_PTR_INFO
    ATLData * volatile src_data = g_data_map.find(src);
    ATLData * volatile dest_data = g_data_map.find(dest);
#else
    hsa_amd_pointer_info_t src_ptr_info;
    hsa_amd_pointer_info_t dest_ptr_info;
    src_ptr_info.size = sizeof(hsa_amd_pointer_info_t);
    dest_ptr_info.size = sizeof(hsa_amd_pointer_info_t);
    err = hsa_amd_pointer_info((void *)src, &src_ptr_info,
                               NULL, /* alloc fn ptr */
                               NULL, /* num_agents_accessible */
                               NULL);/* accessible agents */
    ErrorCheck(Checking src pointer info, err);
    err = hsa_amd_pointer_info((void *)dest, &dest_ptr_info,
                               NULL, /* alloc fn ptr */
                               NULL, /* num_agents_accessible */
                               NULL);/* accessible agents */
    ErrorCheck(Checking dest pointer info, err);
    ATLData * volatile src_data = (ATLData *)src_ptr_info.userData;
    ATLData * volatile dest_data = (ATLData *)dest_ptr_info.userData;
#endif
    atmi_mem_place_t cpu = ATMI_MEM_PLACE_CPU_MEM(0, 0, 0);
    hsa_agent_t cpu_agent = get_mem_agent(cpu);
    hsa_agent_t src_agent;
    hsa_agent_t dest_agent;
    void *temp_host_ptr;
    const void *src_ptr = src;
    void *dest_ptr = dest;
    volatile unsigned type;
    if(src_data && !dest_data) {
        type = ATMI_D2H;
        src_agent = get_mem_agent(src_data->getPlace());
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
        dest_agent = get_mem_agent(dest_data->getPlace());
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
        src_agent = get_mem_agent(src_data->getPlace());
        dest_agent = get_mem_agent(dest_data->getPlace());
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

} // namespace core
