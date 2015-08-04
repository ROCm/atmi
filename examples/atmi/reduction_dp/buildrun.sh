#!/bin/bash
set -e
#  Set HSA Environment variables
[ -z $HSA_TEST_RUNTIME_PATH ] && HSA_TEST_RUNTIME_PATH=/opt/hsa.1_1T/
[ -z $HSA_LIBHSAIL_PATH ] && HSA_LIBHSAIL_PATH=$HSA_TEST_RUNTIME_PATH/lib
[ -z $ATMI_RUNTIME_PATH ] && ATMI_RUNTIME_PATH=/opt/amd/atmi
ATMI_INC=$ATMI_RUNTIME_PATH/include

# Do not compile accelerated functions separately. This script will be invoked by the GCC plugin itself.
#echo cl2brigh.sh -v -gccopt 3 hw.cl
#cl2brigh.sh -v -gccopt 3 hw.cl

# Compile Main and generate the PIF definitions for host and accelerated functions
# in Reduction.cpp.pifdefs.c
#echo 
if [ -f hello ] ; then rm hello ; fi
echo g++ -c -o Reduction.o Reduction.cpp -g -fplugin=atmi_pifgen.so -fplugin-arg-atmi_pifgen-clfile=reduction.cl -fplugin-arg-atmi_pifgen-pifgenfile=pifdefs.cpp -O3 -I$ATMI_INC
g++ -c -o Reduction.o Reduction.cpp -g -fplugin=atmi_pifgen.so -fplugin-arg-atmi_pifgen-clfile=reduction.cl -fplugin-arg-atmi_pifgen-pifgenfile=pifdefs.cpp -O3 -I$ATMI_INC

echo g++ -o reduction Reduction.o pifdefs.cpp -g -O3 -lelf -L$ATMI_RUNTIME_PATH/lib -latmi_runtime -L$HSA_TEST_RUNTIME_PATH/lib -lhsa-runtime64 -I$ATMI_INC -I$HSA_TEST_RUNTIME_PATH/include
g++ -o reduction Reduction.o pifdefs.cpp -g -O3 -lelf -L$ATMI_RUNTIME_PATH/lib -latmi_runtime -L$HSA_TEST_RUNTIME_PATH/lib -lhsa-runtime64 -I$ATMI_INC -I$HSA_TEST_RUNTIME_PATH/include

#  Execute
echo
echo LD_LIBRARY_PATH=$HSA_TEST_RUNTIME_PATH/lib:$LD_LIBRARY_PATH ./reduction 
LD_LIBRARY_PATH=$HSA_TEST_RUNTIME_PATH/lib:$LD_LIBRARY_PATH ./reduction 
#gdb reduction
