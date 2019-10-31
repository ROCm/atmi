/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/

#ifndef SRC_RUNTIME_INCLUDE_TASKGROUP_H_
#define SRC_RUNTIME_INCLUDE_TASKGROUP_H_

#include <hsa.h>

#include <deque>
#include <vector>

#include "ATLMachine.h"
#include "atl_internal.h"
#include "atmi.h"

namespace core {
class TaskgroupImpl {
 public:
  TaskgroupImpl(bool, atmi_place_t);
  ~TaskgroupImpl();
  void sync();

  template <typename ProcType>
  hsa_queue_t *chooseQueueFromPlace(atmi_place_t place) {
    hsa_queue_t *ret_queue = NULL;
    atmi_scheduler_t sched = ordered_ ? ATMI_SCHED_NONE : ATMI_SCHED_RR;
    ProcType &proc = get_processor<ProcType>(place);
    if (ordered_) {
      // Get the taskgroup's CPU or GPU queue depending on the task. If
      // taskgroup is
      // ordered, it will have just one GPU queue for its GPU tasks and just one
      // CPU
      // queue for its CPU tasks. If a taskgroup has interleaved CPU and GPU
      // tasks, then
      // a corresponding barrier packet or dependency edge will capture the
      // relationship
      // between the two queues.
      hsa_queue_t *generic_queue =
          (place.type == ATMI_DEVTYPE_GPU) ? gpu_queue_ : cpu_queue_;
      if (generic_queue == NULL) {
        generic_queue = proc.getQueueAt(id_);
        // put the chosen queue as the taskgroup's designated CPU or GPU queue
        if (place.type == ATMI_DEVTYPE_GPU)
          gpu_queue_ = generic_queue;
        else if (place.type == ATMI_DEVTYPE_CPU)
          cpu_queue_ = generic_queue;
      }
      ret_queue = generic_queue;
    } else {
      ret_queue = proc.getQueueAt(getBestQueueID(sched));
    }
    DEBUG_PRINT("Returned Queue: %p\n", ret_queue);
    return ret_queue;
  }

  hsa_signal_t signal() const { return group_signal_; }

 private:
  atmi_status_t clearSavedTasks();
  int getBestQueueID(atmi_scheduler_t sched);

 public:
  uint32_t id_;
  bool ordered_;
  atl_task_t *last_task_;
  hsa_queue_t *gpu_queue_;
  hsa_queue_t *cpu_queue_;
  atmi_devtype_t last_device_type_;
  int next_best_queue_id_;
  atmi_place_t place_;
  //    int next_gpu_qid;
  //    int next_cpu_qid;
  // dependent tasks for the entire task group
  atl_task_vector_t and_successors_;
  hsa_signal_t group_signal_;
  std::atomic<unsigned int> task_count_;
  pthread_mutex_t group_mutex_;
  std::deque<atl_task_t *> running_ordered_tasks_;
  std::vector<atl_task_t *> running_default_tasks_;
  std::vector<atl_task_t *> running_groupable_tasks_;
  // TODO(ashwinma): for now, all waiting tasks (groupable and individual) are
  // placed in a single queue. does it make sense to have groupable waiting
  // tasks separately waiting in their own queue? perhaps not for now.
  // Should revisit if there are more than one callback threads
  // std::vector<atl_task_t *> waiting_groupable_tasks;
  std::atomic_flag callback_started_;

  // int                maxsize;      /**< Number of tasks allowed in group */
  // atmi_full_policy_t full_policy;/**< What to do if maxsize reached */
};  // class TaskgroupImpl
}  // namespace core
#endif  // SRC_RUNTIME_INCLUDE_TASKGROUP_H_
