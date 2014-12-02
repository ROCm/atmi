#!/bin/bash

#   For this test case we need libbsd-dev for the random number generator
#   sudo apt-get install libbsd-dev
#
#  Set HSA Environment variables
[ -z $HSA_RUNTIME_PATH ] && HSA_RUNTIME_PATH=/opt/hsa
[ -z HSA_LLVM_PATH ] && HSA_LLVM_PATH=/opt/amd/bin
export LD_LIBRARY_PATH=$HSA_RUNTIME_PATH/lib

# Compile accelerated functions
echo 
if [ -f matmulKernels.o ] ; then rm matmulKernels.o ; fi
echo cloc -c -q matmulKernels.cl 
cloc -c -q  matmulKernels.cl 

# Compile Main .c  and link to accelerated functions in matmulKernels.o
echo 
if [ -f matmul ] ; then rm matmul ; fi
echo gcc -O3 -o matmul matmulKernels.o matmul.c -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64 -lelf -lbsd
gcc -O3 -o matmul matmulKernels.o matmul.c -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64 -lelf -lbsd

#  Execute the application
echo 
#  Make sure parci
./matmul 5 6 7
./matmul 2000 2000 2000
