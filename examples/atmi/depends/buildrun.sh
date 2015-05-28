#!/bin/bash

#  Set HSA Environment variables
[ -z $HSA_RUNTIME_PATH ] && HSA_RUNTIME_PATH=/opt/hsa
[ -z $HSA_LIBHSAIL_PATH ] && HSA_LIBHSAIL_PATH=/opt/hsa/lib
[ -z $HSA_LLVM_PATH ] && HSA_LLVM_PATH=/opt/amd/cloc/bin
[ -z $SNACK_RUNTIME_PATH ] && SNACK_RUNTIME_PATH=$HOME/git/CLOC.ashwinma

export LD_LIBRARY_PATH=$HSA_RUNTIME_PATH/lib:$SNACK_RUNTIME_PATH/lib:$LD_LIBRARY_PATH
echo $LD_LIBRARY_PATH

# Compile accelerated functions
echo 
if [ -f csquares.o ] ; then rm csquares.o ; fi
echo snack.sh -snk $SNACK_RUNTIME_PATH -v -gccopt 3 -c csquares.cl 
snack.sh -snk $SNACK_RUNTIME_PATH -v -gccopt 3 -c csquares.cl 

# Compile Main and link to accelerated functions in csquares.o
echo 
if [ -f csquares ] ; then rm csquares ; fi
echo "g++ -o csquares csquares.o csquares.cpp -lsnk -L$SNACK_RUNTIME_PATH/lib -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64 -lelf "
#g++ -o csquares csquares.o csquares.cpp -lsnk -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64 -lelf 
g++ -o csquares csquares.o csquares.cpp -lsnk -L$SNACK_RUNTIME_PATH/lib -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64 -lelf 

#  Execute
echo
echo ./csquares
./csquares
