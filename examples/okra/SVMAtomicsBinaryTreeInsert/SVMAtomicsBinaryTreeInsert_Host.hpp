#ifndef __SVM_BINARY_NODE_HOST__H
#define __SVM_BINARY_NODE_HOST__H

#include <stdlib.h>
#include <stdio.h>
#include "SVMBinaryNode.h"

void initialize_nodes(node *data, size_t num_nodes, int seed);
void insertNode(node* nextData, node** root);
node* cpuMakeBinaryTree(size_t numNodes, node* inroot);

#endif
