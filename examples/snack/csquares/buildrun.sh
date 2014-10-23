#!/bin/bash

#  Set HSA Environment variables
[ -z $HSA_RUNTIME_PATH ] && HSA_RUNTIME_PATH=/usr/local/HSA-Runtime-AMD
[ -z HSA_LIBHSAIL_PATH ] && HSA_LIBHSAIL_PATH=/usr/local/HSAIL-Tools/libHSAIL/build
[ -z HSA_KMT_PATH ] && HSA_KMT_PATH=/usr/local/HSA-Drivers-Linux-AMD/kfd-0.8/libhsakmt
[ -z HSA_LLVM_PATH ] && HSA_LLVM_PATH=/usr/local/HSAIL_LLVM_Backend/bin
export HSA_RUNTIME_PATH HSA_LIBHSAIL_PATH HSA_KMT_PATH HSA_LLVM_PATH

export LD_LIBRARY_PATH=$HSA_KMT_PATH/lnx64a:$HSA_RUNTIME_PATH/lib

# Compile accelerated functions
echo 
if [ -f CSquares.o ] ; then rm CSquares.o ; fi
echo cloc -q -c CSquares.cl 
cloc -q -c CSquares.cl 

# Compile Main and link to accelerated functions in CSquares.o
echo 
if [ -f CSquares ] ; then rm CSquares ; fi
echo "g++ -o CSquares CSquares.o CSquares.cpp -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64 -lelf "
g++ -o CSquares CSquares.o CSquares.cpp -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64 -lelf 

#  Execute
echo
echo ./CSquares
./CSquares
