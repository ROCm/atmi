#include <stdio.h>


const char helloWorld1(volatile float *a, int b,
            float c) __attribute__((cpu("helloWorld")));
void helloWorld2(float *a) __attribute__((cpu("helloWorld")));

int main(int argc, char *argv[]) {
    printf("In main program\n");
    return 0;
}
