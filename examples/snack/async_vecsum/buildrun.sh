#!/bin/bash

#  Set HSA Environment variables
[ -z $HSA_RUNTIME_PATH ] && HSA_RUNTIME_PATH=/opt/hsa
[ -z $HSA_LLVM_PATH ] && HSA_LLVM_PATH=/opt/amd/cloc/bin
export LD_LIBRARY_PATH=$HSA_RUNTIME_PATH/lib
# Compile accelerated functions
echo 
if [ -f sumKernel.o ] ; then rm sumKernel.o ; fi
echo $HSA_LLVM_PATH/snack.sh -c sumKernel.cl 
$HSA_LLVM_PATH/snack.sh -c sumKernel.cl 

echo 
if [ -f vecsum ] ; then rm vecsum ; fi
echo g++ -O3  -o vecsum sumKernel.o vecsum.cpp -L $HSA_RUNTIME_PATH/lib -lhsa-runtime64  
g++ -O3  -o vecsum sumKernel.o vecsum.cpp -L $HSA_RUNTIME_PATH/lib -lhsa-runtime64 

#  Execute
echo
echo ./vecsum 
./vecsum | tee vecsum.out
