# ===--------------------------------------------------------------------------
#               ATMI (Asynchronous Task and Memory Interface)
#
#  This file is distributed under the MIT License. See LICENSE.txt for details.
# ===--------------------------------------------------------------------------
SHELL=/bin/bash

#BUILDROOT
BUILDROOT = $(shell pwd | sed 's/examples.*$$//')
#$(info BUILDROOT $(BUILDROOT))
-include $(BUILDROOT)/atmi-config.mak

#Set ATMI Environment variables
ATMI_RUNTIME_PATH ?= /opt/rocm/atmi

ATMI_BIN ?= ${ATMI_RUNTIME_PATH}/bin
ATMI_INC ?= ${ATMI_RUNTIME_PATH}/include
ATMI_LIB ?= ${ATMI_RUNTIME_PATH}/lib

#Set HSA Environment variables
HSA_RUNTIME_PATH ?= /opt/rocm/hsa
#Set ROCM device environment variables
ROCM_DEVICE_PATH ?= /opt/rocm

#Set LC Environment variables
AMDLLVM ?= /opt/amd/llvm

AMDGPU_TARGET_TRIPLE ?= amdgpu--amdhsa

#MCPU
MCPU ?= $(shell ${ATMI_RUNTIME_PATH}/bin/mygpu)
#$(info MCPU $(MCPU))

# Kernel compiler
CLC ?= 1

INC_FLAGS=-I${ATMI_RUNTIME_PATH}/include -I${HSA_RUNTIME_PATH}/include -I.

# CLOC
CLOC_PATH ?= ${ATMI_RUNTIME_PATH}/bin
CLOCOPTS = -vv -aomp ${AMDLLVM} -triple ${AMDGPU_TARGET_TRIPLE} -libgcn ${ROCM_DEVICE_PATH}
CLOCOPTS += -clopts "$(INC_FLAGS) -O2 -v"

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

LIBS=-latmi_runtime
LIB_FLAGS=-L${ATMI_RUNTIME_PATH}/lib -L${HSA_RUNTIME_PATH}/lib

OBJS = nw

.PHONY: clean all

all: $(OBJS)

nw: nw.cpp nw.cl 
ifeq ($(CLC),1)
	$(CLOC_PATH)/cloc.sh ${CLOCOPTS} -opt 2 nw.cl
else
	$(CLCC) ${CLCFLAGS} -o nw.hsaco nw.cl
endif
	$(CXX) -o $@ nw.cpp $(CXXFLAGS) $(LIBS) $(LIB_FLAGS) $(INC_FLAGS)

clean:
	rm -rf *.o *.hsaco $(OBJS)

test:
	env LD_LIBRARY_PATH=${ATMI_RUNTIME_PATH}/lib:${HSA_RUNTIME_PATH}/lib:${LD_LIBRARY_PATH} ./nw 512 10 10
