#include <iostream>
#include <stdlib.h>
#include <vector>
#include "atmi.h"

using namespace std;

std::vector<atmi_task_t *> tasks;
extern "C" void sum_cpu(atmi_task_t *t, int *a, int *b, int *c) __attribute__((atmi_kernel("sum", "CPU")));

extern "C" void sum_cpu(atmi_task_t *t, int *a, int *b, int *c) { 
    *c = *a + *b;
    delete a;
    delete b;
}

/*  Recursive Fibonacci */
void fib(const int n , int *result , atmi_task_t **my_sum_task) {
    if (n < 2) { 
        *result = n; 
        *my_sum_task = NULL;
    } else {
        atmi_task_t *task_sum1 = NULL;
        atmi_task_t *task_sum2 = NULL; 
        int *result1 = new int;
        int *result2 = new int;
        fib(n-1,result1,&task_sum1);
        fib(n-2,result2,&task_sum2);
        ATMI_LPARM(lparm_child); /* Remember ATMI default is asynchronous execution */
        lparm_child->num_required = 0;
        atmi_task_t *requires[2];
        if (task_sum1 != NULL) {
            requires[lparm_child->num_required]=task_sum1;
            lparm_child->num_required +=1;
        }
        if (task_sum2 != NULL) {
            requires[lparm_child->num_required]=task_sum2;
            lparm_child->num_required +=1;
        }
        lparm_child->requires = requires;
        *my_sum_task = new atmi_task_t;
        tasks.push_back(*my_sum_task);
        lparm_child->task = *my_sum_task;
        sum(lparm_child,result1,result2,result);
    }
}

int main(int argc, char *argv[]) {
    int N = 10;
    if(argc > 1) {
        N = atoi(argv[1]);
    }
    int result;

    atmi_task_t *root_sum_task = NULL;
    fib(N,&result,&root_sum_task);
    SYNC_TASK(root_sum_task);
    cout << "Fib(" << N << ") = " << result << endl;    
    for(std::vector<atmi_task_t *>::iterator it = tasks.begin(); 
           it != tasks.end(); it++) {
        delete *it;
    }
    tasks.clear();
    return 0;
}
