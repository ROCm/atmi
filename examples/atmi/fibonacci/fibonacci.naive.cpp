#include <iostream>
#include "fibonacci.h"
using namespace std;

atmi_stream_t stream;

extern "C" void sum(int a, int b, int *c) {
    *c = a + b;
}

extern "C" void fibonacci(int n, int *result) {
    if(n == 1 || n == 2) {
        *result = n;
        return;
    }

    int temp_result1, temp_result2;
    ATMI_LPARM_CPU(lparm);
    lparm->stream = &stream;
    atmi_task_t *t1 = fibonacci_cpu(n-1, &temp_result1, lparm);
    atmi_task_t *t2 = fibonacci_cpu(n-2, &temp_result2, lparm);

    atmi_task_wait(t1);
    atmi_task_wait(t2);
    *result = temp_result1 + temp_result2;
    return;
}


int main() {
    const int N = 5;
    int result;
    ATMI_LPARM_CPU(lparm);
    stream.ordered = ATMI_FALSE;
    lparm->stream = &stream;
    atmi_task_t *t = fibonacci_cpu(N, &result, lparm);
    //SYNC_TASK(t);
    atmi_task_wait(t);
    //SYNC_STREAM(&stream);
    cout << "Fib(" << N << ") = " << result << endl;    
    return 0;
}
