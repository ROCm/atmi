CLOC -Version 0.6.2
====

   cloc: Convert an OpenCL file to brig, hsail, or .o (object) 
         file using the LLVM to HSAIL backend compiler.

   Usage: cloc [ options ] filename.cl

   Options without values:
    -hsail  Generate hsail instead of brig 
    -str    Saves the output as a string in a header file and compiles it to .o object file 
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
    -p1      <path> HSA_LLVM_PATH, Default=$HSA_LLVM_PATH or /usr/local/HSAIL_LLVM_Backend/bin
    -p2      <path> HSA_LIBHSAIL_PATH, Default=$HSA_LIBHSAIL_PATH or /usr/local/HSAIL-Tools/libHSAIL/build_linux
    -o      <outfilename>, Default=<filename>.<ft> where ft is brig, hsail, or o

   Examples:
      cloc mykernel.cl              /* creates mykernel.brig  */
      cloc -str mykernel.cl           /* creates mykernel.o     */
      cloc -hsail mykernel.cl       /* creates mykernel.hsail */
      cloc -str -hsail mykernel.cl    /* creates mykernel.o     */
      cloc -t /tmp/foo mykernel.cl  /* automatically sets -k  */
      cloc -str -nq -clopts "--opencl=2.0" mykernel.cl

   You may set environment variables HSA_LLVM_PATH, HSA_LIBHSAIL_PATH, CLOPTS, or LKOPTS instead 
   of using the command line options -p, -clopts, or -lkopts.  
   Command line options take precedence over the environment variables. 

   (C) Copyright 2014 AMD 

