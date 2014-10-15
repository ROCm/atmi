CLOC - Version 0.7.2
====================

CLOC: Convert an CL (Kernel c Language) file to brig, hsail, or object

Table of contents
-----------------

- [Copyright and Disclaimer](#Copyright)
- [Software License Agreement](LICENSE.TXT)
- [Command Help](#CommandHelp)
- [Examples](#ReadmeExamples)
- [Install](INSTALL.md)

<A NAME="Copyright">
# Copyright and Disclaimer
------------------------

Copyright 2014 ADVANCED MICRO DEVICES, INC.  

AMD is granting you permission to use this software (the Materials) pursuant to the 
terms and conditions of the Software License Agreement included with the Materials.  
If you do not have a copy of the Software License Agreement, contact your AMD 
representative for a copy or refer to 

  http://github.com/HSAFoundation/CLOC/blob/master/LICENSE.TXT

You agree that you will not reverse engineer or decompile the Materials, in whole or 
in part, except as allowed by applicable law.

WARRANTY DISCLAIMER: THE SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY 
KIND.  AMD DISCLAIMS ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING BUT NOT 
LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
PURPOSE, TITLE, NON-INFRINGEMENT, THAT THE SOFTWARE WILL RUN UNINTERRUPTED OR ERROR-
FREE OR WARRANTIES ARISING FROM CUSTOM OF TRADE OR COURSE OF USAGE.  THE ENTIRE RISK 
ASSOCIATED WITH THE USE OF THE SOFTWARE IS ASSUMED BY YOU.  Some jurisdictions do not 
allow the exclusion of implied warranties, so the above exclusion may not apply to You. 

LIMITATION OF LIABILITY AND INDEMNIFICATION:  AMD AND ITS LICENSORS WILL NOT, 
UNDER ANY CIRCUMSTANCES BE LIABLE TO YOU FOR ANY PUNITIVE, DIRECT, INCIDENTAL, 
INDIRECT, SPECIAL OR CONSEQUENTIAL DAMAGES ARISING FROM USE OF THE SOFTWARE OR THIS 
AGREEMENT EVEN IF AMD AND ITS LICENSORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH 
DAMAGES.  In no event shall AMD's total liability to You for all damages, losses, and 
causes of action (whether in contract, tort (including negligence) or otherwise) 
exceed the amount of $100 USD.  You agree to defend, indemnify and hold harmless 
AMD and its licensors, and any of their directors, officers, employees, affiliates or 
agents from and against any and all loss, damage, liability and other expenses 
(including reasonable attorneys' fees), resulting from Your use of the Software or 
violation of the terms and conditions of this Agreement.  

U.S. GOVERNMENT RESTRICTED RIGHTS: The Materials are provided with "RESTRICTED RIGHTS." 
Use, duplication, or disclosure by the Government is subject to the restrictions as set 
forth in FAR 52.227-14 and DFAR252.227-7013, et seq., or its successor.  Use of the 
Materials by the Government constitutes acknowledgement of AMD's proprietary rights in them.

EXPORT RESTRICTIONS: The Materials may be subject to export restrictions as stated in the 
Software License Agreement.

<A NAME="CommandHelp">
# Command Help 
--------- 

```
   cloc: Convert an CL (Kernel C Language) file to brig, hsail, or
         object file using the LLVM to HSAIL backend compiler.
         Header .h files will also be created with .o files. 

   Usage: cloc [ options ] filename.cl

   Options without values:
    -c      Create .o object file for SNACK 
    -str    Create .o file with string for brig or hsail. e.g. to use with okra
    -hsail  Generate dissassembled hsail from brig 
    -ll     Generate dissassembled ll from bc, for info only
    -v      Display version of cloc then exit
    -q      Run quietly, no messages 
    -n      Dryrun, do nothing, show commands that would execute
    -h      Print this help message
    -k      Keep temporary files
    -nq     Shortcut for -n -q, Show commands without messages. 
    -fort   Generate fortran function names for -c option

   Options with values:
    -clopts  <compiler opts>  Default="-cl-std=CL2.0"
    -lkopts  <linker opts>    Read cloc script for defaults
    -s       <symbolname>     Default=filename (only with -str option)
    -t       <tdir>           Default=/tmp/cloc$$, Temp dir for files
    -o       <outfilename>    Default=<filename>.<ft> ft=brig, hsail, or o
    -p1     <path>            Default=$HSA_LLVM_PATH or /usr/local/HSAIL_LLVM_Backend/bin
    -p2     <path>            Default=$HSA_LIBHSAIL_PATH or /usr/local/HSAIL-Tools/libHSAIL/build
    -p3     <path>            Default=$HSA_RUNTIME_PATH or /usr/local/HSA-Runtime-AMD

   Examples:
    cloc mykernel.cl              /* create mykernel.brig            */
    cloc -c mykernel.cl           /* create mykernel.o for SNACK     */
    cloc -str mykernel.cl         /* create mykernel.o for okra      */
    cloc -hsail mykernel.cl       /* create mykernel.hsail           */
    cloc -str -hsail mykernel.cl  /* create mykernel.o for okra      */
    cloc -t /tmp/foo mykernel.cl  /* will automatically set -k       */

   You may set environment variables HSA_LLVM_PATH, HSA_LIBHSAIL_PATH, 
   HSA_RUNTIME_PATH, CLOPTS, or LKOPTS instead of providing options 
   -p1, -p2, -p3, -clopts, or -lkopts respectively.  
   Command line options will take precedence over environment variables. 

   (C) Copyright 2014 ADVANCED MICRO DEVICES, INC.

```

<A NAME="ReadmeExamples">
# Examples
-------- 

## Example 1: Hello World

This version of cloc supports the SNACK method of writing accelerated 
kernels in c. With SNACK, a host program can directly call the 
accelerated function. 
Here is the c++ source code HelloWorld.cpp using SNACK.
```cpp
//
//    File:  HelloWorld.cpp
//
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
```
The c source for the accelerated kernel is in file hw.cl.
```c
/*
    File:  hw.cl 
*/
__kernel void decode(__global char* in, __global char* out) {
	int num = get_global_id(0);
	out[num] = in[num] + 1;
}
```
The host program includes header file "hw.h" that does not exist yet.
The -c option of cloc will create both the object file and the header file.
The header file will have the function prototype for all kernels declared 
in the .cl file.  Use this command to compile the hw.cl file with cloc.

```
cloc -c hw.cl
```

You can now compile and build the binary "HelloWorld" with any c++ compiler.
Here is the command to build HelloWorld with g++. 

```
g++ -o HelloWorld hw.o HelloWorld.cpp -L$HSA_RUNTIME_PATH/lib/x86_64 -lhsa-runtime64 -lelf 

```

Then execute the program as follows.
```
$ ./HelloWorld
Hello HSA World
```

This example and other examples can be found in the CLOC repository in the directory examples/snack.

## Example 2: Manual HSAIL Optimization Process

This version of cloc supports a process where a programmer can experiment with
manual updates to HSAIL. This requires the use of SNACK (the -c option). 
This process has two steps. 

#### Step 1
The first step compiles the .cl file into the object code needed by a SNACK application.
For example, if your kernels are in the file myKernels.cl, then you can run step 1 as follows.
```
   cloc -c -hsail myKernels.cl
```
When cloc sees the "-c" option and the "-hsail" option, it will save four files 
in the same directory as myKernels.cl file.  The first two files are always created 
with the -c option. 

 1.  The object file (myKernels.o) to link with your application
 2.  The header file (myKernels.h) for your host code to compile correctly
 3.  The c wrapper code (myKernels.snackwrap.c) needed to recreate the .o file in step 2.
 4.  The HSAIL code (myKernels.hsail) to be manually modified in step 2. 

This is a good time to test your host program before making manual changes to the HSAIL.
BE WARNED, rerunning step 1 will overwrite these files. A common mistake is to rerun
step 1 after you have manually updated your HSAIL code for step 2. This would 
naturally destroy those edits.  

#### Step 2
For step 2, make manual edits to the myKernels.hsail file using your favorite editor.  
You may not change any of the calling arguments or insert new kernels.  This is because 
the generated wrapper from step 1 will be incorrect if you make these types of changes.  
If you want different arguments or new kernels, make those changes in your .cl file and
go back to step 1.  After your manual edits, rebuild the object code from your 
modified hsail as follows. 

```
   cloc -c myKernels.hsail
```
The above will fail if either the myKernels.hsail or myKernels.snackwrap.c file are missing.

## Example 3: Single Source

Currently cloc does not support the combination of kernel code and host code in the same
file. A future utility may provide this.  For now, put your kernel and device code
(routines called by kernels) into a .cl file for cloc to compile.  Put your host code into 
separate files with appropriate filetypes for your build environment.  The host code can 
be c++, c, or FORTRAN.  

One advantage of isolating kernel code from host code is that you can use any compiler
for your host code.  Any future utility that supports a single source will likely be bound
to a specific compiler. 
