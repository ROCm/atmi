#include <stdio.h>
#include "atmi.h"


void helloWorld_gpu3(atmi_task_t *thisTask, char *a) __attribute__((launch_info("gpu", "helloWorld")));
void helloWorld_gpu4(atmi_task_t *thisTask, char *a) __attribute__((launch_info("gpu", "helloWorld1")));

void helloWorld_gpu3(atmi_task_t *thisTask, char *a) {}
void helloWorld_gpu4(atmi_task_t *thisTask, char *a) {}

int main(int argc, char *argv[]) {
    printf("In main2 program\n");
    return 0;
}


