#include <string.h>
#include <stdlib.h>
#include <iostream>
using namespace std;
#include "hw.h"
int main(int argc, char* argv[]) {
	const char* input = "Gdkkn\x1FGR@\x1FVnqkc";
	size_t strlength = strlen(input);
	char *output = (char*) malloc(strlength + 1);
        SNK_INIT_LPARM(lparm,strlength);
        decode(input,output,lparm);
	output[strlength] = '\0';
	cout << output << endl;
	free(output);
	return 0;

}
