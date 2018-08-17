/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/

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

// Declare decode as the PIF for the GPU kernel decode_gpu
extern "C" void subTask_cpu(atmi_task_handle_t thisTask) __attribute__((atmi_kernel("subTask", "cpu")));

__kernel void mainTask_gpu(atmi_task_handle_t thisTask, int numTasks) __attribute__((atmi_kernel("mainTask", "GPU")));

__kernel void subTask_gpu(atmi_task_handle_t thisTask) __attribute__((atmi_kernel("subTask", "gpu")));


static int count = 0;
extern "C" void subTask_cpu(atmi_task_handle_t thisTask) {
    //static int count = 0;
    count++;
    //printf("Counter: %d\n", count++);
}

__kernel void mainTask_recursive_gpu(atmi_task_handle_t thisTask, int numTasks) __attribute__((atmi_kernel("mainTask", "GPU")));

__kernel void mainTask_binary_tree_gpu(atmi_task_handle_t thisTask, int numTasks) __attribute__((atmi_kernel("mainTask", "GPU")));

__kernel void mainTask_flat_gpu(atmi_task_handle_t thisTask, int numTasks) __attribute__((atmi_kernel("mainTask", "GPU")));

int main(int argc, char *argv[]) {
    struct timespec start_time;
    struct timespec end_time;
    struct timespec end_launch_time;
    long int nanosecs;
    float kps;

    long int kcalls = 64*64*16;

    ATMI_LPARM_STREAM(lparm, stream);
    stream->ordered = ATMI_FALSE;
    lparm->groupable=ATMI_TRUE;

    lparm->WORKITEMS=1;
    lparm->kernel_id = K_ID_subTask_gpu; 
    subTask(lparm);

    lparm->kernel_id = K_ID_mainTask_flat_gpu; 
    lparm->WORKITEMS = kcalls * 64;
    lparm->synchronous=ATMI_TRUE;
    clock_gettime(CLOCK_MONOTONIC_RAW,&start_time);
    mainTask(lparm, kcalls); 
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_launch_time);
    //atmi_task_group_sync(stream);
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_time);
    print_timing("Synchronous Flat Execution (DP)", 
            kcalls, &start_time, 
            &end_launch_time, &end_time);


    lparm->WORKITEMS=64;
    lparm->kernel_id = K_ID_mainTask_gpu; 
    lparm->synchronous=ATMI_TRUE;
    clock_gettime(CLOCK_MONOTONIC_RAW,&start_time);
    mainTask(lparm, kcalls/lparm->WORKITEMS); 
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_launch_time);
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_time);
    printf("Kernel Calls     =  %ld\n",kcalls);
    print_timing("Synchronous Execution (DP)", 
            kcalls, &start_time, 
            &end_launch_time, &end_time);
   
    lparm->kernel_id = K_ID_mainTask_gpu; 
    lparm->synchronous=ATMI_FALSE;
    clock_gettime(CLOCK_MONOTONIC_RAW,&start_time);
    mainTask(lparm, kcalls/lparm->WORKITEMS); 
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_launch_time);
    atmi_task_group_sync(stream);
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_time);
    print_timing("Asynchronous Execution (DP)", 
            kcalls, &start_time, 
            &end_launch_time, &end_time);

    lparm->kernel_id = K_ID_mainTask_recursive_gpu; 
    lparm->synchronous=ATMI_TRUE;
    clock_gettime(CLOCK_MONOTONIC_RAW,&start_time);
    mainTask(lparm, kcalls/lparm->WORKITEMS); 
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_launch_time);
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_time);
    print_timing("Synchronous Recursive Execution (DP)", 
            kcalls, &start_time, 
            &end_launch_time, &end_time);
/*
    lparm->kernel_id = K_ID_mainTask_binary_tree_gpu; 
    lparm->synchronous=ATMI_FALSE;
    clock_gettime(CLOCK_MONOTONIC_RAW,&start_time);
    mainTask(lparm, (kcalls/lparm->WORKITEMS)); 
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_launch_time);
    atmi_task_group_sync(stream);
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_time);
    print_timing("Asynchronous Binary Tree Execution (DP)", 
            kcalls, &start_time, 
            &end_launch_time, &end_time);
*/
    /*lparm->kernel_id = K_ID_mainTask_4ary_tree_gpu; 
      lparm->synchronous=ATMI_FALSE;
      clock_gettime(CLOCK_MONOTONIC_RAW,&start_time);
      mainTask(lparm, (kcalls/lparm->WORKITEMS)/4); 
      clock_gettime(CLOCK_MONOTONIC_RAW,&end_launch_time);
      atmi_task_group_sync(stream);
      clock_gettime(CLOCK_MONOTONIC_RAW,&end_time);
      print_timing("Asynchronous 4-ary Tree Execution (DP)", 
      kcalls, &start_time, 
      &end_launch_time, &end_time);
      */
    lparm->synchronous=ATMI_TRUE;
    lparm->WORKITEMS = 64;
    lparm->kernel_id = K_ID_subTask_gpu; 
    clock_gettime(CLOCK_MONOTONIC_RAW,&start_time);
    for(int i=0; i<kcalls; i++) subTask(lparm); 
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_launch_time);
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_time);
    print_timing("Synchronous Task Loop (Ordered)", 
            kcalls, &start_time, 
            &end_launch_time, &end_time);

    lparm->synchronous=ATMI_FALSE;
    lparm->WORKITEMS = 64;
    lparm->kernel_id = K_ID_subTask_gpu; 
    clock_gettime(CLOCK_MONOTONIC_RAW,&start_time);
    for(int i=0; i<kcalls; i++) subTask(lparm); 
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_launch_time);
    atmi_task_group_sync(stream);
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_time);
    print_timing("Asynchronous Task Loop (Ordered)", 
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
    printf("Count now: %d\n", count);
    nanosecs = get_nanosecs(*start_time,*end_launch_time);
    kps = ((float) kcalls * (float) NSECPERSEC) / (float) nanosecs ;
    printf("Secs to dispatch =  %10.8f\n",(float)nanosecs/NSECPERSEC);
    printf("KPS dispatched   =  %6.0f\n",kps);
    nanosecs = get_nanosecs(*start_time,*end_time);
    kps = ((float) kcalls * (float) NSECPERSEC) / (float) nanosecs ;
    printf("Secs to complete =  %10.8f\n",(float)nanosecs/NSECPERSEC);
    printf("KPS completed    =  %6.0f\n\n",kps);
}
