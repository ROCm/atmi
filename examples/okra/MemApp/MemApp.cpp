/* Copyright 2014 HSA Foundation Inc.  All Rights Reserved.
 *
 * HSAF is granting you permission to use this software and documentation (if
 * any) (collectively, the "Materials") pursuant to the terms and conditions
 * of the Software License Agreement included with the Materials.  If you do
 * not have a copy of the Software License Agreement, contact the  HSA Foundation for a copy.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.
 */
#include "okra.h"
#include <iostream>
#include <string>
#include "../utils.h"
#include <cstdlib>
#include <cmath>

#define G 1024*1024*1024
#define K 1024

using namespace std;

int main(int argc, char *argv[]) {
	
	string sourceFileName = "MemApp.hsail";
	char* memAppSource = buildStringFromSourceFile(sourceFileName);

	if (argc != 7) {
		cout << " Usage: MemApp <N> <start> <stride> <laps> <skip> <epocs>; Pass '0' if defaults are OK\n ";
		exit (1);
	}

	int *A;
 	int len,start,stride,laps,skip,epocs;
	int i;

	len = atoi(argv[1]);
	if (!len)
   	    len = 1*G;
	A = (int *) malloc(len * sizeof(int));

	for (i=0;i<len;i++)
	    A[i] = -1;

	start = atoi(argv[2]);
	if (!start)
	    start = K;

	stride = atoi(argv[3]);
	if (!stride)
	    stride = 192;

	laps = atoi(argv[4]);
	if (!laps)
	    laps = 256;;

	skip = atoi(argv[5]);
	if (!skip)
	    skip = 10*K;

	epocs = atoi(argv[6]);
	if (!epocs)
	    epocs = 4*K;

	cout << " len = " << len;
	cout << " start = " << start;
	cout << " stride = " << stride;
	cout << " laps = " << laps;
	cout << " skip = " << skip;
	cout << " epocs = " << epocs;

        okra_status_t status;
      
        //create okra context
	okra_context_t* context = NULL;
        
        status = okra_get_context(&context);
        
	if (status != OKRA_SUCCESS) {cout << "Error while creating context:" << (int)status << endl; exit(-1);}
        
        //create kernel from hsail
        okra_kernel_t* kernel = NULL;        
	
        status = okra_create_kernel(context, memAppSource, "&__OpenCL_memApp_kernel", &kernel);

	if (status != OKRA_SUCCESS) {cout << "Error while creating kernel:" << (int)status << endl; exit(-1);}
        
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
        okra_push_pointer(kernel, A);
        okra_push_pointer(kernel, (void *)&len);
        okra_push_pointer(kernel, (void *)&start);
        okra_push_pointer(kernel, (void *)&stride);
        okra_push_pointer(kernel, (void *)&laps);
        okra_push_pointer(kernel, (void *)&skip);
        okra_push_pointer(kernel, (void *)&epocs);

        //setup execution range
        okra_range_t range;
        range.dimension=1;
        range.global_size[0] = 256;
        range.group_size[0] = 64;
	range.global_size[1] = range.group_size[1] = 1;
	range.global_size[2] = range.group_size[2] = 1;
        
        //execute kernel and wait for completion
        status = okra_execute_kernel(context, kernel, &range);
        if(status != OKRA_SUCCESS) {cout << "Error while executing kernel:" << (int)status << endl; exit(-1);}

	cout << " Sampling output: \n";
	for(i=0;i<100;i++)
	    cout << A[i] << " ";
	cout << "\n";

        //dispose okra resources
	okra_dispose_kernel(kernel);
        okra_dispose_context(context);
 	
	return 0;
}
