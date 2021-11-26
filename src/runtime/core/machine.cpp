/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/
#include "machine.h"
#include <hsa.h>
#include <hsa_ext_amd.h>
#include <stdio.h>
#include <stdlib.h>
#include <cassert>
#include <vector>
#include "atmi_runtime.h"
#include "internal.h"
extern ATLMachine g_atl_machine;
extern hsa_region_t atl_cpu_kernarg_region;

void *ATLMemory::alloc(size_t sz) {
  void *ret;
  hsa_status_t err = hsa_amd_memory_pool_allocate(memory_pool_, sz, 0, &ret);
  ErrorCheck(Allocate from memory pool, err);
  return ret;
}

void ATLMemory::free(void *ptr) {
  hsa_status_t err = hsa_amd_memory_pool_free(ptr);
  ErrorCheck(Allocate from memory pool, err);
}

/*atmi_task_handle_t ATLMemory::copy(void *dest, void *ATLMemory &m, bool async)
{
    atmi_task_handle_t ret_task;

    if(async) atmi_task_wait(ret_task);
    return ret_task;
}*/

void ATLProcessor::addMemory(const ATLMemory &mem) {
  for (auto &mem_obj : memories_) {
    // if the memory already exists, then just return
    if (mem.memory().handle == mem_obj.memory().handle) return;
  }
  memories_.push_back(mem);
}

const std::vector<ATLMemory> &ATLProcessor::memories() const {
  return memories_;
}

template <>
std::vector<ATLCPUProcessor> &ATLMachine::processors() {
  return cpu_processors_;
}

template <>
std::vector<ATLGPUProcessor> &ATLMachine::processors() {
  return gpu_processors_;
}

template <>
std::vector<ATLDSPProcessor> &ATLMachine::processors() {
  return dsp_processors_;
}

hsa_amd_memory_pool_t get_memory_pool(const ATLProcessor &proc,
                                      const int mem_id) {
  hsa_amd_memory_pool_t pool;
  const std::vector<ATLMemory> &mems = proc.memories();
  assert(mems.size() && mem_id >= 0 && mem_id < mems.size() &&
         "Invalid memory pools for this processor");
  pool = mems[mem_id].memory();
  return pool;
}

template <>
void ATLMachine::addProcessor(const ATLCPUProcessor &p) {
  cpu_processors_.push_back(p);
}

template <>
void ATLMachine::addProcessor(const ATLGPUProcessor &p) {
  gpu_processors_.push_back(p);
}

template <>
void ATLMachine::addProcessor(const ATLDSPProcessor &p) {
  dsp_processors_.push_back(p);
}

int cu_mask_parser(char *gpu_workers, uint64_t *cu_masks, int count) {
  int cu_mask_enable = 0;

  if (gpu_workers) {
    char *pch, *token;

    // skip num_of_workers
    token = strtok_r(gpu_workers, ":", &pch);
    // printf("num_queues: %s\n", token);

    int qid = 0;
    token = strtok_r(NULL, ";", &pch);

    // parse each queue
    while (token != NULL && qid < count) {
      // printf("qid: %d %s\n", qid, pch);
      char *pch2, *token2;
      cu_mask_enable = 1;
      token2 = strtok_r(token, ",", &pch2);
      // fprintf(stderr, "qid: %d cu:", qid);
      while (token2 != NULL) {
        char *pch3, *token3;
        token3 = strtok_r(token2, "-", &pch3);
        int offset = atoi(token3);
        token3 = strtok_r(NULL, "-", &pch3);
        int num_cus = token3 ? atoi(token3) - offset + 1 : 1;
        token2 = strtok_r(NULL, ",", &pch2);

        // fprintf(stderr, "%d-%d ", offset, num_cus);

        for (int i = 0; i < num_cus; i++) {
          cu_masks[qid] |= (uint64_t)1 << offset;
          offset++;
        }
      }

      // fprintf(stderr, "mask: %lx\n", cu_masks[qid]);

      token = strtok_r(NULL, ";", &pch);
      qid++;
    }
  }

  return cu_mask_enable;
}

void callbackQueue(hsa_status_t status, hsa_queue_t *source, void *data) {
  if (status != HSA_STATUS_SUCCESS) {
    fprintf(stderr, "[%s:%d] GPU error in queue %p %d\n", __FILE__, __LINE__,
            source, status);
    abort();
  }
}

void ATLGPUProcessor::createQueues(const int count) {
  char *gpu_workers = getenv("ATMI_DEVICE_GPU_WORKERS");

  int *num_cus = reinterpret_cast<int *>(calloc(count, sizeof(int)));
  uint64_t *cu_masks =
      reinterpret_cast<uint64_t *>(calloc(count, sizeof(uint64_t)));

  int cu_mask_enable = 0;

  if (gpu_workers)
    cu_mask_enable = cu_mask_parser(gpu_workers, cu_masks, count);

  hsa_status_t err;
  /* Query the maximum size of the queue.  */
  uint32_t queue_size = 0;
  err = hsa_agent_get_info(agent_, HSA_AGENT_INFO_QUEUE_MAX_SIZE, &queue_size);
  ErrorCheck(Querying the agent maximum queue size, err);
  if (queue_size > core::Runtime::getInstance().getMaxQueueSize()) {
    queue_size = core::Runtime::getInstance().getMaxQueueSize();
  }
  /* printf("The maximum queue size is %u.\n", (unsigned int) queue_size);  */

  /* Create queues for each device. */
  int qid;
  for (qid = 0; qid < count; qid++) {
    hsa_queue_t *this_Q;
    err =
        hsa_queue_create(agent_, queue_size, HSA_QUEUE_TYPE_MULTI,
                         callbackQueue, NULL, UINT32_MAX, UINT32_MAX, &this_Q);
    ErrorCheck(Creating the queue, err);
    err = hsa_amd_profiling_set_profiler_enabled(this_Q, 1);
    ErrorCheck(Enabling profiling support, err);

    if (cu_mask_enable) {
      if (!cu_masks[qid]) {
        cu_masks[qid] = -1;
        fprintf(stderr, "Warning: queue[%d]: cu mask is 0x0\n", qid);
      }

      uint32_t *this_cu_mask_v = reinterpret_cast<uint32_t *>(&cu_masks[qid]);
      hsa_status_t ret = hsa_amd_queue_cu_set_mask(this_Q, 64, this_cu_mask_v);

      if (ret != HSA_STATUS_SUCCESS)
        fprintf(stderr, "Error: hsa_amd_queue_cu_set_mask\n");
    }

    queues_.push_back(this_Q);

    DEBUG_PRINT("Queue[%d]: %p\n", qid, this_Q);
  }

  free(cu_masks);
  free(num_cus);
}

