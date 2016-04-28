#Set HSA Environment variables
#HSA_RUNTIME_PATH=/opt/hsa
#HSA_RUNTIME_PATH=${HOME}/opt/hsa
HSA_RUNTIME_PATH=/opt/rocm/hsa

AR=ar
ARFLAGS=-cvq
CC=g++
CFLAGS=-O2 -c -fpic -g -std=c++11
ATMILIB_RUNTIME_PATH=../..
INC_FLAGS=-I${ATMILIB_RUNTIME_PATH}/include -I${HSA_RUNTIME_PATH}/include -I./include 
LIB_FLAGS=-L${HSA_RUNTIME_PATH}/lib -lhsa-runtime64 -Wl,--no-as-needed -Wl,-rpath=${HSA_RUNTIME_PATH}/lib

OBJS=atl_bindthread.o atl_profile.o atl_rt.o atl_cputask.o atl_synckernel.o ATLQueue.o 

LIB=libatmi_runtime.so
LIBDIR=${ATMILIB_RUNTIME_PATH}/lib

.PHONY: clean all

ATMILIB_INC=${ATMILIB_RUNTIME_PATH}/include
CPLUSPLUS=g++
CC_FLAGS=-O3 -lelf -L${ATMILIB_RUNTIME_PATH}/lib -lsnk -L${HSA_RUNTIME_PATH}/lib -lhsa-runtime64 -I${ATMILIB_INC} -I${HSA_RUNTIME_PATH}/include

all: $(LIBDIR)/$(LIB)

clean:
	rm -rf *.o *.a $(LIBDIR)/${LIB}

install:
	rm -rf *.o 

$(LIBDIR)/$(LIB): $(OBJS) 
	mkdir -p $(LIBDIR)
	$(CC) -g -shared $(LIB_FLAGS) -o $@ $^ 	
#	$(AR) $(ARFLAGS) $@ $^

%.o: %.c* ${ATMILIB_RUNTIME_PATH}/include/*.h include/*.h
	$(CC) -o $@ $(INC_FLAGS) $(CFLAGS) $< $(LIB_FLAGS)

