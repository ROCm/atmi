#include <stdio.h>


void helloWorld_gpu3(char *a) __attribute__((gpu("helloWorld")));
void helloWorld_gpu4(char *a) __attribute__((gpu("helloWorld")));


int main(int argc, char *argv[]) {
    printf("In main2 program\n");
    return 0;
}


