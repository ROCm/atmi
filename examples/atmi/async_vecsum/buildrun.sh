#!/bin/bash

#  Set HSA Environment variables
[ -z $HSA_RUNTIME_PATH ] && HSA_RUNTIME_PATH=/opt/hsa
[ -z $HSA_LIBHSAIL_PATH ] && HSA_LIBHSAIL_PATH=/opt/hsa/lib
[ -z $HSA_LLVM_PATH ] && HSA_LLVM_PATH=/opt/amd/bin
[ -z $SNACK_RUNTIME_PATH ] && SNACK_RUNTIME_PATH=$HOME/git/CLOC.ashwinma
SNACK_INC=$SNACK_RUNTIME_PATH/include
export LD_LIBRARY_PATH=$SNACK_RUNTIME_PATH/lib:$HSA_RUNTIME_PATH/lib:$LD_LIBRARY_PATH
# Compile accelerated functions
echo 
if [ -f sumKernel.o ] ; then rm sumKernel.o ; fi
echo snack.sh -snk $SNACK_RUNTIME_PATH -c sumKernel.cl 
snack.sh -snk $SNACK_RUNTIME_PATH -c sumKernel.cl 
#echo snack.sh sumKernel.cl 
#snack.sh sumKernel.cl 
echo 
if [ -f vecsum ] ; then rm vecsum ; fi
echo g++ -O3  -o vecsum sumKernel.o vecsum.cpp -lsnk -L$SNACK_RUNTIME_PATH/lib -L $HSA_RUNTIME_PATH/lib -lhsa-runtime64 -lelf -I$SNACK_INC
g++ -O3  -o vecsum sumKernel.o vecsum.cpp -lsnk -L$SNACK_RUNTIME_PATH/lib -L $HSA_RUNTIME_PATH/lib -lhsa-runtime64 -lelf -I$SNACK_INC

#  Execute
echo
echo ./vecsum 
./vecsum | tee vecsum.out
