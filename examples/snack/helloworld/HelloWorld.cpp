#include <string.h>
#include <stdlib.h>
#include <iostream>
using namespace std;
#include "hw.h"
int main(int argc, char* argv[]) {
	const char* input = "Gdkkn\x1FGR@\x1FVnqkc";
	size_t strlength = strlen(input);
	char *output = (char*) malloc(strlength + 1);
        Launch_params_t lparm={.ndim=1, .gdims={strlength}, .ldims={256}};
        decode(input,output,lparm);
	output[strlength] = '\0';
	cout << output << endl;
	free(output);
	return 0;

}
