HSA_RUNTIME_PATH=/opt/hsa
SNACK_RUNTIME_PATH=${HOME}/git/CLOC.ashwinma
SNACK_INC=${SNACK_RUNTIME_PATH}/include
HOST_GCC=g++
TARGET_GCC=gcc
TARGET_GCC_FLAGS=-O3 -lelf -L${SNACK_RUNTIME_PATH}/lib -lsnk -L${HSA_RUNTIME_PATH}/lib -lhsa-runtime64 -I${SNACK_INC} -I${HSA_RUNTIME_PATH}/include

LIB=libsnk_cpupifgen.so
LIBDIR=../../lib

PLUGIN_SOURCE_FILES= cpu_pifgen_gcc_plugin.c
GCCPLUGINS_DIR:= $(shell $(TARGET_GCC) -print-file-name=plugin)
	CXXFLAGS+= -I$(GCCPLUGINS_DIR)/include -fPIC -fno-rtti -O2

$(LIB): $(PLUGIN_SOURCE_FILES)
	$(HOST_GCC) -shared $(CXXFLAGS) $^ -o $@

test: hello.c
	$(TARGET_GCC) -o hello.o -c $^ -fplugin=./$(LIB) $(TARGET_GCC_FLAGS) 
	$(TARGET_GCC) -o hello_def.o -c $^.pif.def.c $(TARGET_GCC_FLAGS)
	$(TARGET_GCC) -o hello hello_def.o hello.o $(TARGET_GCC_FLAGS) 
test2: hello2.c
	$(TARGET_GCC) -o hello2.o -c $^ -fplugin=./$(LIB) $(TARGET_GCC_FLAGS) 
	$(TARGET_GCC) -o hello2_def.o -c $^.pif.def.c $(TARGET_GCC_FLAGS)
	$(TARGET_GCC) -o hello2 hello2_def.o hello2.o $(TARGET_GCC_FLAGS) 


install: $(LIB)
	cp $^ $(LIBDIR)
