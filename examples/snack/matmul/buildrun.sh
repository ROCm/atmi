#!/bin/bash

#   For this test case we need libbsd-dev for the random number generator
#   sudo apt-get install libbsd-dev
#
#  Set HSA Environment variables
[ -z $HSA_RUNTIME_PATH ] && HSA_RUNTIME_PATH=/opt/hsa
[ -z $HSA_LLVM_PATH ] && HSA_LLVM_PATH=/opt/amd/cloc/bin
export LD_LIBRARY_PATH=$HSA_RUNTIME_PATH/lib

# Compile accelerated functions
echo 
if [ -f matmulKernels.o ] ; then rm matmulKernels.o ; fi
echo $HSA_LLVM_PATH/snack.sh -c -opt 3 -vv  matmulKernels.cl 
$HSA_LLVM_PATH/snack.sh -c -opt 3 -vv  matmulKernels.cl 

# Compile Main .c  and link to accelerated functions in matmulKernels.o
echo 
if [ -f matmul ] ; then rm matmul ; fi
echo gcc -O3 -o matmul matmulKernels.o matmul.c -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64 -lbsd
gcc -O3 -o matmul matmulKernels.o matmul.c -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64 -lbsd

#  Execute the application
echo 
#  Make sure parci
./matmul 5 6 7
./matmul 2000 2000 2000
