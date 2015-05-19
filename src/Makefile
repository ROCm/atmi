#Set HSA Environment variables
HSA_RUNTIME_PATH=/opt/hsa
HSA_LIBHSAIL_PATH=/opt/hsa/lib
HSA_LLVM_PATH=/opt/amd/cloc/bin

AR=ar
ARFLAGS=-cvq
CC=gcc
CFLAGS=-O2 -c -fpic -g
INC_FLAGS=-I../include -I${HSA_RUNTIME_PATH}/include 
LIB_FLAGS=-L${HSA_RUNTIME_PATH}/lib -lhsa-runtime64 -Wl,--no-as-needed -Wl,-rpath=${HSA_RUNTIME_PATH}/lib

OBJS=snk.o snk_cpu_task.o
LIB=libsnk.so
LIBDIR=../lib

all: $(LIB)

$(LIB): $(OBJS)
	$(CC) -shared $(LIB_FLAGS) -o $@ $^ 	
#	$(AR) $(ARFLAGS) $@ $^

%.o: %.c ../include/*.h
	$(CC) -o $@ $(INC_FLAGS) $(CFLAGS) $< $(LIB_FLAGS)

install: $(LIB)
	cp $^ $(LIBDIR)

.PHONY: clean

clean:
	rm -rf *.o *.a *.so
