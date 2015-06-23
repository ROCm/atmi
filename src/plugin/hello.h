#include <atmi.h> 
void helloWorld_cpu3(atmi_task_t *thisTask, char *a) __attribute__((launch_info("cpu", "helloWorld")));
void helloWorld_cpu4(atmi_task_t *thisTask, char *a) __attribute__((launch_info("cpu", "helloWorld")));


