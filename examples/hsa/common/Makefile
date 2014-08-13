
TEST_NAME=elf_utils
LFLAGS= -g  -Wl,--unresolved-symbols=ignore-in-shared-libs
INCS += -I $(HSA_RUNTIME_PATH)/include
CPP_FILES := $(wildcard *.cpp)
OBJ_FILES := $(addprefix obj/, $(notdir $(CPP_FILES:.cpp=.o)))

all: check-build-env $(OBJ_FILES)

obj/%.o: %.cpp
	$(CC) -c $(CFLAGS) $(INCS) -o $@ $< -std=c++11

clean:
	rm -rf obj/*o 
test:
	@echo 'Nothing to be done here'

check-build-env:
ifndef HSA_RUNTIME_PATH
	$(error HSA_RUNTIME_PATH is undefined)
endif



