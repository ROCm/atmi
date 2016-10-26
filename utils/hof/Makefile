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

HSA_RUNTIME_PATH ?= /opt/hsa
#HSA_TEST_RUNTIME_PATH ?= /opt/hsa.1_1T
CC=g++
CC_FLAGS=-O3 -g -lelf -Wl,--no-as-needed -L${HSA_RUNTIME_PATH}/lib -lhsa-runtime64 -I${HSA_RUNTIME_PATH}/include 

all: hof
default: hof

hof: hof.cpp
	$(CC) -o $@ $(CC_FLAGS) $^

install:
	cp hof ../../bin/

clean:
	rm -rf hof

.PHONY: clean all
