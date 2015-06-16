HOST_GCC=g++
TARGET_GCC=gcc
PLUGIN_SOURCE_FILES= multi_target_gcc_plugin.c
GCCPLUGINS_DIR:= $(shell $(TARGET_GCC) -print-file-name=plugin)
	CXXFLAGS+= -I$(GCCPLUGINS_DIR)/include -fPIC -fno-rtti -O2

plugin.so: $(PLUGIN_SOURCE_FILES)
	$(HOST_GCC) -shared $(CXXFLAGS) $^ -o $@

test: hello.c
	$(TARGET_GCC) -fplugin=./plugin.so hello.c
test2: hello2.c
	$(TARGET_GCC) -fplugin=./plugin.so hello2.c
