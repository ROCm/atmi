#include <stdio.h>
#include "helloKernel.h"
#include <unistd.h>

extern "C" void helloKernel(char *str);

void helloKernel(char *str) {
    printf("Executing CPU Task %s!\n", str);
}

int main(int argc, char **argv) {
    ATMI_LPARM_CPU(lparm_cpu); 
    ATMI_LPARM_1D(lparm_gpu, 64);

    atmi_tprofile_t cpu_profile;
    lparm_cpu->profile = &cpu_profile;
    atmi_tprofile_t gpu_profile;
    lparm_gpu->profile = &gpu_profile;

    char *hello = (char *)"hello";

    atmi_task_t* gpu_task = helloKernel_gpu(hello, 
                                        lparm_gpu);

    lparm_cpu->num_required = 1;
    lparm_cpu->requires = &gpu_task;
    atmi_task_t *cpu_task = helloKernel_cpu(hello, 
                                        lparm_cpu);
    //atmi_task_wait(gpu_task);
    SYNC_STREAM(0);
    //SYNC_TASK(cpu_task);
    printf("CPU Task Wait_Time: %lu ns\n", cpu_task->profile->start_time - cpu_task->profile->dispatch_time);
    printf("CPU Task Time: %lu ns\n", cpu_task->profile->end_time - cpu_task->profile->start_time);
    printf("GPU Task Time: %lu ns\n", gpu_task->profile->end_time - gpu_task->profile->start_time);
    printf("Finished.\n");
}
