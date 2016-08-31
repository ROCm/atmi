#Set HSA Environment variables
HSA_RUNTIME_PATH ?= /opt/rocm
ATMI_RUNTIME_PATH ?= /opt/rocm/libatmi
ATMI_INC=${ATMI_RUNTIME_PATH}/include
PLUGIN_LIB  = ${ATMI_RUNTIME_PATH}/lib/atmi_pifgen.so

CC=g++
CFLAGS=-O3 -g -std=c++11
INC_FLAGS=-I${ATMI_INC} -I${HSA_RUNTIME_PATH}/hsa/include -I. 

LIBS=-latmi_runtime 
LIB_FLAGS=-L${ATMI_RUNTIME_PATH}/lib -L${HSA_RUNTIME_PATH}/lib

OBJS = hello

.PHONY: clean all

all: $(OBJS)

hello: HelloWorld.cpp 
	#$(CC) -c -o HelloWorld.o $^ -fplugin=$(PLUGIN_LIB) -fplugin-arg-atmi_pifgen-brigfile=hw.brig -fplugin-arg-atmi_pifgen-pifgenfile=pifdefs.cpp $(CFLAGS) $(INC_FLAGS)
	$(CC) -c -o HelloWorld.o $^ -fplugin=$(PLUGIN_LIB) -fplugin-arg-atmi_pifgen-clfile=hw.cl -fplugin-arg-atmi_pifgen-pifgenfile=pifdefs.cpp $(CFLAGS) $(INC_FLAGS)
	#$(CC) -c -o HelloWorld.o $^ -fplugin=$(PLUGIN_LIB) -fplugin-arg-atmi_pifgen-clfile=hw.cl -fplugin-arg-atmi_pifgen-pifgenfile=pifdefs.cpp -fplugin-arg-atmi_pifgen-jitcompile=false $(CFLAGS) $(INC_FLAGS)
	#$(CC) -o $@ HelloWorld.o hw.c $(CFLAGS) $(LIBS) $(LIB_FLAGS) $(INC_FLAGS)
	$(CC) -o $@ HelloWorld.o pifdefs.cpp $(CFLAGS) $(LIBS) $(LIB_FLAGS) $(INC_FLAGS)

clean:
	rm -rf *.o *.brig *.hof *_brig.h *pifdefs.c* $(OBJS)

