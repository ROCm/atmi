#Set HSA Environment variables
#HSA_RUNTIME_PATH=/opt/hsa
HSA_RUNTIME_PATH=/opt/hsa-oct15
HSA_LLVM_PATH=/opt/amd/cloc/bin

AR=ar
ARFLAGS=-cvq
CC=g++
CFLAGS=-O2 -c -fpic -g -std=c++11
INC_FLAGS=-I../include -I${HSA_RUNTIME_PATH}/include -I./include 
LIB_FLAGS=-L${HSA_RUNTIME_PATH}/lib -lhsa-runtime64 -Wl,--no-as-needed -Wl,-rpath=${HSA_RUNTIME_PATH}/lib

OBJS=atl_bindthread.o atl_profile.o atl_rt.o atl_cputask.o atl_synckernel.o #atl_rt2.o

LIB=libatmi_runtime.so
LIBDIR=../lib

.PHONY: clean all

ATMILIB_RUNTIME_PATH=..
ATMILIB_INC=${ATMILIB_RUNTIME_PATH}/include
CPLUSPLUS=g++
CC_FLAGS=-O3 -lelf -L${ATMILIB_RUNTIME_PATH}/lib -lsnk -L${HSA_RUNTIME_PATH}/lib -lhsa-runtime64 -I${ATMILIB_INC} -I${HSA_RUNTIME_PATH}/include

GENLIB=atmi_pifgen.so

PLUGIN_SOURCE_FILES= atl_pifgen_plugin.c
GCCPLUGINS_DIR:= $(shell $(CC) -print-file-name=plugin)
	CXXFLAGS+= -I$(GCCPLUGINS_DIR)/include -fPIC -O2 -I${ATMILIB_INC} ${INC_FLAGS}

all: $(LIBDIR)/$(GENLIB) $(LIBDIR)/$(LIB)

$(LIBDIR)/$(GENLIB): $(PLUGIN_SOURCE_FILES)
	mkdir -p $(LIBDIR)
	$(CPLUSPLUS) -shared $(CXXFLAGS) $^ -o $@

clean:
	rm -rf *.o *.a $(LIBDIR)/*.so

install:
	rm -rf *.o 


$(LIBDIR)/$(LIB): $(OBJS) 
	mkdir -p $(LIBDIR)
	$(CC) -g -shared $(LIB_FLAGS) -o $@ $^ 	
#	$(AR) $(ARFLAGS) $@ $^

%.o: %.c ../include/*.h include/*.h
	$(CC) -o $@ $(INC_FLAGS) $(CFLAGS) $< $(LIB_FLAGS)

