#Set HSA Environment variables
HSA_RUNTIME_PATH ?= /opt/hsa
HSA_LIBHSAIL_PATH ?= /opt/hsa/lib
CLOC_PATH ?= /usr/bin
ATMI_RUNTIME_PATH ?= /opt/amd/atmi
ATMI_INC=${ATMI_RUNTIME_PATH}/include

CC=g++
CFLAGS=-O3 -g -std=c++11
INC_FLAGS=-I${ATMI_INC} -I${HSA_RUNTIME_PATH}/include -I. 

LIBS=-latmi_runtime -lhsa-runtime64 -lelf
LIB_FLAGS=-L${ATMI_RUNTIME_PATH}/lib -L${HSA_RUNTIME_PATH}/lib

OBJS = hello

.PHONY: clean all

all: $(OBJS)

hello: hw.cpp hw.cl hw_structs.h
	$(CLOC_PATH)/cloc.sh -brig -clopts "-I. -I${ATMI_RUNTIME_PATH}/include" -opt 2 hw.cl
	$(CC) -o $@ hw.cpp $(CFLAGS) $(LIBS) $(LIB_FLAGS) $(INC_FLAGS)

clean:
	rm -rf *.o *.brig *.hsaco $(OBJS)

