#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif 

#include <sched.h>
#include <stdio.h>

#include "atl_bindthread.h"

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
