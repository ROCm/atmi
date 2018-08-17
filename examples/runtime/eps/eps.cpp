/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <vector>
#include <time.h>
#include <math.h>
#include "atmi_runtime.h"
#define NSECPERSEC 1000000000L
#define NTIMERS 4

static int TDEPTH = 15;
static int TDEGREE = 2;

long int get_nanosecs( struct timespec start_time, struct timespec end_time) ;

enum {
    GPU_IMPL = 42
};    

static atmi_kernel_t kernel;
/*  Recursive call to create the inverted task tree */
void fib(const int cur_depth, atmi_task_handle_t *thisTaskHandle) {
    ATMI_LPARM_1D(lparm, 1); 
    lparm->kernel_id = GPU_IMPL;
    atmi_task_handle_t *requires = NULL;
    if(cur_depth < TDEPTH) {
        requires = (atmi_task_handle_t *)malloc(TDEGREE * sizeof(atmi_task_handle_t));
        for(int i = 0; i < TDEGREE; i++) {
            atmi_task_handle_t t;
            //printf("[EPS] Creating the %luth task\n", tasks.size() - 1);
            fib(cur_depth + 1, &t);
            requires[i] = t;
        }
        lparm->num_required = TDEGREE;
        lparm->requires = requires;
    }
    *thisTaskHandle = atmi_task_launch(lparm, kernel, NULL);
    if(requires) free(requires);
}

int main(int argc, char *argv[]) {
    atmi_status_t err = atmi_init(ATMI_DEVTYPE_ALL);
    if(err != ATMI_STATUS_SUCCESS) return -1;
#ifndef USE_BRIG
    const char *module = "nullKernel.hsaco";
    atmi_platform_type_t module_type = AMDGCN;
#else
    const char *module = "nullKernel.brig";
    atmi_platform_type_t module_type = BRIG;
#endif
    err = atmi_module_register(&module, &module_type, 1);

    const unsigned int num_args = 0;
    atmi_kernel_create_empty(&kernel, num_args, NULL);
    atmi_kernel_add_gpu_impl(kernel, "nullKernel_impl", GPU_IMPL);

    struct timespec start_time[NTIMERS],end_time[NTIMERS];
    long int tdegree,nanosecs[NTIMERS];
    float eps[NTIMERS];
    
    if(argc == 3) {
        TDEGREE = atoi(argv[1]);
        TDEPTH = atoi(argv[2]);
    }
    /* Initialize the Kernel */
    ATMI_LPARM_1D(lparm, 1);
    lparm->synchronous=ATMI_TRUE;
    lparm->kernel_id = GPU_IMPL;
    atmi_task_launch(lparm, kernel, NULL);

    int ntasks = (TDEGREE <= 1) ? TDEPTH : (1 - pow((float)TDEGREE, (float)TDEPTH)) / (1 - TDEGREE);
    int ndependencies = ntasks - 1; 

#if 1
    std::vector<atmi_task_handle_t> task_handles;
    task_handles.resize(ntasks);

    clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[2]);
    clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[3]);
    int ntasks_n = pow((float)TDEGREE, (float)TDEPTH-1);
    int start_idx = ntasks;
    for(int level = TDEPTH - 1; level >= 0; level--) {
        start_idx -= ntasks_n;
        //printf("Level: %d N_Tasks: %d\n", level, ntasks_n);
        for(int i = 0; i < ntasks_n; i++) {
            ATMI_LPARM_1D(lparm, 1);
            lparm->synchronous = ATMI_FALSE;
            lparm->kernel_id = GPU_IMPL;
            atmi_task_handle_t *requires = (atmi_task_handle_t *)malloc(TDEGREE * sizeof(atmi_task_handle_t));
            if(level != TDEPTH - 1) {
                for(int deg = 0; deg < TDEGREE; deg++) {
                    int req_idx = (start_idx + i) * TDEGREE + deg + 1;
                    requires[deg] = task_handles[req_idx];
                }
                lparm->num_required = TDEGREE;
                lparm->requires = requires;
            }
            task_handles[start_idx + i] = atmi_task_launch(lparm, kernel, NULL);
            free(requires); 
        }
        ntasks_n /= TDEGREE;
    }
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[2]);
    atmi_task_handle_t t_bf = task_handles[0];
    atmi_task_wait(t_bf);
    //SYNC_TASK(t_bf);

    clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[3]);
#endif
#if 1
    clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[0]);
    clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[1]);
    atmi_task_handle_t t_dfs;
    fib(1, &t_dfs);
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[0]);
    atmi_task_wait(t_dfs);
    //SYNC_TASK(t_dfs);
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[1]);
#endif

    for(int i=0; i<NTIMERS; i++) {
        nanosecs[i] = get_nanosecs(start_time[i],end_time[i]);
        eps[i] = ((float) ndependencies * (float) NSECPERSEC) / (float) nanosecs[i] ;
    }

    printf("Kernel Calls     =  %d\n",ntasks);
    printf("Number of Edges (Dependencies)     =  %d\n",ndependencies);
   
    int barrier_pkt_count = (TDEGREE + 4 - 1) / 4;
    //int nsignals = (ntasks - pow((float) TDEGREE, (float) (TDEPTH-1))) * barrier_pkt_count;
    //printf("Number of signals: %d\n", ntasks + nsignals + 10);
    printf("Task Tree Depth-First Navigation\n");
    printf("Secs to dispatch =  %10.8f\n",((float)nanosecs[0])/NSECPERSEC);
    printf("EPS DF dispatched   =  %6.0f\n",eps[0]);
    printf("Secs to complete =  %10.8f\n",((float)nanosecs[1])/NSECPERSEC);
    printf("EPS DF completed    =  %6.0f\n\n",eps[1]);
    
    printf("Task Tree Breadth-First Navigation\n");
    printf("Secs to dispatch =  %10.8f\n",((float)nanosecs[2])/NSECPERSEC);
    printf("EPS BF dispatched   =  %6.0f\n",eps[2]);
    printf("Secs to complete =  %10.8f\n",((float)nanosecs[3])/NSECPERSEC);
    printf("EPS BF completed    =  %6.0f\n\n",eps[3]);
    return 0;
}

long int get_nanosecs( struct timespec start_time, struct timespec end_time) {
   long int nanosecs;
   if ((end_time.tv_nsec-start_time.tv_nsec)<0) nanosecs = 
      ((((long int) end_time.tv_sec- (long int) start_time.tv_sec )-1)*NSECPERSEC ) +
      ( NSECPERSEC + (long int) end_time.tv_nsec - (long int) start_time.tv_nsec) ;
   else nanosecs = 
      (((long int) end_time.tv_sec- (long int) start_time.tv_sec )*NSECPERSEC ) +
      ( (long int) end_time.tv_nsec - (long int) start_time.tv_nsec );
   return nanosecs;
}
