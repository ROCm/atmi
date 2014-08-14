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

using namespace std;

static const int M = 4;
static const int N = 5;
static const int P = 6;
int *A, *B, *C, *D;
// C = A * B 

void print_mat(int *A, int m, int n) {
int i,j;
for (i=0;i<m;i++) {
   for(j=0;j<n;j++)
        printf(" %d",A[i*n+j]);
   printf("\n");
   }
}


int main(int argc, char *argv[]) {
	
	string sourceFileName = "MatMul.hsail";
	char* matMulSource = buildStringFromSourceFile(sourceFileName);
	int i,j,k;

	A = (int *) malloc(M*N*sizeof(int));
	B = (int *) malloc(N*P*sizeof(int));
	C = (int *) malloc(M*P*sizeof(int));
	D = (int *) malloc(M*P*sizeof(int));

	for (i=0;i<M; i++) 
	    for (j=0;j<N; j++) 
		A[i*N+j] = (i + j);
//		A[i][j] = rand() ;

	for (i=0;i<N; i++) 
	    for (j=0;j<P; j++) 
		B[i*P+j] = (i - j);
//		B[i][j] = rand() ;

        okra_status_t status;
      
        //create okra context
	okra_context_t* context = NULL;
        
        status = okra_get_context(&context);
        
	if (status != OKRA_SUCCESS) {cout << "Error while creating context:" << (int)status << endl; exit(-1);}
        
        //create kernel from hsail
        okra_kernel_t* kernel = NULL;        
	
        status = okra_create_kernel(context, matMulSource, "&__OpenCL_matmul_kernel", &kernel);

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
        okra_push_pointer(kernel, B);
        okra_push_pointer(kernel, C);
        okra_push_pointer(kernel, (void *)&N);
        okra_push_pointer(kernel, (void *)&P);

        //setup execution range
        okra_range_t range;
        range.dimension=2;
        range.global_size[0] = M;
        range.global_size[1] = P;
        range.group_size[0] = 32;
        range.group_size[1] = 32;
	range.global_size[2] = range.group_size[2] = 1;
        
        //execute kernel and wait for completion
        status = okra_execute_kernel(context, kernel, &range);
        if(status != OKRA_SUCCESS) {cout << "Error while executing kernel:" << (int)status << endl; exit(-1);}

	bool passed = true;
	for (i=0;i<M;i++) 
	    for (j=0;j<P; j++)  {
		D[i*P+j] = 0;
		for (k=0;k<N;k++)
		    D[i*P+j] += A[i*N+k]*B[k*P+j];
		if (D[i*P+j] != C[i*P+j]) passed = false;
	    }

 	cout << endl << (passed ? "PASSED" : "FAILED") << endl;

#if 0
	cout << " Matrix A is : " << endl;
	print_mat(A, M, N);
	cout << " Matrix B is : " << endl;
	print_mat(B, N, P);
	cout << " Matrix C is : " << endl;
	print_mat(C, M, P);
	cout << " Matrix D is : " << endl;
	print_mat(D, M, P);
#endif

        //dispose okra resources
	okra_dispose_kernel(kernel);
        okra_dispose_context(context);
 	
	return 0;
}
