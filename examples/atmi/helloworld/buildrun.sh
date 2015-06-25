#!/bin/bash
set -e
#  Set HSA Environment variables
[ -z $HSA_RUNTIME_PATH ] && HSA_RUNTIME_PATH=/opt/hsa
[ -z $HSA_LIBHSAIL_PATH ] && HSA_LIBHSAIL_PATH=/opt/hsa/lib
[ -z $HSA_LLVM_PATH ] && HSA_LLVM_PATH=/opt/amd/cloc/bin
[ -z $ATMI_RUNTIME_PATH ] && ATMI_RUNTIME_PATH=$HOME/git/CLOC.ashwinma
ATMI_INC=$ATMI_RUNTIME_PATH/include
export LD_LIBRARY_PATH=$HSA_RUNTIME_PATH/lib:$ATMI_RUNTIME_PATH/lib:$LD_LIBRARY_PATH
echo $LD_LIBRARY_PATH
# Compile accelerated functions
echo 
#if [ -f hw.o ] ; then rm hw.o ; fi
#echo atmi_gpupifgen.sh -snk $ATMI_RUNTIME_PATH -v -gccopt 3 -c  hw.cl
#atmi_gpupifgen.sh -snk $ATMI_RUNTIME_PATH -v -gccopt 3 -c  hw.cl

echo 
if [ -f hello ] ; then rm hello ; fi
echo g++ -o HelloWorld.o -c HelloWorld.cpp -g -fplugin=libatmi_pifgen.so -O3 -I$ATMI_INC
g++ -c -o HelloWorld.o HelloWorld.cpp -g -fplugin=libatmi_pifgen.so -O3 -I$ATMI_INC

echo g++ -o hello HelloWorld.o HelloWorld.cpp.pifdefs.c -g -O3 -lelf -L$ATMI_RUNTIME_PATH/lib -lsnk -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64 -I$ATMI_INC -I$HSA_RUNTIME_PATH/include
g++ -o hello HelloWorld.o HelloWorld.cpp.pifdefs.c -g -O3 -lelf -L$ATMI_RUNTIME_PATH/lib -lsnk -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64 -I$ATMI_INC -I$HSA_RUNTIME_PATH/include

#  Execute
echo
echo ./hello 
./hello 
