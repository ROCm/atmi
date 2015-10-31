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
#define NTIMERS 13

#define TDEGREE 2
#define TDEPTH  10

long int get_nanosecs( struct timespec start_time, struct timespec end_time) ;

__kernel void nullKernel_impl(__global atmi_task_t *thisTask, long int tdegree) __attribute__((atmi_kernel("nullKernel", "GPU")));

std::vector<atmi_task_t *> tasks;
/*  Recursive Fibonacci */
void fib(const int cur_depth, atmi_task_t *my_sum_task) {
    ATMI_LPARM_1D(lparm, 64); /* Remember ATMI default is asynchronous execution */
    lparm->synchronous=ATMI_FALSE;
    if(cur_depth < TDEPTH) {
        atmi_task_t *requires[TDEGREE];
        for(int i = 0; i < TDEGREE; i++) {
            atmi_task_t *t = new atmi_task_t;
            memset(t, 0, sizeof(atmi_task_t));
            tasks.push_back(t);
            requires[i] = tasks[tasks.size() - 1];
            printf("[EPS] Creating the %luth task\n", tasks.size() - 1);
        }
        for(int i = 0; i < TDEGREE; i++) {
            fib(cur_depth + 1, requires[i]);
        }
        lparm->num_required = TDEGREE;
        lparm->requires = requires;
    }
    lparm->task = my_sum_task;
    nullKernel(lparm, TDEGREE);
}

int main(int argc, char *argv[]) {
    struct timespec start_time[NTIMERS],end_time[NTIMERS];
    long int tdegree,nanosecs[NTIMERS];
    float eps[NTIMERS];

    /* Inidialize the Kernel */
    //printf("Null Kernel Starts\n");
    //fflush(stdout);
    ATMI_LPARM_1D(lparm, 1);
    lparm->synchronous=ATMI_TRUE;
    nullKernel(lparm, TDEGREE);

    //printf("Async Unordered Starts\n");
    //fflush(stdout);
    int ntasks = (TDEGREE <= 1) ? TDEPTH : (1 - pow((float)TDEGREE, (float)TDEPTH)) / (1 - TDEGREE);
    printf("Task count: %d\n", ntasks);

#if 0
    clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[3]);
    clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[4]);
    atmi_task_t root_sum_task;
    fib(1, &root_sum_task);
    atmi_task_t *t = &root_sum_task;
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[3]);
    atmi_task_wait(t);
    //SYNC_TASK(t);
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[4]);
#endif

    //printf("Async BFS Starts\n");
    fflush(stdout);
    tasks.clear();
    tasks.resize(ntasks);
    clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[5]);
    clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[6]);
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
            atmi_task_t *requires[TDEGREE];
            if(level != TDEPTH - 1) {
                for(int deg = 0; deg < TDEGREE; deg++) {
                    int req_idx = (start_idx + i) * TDEGREE + deg + 1;
                    //printf("Task[%d] depend on Task[%d]\n", start_idx+i, req_idx);
                    requires[deg] = tasks[req_idx];
                }
                lparm->num_required = TDEGREE;
                lparm->requires = requires;
                for(int j = 0; j < TDEGREE; j++) {
                    //printf("Setting required_task[%d] : %p\n", j, requires[j]);
                }
            }
            lparm->task = tasks[start_idx + i];
            nullKernel(lparm, TDEGREE);
        }
        ntasks_n /= TDEGREE;
    }
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[5]);
    atmi_task_t *t = tasks[0];
    //atmi_task_wait(t);
    SYNC_TASK(t);
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[6]);

