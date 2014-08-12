// University of Illinois/NCSA
// Open Source License
// 
// Copyright (c) 2013, Advanced Micro Devices, Inc.
// All rights reserved.
// 
// Developed by:
// 
//     Runtimes Team
// 
//     Advanced Micro Devices, Inc
// 
//     www.amd.com
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal with
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:
// 
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimers.
// 
//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimers in the
//       documentation and/or other materials provided with the distribution.
// 
//     * Neither the names of the LLVM Team, University of Illinois at
//       Urbana-Champaign, nor the names of its contributors may be used to
//       endorse or promote products derived from this Software without specific
//       prior written permission.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE
// SOFTWARE.
//===----------------------------------------------------------------------===//

#include "okra.h"
#include <iostream>
#include <string>
#include "utils.h"
#include <cstdlib>
#include <cmath>

using namespace std;

static const int NUMELEMENTS = 100000;
float *rX = new float[NUMELEMENTS];
float *rY = new float[NUMELEMENTS];
int *out = new int[NUMELEMENTS];

int main(int argc, char *argv[]) {
	
	string sourceFileName = "CalcPI.hsail";
	char* calcPISource = buildStringFromSourceFile(sourceFileName);
	double piValue;

	for (int i=0;i<NUMELEMENTS; i++) {
		rX[i] = (double)rand() / (double)RAND_MAX ;
		rY[i] = (double)rand() / (double)RAND_MAX ;
	}

        okra_status_t status;
      
        //create okra context
	okra_context_t* context = NULL;
        
        status = okra_get_context(&context);
        
	if (status != OKRA_SUCCESS) {cout << "Error while creating context:" << (int)status << endl; exit(-1);}
        
        //create kernel from hsail
        okra_kernel_t* kernel = NULL;        
	
        status = okra_create_kernel(context, calcPISource, "&__OpenCL_calcPI_kernel", &kernel);

	if (status != OKRA_SUCCESS) {cout << "Error while creating kernel:" << (int)status << endl; exit(-1);}
        
        //setup kernel arguments        
        okra_clear_args(kernel);
        okra_push_pointer(kernel, rX);
        okra_push_pointer(kernel, rY);
        okra_push_pointer(kernel, out);

        //setup execution range
        okra_range_t range;
        range.dimension=1;
        range.global_size[0] = NUMELEMENTS;
        range.global_size[1] = range.global_size[2] = 1;
        range.group_size[0] = 256;
        range.group_size[1] = range.group_size[2] = 1;
        
        //execute kernel and wait for completion
        status = okra_execute_kernel(context, kernel, &range);
        if(status != OKRA_SUCCESS) {cout << "Error while executing kernel:" << (int)status << endl; exit(-1);}

	int total = 0;
	for (int i=0;i<NUMELEMENTS; i++) 
		if (out[i]) total++;
	piValue = (double)total/(double)NUMELEMENTS * 4;

	cout << endl << " pi = " << piValue << endl;
	bool passed = (abs(piValue - 3.141516) < 0.0001) ? true : false;

 	cout << endl << (passed ? "PASSED" : "FAILED") << endl;

        //dispose okra resources
	okra_dispose_kernel(kernel);
        okra_dispose_context(context);
 	
	return 0;
}
