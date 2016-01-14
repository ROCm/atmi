#include <iostream>
#include <stdlib.h>
#include <vector>
#include "atmi.h"
using namespace std;

std::vector<atmi_task_t *> tasks;

pthread_mutex_t g_mutex;
extern "C" void sum_cpu(atmi_task_t *t, int *a, int *b, int *c) __attribute__((atmi_kernel("sum", "CPU")));

extern "C" void fib_cpu(atmi_task_t *t, const int n , int *result) __attribute__((atmi_kernel("fib", "CPU")));

extern "C" void sum_cpu(atmi_task_t *t, int *a, int *b, int *c) { 
    if(a && b && c) {
        *c = *a + *b;
        delete a;
        delete b;
    }
}

/*  Recursive Fibonacci */
extern "C" void fib_cpu(atmi_task_t *t, const int n , int *result) {
    if (n < 2) { 
        *result = n;
        ATMI_LPARM_WITH_TASK(lp, t->continuation);
        sum(lp, NULL, NULL, NULL);
    } else {
        ATMI_TASK_INIT_WITH_CONTINUATION(task_sum1);
        ATMI_TASK_INIT_WITH_CONTINUATION(task_sum2);
        pthread_mutex_lock(&g_mutex);
        tasks.push_back(task_sum1);
        tasks.push_back(task_sum2);
        pthread_mutex_unlock(&g_mutex);
        ATMI_LPARM_WITH_TASK(lp1, task_sum1);
        ATMI_LPARM_WITH_TASK(lp2, task_sum2);
        int *result1 = new int;
        int *result2 = new int;
        fib(lp1, n-1, result1);
        fib(lp2, n-2, result2);
        ATMI_LPARM_WITH_TASK(lparm_child, t->continuation); /* Remember ATMI default is asynchronous execution */
        lparm_child->num_required = 2;
        atmi_task_t *requires[2];
        requires[0]=task_sum1->continuation;
        requires[1]=task_sum2->continuation;
        lparm_child->requires = requires;
        sum(lparm_child,result1,result2,result);
    }
}

int main(int argc, char *argv[]) {
    int N = 10;
    if(argc > 1) {
        N = atoi(argv[1]);
    }
    int result;
    
    pthread_mutex_init(&g_mutex, NULL);
    ATMI_TASK_INIT_WITH_CONTINUATION(root_sum_task);
    pthread_mutex_lock(&g_mutex);
    tasks.push_back(root_sum_task);
    pthread_mutex_unlock(&g_mutex);
    ATMI_LPARM_WITH_TASK(lp, root_sum_task);
    fib(lp, N, &result);
    SYNC_TASK(root_sum_task->continuation);

    for(std::vector<atmi_task_t *>::iterator it = tasks.begin();
            it != tasks.end(); it++) {
        ATMI_TASK_FINALIZE_WITH_CONTINUATION(*it);
    }
    tasks.clear();
    pthread_mutex_destroy(&g_mutex);
    cout << "Fib(" << N << ") = " << result << endl;    
    return 0;
}
