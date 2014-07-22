CLOC -Version 0.6
====

CL Offline Compiler : Compile OpenCL kernels to HSAIL


cloc: Convert an OpenCL file to brig, hsail, or .o (object) 
         file using the LLVM to HSAIL backend compiler.

   Usage: cloc [ options ] filename.cl

   Options without values:
    -hsail  Generate hsail instead of brig 
    -c      Compile hsail or brig to .o object file 
    -v      Display version of cloc then exit
    -q      Run quietly, no messages 
    -n      Dryrun, do nothing, show commands that would execute
    -h      Print this help message
    -k      Keep temporary files
    -nq     Shortcut for -n -q, Show commands without messages. 

   Options with values:
    -clopts <cl compiler options> CLOPTS, Default="--opencl=1.2"
    -lkopts <linker options> LKOPTS, Read cloc script for defaults
    -s      <symbolname> , Default=filename (only with -c)
    -t      <tdir> Temporary directory, Default=/tmp/cloc$$
    -p      <path> HSBEPATH, Default=$HSBEPATH or /usr/local/HSAIL_LLVM_Backend/bin
    -o      <outfilename>, Default=<filename>.<ft> where ft is brig, hsail, or o

   Examples:
      cloc mykernel.cl              /* creates mykernel.brig  */
      cloc -c mykernel.cl           /* creates mykernel.o     */
      cloc -hsail mykernel.cl       /* creates mykernel.hsail */
      cloc -c -hsail mykernel.cl    /* creates mykernel.o     */
      cloc -t /tmp/foo mykernel.cl  /* automatically sets -k  */
      cloc -c -nq -clopts "--opencl=2.0" mykernel.cl

   You may set environment variables HSBEPATH, CLOPTS, or LKOPTS instead 
   of using the command line options -p, -clopts, or -lkopts.  
   Command line options take precedence over the environment variables. 

   (C) Copyright 2014 AMD 

Note: HSBEPATH must be set to the path containing the binaries of HSAIL_LLVM_BACKEND (https://github.com/HSAFoundation/HSAIL_LLVM_Backend)

