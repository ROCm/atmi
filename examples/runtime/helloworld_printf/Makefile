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

# Kernel format
BRIG ?= 0
ifeq ($(BRIG),1)
	BRIGCLOPTS=-brig
	BRIGCPPOPTS=-DUSE_BRIG
endif

# Kernel compiler
CLC ?= 1

# Temp files
SAVETEMP ?= 0

INC_FLAGS=-I${ATMI_RUNTIME_PATH}/include -I${HSA_RUNTIME_PATH}/include -I.

# CLOC
CLOC_PATH ?= ${ATMI_RUNTIME_PATH}/bin

# CL options
CLOPTS = $(INC_FLAGS)
CLOPTS += -v
# Frontend optimization
ifneq ($(NOOPT),1)
CLOPTS += -O2
endif
# Temp file
ifneq ($(SAVETEMP),0)
CLOPTS += -save-temps
endif

# cloc options
CLOCOPTS = -vv -hcc2 ${AMDLLVM} -triple ${AMDGPU_TARGET_TRIPLE} -libgcn ${ROCM_DEVICE_PATH}
CLOCOPTS += -clopts "$(CLOPTS)"

# ROCm-Device-lib
BITCODE_LIB ?= ${ROCMLIB}/dist/lib

# GPU compiler
CLCC=$(AMDLLVM)/bin/clang

LLK=$(AMDLLVM)/bin/llvm-link
LLC=$(AMDLLVM)/bin/llc
LLD=$(AMDLLVM)/bin/lld

CLCFLAGS = -x cl -Xclang -cl-std=CL2.0 -Xclang -finclude-default-header
CLCFLAGS += -target amdgcn--amdhsa

ifeq ($(SAVETEMP),0)
CLCFLAGS += -mcpu=$(MCPU)

CLCFLAGS += -Xclang -mlink-bitcode-file -Xclang $(BITCODE_LIB)/opencl.amdgcn.bc
CLCFLAGS += -Xclang -mlink-bitcode-file -Xclang $(BITCODE_LIB)/ockl.amdgcn.bc
CLCFLAGS += -Xclang -mlink-bitcode-file -Xclang $(BITCODE_LIB)/ocml.amdgcn.bc
CLCFLAGS += -Xclang -mlink-bitcode-file -Xclang $(BITCODE_LIB)/irif.amdgcn.bc
else
CLCFLAGS += -emit-llvm -c

#LLFLAGS = -suppress-warnings
LLFLAGS += $(BITCODE_LIB)/opencl.amdgcn.bc
LLFLAGS += $(BITCODE_LIB)/ockl.amdgcn.bc
LLFLAGS += $(BITCODE_LIB)/ocml.amdgcn.bc
LLFLAGS += $(BITCODE_LIB)/irif.amdgcn.bc

LCFLAGS = -O2
LCFLAGS += -filetype=obj
LCFLAGS += -mtriple amdgcn--amdhsa
LCFLAGS += -mcpu=$(MCPU)

LDFLAGS = -flavor gnu -shared
endif


# CPU compiler
CC=$(AMDLLVM)/bin/clang
CCFLAGS = -g
#CCFLAGS += -v

# Host compiler
CXX = g++
CXXFLAGS = -g -std=c++11
#CXXFLAGS += -v

LIBS=-latmi_runtime
LIB_FLAGS=-L${ATMI_RUNTIME_PATH}/lib -L${HSA_RUNTIME_PATH}/lib

OBJS = hello

.PHONY: clean all

all: $(OBJS)

%.hsaco: %.cl $(INC_FILES)
ifeq ($(CLC),1)
	$(CLOC_PATH)/cloc.sh ${CLOCOPTS} ${BRIGCLOPTS} -opt 2 -o $@ $<
else
ifeq ($(SAVETEMP),0)
	$(CLCC) ${CLCFLAGS} -o $@ $<
else
	$(CLCC) ${CLCFLAGS} -o $*.bc $<
	$(LLK) -o $*.linked.bc $*.bc ${LLFLAGS}
	$(LLC) ${LCFLAGS} -o $*.o $*.linked.bc
	$(LLD) ${LDFLAGS} -o $@ $*.o
endif
endif
	@echo

%.o: %.cpp $(INC_FILES)
	$(CXX) $(CXXFLAGS) $(INC_FLAGS) -o $@ -c $<
	@echo

%.o: %.c $(INC_FILES)
	$(CC) $(CCFLAGS) $(INC_FLAGS) -o $@ -c $<
	@echo

hello: hw_gpu.hsaco hw_cpu.o hw_host.o
	$(CXX) -o $@ hw_host.o hw_cpu.o $(LIBS) $(LIB_FLAGS)

clean:
	rm -rf *.o *.brig *.i *.bc *.hsaco $(OBJS)

test:
	env LD_LIBRARY_PATH=${ATMI_RUNTIME_PATH}/lib:${HSA_RUNTIME_PATH}/lib:${LD_LIBRARY_PATH} ./hello
