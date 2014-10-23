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


using namespace std;
#include "SVMAtomicsBinaryTreeInsert.hpp"
#include "SVMAtomicsBinaryTreeInsert_Host.hpp"

char *SVMAtomicsBinaryTreeInsert::buildStringFromSourceFile(string fname) {
        cout << "using source from " << fname << endl;
        ifstream infile;
        infile.open(fname.c_str());
        if (!infile) {cout << "could not open " << fname << endl; exit(1);}
        infile.seekg(0, ios::end);
        int len = infile.tellg();
        char *str = new char[len+1];
        infile.seekg(0, ios::beg);
        infile.read(str, len);
        int lenRead = infile.gcount();
        // if (!infile) {cout << "could not read " << len << " bytes from " << fname << " but read " << lenRead << endl;}
        str[lenRead] = (char)0;  // terminate
        // cout << "Source String -----------\n" << str << "----------- end of Source String\n";
        return str;
};

int SVMAtomicsBinaryTreeInsert::setupSVMBinaryTree()
{
  //Ensure that there is atleast 1 node to start with
  if (init_tree_insert < 1)
	init_tree_insert = 1;

  if (num_insert > 125000000)
	num_insert = 125000000;

  //Num of nodes to insert on host and device
  host_nodes = (size_t)((double)num_insert * ((float)hostCompPercent / 100));
  device_nodes = num_insert - host_nodes;

  total_nodes = num_insert + init_tree_insert;

  return SDK_SUCCESS;
}

int SVMAtomicsBinaryTreeInsert::setupCL(void)
{

  string sourceFileName = "SVMAtomicsBinaryTreeInsert.hsail";
  char* svmTreeInsertSource = buildStringFromSourceFile(sourceFileName);

  okra_status_t status;

  //create okra context
  context = NULL;

  status = okra_get_context(&context);

  if (status != OKRA_SUCCESS) {cout << "Error while creating context:" << (int)status << endl; exit(-1);}

  //create kernel from hsail
  kernel = NULL;

  status = okra_create_kernel(context, svmTreeInsertSource, "&__OpenCL_binTreeInsert_kernel", &kernel);

  if (status != OKRA_SUCCESS) {cout << "Error while creating kernel:" << (int)status << endl; exit(-1);}

  // initialize any device/SVM memory here.
  svmTreeBuf = (node *) malloc( total_nodes*sizeof(node) );

  if(NULL == svmTreeBuf) {
        cout << " Malloc (svmTreeBuf) failed\n";
        exit (-1);
        }

  return SDK_SUCCESS;
}

int SVMAtomicsBinaryTreeInsert::runCLKernels(void)
{

    if (host_nodes > 0)
    {
#pragma omp parallel for
	for (long k = 0; k < host_nodes; k++)
    	{
  	    insertNode(&(currNode[(size_t)k]), &svmRoot);
	}
    }

    if (device_nodes > 0) 
    {

        size_t localThreads  = WORKGROUP_SIZE; // 256
        size_t globalThreads = device_nodes;
        size_t deviceStartNode = init_tree_insert + host_nodes;

        //setup kernel arguments
        okra_clear_args(kernel);

#ifdef DUMMY_ARGS
        //This flags should be set if HSA_HLC_Stable is used
        // This is because the high level compiler generates 6 extra args
        okra_push_pointer(kernel, NULL);
        okra_push_pointer(kernel, NULL);
        okra_push_pointer(kernel, NULL);
        okra_push_pointer(kernel, NULL);
        okra_push_pointer(kernel, NULL);
        okra_push_pointer(kernel, NULL);
#endif
        okra_push_pointer(kernel, (void *)svmTreeBuf);
        okra_push_pointer(kernel, (void *)(svmTreeBuf+deviceStartNode));
        okra_push_pointer(kernel, (void *)&device_nodes);
        cout << "Setting kernel args done device_nodes = " << globalThreads << " deviceStartNode = " << deviceStartNode << "\n";

        //setup execution range
        okra_range_t range;
        range.dimension=1;
        range.global_size[0] = globalThreads;
        range.global_size[1] = range.global_size[2] = 1;
        range.group_size[0] = localThreads;
        range.group_size[1] = range.group_size[2] = 1;

        //execute kernel and wait for completion
        okra_status_t status = okra_execute_kernel(context, kernel, &range);
        if(status != OKRA_SUCCESS) {cout << "Error while executing kernel:" << (int)status << endl; exit(-1);}
    }

    return SDK_SUCCESS;
}

