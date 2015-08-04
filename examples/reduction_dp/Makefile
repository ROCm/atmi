#Set HSA Environment variables
HSA_RUNTIME_PATH ?= /opt/hsa
HSA_TEST_RUNTIME_PATH ?= /opt/hsa.1_1T
HSA_LIBHSAIL_PATH ?= ${HSA_TEST_RUNTIME_PATH}/lib
HSA_LLVM_PATH ?= /opt/amd/cloc/bin
ATMI_RUNTIME_PATH ?= /opt/amd/atmi
ATMI_INC=${ATMI_RUNTIME_PATH}/include
PLUGIN_LIB  = ${ATMI_RUNTIME_PATH}/lib/atmi_pifgen.so

CC=g++
CFLAGS=-O3 -g
INC_FLAGS=-I${ATMI_INC} -I${HSA_TEST_RUNTIME_PATH}/include -I. 

LIBS=-latmi_runtime -lhsa-runtime64 -lelf
LIB_FLAGS=-L${ATMI_RUNTIME_PATH}/lib -L${HSA_TEST_RUNTIME_PATH}/lib

OBJS = reduction

.PHONY: clean all

all: $(OBJS)

reduction: Reduction.cpp
	$(CC) -c -o Reduction.o $^ -fplugin=$(PLUGIN_LIB) -fplugin-arg-atmi_pifgen-clfile=reduction.cl -fplugin-arg-atmi_pifgen-pifgenfile=pifdefs.cpp $(CFLAGS) $(INC_FLAGS)
	$(CC) -o $@ Reduction.o pifdefs.cpp $(CFLAGS) $(LIBS) $(LIB_FLAGS) $(INC_FLAGS)

clean:
	rm -rf *.o *.brig *_brig.h *pifdefs.c* $(OBJS)



