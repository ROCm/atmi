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
