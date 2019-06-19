/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/

#ifndef __ATMI_TASKGROUP_H
#define __ATMI_TASKGROUP_H

#include "atmi.h"
#include <hsa.h>
#include "atl_internal.h"
#include "ATLMachine.h"

namespace core {
  class TaskgroupImpl {
    public:
      TaskgroupImpl(bool, atmi_place_t);
      ~TaskgroupImpl();
      void sync();
      atmi_status_t registerTask(atl_task_t *task);
      atmi_status_t clearSavedTasks();

      template<typename ProcType>
      hsa_queue_t *chooseQueueFromPlace(atmi_place_t place) {
        hsa_queue_t *ret_queue = NULL;
        hsa_queue_t *_queue = NULL;
        atmi_scheduler_t sched = _ordered ? ATMI_SCHED_NONE : ATMI_SCHED_RR;
        ProcType &proc = get_processor<ProcType>(place);
        if(place.type == ATMI_DEVTYPE_GPU)
          _queue = _gpu_queue;
        else if(place.type == ATMI_DEVTYPE_CPU)
          _queue = _cpu_queue;
        if(_ordered) {
          if(_queue == NULL) {
            _queue = proc.getQueue(_id);
          }
          ret_queue = _queue;
        }
        else {
          ret_queue = proc.getBestQueue(sched);
        }
        DEBUG_PRINT("Returned Queue: %p\n", ret_queue);
        return ret_queue;
      }

      hsa_signal_t getSignal() const { return _group_signal; }

    public:
      uint32_t _id;
      bool _ordered;
      atl_task_t* _last_task;
      hsa_queue_t* _gpu_queue;
      hsa_queue_t* _cpu_queue;
      atmi_devtype_t _last_device_type;
      atmi_place_t _place;
      //    int next_gpu_qid;
      //    int next_cpu_qid;
      // dependent tasks for the entire task group
      atl_task_vector_t _and_successors;
      hsa_signal_t _group_signal;
      std::atomic<unsigned int> _task_count;
      pthread_mutex_t _group_mutex;
      std::deque<atl_task_t *> _running_ordered_tasks;
      std::vector<atl_task_t *> _running_groupable_tasks;
      // TODO: for now, all waiting tasks (groupable and individual) are placed in a
      // single queue. does it make sense to have groupable waiting tasks separately
      // waiting in their own queue? perhaps not for now. should revisit if there
      // are more than one callback threads
      // std::vector<atl_task_t *> waiting_groupable_tasks;
      std::atomic_flag _callback_started;

      //int                maxsize;      /**< Number of tasks allowed in group       */
      //atmi_full_policy_t full_policy;/**< What to do if maxsize reached          */
  }; // class TaskgroupImpl
} // namespace core
#endif //__ATMI_TASKGROUP_H
