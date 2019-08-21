/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/

#include "atl_internal.h"
#include "atl_profile.h"
#include "atl_bindthread.h"
#include "ATLQueue.h"
#include "ATLMachine.h"
#include "ATLData.h"
#include "rt.h"
#include <pthread.h>
#include <time.h>
#include <cassert>
#include <stdarg.h>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <deque>
#include <vector>
#include <atomic>
#include <unistd.h>
#include <errno.h>
#include <algorithm>
#include <sys/syscall.h>
#include <malloc.h>
#include "RealTimerClass.h"
#include "taskgroup.h"
using namespace Global;

pthread_mutex_t mutex_all_tasks_;
pthread_mutex_t mutex_readyq_;
#define NANOSECS 1000000000L

//  set NOTCOHERENT needs this include
#include "hsa_ext_amd.h"

extern bool handle_signal(hsa_signal_value_t value, void *arg);

void print_atl_kernel(const char * str, const int i);

std::vector<atl_task_t *> AllTasks;
std::queue<atl_task_t *> ReadyTaskQueue;
std::deque<atl_task_t *> TaskList;
std::queue<hsa_signal_t> FreeSignalPool;
std::set<atl_task_t *> SinkTasks;

std::map<uint64_t, atl_kernel_t *> KernelImplMap;
//std::map<uint64_t, std::vector<std::string> > ModuleMap;
std::vector<atl_task_t *> DispatchedTasks;
bool setCallbackToggle = false;

static atl_dep_sync_t g_dep_sync_type = (atl_dep_sync_t)core::Runtime::getInstance().getDepSyncType();

atmi_task_handle_t ATMI_NULL_TASK_HANDLE = ATMI_TASK_HANDLE(0xFFFFFFFFFFFFFFFF);
atmi_place_t ATMI_DEFAULT_PLACE = {0, ATMI_DEVTYPE_GPU, 0, 0xFFFFFFFFFFFFFFFF};
atmi_mem_place_t ATMI_DEFAULT_MEM_PLACE = {0, ATMI_DEVTYPE_GPU, 0, 0};

extern RealTimer SignalAddTimer;
extern RealTimer HandleSignalTimer;
extern RealTimer HandleSignalInvokeTimer;
extern RealTimer TaskWaitTimer;
extern RealTimer TryLaunchTimer;
extern RealTimer ParamsInitTimer;
extern RealTimer TryLaunchInitTimer;
extern RealTimer ShouldDispatchTimer;
extern RealTimer RegisterCallbackTimer;
extern RealTimer LockTimer;
extern RealTimer TryDispatchTimer;
extern size_t max_ready_queue_sz;
extern size_t waiting_count;
extern size_t direct_dispatch;
extern size_t callback_dispatch;

extern ATLMachine g_atl_machine;

extern hsa_signal_t IdentityORSignal;
extern hsa_signal_t IdentityANDSignal;
extern hsa_signal_t IdentityCopySignal;
extern atmi_context_t atmi_context_data;
extern atmi_context_t * atmi_context;
extern atl_context_t atlc;
extern atl_context_t * atlc_p;

