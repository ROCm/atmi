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
#include "SVMBinaryTreeSearch.hpp"

char *SVMBinaryTreeSearch::buildStringFromSourceFile(string fname) {
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

int SVMBinaryTreeSearch::setupSVMBinaryTree()
{

  /* setup number of keys */
  if(numKeys == 0)
    numKeys = numNodes*SEARCH_KEY_NODE_RATIO;

  /* if localRandMax GT RAND_MAX set it to RAND_MAX */
  if(localRandMax > RAND_MAX)
    localRandMax = RAND_MAX;

  /* initialize random number generator */
  if(localSeed == 0)
    srand(time(NULL));
  else
    srand(localSeed);


  return SDK_SUCCESS;
}

int SVMBinaryTreeSearch::setupCL(void)
{

  string sourceFileName = "SVMBinaryTreeSearch.hsail";
  char* svmTreeSource = buildStringFromSourceFile(sourceFileName);

  okra_status_t status;

  //create okra context
  context = NULL;

  status = okra_get_context(&context);

  if (status != OKRA_SUCCESS) {cout << "Error while creating context:" << (int)status << endl; exit(-1);}

  //create kernel from hsail
  kernel = NULL;

  status = okra_create_kernel(context, svmTreeSource, "&__OpenCL_btree_search_kernel", &kernel);

  if (status != OKRA_SUCCESS) {cout << "Error while creating kernel:" << (int)status << endl; exit(-1);}

  // initialize any device/SVM memory here.
  svmTreeBuf = malloc( numNodes*sizeof(node) );
  
  if(NULL == svmTreeBuf) {
	cout << " Malloc (svmTreeBuf) failed\n";
	exit (-1);
	}

  svmSearchBuf = malloc( numKeys*sizeof(searchKey));

  if(NULL == svmSearchBuf) {
	cout << " Malloc (SearchBuf) failed\n";
	exit (-1);
	}

  return SDK_SUCCESS;
}

int SVMBinaryTreeSearch::runCLKernels(void)
{
    int status;

    /* run global kernels for stage decided by input length */
    status = runSampleKernel();
    CHECK_ERROR(status, SDK_SUCCESS, "runSampleKernel() failed.");

    return SDK_SUCCESS;
}

int SVMBinaryTreeSearch::runSampleKernel()
{
    size_t localThreads  = WORKGROUP_SIZE; // 256
    size_t globalThreads = numKeys;

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
    okra_push_pointer(kernel, svmTreeBuf);
    okra_push_pointer(kernel, svmSearchBuf);
    cout << "Setting kernel args done!\n";

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

    return SDK_SUCCESS;
}

int SVMBinaryTreeSearch::svmBinaryTreeCPUReference()
{
  searchKey* keyPtr        = (searchKey*)svmSearchBuf;
  searchKey* currKey       = keyPtr;
  node*      searchNode    = svmRoot;
  int        status        = SDK_SUCCESS;

  for(int i = 0; i < numKeys; ++i)
    {
      /* search tree */
      searchNode    = svmRoot;

      while(NULL != searchNode)
	{
	  if(currKey->key == searchNode->value)
	    {
	      /* rejoice on finding key */
	      currKey->nativeNode = searchNode;
	      searchNode          = NULL;
	    }
	  else if(currKey->key < searchNode->value)
	    {
	      /* move left */
	      searchNode = searchNode->left;
	    }
	  else
	    {
	      /* move right */
	      searchNode = searchNode->right;
	    }
	}

      /* move to next key */
      currKey += 1;
    } 

  return SDK_SUCCESS;
}

int SVMBinaryTreeSearch::setup()
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

int SVMBinaryTreeSearch::run()
{
    int status = 0;

    //create the binary tree
    status = cpuCreateBinaryTree();
    CHECK_ERROR(status, SDK_SUCCESS, "cpuCreateBinaryTree() failed.");

    //initialize search keys
    status = cpuInitSearchKeys();
    CHECK_ERROR(status, SDK_SUCCESS, "cpuInitSearchKeys() failed.");

    //warm up run
    if(runCLKernels() != SDK_SUCCESS)
      {
	return SDK_FAILURE;
      }
    
    cout << "-------------------------------------------" << std::endl;
    cout << "Executing kernel for " << iterations
              << " iterations" << std::endl;
    cout << "-------------------------------------------" << std::endl;

    for(int i = 0; i < iterations; i++)
    {
        // Arguments are set and execution call is enqueued on command buffer
        if(runCLKernels() != SDK_SUCCESS)
        {
            return SDK_FAILURE;
        }
    }

    return SDK_SUCCESS;
}

int SVMBinaryTreeSearch::verifyResults()
{
  int status = SDK_SUCCESS;
      // reference implementation
      svmBinaryTreeCPUReference();
      
      // compare the results and see if they match
      status = compare();
      if(SDK_SUCCESS == status)
        {
	  cout << "Passed!\n" << std::endl;
        }
      else
	{
	  cout << "Failed\n" << std::endl;
	}
  return status;
}

int SVMBinaryTreeSearch::cleanup()
{
    // Releases OpenCL resources (Context, Memory etc.)
    int status = 0;

    //dispose okra resources
    okra_dispose_kernel(kernel);
    okra_dispose_context(context);

    free(svmTreeBuf);
    free(svmSearchBuf);
    return SDK_SUCCESS;
}

int SVMBinaryTreeSearch::lrand()
{
  float frand;

  /* generate a real random number between 0 and 1.0 */
  frand = (float)rand()/(float)(RAND_MAX);

  /* convert to the range needed */
  return (int)(frand*localRandMax);
}

/**
 * cpuCreateBinaryTree()
 * creates a tree from the data in "svmTreeBuf". If this is NULL returns NULL
 * else returns root of the tree. 
 **/
int SVMBinaryTreeSearch::cpuCreateBinaryTree()
{
  node*    root;
  int   status;

  status = cpuInitNodes();
  CHECK_ERROR(status, SDK_SUCCESS, "cpuInitNodes() failed.");

  root   = cpuMakeBinaryTree();
  CHECK_ERROR(status, SDK_SUCCESS, "cpuMakeBinaryTree() failed.");
  
  svmRoot = root;

  return SDK_SUCCESS;
}

node* SVMBinaryTreeSearch::cpuMakeBinaryTree()
{
  node* root = NULL;
  node* data;
  node* nextData;
  node* nextNode;
  bool  insertedFlag = false;

  if (NULL != svmTreeBuf)
    {
      /* allocate first node to root */
      data     = (node *)svmTreeBuf;
      nextData = data;
      root     = nextData;

      /* iterative tree insert */
      for (int i = 1; i < numNodes; ++i)
	{
	  nextData = nextData + 1;

	  nextNode     = root;
	  insertedFlag = false;
	  
	  while(false == insertedFlag)
	    {
	      if(nextData->value <= nextNode->value)
		{
		  /* move left */
		  if(NULL == nextNode->left)
		    {
		      nextNode->left   = nextData;
		      insertedFlag     = true;
		    }
		  else
		    {
		      nextNode = nextNode->left;
		    }
		}
	      else
		{
		  /* move right */
		  if(NULL == nextNode->right)
		    {
		      nextNode->right  = nextData;
		      insertedFlag     = true;
		    }
		  else
		    {
		      nextNode = nextNode->right;
		    }
		}
	    }
	}
    }

  return root;
}

int SVMBinaryTreeSearch::cpuInitNodes()
{
  node* nextData;

  if (NULL != svmTreeBuf)
    {
      /* get the first node */
      nextData = (node *)svmTreeBuf;

      /* initialize nodes */
      for (int i = 0; i < numNodes; ++i)
	{
	  /* allocate a random value to node */
	  nextData->value  = lrand();

	  /* all pointers are null */
	  nextData->left   = NULL;
	  nextData->right  = NULL;

	  nextData = nextData + 1;
	}
    }
  else
    {
      return SDK_FAILURE;
    }

  return SDK_SUCCESS;
}

int SVMBinaryTreeSearch::cpuInitSearchKeys()
{
  searchKey* nextData;
  int        status = SDK_SUCCESS;

  if (NULL != svmSearchBuf)
    {
      /* get the first node */
      nextData = (searchKey *)svmSearchBuf;

      /* initialize nodes */
      for (int i = 0; i < numKeys; ++i)
	{
	  /* allocate a random value to node */
	  nextData->key        = lrand();
	  nextData->oclNode    = NULL;
	  nextData->nativeNode = NULL;

	  nextData = nextData + 1;
	}
    }
  else
    {
      status =  SDK_FAILURE;
    }

  return status;
}

int SVMBinaryTreeSearch::compare()
{
  searchKey* keyPtr         = (searchKey*)svmSearchBuf;
  searchKey* currKey        = keyPtr;
  int        compare_status = SDK_SUCCESS;
  int        status;

  for(int i = 0; i < numKeys; ++i)
    {
      /* compare OCL and native nodes */
      if(currKey->oclNode != currKey->nativeNode)
	{
	  compare_status = SDK_FAILURE;
	}

      /* next key */
      currKey += 1;
    }

  return compare_status;
}


int SVMBinaryTreeSearch::printInOrder()
{
  int status;

  status  = recursiveInOrder(svmRoot);

  return SDK_SUCCESS;
}

int SVMBinaryTreeSearch::recursiveInOrder(node* leaf)
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
    SVMBinaryTreeSearch clSVMBinaryTree;

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
