#!/bin/bash
set -e
#  Set HSA Environment variables
[ -z $HSA_RUNTIME_PATH ] && HSA_RUNTIME_PATH=/opt/rocm/hsa
[ -z $HSA_LIBHSAIL_PATH ] && HSA_LIBHSAIL_PATH=/opt/rocm/hsa/lib
[ -z $HSA_LLVM_PATH ] && HSA_LLVM_PATH=/usr/bin
[ -z $ATMI_RUNTIME_PATH ] && ATMI_RUNTIME_PATH=/opt/amd/atmi
ATMI_INC=$ATMI_RUNTIME_PATH/include

export LD_LIBRARY_PATH=$HSA_RUNTIME_PATH/lib:$ATMI_RUNTIME_PATH/lib:$LD_LIBRARY_PATH
#echo $LD_LIBRARY_PATH

# Do not compile accelerated functions separately. This script will be invoked by the GCC plugin itself.
echo 
#if [ -f csquares_kernels.o ] ; then rm csquares_kernels.o ; fi
#echo $ATMI_RUNTIME_PATH/bin/cl2brigh.sh -gccopt 3 csquares_kernels.cl 
#$ATMI_RUNTIME_PATH/bin/cl2brigh.sh -gccopt 3 csquares_kernels.cl 

# Compile Main and generate the PIF definitions for host and accelerated functions in csquares.cpp.pifdefs.c
echo 
if [ -f csquares ] ; then rm csquares ; fi
echo g++ -o csquares.o -c csquares.cpp -fplugin=atmi_pifgen.so -fplugin-arg-atmi_pifgen-clfile=csquares_kernels.cl -I$ATMI_INC 
g++ -o csquares.o -c csquares.cpp -std=c++11 -fplugin=atmi_pifgen.so -fplugin-arg-atmi_pifgen-clfile=csquares_kernels.cl -I$ATMI_INC 

echo g++ -o csquares csquares.o csquares.cpp.pifdefs.c -latmi_runtime -L$ATMI_RUNTIME_PATH/lib -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64 -lelf -I$ATMI_INC -I$HSA_RUNTIME_PATH/include 
g++ -o csquares csquares.o csquares.cpp.pifdefs.c -std=c++11 -latmi_runtime -L$ATMI_RUNTIME_PATH/lib -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64 -lelf -I$ATMI_INC -I$HSA_RUNTIME_PATH/include 

#  Execute
echo
echo ./csquares
./csquares