namespace core {
extern void lock_set(std::set<pthread_mutex_t *> &mutexes);
extern void unlock_set(std::set<pthread_mutex_t *> &mutexes);
// this file will eventually be refactored into rt.cpp and other module-specific files

long int get_nanosecs( struct timespec start_time, struct timespec end_time) {
    long int nanosecs;
    if ((end_time.tv_nsec-start_time.tv_nsec)<0) nanosecs =
        ((((long int) end_time.tv_sec- (long int) start_time.tv_sec )-1)*NANOSECS ) +
            ( NANOSECS + (long int) end_time.tv_nsec - (long int) start_time.tv_nsec) ;
    else nanosecs =
        (((long int) end_time.tv_sec- (long int) start_time.tv_sec )*NANOSECS ) +
            ( (long int) end_time.tv_nsec - (long int) start_time.tv_nsec );
    return nanosecs;
}

void packet_store_release(uint32_t* packet, uint16_t header, uint16_t rest){
  __atomic_store_n(packet,header|(rest<<16),__ATOMIC_RELEASE);
}

uint16_t create_header(hsa_packet_type_t type, int barrier,
                       atmi_task_fence_scope_t acq_fence,
                       atmi_task_fence_scope_t rel_fence) {
   uint16_t header = type << HSA_PACKET_HEADER_TYPE;
   header |= barrier << HSA_PACKET_HEADER_BARRIER;
   header |= (hsa_fence_scope_t)(int)acq_fence << HSA_PACKET_HEADER_ACQUIRE_FENCE_SCOPE;
   header |= (hsa_fence_scope_t)(int)rel_fence << HSA_PACKET_HEADER_RELEASE_FENCE_SCOPE;
   //__atomic_store_n((uint8_t*)(&header), (uint8_t)type, __ATOMIC_RELEASE);
   return header;
}

int get_task_handle_ID(atmi_task_handle_t t) {
    #if 1
    return (int)(0xFFFFFFFF & t);
    #else
    return t.lo;
    #endif
}

void set_task_handle_ID(atmi_task_handle_t *t, int ID) {
    #if 1
    unsigned long int task_handle = *t;
    task_handle |= 0xFFFFFFFFFFFFFFFF;
    task_handle &= ID;
    *t = task_handle;
    #else
    t->lo = ID;
    #endif
}

atmi_taskgroup_handle_t get_taskgroup(atmi_task_handle_t t) {
    atl_task_t *task = get_task(t);
    atmi_taskgroup_handle_t ret;
    if(task)
        ret = task->taskgroup;
    return ret;
}

atl_task_t *get_task(atmi_task_handle_t t) {
    /* FIXME: node 0 only for now */
    atl_task_t *ret = NULL;
    if(t != ATMI_NULL_TASK_HANDLE) {
        lock(&mutex_all_tasks_);
        ret = AllTasks[get_task_handle_ID(t)];
        unlock(&mutex_all_tasks_);
    }
    return ret;
    //return AllTasks[t.lo];
}

atl_task_t *get_continuation_task(atmi_task_handle_t t) {
    /* FIXME: node 0 only for now */
    //return AllTasks[t.hi];
    return NULL;
}

hsa_signal_t enqueue_barrier_async(atl_task_t *task, hsa_queue_t *queue, const int dep_task_count,
                                   atl_task_t **dep_task_list, int barrier_flag,
                                   bool need_completion) {
    /* This routine will enqueue a barrier packet for all dependent packets to complete
       irrespective of their taskgroup
     */

    long t_barrier_wait = 0L;
    long t_barrier_dispatch = 0L;
    hsa_status_t err;
    hsa_signal_t last_signal;
    hsa_signal_t identity_signal;
    if(queue == NULL || dep_task_list == NULL || dep_task_count <= 0) return last_signal;
    if(barrier_flag == SNK_OR)
        identity_signal = IdentityORSignal;
    else
        identity_signal = IdentityANDSignal;
    atl_task_t **tasks = dep_task_list;
    int tasks_remaining = dep_task_count;
    /* Keep adding barrier packets in multiples of 5 because that is the maximum signals that
       the HSA barrier packet can support today
     */
    const int HSA_BARRIER_MAX_DEPENDENT_TASKS = 5;
    /* round up */
    int barrier_pkt_count = (dep_task_count + HSA_BARRIER_MAX_DEPENDENT_TASKS - 1) / HSA_BARRIER_MAX_DEPENDENT_TASKS;
    int barrier_pkt_id = 0;

    // We are inserting barrier_pkt_count number of barrier packets. Only the last
    // barrier packet will have the barrier bit set so that the next AQL packet is
    // guaranteed to have all its predecessor barrier packets completed.
    // All others will not have the barrier bit set.
    for(barrier_pkt_id = 0; barrier_pkt_id < barrier_pkt_count; barrier_pkt_id++) {
        bool last_bpkt = (barrier_pkt_id == barrier_pkt_count - 1);
        /* Increment write index and get current write index for this queue/packet. */
        uint64_t index = hsa_queue_add_write_index_relaxed(queue, 1);
        // Wait until the queue is not full before writing the packet
        while(index - hsa_queue_load_read_index_acquire(queue) >= queue->size);

        const uint32_t queueMask = queue->size - 1;
        hsa_barrier_and_packet_t* barrier = &(((hsa_barrier_and_packet_t*)(queue->base_address))[index&queueMask]);
        DEBUG_PRINT("BP %p [%lu]\n", queue, index&queueMask);
        assert(sizeof(hsa_barrier_or_packet_t) == sizeof(hsa_barrier_and_packet_t));
        if(barrier_flag == SNK_OR) {
            /* Define the barrier packet to be at the calculated queue index address.
             * We set the memory scope to NONE because we assume that the right scopes
             * have been set by the AQL kernels around these barriers */
            memset(barrier, 0, sizeof(hsa_barrier_or_packet_t));
            barrier->header = create_header(HSA_PACKET_TYPE_BARRIER_OR, last_bpkt,
                                            ATMI_FENCE_SCOPE_NONE, ATMI_FENCE_SCOPE_NONE);
        }
        else {
            /* Define the barrier packet to be at the calculated queue index address.
             * We set the memory scope to NONE because we assume that the right scopes
             * have been set by the AQL kernels around these barriers */
            memset(barrier, 0, sizeof(hsa_barrier_and_packet_t));
            barrier->header = create_header(HSA_PACKET_TYPE_BARRIER_AND, last_bpkt,
                                            ATMI_FENCE_SCOPE_NONE, ATMI_FENCE_SCOPE_NONE);
        }
        //Only set dep_signals when needed. Setting dep_signals without an actual
        //dependent task increases latency of the barrier packet processing. It is
        //more performant to set just the minimal set of dep_signals.
        //int j;
        //for(j = 0; j < 5; j++) {
        //    barrier->dep_signal[j] = last_signal;
        //}
        /* populate all dep_signals */
        int dep_signal_id = 0;
        int iter = 0;
        for(dep_signal_id = 0; dep_signal_id < HSA_BARRIER_MAX_DEPENDENT_TASKS; dep_signal_id++) {
            if(*tasks != NULL && tasks_remaining > 0) {
                DEBUG_PRINT("Barrier Packet %d\n", iter);
                iter++;
                /* fill out the barrier packet and ring doorbell */
                barrier->dep_signal[dep_signal_id] = (*tasks)->signal;
                DEBUG_PRINT("Enqueue wait for task %lu signal handle: %" PRIu64 "\n", (*tasks)->id, barrier->dep_signal[dep_signal_id].handle);
                tasks++;
                tasks_remaining--;
            }
        }
        // completion signal if needed for reclaiming resources,
        // have barrier bit for next AQL packet to implicitly wait
        if(last_bpkt) {
          if(need_completion)
            barrier->completion_signal = identity_signal;
          /* ring doorbell to dispatch the kernel.  */
          hsa_signal_store_relaxed(queue->doorbell_signal, index);
        }
    }
    return last_signal;
}

void enqueue_barrier(atl_task_t *task, hsa_queue_t *queue, const int dep_task_count,
                     atl_task_t **dep_task_list, int wait_flag, int barrier_flag,
                     atmi_devtype_t devtype,
                     bool need_completion) {
    hsa_signal_t last_signal = enqueue_barrier_async(task, queue, dep_task_count,
                                                     dep_task_list, barrier_flag, need_completion);
    if(devtype == ATMI_DEVTYPE_CPU) {
        signal_worker(queue, PROCESS_PKT);
    }
    /* Wait on completion signal if blockine */
    if(wait_flag == SNK_WAIT) {
        hsa_signal_wait_acquire(last_signal, HSA_SIGNAL_CONDITION_EQ, 0, UINT64_MAX, ATMI_WAIT_STATE);
        hsa_signal_destroy(last_signal);
    }
}

extern void atl_task_wait(atl_task_t *task) {
    TaskWaitTimer.Start();
    if(task != NULL) {
        // if task is not yet ready, then it has not gotten all resources yet, so we wait
        // for resources and then issue a callback; only then we can wait for the task to
        // complete
        if(g_dep_sync_type == ATL_SYNC_BARRIER_PKT) {
          while(task->state < ATMI_DISPATCHED) {}
          if(task->state < ATMI_EXECUTED) {
            // Now, this task has the resources, so it can get dispatched any time.
            // So, create a barrier packet for current sink tasks and add async handler
            // for its completion.
            // All dispatched tasks get signal from device-only signal pool. All async
            // handlers are registered to interruptible signals
            lock(&mutex_readyq_);
            atl_task_vector_t tasks;
            atl_task_vector_t *dispatched_tasks_ptr = NULL;
            tasks.insert(tasks.end(), SinkTasks.begin(), SinkTasks.end());
            SinkTasks.clear();
            // will be deleted in the callback
            dispatched_tasks_ptr = new atl_task_vector_t;
            dispatched_tasks_ptr->insert(dispatched_tasks_ptr->end(), DispatchedTasks.begin(), DispatchedTasks.end());
            DispatchedTasks.clear();
            unlock(&mutex_readyq_);
            if(dispatched_tasks_ptr) {
              enqueue_barrier_tasks(tasks);
              if(!tasks.empty()) {
                DEBUG_PRINT("Registering callback for task %lu\n", task->id);
                hsa_status_t err = hsa_amd_signal_async_handler(IdentityANDSignal,
                    HSA_SIGNAL_CONDITION_EQ, 0, handle_signal, (void *)dispatched_tasks_ptr);
                ErrorCheck(Creating signal handler, err);
              }
            }
          }
        }
        while(task->state != ATMI_COMPLETED) { }

        /* Flag this task as completed */
        /* FIXME: How can HSA tell us if and when a task has failed? */
        set_task_state(task, ATMI_COMPLETED);
    }

    TaskWaitTimer.Stop();
    return;// ATMI_STATUS_SUCCESS;
}

extern atmi_status_t Runtime::TaskWait(atmi_task_handle_t task) {
    DEBUG_PRINT("Waiting for task ID: %lu\n", task);
    atl_task_wait(get_task(task));
    return ATMI_STATUS_SUCCESS;
}

atmi_status_t queue_sync(hsa_queue_t *queue) {
    if(queue == NULL) return ATMI_STATUS_SUCCESS;
    /* This function puts a barrier packet into the queue
       This routine will wait for all packets to complete on this queue.
    */
    hsa_signal_t signal;
    hsa_signal_create(1, 0, NULL, &signal);

    /* Obtain the write index for the command queue for this taskgroup.  */
    uint64_t index = hsa_queue_load_write_index_relaxed(queue);
    const uint32_t queueMask = queue->size - 1;

    /* Define the barrier packet to be at the calculated queue index address.  */
    hsa_barrier_and_packet_t* barrier = &(((hsa_barrier_and_packet_t*)(queue->base_address))[index&queueMask]);
    memset(barrier, 0, sizeof(hsa_barrier_and_packet_t));

    barrier->header = create_header(HSA_PACKET_TYPE_BARRIER_AND, 1);

    barrier->completion_signal = signal;

    /* Increment write index and ring doorbell to dispatch the kernel.  */
    hsa_queue_store_write_index_relaxed(queue, index+1);
    hsa_signal_store_relaxed(queue->doorbell_signal, index);

    /* Wait on completion signal til kernel is finished.  */
    hsa_signal_wait_acquire(signal, HSA_SIGNAL_CONDITION_EQ, 0, UINT64_MAX, ATMI_WAIT_STATE);
    hsa_signal_destroy(signal);

    return ATMI_STATUS_SUCCESS;
}

void init_dag_scheduler() {
    if(atlc.g_mutex_dag_initialized == 0) {
        pthread_mutex_init(&mutex_all_tasks_, NULL);
        pthread_mutex_init(&mutex_readyq_, NULL);
        AllTasks.clear();
        AllTasks.reserve(500000);
        //PublicTaskMap.clear();
        atlc.g_mutex_dag_initialized = 1;
        DEBUG_PRINT("main tid = %lu\n", syscall(SYS_gettid));
    }
}

#if 0
//FIXME: this implementation may be needed to have a common lock/unlock for CPU/GPU
lock(int *mutex) {
    // atomic cas of mutex with 1 and return 0
}

unlock(int *mutex) {
    // atomic exch with 1
}
#endif


void set_task_state(atl_task_t *t, const atmi_state_t state) {
    t->state = state;
    //t->state.store(state, std::memory_order_seq_cst);
    if(t->atmi_task != NULL) t->atmi_task->state = state;
}

void set_task_metrics(atl_task_t *task) {
    hsa_status_t err = HSA_STATUS_SUCCESS;
    //if(task->profile != NULL) {
    if(task->profilable == ATMI_TRUE) {
        hsa_signal_t signal = task->signal;
        hsa_amd_profiling_dispatch_time_t metrics;
        if(task->devtype == ATMI_DEVTYPE_GPU) {
            err = hsa_amd_profiling_get_dispatch_time(get_compute_agent(task->place),
                    signal, &metrics);
            ErrorCheck(Profiling GPU dispatch, err);
            if(task->atmi_task) {
                uint64_t freq;
                err = hsa_system_get_info(HSA_SYSTEM_INFO_TIMESTAMP_FREQUENCY,
                    &freq);
                ErrorCheck(Getting system timestamp frequency info, err);
                uint64_t start = metrics.start / (freq/NANOSECS);
                uint64_t end = metrics.end / (freq/NANOSECS);
                DEBUG_PRINT("Ticks: (%lu->%lu)\nFreq: %lu\nTime(ns: (%lu->%lu)\n",
                        metrics.start, metrics.end, freq, start, end);
                task->atmi_task->profile.start_time = start;
                task->atmi_task->profile.end_time = end;
                task->atmi_task->profile.dispatch_time = start;
                task->atmi_task->profile.ready_time = start;
            }
        }
        else {
            /* metrics for CPU tasks will be populated in the
             * worker pthread itself. No special function call */
        }
    }
}
// function pointers
task_process_init_buffer_t task_process_init_buffer;
task_process_fini_buffer_t task_process_fini_buffer;

atmi_status_t Runtime::RegisterTaskInitBuffer(task_process_init_buffer_t fp)
{
  //printf("register function pointer \n");
  task_process_init_buffer = fp;

  return ATMI_STATUS_SUCCESS;
}

atmi_status_t Runtime::RegisterTaskFiniBuffer(task_process_fini_buffer_t fp)
{
  //printf("register function pointer \n");
  task_process_fini_buffer = fp;

  return ATMI_STATUS_SUCCESS;
}

pthread_mutex_t sort_mutex_2(pthread_mutex_t *addr1, pthread_mutex_t *addr2, pthread_mutex_t **addr_low, pthread_mutex_t **addr_hi) {
  if((uint64_t)addr1 < (uint64_t)addr2) {
    *addr_low = addr1;
    *addr_hi = addr2;
  }
  else {
    *addr_hi = addr1;
    *addr_low = addr2;
  }
}

void lock(pthread_mutex_t *m) {
  //printf("Locked Mutex: %p\n", m);
  //LockTimer.Start();
  pthread_mutex_lock(m);
  //LockTimer.Stop();
}

void unlock(pthread_mutex_t *m) {
  //printf("Unlocked Mutex: %p\n", m);
  //LockTimer.Start();
  pthread_mutex_unlock(m);
  //LockTimer.Stop();
}

void print_rset(std::set<pthread_mutex_t *> &mutexes) {
  printf("[ ");
  for(std::set<pthread_mutex_t *>::reverse_iterator it = mutexes.rbegin();
      it != mutexes.rend(); it++) {
    printf("%p ", *it);
  }
  printf("]\n");
}

void print_set(std::set<pthread_mutex_t *> &mutexes) {
  printf("[ ");
  for(std::set<pthread_mutex_t *>::iterator it = mutexes.begin();
      it != mutexes.end(); it++) {
    printf("%p ", *it);
  }
  printf("]\n");
}

void print_sorted_vec(std::vector<pthread_mutex_t *> &mutexes) {
  std::sort(mutexes.begin(), mutexes.end());
  printf("[ ");
  for(size_t i = 0; i < mutexes.size(); i++) {
    printf("%p ", mutexes[i]);
  }
  printf("]\n");
}


void lock_set(std::set<pthread_mutex_t *> &mutexes) {
  DEBUG_PRINT("Locking [ ");
  for(std::set<pthread_mutex_t *>::iterator it = mutexes.begin();
      it != mutexes.end(); it++) {
    DEBUG_PRINT("%p ", *it);
    lock((pthread_mutex_t *)(*it));
  }
  DEBUG_PRINT("]\n");
}

void unlock_set(std::set<pthread_mutex_t *> &mutexes) {
  DEBUG_PRINT("Unlocking [ ");
  for(std::set<pthread_mutex_t *>::reverse_iterator it = mutexes.rbegin();
      it != mutexes.rend(); it++) {
    DEBUG_PRINT("%p ", *it);
    unlock((pthread_mutex_t *)(*it));
  }
  DEBUG_PRINT("]\n");
}

void dispatch_all_tasks(std::vector<atl_task_t*>::iterator &start,
    std::vector<atl_task_t*>::iterator &end) {
  for(std::vector<atl_task_t*>::iterator it = start; it != end; it++) {
    dispatch_task(*it);
  }
}

void handle_signal_callback(atl_task_t *task) {
  // tasks without atmi_task handle should not be added to callbacks anyway
  assert(task->groupable != ATMI_TRUE);

  // process printf buffer
  {
    atl_kernel_impl_t *kernel_impl = NULL;
    if (task_process_fini_buffer) {
      if(task->kernel) {
        kernel_impl = get_kernel_impl(task->kernel, task->kernel_id);
        //printf("Task Id: %lu, kernel name: %s\n", task->id, kernel_impl->kernel_name.c_str());
        char *kargs = (char *)(task->kernarg_region);
        if(task->type == ATL_KERNEL_EXECUTION &&
            task->devtype == ATMI_DEVTYPE_GPU &&
            kernel_impl->kernel_type == AMDGCN) {
          atmi_implicit_args_t *impl_args = (atmi_implicit_args_t *)(kargs + (task->kernarg_region_size - sizeof(atmi_implicit_args_t)));

          (*task_process_fini_buffer)((void *)impl_args->pipe_ptr, MAX_PIPE_SIZE);

        }
      }
    }
  }

  lock(&(task->mutex));
  set_task_state(task, ATMI_EXECUTED);
  unlock(&(task->mutex));
  // after predecessor is done, decrement all successor's dependency count.
  // If count reaches zero, then add them to a 'ready' task list. Next,
  // dispatch all ready tasks in a round-robin manner to the available
  // GPU/CPU queues.
  // decrement reference count of its dependencies; add those with ref count = 0 to a
  // “ready” list
  atl_task_vector_t &deps = task->and_successors;
  DEBUG_PRINT("Deps list of %lu [%lu]: ", task->id, deps.size());
  atl_task_vector_t temp_list;
  for(atl_task_vector_t::iterator it = deps.begin();
      it != deps.end(); it++) {
    // FIXME: should we be grabbing a lock on each successor before
    // decrementing their predecessor count? Currently, it may not be
    // required because there is only one callback thread, but what if there
    // were more?
    lock(&((*it)->mutex));
    DEBUG_PRINT(" %lu(%d) ", (*it)->id, (*it)->num_predecessors);
    (*it)->num_predecessors--;
    if((*it)->num_predecessors == 0) {
      // add to ready list
      temp_list.push_back(*it);
    }
    unlock(&((*it)->mutex));
  }
  std::set<pthread_mutex_t *> mutexes;
  // release the kernarg segment back to the kernarg pool
  atl_kernel_t *kernel = task->kernel;
  atl_kernel_impl_t *kernel_impl = NULL;
  if(kernel) {
    kernel_impl = get_kernel_impl(kernel, task->kernel_id);
    mutexes.insert(&(kernel_impl->mutex));
  }
  mutexes.insert(&mutex_readyq_);
  lock_set(mutexes);
  DEBUG_PRINT("\n");
  for(atl_task_vector_t::iterator it = temp_list.begin();
      it != temp_list.end(); it++) {
    // FIXME: if groupable task, push it in the right taskgroup
    // else push it to readyqueue
    ReadyTaskQueue.push(*it);
  }
  // release your own signal to the pool
  FreeSignalPool.push(task->signal);
  DEBUG_PRINT("Freeing Kernarg Segment Id: %d\n", task->kernarg_region_index);
  // release your kernarg region to the pool
  if(kernel) kernel_impl->free_kernarg_segments.push(task->kernarg_region_index);
  unlock_set(mutexes);
  DEBUG_PRINT("[Handle Signal %lu ] Free Signal Pool Size: %lu; Ready Task Queue Size: %lu\n", task->id, FreeSignalPool.size(), ReadyTaskQueue.size());
  // dispatch from ready queue if any task exists
  lock(&(task->mutex));
  set_task_metrics(task);
  set_task_state(task, ATMI_COMPLETED);
  unlock(&(task->mutex));
  do_progress(task->taskgroup_obj);
  //dispatch_ready_task_for_free_signal();
  //dispatch_ready_task_or_release_signal(task);
}

void handle_signal_barrier_pkt(atl_task_t *task, atl_task_vector_t *dispatched_tasks_ptr) {
  // list of signals that can be reclaimed at every iteration
  std::vector<hsa_signal_t> temp_list;

  // since we attach barrier packet to sink tasks, at this point in the callback,
  // we are guaranteed that all dispatched_tasks have completed execution.
  atl_task_vector_t dispatched_tasks = *dispatched_tasks_ptr;

  for (auto task : dispatched_tasks) {
    assert(task->groupable != ATMI_TRUE);

    // process printf buffer
    {
      atl_kernel_impl_t *kernel_impl = NULL;
      if (task_process_fini_buffer) {
        if(task->kernel) {
          kernel_impl = get_kernel_impl(task->kernel, task->kernel_id);
          //printf("Task Id: %lu, kernel name: %s\n", task->id, kernel_impl->kernel_name.c_str());
          char *kargs = (char *)(task->kernarg_region);
          if(task->type == ATL_KERNEL_EXECUTION &&
              task->devtype == ATMI_DEVTYPE_GPU &&
              kernel_impl->kernel_type == AMDGCN) {
            atmi_implicit_args_t *impl_args = (atmi_implicit_args_t *)(kargs + (task->kernarg_region_size - sizeof(atmi_implicit_args_t)));

            (*task_process_fini_buffer)((void *)impl_args->pipe_ptr, MAX_PIPE_SIZE);

          }
        }
      }
    }

    // This dispatched task is now executed
    lock(&(task->mutex));
    set_task_state(task, ATMI_EXECUTED);
    unlock(&(task->mutex));

    // now reclaim its resources (kernel args and signal)
    std::set<pthread_mutex_t *> mutexes;
    atl_kernel_t *kernel = task->kernel;
    atl_kernel_impl_t *kernel_impl = NULL;
    if(kernel) {
      kernel_impl = get_kernel_impl(kernel, task->kernel_id);
      mutexes.insert(&(kernel_impl->mutex));
    }
    mutexes.insert(&(task->mutex));
    mutexes.insert(&mutex_readyq_);

    lock_set(mutexes);

    // release the kernarg segment back to the kernarg pool
    DEBUG_PRINT("Freeing Kernarg Segment Id: %d\n", task->kernarg_region_index);
    if(kernel) kernel_impl->free_kernarg_segments.push(task->kernarg_region_index);

    // release the signal back to the signal pool
    DEBUG_PRINT("Task %lu completed\n", task->id);
    FreeSignalPool.push(task->signal);

    set_task_metrics(task);
    set_task_state(task, ATMI_COMPLETED);

    unlock_set(mutexes);
  }

  // this was created on heap when sink tasks was cleared, so reclaim that memory
  delete dispatched_tasks_ptr;

  do_progress(NULL);
}

bool handle_signal(hsa_signal_value_t value, void *arg) {
  //HandleSignalInvokeTimer.Stop();
  static bool is_called = false;
  if(!is_called) {
    set_thread_affinity(1);
    /*
       int policy;
       struct sched_param param;
       pthread_getschedparam(pthread_self(), &policy, &param);
       param.sched_priority = sched_get_priority_max(SCHED_RR);
       printf("Setting Priority Policy for %d: %d\n", SCHED_RR, param.sched_priority);
       pthread_setschedparam(pthread_self(), SCHED_RR, &param);
       */
    is_called = true;
  }
  HandleSignalTimer.Start();
  atl_task_t *task = NULL;
  atl_task_vector_t *dispatched_tasks_ptr = NULL;

  if(g_dep_sync_type == ATL_SYNC_CALLBACK)
    task = (atl_task_t *)arg;
  else if(g_dep_sync_type == ATL_SYNC_BARRIER_PKT) {
    dispatched_tasks_ptr = (atl_task_vector_t *)arg;
    task = (*dispatched_tasks_ptr)[0];
  }
  DEBUG_PRINT("Handle signal from task %lu\n", task->id);

  if(g_dep_sync_type == ATL_SYNC_CALLBACK) {
    handle_signal_callback(task);
  }
  else if(g_dep_sync_type == ATL_SYNC_BARRIER_PKT) {
    handle_signal_barrier_pkt(task, dispatched_tasks_ptr);
  }
  HandleSignalTimer.Stop();
  //HandleSignalInvokeTimer.Start();
  return false;
}

bool handle_group_signal(hsa_signal_value_t value, void *arg) {
  //HandleSignalInvokeTimer.Stop();
  static bool is_called = false;
  if(!is_called) {
    set_thread_affinity(1);
    /*
       int policy;
       struct sched_param param;
       pthread_getschedparam(pthread_self(), &policy, &param);
       param.sched_priority = sched_get_priority_max(SCHED_RR);
       printf("Setting Priority Policy for %d: %d\n", SCHED_RR, param.sched_priority);
       pthread_setschedparam(pthread_self(), SCHED_RR, &param);
       */
    is_called = true;
  }
  HandleSignalTimer.Start();
  TaskgroupImpl *taskgroup = (TaskgroupImpl *)arg;
  // TODO: what if the group task has dependents? this is not clear as of now.
  // we need to define dependencies between tasks and task groups better...

  // release resources
  lock(&(taskgroup->_group_mutex));
  hsa_signal_wait_acquire(taskgroup->_group_signal, HSA_SIGNAL_CONDITION_EQ, 0, UINT64_MAX, ATMI_WAIT_STATE);

  std::vector<atl_task_t *> group_tasks = taskgroup->_running_groupable_tasks;
  taskgroup->_running_groupable_tasks.clear();
  if(taskgroup->_ordered) {
    DEBUG_PRINT("Handling group signal callback for task group: %p size: %lu\n", taskgroup, group_tasks.size());
    for(int t = 0; t < group_tasks.size(); t++) {
      // does it matter which task was enqueued first, as long as we are
      // popping one task for every push?
      if(!taskgroup->_running_ordered_tasks.empty()) {
        DEBUG_PRINT("Removing task %lu with state: %d\n",
            taskgroup->_running_ordered_tasks.front()->id,
            taskgroup->_running_ordered_tasks.front()->state.load()
            );
        taskgroup->_running_ordered_tasks.pop_front();
      }
    }
  }
  taskgroup->_callback_started.clear();
  unlock(&(taskgroup->_group_mutex));

  std::set<pthread_mutex_t *> mutexes;
  for(std::vector<atl_task_t *>::iterator it = group_tasks.begin();
      it != group_tasks.end(); it++) {
    atl_task_t *task = *it;
    mutexes.insert(&(task->mutex));
    atl_kernel_t *kernel = task->kernel;
    atl_kernel_impl_t *kernel_impl = NULL;
    if(kernel) {
      kernel_impl = get_kernel_impl(kernel, task->kernel_id);
      mutexes.insert(&(kernel_impl->mutex));

      // process printf buffer
      {
        atl_kernel_impl_t *kernel_impl = NULL;
        if (task_process_fini_buffer) {
          if(task->kernel) {
            kernel_impl = get_kernel_impl(task->kernel, task->kernel_id);
            //printf("Task Id: %lu, kernel name: %s\n", task->id, kernel_impl->kernel_name.c_str());
            char *kargs = (char *)(task->kernarg_region);
            if(task->type == ATL_KERNEL_EXECUTION &&
                task->devtype == ATMI_DEVTYPE_GPU &&
                kernel_impl->kernel_type == AMDGCN) {
              atmi_implicit_args_t *impl_args = (atmi_implicit_args_t *)(kargs + (task->kernarg_region_size - sizeof(atmi_implicit_args_t)));
              (*task_process_fini_buffer)((void *)impl_args->pipe_ptr, MAX_PIPE_SIZE);
            }
          }
        }
      }

    }
    if(g_dep_sync_type == ATL_SYNC_BARRIER_PKT) {
      atl_task_vector_t &requires = task->and_predecessors;
      for(int idx = 0; idx < requires.size(); idx++) {
        mutexes.insert(&(requires[idx]->mutex));
      }
    }
  }
  mutexes.insert(&mutex_readyq_);
  lock_set(mutexes);
  for(std::vector<atl_task_t *>::iterator itg = group_tasks.begin();
      itg != group_tasks.end(); itg++) {
    atl_task_t *task = *itg;
    atl_kernel_t *kernel = task->kernel;
    atl_kernel_impl_t *kernel_impl = NULL;
    if(kernel) {
      kernel_impl = get_kernel_impl(kernel, task->kernel_id);
      kernel_impl->free_kernarg_segments.push(task->kernarg_region_index);
    }
    if(g_dep_sync_type == ATL_SYNC_BARRIER_PKT) {
      std::vector<hsa_signal_t> temp_list;
      temp_list.clear();
      // if num_successors == 0 then we can reuse their signal.
      atl_task_vector_t &requires = task->and_predecessors;
      for(atl_task_vector_t::iterator it = requires.begin();
          it != requires.end(); it++) {
        assert((*it)->state >= ATMI_DISPATCHED);
        (*it)->num_successors--;
        if((*it)->state >= ATMI_EXECUTED && (*it)->num_successors == 0) {
          // release signal because this predecessor is done waiting for
          temp_list.push_back((*it)->signal);
        }
      }
      for(std::vector<hsa_signal_t>::iterator it = temp_list.begin();
          it != temp_list.end(); it++) {
        //hsa_signal_store_relaxed((*it), 1);
        FreeSignalPool.push(*it);
        DEBUG_PRINT("[Handle Signal %lu ] Free Signal Pool Size: %lu; Ready Task Queue Size: %lu\n", task->id, FreeSignalPool.size(), ReadyTaskQueue.size());
      }
    }
    else if(g_dep_sync_type == ATL_SYNC_CALLBACK) {
      // TODO: we dont know how to specify dependencies between task groups and
      // individual tasks yet. once we figure that out, we need to include
      // logic here to push the successor tasks to the ready task queue.
    }
    DEBUG_PRINT("Completed task %lu in group\n", task->id);
    set_task_metrics(task);
    set_task_state(task, ATMI_COMPLETED);
  }
  unlock_set(mutexes);

  // make progress on all other ready tasks
  do_progress(taskgroup, group_tasks.size());
  DEBUG_PRINT("Releasing %lu tasks from %p task group\n", group_tasks.size(), taskgroup);
  taskgroup->_task_count -= group_tasks.size();
  if(g_dep_sync_type == ATL_SYNC_CALLBACK) {
    if(!taskgroup->_task_count.load()) {
      // after predecessor is done, decrement all successor's dependency count.
      // If count reaches zero, then add them to a 'ready' task list. Next,
      // dispatch all ready tasks in a round-robin manner to the available
      // GPU/CPU queues.
      // decrement reference count of its dependencies; add those with ref count = 0 to a
      // “ready” list
      lock(&(taskgroup->_group_mutex));
      atl_task_vector_t deps = taskgroup->_and_successors;
      taskgroup->_and_successors.clear();
      unlock(&(taskgroup->_group_mutex));
      DEBUG_PRINT("Deps list of %p [%lu]: ", taskgroup, deps.size());
      atl_task_vector_t temp_list;
      for(atl_task_vector_t::iterator it = deps.begin();
          it != deps.end(); it++) {
        // FIXME: should we be grabbing a lock on each successor before
        // decrementing their predecessor count? Currently, it may not be
        // required because there is only one callback thread, but what if there
        // were more?
        lock(&((*it)->mutex));
        DEBUG_PRINT(" %lu(%d) ", (*it)->id, (*it)->num_predecessors);
        (*it)->num_predecessors--;
        if((*it)->num_predecessors == 0) {
          // add to ready list
          temp_list.push_back(*it);
        }
        unlock(&((*it)->mutex));
      }
      lock(&mutex_readyq_);
      for(atl_task_vector_t::iterator it = temp_list.begin();
          it != temp_list.end(); it++) {
        // FIXME: if groupable task, push it in the right taskgroup
        // else push it to readyqueue
        ReadyTaskQueue.push(*it);
      }
      unlock(&mutex_readyq_);

      if(!temp_list.empty()) do_progress(taskgroup);
    }
  }
  HandleSignalTimer.Stop();
  //HandleSignalInvokeTimer.Start();
  // return true because we want the callback function to always be
  // 'registered' for any more incoming tasks belonging to this task group
  DEBUG_PRINT("Done handle_group_signal\n");
  return false;
}

void do_progress(TaskgroupImpl *taskgroup, int progress_count) {
  if(g_dep_sync_type == ATL_SYNC_CALLBACK) {
    if(taskgroup && taskgroup->_ordered) {
      // pop from front of ordered task list and try dispatch as many as possible
      bool should_dispatch = false;
      do {
        atl_task_t *ready_task = NULL;
        should_dispatch = false;
        lock(&taskgroup->_group_mutex);
        if (!taskgroup->_running_ordered_tasks.empty()) {
          ready_task = taskgroup->_running_ordered_tasks.front();
        }
        unlock(&taskgroup->_group_mutex);
        if (ready_task) {
          should_dispatch = try_dispatch(ready_task, NULL, ready_task->synchronous);
        }
      } while(should_dispatch);
    }
    else {
      lock(&mutex_readyq_);
      size_t queue_sz = ReadyTaskQueue.size();
      unlock(&mutex_readyq_);

      for(int i = 0; i < queue_sz; i++) {
        atl_task_t *ready_task = NULL;
        lock(&mutex_readyq_);
        if(!ReadyTaskQueue.empty()) {
          ready_task = ReadyTaskQueue.front();
          ReadyTaskQueue.pop();
        }
        unlock(&mutex_readyq_);

        if(ready_task) {
          try_dispatch(ready_task, NULL, ATMI_FALSE);
        }
      }
    }
  }
  else if (g_dep_sync_type == ATL_SYNC_BARRIER_PKT) {
    bool should_dispatch = false;
    do {
      atl_task_t *ready_task = NULL;
      should_dispatch = false;
      lock(&mutex_readyq_);
      if (!TaskList.empty()) {
        ready_task = TaskList.front();
      }
      unlock(&mutex_readyq_);
      if (ready_task) {
        should_dispatch = try_dispatch(ready_task, NULL, ATMI_FALSE);
      }
    } while(should_dispatch);
  }
}

void *acquire_kernarg_segment(atl_kernel_impl_t *impl, int *segment_id) {
  uint32_t kernel_segment_size = impl->kernarg_segment_size;
  void * ret_address = NULL;
  int free_idx = -1;
  lock(&(impl->mutex));
  if(!(impl->free_kernarg_segments.empty())) {
    free_idx = impl->free_kernarg_segments.front();
    DEBUG_PRINT("Acquiring Kernarg Segment Id: %d\n", free_idx);
    ret_address = (void *)((char *)impl->kernarg_region +
        (free_idx * kernel_segment_size));
    impl->free_kernarg_segments.pop();
  }
  else {
    fprintf(stderr, "No free kernarg segments. Increase MAX_NUM_KERNELS and recompile.\n");
  }
  unlock(&(impl->mutex));
  *segment_id = free_idx;
  return ret_address;
  // check if pool is empty by comparing the read and write indexes
  // if pool is empty
  //      extend existing pool
  //      if pool size is beyond a threshold max throw error
  // fi
  // get a free pool object and return
  //
  //
  // on handle signal, just return the free pool object?
}

std::vector<hsa_queue_t *> get_cpu_queues(atmi_place_t place) {
    ATLCPUProcessor &proc = get_processor<ATLCPUProcessor>(place);
    return proc.getQueues();
}

atmi_status_t dispatch_task(atl_task_t *task) {
  //DEBUG_PRINT("GPU Place Info: %d, %lx %lx\n", lparm->place.node_id, lparm->place.cpu_set, lparm->place.gpu_set);

  if(task->type == ATL_DATA_MOVEMENT) {
    return dispatch_data_movement(task, task->data_dest_ptr, task->data_src_ptr,
        task->data_size);
  }
  else {
    TryDispatchTimer.Start();
    TaskgroupImpl *taskgroup_obj = task->taskgroup_obj;
    /* get this taskgroup's HSA queue (could be dynamically mapped or round robin
     * if it is an unordered taskgroup */
    // FIXME: round robin for now, but may use some other load balancing algo
    // enqueue task's packet to that queue

    int proc_id = task->place.device_id;
    if(proc_id == -1) {
      // user is asking runtime to pick a device
      // TODO: best device of this type? pick 0 for now
      proc_id = 0;
    }
    hsa_queue_t* this_Q = task->packets[0].first;
    //printf("Task already populated with Q[%p, %lu]\n", this_Q, task->packets[0].second);
    if(!this_Q) return ATMI_STATUS_ERROR;

    int ndim = -1;
    if(task->gridDim[2] > 1)
      ndim = 3;
    else if(task->gridDim[1] > 1)
      ndim = 2;
    else
      ndim = 1;
    if(task->devtype == ATMI_DEVTYPE_GPU) {
      hsa_kernel_dispatch_packet_t *this_aql = NULL;
      uint64_t index = 0ull;
      index = task->packets[0].second;
      /* Find the queue index address to write the packet info into.  */
      const uint32_t queueMask = this_Q->size - 1;
      this_aql = &(((hsa_kernel_dispatch_packet_t*)(this_Q->base_address))[index&queueMask]);
      DEBUG_PRINT("K for %p (%lu) %p [%lu]\n", task, task->id, this_Q, index&queueMask);

      atl_kernel_impl_t *kernel_impl = get_kernel_impl(task->kernel, task->kernel_id);
      //this_aql->header = create_header(HSA_PACKET_TYPE_INVALID, ATMI_FALSE);
      //memset(this_aql, 0, sizeof(hsa_kernel_dispatch_packet_t));
      /*  FIXME: We need to check for queue overflow here. */
      //SignalAddTimer.Start();
      if(task->groupable == ATMI_TRUE) {
        lock(&(taskgroup_obj->_group_mutex));
        taskgroup_obj->_running_groupable_tasks.push_back(task);
        unlock(&(taskgroup_obj->_group_mutex));
      }
      //SignalAddTimer.Stop();
      this_aql->completion_signal = task->signal;

      /* pass this task handle to the kernel as an argument */
      char *kargs = (char *)(task->kernarg_region);
      // AMDGCN device libs assume that first three args beyond the kernel args are grid
      // offsets in X, Y and Z dimensions
      atmi_implicit_args_t *impl_args = (atmi_implicit_args_t *)(kargs + (task->kernarg_region_size - sizeof(atmi_implicit_args_t)));
      impl_args->offset_x = 0;
      impl_args->offset_y = 0;
      impl_args->offset_z = 0;
      // char *pipe_ptr = impl_args->pipe_ptr;

      // initialize printf buffer
      {
        atl_kernel_impl_t *kernel_impl = NULL;
        if (task_process_init_buffer) {
          if(task->kernel) {
            kernel_impl = get_kernel_impl(task->kernel, task->kernel_id);
            //printf("Task Id: %lu, kernel name: %s\n", task->id, kernel_impl->kernel_name.c_str());
            char *kargs = (char *)(task->kernarg_region);
            if(task->type == ATL_KERNEL_EXECUTION &&
                task->devtype == ATMI_DEVTYPE_GPU &&
                kernel_impl->kernel_type == AMDGCN) {
              atmi_implicit_args_t *impl_args = (atmi_implicit_args_t *)(kargs + (task->kernarg_region_size - sizeof(atmi_implicit_args_t)));

              // initalize printf buffer
              (*task_process_init_buffer)((void *)impl_args->pipe_ptr, MAX_PIPE_SIZE);
            }
          }
        }
      }

      /*  Process task values */
      /*  this_aql.dimensions=(uint16_t) ndim; */
      this_aql->setup  |= (uint16_t) ndim << HSA_KERNEL_DISPATCH_PACKET_SETUP_DIMENSIONS;
      this_aql->grid_size_x=task->gridDim[0];
      this_aql->workgroup_size_x=task->groupDim[0];
      if (ndim>1) {
        this_aql->grid_size_y=task->gridDim[1];
        this_aql->workgroup_size_y=task->groupDim[1];
      } else {
        this_aql->grid_size_y=1;
        this_aql->workgroup_size_y=1;
      }
      if (ndim>2) {
        this_aql->grid_size_z=task->gridDim[2];
        this_aql->workgroup_size_z=task->groupDim[2];
      } else {
        this_aql->grid_size_z=1;
        this_aql->workgroup_size_z=1;
      }

      /*  Bind kernel argument buffer to the aql packet.  */
      this_aql->kernarg_address = task->kernarg_region;
      this_aql->kernel_object = kernel_impl->kernel_objects[proc_id];
      this_aql->private_segment_size = kernel_impl->private_segment_sizes[proc_id];
      this_aql->group_segment_size = kernel_impl->group_segment_sizes[proc_id];

      this_aql->reserved2 = task->id;
      set_task_state(task, ATMI_DISPATCHED);
      /*  Prepare and set the packet header */
      packet_store_release((uint32_t*) this_aql,
          create_header(HSA_PACKET_TYPE_KERNEL_DISPATCH, taskgroup_obj->_ordered,
                        task->acquire_scope, task->release_scope),
          this_aql->setup);
      /* Increment write index and ring doorbell to dispatch the kernel.  */
      //hsa_queue_store_write_index_relaxed(this_Q, index+1);
      hsa_signal_store_relaxed(this_Q->doorbell_signal, index);
    }
    else if(task->devtype == ATMI_DEVTYPE_CPU) {
      int thread_count = task->gridDim[0] * task->gridDim[1] * task->gridDim[2];
      std::vector<hsa_queue_t *> this_queues = get_cpu_queues(task->place);
      int q_count = this_queues.size();
      struct timespec dispatch_time;
      clock_gettime(CLOCK_MONOTONIC_RAW,&dispatch_time);
      /* this "virtual" task encompasses thread_count number of ATMI CPU tasks performing
       * data parallel SPMD style of processing
       */
      if(task->groupable == ATMI_TRUE) {
        lock(&(taskgroup_obj->_group_mutex));
        taskgroup_obj->_running_groupable_tasks.push_back(task);
        unlock(&(taskgroup_obj->_group_mutex));
      }
      for(int tid = 0; tid < thread_count; tid++) {
        hsa_agent_dispatch_packet_t *this_aql = NULL;
        uint64_t index = 0ull;
        hsa_queue_t *this_Q = task->packets[tid].first;
        index = task->packets[tid].second;
        /* Find the queue index address to write the packet info into.  */
        const uint32_t queueMask = this_Q->size - 1;
        this_aql = &(((hsa_agent_dispatch_packet_t*)(this_Q->base_address))[index&queueMask]);

        memset(this_aql, 0, sizeof(hsa_agent_dispatch_packet_t));
        /*  FIXME: We need to check for queue overflow here. Do we need
         *  to do this for CPU agents too? */
        //SignalAddTimer.Start();
        //hsa_signal_store_relaxed(task->signal, 1);
        //SignalAddTimer.Stop();
        this_aql->completion_signal = task->signal;

        /* Set the type and return args.*/
        // FIXME FIXME FIXME: Use the hierarchical pif-kernel table to
        // choose the best kernel. Don't use a flat table structure
        this_aql->type = (uint16_t)get_kernel_index(task->kernel, task->kernel_id);
        /* FIXME: We are considering only void return types for now.*/
        //this_aql->return_address = NULL;
        /* Set function args */
        this_aql->arg[0] = (uint64_t) task->id;
        this_aql->arg[1] = (uint64_t) task->kernarg_region;
        this_aql->arg[2] = (uint64_t) task->kernel; // pass task handle to fill in metrics
        this_aql->arg[3] = tid; // tasks can query for current task ID

        /*  Prepare and set the packet header */
        /* FIXME: CPU tasks ignore barrier bit as of now. Change
         * implementation? I think it doesn't matter because we are
         * executing the subroutines one-by-one, so barrier bit is
         * inconsequential.
         */
        packet_store_release((uint32_t*) this_aql,
            create_header(HSA_PACKET_TYPE_AGENT_DISPATCH, taskgroup_obj->_ordered,
                          task->acquire_scope, task->release_scope),
            this_aql->type);

      }
      set_task_state(task, ATMI_DISPATCHED);
      /* Store dispatched time */
      if(task->profilable == ATMI_TRUE && task->atmi_task)
        task->atmi_task->profile.dispatch_time = get_nanosecs(context_init_time, dispatch_time);
      // FIXME: in the current logic, multiple CPU threads are round-robin
      // scheduled across queues from Q0. So, just ring the doorbell on only
      // the queues that are touched and leave the other queues alone
      int doorbell_count = (q_count < thread_count) ? q_count : thread_count;
      for(int q = 0; q < doorbell_count; q++) {
        /* fetch write index and ring doorbell on one lesser value
         * (because the add index call will have incremented it already by
         * 1 and dispatch the kernel.  */
        uint64_t index = hsa_queue_load_write_index_acquire(this_queues[q]) - 1;
        hsa_signal_store_relaxed(this_queues[q]->doorbell_signal, index);
        signal_worker(this_queues[q], PROCESS_PKT);
      }
    }
    TryDispatchTimer.Stop();
    DEBUG_PRINT("Task %lu (%d) Dispatched\n", task->id, task->devtype);
    return ATMI_STATUS_SUCCESS;
  }
}

void *try_grab_kernarg_region(atl_task_t *ret, int *free_idx) {
  atl_kernel_impl_t *kernel_impl = get_kernel_impl(ret->kernel, ret->kernel_id);
  uint32_t kernarg_segment_size = kernel_impl->kernarg_segment_size;
  /* acquire_kernarg_segment and copy args here before dispatch */
  void * ret_address = NULL;
  if(!(kernel_impl->free_kernarg_segments.empty())) {
    *free_idx = kernel_impl->free_kernarg_segments.front();
    DEBUG_PRINT("Acquiring Kernarg Segment Id: %p\n", free_idx);
    ret_address = (void *)((char *)kernel_impl->kernarg_region +
        ((*free_idx) * kernarg_segment_size));
    kernel_impl->free_kernarg_segments.pop();
  }
  return ret_address;
}

void set_kernarg_region(atl_task_t *ret, void **args) {
  char *thisKernargAddress = (char *)(ret->kernarg_region);
  if(ret->kernel->num_args && thisKernargAddress == NULL) {
    fprintf(stderr, "Unable to allocate/find free kernarg segment\n");
  }
  atl_kernel_impl_t *kernel_impl = get_kernel_impl(ret->kernel, ret->kernel_id);

  // Argument references will be copied to a contiguous memory region here
  // TODO: resolve all data affinities before copying, depending on
  // atmi_data_affinity_policy_t: ATMI_COPY, ATMI_NOCOPY
  atmi_place_t place = ret->place;
  for(int i = 0; i < ret->kernel->num_args; i++) {
    memcpy(thisKernargAddress + kernel_impl->arg_offsets[i],
           args[i],
           ret->kernel->arg_sizes[i]);
    //hsa_memory_register(thisKernargAddress, ???
    DEBUG_PRINT("Arg[%d] = %p\n",
                i,
                *(void **)((char *)thisKernargAddress + kernel_impl->arg_offsets[i]));
  }
}

void acquire_aql_packet(atl_task_t *task) {
  TaskgroupImpl *taskgroup_obj = task->taskgroup_obj;
  if(task->type == ATL_KERNEL_EXECUTION) {
    // get AQL queue for GPU tasks and CPU tasks
    hsa_queue_t* this_Q = NULL;
    if(task->devtype == ATMI_DEVTYPE_GPU)
      this_Q = taskgroup_obj->chooseQueueFromPlace<ATLGPUProcessor>(task->place);
    else if(task->devtype == ATMI_DEVTYPE_CPU)
      this_Q = taskgroup_obj->chooseQueueFromPlace<ATLCPUProcessor>(task->place);
    if(!this_Q) ATMIErrorCheck(Getting queue for dispatch, ATMI_STATUS_ERROR);
    if(g_dep_sync_type == ATL_SYNC_BARRIER_PKT) {
      // enqueue barrier if and_predecessors is not empty
      if (!(task->and_predecessors.empty())) {
        enqueue_barrier(task, this_Q, task->and_predecessors.size(), &(task->and_predecessors[0]),
            SNK_NOWAIT, SNK_AND, task->devtype);
      }
    }

    if(task->devtype == ATMI_DEVTYPE_GPU) {
      hsa_signal_add_acq_rel(task->signal, 1);
      /*  Obtain the current queue write index. increases with each call to kernel  */
      //uint64_t index = hsa_queue_add_write_index_relaxed(this_Q, 1);
      // Atomically request a new packet ID.
      uint64_t index = hsa_queue_add_write_index_relaxed(this_Q, 1);
      // Wait until the queue is not full before writing the packet
      DEBUG_PRINT("Queue %p ordered? %d task: %lu index: %lu\n", this_Q, taskgroup_obj->_ordered, task->id, index);
      //printf("%lu, %lu, K\n", task->id, index);
      while(index - hsa_queue_load_read_index_acquire(this_Q) >= this_Q->size);
      task->packets.push_back(std::make_pair(this_Q, index));
    }
    else if(task->devtype == ATMI_DEVTYPE_CPU) {
      std::vector<hsa_queue_t *> this_queues = get_cpu_queues(task->place);
      int q_count = this_queues.size();
      int thread_count = task->gridDim[0] * task->gridDim[1] * task->gridDim[2];
      if(thread_count == 0) {
        fprintf(stderr, "WARNING: one of the dimensionsions is set to 0 threads. \
            Choosing 1 thread by default. \n");
        thread_count = 1;
      }
      if(thread_count == 1) {
        int q;
        for(q = 0; q < q_count; q++) {
          if(this_queues[q] == this_Q)
            break;
        }
        hsa_queue_t *tmp = this_queues[0];
        this_queues[0] = this_queues[q];
        this_queues[q] = tmp;
      }
      hsa_signal_add_acq_rel(task->signal, thread_count);
      for(int tid = 0; tid < thread_count; tid++) {
        hsa_queue_t *this_queue = this_queues[tid % q_count];
        /*  Obtain the current queue write index. increases with each call to kernel  */
        uint64_t index = hsa_queue_add_write_index_relaxed(this_queue, 1);
        while(index - hsa_queue_load_read_index_acquire(this_queue) >= this_queue->size);
        /* Find the queue index address to write the packet info into.  */
        task->packets.push_back(std::make_pair(this_queue, index));
      }
    }
  }
  else {
    // get signal for SDMA and increment it accordingly
    hsa_status_t err;
    void *src = task->data_src_ptr;
    void *dest = task->data_dest_ptr;
    size_t size = task->data_size;
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
    void *temp_host_ptr;
    const void *src_ptr = src;
    void *dest_ptr = dest;
    volatile unsigned type;
    if(is_src_host && is_dest_host) {
      type = ATMI_H2H;
    }
    else if(src_data && !dest_data) {
      type = ATMI_D2H;
    }
    else if(!src_data && dest_data) {
      type = ATMI_H2D;
    }
    else {
      type = ATMI_D2D;
    }

    if(type == ATMI_H2D || type == ATMI_D2H)
      hsa_signal_add_acq_rel(task->signal, 2);
    else
      hsa_signal_add_acq_rel(task->signal, 1);
  }
}

bool try_dispatch_barrier_pkt(atl_task_t *ret, void **args) {

  bool resources_available = true;
  bool should_dispatch = true;
  hsa_signal_t new_signal;

  //lock(&mutex_readyq_);
  // try dispatching the head of TaskList
  //atl_task_t *ret = TaskList.front();
  //unlock(&mutex_readyq_);

  std::set<pthread_mutex_t *> req_mutexes;
  std::vector<atl_task_t *> &temp_vecs = ret->predecessors;
  req_mutexes.clear();
  for(int idx = 0; idx < ret->predecessors.size(); idx++) {
    atl_task_t *pred_task = temp_vecs[idx];
    req_mutexes.insert((pthread_mutex_t *)&(pred_task->mutex));
  }
  req_mutexes.insert((pthread_mutex_t *)&(ret->mutex));
  req_mutexes.insert(&mutex_readyq_);
  if(ret->prev_ordered_task) req_mutexes.insert(&(ret->prev_ordered_task->mutex));
  atl_kernel_impl_t *kernel_impl = NULL;
  if(ret->kernel) {
    kernel_impl = get_kernel_impl(ret->kernel, ret->kernel_id);
    if(kernel_impl) req_mutexes.insert(&(kernel_impl->mutex));
  }
  TaskgroupImpl *taskgroup_obj = ret->taskgroup_obj;

  lock_set(req_mutexes);
  // assert that this task is TaskList.front() because there should be no other scenario
  //assert(ret == TaskList.front());
  //if(ret != TaskList.front()) {
  //  fprintf(stderr, "Task (%p --> %lu) not at front!\n", ret, ret->id);
  //  exit(-1);
  //}
  //std::cout << "[" << ret->id << "]Signals Before: " << FreeSignalPool.size() << std::endl;

  if(ret->state >= ATMI_READY) {
    // If someone else is trying to dispatch this task, give up
    DEBUG_PRINT("Some other thread trying to dispatch task %lu, giving up\n", ret->id);
    unlock_set(req_mutexes);
    return false;
  }
  if(should_dispatch) {
    // find if all dependencies are satisfied
    int dep_count = 0;
    for (int idx = 0; idx < ret->predecessors.size(); idx++) {
      atl_task_t* pred_task = temp_vecs[idx];
      DEBUG_PRINT("Task %lu depends on %lu as %d th predecessor ",ret->id, pred_task->id, idx);
      if(pred_task->state < ATMI_DISPATCHED) {
        /* still in ready queue and not received a signal */
        should_dispatch = false;
        DEBUG_PRINT("(waiting)\n");
        waiting_count++;
      }
      else{
        DEBUG_PRINT("(dispatched)\n");
      }
    }
    if(ret->prev_ordered_task) {
      DEBUG_PRINT("Task %lu depends on %lu as ordered predecessor ",  ret->id, ret->prev_ordered_task->id);
      if(ret->prev_ordered_task->state < ATMI_DISPATCHED) {
        should_dispatch = false;
        DEBUG_PRINT("(waiting)\n");
        waiting_count++;
      }
      else {
        DEBUG_PRINT("(dispatched)\n");
      }
    }
  }

  if(should_dispatch) {
    if(FreeSignalPool.empty()
        || (kernel_impl && kernel_impl->free_kernarg_segments.empty())) {
      should_dispatch = false;
      resources_available = false;
      ret->and_predecessors.clear();
      DEBUG_PRINT("Do not dispatch because (signals: %lu, kernels: %lu, ready tasks: %lu)\n", FreeSignalPool.size(),
          kernel_impl->free_kernarg_segments.size(),
          ReadyTaskQueue.size());
    }
  }

  if(should_dispatch) {
    if(ret->groupable != ATMI_TRUE) {
      // this is a task that uses individual signals (not taskgroup-signals)
      new_signal = FreeSignalPool.front();
      FreeSignalPool.pop();
      ret->signal = new_signal;
    }
    else {
      ret->signal = taskgroup_obj->getSignal();
    }
    if(ret->kernel) {
      // get kernarg resource
      uint32_t kernarg_segment_size = kernel_impl->kernarg_segment_size;
      int free_idx = kernel_impl->free_kernarg_segments.front();
      ret->kernarg_region_index = free_idx;
      DEBUG_PRINT("Acquiring Kernarg Segment Id: %d\n", free_idx);
      void *addr = (void *)((char *)kernel_impl->kernarg_region +
          (free_idx * kernarg_segment_size));
      kernel_impl->free_kernarg_segments.pop();
      if(ret->kernarg_region != NULL) {
        // we had already created a memory region using malloc. Copy it
        // to the newly availed space
        size_t size_to_copy = ret->kernarg_region_size;
        if(ret->devtype == ATMI_DEVTYPE_GPU && kernel_impl->kernel_type == AMDGCN) {
          // do not copy the implicit args from saved region
          // they are to be set/reset during task dispatch
          size_to_copy -= sizeof(atmi_implicit_args_t);
        }
        memcpy(addr, ret->kernarg_region, size_to_copy);
        // free existing region
        free(ret->kernarg_region);
        ret->kernarg_region = addr;
      }
      else {
        // first time allocation/assignment
        ret->kernarg_region = addr;
        set_kernarg_region(ret, args);
      }
    }

    for(int idx = 0; idx < ret->predecessors.size(); idx++) {
      atl_task_t *pred_task = temp_vecs[idx];
      if(pred_task->state /*.load(std::memory_order_seq_cst)*/ < ATMI_EXECUTED) {
        pred_task->num_successors++;
        DEBUG_PRINT("Task %p (%lu) adding %p (%lu) as successor\n",
            pred_task, pred_task->id, ret, ret->id);
        ret->and_predecessors.push_back(pred_task);
      }
    }
    DispatchedTasks.push_back(ret);
    // we will be dispatching the task in front of the deque, pop it
    // what if two threads have peeked at a different task and try_dispatch
    // is called in a different order? does it matter if the tasks are popped
    // out of order?
    TaskList.pop_front();
    // save the current set of sink tasks
    SinkTasks.insert(ret);
    atl_task_vector_t::iterator it = ret->and_predecessors.begin();
    for(; it != ret->and_predecessors.end(); it++) {
      // The predecessors are no longer sink tasks (if they were until now). The current
      // task will be the sink task for its predecessors
      SinkTasks.erase(*it);
    }
    if(ret->prev_ordered_task) {
      // remove previous task in the ordered list from sink tasks
      SinkTasks.erase(ret->prev_ordered_task);

      // If the previous ordered task was for a different queue type, then add to
      // and_predecessors (from SDMA to GPU or CPU to GPU or CPU to GPU or SDMA to CPU).
      // Later, enqueue barrier SDMA or CPU/GPU to GPU/CPU. If CPU/GPU to SDMA, then
      // ROCr SDMA API handles dependencies separately.
      if(taskgroup_obj->_ordered && ret->prev_ordered_task) {
        if((ret->prev_ordered_task->type == ATL_DATA_MOVEMENT && ret->type == ATL_KERNEL_EXECUTION)
           || (ret->prev_ordered_task->type == ATL_KERNEL_EXECUTION && ret->type == ATL_DATA_MOVEMENT)
           || (ret->prev_ordered_task->devtype==ATMI_DEVTYPE_GPU && ret->devtype==ATMI_DEVTYPE_CPU)
           || (ret->prev_ordered_task->devtype==ATMI_DEVTYPE_CPU && ret->devtype==ATMI_DEVTYPE_GPU)){
            atl_task_t *pred_task = ret->prev_ordered_task;
            if(pred_task->state /*.load(std::memory_order_seq_cst)*/ < ATMI_EXECUTED) {
              pred_task->num_successors++;
              DEBUG_PRINT("Task %p (%lu) adding %p (%lu) as successor\n",
                pred_task, pred_task->id, ret, ret->id);
              ret->and_predecessors.push_back(pred_task);
          }
        }
      }
    }

    acquire_aql_packet(ret);

    // now the task (kernel/data movement) has the signal/kernarg resource
    // and can be set to the "ready" state
    set_task_state(ret, ATMI_READY);
  }
  else {
    if(ret->kernel && ret->kernarg_region == NULL) {
      // first time allocation/assignment
      ret->kernarg_region = malloc(ret->kernarg_region_size);
      //ret->kernarg_region_copied = true;
      set_kernarg_region(ret, args);
    }
    max_ready_queue_sz++;
    set_task_state(ret, ATMI_INITIALIZED);
  }
  DEBUG_PRINT("[Try Dispatch] Free Signal Pool Size: %lu; Ready Task Queue Size: %lu\n", FreeSignalPool.size(), ReadyTaskQueue.size());
  unlock_set(req_mutexes);
  return should_dispatch;
}

bool try_dispatch_callback(atl_task_t *ret, void **args) {
  bool should_try_dispatch = true;
  bool resources_available = true;
  bool predecessors_complete = true;
  bool should_dispatch = false;

  std::set<pthread_mutex_t *> req_mutexes;
  std::vector<atl_task_t *> &temp_vecs = ret->predecessors;
  req_mutexes.clear();
  for(int idx = 0; idx < ret->predecessors.size(); idx++) {
    atl_task_t *pred_task = ret->predecessors[idx];
    req_mutexes.insert((pthread_mutex_t *)&(pred_task->mutex));
  }
  req_mutexes.insert((pthread_mutex_t *)&(ret->mutex));
  req_mutexes.insert(&mutex_readyq_);
  if(ret->prev_ordered_task) req_mutexes.insert(&(ret->prev_ordered_task->mutex));
  TaskgroupImpl *taskgroup_obj = ret->taskgroup_obj;
  req_mutexes.insert(&(taskgroup_obj->_group_mutex));
  atl_kernel_impl_t *kernel_impl = NULL;
  if(ret->kernel) {
    kernel_impl = get_kernel_impl(ret->kernel, ret->kernel_id);
    req_mutexes.insert(&(kernel_impl->mutex));
  }
  lock_set(req_mutexes);

  if(ret->state >= ATMI_READY) {
    // If someone else is trying to dispatch this task, give up
    unlock_set(req_mutexes);
    return false;
  }
  // do not add predecessor-successor link if task is already initialized using
  // create-activate pattern
  if(ret->state < ATMI_INITIALIZED && should_try_dispatch) {
    if(!ret->predecessors.empty()) {
      // add to its predecessor's dependents list and return
      for(int idx = 0; idx < ret->predecessors.size(); idx++) {
        atl_task_t *pred_task = ret->predecessors[idx];
        DEBUG_PRINT("Task %p depends on %p as predecessor ",
            ret, pred_task);
        if(pred_task->state /*.load(std::memory_order_seq_cst)*/ < ATMI_EXECUTED) {
          should_try_dispatch = false;
          predecessors_complete = false;
          pred_task->and_successors.push_back(ret);
          ret->num_predecessors++;
          DEBUG_PRINT("(waiting)\n");
          waiting_count++;
        }
        else {
          DEBUG_PRINT("(completed)\n");
        }
      }
    }
    if(ret->pred_taskgroup_objs.size() > 0) {
      // add to its predecessor's dependents list and return
      DEBUG_PRINT("Task %lu has %lu predecessor task groups\n", ret->id, ret->pred_taskgroup_objs.size());
      for(int idx = 0; idx < ret->pred_taskgroup_objs.size(); idx++) {
        TaskgroupImpl *pred_tg = ret->pred_taskgroup_objs[idx];
        DEBUG_PRINT("Task %p depends on %p as predecessor task group ",
            ret, pred_tg);
        if(pred_tg && pred_tg->_task_count.load() > 0) {
          // predecessor task group is still running, so add yourself to its successor list
          should_try_dispatch = false;
          predecessors_complete = false;
          pred_tg->_and_successors.push_back(ret);
          ret->num_predecessors++;
          DEBUG_PRINT("(waiting)\n");
        }
        else {
          DEBUG_PRINT("(completed)\n");
        }
      }
    }
  }

  if(ret->prev_ordered_task && should_try_dispatch) {
    DEBUG_PRINT("Task %lu depends on %lu as ordered predecessor ",
        ret->id, ret->prev_ordered_task->id);
    // if this task is of a certain type and its previous task was also of the same type,
    // then we can dispatch this task if the previous task has also been dispatched
    // (received task signal and set its value)
    if(ret->prev_ordered_task->state < ATMI_READY && (
        (ret->prev_ordered_task->type == ATL_DATA_MOVEMENT && ret->type == ATL_DATA_MOVEMENT)
        || (ret->prev_ordered_task->type==ATL_KERNEL_EXECUTION && ret->type==ATL_KERNEL_EXECUTION
          && ((ret->prev_ordered_task->devtype==ATMI_DEVTYPE_CPU && ret->devtype==ATMI_DEVTYPE_CPU)
          || (ret->prev_ordered_task->devtype==ATMI_DEVTYPE_GPU && ret->devtype==ATMI_DEVTYPE_GPU)))
    )) {
      should_try_dispatch = false;
      predecessors_complete = false;
      DEBUG_PRINT("(waiting)\n");
      waiting_count++;
    }
    // if this task is of a certain type and its previous task is of a different type,
    // then we can dispatch this task ONLY if the previous task has been at least
    // executed; and also add the previous task as a predecessor
    else if(ret->prev_ordered_task->state < ATMI_EXECUTED && (
         (ret->prev_ordered_task->type == ATL_DATA_MOVEMENT && ret->type == ATL_KERNEL_EXECUTION)
         || (ret->prev_ordered_task->type == ATL_KERNEL_EXECUTION && ret->type == ATL_DATA_MOVEMENT)
         || (ret->prev_ordered_task->devtype==ATMI_DEVTYPE_GPU && ret->devtype==ATMI_DEVTYPE_CPU)
         || (ret->prev_ordered_task->devtype==ATMI_DEVTYPE_CPU && ret->devtype==ATMI_DEVTYPE_GPU))
    ){
      should_try_dispatch = false;
      predecessors_complete = false;
      if(ret->state < ATMI_INITIALIZED) {
        ret->prev_ordered_task->and_successors.push_back(ret);
        ret->num_predecessors++;
      }
      DEBUG_PRINT("(waiting)\n");
      waiting_count++;
    }
    else {
      DEBUG_PRINT("(dispatched)\n");
    }
  }

  if(should_try_dispatch) {
    if((kernel_impl && kernel_impl->free_kernarg_segments.empty()) ||
        (ret->groupable == ATMI_FALSE && FreeSignalPool.empty())) {
      should_try_dispatch = false;
      resources_available = false;
    }
  }

  if(should_try_dispatch) {
    // try to dispatch if
    // a) you are using callbacks to resolve dependencies and all
    // your predecessors are done executing, OR
    // b) you are using barrier packets, in which case always try
    // to launch if you have a free signal at hand
    if(ret->groupable == ATMI_TRUE) {
      ret->signal = taskgroup_obj->getSignal();
    }
    else {
      // get a free signal
      hsa_signal_t new_signal = FreeSignalPool.front();
      ret->signal = new_signal;
      DEBUG_PRINT("Before pop Signal handle: %" PRIu64 " Signal value:%ld (Signal pool sz: %lu)\n", ret->signal.handle, hsa_signal_load_relaxed(ret->signal),
          FreeSignalPool.size());
      FreeSignalPool.pop();
      DEBUG_PRINT("[Try Dispatch] Free Signal Pool Size: %lu; Ready Task Queue Size: %lu\n", FreeSignalPool.size(), ReadyTaskQueue.size());
      //unlock(&mutex_readyq_);
    }
    if(ret->kernel) {
      // get kernarg resource
      uint32_t kernarg_segment_size = kernel_impl->kernarg_segment_size;
      int free_idx = kernel_impl->free_kernarg_segments.front();
      DEBUG_PRINT("Acquiring Kernarg Segment Id: %d\n", free_idx);
      ret->kernarg_region_index = free_idx;
      void *addr = (void *)((char *)kernel_impl->kernarg_region +
          (free_idx * kernarg_segment_size));
      kernel_impl->free_kernarg_segments.pop();
      assert(!(ret->kernel->num_args) || (ret->kernel->num_args && (ret->kernarg_region || args)));
      if(ret->kernarg_region != NULL) {
        // we had already created a memory region using malloc. Copy it
        // to the newly availed space
        size_t size_to_copy = ret->kernarg_region_size;
        if(ret->devtype == ATMI_DEVTYPE_GPU && kernel_impl->kernel_type == AMDGCN) {
          // do not copy the implicit args from saved region
          // they are to be set/reset during task dispatch
          size_to_copy -= sizeof(atmi_implicit_args_t);
        }
        if(size_to_copy) memcpy(addr, ret->kernarg_region, size_to_copy);
        // free existing region
        free(ret->kernarg_region);
        ret->kernarg_region = addr;
      }
      else {
        // first time allocation/assignment
        ret->kernarg_region = addr;
        set_kernarg_region(ret, args);
      }
    }

    if(taskgroup_obj->_ordered)
      taskgroup_obj->_running_ordered_tasks.pop_front();

    acquire_aql_packet(ret);

    // now the task (kernel/data movement) has the signal/kernarg resource
    // and can be set to the "ready" state
    set_task_state(ret, ATMI_READY);
  }
  else {
    if(ret->kernel && ret->kernarg_region == NULL) {
      // first time allocation/assignment
      ret->kernarg_region = malloc(ret->kernarg_region_size);
      //ret->kernarg_region_copied = true;
      set_kernarg_region(ret, args);
    }
    set_task_state(ret, ATMI_INITIALIZED);
  }
  if(predecessors_complete == true &&
      resources_available == false  &&
      ret->taskgroup_obj->_ordered == false) {
    // Ready task but no resources available. So, we push it to a
    // ready queue
    ReadyTaskQueue.push(ret);
    max_ready_queue_sz++;
  }

  unlock_set(req_mutexes);
  return should_try_dispatch;
}

atl_task_t *get_new_task() {
  atl_task_t *ret = new atl_task_t;
  memset(ret, 0, sizeof(atl_task_t));
  //ret->is_continuation = false;
  lock(&mutex_all_tasks_);
  AllTasks.push_back(ret);
  set_task_state(ret, ATMI_UNINITIALIZED);
  atmi_task_handle_t new_id;
  //new_id.node = 0;
  set_task_handle_ID(&new_id, AllTasks.size() - 1);
  //new_id.lo = AllTasks.size() - 1;
  //PublicTaskMap[new_id] = ret;
  unlock(&mutex_all_tasks_);
  ret->id = new_id;
  ret->and_successors.clear();
  ret->and_predecessors.clear();
  ret->predecessors.clear();
  ret->continuation_task = NULL;
  ret->pred_taskgroup_objs.clear();
  pthread_mutex_init(&(ret->mutex), NULL);
  return ret;
}

void enqueue_barrier_tasks(atl_task_vector_t tasks) {
  if(!tasks.empty()) {
    hsa_signal_store_relaxed(IdentityANDSignal, 1);
    atl_task_t *task = tasks[tasks.size() - 1];
    // for all sink tasks, enqueue a barrier packet
    hsa_queue_t *queue = NULL;
    // task->packets will be set by try_dispatch only for
    // kernel tasks and not data movement tasks; so we pick
    // an arbitrary queue just to place the BP if no obvious
    // queue is found.
    if(!task->packets.empty())
      queue = task->packets[0].first;
    else
      queue = task->taskgroup_obj->chooseQueueFromPlace<ATLGPUProcessor>(task->place);
    enqueue_barrier(task, queue, tasks.size(), &(tasks[0]), SNK_NOWAIT, SNK_AND,
        task->devtype, true);
  }
}

bool try_dispatch(atl_task_t *ret, void **args, boolean synchronous) {
  ShouldDispatchTimer.Start();
  bool should_dispatch = true;
  bool register_task_callback = (ret->groupable != ATMI_TRUE);
  if(g_dep_sync_type == ATL_SYNC_CALLBACK) {
    should_dispatch = try_dispatch_callback(ret, args);
  }
  else if(g_dep_sync_type == ATL_SYNC_BARRIER_PKT) {
    should_dispatch = try_dispatch_barrier_pkt(ret, args);
  }
  ShouldDispatchTimer.Stop();

  if(should_dispatch) {
    if(ret->atmi_task) {
      // FIXME: set a lookup table for dynamic parallelism kernels
      // perhaps?
      ret->atmi_task->handle = (void *)(&(ret->signal));
    }
    //direct_dispatch++;
    dispatch_task(ret);

    RegisterCallbackTimer.Start();
    if (register_task_callback) {
      if (g_dep_sync_type == ATL_SYNC_CALLBACK) {
        DEBUG_PRINT("Registering callback for task %lu\n", ret->id);
        hsa_status_t err = hsa_amd_signal_async_handler(ret->signal, HSA_SIGNAL_CONDITION_EQ, 0, handle_signal, (void *)ret);
        ErrorCheck(Creating signal handler, err);
      }
    } else {
      if (!ret->taskgroup_obj->_callback_started.test_and_set()) {
        DEBUG_PRINT("Registering callback for task groups\n");
        hsa_status_t err = hsa_amd_signal_async_handler(ret->signal, HSA_SIGNAL_CONDITION_EQ, 0, handle_group_signal,(void *)(ret->taskgroup_obj));
        ErrorCheck(Creating signal handler, err);
      }
    }
    RegisterCallbackTimer.Stop();
  }
  else {
    if(g_dep_sync_type == ATL_SYNC_BARRIER_PKT) {
      // We could not dispatch the packet because of lack of resources.
      // So, create a barrier packet for current sink tasks and add async handler
      // for its completion.
      // All dispatched tasks get signal from device-only signal pool. All async
      // handlers are registered to interruptible signals
      atl_task_t *last_dispatched_task = NULL;
      atl_task_vector_t tasks;
      atl_task_vector_t *dispatched_tasks_ptr = NULL;
      lock(&mutex_readyq_);
      if(!SinkTasks.empty()) {
        // this also means that DispatchedTasks is not empty
        tasks.insert(tasks.end(), SinkTasks.begin(), SinkTasks.end());
        SinkTasks.clear();
        // will be deleted in the callback
        dispatched_tasks_ptr = new atl_task_vector_t;
        dispatched_tasks_ptr->insert(dispatched_tasks_ptr->end(), DispatchedTasks.begin(), DispatchedTasks.end());
        DispatchedTasks.clear();
        last_dispatched_task = tasks[tasks.size() - 1];
      }
      unlock(&mutex_readyq_);
      if(dispatched_tasks_ptr) {
        enqueue_barrier_tasks(tasks);
        if(last_dispatched_task) {
          hsa_amd_signal_async_handler(
              IdentityANDSignal, HSA_SIGNAL_CONDITION_EQ, 0,
              handle_signal, (void *)dispatched_tasks_ptr);
        }
      }
      // else this task did not get the resources AND DispatchedTasks
      // is already being processed, so this task will be taken care of
      // when do_progress handles the TaskList
    }
  }
  if ( synchronous == ATMI_TRUE ) { /*  Sychronous execution */
    /* For default synchrnous execution, wait til kernel is finished.  */
    //    TaskWaitTimer.Start();
    if(ret->groupable != ATMI_TRUE) {
      atl_task_wait(ret);
    }
    else {
      ret->taskgroup_obj->sync();
    }
    set_task_metrics(ret);
    set_task_state(ret, ATMI_COMPLETED);
    //  TaskWaitTimer.Stop();
    //std::cout << "Task Wait Interim Timer " << TaskWaitTimer << std::endl;
    //std::cout << "Launch Time: " << TryLaunchTimer << std::endl;
  }
  return should_dispatch;
}

atl_task_t *atl_trycreate_task(atl_kernel_t *kernel) {
  atl_task_t *ret = get_new_task();
  ret->kernel = kernel;
  return ret;
}

void set_task_params(atl_task_t *task_obj,
    const atmi_lparm_t *lparm,
    unsigned int kernel_id,
    void **args) {
  TryLaunchInitTimer.Start();
  if(!task_obj) return;
  DEBUG_PRINT("GPU Place Info: %d, %d, %d : %lx\n", lparm->place.node_id, lparm->place.type, lparm->place.device_id, lparm->place.cu_mask);
  static bool is_called = false;
  if(!is_called) {
    set_thread_affinity(0);

    /* int policy;
       struct sched_param param;
       pthread_getschedparam(pthread_self(), &policy, &param);
       param.sched_priority = sched_get_priority_min(policy);
       printf("Setting Priority Policy for %d: %d\n", policy, param.sched_priority);
       pthread_setschedparam(pthread_self(), policy, &param);
       */
    is_called = true;
  }

  uint16_t i;
  atl_task_t *ret = task_obj;
  atl_task_t *continuation_task = NULL;
  bool has_continuation = false;
#if 0
  if(lparm->task) {
    has_continuation = (lparm->task->continuation != NULL);
    if(PublicTaskMap.find(lparm->task) != PublicTaskMap.end()) {
      // task is already found, so use it and dont create a new one
      ret = PublicTaskMap[lparm->task];
      DEBUG_PRINT("Task %p found\n", lparm->task);
    }
  }
  if(ret == NULL || ret->is_continuation == false) {
    /* create anyway if not part of a continuation? */
    ret = new atl_task_t;
    memset(ret, 0, sizeof(atl_task_t));
    ret->is_continuation = false;
    lock(&mutex_all_tasks_);
    AllTasks.push_back(ret);
    //AllTasks.push_back(atl_task_t());
    //AllTasks.emplace_back();
    //ret = AllTasks[AllTasks.size() - 1];
    atmi_task_handle_t new_id;
    new_id.node = 0;
    new_id.hi = 0;
    new_id.lo = AllTasks.size() - 1;
    ret->id = new_id;
    ret->and_successors.clear();
    ret->and_predecessors.clear();
    ret->predecessors.clear();
    //AllTasks.push_back(task);
    //atl_task_t *ret = task; //AllTasks[AllTasks.size() - 1];
    unlock(&mutex_all_tasks_);
    if(lparm->task) {
      PublicTaskMap[lparm->task] = ret;
      DEBUG_PRINT("Task Map[%p] = %p (%s)\n", lparm->task, PublicTaskMap[lparm->task], kernel_name);
    }
    pthread_mutex_init(&(ret->mutex), NULL);
  }
  if(has_continuation) {
    continuation_task = new atl_task_t;
    memset(continuation_task, 0, sizeof(atl_task_t));
    continuation_task->is_continuation = true;
    lock(&mutex_all_tasks_);
    AllTasks.push_back(continuation_task);
    atmi_task_handle_t new_id;
    new_id.node = 0;
    new_id.hi = 0;
    new_id.lo = AllTasks.size() - 1;
    continuation_task->id = new_id;
    ret->id.hi = AllTasks.size();
    unlock(&mutex_all_tasks_);
    PublicTaskMap[lparm->task->continuation] = continuation_task;
    DEBUG_PRINT("Continuation Map[%p] = %p (%s)\n", lparm->task->continuation, PublicTaskMap[lparm->task->continuation], kernel_name);
    pthread_mutex_init(&(continuation_task->mutex), NULL);
  }
#else
#endif
  atl_kernel_t *kernel = ret->kernel;
  ret->kernel_id = kernel_id;
  atl_kernel_impl_t *kernel_impl = get_kernel_impl(kernel, ret->kernel_id);
  ret->kernarg_region = NULL;
  ret->kernarg_region_size = kernel_impl->kernarg_segment_size;
  ret->devtype = kernel_impl->devtype;
  ret->profilable = lparm->profilable;
  ret->groupable = lparm->groupable;
  ret->atmi_task = lparm->task_info;

  // assign the memory scope
  ret->acquire_scope = lparm->acquire_scope;
  ret->release_scope = lparm->release_scope;

  // fill in from lparm
  for(int i = 0; i < 3 /* 3dims */; i++) {
    ret->gridDim[i] = lparm->gridDim[i];
    ret->groupDim[i] = lparm->groupDim[i];
  }
  //DEBUG_PRINT("Requires LHS: %p and RHS: %p\n", ret->lparm.requires, lparm->requires);
  //DEBUG_PRINT("Requires ThisTask: %p and ThisTask: %p\n", ret->lparm.task_info, lparm->task_info);

  /* Add row to taskgroup table for purposes of future synchronizations */
  ret->taskgroup = lparm->group;
  ret->taskgroup_obj = get_taskgroup_impl(ret->taskgroup);

  ret->place = lparm->place;
  ret->synchronous = lparm->synchronous;
  //DEBUG_PRINT("Taskgroup LHS: %p and RHS: %p\n", ret->lparm.group, lparm->group);
  ret->num_predecessors = 0;
  ret->num_successors = 0;

  /* For dependent child tasks, add dependent parent kernels to barriers.  */
  DEBUG_PRINT("Pif %s requires %d task\n", kernel_impl->kernel_name.c_str(), lparm->num_required);

  ret->predecessors.clear();
  //ret->predecessors.resize(lparm->num_required);
  for(int idx = 0; idx < lparm->num_required; idx++) {
    atl_task_t *pred_task = get_task(lparm->requires[idx]);
    //assert(pred_task != NULL);
    if(pred_task) {
      DEBUG_PRINT("Task %lu adding %lu task as predecessor\n", ret->id, pred_task->id);
      ret->predecessors.push_back(pred_task);
    }
  }
  ret->pred_taskgroup_objs.clear();
  ret->pred_taskgroup_objs.resize(lparm->num_required_groups);
  for(int idx = 0; idx < lparm->num_required_groups; idx++) {
    ret->pred_taskgroup_objs[idx] = get_taskgroup_impl(lparm->required_groups[idx]);
  }

  ret->type = ATL_KERNEL_EXECUTION;
  ret->data_src_ptr = NULL;
  ret->data_dest_ptr = NULL;
  ret->data_size = 0;

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
  TryLaunchInitTimer.Stop();
}

atmi_task_handle_t atl_trylaunch_kernel(const atmi_lparm_t *lparm,
    atl_task_t *task_obj,
    unsigned int kernel_id,
    void **args) {
  atmi_task_handle_t ret_handle = ATMI_NULL_TASK_HANDLE;
  atl_task_t *ret = task_obj;
  if(ret) {
    set_task_params(ret, lparm, kernel_id, args);
    if (g_dep_sync_type == ATL_SYNC_BARRIER_PKT) {
      std::set<pthread_mutex_t *> req_mutexes;
      req_mutexes.clear();

      req_mutexes.insert(&mutex_readyq_);
      req_mutexes.insert((pthread_mutex_t *)&(ret->mutex));
      lock_set(req_mutexes);
      // Save kernel args because it will be activated
      // later. This way, the user will be able to
      // reuse their kernarg region that was created
      // in the application space.
      if(ret->kernel && ret->kernarg_region == NULL) {
        // first time allocation/assignment
        ret->kernarg_region = malloc(ret->kernarg_region_size);
        //ret->kernarg_region_copied = true;
        set_kernarg_region(ret, args);
      }
      TaskList.push_back(ret);
      unlock_set(req_mutexes);
    }
    try_dispatch(ret, args, lparm->synchronous);
    ret_handle = ret->id;
  }
  return ret_handle;
}

atl_kernel_t *get_kernel_obj(atmi_kernel_t atmi_kernel) {
  uint64_t pif_id = atmi_kernel.handle;
  std::map<uint64_t, atl_kernel_t *>::iterator map_iter;
  map_iter = KernelImplMap.find(pif_id);
  if(map_iter == KernelImplMap.end()) {
    DEBUG_PRINT("ERROR: Kernel/PIF %lu not found\n", pif_id);
    return NULL;
  }
  atl_kernel_t *kernel = map_iter->second;
  return kernel;
}

atmi_task_handle_t Runtime::CreateTaskTemplate(atmi_kernel_t atmi_kernel) {
  atmi_task_handle_t ret = ATMI_NULL_TASK_HANDLE;
  atl_kernel_t *kernel = get_kernel_obj(atmi_kernel);
  if(kernel) {
    atl_task_t *ret_obj = atl_trycreate_task(kernel);
    if(ret_obj) ret = ret_obj->id;
  }
  return ret;
}

int get_kernel_id(atmi_lparm_t *lparm, atl_kernel_t *kernel) {
  int num_args = kernel->num_args;
  int kernel_id = lparm->kernel_id;
  if(kernel_id == -1) {
    // choose the first available kernel for the given devtype
    atmi_devtype_t type = lparm->place.type;
    for(std::vector<atl_kernel_impl_t *>::iterator it = kernel->impls.begin();
        it != kernel->impls.end(); it++) {
      if((*it)->devtype == type) {
        kernel_id = (*it)->kernel_id;
        break;
      }
    }
    if(kernel_id == -1) {
      fprintf(stderr, "ERROR: Kernel/PIF %lu doesn't have any implementations\n",
          kernel->pif_id);
      return -1;
    }
  }
  else {
    if(!is_valid_kernel_id(kernel, kernel_id)) {
      DEBUG_PRINT("ERROR: Kernel ID %d not found\n", kernel_id);
      return -1;
    }
  }
  atl_kernel_impl_t *kernel_impl = get_kernel_impl(kernel, kernel_id);
  if(kernel->num_args && kernel_impl->kernarg_region == NULL) {
    fprintf(stderr, "ERROR: Kernel Arguments not initialized for Kernel %s\n",
        kernel_impl->kernel_name.c_str());
    return -1;
  }

  return kernel_id;
}

atmi_task_handle_t Runtime::ActivateTaskTemplate(atmi_task_handle_t task, atmi_lparm_t *lparm,
    void **args) {
  atmi_task_handle_t ret = ATMI_NULL_TASK_HANDLE;
  atl_task_t *task_obj = get_task(task);
  if(!task_obj) return ret;

  /*if(lparm == NULL && args == NULL) {
    DEBUG_PRINT("Signaling the completed task\n");
    set_task_state(task_obj, ATMI_COMPLETED);
    return task;
    }*/

  atl_kernel_t *kernel = task_obj->kernel;
  int kernel_id = get_kernel_id(lparm, kernel);
  if(kernel_id == -1) return ret;

  ret = atl_trylaunch_kernel(lparm, task_obj, kernel_id, args);
  DEBUG_PRINT("[Returned Task: %lu]\n", ret);
  return ret;
}

atmi_task_handle_t Runtime::CreateTask(atmi_lparm_t *lparm,
    atmi_kernel_t atmi_kernel,
    void **args) {

  atmi_task_handle_t ret = ATMI_NULL_TASK_HANDLE;
  if((lparm->place.type & ATMI_DEVTYPE_GPU && !atlc.g_gpu_initialized) ||
      (lparm->place.type & ATMI_DEVTYPE_CPU && !atlc.g_cpu_initialized))
    return ret;
  atl_kernel_t *kernel = get_kernel_obj(atmi_kernel);
  if(kernel) {
    atl_task_t *ret_obj = atl_trycreate_task(kernel);
    if(ret_obj) {
      int kernel_id = get_kernel_id(lparm, kernel);
      if(kernel_id == -1) return ret;
      set_task_params(ret_obj, lparm, kernel_id, args);

      std::set<pthread_mutex_t *> req_mutexes;
      req_mutexes.clear();

      if(g_dep_sync_type == ATL_SYNC_BARRIER_PKT) // may need to add to readyQ
        req_mutexes.insert(&mutex_readyq_);

      req_mutexes.insert((pthread_mutex_t *)&(ret_obj->mutex));
      std::vector<atl_task_t *> &temp_vecs = ret_obj->predecessors;
      for(int idx = 0; idx < ret_obj->predecessors.size(); idx++) {
        atl_task_t *pred_task = ret_obj->predecessors[idx];
        req_mutexes.insert((pthread_mutex_t *)&(pred_task->mutex));
      }
#if 0
      //FIXME: do we care about locking for ordered and task group mutexes?
      if(ret_obj->prev_ordered_task)
        req_mutexes.insert(&(ret_obj->prev_ordered_task->mutex));
#endif
      TaskgroupImpl *taskgroup_obj = ret_obj->taskgroup_obj;
      req_mutexes.insert(&(taskgroup_obj->_group_mutex));
      lock_set(req_mutexes);
      // populate its predecessors' successor list
      // similar to a double linked list
      if(ret_obj->predecessors.size() > 0) {
        // add to its predecessor's dependents list and ret_objurn
        for(int idx = 0; idx < ret_obj->predecessors.size(); idx++) {
          atl_task_t *pred_task = ret_obj->predecessors[idx];
          DEBUG_PRINT("Task %p depends on %p as predecessor ",
              ret_obj, pred_task);
          if(pred_task->state /*.load(std::memory_order_seq_cst)*/ < ATMI_EXECUTED) {
            //should_try_dispatch = false;
            //predecessors_complete = false;
            pred_task->and_successors.push_back(ret_obj);
            ret_obj->num_predecessors++;
            DEBUG_PRINT("(waiting)\n");
            //waiting_count++;
          }
          else {
            DEBUG_PRINT("(completed)\n");
          }
        }
      }
      if(ret_obj->pred_taskgroup_objs.size() > 0) {
        // add to its predecessor's dependents list and ret_objurn
        DEBUG_PRINT("Task %lu has %lu predecessor task groups\n", ret_obj->id, ret_obj->pred_taskgroup_objs.size());
        for(int idx = 0; idx < ret_obj->pred_taskgroup_objs.size(); idx++) {
          TaskgroupImpl *pred_tg = ret_obj->pred_taskgroup_objs[idx];
          DEBUG_PRINT("Task %p depends on %p as predecessor task group ",
              ret_obj, pred_tg);
          if(pred_tg && pred_tg->_task_count.load() > 0) {
            // predecessor task group is still running, so add yourself to its successor list
            //should_try_dispatch = false;
            //predecessors_complete = false;
            pred_tg->_and_successors.push_back(ret_obj);
            ret_obj->num_predecessors++;
            DEBUG_PRINT("(waiting)\n");
          }
          else {
            DEBUG_PRINT("(completed)\n");
          }
        }
      }

      // Save kernel args because it will be activated
      // later. This way, the user will be able to
      // reuse their kernarg region that was created
      // in the application space.
      if(ret_obj->kernel && ret_obj->kernarg_region == NULL) {
        // first time allocation/assignment
        ret_obj->kernarg_region = malloc(ret_obj->kernarg_region_size);
        //ret->kernarg_region_copied = true;
        set_kernarg_region(ret_obj, args);
      }
      if (g_dep_sync_type == ATL_SYNC_BARRIER_PKT) {
        TaskList.push_back(ret_obj);
      }

      set_task_state(ret_obj, ATMI_INITIALIZED);

      unlock_set(req_mutexes);
      ret = ret_obj->id;
    }
  }
  return ret;
}

atmi_task_handle_t Runtime::ActivateTask(atmi_task_handle_t task) {
    atmi_task_handle_t ret = ATMI_NULL_TASK_HANDLE;
    atl_task_t *ret_obj = get_task(task);
    if(!ret_obj) return ret;
    ret = ret_obj->id;
    // On activate, tasks in TaskList will be dispatched
    if (g_dep_sync_type == ATL_SYNC_BARRIER_PKT) {
      bool should_dispatch = false;
      // Do we assume that it is a logical error if someone wants to
      // activate the task graph while at the same time trying to
      // add nodes to it? If yes, we can remove the locks surrounding TaskList.
      do {
        atl_task_t *ready_task = NULL;
        should_dispatch = false;
        lock(&mutex_readyq_);
        if (!TaskList.empty()) {
          ready_task = TaskList.front();
        }
        unlock(&mutex_readyq_);
        if (ready_task) {
          should_dispatch = try_dispatch(ready_task, NULL, ready_task->synchronous);
        }
      } while(should_dispatch);
    }
    else {
      if(ret_obj->taskgroup_obj && ret_obj->taskgroup_obj->_ordered) {
        // pop from front of ordered task list and try dispatch as many as possible
        bool should_dispatch = false;
        do {
          atl_task_t *ready_task = NULL;
          should_dispatch = false;
          lock(&ret_obj->taskgroup_obj->_group_mutex);
          if (!ret_obj->taskgroup_obj->_running_ordered_tasks.empty()) {
            ready_task = ret_obj->taskgroup_obj->_running_ordered_tasks.front();
          }
          unlock(&ret_obj->taskgroup_obj->_group_mutex);
          if (ready_task) {
            should_dispatch = try_dispatch(ready_task, NULL, ready_task->synchronous);
          }
        } while(should_dispatch);
      }
      // If the task has predecessors then you cannot activate this task. Task
      // activation is supported only for tasks without predecessors.
      else if(ret_obj->predecessors.size() <= 0)
        try_dispatch(ret_obj, NULL, ret_obj->synchronous);
      DEBUG_PRINT("[Returned Task: %lu]\n", ret);
    }
    return ret;
}

atmi_task_handle_t Runtime::LaunchTask(atmi_lparm_t *lparm, atmi_kernel_t atmi_kernel,
    void **args/*, more params for place info? */) {
  ParamsInitTimer.Start();
  atmi_task_handle_t ret = ATMI_NULL_TASK_HANDLE;
  if((lparm->place.type & ATMI_DEVTYPE_GPU && !atlc.g_gpu_initialized) ||
      (lparm->place.type & ATMI_DEVTYPE_CPU && !atlc.g_cpu_initialized))
    return ret;
  atl_kernel_t *kernel = get_kernel_obj(atmi_kernel);
  if(!kernel) return ret;

  int kernel_id = get_kernel_id(lparm, kernel);
  if(kernel_id == -1) return ret;
  /*lock(&(kernel_impl->mutex));
    if(kernel_impl->free_kernarg_segments.empty()) {
  // no free kernarg segments -- allocate some more?
  // FIXME: realloc instead? HSA realloc?
  }
  unlock(&(kernel_impl->mutex));
  */
  ParamsInitTimer.Stop();
  TryLaunchTimer.Start();
  atl_task_t *ret_obj = atl_trycreate_task(kernel);
  ret = atl_trylaunch_kernel(lparm, ret_obj, kernel_id, args);
  TryLaunchTimer.Stop();
  DEBUG_PRINT("[Returned Task: %lu]\n", ret);
  return ret;
}

} // namespace core
