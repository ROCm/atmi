#!/bin/bash
set -e
#  Set HSA Environment variables
[ -z $HSA_RUNTIME_PATH ] && HSA_RUNTIME_PATH=/opt/hsa
[ -z $HSA_LIBHSAIL_PATH ] && HSA_LIBHSAIL_PATH=/opt/hsa/lib
[ -z $HSA_LLVM_PATH ] && HSA_LLVM_PATH=/opt/amd/cloc/bin
[ -z $ATMI_RUNTIME_PATH ] && ATMI_RUNTIME_PATH=$HOME/git/atmi
ATMI_RUNTIME_PATH=$HOME/git/atmi
ATMI_INC=$ATMI_RUNTIME_PATH/include
export LD_LIBRARY_PATH=$HSA_RUNTIME_PATH/lib:$ATMI_RUNTIME_PATH/lib:$LD_LIBRARY_PATH
echo $LD_LIBRARY_PATH
# Compile accelerated functions
#echo 
#if [ -f fibonacci.o ] ; then rm fibonacci.o ; fi
#echo atmi_gpupifgen.sh -snk $ATMI_RUNTIME_PATH -v -gccopt 3 -c  fibonacci.cl
#atmi_gpupifgen.sh -snk $ATMI_RUNTIME_PATH -v -gccopt 3 -c  fibonacci.cl

echo 
if [ -f fibonacci ] ; then rm fibonacci ; fi
echo g++ -c -o fibonacci.o fibonacci.cpp -g -fplugin=libatmi_pifgen.so -O3 -I$ATMI_INC 
g++ -c -o fibonacci.o fibonacci.cpp -g -fplugin=libatmi_pifgen.so -O3 -I$ATMI_INC 

echo g++ -o fibonacci fibonacci.o fibonacci.cpp.pifdefs.c -O3 -lelf -L$ATMI_RUNTIME_PATH/lib -lsnk -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64 -I$ATMI_INC -I$HSA_RUNTIME_PATH/include
g++ -o fibonacci fibonacci.o fibonacci.cpp.pifdefs.c -O3 -lelf -L$ATMI_RUNTIME_PATH/lib -lsnk -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64 -I$ATMI_INC -I$HSA_RUNTIME_PATH/include


#  Execute
echo
echo ./fibonacci $1
./fibonacci $1
