
#ifndef _SVM_DATA_STRUCT_H
#define _SVM_DATA_STRUCT_H

#define SVM_MUTEX_LOCK    1
#define SVM_MUTEX_UNLOCK  0

#ifndef SVM_DATA_STRUCT_OPENCL_DEVICE

// C++11 implementation of the mutex.
// It is compatible with the OpenCL implementation
#include <atomic>

typedef struct {
  std::atomic<int> count;
} svm_mutex;

void svm_mutex_init(svm_mutex* lock, int value) {
  lock->count.store(value, std::memory_order_release);
}

void svm_mutex_lock(svm_mutex* lock) {
  int expected = SVM_MUTEX_UNLOCK;
  while(!lock->count.compare_exchange_strong(expected, SVM_MUTEX_LOCK, std::memory_order_acquire)) {
    expected = SVM_MUTEX_UNLOCK;
  }
}

void svm_mutex_unlock(svm_mutex* lock) {
  lock->count.store(SVM_MUTEX_UNLOCK, std::memory_order_release);
}

#else /* SVM_DATA_STRUCT_OPENCL_DEVICE */

// OpenCL implementation of the mutex.
// It is compatible with the OpenCL implementation

typedef struct {
    // atomic_int count;
    volatile int count;
} svm_mutex;

void svm_mutex_init(__global svm_mutex* lock, int value) {
    // atomic_store_explicit(&lock->count, value, memory_order_release, memory_scope_all_svm_devices);
    atomic_store_explicit((atomic_int *)&lock->count, value, memory_order_release);
}

void svm_mutex_lock(__global svm_mutex* lock) {
  int expected = SVM_MUTEX_UNLOCK;
  while(!atomic_compare_exchange_strong_explicit((atomic_int *)&lock->count, &expected, SVM_MUTEX_LOCK
					      , memory_order_acquire, memory_order_release, memory_scope_all_svm_devices)) {
    expected = SVM_MUTEX_UNLOCK;
  }
}

void svm_mutex_unlock(__global svm_mutex* lock) {
  atomic_store_explicit((atomic_int *)&lock->count, SVM_MUTEX_UNLOCK, memory_order_release, memory_scope_all_svm_devices);
}

#endif /* SVM_DATA_STRUCT_OPENCL_DEVICE */

#endif // _SVM_DATA_STRUCT_H
