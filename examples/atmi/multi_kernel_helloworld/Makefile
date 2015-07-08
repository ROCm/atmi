HSA_RUNTIME_PATH=/opt/hsa
ATMILIB_RUNTIME_PATH=/opt/amd/atmi
ATMILIB_INC=${ATMILIB_RUNTIME_PATH}/include
CC=gcc
CC_FLAGS=-O3 -lelf -L${ATMILIB_RUNTIME_PATH}/lib -latmi_runtime -L${HSA_RUNTIME_PATH}/lib -lhsa-runtime64 -I${ATMILIB_INC} -I${HSA_RUNTIME_PATH}/include

LIB=atmi_pifgen.so

default: hello.c
	$(CC) -o hello.o -c $^ -fplugin=$(LIB) $(CC_FLAGS) 
	$(CC) -o hello hello.o $^.pifdefs.c $(CC_FLAGS)

clean:
	rm *.o hello hello.c.pifdefs.c