#if 0
    stream1->ordered=ATMI_FALSE;
    stream2->ordered=ATMI_FALSE;
    clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[7]);
    clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[8]);
    for(int i=0; i<tdegree/2; i++) {
        nullKernel(lparm1, tdegree); 
        nullKernel(lparm2, tdegree); 
    }
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[7]);
    SYNC_STREAM(stream1); 
    SYNC_STREAM(stream2); 
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[8]);

    stream1->ordered=ATMI_TRUE;
    stream2->ordered=ATMI_TRUE;
    stream3->ordered=ATMI_TRUE;
    stream4->ordered=ATMI_TRUE;
    clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[9]);
    clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[10]);
    for(int i=0; i<tdegree/4; i++) {
        nullKernel(lparm1, tdegree); 
        nullKernel(lparm2, tdegree); 
        nullKernel(lparm3, tdegree); 
        nullKernel(lparm4, tdegree); 
    }
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[9]);
    SYNC_STREAM(stream1); 
    SYNC_STREAM(stream2); 
    SYNC_STREAM(stream3); 
    SYNC_STREAM(stream4); 
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[10]);

    stream1->ordered=ATMI_FALSE;
    stream2->ordered=ATMI_FALSE;
    stream3->ordered=ATMI_FALSE;
    stream4->ordered=ATMI_FALSE;
    clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[11]);
    clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[12]);
    for(int i=0; i<tdegree/4; i++) {
        nullKernel(lparm1, tdegree); 
        nullKernel(lparm2, tdegree); 
        nullKernel(lparm3, tdegree); 
        nullKernel(lparm4, tdegree); 
    }
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[11]);
    SYNC_STREAM(stream1); 
    SYNC_STREAM(stream2); 
    SYNC_STREAM(stream3); 
    SYNC_STREAM(stream4); 
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[12]);
#endif
    for(int i=5; i<=6; i++) {
    //for(int i=0; i<NTIMERS; i++) {
        nanosecs[i] = get_nanosecs(start_time[i],end_time[i]);
        eps[i] = ((float) ntasks * (float) NSECPERSEC) / (float) nanosecs[i] ;
    }

    //printf("Synchronous Execution  \n");
    printf("Kernel Calls     =  %d\n",ntasks);
    //printf("Secs             =  %6.4f\n",((float)nanosecs[0])/NSECPERSEC);
    //printf("Synchronous KPS  =  %6.0f\n\n",eps[0]);
    /*printf("Asynchronous Ordered Execution in 1 stream (stream->ordered=TRUE)\n");
    printf("Secs to dispatch =  %10.8f\n",((float)nanosecs[1])/NSECPERSEC);
    printf("KPS dispatched   =  %6.0f\n",eps[1]);
    printf("Secs to complete =  %10.8f\n",((float)nanosecs[2])/NSECPERSEC);
    printf("KPS completed    =  %6.0f\n\n",eps[2]);
    printf("Asynchronous Unordered Execution in 1 stream (stream->ordered=FALSE)\n");
    printf("Secs to dispatch =  %10.8f\n",((float)nanosecs[3])/NSECPERSEC);
    printf("KPS dispatched   =  %6.0f\n",eps[3]);
    printf("Secs to complete =  %10.8f\n",((float)nanosecs[4])/NSECPERSEC);
    printf("KPS completed    =  %6.0f\n\n",eps[4]);
    */
    //printf("Asynchronous Ordered Eecution in 2 streams\n");
    printf("EPS2 %f = (%f * %f) / %f\n", eps[6], (float) ntasks, (float) NSECPERSEC, (float) nanosecs[6]);
    printf("Secs to dispatch =  %10.8f\n",((float)nanosecs[5])/NSECPERSEC);
    printf("KPS dispatched   =  %6.0f\n",eps[5]);
    printf("Secs to complete =  %10.8f\n",((float)nanosecs[6])/NSECPERSEC);
    printf("KPS completed    =  %6.0f\n\n",eps[6]);
    /*printf("Asynchronous Unordered Execution in 2 streams\n");
    printf("Secs to dispatch =  %10.8f\n",((float)nanosecs[7])/NSECPERSEC);
    printf("KPS dispatched   =  %6.0f\n",eps[7]);
    printf("Secs to complete =  %10.8f\n",((float)nanosecs[8])/NSECPERSEC);
    printf("KPS completed    =  %6.0f\n\n",eps[8]);
    printf("Asynchronous Ordered Execution in 4 streams\n");
    printf("Secs to dispatch =  %10.8f\n",((float)nanosecs[9])/NSECPERSEC);
    printf("KPS dispatched   =  %6.0f\n",eps[9]);
    printf("Secs to complete =  %10.8f\n",((float)nanosecs[10])/NSECPERSEC);
    printf("KPS completed    =  %6.0f\n\n",eps[10]);
    printf("Asynchronous Unordered Execution in 4 streams\n");
    printf("Secs to dispatch =  %10.8f\n",((float)nanosecs[11])/NSECPERSEC);
    printf("KPS dispatched   =  %6.0f\n",eps[11]);
    printf("Secs to complete =  %10.8f\n",((float)nanosecs[12])/NSECPERSEC);
    printf("KPS completed    =  %6.0f\n\n",eps[12]);
    */
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
