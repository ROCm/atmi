#
#MIT License 
#
#Copyright © 2016 Advanced Micro Devices, Inc.  
#
#Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software
#without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
#persons to whom the Software is furnished to do so, subject to the following conditions:
#
#The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
#
#THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
#PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
#OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#

#Set HSA Environment variables
#HSA_RUNTIME_PATH=/opt/hsa
#HSA_RUNTIME_PATH=${HOME}/opt/hsa
HSA_RUNTIME_PATH=/opt/rocm/hsa
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

