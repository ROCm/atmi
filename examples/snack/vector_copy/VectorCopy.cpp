#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <libelf.h>
#include <iostream>
#include "vector_copy.h"

int main(int argc, char **argv)
{

	//Setup kernel arguments
	int* in=(int*)malloc(1024*1024*4);
	int* out=(int*)malloc(1024*1024*4);
	memset(out, 0, 1024*1024*4);
	memset(in, 1, 1024*1024*4);

        SNK_INIT_LPARM(lparm,1024*1024);
        vcopy(out,in,lparm);

	//Validate
	bool valid=true;
	int failIndex=0;
	for(int i=0; i<1024*1024; i++) {
		if(out[i]!=in[i]) {
			failIndex=i;
			valid=false;
			break;
		}
	}
	if(valid)
		printf("passed validation\n");
	else 
		printf("VALIDATION FAILED!\nBad index: %d\n", failIndex);


	free(in);
	free(out);

    return 0;
}

