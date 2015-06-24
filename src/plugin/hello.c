#include <stdio.h>
#include "atmi.h"

#ifdef __cplusplus 
#define _CPPSTRING_ "C" 
#endif 
#ifndef __cplusplus 
#define _CPPSTRING_ 
#endif 
//extern _CPPSTRING_ void helloWorld_cpu3(atmi_task_t *thisTask, char *a) __attribute__((launch_info("cpu", "helloWorld")));
extern _CPPSTRING_ void helloWorld_cpu4(atmi_task_t *thisTask, char *a) __attribute__((launch_info("cpu", "helloWorld")));

void helloWorld_cpu3(atmi_task_t *thisTask, char *a) {
    printf("In Task Three: %s\n", a);
}
void helloWorld_cpu4(atmi_task_t *thisTask, char *a) {
    printf("In Task Four: %s\n", a);
}

int main(int argc, char *argv[]) {
    printf("In main2 program\n");

    ATMI_LPARM_CPU(lparm);
    lparm->synchronous = ATMI_TRUE;
    lparm->kernel_id = 0; // helloWorld_cpu3
    atmi_task_t *t = helloWorld(lparm, "Hello HSA World");
    if(!t) printf("Task not executed!\n");
    lparm->kernel_id = 1; // helloWorld_cpu4
    t = helloWorld(lparm, "Hello HSA World");
    if(!t) printf("Task not executed!\n");

    return 0;
}


