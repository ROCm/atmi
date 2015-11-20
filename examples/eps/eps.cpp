#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <libelf.h>
#include <iostream>
#include <vector>
#include <sys/param.h> 
#include <time.h>
#include <math.h>
#include "atmi.h"
#define NSECPERSEC 1000000000L
#define NTIMERS 4

static int TDEPTH = 15;
static int TDEGREE = 2;

long int get_nanosecs( struct timespec start_time, struct timespec end_time) ;

__kernel void nullKernel_impl(__global atmi_task_t *thisTask) __attribute__((atmi_kernel("nullKernel", "GPU")));

std::vector<atmi_task_t *> tasks;
/*  Recursive Fibonacci */
void fib(const int cur_depth, atmi_task_t *my_sum_task) {
    ATMI_LPARM_1D(lparm, 64); /* Remember ATMI default is asynchronous execution */
    lparm->synchronous=ATMI_FALSE;
    atmi_task_t **requires = NULL;
    if(cur_depth < TDEPTH) {
        requires = (atmi_task_t **)malloc(TDEGREE * sizeof(atmi_task_t *));
        for(int i = 0; i < TDEGREE; i++) {
            atmi_task_t *t = new atmi_task_t;
            memset(t, 0, sizeof(atmi_task_t));
            tasks.push_back(t);
            requires[i] = t;// tasks[tasks.size() - 1];
            //printf("[EPS] Creating the %luth task\n", tasks.size() - 1);
        }
        for(int i = 0; i < TDEGREE; i++) {
            fib(cur_depth + 1, requires[i]);
        }
        lparm->num_required = TDEGREE;
        lparm->requires = requires;
    }
    lparm->task = my_sum_task;
    nullKernel(lparm);
    if(requires) free(requires);
}

int main(int argc, char *argv[]) {
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
    nullKernel(lparm);

    int ntasks = (TDEGREE <= 1) ? TDEPTH : (1 - pow((float)TDEGREE, (float)TDEPTH)) / (1 - TDEGREE);
    int ndependencies = ntasks - 1; 
    printf("Task count: %d Dependency count: %d\n", ntasks, ndependencies);

#if 1
    clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[0]);
    clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[1]);
    atmi_task_t root_dfs;
    fib(1, &root_dfs);
    atmi_task_t *t_dfs = &root_dfs;
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[0]);
    //atmi_task_wait(t_dfs);
    SYNC_TASK(t_dfs);
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[1]);
#endif

    /*for(std::vector<atmi_task_t *>::iterator it = tasks.begin(); 
            it != tasks.end(); it++) {
        delete *it;
    }*/
    tasks.clear();
    tasks.resize(ntasks);
    #if 1
    clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[2]);
    clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[3]);
    atmi_task_t root_sum_task;

    int ntasks_n = pow((float)TDEGREE, (float)TDEPTH-1);
    int start_idx = ntasks;
    for(int level = TDEPTH - 1; level >= 0; level--) {
        start_idx -= ntasks_n;
        printf("Level: %d N_Tasks: %d\n", level, ntasks_n);
        for(int i = 0; i < ntasks_n; i++) {
            ATMI_LPARM_1D(lparm, 1);
            lparm->synchronous = ATMI_FALSE;
            atmi_task_t *t = new atmi_task_t;
            memset(t, 0, sizeof(atmi_task_t));
            tasks[start_idx + i] = t;
            atmi_task_t **requires = (atmi_task_t **)malloc(TDEGREE * sizeof(atmi_task_t *));
            if(level != TDEPTH - 1) {
                for(int deg = 0; deg < TDEGREE; deg++) {
                    int req_idx = (start_idx + i) * TDEGREE + deg + 1;
                    requires[deg] = tasks[req_idx];
                }
                lparm->num_required = TDEGREE;
                lparm->requires = requires;
            }
            lparm->task = tasks[start_idx + i];
            nullKernel(lparm);
            free(requires); 
        }
        ntasks_n /= TDEGREE;
    }
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[2]);
    atmi_task_t *t = tasks[0];
    //atmi_task_wait(t);
    SYNC_TASK(t);
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[3]);
    #endif
    for(int i=0; i<NTIMERS; i++) {
        nanosecs[i] = get_nanosecs(start_time[i],end_time[i]);
        eps[i] = ((float) ndependencies * (float) NSECPERSEC) / (float) nanosecs[i] ;
    }

    printf("Kernel Calls     =  %d\n",ntasks);
    printf("Number of Edges (Dependencies)     =  %d\n",ndependencies);
    
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
