#include <string.h>
#include <stdlib.h>
#include <iostream>
using namespace std;
#include "hw.h"

size_t strlength;

extern "C" void decode(const char* in, char* out) {
	int num;
    for (num = 0; num < strlength; num++) {
        out[num] = in[num] + 1;
    }

}

int main(int argc, char* argv[]) {
	const char* input = "Gdkkn\x1FGR@\x1FVnqkc";
	strlength = strlen(input);
	char *output = (char*) malloc(strlength + 1);
    
    ATMI_LPARM_1D(lparm,strlength);
    lparm->synchronous = ATMI_TRUE;
    atmi_task_t *t = decode_pif(lparm,input,output);

    output[strlength] = '\0';
	cout << output << endl;
	free(output);
	return 0;

}
