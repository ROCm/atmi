#include <stdio.h>
#include <atmi.h>

//#include "hello.c.pif.def"

extern atmi_task_t *helloWorld (atmi_lparm_t *lparm, const char *str);

void helloWorld_sub2(atmi_task_t *thisTask, const char *str) __attribute__((launch_info("CPU", "helloWorld")));

void helloWorld_sub2(atmi_task_t *thisTask, const char *str) {
    printf("In CPU Task 2: %s\n", str);
}

void helloWorld_sub1(atmi_task_t *thisTask, const char *str) __attribute__((launch_info("cpu", "helloWorld1")));

void helloWorld_sub1(atmi_task_t *thisTask, const char *str) {
    printf("In CPU Task 1: %s\n", str);
}

//void helloWorld_sub2(float *a) __attribute__((atmi_cpu_task("helloWorld")));

int main(int argc, char *argv[]) {
    printf("In main program\n");
    ATMI_LPARM_CPU(lparm);
    lparm->synchronous = ATMI_TRUE;
    helloWorld(lparm, "Hello HSA World");
    helloWorld1(lparm, "Hello HSA World Too");
    return 0;
}
