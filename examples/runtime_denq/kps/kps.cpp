/*
 * MIT License
 *
 * Copyright © 2016 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software
 * without restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies of the
 * Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 * */

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <iostream>
#include <sys/param.h> 
#include <time.h>
#include "atmi_runtime.h"
#define NSECPERSEC 1000000000L

void print_timing(const char *title, 
        int kcalls,
        struct timespec *start_time, 
        struct timespec *end_launch_time, 
        struct timespec *end_time);

static int count = 0;
extern "C" void subTask_cpu() {
    //static int count = 0;
    printf("Counter: %d\n", count++);
}

enum {
    K_ID_mainTask_gpu = 0,
    K_ID_mainTask_recursive_gpu,
    K_ID_mainTask_binary_tree_gpu,
    K_ID_mainTask_flat_gpu
};

enum {
    K_ID_subTask_gpu = 0,
    K_ID_subTask_cpu
};

int main(int argc, char *argv[]) {
    struct timespec start_time;
    struct timespec end_time;
    struct timespec end_launch_time;
    long int nanosecs;
    float kps;

    long int kcalls = 1024 * 128;

    atmi_status_t err = atmi_init(ATMI_DEVTYPE_ALL);
    if(err != ATMI_STATUS_SUCCESS) return -1;
    const char *module = "nullKernel.hsaco";
    atmi_platform_type_t module_type = AMDGCN;
    err = atmi_module_register(&module, &module_type, 1);

    atmi_kernel_t main_kernel, sub_kernel;
    const unsigned int main_num_args = 1;
    size_t main_arg_sizes[] = { sizeof(long int) };
    atmi_kernel_create(&main_kernel, main_num_args, main_arg_sizes, 
                       1, 
                       ATMI_DEVTYPE_GPU, "mainTask_gpu"
                       //ATMI_DEVTYPE_GPU, "mainTask_recursive_gpu",
                       //ATMI_DEVTYPE_GPU, "mainTask_binary_tree_gpu",
                       //ATMI_DEVTYPE_GPU, "mainTask_flat_gpu"
                       );
    atmi_kernel_create(&sub_kernel, 0, NULL,
                       2,
                       ATMI_DEVTYPE_GPU, "subTask_gpu",
                       ATMI_DEVTYPE_CPU, (atmi_generic_fp)subTask_cpu);


    ATMI_LPARM_1D(lparm, 1);
    //lparm->WORKITEMS = numTasks;
    lparm->groupDim[0] = 64;
    lparm->synchronous = ATMI_TRUE;
    lparm->place = ATMI_PLACE_GPU(0, 0);
    lparm->groupable = ATMI_TRUE;
    lparm->kernel_id = K_ID_subTask_gpu; 
    atmi_task_launch(lparm, sub_kernel, NULL);

    clock_gettime(CLOCK_MONOTONIC_RAW,&start_time);
    lparm->WORKITEMS = 512;//kcalls * 64;
    lparm->kernel_id = K_ID_mainTask_gpu;
    void *args[] = { &kcalls };
    atmi_task_launch(lparm, main_kernel, args);
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_launch_time);
    //atmi_task_group_sync(stream);
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_time);
    print_timing("Synchronous Flat Execution (DP)", 
            kcalls, &start_time, 
            &end_launch_time, &end_time);

/*

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

    lparm->WORKITEMS = 64;
    lparm->kernel_id = K_ID_subTask_gpu; 
    clock_gettime(CLOCK_MONOTONIC_RAW,&start_time);
    for(int i=0; i<kcalls; i++) atmi_task_launch(lparm, sub_kernel, NULL);
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_launch_time);
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_time);
    print_timing("Synchronous Task Loop (Ordered)", 
            kcalls, &start_time, 
            &end_launch_time, &end_time);
/*
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
*/

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
