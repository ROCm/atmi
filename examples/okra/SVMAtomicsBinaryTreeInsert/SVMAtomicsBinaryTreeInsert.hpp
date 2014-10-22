/**********************************************************************
Copyright ©2014 Advanced Micro Devices, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

•   Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
•   Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
 other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
********************************************************************/


#ifndef _SVM_BINARY_TREE_H_
#define _SVM_BINARY_TREE_H_

#include "okra.h"
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <omp.h>

#include "SVMBinaryNode.h"

#define   NUMBER_OF_NODES         10 * 1024 * 1024
#define	  WORKGROUP_SIZE 	  256
#define   HOST_PERCENT		  10
#define  SDK_SUCCESS 0
#define  SDK_FAILURE 1

static void error(std::string errorMsg)
{
    std::cout<<"Error: "<<errorMsg<<std::endl;
}


#define CHECK_ALLOCATION(actual, msg) \
    if(actual == NULL) \
    { \
        error(msg); \
        std::cout << "Location : " << __FILE__ << ":" << __LINE__<< std::endl; \
        return SDK_FAILURE; \
    }

#define CHECK_ERROR(actual, reference, msg) \
    if(actual != reference) \
    { \
        error(msg); \
        std::cout << "Location : " << __FILE__ << ":" << __LINE__<< std::endl; \
        return SDK_FAILURE; \
    }


/**
 * SVM Binary Tree.
 * The class implements host and OpenCL versions of node insertion into a Binary Tree
 */

class SVMAtomicsBinaryTreeInsert
{
private:
  /* OKRA runtime */
  okra_context_t*       context;
  okra_kernel_t*        kernel;

  /* seed to random number generator */
  int                   localSeed;

  size_t				num_insert;
  size_t				numKeys;
  size_t				host_nodes;
  size_t				device_nodes;
  size_t				init_tree_insert;
  size_t				total_nodes ;

  int					hostCompPercent;

  bool					printTreeOrder;

  __global node			        *svmTreeBuf;
  node					*svmRoot;
  node					*currNode;

public:
  
  SVMAtomicsBinaryTreeInsert()
  {
    localSeed    = 123;
    hostCompPercent = HOST_PERCENT;
    printTreeOrder = false;

    num_insert = NUMBER_OF_NODES; //Num of nodes to insert
    init_tree_insert = 10;
    svmRoot = NULL;
  };
  
  ~SVMAtomicsBinaryTreeInsert()
  {
  };

  char *buildStringFromSourceFile(string fname);

  /**
   *************************************************************************
   * @fn setupSVMBinaryTree
   * @brief Allocates and initializes any host memory pointers.
   * @return SDK_SUCCESS on success and SDK_FAILURE on failure
   *************************************************************************
   */
  int     setupSVMBinaryTree();

  /**
   *************************************************************************
   * @fn     setupCL
   * @brief  Sets up OpenCL runtime including querying OCL divices, setting
   *         up context and command queues.
   * @return SDK_SUCCESS on success and SDK_FAILURE on failure
   *************************************************************************
   */
  int     setupCL();

  /**
   *************************************************************************
   * @fn     runCLKernels
   * @brief  Calls kernel functions for warm up run and then running them
   *         for number of iteration specified.Gets kernel start and end 
   *         time if timing is enabled.
   * @return SDK_SUCCESS on success and SDK_FAILURE on failure
   *************************************************************************
   */
  int     runCLKernels();

  /**
   *************************************************************************
   * @fn     svmBinaryTreeCPUReference
   * @brief  Executes an equivalent of OpenCL code on host device and 
   *         generates output used to compare with OpenCL code.
   *         
   * @return SDK_SUCCESS on success and SDK_FAILURE on failure
   *************************************************************************
   */
  int     svmBinaryTreeCPUReference();

  /**
   *************************************************************************
   * @fn     setup
   * @brief  Top level setup. Calls host side and device side setup 
   *         functions.
   *         
   * @return SDK_SUCCESS on success and SDK_FAILURE on failure
   *************************************************************************
   */
  int     setup();

  /**
   *************************************************************************
   * @fn     run
   * @brief  Top level function. Initializes data needed for kernels and
   *         calls functions executing kernels.
   *         
   * @return SDK_SUCCESS on success and SDK_FAILURE on failure
   *************************************************************************
   */
  int     run();

  /**
   *************************************************************************
   * @fn     verifyResults
   * @brief  Calls host reference code to generate host side output. 
   *         Compares this with OpenCL output.
   *         
   * @return SDK_SUCCESS on success and SDK_FAILURE on failure
   *************************************************************************
   */
  int     verifyResults();

  /**
   *************************************************************************
   * @fn     cleanup
   * @brief  Releases resources utilized by the program.
   *         
   * @return SDK_SUCCESS on success and SDK_FAILURE on failure.
   *************************************************************************
   */
  int     cleanup();

  /**
   *************************************************************************
   * @fn     cpuCreateBinaryTree
   * @brief  Given an array of nodes, creates a binary tree out of it.
   *         
   * @return SDK_SUCCESS on success and SDK_FAILURE on failure.
   *************************************************************************
   */
  int     cpuCreateBinaryTree();


  /**
   *************************************************************************
   * @fn     recursiveInOrder
   * @brief  Used by printInOrder for in-odrer traversal of binary tree.
   *         
   * @return SDK_SUCCESS on success and SDK_FAILURE on failure.
   *************************************************************************
   */
  int     recursiveInOrder(node* leaf);
 
  /** 
 *******************************************************************************
 *  @fn     count_nodes
 *  @brief  This function returns the number of nodes in the tree
 *           
 *  @param[node*] root : Root node to start the traverse
 *          
 *  @return size_t : Number of nodes in the tree
 *******************************************************************************
 */
  size_t count_nodes(node* root);

};
#endif