int SVMAtomicsBinaryTreeInsert::setup()
{
  if(setupSVMBinaryTree() != SDK_SUCCESS)
  {
     return SDK_FAILURE;
  }
  
  if (setupCL() != SDK_SUCCESS)
  {
     return SDK_FAILURE;
  }
  
  return SDK_SUCCESS;
}

int SVMAtomicsBinaryTreeInsert::run()
{
    int status = 0;
	
    //create the initial binary tree with init_tree_insert nodes
    status = cpuCreateBinaryTree();
    CHECK_ERROR(status, SDK_SUCCESS, "cpuCreateBinaryTree() failed.");
	
    //Advance the current node after initial insert
    currNode = svmRoot + init_tree_insert;

    cout << "--------------------------------------------------";
    cout << "-----------------------" << endl;
    cout << "Inserting " << num_insert << " nodes in  a Binary Tree having ";
    cout << init_tree_insert << " Nodes..." << endl;
 
    cout << "--------------------------------------------------";
    cout << "-----------------------" << endl;

    // Arguments are set and execution call is enqueued on command buffer
    if(runCLKernels() != SDK_SUCCESS)
    {
	return SDK_FAILURE;
    }
    
    cout << "Nodes inserted on host   = " << host_nodes << endl;
    cout << "Nodes inserted on device = " << device_nodes << endl;

    if (printTreeOrder)
	recursiveInOrder(svmRoot);

    return SDK_SUCCESS;
}

size_t SVMAtomicsBinaryTreeInsert::count_nodes(node* root)
{
    size_t count = 0;
    if (root)
	count = 1;

    if (root->left)
	count += count_nodes(root->left);

    if (root->right)
	count += count_nodes(root->right);

    return count;
}

int SVMAtomicsBinaryTreeInsert::verifyResults()
{
  int status = SDK_SUCCESS;
  size_t actualNodes = count_nodes(svmTreeBuf);
  cout << "Actual Nodes (including the initial nodes) = " << actualNodes << " total_nodes = " << total_nodes << endl;

  if (actualNodes == total_nodes)      
  {
      cout << "Passed!\n" << endl;
  }
  else
  {
      cout << "Failed\n" << endl;
  }
  return status;
}

int SVMAtomicsBinaryTreeInsert::cleanup()
{
    free(svmTreeBuf);

    okra_dispose_kernel(kernel);
    okra_dispose_context(context);

    return SDK_SUCCESS;
}

int SVMAtomicsBinaryTreeInsert::cpuCreateBinaryTree()
{
  node*    root;

  //Initialize the node elements
  initialize_nodes(svmTreeBuf, total_nodes, localSeed);

  //Make tree with given initial nodes - init_tree_insert
  root   = cpuMakeBinaryTree(init_tree_insert, svmTreeBuf);
  
  /* set the root */
  svmRoot = root;

  return SDK_SUCCESS;
}

int SVMAtomicsBinaryTreeInsert::recursiveInOrder(node* leaf)
{
  if(NULL != leaf)
    {
      recursiveInOrder(leaf->left);
      cout << leaf->value << ", ";
      recursiveInOrder(leaf->right);
    }

  return SDK_SUCCESS;
}

int main(int argc, char * argv[])
{
    SVMAtomicsBinaryTreeInsert clSVMBinaryTree;

    // Setup
    if(clSVMBinaryTree.setup() != SDK_SUCCESS)
    {
        return SDK_FAILURE;
    }

    // Run
    if(clSVMBinaryTree.run() != SDK_SUCCESS)
    {
        return SDK_FAILURE;
    }

    // VerifyResults
    if(clSVMBinaryTree.verifyResults() != SDK_SUCCESS)
    {
        return SDK_FAILURE;
    }

    // Cleanup
    if (clSVMBinaryTree.cleanup() != SDK_SUCCESS)
    {
        return SDK_FAILURE;
    }

    return SDK_SUCCESS;
}
