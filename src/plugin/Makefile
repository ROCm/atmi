HSA_RUNTIME_PATH=/opt/hsa
ATMILIB_RUNTIME_PATH=${HOME}/git/atmi
ATMILIB_INC=${ATMILIB_RUNTIME_PATH}/include
CPLUSPLUS=g++
CC=gcc
CC_FLAGS=-O3 -lelf -L${ATMILIB_RUNTIME_PATH}/lib -lsnk -L${HSA_RUNTIME_PATH}/lib -lhsa-runtime64 -I${ATMILIB_INC} -I${HSA_RUNTIME_PATH}/include

LIB=libatmi_pifgen.so
LIBDIR=../../lib

PLUGIN_SOURCE_FILES= pifgen_gcc_plugin.c
GCCPLUGINS_DIR:= $(shell $(CC) -print-file-name=plugin)
	CXXFLAGS+= -I$(GCCPLUGINS_DIR)/include -fPIC -O2

$(LIB): $(PLUGIN_SOURCE_FILES)
	$(CPLUSPLUS) -shared $(CXXFLAGS) $^ -o $@

test: hello.c
	$(CC) -o hello.o -c $^ -fplugin=./$(LIB) $(CC_FLAGS) 
	$(CC) -o hello.pifdefs.o -c $^.pifdefs.c $(CC_FLAGS)
	$(CC) -o hello hello.pifdefs.o hello.o $(CC_FLAGS) 

install: $(LIB)
	cp $^ $(LIBDIR)
