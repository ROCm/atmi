HSA_RUNTIME_PATH=/opt/hsa
ATMILIB_RUNTIME_PATH=${HOME}/git/ATMI
ATMILIB_INC=${ATMILIB_RUNTIME_PATH}/include
CPLUSPLUS=g++-4.9
CC=gcc-4.9
CC_FLAGS=-O3 -lelf -L${ATMILIB_RUNTIME_PATH}/lib -lsnk -L${HSA_RUNTIME_PATH}/lib -lhsa-runtime64 -I${ATMILIB_INC} -I${HSA_RUNTIME_PATH}/include

LIB=atmi_pifgen.so
LIBDIR=../../lib

PLUGIN_SOURCE_FILES= pifgen_gcc_plugin.c
GCCPLUGINS_DIR:= $(shell $(CC) -print-file-name=plugin)
	CXXFLAGS+= -I$(GCCPLUGINS_DIR)/include -fPIC -O2 -I${ATMILIB_INC}

$(LIB): $(PLUGIN_SOURCE_FILES)
	$(CPLUSPLUS) -shared $(CXXFLAGS) $^ -o $@

install: $(LIB)
	cp $^ $(LIBDIR)/
