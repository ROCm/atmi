#
#MIT License 
#
#Copyright © 2016 Advanced Micro Devices, Inc.  
#
#Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software
#without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
#persons to whom the Software is furnished to do so, subject to the following conditions:
#
#The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
#
#THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
#PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
#OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
ATMI_RUNTIME_PATH ?= /opt/rocm/libatmi
ATMI_INC=${ATMI_RUNTIME_PATH}/include

#Set HSA Environment variables
HSA_RUNTIME_PATH ?= /opt/rocm

#Set LC Environment variables
AMDLLVM ?= /opt/amd/llvm

#MCPU
MCPU ?= $(shell mymcpu)

# Kernel format
BRIG ?= 0
ifeq ($(BRIG),1)
	BRIGCLOPTS=-brig
	BRIGCPPOPTS=-DUSE_BRIG
endif

# Kernel compiler
CLC ?= 1

# CLOC
CLOC_PATH ?= /usr/bin
CLOCOPTS = -vv
CLOCOPTS += -clopts "-v"

# ROCm-Device-lib
BITCODE_LIB ?= ${ROCMLIB}/dist/lib

# GPU compiler
CLCC=$(AMDLLVM)/bin/clang
CLCFLAGS = -x cl -Xclang -cl-std=CL2.0 -Xclang -finclude-default-header
CLCFLAGS += -target amdgcn--amdhsa
CLCFLAGS += -mcpu=$(MCPU)
CLCFLAGS += -Xclang -mlink-bitcode-file -Xclang $(BITCODE_LIB)/opencl.amdgcn.bc
CLCFLAGS += -Xclang -mlink-bitcode-file -Xclang $(BITCODE_LIB)/ockl.amdgcn.bc
CLCFLAGS += -Xclang -mlink-bitcode-file -Xclang $(BITCODE_LIB)/ocml.amdgcn.bc
CLCFLAGS += -Xclang -mlink-bitcode-file -Xclang $(BITCODE_LIB)/irif.amdgcn.bc

# Host compiler
CXX = g++
CXXFLAGS =-O3 -g -std=c++11
#CXXFLAGS += -v

INC_FLAGS=-I${ATMI_INC} -I${HSA_RUNTIME_PATH}/include -I.

LIBS=-latmi_runtime
LIB_FLAGS=-L${ATMI_RUNTIME_PATH}/lib -L${HSA_RUNTIME_PATH}/lib


OBJS = eps

.PHONY: clean all

all: $(OBJS)

eps: eps.cpp nullKernel.cl 
ifeq ($(CLC),1)
	$(CLOC_PATH)/cloc.sh ${BRIGCLOPTS} -opt 2 nullKernel.cl
else
	$(CLCC) ${CLCFLAGS} -o nullKernel.hsaco nullKernel.cl
endif
	$(CXX) -o $@ eps.cpp $(BRIGCPPOPTS) $(CXXFLAGS) $(LIBS) $(LIB_FLAGS) $(INC_FLAGS)

clean:
	rm -rf *.o *.brig *.hsaco $(OBJS)

test:
	ATMI_DEPENDENCY_SYNC_TYPE=ATMI_SYNC_CALLBACK ./eps 2 15
