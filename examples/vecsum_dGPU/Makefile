#Set HSA Environment variables
HSA_RUNTIME_PATH ?= /opt/hsa
HSA_LIBHSAIL_PATH ?= /opt/hsa/lib
ATMI_RUNTIME_PATH ?= /opt/amd/atmi-0.2
ATMI_INC=${ATMI_RUNTIME_PATH}/include
PLUGIN_LIB  = ${ATMI_RUNTIME_PATH}/lib/atmi_pifgen.so

CC=g++
CFLAGS=-O3 -g
INC_FLAGS=-I${ATMI_INC} -I${HSA_RUNTIME_PATH}/include -I. 

LIBS=-latmi_runtime -lhsa-runtime64 -lelf
LIB_FLAGS=-L${ATMI_RUNTIME_PATH}/lib -L${HSA_RUNTIME_PATH}/lib

OBJS = vecsum

.PHONY: clean all

all: $(OBJS)

vecsum: vecsum.cpp sumKernel.cl 
	#HSA_LLVM_PATH=$(HOME)/git/CLOC/bin $(CC) -c -o vecsum.o $^ -fplugin=$(PLUGIN_LIB) -fplugin-arg-atmi_pifgen-brigfile=sumKernel.brig -fplugin-arg-atmi_pifgen-pifgenfile=pifdefs.cpp $(CFLAGS) $(INC_FLAGS)
	HSA_LLVM_PATH=$(HOME)/git/CLOC/bin $(CC) -c -o vecsum.o $^ -fplugin=$(PLUGIN_LIB) -fplugin-arg-atmi_pifgen-clfile=sumKernel.cl -fplugin-arg-atmi_pifgen-pifgenfile=pifdefs.cpp $(CFLAGS) $(INC_FLAGS)
	#HSA_LLVM_PATH=$(HOME)/git/CLOC/bin $(CC) -c -o vecsum.o $^ -fplugin=$(PLUGIN_LIB) -fplugin-arg-atmi_pifgen-clfile=sumKernel.cl -fplugin-arg-atmi_pifgen-pifgenfile=pifdefs.cpp -fplugin-arg-atmi_pifgen-jitcompile=false $(CFLAGS) $(INC_FLAGS)
	#$(CC) -o $@ vecsum.o sumKernel.c $(CFLAGS) $(LIBS) $(LIB_FLAGS) $(INC_FLAGS)
	$(CC) -o $@ vecsum.o pifdefs.cpp $(CFLAGS) $(LIBS) $(LIB_FLAGS) $(INC_FLAGS)

clean:
	rm -rf *.o *.brig *.hof *_brig.h *pifdefs.c* $(OBJS)

