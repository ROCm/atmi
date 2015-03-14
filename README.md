CLOC - Version 0.8.0
====================

CLOC:  CL Offline Compiler
       Generate HSAIL or brig from a cl (Kernel c Language) file.
SNACK: Structured No API Compiled Kernels.
       Launch GPU kernels as host-callable functions with structured launch parameters.

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

Copyright (c) 2015 ADVANCED MICRO DEVICES, INC.  

AMD is granting you permission to use this software and documentation (if any) (collectively, the 
Materials) pursuant to the terms and conditions of the Software License Agreement included with the 
Materials.  If you do not have a copy of the Software License Agreement, contact your AMD 
representative for a copy.

You agree that you will not reverse engineer or decompile the Materials, in whole or in part, except for 
example code which is provided in source code form and as allowed by applicable law.

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
## The cloc.sh command 

```
   cloc.sh: Convert a cl file to brig or hsail using the
            LLVM to HSAIL backend compiler.

   Usage: cloc [ options ] filename.cl

   Options without values:
    -hsail    Generate dissassembled hsail from brig 
    -ll       Generate dissassembled ll from bc, for info only
    -version  Display version of cloc then exit
    -v        Verbose messages
    -n        Dryrun, do nothing, show commands that would execute
    -h        Print this help message
    -k        Keep temporary files

   Options with values:
    -clopts  <compiler opts>  Default="-cl-std=CL2.0"
    -lkopts  <linker opts>    Read cloc script for defaults
    -t       <tdir>           Default=/tmp/cloc$$, Temp dir for files
    -o       <outfilename>    Default=<filename>.<ft> ft=brig or hsail
    -p       <path>           Default=$HSA_LLVM_PATH or /opt/amd/bin

   Examples:
    cloc my.cl              /* create my.brig                   */
    cloc -hsail my.cl       /* --> my.hsail and my.brig         */
    cloc -t /tmp/foo my.cl  /* Set tempdir will force -k option */

   You may set environment variables HSA_LLVM_PATH, CLOPTS, or LKOPTS 
   instead of providing options -p, -clopts, or -lkopts respectively.  
   Command line options will take precedence over environment variables. 

   Copyright (c) 2015 ADVANCED MICRO DEVICES, INC.

```

## The snack.sh Command 

```
   snack: Generate host-callable "snack" functions for GPU kernels.
          Snack generates the source code and headers for each kernel 
          in the input filename.cl file.  The -c option will compile 
          the source with gcc so you can link with your host application.
          Host applicaton requires no API to use snack functions.

   Usage: snack.sh [ options ] filename.cl

   Options without values:
    -c      Compile generated source code to create .o file
    -hsail  Generate dissassembled hsail from brig 
    -v      Display version of snack then exit
    -q      Run quietly, no messages 
    -n      Dryrun, do nothing, show commands that would execute
    -h      Print this help message
    -k      Keep temporary files
    -nq     Shortcut for -n -q, Show commands without messages. 
    -fort   Generate fortran function names
    -noglobs Do not generate global functions 
    -str    Create .o file with string for brig or hsail. e.g. to use with okra

   Options with values:
    -clopts  <compiler opts>  Default="-cl-std=CL2.0"
    -lkopts  <linker opts>    Read snack script for defaults
    -s       <symbolname>     Default=filename (only with -str option)
    -t       <tdir>           Default=/tmp/cloc$$, Temp dir for files
    -o       <outfilename>    Default=<filename>.<ft> ft=snackwrap.c or o
    -p1     <path>            Default=$HSA_LLVM_PATH or /opt/amd/bin
    -p2     <path>            Default=$HSA_RUNTIME_PATH or /opt/hsa
    -opt    <cl opt>          Default=2
    -gccopt <gcc opt>         Default=2

   Examples:
    snack my.cl              /* create my.snackwrap.c and my.h  */
    snack -c my.cl           /* gcc compile to creat  my.o      */
    snack -str my.cl         /* create mykernel.o for okra      */
    snack -hsail my.cl       /* create hsail and snackwrap.c    */
    snack -t /tmp/foo my.cl  /* will automatically set -k       */

   You may set environment variables HSA_LLVM_PATH, HSA_RUNTIME_PATH, 
   CLOPTS, or LKOPTS instead of providing options -p1, -p2, -clopts, 
   or -lkopts respectively.  
   Command line options will take precedence over environment variables. 

   Copyright (c) 2015 ADVANCED MICRO DEVICES, INC.

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
        SNK_INIT_LPARM(lparm,strlength);
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
The -c option of snack.sh will call the gcc compiler and create 
the object file and the header file.  Without -c you get the 
generated c code hw.snackwrap.c and the header file.  The header file
has function prototypes for all kernels declared in the .cl file.  Use 
this command to compile the hw.cl file with snack.sh.

```
snack.sh -c hw.cl
```

You can now compile and build the binary "HelloWorld" with any c++ compiler.
Here is the command to build HelloWorld with g++. 

```
g++ -o HelloWorld hw.o HelloWorld.cpp -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64 -lelf 

```

Then execute the program as follows.
```
$ ./HelloWorld
Hello HSA World
```

This example and other examples can be found in the CLOC repository in the directory examples/snack.

## Example 2: Manual HSAIL Optimization Process

This version of snack supports a process where a programmer can experiment with
manual updates to HSAIL.  This process has two steps. 

#### Step 1
The first step compiles the .cl file into the object code needed by a SNACK application.
For example, if your kernels are in the file myKernels.cl, then you can run step 1 as follows.
```
   snack -c -hsail myKernels.cl
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
   snack.sh -c  myKernels.hsail
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
