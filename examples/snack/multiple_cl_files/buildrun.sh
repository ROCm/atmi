#!/bin/bash

#  This is a demo to show that cloc can work with multiple cl files. 

#  Set HSA Environment variables
[ -z $HSA_RUNTIME_PATH ] && HSA_RUNTIME_PATH=/opt/hsa
[ -z $HSA_LLVM_PATH ] && HSA_LLVM_PATH=/opt/amd/cloc/bin
export LD_LIBRARY_PATH=$HSA_RUNTIME_PATH/lib

#  First compile all files with acclerated functions to create hw.o and hw2.o
echo 
echo $HSA_LLVM_PATH snack.sh -q -c hw.cl 
$HSA_LLVM_PATH snack.sh -q -c hw.cl 
echo 
echo $HSA_LLVM_PATH/snack.sh -q -c -noglobs hw2.cl 
$HSA_LLVM_PATH/snack.sh -q -c -noglobs hw2.cl 

#  Compile the main program and link to hw.o
#  Main program can be c, cpp, or fotran
echo 
echo "gcc -o HelloWorld hw.o hw2.o HelloWorld.c -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64 "
gcc -o HelloWorld hw.o hw2.o HelloWorld.c -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64 

echo 
echo ./HelloWorld
./HelloWorld
