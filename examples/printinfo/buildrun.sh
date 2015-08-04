#!/bin/bash
set -e
#  Set HSA Environment variables
[ -z $HSA_RUNTIME_PATH ] && HSA_RUNTIME_PATH=/opt/hsa
[ -z $HSA_LLVM_PATH ] && HSA_LLVM_PATH=/opt/amd/cloc/bin
[ -z $ATMI_RUNTIME_PATH ] && ATMI_RUNTIME_PATH=/opt/amd/atmi
ATMI_INC=$ATMI_RUNTIME_PATH/include
#echo export LD_LIBRARY_PATH=$HSA_RUNTIME_PATH/lib:$ATMI_RUNTIME_PATH/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=$HSA_RUNTIME_PATH/lib:$ATMI_RUNTIME_PATH/lib:$LD_LIBRARY_PATH

# Compile Main and generate the PIF definitions for host and accelerated functions
echo 
if [ -f atmi_info ] ; then rm atmi_info ; fi
echo g++ -c -o printinfo.o printinfo.cpp -g -fplugin=atmi_pifgen.so -fplugin-arg-atmi_pifgen-pifgenfile=pifdefs.cpp -O3 -I$ATMI_INC
g++ -c -o printinfo.o printinfo.cpp -g -fplugin=atmi_pifgen.so -fplugin-arg-atmi_pifgen-pifgenfile=pifdefs.cpp -O3 -I$ATMI_INC

echo g++ -o atmi_info printinfo.o pifdefs.cpp -g -O3 -lelf -L$ATMI_RUNTIME_PATH/lib -latmi_runtime -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64 -I$ATMI_INC -I$HSA_RUNTIME_PATH/include
g++ -o atmi_info printinfo.o pifdefs.cpp -g -O3 -lelf -L$ATMI_RUNTIME_PATH/lib -latmi_runtime -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64 -I$ATMI_INC -I$HSA_RUNTIME_PATH/include

#  Execute
echo
echo ./atmi_info
./atmi_info
