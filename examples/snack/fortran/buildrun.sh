#!/bin/bash

#  Set HSA Environment variables
[ -z $HSA_RUNTIME_PATH ] && HSA_RUNTIME_PATH=/opt/hsa
[ -z $HSA_LLVM_PATH ] && HSA_LLVM_PATH=/opt/amd/cloc/bin
export LD_LIBRARY_PATH=$HSA_RUNTIME_PATH/lib

#  First compile the acclerated functions to create hw.o
#  Tell cloc to use fortran names for external references
echo 
echo $HSA_LLVM_PATH/snack.sh -q -fort -c hw.cl 
$HSA_LLVM_PATH/snack.sh -q -fort -c hw.cl 

#  Compile the main Fortran program and link to hw.o
echo 
echo "f95 -o HelloWorld hw.o HelloWorld.f -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64  "
f95 -o HelloWorld hw.o HelloWorld.f -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64 

echo 
echo ./HelloWorld
./HelloWorld
