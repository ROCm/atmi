#!/bin/bash

#  Set HSA Environment variables
[ -z $HSA_RUNTIME_PATH ] && HSA_RUNTIME_PATH=/opt/hsa
[ -z $HSA_LIBHSAIL_PATH ] && HSA_LIBHSAIL_PATH=/opt/hsa/lib
[ -z $HSA_LLVM_PATH ] && HSA_LLVM_PATH=/opt/amd/cloc/bin
[ -z $SNACK_RUNTIME_PATH ] && SNACK_RUNTIME_PATH=$HOME/git/atmi
SNACK_RUNTIME_PATH=$HOME/git/atmi
SNACK_INC=$SNACK_RUNTIME_PATH/include
export LD_LIBRARY_PATH=$HSA_RUNTIME_PATH/lib:$SNACK_RUNTIME_PATH/lib:$LD_LIBRARY_PATH
echo $LD_LIBRARY_PATH
# Compile accelerated functions
#echo 
#if [ -f fibonacci.o ] ; then rm fibonacci.o ; fi
#echo snack.sh -snk $SNACK_RUNTIME_PATH -v -gccopt 3 -c  fibonacci.cl
#snack.sh -snk $SNACK_RUNTIME_PATH -v -gccopt 3 -c  fibonacci.cl

echo 
if [ -f fibonacci ] ; then rm fibonacci ; fi
echo g++ -c -o fibonacci.o fibonacci.cpp -g -fplugin=libatmi_pifgen.so -O3 -lelf -L$SNACK_RUNTIME_PATH/lib -lsnk -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64 -I$SNACK_INC
g++ -c -o fibonacci.o fibonacci.cpp -g -fplugin=libatmi_pifgen.so -O3 -I$SNACK_INC 

echo g++ -o fibonacci fibonacci.o fibonacci_pifdef.o -O3 -lelf -L$SNACK_RUNTIME_PATH/lib -lsnk -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64 -I$SNACK_INC
g++ -o fibonacci fibonacci.o fibonacci.cpp.pifdefs.c -O3 -lelf -L$SNACK_RUNTIME_PATH/lib -lsnk -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64 -I$SNACK_INC -I$HSA_RUNTIME_PATH/include


#  Execute
echo
echo ./fibonacci $1
./fibonacci $1
