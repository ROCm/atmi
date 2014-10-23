#!/bin/bash

#  Set HSA Environment variables
[ -z $HSA_RUNTIME_PATH ] && HSA_RUNTIME_PATH=/usr/local/HSA-Runtime-AMD
[ -z HSA_LIBHSAIL_PATH ] && HSA_LIBHSAIL_PATH=/usr/local/HSAIL-Tools/libHSAIL/build
[ -z HSA_KMT_PATH ] && HSA_KMT_PATH=/usr/local/HSA-Drivers-Linux-AMD/kfd-0.8/libhsakmt
[ -z HSA_LLVM_PATH ] && HSA_LLVM_PATH=/usr/local/HSAIL_LLVM_Backend/bin
export HSA_RUNTIME_PATH HSA_LIBHSAIL_PATH HSA_KMT_PATH HSA_LLVM_PATH
export LD_LIBRARY_PATH=$HSA_KMT_PATH/lnx64a:$HSA_RUNTIME_PATH/lib

#  First compile the acclerated functions to create hw.o
#  Tell cloc to use fortran names for external references
echo 
echo cloc -q -fort -c hw.cl 
cloc -q -fort -c hw.cl 

#  Compile the main Fortran program and link to hw.o
echo 
echo "f95 -o HelloWorld hw.o HelloWorld.f -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64 -lelf "
f95 -o HelloWorld hw.o HelloWorld.f -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64 -lelf 

echo 
echo ./HelloWorld
./HelloWorld
