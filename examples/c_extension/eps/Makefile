#Set HSA Environment variables
HSA_RUNTIME_PATH ?= /opt/hsa
HSA_LIBHSAIL_PATH ?= /opt/hsa/lib
HSA_LLVM_PATH ?= /opt/amd/cloc/bin
ATMI_RUNTIME_PATH ?= /opt/amd/atmi
ATMI_INC=${ATMI_RUNTIME_PATH}/include
PLUGIN_LIB  = ${ATMI_RUNTIME_PATH}/lib/atmi_pifgen.so

CC=g++
CFLAGS=-O3 -g
INC_FLAGS=-I${ATMI_INC} -I${HSA_RUNTIME_PATH}/include -I. 

LIBS=-latmi_runtime -lhsa-runtime64 
LIB_FLAGS=-L${ATMI_RUNTIME_PATH}/lib -L${HSA_RUNTIME_PATH}/lib

OBJS = eps

.PHONY: clean all

all: $(OBJS)

eps: eps.cpp 
	#$(CC) -c -o HelloWorld.o $^ -fplugin=$(PLUGIN_LIB) -fplugin-arg-atmi_pifgen-brigfile=hw.brig -fplugin-arg-atmi_pifgen-pifgenfile=pifdefs.cpp $(CFLAGS) $(INC_FLAGS)
	#$(CC) -c -o HelloWorld.o $^ -fplugin=$(PLUGIN_LIB) -fplugin-arg-atmi_pifgen-clfile=hw.cl -fplugin-arg-atmi_pifgen-pifgenfile=pifdefs.cpp $(CFLAGS) $(INC_FLAGS)
	$(CC) -c -o nullKernel.o $^ -fplugin=$(PLUGIN_LIB) -fplugin-arg-atmi_pifgen-clfile=nullKernel.cl -fplugin-arg-atmi_pifgen-pifgenfile=pifdefs.cpp $(CFLAGS) $(INC_FLAGS)
	#$(CC) -c -o nullKernel.o $^ -fplugin=$(PLUGIN_LIB) -fplugin-arg-atmi_pifgen-clfile=nullKernel.cl -fplugin-arg-atmi_pifgen-pifgenfile=pifdefs.cpp -fplugin-arg-atmi_pifgen-jitcompile=false $(CFLAGS) $(INC_FLAGS)
	#$(CC) -o $@ HelloWorld.o hw.c $(CFLAGS) $(LIBS) $(LIB_FLAGS) $(INC_FLAGS)
	$(CC) -o $@ nullKernel.o pifdefs.cpp $(CFLAGS) $(LIBS) $(LIB_FLAGS) $(INC_FLAGS)

clean:
	rm -rf *.o *.brig *.hof *_brig.h *pifdefs.c* $(OBJS)

run: eps
	ATMI_DEPENDENCY_SYNC_TYPE=ATMI_SYNC_CALLBACK ATMI_MAX_HSA_SIGNALS=8 ./eps 2 15
