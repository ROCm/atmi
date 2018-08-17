/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/

#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <vector>
#include <string.h>
#include "atmi.h"

using namespace std;

bool is_null_task(atmi_task_handle_t t) {
    if(t == 0ull) {
        return true;
    }
    else {
        return false;
    }
}

extern "C" void sum_cpu(int *a, int *b, int *c) __attribute__((atmi_kernel("sum", "CPU")));

extern "C" void sum_cpu(int *a, int *b, int *c) { 
    *c = *a + *b;
    delete a;
    delete b;
}

/*  Recursive Fibonacci */
void fib(const int n , int *result , atmi_task_handle_t *my_sum_task) {
    if (n < 2) { 
        *result = n; 
        *my_sum_task = NULL_TASK;
    } else {
        atmi_task_handle_t task_sum1;
        atmi_task_handle_t task_sum2; 
        int *result1 = new int;
        int *result2 = new int;
        fib(n-1,result1,&task_sum1);
        fib(n-2,result2,&task_sum2);
        ATMI_LPARM(lparm_child); 
        lparm_child->num_required = 0;
        atmi_task_handle_t requires[2];
        if (!is_null_task(task_sum1)) {
            requires[lparm_child->num_required]=task_sum1;
            lparm_child->num_required +=1;
        }
        if (!is_null_task(task_sum2)) {
            requires[lparm_child->num_required]=task_sum2;
            lparm_child->num_required +=1;
        }
        lparm_child->requires = requires;
        *my_sum_task = sum(lparm_child,result1,result2,result);
    }
}

int main(int argc, char *argv[]) {
    int N = 10;
    if(argc > 1) {
        N = atoi(argv[1]);
    }
    int result;

    atmi_task_handle_t root_sum_task;
    fib(N,&result,&root_sum_task);
    if(!is_null_task(root_sum_task)) SYNC_TASK(root_sum_task);
    cout << "Fib(" << N << ") = " << result << endl;    
    return 0;
}
