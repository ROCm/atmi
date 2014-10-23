#include "SVMAtomicsBinaryTreeInsert_Host.hpp"

void svm_mutex_init(svm_mutex* lock, int value) {
    atomic_store_explicit(&lock->count, value, std::memory_order_release);
}

void svm_mutex_lock(svm_mutex* lock) {
  int expected = SVM_MUTEX_UNLOCK;
  while(!atomic_compare_exchange_strong_explicit(&lock->count, &expected, SVM_MUTEX_LOCK
					      ,   std::memory_order_seq_cst,std::memory_order_seq_cst)) {
    expected = SVM_MUTEX_UNLOCK;
  }
}

void svm_mutex_unlock(svm_mutex* lock) {
  atomic_store_explicit(&lock->count, SVM_MUTEX_UNLOCK, std::memory_order_release);
}

void initialize_nodes(node *data, size_t num_nodes, int seed)
{
	node *tmp_node;
	int val;

	srand(seed);
	for (size_t i = 0; i < num_nodes; i++)
	{
		tmp_node = &(data[i]);

		val = (((rand() & 255)<<8 | (rand() & 255))<<8 | (rand() & 255))<<7 | (rand() & 127);
				
		(tmp_node->value) = val;
		tmp_node->left = NULL;
		tmp_node->right = NULL;
		
		svm_mutex_init(&tmp_node->mutex_node, SVM_MUTEX_UNLOCK);
	}
}

node* cpuMakeBinaryTree(size_t numNodes, node* inroot)
{
  node* root = NULL;
  node* data;
  node* nextData;

  if (NULL != inroot)
  {
      /* allocate first node to root */
      data     = (node *)inroot;
      nextData = data;
      root     = nextData;

      /* iterative tree insert */
      for (size_t i = 1; i < numNodes; ++i)
      {
	  nextData = nextData + 1;

	  insertNode(nextData, &root);
      }
  }

  return root;
}

void insertNode(node* nextData, node** root)
{	
	node* nextNode     = *root;
	node* tmp_parent     = NULL;
	int key = nextData->value;
	int flag = 0;
	int done = 0;

	while (nextNode)
	{
		tmp_parent = nextNode;
		flag = (key - (nextNode->value));
		nextNode = (flag < 0) ? nextNode->left : nextNode->right;
	}
	
	node *child = nextNode;

	do
	{
		svm_mutex *parent_mutex = &tmp_parent->mutex_node;
		svm_mutex_lock(parent_mutex);

		child = (flag < 0) ? tmp_parent->left : tmp_parent->right;
		if(child)
		{
			tmp_parent = child;
			flag = (key - (child->value));
			child = (flag < 0) ? tmp_parent->left : tmp_parent->right;
		}
		else
		{
			tmp_parent->left = (flag < 0) ? nextData : tmp_parent->left ;
				
			tmp_parent->right = (flag >= 0) ? nextData : tmp_parent->right ;
			done = 1;
		}

		svm_mutex_unlock(parent_mutex);

	}while (!done);
}
