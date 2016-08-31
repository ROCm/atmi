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
	$(CC) -g -shared -o $@ -Wl,--as-needed $^ $(LIB_FLAGS) 
#	$(AR) $(ARFLAGS) $@ $^

%.o: %.c* ${ATMILIB_RUNTIME_PATH}/include/*.h include/*.h
	$(CC) -o $@ $(INC_FLAGS) $(CFLAGS) $< 

