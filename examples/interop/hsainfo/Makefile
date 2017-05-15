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
ATMI_RUNTIME_PATH ?= /opt/rocm/atmi
ATMI_INC=${ATMI_RUNTIME_PATH}/include

#Set HSA Environment variables
HSA_RUNTIME_PATH ?= /opt/rocm/

# Host compiler
CXX = g++
CXXFLAGS =-O3 -g -std=c++11
#CXXFLAGS += -v

INC_FLAGS=-I${ATMI_INC} -I${HSA_RUNTIME_PATH}/hsa/include -I.

LIBS=-latmi_interop_hsa -latmi_runtime -lhsa-runtime64
LIB_FLAGS=-L${ATMI_RUNTIME_PATH}/lib -L${HSA_RUNTIME_PATH}/lib

OBJS = hsainfo

.PHONY: clean all

all: $(OBJS)

hsainfo: hsainfo.cpp 
	$(CXX) -o $@ hsainfo.cpp $(CXXFLAGS) $(LIBS) $(LIB_FLAGS) $(INC_FLAGS)

clean:
	rm -rf *.o $(OBJS)

test:
	./hsainfo
