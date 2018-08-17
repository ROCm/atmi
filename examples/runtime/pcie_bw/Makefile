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

BIBW ?= 0
ifeq ($(BIBW),1)
	BIBW_FLAG=-DBIBW
else
	BIBW_FLAG=
endif

# Host compiler
CXX = g++
CXXFLAGS =-O3 -g -std=c++11 $(BIBW_FLAG)
#CXXFLAGS += -v

INC_FLAGS=-I${ATMI_INC} -I.

LIBS=-latmi_runtime
LIB_FLAGS=-L${ATMI_RUNTIME_PATH}/lib -L${HSA_RUNTIME_PATH}/lib

OBJS = pcie_bw

.PHONY: clean all

all: $(OBJS)

pcie_bw: pcie_bw.cpp 
	$(CXX) -o $@ pcie_bw.cpp $(CXXFLAGS) $(LIBS) $(LIB_FLAGS) $(INC_FLAGS)

clean:
	rm -rf *.o $(OBJS)

test:
	env LD_LIBRARY_PATH=${ATMI_RUNTIME_PATH}/lib:${HSA_RUNTIME_PATH}/lib:${LD_LIBRARY_PATH} ./pcie_bw
