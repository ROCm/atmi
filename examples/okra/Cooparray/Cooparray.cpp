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
#include <sstream>
#include "../utils.h"
#include <math.h>

using namespace std;

/*************************
 * This example dispatches an hsail kernel that might be generated from
 * something like:
 *
 *       (gid) -> {
 *                  Point pt = inArray[gid];
 *                  float distance = sqrt(pt.x * pt.x + pt.y * pt.y + pt.z * pt.z);
 *                  outArray[gid] = distance;
 *                }
 *
 * in other words inArray is an array of references to Point objects.
 * In this example, the lambda is treated as static in that both inArray and outArray 
 * are passed to the resulting kernel.
 *
 ******************/


static const int NUMELEMENTS = 40;

static string showFloat(float f) {
	ostringstream buff;
	buff << f << " (" << hex << *(unsigned int *)&f << dec << ")";
	return buff.str();
}

typedef struct {
	float x,y,z;
} Point;


Point *inArray = new Point[NUMELEMENTS];
float *outArray = new float[NUMELEMENTS];



int main(int argc, char *argv[]) {
	string sourceFileName = "Cooparray.hsail";
	char* sourceStr = buildStringFromSourceFile(sourceFileName);

        okra_status_t status;
      
	//create okra context
	okra_context_t* context = NULL;

	status = okra_get_context(&context);
	
	if (status != OKRA_SUCCESS) {cout << "Error while creating context:" << (int)status << endl; exit(-1);}

	//create kernel from hsail
	okra_kernel_t* kernel = NULL;

	status = okra_create_kernel(context, sourceStr, "&__OpenCL_coop_kernel", &kernel);

	if (status != OKRA_SUCCESS) {cout << "Error while creating kernel:" << (int)status << endl; exit(-1);}	

	// initialize inArray
	for (int i=0; i<NUMELEMENTS; i++) {
		inArray[i].x = i;
		inArray[i].y = i+1;
		inArray[i].z = i+2;
	}

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
        okra_push_pointer(kernel, outArray);
        okra_push_pointer(kernel, inArray);       	

        //setup execution range
        okra_range_t range;
        range.dimension=1;
        range.global_size[0] = NUMELEMENTS;
        range.global_size[1] = range.global_size[2] = 1;
        range.group_size[0] = NUMELEMENTS;
        range.group_size[1] = range.group_size[2] = 1;

	    //execute kernel and wait for completion
        status = okra_execute_kernel(context, kernel, &range);
        if(status != OKRA_SUCCESS) {cout << "Error while executing kernel:" << (int)status << endl; exit(-1);}

	// show results
	bool passed = true;
	for (int i=0; i<NUMELEMENTS; i++) {
//		cout << dec << i << "->" << showFloat(outArray[i]) << ",  ";
//		if (i%5 == 4) cout << endl;
		float expected = sqrt((float)(i*i + (i+1)*(i+1) + (i+2)*(i+2)));
		if (abs(outArray[i] - expected) > 0.00001) {
			cout << "saw " << outArray[i] << ", expected " << expected << endl;
			passed = false;
		}
	}
	cout << endl << (passed ? "PASSED" : "FAILED") << endl;

        //dispose okra resources
	okra_dispose_kernel(kernel);
        okra_dispose_context(context);
	
	return 0;
}
