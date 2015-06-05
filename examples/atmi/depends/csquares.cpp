//
// csquares.cpp : Demo of SNACK task dependencies
//
// Creates a diamond DAG using three kernels.
// Demo only, not intended as efficient algorithm.
//
// Init N values:              (init)        
//                            /      \
//                          |/        \|
// Do 1/4 N each: (even_squares)   (even_squares)  
//                           \        /
//                            \|    |/
// Do odd 1/2 N:            (odd_squares)  
//

#include <string.h>
#include <stdlib.h>
#include <iostream>
using namespace std;
typedef int TYPE;
//#define GPU_TASKS
// Include snack function headers created by "snack.sh -c csquares.cl"
#include "csquares.h"

static const int N = 16000; /* multiple of 4 for demo */

extern "C" void init_kernel(int *in) {
	int i;
    for(i = 0; i < N; i++)
	    in[i] = (int)  i;
}

/*  Middle children calculate squares for even numbers */
extern "C" void even_squares_kernel(
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
extern "C" void odd_squares_kernel(int *out) 
{
    int ctr;
    for(ctr = 0; ctr < N/2; ctr++) {
        int i = (ctr*2) + 1;
        out[i] = out[i-1] + (2*i) - 1; 
    }
}

int main(int argc, char *argv[]) {
   TYPE *inArray = new TYPE[N];
   TYPE *outArray = new TYPE[N];

   // Create launch parameters with thread counts
   ATMI_LPARM_1D(init_gpu_lp,N);
   ATMI_LPARM_CPU(init_cpu_lp);
   // Each even tasks caclulates 1/4 of the squares
   ATMI_LPARM_1D(even_gpu_lp,N/4);
   // The final odd task does 1/2 of the squares
   ATMI_LPARM_1D(odd_gpu_lp,N/2);
   ATMI_LPARM_CPU(odd_cpu_lp);
 
   atmi_task_t *init_tasks[1];
   atmi_task_t *even_tasks[2];

#ifdef GPU_TASKS
   // Dispatch init_kernel and set even_gpu_lp to require init to complete
   init_tasks[0] = init_kernel_pif(init_gpu_lp, inArray);
   even_gpu_lp->num_required = 1;
   even_gpu_lp->requires = init_tasks;

   // Dispatch 2 even_squares kernels and build dependency list for odd_squares.
   even_tasks[0] = even_squares_kernel_pif(even_gpu_lp, inArray, outArray);
   even_tasks[1] = even_squares_kernel_pif(even_gpu_lp, &inArray[N/2], &outArray[N/2]);
   odd_gpu_lp->num_required = 2;
   odd_gpu_lp->requires = even_tasks;
  
   // Now dispatch odd_squares kernel dependent on BOTH even_squares  
   atmi_task_t* ret_task = odd_squares_kernel_pif(odd_gpu_lp, outArray);
#else
   // Dispatch init_kernel and set even_gpu_lp to require init to complete
   init_tasks[0] = init_kernel_pif(init_cpu_lp, inArray);
   even_gpu_lp->num_required = 1;
   even_gpu_lp->requires = init_tasks;

   // Dispatch 2 even_squares kernels and build dependency list for odd_squares.
   even_tasks[0] = even_squares_kernel_pif(even_gpu_lp, inArray, outArray);
   even_tasks[1] = even_squares_kernel_pif(even_gpu_lp, &inArray[N/2], &outArray[N/2]);
   odd_cpu_lp->num_required = 2;
   odd_cpu_lp->requires = even_tasks;
   // Now dispatch odd_squares kernel dependent on BOTH even_squares  
   atmi_tprofile_t odd_cpu_profile; 
   odd_cpu_lp->profile = &odd_cpu_profile;
   atmi_task_t* ret_task = odd_squares_kernel_pif(odd_cpu_lp, outArray);
#endif
   // Wait for all kernels to complete
   SYNC_STREAM(0);
   //SYNC_TASK(ret_task);
   //cout << "Last Task Dispatch Timestamp: " << ret_task->profile->dispatch_time << endl;
   //cout << "Last Task Start Timestamp: " << ret_task->profile->start_time << endl;
   //cout << "Last Task End Timestamp: " << ret_task->profile->end_time << endl;
   //cout << "Last Task Wait_Time: " << ret_task->profile->start_time - ret_task->profile->dispatch_time << " ns" << endl;
   //cout << "Last Task Time: " << ret_task->profile->end_time - ret_task->profile->start_time << " ns" << endl;
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
