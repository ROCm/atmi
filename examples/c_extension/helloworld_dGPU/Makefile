#MIT License
#
#Copyright © 2016 Advanced Micro Devices, Inc.
#
#Permission is hereby granted, free of charge, to any person obtaining a copy of
#this software and associated documentation files (the "Software"), to deal in
#the Software
#without restriction, including without limitation the rights to use, copy,
#modify, merge, publish, distribute, sublicense, and/or sell copies of the
#Software, and to permit
#persons to whom the Software is furnished to do so, subject to the following
#conditions:
#
#The above copyright notice and this permission notice shall be included in all
#copies or substantial portions of the Software.
#
#THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#FITNESS FOR A PARTICULAR
#PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
#BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
#CONTRACT, TORT OR
#OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
#OR OTHER DEALINGS IN THE SOFTWARE.

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

