#include <iostream>
#include "fibonacci.h"
using namespace std;

atmi_stream_t stream;

extern "C" void fibonacci(int n, int *result, atmi_task_t **ret_task) {
    if(n < 2) {
        __atomic_add_fetch(result, n, __ATOMIC_ACQ_REL);
        return;
    }

    ATMI_LPARM_CPU(lparm);
    lparm->stream = &stream;
    fibonacci_cpu(n-1, result, lparm);
    fibonacci_cpu(n-2, result, lparm);
    return;
}


int main() {
    const int N = 5;
    int result = 0; // initialize to 0
    ATMI_LPARM_CPU(lparm);
    stream.ordered = ATMI_FALSE;
    lparm->stream = &stream;
    atmi_task_t *t = fibonacci_cpu(N, &result, lparm);
    //SYNC_TASK(t);
    atmi_task_wait(t);
    cout << "Fib(" << N << ") = " << result << endl;    
    return 0;
}
