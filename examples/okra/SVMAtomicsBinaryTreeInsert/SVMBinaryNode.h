#ifndef __SVM_BINARY_NODE__
#define __SVM_BINARY_NODE__

#define SVM_MUTEX_LOCK    1
#define SVM_MUTEX_UNLOCK  0

#ifndef SVM_DATA_STRUCT_OPENCL_DEVICE

#include <atomic>
#define __global 

#endif

typedef struct {
#ifndef SVM_DATA_STRUCT_OPENCL_DEVICE
	std::atomic<int> count;
#else
	volatile int count;
#endif
} svm_mutex;

typedef struct bin_tree
{
	int value;                  // Value at a node
	__global struct bin_tree *left;      // Pointer to the left node
	__global struct bin_tree *right;     // Pointer to the right node
	svm_mutex mutex_node;
} node;


#endif //__SVM_BINARY_NODE__

