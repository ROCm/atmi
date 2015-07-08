//
// csquares.cpp : Demo of ATMI task dependencies
//
// Creates a diamond DAG using three kernels.
// Demo only, not intended as efficient algorithm.
//
// Init N values:              (init) on GPU       
//                            /      \
//                          |/        \|
// Do 1/4 N each: (even_squares)   (even_squares)  
//                   on CPU  \        /  on GPU
//                            \|    |/
// Do odd 1/2 N:            (odd_squares) on GPU
//

#include <string.h>
#include <stdlib.h>
#include <iostream>

#include "atmi.h"

using namespace std;
typedef int TYPE;

static const int N = 16; /* multiple of 4 for demo */

/* ---------------- Kernel declarations -------------*/
// Declare init_kernel as the PIF for the init_kernel_cpu subroutine
__kernel void init_kernel_gpu(__global atmi_task_t *thisTask, __global int *in) __attribute__((atmi_kernel("init_kernel", "gpu")));
extern "C" void init_kernel_cpu(atmi_task_t *thisTask, int *in) __attribute__((atmi_kernel("init_kernel", "cpu")));


// Declare even_squares_kernel as the PIF for the even_squares_kernel_cpu subroutine
__kernel void even_squares_kernel_gpu(__global atmi_task_t *thisTask, 
   __global const int *in , __global int *out) __attribute__((atmi_kernel("even_squares_kernel", "gpu")));
extern "C" void even_squares_kernel_cpu(atmi_task_t *thisTask, 
   const int *in , int *out) __attribute__((atmi_kernel("even_squares_kernel", "cpu")));


// Declare odd_squares_kernel as the PIF for the odd_squares_kernel_cpu subroutine
__kernel void odd_squares_kernel_gpu(__global atmi_task_t *thisTask, __global int *out) __attribute__((atmi_kernel("odd_squares_kernel", "gpu")));
extern "C" void odd_squares_kernel_cpu(atmi_task_t *thisTask, int *out) __attribute__((atmi_kernel("odd_squares_kernel", "cpu")));



/* ---------------- Kernel definitions -------------*/
extern "C" void init_kernel_cpu(atmi_task_t *thisTask, int *in) {
	int i;
    for(i = 0; i < N; i++) {
	    in[i] = (int)  i;
        cout << "Initializing in[" << i << "] = " << i << endl;
    }
}

/*  Middle children calculate squares for even numbers */
extern "C" void even_squares_kernel_cpu(atmi_task_t *thisTask, 
   const int *in , int *out) 
{
    int ctr;
    for(ctr = 0; ctr < N/4; ctr++) {
        int i = ctr*2;
        out[i] = in[i] * in[i];
    }
}

/*  The last child calculate squares for odd numbers 
    using squares from even numbers because.
        (X-1)**2 = X**2 - 2X + 1 
    so  X**2 = ((X-1)**2) + 2X - 1
*/
extern "C" void odd_squares_kernel_cpu(atmi_task_t *thisTask, int *out) 
{
    int ctr;
    for(ctr = 0; ctr < N/2; ctr++) {
        int i = (ctr*2) + 1;
        out[i] = out[i-1] + (2*i) - 1; 
    }
}


/* -------------- main ------------------*/
int main(int argc, char *argv[]) {
   TYPE *inArray = new TYPE[N];
   TYPE *outArray = new TYPE[N];

   // Create launch parameters with thread counts
   ATMI_LPARM_1D(init_lp,N);
   // Each even tasks caclulates 1/4 of the squares
   ATMI_LPARM_1D(even_lp,N/4);
   // The final odd task does 1/2 of the squares
   ATMI_LPARM_1D(odd_lp,N/2);
 
   atmi_task_t *init_tasks[1];
   atmi_task_t *even_tasks[2];
   
   // Dispatch init_kernel and set even_lp to require init to complete
   init_tasks[0] = init_kernel(init_lp, inArray);
   even_lp->num_required = 1;
   even_lp->requires = init_tasks;

   // Dispatch 2 even_squares kernels and build dependency list for odd_squares.
   even_tasks[0] = even_squares_kernel(even_lp, inArray, outArray); // Half of even kernels go to CPU 
   even_lp->kernel_id = 1;
   even_tasks[1] = even_squares_kernel(even_lp, &inArray[N/2], &outArray[N/2]); // Other half goes to the GPU
   odd_lp->num_required = 2;
   odd_lp->requires = even_tasks;
   
   // Now dispatch odd_squares kernel dependent on BOTH even_squares  
   atmi_task_t* ret_task = odd_squares_kernel(odd_lp, outArray);

   // Wait for all kernels to complete
   SYNC_TASK(ret_task);

   // Check results
   bool passed = true;
   for (int i=0; i<N; i++) {
     if (outArray[i] != (TYPE)i * (TYPE)i) {
        cout << i << "^2 = " << (TYPE)i * (TYPE)i  << " -> " << outArray[i] << endl;
	 	passed = false;
	 }
   }
   cout << endl << (passed ? "PASSED" : "FAILED") << endl;
   return 0;
}
