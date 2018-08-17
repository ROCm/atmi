/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/

#include <iostream>
#include <stdlib.h>
#include <vector>
#include "atmi_runtime.h"
#include <time.h>
using namespace std;

#ifdef __cplusplus 
#define _CPPSTRING_ "C" 
#endif 
#ifndef __cplusplus 
#define _CPPSTRING_ 
#endif 

enum {
    CPU_IMPL = 10565
};

atmi_kernel_t fib_kernel;
atmi_kernel_t sum_kernel;

extern _CPPSTRING_ void sum_cpu(int **a_ref, int **b_ref, int **c_ref) {
    int *a = *a_ref;
    int *b = *b_ref;
    int *c = *c_ref;
    if(a && b && c) {
        *c = *a + *b;
        delete a;
        delete b;
    }
}

/*  Recursive Fibonacci */
extern _CPPSTRING_ void fib_cpu(atmi_task_handle_t *sum_task_ref, 
                                const int *n_ref, int **result_ref) {
    atmi_task_handle_t sum_task = *sum_task_ref;
    const int n = *n_ref;
    int *result = *result_ref;
    if (n < 5) {
        //*result = n;
        int fib_array[3];
        int i = 0;
        if(n >= i) fib_array[i] = 0; i++;
        if(n >= i) fib_array[i] = 1; i++;
        for(; i <= n; i++) {
            fib_array[i % 3] = fib_array[(i - 1) % 3] + fib_array[(i - 2) % 3]; 
        }
        //printf("I: %d, n: %d\n", i-1, n);
        *result = fib_array[(i-1) % 3];
        ATMI_LPARM(lp);
        lp->kernel_id = CPU_IMPL;
        lp->place = ATMI_PLACE_CPU(0, 0);
        int *null_arg = NULL;
        void *sum_args[] = { &null_arg, &null_arg, &null_arg };
        atmi_task_activate(sum_task, lp, sum_args);
        //atmi_task_activate(sum_task, NULL, NULL);
    } else {
        ATMI_LPARM(lp1);
        lp1->kernel_id = CPU_IMPL;
        lp1->place = ATMI_PLACE_CPU(0, 0);
        int *result1 = new int;
        atmi_task_handle_t task_sum1 = atmi_task_create(sum_kernel);
        int n1 = n - 1;
        void *fib_args1[] = { &task_sum1, &n1, &result1 };
        ATMI_LPARM(lp2);
        lp2->kernel_id = CPU_IMPL;
        lp2->place = ATMI_PLACE_CPU(0, 0);
        int *result2 = new int;
        atmi_task_handle_t task_sum2 = atmi_task_create(sum_kernel);
        int n2 = n - 2;
        void *fib_args2[] = { &task_sum2, &n2, &result2 };

        atmi_task_launch(lp1, fib_kernel, fib_args1);
        atmi_task_launch(lp2, fib_kernel, fib_args2);

        ATMI_LPARM(lparm_child);
        lparm_child->kernel_id = CPU_IMPL;
        lparm_child->place = ATMI_PLACE_CPU(0, 0);
        ATMI_PARM_SET_DEPENDENCIES(lparm_child, task_sum1, task_sum2);

        void *sum_args[] = { &result1, &result2, &result };
        atmi_task_activate(sum_task, lparm_child, sum_args);
    }
}

int main(int argc, char *argv[]) {
    atmi_status_t err = atmi_init(ATMI_DEVTYPE_ALL);
    if(err != ATMI_STATUS_SUCCESS) return -1;
    int N = 10;
    if(argc > 1) {
        N = atoi(argv[1]);
    }
    if(N < 1) {
        fprintf(stderr, "N should be >= 0\n");
        exit(-1);
    }
    int *result = new int;

    size_t fib_arg_sizes[] = { sizeof(atmi_task_handle_t), sizeof(const int), sizeof(int *)};
    const int fib_num_args = sizeof(fib_arg_sizes)/sizeof(fib_arg_sizes[0]);
    atmi_kernel_create_empty(&fib_kernel, fib_num_args, fib_arg_sizes);
    atmi_kernel_add_cpu_impl(fib_kernel, (atmi_generic_fp)fib_cpu, CPU_IMPL);

    size_t sum_arg_sizes[] = { sizeof(int *), sizeof(int *), sizeof(int *)};
    const int sum_num_args = sizeof(sum_arg_sizes)/sizeof(sum_arg_sizes[0]);
    atmi_kernel_create_empty(&sum_kernel, sum_num_args, sum_arg_sizes);
    atmi_kernel_add_cpu_impl(sum_kernel, (atmi_generic_fp)sum_cpu, CPU_IMPL);

    atmi_task_handle_t root_sum_task_continuation = atmi_task_create(sum_kernel);
    void *fib_args[fib_num_args] = { &root_sum_task_continuation, &N, &result };

    // Time how long it takes to calculate the nth Fibonacci number
    clock_t start = clock();
#if 1
    ATMI_LPARM(lp);
    lp->kernel_id = CPU_IMPL;
    lp->place = ATMI_PLACE_CPU(0, 0);
    atmi_task_handle_t root_sum_task = atmi_task_launch(lp, fib_kernel, fib_args);
    atmi_task_wait(root_sum_task_continuation);
#else
    int fib_array[3];
    int i = 0;
    if(N >= i) fib_array[i] = 0; i++;
    if(N >= i) fib_array[i] = 1; i++;
    for(; i <= N; i++) {
        fib_array[i % 3] = fib_array[(i - 1) % 3] + fib_array[(i - 2) % 3]; 
    }
    //printf("I: %d, n: %d\n", i-1, n);
    *result = fib_array[(i-1) % 3];
#endif
    clock_t end = clock();

    // Display our results
    double duration = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Fibonacci number #%d is %d.\n", N, *result);
    printf("Calculated in %.3f seconds.\n",
            duration);

    delete result;
    return 0;
}

