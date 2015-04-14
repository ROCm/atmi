#!/bin/bash

#  Call buildrun.sh as follows

#  ./buildrun 
#  ./buildrun cpp
#  ./buildrun f
#

#  Set HSA Environment variables
[ -z $HSA_RUNTIME_PATH ] && HSA_RUNTIME_PATH=/opt/hsa
[ -z $HSA_LLVM_PATH ] && HSA_LLVM_PATH=/opt/amd/cloc/bin
export LD_LIBRARY_PATH=$HSA_RUNTIME_PATH/lib

#  First compile the acclerated functions to create hw.o
echo 
if [ "$1" == "f" ] ; then 
   echo $HSA_LLVM_PATH/snack.sh -q -fort -c hw.cl 
   $HSA_LLVM_PATH/snack.sh -q -fort -c hw.cl 
else
   echo $HSA_LLVM_PATH/snack.sh -q -c hw.cl 
   $HSA_LLVM_PATH/snack.sh -q -c hw.cl 
fi

#  Compile the main program and link to hw.o
#  Main program can be c, cpp, or fotran
echo 
if [ "$1" == "cpp" ] ; then 
   echo "g++ -o HelloWorld hw.o HelloWorld.cpp -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64  "
   g++ -o HelloWorld hw.o HelloWorld.cpp -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64  
elif [ "$1" == "f" ] ; then 
   echo "f95 -o HelloWorld hw.o HelloWorld.f -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64  "
   f95 -o HelloWorld hw.o HelloWorld.f -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64 
else
   echo "gcc -o HelloWorld hw.o HelloWorld.c -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64 "
   gcc -o HelloWorld hw.o HelloWorld.c -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64 
fi

echo 
echo ./HelloWorld
./HelloWorld
