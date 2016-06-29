#Set HSA Environment variables
HSA_RUNTIME_PATH ?= /opt/hsa
HSA_LIBHSAIL_PATH ?= /opt/hsa/lib
CLOC_PATH ?= /usr/bin
ATMI_RUNTIME_PATH ?= /opt/amd/atmi
ATMI_INC=${ATMI_RUNTIME_PATH}/include

CC=g++
CFLAGS=-O3 -g -std=c++11
INC_FLAGS=-I${ATMI_INC} -I${HSA_RUNTIME_PATH}/include -I. 

TARGET_DEVICE=fiji
LLVM_BIN_DIR=/opt/amd/llvm/bin
LIBCLC_DIR=$(HOME)/git/libclc
CLC=$(LLVM_BIN_DIR)/clang
CL_FLAGS=-Dcl_clang_storage_class_specifiers
CL_INCP=-include $(LIBCLC_DIR)/generic/include/clc/clc.h -I $(LIBCLC_DIR)/generic/include/
CL_BITL=-Xclang -mlink-bitcode-file -Xclang $(LIBCLC_DIR)/built_libs/amdgcn--amdhsa.bc
CL_SRC=hw.cl
CL_OBJ=$(CL_SRC:.cl=.hsaco)

LIBS=-latmi_runtime -lhsa-runtime64 -lelf
LIB_FLAGS=-L${ATMI_RUNTIME_PATH}/lib -L${HSA_RUNTIME_PATH}/lib

OBJS = hello

.PHONY: clean all

all: $(OBJS)

hello: hw.cpp hw.cl 
	$(CLOC_PATH)/cloc.sh -vv -opt 2  hw.cl
	#$(CLC) -O0 -x cl -Xclang -cl-std=CL2.0 $(CL_BITL) $(CL_INCP) $(CL_FLAGS) -target amdgcn--amdhsa -mcpu=$(TARGET_DEVICE) $(CL_SRC) -o $(CL_OBJ)
	$(CC) -o $@ hw.cpp $(CFLAGS) $(LIBS) $(LIB_FLAGS) $(INC_FLAGS)

clean:
	rm -rf *.o *.brig *.hsaco $(OBJS)

test:
	./hello
