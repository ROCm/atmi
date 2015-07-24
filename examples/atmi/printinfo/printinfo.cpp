#include <string.h>
#include <stdlib.h>
#include <iostream>
using namespace std;
#include "atmi.h"

extern "C" void atmi_print_info(atmi_context_t * context) {
   cout<< "ATMI System Info:" << endl;
}

/* Declare the nk kernel that will call the "nullKernelPIF" PIF */
extern "C" void nk(atmi_task_t*thisTask) __attribute__((atmi_kernel("nullKernelPIF", "cpu")));

/* Define a null CPU kernel */
extern "C" void nk(atmi_task_t*thisTask) {} ;

int main(int argc, char* argv[]) {
    ATMI_LPARM(lparm);
    lparm->synchronous = ATMI_TRUE;
    atmi_task_t *nullTask = nullKernelPIF(lparm); 
    atmi_print_info(nullTask->context);
}

