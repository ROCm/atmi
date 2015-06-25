#!/bin/bash
set -e
#  Set HSA Environment variables
[ -z $HSA_RUNTIME_PATH ] && HSA_RUNTIME_PATH=/opt/hsa
[ -z $HSA_LIBHSAIL_PATH ] && HSA_LIBHSAIL_PATH=/opt/hsa/lib
[ -z $HSA_LLVM_PATH ] && HSA_LLVM_PATH=/opt/amd/bin
[ -z $ATMI_RUNTIME_PATH ] && ATMI_RUNTIME_PATH=$HOME/git/atmi
ATMI_INC=$ATMI_RUNTIME_PATH/include
export LD_LIBRARY_PATH=$ATMI_RUNTIME_PATH/lib:$HSA_RUNTIME_PATH/lib:$LD_LIBRARY_PATH
# Compile accelerated functions
echo 
if [ -f sumKernel.o ] ; then rm sumKernel.o ; fi
echo atmi_gpupifgen.sh -snk $ATMI_RUNTIME_PATH -c sumKernel.cl 
atmi_gpupifgen.sh -snk $ATMI_RUNTIME_PATH -c sumKernel.cl 

echo 
if [ -f vecsum ] ; then rm vecsum ; fi
echo g++ -O3  -o vecsum sumKernel.o vecsum.cpp -lsnk -L$ATMI_RUNTIME_PATH/lib -L $HSA_RUNTIME_PATH/lib -lhsa-runtime64 -lelf -I$ATMI_INC
g++ -O3  -o vecsum sumKernel.o vecsum.cpp -lsnk -L$ATMI_RUNTIME_PATH/lib -L $HSA_RUNTIME_PATH/lib -lhsa-runtime64 -lelf -I$ATMI_INC

#  Execute
echo
echo ./vecsum 
./vecsum | tee vecsum.out