thread_agent_t *ATLCPUProcessor::getThreadAgentAt(const int index) {
  if (index < 0 || index >= thread_agents_.size())
    DEBUG_PRINT("CPU Agent index out of bounds!\n");
  return thread_agents_[index];
}

hsa_signal_t *ATLCPUProcessor::get_worker_sig(hsa_queue_t *q) {
  hsa_signal_t *ret = NULL;
  for (auto agent : thread_agents_) {
    if (agent->queue == q) {
      ret = &(agent->worker_sig);
      break;
    }
  }
  return ret;
}

void *agent_worker(void *agent_args);

void ATLCPUProcessor::createQueues(const int count) {
  hsa_status_t err;
  for (int qid = 0; qid < count; qid++) {
    thread_agent_t *agent = new thread_agent_t;
    agent->id = qid;
    // signal between the host thread and the CPU tasking queue thread
    err = hsa_signal_create(IDLE, 0, NULL, &(agent->worker_sig));
        ErrorCheck(Creating a HSA signal for agent dispatch worker threads,
            err);

        hsa_signal_t db_signal;
        err = hsa_signal_create(1, 0, NULL, &db_signal);
        ErrorCheck(Creating a HSA signal for agent dispatch db signal, err);

        hsa_queue_t *this_Q;
        const int capacity = core::Runtime::getInstance().getMaxQueueSize();
        hsa_amd_memory_pool_t cpu_pool = get_memory_pool(*this, 0);
        // FIXME: How to convert hsa_amd_memory_pool_t to hsa_region_t?
        // Using fine grained system memory REGION for now
        err = hsa_soft_queue_create(
            atl_cpu_kernarg_region, capacity, HSA_QUEUE_TYPE_SINGLE,
            HSA_QUEUE_FEATURE_AGENT_DISPATCH, db_signal, &this_Q);
        ErrorCheck(Creating an agent queue, err);
        queues_.push_back(this_Q);
        agent->queue = this_Q;

        hsa_queue_t *q = this_Q;
        // err = hsa_ext_set_profiling( q, 1);
        // check(Enabling CPU profiling support, err);
        // profiling does not work for CPU queues
        /* FIXME: Looks like a HSA bug. The doorbell signal that we pass to the
         * soft queue creation API never seems to be set. Workaround is to
         * manually set it again like below.
         */
        q->doorbell_signal = db_signal;
        thread_agents_.push_back(agent);
        int last_index = thread_agents_.size() - 1;
        pthread_create(&(agent->thread), NULL, agent_worker,
                       reinterpret_cast<void *>(agent));
  }
}

void ATLDSPProcessor::createQueues(const int count) {}

void ATLProcessor::destroyQueues() {
  for (auto queue : queues_) {
    hsa_status_t err = hsa_queue_destroy(queue);
    ErrorCheck(Destroying the queue, err);
  }
}

hsa_queue_t *ATLProcessor::getBestQueue(atmi_scheduler_t sched) {
  hsa_queue_t *ret = NULL;
  switch (sched) {
    case ATMI_SCHED_NONE:
      ret = getQueueAt(__atomic_load_n(&next_best_queue_id_, __ATOMIC_ACQUIRE) %
                       queues_.size());
      break;
    case ATMI_SCHED_RR:
      ret = getQueueAt(
          __atomic_fetch_add(&next_best_queue_id_, 1, __ATOMIC_ACQ_REL) %
          queues_.size());
      break;
  }
  return ret;
}

hsa_queue_t *ATLProcessor::getQueueAt(const int index) {
  return queues_[index % queues_.size()];
}

int ATLProcessor::num_cus() const {
  hsa_status_t err;
  /* Query the number of compute units.  */
  uint32_t num_cus = 0;
  err = hsa_agent_get_info(
      agent_, (hsa_agent_info_t)HSA_AMD_AGENT_INFO_COMPUTE_UNIT_COUNT,
      &num_cus);
  ErrorCheck(Querying the agent number of compute units, err);

  return num_cus;
}

int ATLProcessor::wavefront_size() const {
  hsa_status_t err;
  /* Query the number of compute units.  */
  uint32_t w_size = 0;
  err = hsa_agent_get_info(
      agent_, (hsa_agent_info_t)HSA_AGENT_INFO_WAVEFRONT_SIZE, &w_size);
  ErrorCheck(Querying the agent wavefront size, err);

  return w_size;
}
