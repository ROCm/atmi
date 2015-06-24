HSA_RUNTIME_PATH=/opt/hsa
SNACK_RUNTIME_PATH=${HOME}/git/CLOC.ashwinma
SNACK_INC=${SNACK_RUNTIME_PATH}/include
CPLUSPLUS=g++
CC=gcc
CC_FLAGS=-O3 -lelf -L${SNACK_RUNTIME_PATH}/lib -lsnk -L${HSA_RUNTIME_PATH}/lib -lhsa-runtime64 -I${SNACK_INC} -I${HSA_RUNTIME_PATH}/include

LIB=libatmi_pifgen.so
LIBDIR=../../lib

PLUGIN_SOURCE_FILES= pifgen_gcc_plugin.c
GCCPLUGINS_DIR:= $(shell $(CC) -print-file-name=plugin)
	CXXFLAGS+= -I$(GCCPLUGINS_DIR)/include -fPIC -fno-rtti -O2

$(LIB): $(PLUGIN_SOURCE_FILES)
	$(CPLUSPLUS) -shared $(CXXFLAGS) $^ -o $@

test: hello.c
	$(CC) -o hello.o -c $^ -fplugin=./$(LIB) $(CC_FLAGS) 
	$(CC) -o hello.pifdefs.o -c $^.pifdefs.c $(CC_FLAGS)
	$(CC) -o hello hello.pifdefs.o hello.o $(CC_FLAGS) 
test2: hello2.c
	$(CC) -o hello2 $^ -fplugin=./$(LIB) $(CC_FLAGS) 
	#$(CC) -o hello2_def.o -c $^.pif.def.c $(CC_FLAGS)
	#$(CC) -o hello2 hello2_def.o hello2.o $(CC_FLAGS) 


install: $(LIB)
	cp $^ $(LIBDIR)
