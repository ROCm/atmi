#Set HSA Environment variables
HSA_RUNTIME_PATH ?= /opt/rocm
CLOC_PATH ?= /usr/bin
ATMI_RUNTIME_PATH ?= /opt/rocm/libatmi
ATMI_INC=${ATMI_RUNTIME_PATH}/include
BRIG ?= 0

ifeq ($(BRIG),1)
	BRIGCLOPTS=-brig
	BRIGCPPOPTS=-DUSE_BRIG
endif

CC=g++
CFLAGS=-O3 -g -std=c++11
INC_FLAGS=-I${ATMI_INC} -I${HSA_RUNTIME_PATH}/include -I. 

LIBS=-latmi_runtime 
LIB_FLAGS=-L${ATMI_RUNTIME_PATH}/lib -L${HSA_RUNTIME_PATH}/lib

OBJS = eps

.PHONY: clean all

all: $(OBJS)

eps: eps.cpp nullKernel.cl 
	$(CLOC_PATH)/cloc.sh ${BRIGCLOPTS} -opt 2 nullKernel.cl
	$(CC) -o $@ eps.cpp $(BRIGCPPOPTS) $(CFLAGS) $(LIBS) $(LIB_FLAGS) $(INC_FLAGS)

clean:
	rm -rf *.o *.brig *.hsaco $(OBJS)

test:
	ATMI_DEPENDENCY_SYNC_TYPE=ATMI_SYNC_CALLBACK ./eps 2 15
