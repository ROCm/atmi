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

#include "atmi.h"
__kernel void subTask_gpu(atmi_task_handle_t thisTask) {
return;
    ATMI_KLPARM_1D(klparm, 1, thisTask);
    klparm->kernel_id = K_ID_subTask_cpu; 
    subTask(klparm);
}

__kernel void mainTask_gpu(atmi_task_handle_t thisTask, int numTasks) {
    int gid = get_global_id(0);
    ATMI_KLPARM_1D(klparm, 1, thisTask);
    klparm->kernel_id = K_ID_subTask_gpu; 
    int i;
    for(i = 0; i < numTasks; i++)
      subTask(klparm);
}

__kernel void mainTask_recursive_gpu(atmi_task_handle_t thisTask, int numTasks) {
    int gid = get_global_id(0);
    int gsize = get_global_size(0);
    //if(gid == 0) {
    if(numTasks > 1) {
        int new_numTasks;; 
        int new_workitems;
        if(gsize >= numTasks) {
            new_numTasks = 1; 
            new_workitems = numTasks;
        }
        else {
            new_numTasks = numTasks/gsize;
            new_workitems = gsize;
        }
        ATMI_KLPARM_1D(klparm, new_workitems, thisTask);
        klparm->kernel_id = K_ID_mainTask_recursive_gpu; 
        mainTask(klparm, new_numTasks);
    }
    ATMI_KLPARM_1D(klparm_sub, 1, thisTask);
    klparm_sub->kernel_id = K_ID_subTask_gpu; 
    subTask(klparm_sub);
    //}
}

__kernel void mainTask_binary_tree_gpu(atmi_task_handle_t thisTask, int numTasks) {
    int gid = get_global_id(0);
    //if(gid == 0) {
    ATMI_KLPARM_1D(klparm, 1, thisTask);
    klparm->kernel_id = K_ID_mainTask_recursive_gpu; 
    if(numTasks > 1) {
        mainTask(klparm, numTasks/2);
        mainTask(klparm, numTasks/2 - 1);
    }
    //}
}

__kernel void mainTask_flat_gpu(atmi_task_handle_t thisTask, int numTasks) {
    if(get_global_id(0) % 64 == 0) {
        ATMI_KLPARM_1D(klparm, 1, thisTask);
        klparm->kernel_id = K_ID_subTask_gpu; 
        subTask(klparm);
    }
}
