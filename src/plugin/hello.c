#include <stdio.h>
#include "atmi.h"

#include "hello.h.pifdefs.h"

void helloWorld_cpu3(atmi_task_t *thisTask, char *a) {}
void helloWorld_cpu4(atmi_task_t *thisTask, char *a) {}

int main(int argc, char *argv[]) {
    printf("In main2 program\n");

    ATMI_LPARM_CPU(lparm);
    lparm->synchronous = ATMI_TRUE;
    lparm->kernel_id = 0; // helloWorld_cpu3
    helloWorld(lparm, "Hello HSA World");
    lparm->kernel_id = 1; // helloWorld_cpu4
    helloWorld(lparm, "Hello HSA World");

    return 0;
}


