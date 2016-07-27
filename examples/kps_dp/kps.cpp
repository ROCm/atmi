#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <iostream>
#include <sys/param.h> 
#include <time.h>
#include "atmi.h"
#define NSECPERSEC 1000000000L

void print_timing(const char *title, 
        int kcalls,
        struct timespec *start_time, 
        struct timespec *end_launch_time, 
        struct timespec *end_time);

__kernel void subTask_gpu(atmi_task_handle_t thisTask) __attribute__((atmi_kernel("subTask", "gpu")));

__kernel void mainTask_gpu(atmi_task_handle_t thisTask, int numTasks) __attribute__((atmi_kernel("mainTask", "GPU")));

__kernel void mainTask_recursive_gpu(atmi_task_handle_t thisTask, int numTasks) __attribute__((atmi_kernel("mainTask", "GPU")));

__kernel void mainTask_binary_tree_gpu(atmi_task_handle_t thisTask, int numTasks) __attribute__((atmi_kernel("mainTask", "GPU")));

__kernel void mainTask_4ary_tree_gpu(atmi_task_handle_t thisTask, int numTasks) __attribute__((atmi_kernel("mainTask", "GPU")));


int main(int argc, char *argv[]) {
    struct timespec start_time;
    struct timespec end_time;
    struct timespec end_launch_time;
    long int nanosecs;
    float kps;

    long int kcalls_dp = 10240*4;
    long int kcalls = 10240*4;

    ATMI_LPARM_STREAM(lparm,stream);
    lparm->groupable=ATMI_TRUE;
    lparm->synchronous=ATMI_TRUE;
    stream->ordered=ATMI_TRUE;

    lparm->WORKITEMS=1;
    lparm->kernel_id = K_ID_mainTask_gpu; 
    mainTask(lparm, 1);

    lparm->WORKITEMS=1024;
    lparm->kernel_id = K_ID_mainTask_gpu; 
    clock_gettime(CLOCK_MONOTONIC_RAW,&start_time);
    mainTask(lparm, kcalls_dp/lparm->WORKITEMS); 
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_launch_time);
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_time);
    printf("Kernel Calls     =  %ld\n",kcalls);
    print_timing("Synchronous Execution (DP)", 
            kcalls, &start_time, 
            &end_launch_time, &end_time);

    lparm->kernel_id = K_ID_mainTask_recursive_gpu; 
    lparm->synchronous=ATMI_FALSE;
    stream->ordered = ATMI_FALSE;
    clock_gettime(CLOCK_MONOTONIC_RAW,&start_time);
    mainTask(lparm, kcalls_dp/lparm->WORKITEMS); 
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_launch_time);
    atmi_task_group_sync(stream);
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_time);
    print_timing("Asynchronous Recursive Execution (DP)", 
            kcalls_dp, &start_time, 
            &end_launch_time, &end_time);

    lparm->kernel_id = K_ID_mainTask_binary_tree_gpu; 
    lparm->synchronous=ATMI_FALSE;
    stream->ordered = ATMI_FALSE;
    clock_gettime(CLOCK_MONOTONIC_RAW,&start_time);
    mainTask(lparm, (kcalls_dp/lparm->WORKITEMS)/2); 
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_launch_time);
    atmi_task_group_sync(stream);
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_time);
    print_timing("Asynchronous Binary Tree Execution (DP)", 
            kcalls_dp, &start_time, 
            &end_launch_time, &end_time);

    /*lparm->kernel_id = K_ID_mainTask_4ary_tree_gpu; 
      lparm->synchronous=ATMI_FALSE;
      stream->ordered = ATMI_FALSE;
      clock_gettime(CLOCK_MONOTONIC_RAW,&start_time);
      mainTask(lparm, (kcalls_dp/lparm->WORKITEMS)/4); 
      clock_gettime(CLOCK_MONOTONIC_RAW,&end_launch_time);
      atmi_task_group_sync(stream);
      clock_gettime(CLOCK_MONOTONIC_RAW,&end_time);
      print_timing("Asynchronous 4-ary Tree Execution (DP)", 
      kcalls_dp, &start_time, 
      &end_launch_time, &end_time);
      */
    lparm->synchronous=ATMI_FALSE;
    stream->ordered=ATMI_TRUE;
    lparm->kernel_id = K_ID_subTask_gpu; 
    clock_gettime(CLOCK_MONOTONIC_RAW,&start_time);
    for(int i=0; i<kcalls; i++) subTask(lparm); 
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_launch_time);
    //SYNC_STREAM(stream1); 
    atmi_task_group_sync(stream);
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_time);
    print_timing("Asynchronous Tasks (Ordered)", 
            kcalls, &start_time, 
            &end_launch_time, &end_time);

    lparm->synchronous = ATMI_FALSE;
    stream->ordered = ATMI_FALSE;
    clock_gettime(CLOCK_MONOTONIC_RAW,&start_time);
    for(int i=0; i<kcalls; i++) subTask(lparm); 
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_launch_time);
    //SYNC_STREAM(stream1); 
    atmi_task_group_sync(stream);
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_time);
    print_timing("Asynchronous Tasks (Unordered)", 
            kcalls, &start_time, 
            &end_launch_time, &end_time);
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

void print_timing(const char *title, 
        int kcalls,
        struct timespec *start_time, 
        struct timespec *end_launch_time, 
        struct timespec *end_time) {
    long int nanosecs;
    float kps;
    printf("%s\n", title);
    nanosecs = get_nanosecs(*start_time,*end_launch_time);
    kps = ((float) kcalls * (float) NSECPERSEC) / (float) nanosecs ;
    printf("Secs to dispatch =  %10.8f\n",(float)nanosecs/NSECPERSEC);
    printf("KPS dispatched   =  %6.0f\n",kps);
    nanosecs = get_nanosecs(*start_time,*end_time);
    kps = ((float) kcalls * (float) NSECPERSEC) / (float) nanosecs ;
    printf("Secs to complete =  %10.8f\n",(float)nanosecs/NSECPERSEC);
    printf("KPS completed    =  %6.0f\n\n",kps);
}
