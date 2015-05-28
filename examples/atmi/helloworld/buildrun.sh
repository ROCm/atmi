#!/bin/bash

#  Set HSA Environment variables
[ -z $HSA_RUNTIME_PATH ] && HSA_RUNTIME_PATH=/opt/hsa
[ -z $HSA_LIBHSAIL_PATH ] && HSA_LIBHSAIL_PATH=/opt/hsa/lib
[ -z $HSA_LLVM_PATH ] && HSA_LLVM_PATH=/opt/amd/cloc/bin
[ -z $SNACK_RUNTIME_PATH ] && SNACK_RUNTIME_PATH=$HOME/git/CLOC.ashwinma
export LD_LIBRARY_PATH=$HSA_RUNTIME_PATH/lib:$SNACK_RUNTIME_PATH/lib:$LD_LIBRARY_PATH
echo $LD_LIBRARY_PATH
# Compile accelerated functions
echo 
if [ -f helloKernel.o ] ; then rm helloKernel.o ; fi
echo snack.sh -snk $SNACK_RUNTIME_PATH -v -gccopt 3 -c  helloKernel.cl
snack.sh -snk $SNACK_RUNTIME_PATH -v -gccopt 3 -c  helloKernel.cl

echo 
if [ -f hello ] ; then rm hello ; fi
echo g++ -o hello helloKernel.o hello.cpp -O3 -lelf -L$SNACK_RUNTIME_PATH/lib -lsnk -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64 
g++ -o hello hello.cpp -g helloKernel.o -O3 -lelf -L$SNACK_RUNTIME_PATH/lib -lsnk -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64 

#  Execute
echo
echo ./hello 
./hello 
