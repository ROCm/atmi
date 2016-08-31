/*
MIT License 

Copyright © 2016 Advanced Micro Devices, Inc.  

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software
without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif 

#include <sched.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>

#include "atl_bindthread.h"

#define handle_error_en(en, msg) \
    do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

int atmi_cpu_bindthread(int cpu_index)
{
#if defined (HAVE_HWLOC)
#else
    cpu_set_t cpuset;
    int err;

    CPU_ZERO(&cpuset);
    CPU_SET(cpu_index+1, &cpuset);
    err = sched_setaffinity(0, sizeof(cpuset), &cpuset);
    if (err != 0) {
        return err;
    } else {
        DEBUG_PRINT("cpu %d bind correctly\n", cpu_index);
        return 0;
    }   
#endif
}

atmi_status_t set_thread_affinity(int id) {
    int s, j;
    cpu_set_t cpuset;
    pthread_t thread;

    thread = pthread_self();

    /* Set affinity mask to include CPUs 0 to 7 */

    CPU_ZERO(&cpuset);
    CPU_SET(id, &cpuset);

    s = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    if (s != 0)
        handle_error_en(s, "pthread_setaffinity_np");

    /* Check the actual affinity mask
     * assigned to the thread */
    s = pthread_getaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    if (s != 0)
        handle_error_en(s, "pthread_getaffinity_np");

    /*printf("Set returned by pthread_getaffinity_np() contained:\n");
    for (j = 0; j < CPU_SETSIZE; j++)
        if (CPU_ISSET(j, &cpuset))
            printf("    CPU %d\n", j);
    */
    return ATMI_STATUS_SUCCESS; 
}


