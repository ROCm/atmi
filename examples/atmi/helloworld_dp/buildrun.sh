#!/bin/bash
set -e
#  Set HSA Environment variables
[ -z $HSA_TEST_RUNTIME_PATH ] && HSA_TEST_RUNTIME_PATH=/opt/hsa.1_1T/
[ -z $HSA_LIBHSAIL_PATH ] && HSA_LIBHSAIL_PATH=$HSA_TEST_RUNTIME_PATH/lib
[ -z $ATMI_RUNTIME_PATH ] && ATMI_RUNTIME_PATH=/opt/amd/atmi
ATMI_INC=$ATMI_RUNTIME_PATH/include


# Optionally compile accelerated functions separately. This script may also be invoked by the GCC plugin itself.
#echo cl2brigh.sh hw.cl
#cl2brigh.sh hw.cl
#echo $HSA_LLVM_PATH/cloc.sh -clopts \"-I. -I${ATMI_RUNTIME_PATH}/include\" hw.cl
#$HSA_LLVM_PATH/cloc.sh -clopts "-I. -I${ATMI_RUNTIME_PATH}/include" hw.cl

# Compile Main and generate the PIF definitions for host and accelerated functions
echo 
if [ -f hello ] ; then rm hello ; fi
echo g++ -c -o HelloWorld.o HelloWorld.cpp -g -fplugin=atmi_pifgen.so -fplugin-arg-atmi_pifgen-clfile=hw.cl -fplugin-arg-atmi_pifgen-pifgenfile=pifdefs.cpp -O3 -I$ATMI_INC
g++ -c -o HelloWorld.o HelloWorld.cpp -g -fplugin=atmi_pifgen.so -fplugin-arg-atmi_pifgen-clfile=hw.cl -fplugin-arg-atmi_pifgen-pifgenfile=pifdefs.cpp -O3 -I$ATMI_INC

echo g++ -o hello HelloWorld.o pifdefs.cpp -g -O3 -lelf -L$ATMI_RUNTIME_PATH/lib -latmi_runtime -L$HSA_TEST_RUNTIME_PATH/lib -lhsa-runtime64 -I$ATMI_INC -I$HSA_TEST_RUNTIME_PATH/include
g++-4.9 -o hello HelloWorld.o pifdefs.cpp -g -O3 -lelf -L$ATMI_RUNTIME_PATH/lib -latmi_runtime -L$HSA_TEST_RUNTIME_PATH/lib -lhsa-runtime64 -I$ATMI_INC -I$HSA_TEST_RUNTIME_PATH/include
#g++ -o hello HelloWorld.o HelloWorld.cpp.pifdefs.cpp -g -O3 -lelf -L$ATMI_RUNTIME_PATH/lib -latmi_runtime -L$HSA_TEST_RUNTIME_PATH/lib -lhsa-runtime64 -I$ATMI_INC -I$HSA_TEST_RUNTIME_PATH/include

#  Execute
echo
echo LD_LIBRARY_PATH=$HSA_TEST_RUNTIME_PATH/lib:$LD_LIBRARY_PATH ./hello 
LD_LIBRARY_PATH=$HSA_TEST_RUNTIME_PATH/lib:$LD_LIBRARY_PATH ./hello 
