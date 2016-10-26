#MIT License
#
#Copyright © 2016 Advanced Micro Devices, Inc.
#
#Permission is hereby granted, free of charge, to any person obtaining a copy of
#this software and associated documentation files (the "Software"), to deal in
#the Software
#without restriction, including without limitation the rights to use, copy,
#modify, merge, publish, distribute, sublicense, and/or sell copies of the
#Software, and to permit
#persons to whom the Software is furnished to do so, subject to the following
#conditions:
#
#The above copyright notice and this permission notice shall be included in all
#copies or substantial portions of the Software.
#
#THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#FITNESS FOR A PARTICULAR
#PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
#BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
#CONTRACT, TORT OR
#OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
#OR OTHER DEALINGS IN THE SOFTWARE.

#!/bin/bash
set -e
#  Set HSA Environment variables
[ -z $HSA_RUNTIME_PATH ] && HSA_RUNTIME_PATH=/home/aaji/opt/hsa-nov15
#[ -z $HSA_RUNTIME_PATH ] && HSA_RUNTIME_PATH=/opt/hsa
[ -z $HSA_LIBHSAIL_PATH ] && HSA_LIBHSAIL_PATH=/opt/hsa/lib
[ -z $HSA_LLVM_PATH ] && HSA_LLVM_PATH=/opt/amd/cloc/bin
[ -z $ATMI_RUNTIME_PATH ] && ATMI_RUNTIME_PATH=/opt/amd/atmi
ATMI_INC=$ATMI_RUNTIME_PATH/include

echo 
#export VT_MODE="STAT:TRACE"
if [ -f kps ] ; then rm kps ; fi
echo g++ -c -o nullKernel.o kps.cpp -g -fplugin=atmi_pifgen.so -fplugin-arg-atmi_pifgen-clfile=nullKernel.cl -O3 -I$ATMI_INC
#vtc++ -vt:inst compinst -c -o nullKernel.o kps.cpp -g -fplugin=atmi_pifgen.so -fplugin-arg-atmi_pifgen-clfile=nullKernel.cl -O3 -I$ATMI_INC
g++ -c -o nullKernel.o kps.cpp -g -fplugin=atmi_pifgen.so -fplugin-arg-atmi_pifgen-clfile=nullKernel.cl -O3 -I$ATMI_INC

echo g++ -o kps nullKernel.o kps.cpp.pifdefs.c -g -O3 -lelf -L$ATMI_PATH/lib -latmi_runtime -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64 -I$ATMI_INC -I$HSA_RUNTIME_PATH/include
#vtc++ -vt:inst compinst -o kps nullKernel.o kps.cpp.pifdefs.c -g -O3 -lelf -L$ATMI_PATH/lib -latmi_runtime -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64 -I$ATMI_INC -I$HSA_RUNTIME_PATH/include
g++ -o kps nullKernel.o kps.cpp.pifdefs.c -g -O3 -lelf -L$ATMI_PATH/lib -latmi_runtime -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64 -I$ATMI_INC -I$HSA_RUNTIME_PATH/include

#  Execute
echo
echo ./kps 
./kps 

