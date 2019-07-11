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

    long int kcalls = 16;

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
    //lparm->kernel_id = K_ID_subTask_gpu;
    atmi_task_launch(lparm, sub_kernel, NULL);

    clock_gettime(CLOCK_MONOTONIC_RAW,&start_time);
    lparm->WORKITEMS = kcalls * 64;
    //lparm->kernel_id = K_ID_mainTask_gpu;
    void *args[] = { &kcalls };
    atmi_task_launch(lparm, main_kernel, args);
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_launch_time);
    //atmi_taskgroup_wait(stream);
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_time);
    print_timing("Synchronous Flat Execution (DP)", 
            kcalls, &start_time, 
            &end_launch_time, &end_time);

    lparm->WORKITEMS = 64;
    //lparm->kernel_id = K_ID_subTask_gpu;
    clock_gettime(CLOCK_MONOTONIC_RAW,&start_time);
    for(int i=0; i<kcalls; i++) atmi_task_launch(lparm, sub_kernel, NULL);
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_launch_time);
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_time);
    print_timing("Synchronous Task Loop (Ordered)", 
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
