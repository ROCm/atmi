#Set ROCM/HSA Environment variables
ROCM_RUNTIME_PATH=/opt/rocm

AR=ar
ARFLAGS=-cvq
CC=g++
CFLAGS=-O2 -c -fpic -g -std=c++11
ATMILIB_RUNTIME_PATH=../..
INC_FLAGS=-I${ATMILIB_RUNTIME_PATH}/include -I${ROCM_RUNTIME_PATH}/hsa/include -I./include 
LIB_FLAGS=-L${ROCM_RUNTIME_PATH}/lib -lhsa-runtime64 

OBJS=atl_bindthread.o atl_profile.o atl_rt.o atl_cputask.o atl_synckernel.o ATLQueue.o ATLMachine.o ATLData.o

LIB=libatmi_runtime.so
LIBDIR=${ATMILIB_RUNTIME_PATH}/lib

.PHONY: clean all

ATMILIB_INC=${ATMILIB_RUNTIME_PATH}/include

all: $(LIBDIR)/$(LIB)

clean:
	rm -rf *.o *.a $(LIBDIR)/${LIB}

install:
	rm -rf *.o 

$(LIBDIR)/$(LIB): $(OBJS) 
	mkdir -p $(LIBDIR)
	$(CC) -g -shared -o $@ $^ $(LIB_FLAGS) 
#	$(AR) $(ARFLAGS) $@ $^

%.o: %.c* ${ATMILIB_RUNTIME_PATH}/include/*.h include/*.h
	$(CC) -o $@ $(INC_FLAGS) $(CFLAGS) $< 

