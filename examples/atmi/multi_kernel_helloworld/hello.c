#include <stdio.h>
#include "atmi.h"

#ifdef __cplusplus 
#define _CPPSTRING_ "C" 
#endif 
#ifndef __cplusplus 
#define _CPPSTRING_ 
#endif 
extern _CPPSTRING_ void helloWorld_cpu_1(atmi_task_t *thisTask, const char *a) __attribute__((launch_info("cpu", "helloWorld")));
extern _CPPSTRING_ void helloWorld_cpu_2(atmi_task_t *thisTask, const char *a) __attribute__((launch_info("cpu", "helloWorld")));

void helloWorld_cpu_1(atmi_task_t *thisTask, const char *a) {
    printf("In Task One: \"%s\"\n", a);
}
void helloWorld_cpu_2(atmi_task_t *thisTask, const char *a) {
    printf("In Task Two: \"%s\"\n", a);
}

int main(int argc, char *argv[]) {
    ATMI_LPARM_CPU(lparm);
    lparm->synchronous = ATMI_TRUE;
    
    lparm->kernel_id = 0; // helloWorld_cpu_1
    atmi_task_t *t = helloWorld(lparm, "Hello HSA World");
    if(!t) printf("Task 1 not executed!\n");

    lparm->kernel_id = 1; // helloWorld_cpu_2
    t = helloWorld(lparm, "Hello HSA World");
    if(!t) printf("Task 2 not executed!\n");

    return 0;
}


