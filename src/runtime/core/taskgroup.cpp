#include "taskgroup.h"
#include "RealTimerClass.h"
#include <cassert>
using namespace Global;
extern RealTimer TaskWaitTimer;

/* Taskgroup specific globals */
atmi_taskgroup_handle_t ATMI_DEFAULT_TASKGROUP_HANDLE = {0ull};

namespace core {

/* Taskgroup vector to hold the runtime state of the
 * taskgroup and its tasks. Which was the latest
 * device used, latest queue used and also a
 * pool of tasks for synchronization if need be */
std::vector<core::TaskgroupImpl *> AllTaskgroups;

TaskgroupImpl *get_taskgroup_impl(atmi_taskgroup_handle_t t) {
    TaskgroupImpl *taskgroup_obj = NULL;
    lock(&mutex_all_tasks_);
    if(t < AllTaskgroups.size()) {
      taskgroup_obj = AllTaskgroups[t];
    }
    unlock(&mutex_all_tasks_);
    return taskgroup_obj;
}

atmi_status_t Runtime::TaskGroupCreate(atmi_taskgroup_handle_t *group_handle,
                              bool ordered,
                              atmi_place_t place) {
  atmi_status_t status = ATMI_STATUS_ERROR;
  if(group_handle) {
    TaskgroupImpl *taskgroup_obj = new TaskgroupImpl(ordered, place);
    // add to global taskgroup vector
    lock(&mutex_all_tasks_);
    AllTaskgroups.push_back(taskgroup_obj);
    // the below assert is always true because we insert into the
    // vector but dont delete that slot upon release.
    assert((AllTaskgroups.size()-1)==taskgroup_obj->_id && "Taskgroup ID and vec size mismatch");
    *group_handle = taskgroup_obj->_id;
    unlock(&mutex_all_tasks_);
    status = ATMI_STATUS_SUCCESS;
  }
  return status;
}

atmi_status_t Runtime::TaskGroupRelease(atmi_taskgroup_handle_t group_handle) {
  atmi_status_t status = ATMI_STATUS_ERROR;
  TaskgroupImpl *taskgroup_obj = get_taskgroup_impl(group_handle);
  if(taskgroup_obj) {
    lock(&mutex_all_tasks_);
    delete taskgroup_obj;
    AllTaskgroups[group_handle] = NULL;
    unlock(&mutex_all_tasks_);
    status = ATMI_STATUS_SUCCESS;
  }
  return status;
}

atmi_status_t Runtime::TaskGroupSync(atmi_taskgroup_handle_t group_handle) {
  TaskgroupImpl *taskgroup_obj = get_taskgroup_impl(group_handle);
  TaskWaitTimer.Start();
  if(taskgroup_obj)
    taskgroup_obj->sync();
  else
    DEBUG_PRINT("Waiting for invalid task group signal!\n");
  TaskWaitTimer.Stop();
  return ATMI_STATUS_SUCCESS;
}

void TaskgroupImpl::sync() {
  DEBUG_PRINT("Waiting for %u group tasks to complete\n", _task_count.load());
  while(_task_count.load() != 0) {}
  if(_ordered == ATMI_TRUE) {
    atl_task_wait(_last_task);
    lock(&(_group_mutex));
    _last_task = NULL;
    unlock(&(_group_mutex));
  }
  clearSavedTasks();
}

#if 0
hsa_queue_t *acquire_and_set_next_cpu_queue(atl_taskgroup_t *taskgroup_obj, atmi_place_t place) {
    hsa_queue_t *queue = NULL;
    atmi_scheduler_t sched = _ordered ? ATMI_SCHED_NONE : ATMI_SCHED_RR;
    ATLCPUProcessor &proc = get_processor<ATLCPUProcessor>(place);
    if(_ordered) {
        if(_cpu_queue == NULL) {
            _cpu_queue = proc.getQueue(_group_queue_id);
        }
        queue = _cpu_queue;
    }
    else {
        queue = proc.getBestQueue(sched);
    }
    DEBUG_PRINT("Returned Queue: %p\n", queue);
    return queue;
}
#endif

atmi_status_t TaskgroupImpl::clearSavedTasks() {
    lock(&_group_mutex);
    _running_ordered_tasks.clear();
    _running_groupable_tasks.clear();
    unlock(&_group_mutex);
    return ATMI_STATUS_SUCCESS;
}

atmi_status_t TaskgroupImpl::registerTask(atl_task_t *task) {
    if(task->groupable)
        _running_groupable_tasks.push_back(task);
    //DEBUG_PRINT("Registering %s task %p Profilable? %s\n",
    //            (devtype == ATMI_DEVTYPE_GPU) ? "GPU" : "CPU",
    //            task, (profilable == ATMI_TRUE) ? "Yes" : "No");
    return ATMI_STATUS_SUCCESS;
}

/*
 * do not use the below because they will not work if we want to sort mutexes
atmi_status_t get_taskgroup_mutex(atl_taskgroup_t *taskgroup_obj, pthread_mutex_t *m) {
    *m = taskgroup_obj->group_mutex;
    return ATMI_STATUS_SUCCESS;
}

atmi_status_t set_taskgroup_mutex(atl_taskgroup_t *taskgroup_obj, pthread_mutex_t *m) {
    taskgroup_obj->group_mutex = *m;
    return ATMI_STATUS_SUCCESS;
}
*/

// constructor
core::TaskgroupImpl::TaskgroupImpl(bool ordered, atmi_place_t place) :
  _ordered(ordered),
  _place(place),
  _last_task(NULL),
  _cpu_queue(NULL),
  _gpu_queue(NULL)
{
  static unsigned int taskgroup_id = 0;
  _id = taskgroup_id++;

  _running_groupable_tasks.clear();
  _running_ordered_tasks.clear();
  _and_successors.clear();
  _task_count.store(0);
  _callback_started.clear();

  pthread_mutex_init(&(_group_mutex), NULL);

  // create the group signal with initial value 0; task dispatch is
  // then responsible for incrementing this value before ringing the
  // doorbell.
  hsa_status_t err = hsa_signal_create(0, 0, NULL, &_group_signal);
  ErrorCheck(Taskgroup signal creation, err);
}

// destructor
core::TaskgroupImpl::~TaskgroupImpl() {
  hsa_status_t err = hsa_signal_destroy(_group_signal);
  ErrorCheck(Taskgroup signal destruction, err);

  _running_groupable_tasks.clear();
  _running_ordered_tasks.clear();
  _and_successors.clear();
}

} //namespace core
