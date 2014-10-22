/*******************************************************************************
Copyright ©2013 Advanced Micro Devices, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1 Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2 Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

/**
********************************************************************************
* @file <HsaApp.cpp>
*
* @brief This file contains functions for initializing HSA CU.
* It creates a binary search tree in shared virtual memory and also
* enqueues work to the CU for creating node and inserting in the same Binary Search Tree
*
* This shows SVM and atomics functionality of HSA.
********************************************************************************
*/

#include "okra.h"
#include <iostream>
#include <fstream>
#include <ostream>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <cstdlib>
#include <cstring>
#include <math.h>

#include "BVH.h"
#include "HsaApp.h"

#define USE_OPENMP
#ifdef _DEBUG
#undef USE_OPENMP
#endif

using namespace std;

bool bvhsortx_float (float_3 s,float_3 t) { return (s.x<t.x); }
inline Axis NextAxis(Axis d){ return Axis((d+1)%(Z+1));}

#define DIM         1024
//#define NSPHERES    2*1024*1024

#define BUILD_LOG_SIZE  102400
//#define OBJ_SIZE        (size_t)256*NSPHERES
//#define BVH_NODES_SIZE  (size_t)128*NSPHERES

long long int num_search_points = 1024*1024;
int gNSpheres;
int iteration = 1;
int search_per_wi = 1;
int size[4] = {64,16,4,256};//, 16,4};
float_3 *search_points = NULL;

double time_spent;
int globalNodeCount;

void* cb_malloc(size_t size) {
    return malloc(size);
}

void cb_free(void* ptr) {
    free(ptr);
}

void* cb_svmalloc(size_t size) {
    return cb_malloc (size);
}

void cb_svmfree(void* ptr) {
    cb_free (ptr);
}

void initialize_search_points(Sphere* Slist,float_3 *search_points, long long int num_search_points)
{

    // Search points are random points
    for (int i = 0; i < num_search_points; i++)
    {
       search_points[i].x = floor(random(-3500.0f,3500.0f));
        search_points[i].y = floor(random(-3500.0f, 3500.0f));
        search_points[i].z = floor(random(-35.0f, 35.0f));

	}
   //std::sort(search_points,search_points+num_search_points,bvhsortx_float);
}

char *buildStringFromSourceFile(string fname) {
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


int main(int argc, char* argv[])
{
    bool runHSA = true;
    if (argc > 1) {
        int temp = atoi(argv[1]);
        runHSA = ((temp & 1) != 0);
#if 0//def USE_OPENMP
        if ((!runHSA) && (temp == 0))
            omp_set_num_threads(1);
#endif
    }
    srand(100);
    gNSpheres = 1024*1024*atoi(argv[2]);
    size_t  _BVH_NODES_SIZE = (size_t) 128*gNSpheres;
    size_t  _OBJ_SIZE =(size_t) 256*gNSpheres;
    /* Generate random sphere data */
    Sphere *tempS = (Sphere*)malloc(sizeof(Sphere) * gNSpheres);
    int x = -3500, y= -3500;
    for (int i=0; i<gNSpheres; i++) {
        Sphere S;
        memset(&S, 0, sizeof(Sphere));
        S.r = random(0.0f, 1.0f);
        S.g = random(0.0f, 1.0f);
        S.b = random(0.0f, 1.0f);
	S.x = x;
        S.y = y;
        S.z = 0;
	x += 2;
	if (x == 3500) {
		x = -3500;
		y += 2;
	}
        S.radius = 0.25f;
        S.radius2 = S.radius*S.radius;
        S.idx = i;
        tempS[i] = S;
    }

    /* Populate search points */
    if ((search_points = (float_3*)malloc(num_search_points * sizeof(float_3))) == NULL) {
        std::cerr << "Error allocating memory for search points" << std::endl;
        exit(-1);
    }
    initialize_search_points(tempS, search_points, num_search_points);
    std::cout << "Search point range -3500 to 3500 " <<  std::endl;

    okra_status_t status;
    string sourceFileName = "BVH.hsail";
    char* svmTreeSource = buildStringFromSourceFile(sourceFileName);

    context = NULL;

    status = okra_get_context(&context);
    if (status != OKRA_SUCCESS) {std::cout << "Error while creating context:" << (int)status << std::endl; exit(-1);}

    status = okra_create_kernel(context, svmTreeSource, "&__OpenCL_bvh_search_kernel", &kernel);

    if (status != OKRA_SUCCESS) {std::cout << "Error while creating kernel:" << (int)status << std::endl; exit(-1);}

    /* Build BVH nodes */
    mem_mgr nodeData, objectData;
    create_mem_mgr(&nodeData, cb_svmalloc, cb_svmfree);
    create_mem_mgr(&objectData, cb_svmalloc, cb_svmfree);
    nodeData->mem_init(nodeData, _BVH_NODES_SIZE);
    objectData->mem_init(objectData, _OBJ_SIZE);

    BVH *b;
    if ((b = (BVH*)nodeData->mem_malloc(nodeData, sizeof(BVH))) == NULL) {
       std::cerr << "Error allocating memory for BVH object." << std::endl;
       exit(1);
    }

    OutIdx *found_idxs_HSA = NULL;
    if ((found_idxs_HSA = (OutIdx *) malloc(num_search_points * sizeof(OutIdx))) == NULL) {
        std::cerr << "Error allocating memory for found indices." << std::endl;
        exit(1);
    }

    buildBVH(b, tempS, NULL, gNSpheres, X, nodeData, objectData);

    /* Run BVH traversal kernel */
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
    okra_push_pointer(kernel, b);
    okra_push_pointer(kernel, search_points);
    okra_push_pointer(kernel, found_idxs_HSA);

    std::cout << "search_per_wi = " << search_per_wi << std::endl;

    bool warmmedup = false;
		
//    for (int j = 0; j < sizeof(size) / sizeof(int); j++)
    for (int j = 0; j < 1; j++)
    {
        size_t globalThreads = (size_t)(num_search_points / search_per_wi);
        size_t localThreads = size[j];
			
        memset(found_idxs_HSA, 0, num_search_points * sizeof(OutIdx));
        for (int i = 0; i < 1; i++)
        {
            okra_range_t range;
            range.dimension=1;
            range.global_size[0] = globalThreads;
            range.global_size[1] = range.global_size[2] = 1;
            range.group_size[0] = localThreads;
            range.group_size[1] = range.group_size[2] = 1;

            //execute kernel and wait for completion
            okra_status_t status = okra_execute_kernel(context, kernel, &range);
            if(status != OKRA_SUCCESS) {std::cout << "Error while executing kernel:" << (int)status << std::endl; exit(-1);}

         }

    int numFound = 0;
    for(int i=0;i<num_search_points;i++)  {
	if (found_idxs_HSA[i].idx != 0) numFound++;
//	std::cout << " found_idxs_HSA[i] = " << (int)found_idxs_HSA[i].idx << std::endl;
	}
    std::cout << " num_search_points = " << num_search_points << " numFound = " << numFound << std::endl;

    }

    /* Cleanup */
    destroyBVH(b);
    free(found_idxs_HSA);
    objectData->mem_destroy(objectData);
    nodeData->mem_destroy(nodeData);
    //dispose okra resources
    okra_dispose_kernel(kernel);
    okra_dispose_context(context);

    /* Final cleanup */
    free(search_points);
    free(tempS);
    return 0;
}
