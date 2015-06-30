#!/bin/bash
set -e
#  Set HSA Environment variables
[ -z $HSA_RUNTIME_PATH ] && HSA_RUNTIME_PATH=/opt/hsa
[ -z $HSA_LIBHSAIL_PATH ] && HSA_LIBHSAIL_PATH=/opt/hsa/lib
[ -z $HSA_LLVM_PATH ] && HSA_LLVM_PATH=/opt/amd/cloc/bin
[ -z $ATMI_RUNTIME_PATH ] && ATMI_RUNTIME_PATH=/opt/amd/atmi
ATMI_INC=$ATMI_RUNTIME_PATH/include
export LD_LIBRARY_PATH=$HSA_RUNTIME_PATH/lib:$ATMI_RUNTIME_PATH/lib:$LD_LIBRARY_PATH


# Do not compile accelerated functions separately. This script will be invoked by the GCC plugin itself.
#echo cl2brigh.sh -v -gccopt 3 hw.cl
#cl2brigh.sh -v -gccopt 3 hw.cl

# Compile Main and generate the PIF definitions for host and accelerated functions
# in HelloWorld.cpp.pifdefs.c
echo 
if [ -f hello ] ; then rm hello ; fi
echo g++ -c -o HelloWorld.o HelloWorld.cpp -g -fplugin=atmi_pifgen.so -fplugin-arg-atmi_pifgen-clfile=hw.cl -O3 -I$ATMI_INC
g++ -c -o HelloWorld.o HelloWorld.cpp -g -fplugin=atmi_pifgen.so -fplugin-arg-atmi_pifgen-clfile=hw.cl -O3 -I$ATMI_INC

echo g++ -o hello HelloWorld.o HelloWorld.cpp.pifdefs.c -g -O3 -lelf -L$ATMI_RUNTIME_PATH/lib -latmi_runtime -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64 -I$ATMI_INC -I$HSA_RUNTIME_PATH/include
g++ -o hello HelloWorld.o HelloWorld.cpp.pifdefs.c -g -O3 -lelf -L$ATMI_RUNTIME_PATH/lib -latmi_runtime -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64 -I$ATMI_INC -I$HSA_RUNTIME_PATH/include

#  Execute
echo
echo ./hello 
./hello 
