#!/bin/bash
set -e
#  Set HSA Environment variables
[ -z $HSA_RUNTIME_PATH ] && HSA_RUNTIME_PATH=/opt/hsa
[ -z $HSA_LIBHSAIL_PATH ] && HSA_LIBHSAIL_PATH=/opt/hsa/lib
[ -z $HSA_LLVM_PATH ] && HSA_LLVM_PATH=/opt/amd/cloc/bin
[ -z $ATMI_RUNTIME_PATH ] && ATMI_RUNTIME_PATH=$HOME/git/atmi
ATMI_INC=$ATMI_RUNTIME_PATH/include

export LD_LIBRARY_PATH=$HSA_RUNTIME_PATH/lib:$ATMI_RUNTIME_PATH/lib:$LD_LIBRARY_PATH
echo $LD_LIBRARY_PATH

# Compile accelerated functions
echo 
if [ -f csquares_kernels.o ] ; then rm csquares_kernels.o ; fi
echo atmi_gpupifgen.sh -snk $ATMI_RUNTIME_PATH -v -gccopt 3 -c csquares_kernels.cl 
atmi_gpupifgen.sh -snk $ATMI_RUNTIME_PATH -v -gccopt 3 -c csquares_kernels.cl 

# Compile Main and link to accelerated functions in csquares.o
echo 
if [ -f csquares ] ; then rm csquares ; fi
echo g++ -o csquares.o -c csquares.cpp -fplugin=libatmi_pifgen.so -I$ATMI_INC 
g++ -o csquares.o -c csquares.cpp -fplugin=libatmi_pifgen.so -I$ATMI_INC 

echo g++ -o csquares csquares_kernels.o csquares.o csquares.cpp.pifdefs.c -lsnk -L$ATMI_RUNTIME_PATH/lib -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64 -lelf -I$ATMI_INC -I$HSA_RUNTIME_PATH/include 
g++ -o csquares csquares_kernels.o csquares.o csquares.cpp.pifdefs.c -lsnk -L$ATMI_RUNTIME_PATH/lib -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64 -lelf -I$ATMI_INC -I$HSA_RUNTIME_PATH/include 

#  Execute
echo
echo ./csquares
./csquares
