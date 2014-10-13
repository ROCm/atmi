/**********************************************************************
Copyright �2014 Advanced Micro Devices, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

�   Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
�   Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
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
#include  <iostream>
#include  <fstream>
#include <ostream>
#include <assert.h>
#include <string>
#include <time.h>
#include <cstdlib>
#include "SVMBinaryNode.h"

#define   NUMBER_OF_NODES         1 * 1024 * 1024
#define   SEARCH_KEY_NODE_RATIO   (0.25)
#define   DEFAULT_LOCAL_SIZE      256

#define  SDK_SUCCESS 0
#define  SDK_FAILURE 1

#define WORKGROUP_SIZE 256

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
 * The class implements
 */

class SVMBinaryTreeSearch
{
private:
  /* OpenCL runtime */
  okra_context_t*       context;      
  okra_kernel_t*        kernel;
  
  /* Timing information */
  double             setupTime;   
  double             kernelTime;      
  
  /* kernel iterations for exact kernel timing measurement */
  int                   iterations;      

  /* maximum limit to random numbers */
  int                   localRandMax;

  /* seed to random number generator */
  int                   localSeed;

  /* number of nodes in the tree */
  long                   numNodes;

  /* number of nodes in the tree */
  int                   numKeys;

  /* root node of the binary tree */
  node*                 svmRoot;

  /* svm buffer for binary tree */
  void*                 svmTreeBuf;

  /* svm buffer for search keys */
  void*                 svmSearchBuf;

public:
  
  SVMBinaryTreeSearch()
  {
    numNodes     = NUMBER_OF_NODES;
    numKeys      = 0;
    svmTreeBuf   = NULL;
    svmSearchBuf = NULL;
    svmRoot      = NULL;
    
    iterations   = 1;
    localRandMax = RAND_MAX;
    localSeed    = 0;
  };
  
  ~SVMBinaryTreeSearch()
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
   * @fn     genBinaryImage
   * @brief  generates binary image of the .cl source.
   *         
   * @return SDK_SUCCESS on success and SDK_FAILURE on failure
   *************************************************************************
   */
  int     genBinaryImage();

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
   * @fn     runSampleKernel
   * @brief  Sets up kernel arguments and enqueues them. Waits till all
   *         kernels finish execution.Called by runCLKernels. 
   * @return SDK_SUCCESS on success and SDK_FAILURE on failure
   *************************************************************************
   */
  int     runSampleKernel();

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
   * @fn     initialize
   * @brief  Top level initialization. Sets up any new command line options.
   *         
   * @return SDK_SUCCESS on success and SDK_FAILURE on failure
   *************************************************************************
   */
  int     initialize();

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
   * @fn     lrand
   * @brief  A local rand function to generate node and key values within 
   *         given limit. 
   *         
   * @return generated random number.
   *************************************************************************
   */
  int     lrand();

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
   * @fn     cpuMakeBinaryTree
   * @brief  Called by cpuCreateBinaryTree. Helps in making binary tree.
   *         
   * @return root node of created binary tree.
   *************************************************************************
   */
  node*   cpuMakeBinaryTree();

  /**
   *************************************************************************
   * @fn     cpuInitNodes
   * @brief  Called by cpuCreateBinaryTree. Initializes nodes with random
   *         values.
   *         
   * @return SDK_SUCCESS on success and SDK_FAILURE on failure.
   *************************************************************************
   */
  int     cpuInitNodes();

  /**
   *************************************************************************
   * @fn     cpuInitSearchKeys
   * @brief  Initializes an array of serach keys. These keys are searched
   *         in the binary tree.
   *         
   * @return SDK_SUCCESS on success and SDK_FAILURE on failure.
   *************************************************************************
   */
  int     cpuInitSearchKeys();

  /**
   *************************************************************************
   * @fn     compare
   * @brief  compares host side and OpenCL output and establishes 
   *         correctness of the sample.
   *         
   * @return SDK_SUCCESS on success and SDK_FAILURE on failure.
   *************************************************************************
   */
  int     compare();

  /**
   *************************************************************************
   * @fn     printInOrder
   * @brief  A utility function to print binary tree with  in-order
   *         traversal.
   *         
   * @return SDK_SUCCESS on success and SDK_FAILURE on failure.
   *************************************************************************
   */
  int     printInOrder();

  /**
   *************************************************************************
   * @fn     recursiveInOrder
   * @brief  Used by printInOrder for in-odrer traversal of binary tree.
   *         
   * @return SDK_SUCCESS on success and SDK_FAILURE on failure.
   *************************************************************************
   */
  int     recursiveInOrder(node* leaf);

};
#endif
