#Set HSA Environment variables
#HSA_RUNTIME_PATH=/opt/hsa
#HSA_RUNTIME_PATH=${HOME}/opt/hsa
HSA_RUNTIME_PATH=/opt/hsa-nov15
HSA_LLVM_PATH=/opt/amd/cloc/bin

AR=ar
ARFLAGS=-cvq
CC=g++
CFLAGS=-O2 -c -fpic -g -std=c++11
ATMILIB_RUNTIME_PATH=../..
INC_FLAGS=-I${ATMILIB_RUNTIME_PATH}/include -I${HSA_RUNTIME_PATH}/include -I./include 
LIB_FLAGS=-L${HSA_RUNTIME_PATH}/lib -lhsa-runtime64 -Wl,--no-as-needed -Wl,-rpath=${HSA_RUNTIME_PATH}/lib

LIBDIR=${ATMILIB_RUNTIME_PATH}/lib

.PHONY: clean all

ATMILIB_INC=${ATMILIB_RUNTIME_PATH}/include
CPLUSPLUS=g++
CC_FLAGS=-O3 -lelf -L${ATMILIB_RUNTIME_PATH}/lib -lsnk -L${HSA_RUNTIME_PATH}/lib -lhsa-runtime64 -I${ATMILIB_INC} -I${HSA_RUNTIME_PATH}/include

GENLIB=atmi_pifgen.so

PLUGIN_SOURCE_FILES= atl_pifgen_plugin.c
GCCPLUGINS_DIR:= $(shell $(CC) -print-file-name=plugin)
	CXXFLAGS+= -I$(GCCPLUGINS_DIR)/include -fPIC -O2 -I${ATMILIB_INC} ${INC_FLAGS}

all: $(LIBDIR)/$(GENLIB) 

$(LIBDIR)/$(GENLIB): $(PLUGIN_SOURCE_FILES)
	mkdir -p $(LIBDIR)
	$(CPLUSPLUS) -shared $(CXXFLAGS) $^ -o $@

clean:
	rm -rf *.o *.a $(LIBDIR)/${GENLIB}

install:
	rm -rf *.o 

