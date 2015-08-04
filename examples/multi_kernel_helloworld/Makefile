HSA_RUNTIME_PATH ?= /opt/hsa
ATMI_RUNTIME_PATH ?= /opt/amd/atmi
ATMI_INC=${ATMI_RUNTIME_PATH}/include
CC=gcc
CC_FLAGS=-O3 -lelf -L${ATMI_RUNTIME_PATH}/lib -latmi_runtime -L${HSA_RUNTIME_PATH}/lib -lhsa-runtime64 -I${ATMI_INC} -I${HSA_RUNTIME_PATH}/include

LIB=${ATMI_RUNTIME_PATH}/lib/atmi_pifgen.so

default: hello.c
	$(CC) -o hello.o -c $^ -fplugin=$(LIB) $(CC_FLAGS) 
	$(CC) -o hello hello.o $^.pifdefs.c $(CC_FLAGS)

clean:
	rm *.o hello hello.c.pifdefs.c
