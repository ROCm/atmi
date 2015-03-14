#!/bin/bash

#  Set HSA Environment variables
[ -z $HSA_RUNTIME_PATH ] && HSA_RUNTIME_PATH=/opt/hsa
[ -z HSA_LIBHSAIL_PATH ] && HSA_LIBHSAIL_PATH=/opt/hsa/lib
[ -z HSA_LLVM_PATH ] && HSA_LLVM_PATH=/opt/amd/bin

export LD_LIBRARY_PATH=$HSA_RUNTIME_PATH/lib

# Compile accelerated functions
echo 
if [ -f CSquares.o ] ; then rm CSquares.o ; fi
echo snack.sh -q -c CSquares.cl 
snack.sh -q -c CSquares.cl 

# Compile Main and link to accelerated functions in CSquares.o
echo 
if [ -f CSquares ] ; then rm CSquares ; fi
echo "g++ -o CSquares CSquares.o CSquares.cpp -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64 -lelf "
g++ -o CSquares CSquares.o CSquares.cpp -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64 -lelf 

#  Execute
echo
echo ./CSquares
./CSquares
