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

#include "atmi_runtime.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
using namespace std; 
#ifdef __cplusplus 
#define _CPPSTRING_ "C" 
#endif 
#ifndef __cplusplus 
#define _CPPSTRING_ 
#endif 

#define NSECPERSEC 1000000000L
#define NTIMERS 13
long int get_nanosecs( struct timespec start_time, struct timespec end_time) ;

int main(int argc, char **argv) {
    atmi_init(ATMI_DEVTYPE_ALL);

    int gpu_id = 0;
    int cpu_id = 0;
    atmi_machine_t *machine = atmi_machine_get_info();
    int gpu_count = machine->device_count_by_type[ATMI_DEVTYPE_GPU];
    if(argv[1] != NULL) gpu_id = (atoi(argv[1]) % gpu_count);
    printf("Choosing GPU %d/%d\n", gpu_id, gpu_count);

    struct timespec start_time[NTIMERS],end_time[NTIMERS];
    long int kcalls, nanosecs[NTIMERS];
    float bw[NTIMERS];
    kcalls = 100;

    /* Run HelloWorld on GPU */
    atmi_mem_place_t gpu = ATMI_MEM_PLACE(ATMI_DEVTYPE_GPU, gpu_id, 0);
    atmi_mem_place_t cpu = ATMI_MEM_PLACE(ATMI_DEVTYPE_CPU, cpu_id, 0);
    void *d_input, *d_output;
    atmi_task_group_t group;
    group.id = 0;
    group.ordered = ATMI_FALSE; 

    ATMI_CPARM(cparm);
    cparm->groupable = ATMI_TRUE;
    cparm->group = &group;

    printf("Size (MB)\t");
#ifdef BIBW
    printf("Bi-dir BW(MBps)\n");
#else
    printf("H2D BW(MBps)\tD2H BW(MBps)\n");
#endif
    const long MB = 1024 * 1024;
    for(long size = 1*MB; size <= 1024*MB; size *= 2) {
        clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[0]);
        atmi_malloc(&d_input, size, cpu);
        atmi_malloc(&d_output, size, gpu);
        /* touch */
        memset(d_input, 0, size);
        atmi_memcpy(d_output, d_input, size);
        clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[0]);

        clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[1]);
        clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[2]);
        for(int i=0; i<kcalls; i++) {
            atmi_memcpy_async(cparm, d_output, d_input, size);
        }
        // wait for all tasks to complete
#ifndef BIBW        
        atmi_task_group_sync(&group);
#endif        
        clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[2]);

        clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[3]);
        for(int i=0; i<kcalls; i++) {
            atmi_memcpy_async(cparm, d_input, d_output, size);
        }
        // wait for all tasks to complete
        atmi_task_group_sync(&group);
        clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[3]);
        clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[1]);

        for(int i=0; i<NTIMERS; i++) {
            nanosecs[i] = get_nanosecs(start_time[i],end_time[i]);
            bw[i] = ((float) kcalls * (size/MB) * (float) NSECPERSEC) / (float) nanosecs[i];
        }

        printf("%lu\t\t", size/MB);
#ifdef BIBW
        printf("%.0f\n",2*bw[1]);
#else        
        printf("%.0f\t\t",bw[2]);
        printf("%.0f\n",bw[3]);
#endif        

        /* cleanup */
        atmi_free(d_input);
        atmi_free(d_output);
    }

    atmi_finalize();
    return 0;
}
