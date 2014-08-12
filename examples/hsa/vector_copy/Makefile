LFLAGS= -g  -Wl,--unresolved-symbols=ignore-in-shared-libs

OBSEDIAN_PATH=/home/shreyas/work/foundation/HSA_Member_Projects_AMD/obsidian/release.hsa_foundation
#Temporary fix till libHSAIL on GitHub is fixed
LIBHSAIL_PATH=/home/shreyas/work/foundation/HSAIL-Tools/libHSAIL
BUILD_DIR=build_linux

INCS = -I $(OBSEDIAN_PATH)/include
#Temporary fix to libHSAIL on GitHUB is fixed
INCS += -I $(LIBHSAIL_PATH)/libHSAIL
INCS += -I $(LIBHSAIL_PATH)/libHSAIL/generated
LIB_PATH=$(OBSEDIAN_PATH)/lib/x86_64


CPP_FILES := $(wildcard src/*.cpp)
OBJ_FILES := $(addprefix obj/, $(notdir $(CPP_FILES:.cpp=.o)))
all: vector_copy vector_copy.brig

vector_copy: $(OBJ_FILES)
	$(CC) $(LFLAGS) $(OBJ_FILES) -lelf -L$(LIB_PATH) -lhsa-runtime64 -o vector_copy

vector_copy.brig :
	./cloc vector_copy.cl

obj/%.o: src/%.cpp
	$(CC) -c $(CFLAGS) $(INCS) -o $@ $< -std=c++11

clean:
	rm -rf obj/*o vector_copy vector_copy.brig vector_copy.hsail

