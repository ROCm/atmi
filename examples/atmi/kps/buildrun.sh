#!/bin/bash
set -e
#  Set HSA Environment variables
[ -z $HSA_RUNTIME_PATH ] && HSA_RUNTIME_PATH=/opt/hsa
[ -z $HSA_LIBHSAIL_PATH ] && HSA_LIBHSAIL_PATH=/opt/hsa/lib
[ -z $HSA_LLVM_PATH ] && HSA_LLVM_PATH=/opt/amd/cloc/bin
[ -z $ATMI_RUNTIME_PATH ] && ATMI_RUNTIME_PATH=/opt/amd/atmi
ATMI_INC=$ATMI_RUNTIME_PATH/include

echo 
if [ -f kps ] ; then rm kps ; fi
echo g++ -c -o nullKernel.o kps.cpp -g -fplugin=atmi_pifgen.so -fplugin-arg-atmi_pifgen-clfile=nullKernel.cl -O3 -I$ATMI_INC
g++ -c -o nullKernel.o kps.cpp -g -fplugin=atmi_pifgen.so -fplugin-arg-atmi_pifgen-clfile=nullKernel.cl -O3 -I$ATMI_INC

echo g++ -o kps nullKernel.o kps.cpp.pifdefs.c -g -O3 -lelf -L$ATMI_PATH/lib -latmi_runtime -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64 -I$ATMI_INC -I$HSA_RUNTIME_PATH/include
g++ -o kps nullKernel.o kps.cpp.pifdefs.c -g -O3 -lelf -L$ATMI_PATH/lib -latmi_runtime -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64 -I$ATMI_INC -I$HSA_RUNTIME_PATH/include

#  Execute
echo
echo ./kps 
./kps 

