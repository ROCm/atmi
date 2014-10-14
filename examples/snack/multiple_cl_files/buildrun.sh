#!/bin/bash

#  This is a demo to show that cloc can work with multiple cl files. 

#  Set HSA Environment variables
[ -z $HSA_RUNTIME_PATH ] && HSA_RUNTIME_PATH=/usr/local/HSA-Runtime-AMD
[ -z HSA_LIBHSAIL_PATH ] && HSA_LIBHSAIL_PATH=/usr/local/HSAIL-Tools/libHSAIL/build
[ -z HSA_KMT_PATH ] && HSA_KMT_PATH=/usr/local/HSA-Drivers-Linux-AMD/kfd-0.8/libhsakmt
[ -z HSA_LLVM_PATH ] && HSA_LLVM_PATH=/usr/local/HSAIL_LLVM_Backend/bin
export HSA_RUNTIME_PATH HSA_LIBHSAIL_PATH HSA_KMT_PATH HSA_LLVM_PATH
export LD_LIBRARY_PATH=$HSA_KMT_PATH/lnx64a:$HSA_RUNTIME_PATH/lib/x86_64

#  First compile all files with acclerated functions to create hw.o and hw2.o
echo 
echo cloc -q -c hw.cl 
cloc -q -c hw.cl 
echo 
echo cloc -q -c hw2.cl 
cloc -q -c hw2.cl 

#  Compile the main program and link to hw.o
#  Main program can be c, cpp, or fotran
echo 
echo "gcc -o HelloWorld hw.o hw2.o HelloWorld.c -L$HSA_RUNTIME_PATH/lib/x86_64 -lhsa-runtime64 -lelf"
gcc -o HelloWorld hw.o hw2.o HelloWorld.c -L$HSA_RUNTIME_PATH/lib/x86_64 -lhsa-runtime64 -lelf

echo 
echo ./HelloWorld
./HelloWorld
