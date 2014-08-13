
TEST_NAME=vector_copy
LFLAGS= -g  -Wl,--unresolved-symbols=ignore-in-shared-libs
INCS = -I $(HSA_RUNTIME_PATH)/include
INCS += -I $(HSA_LIBHSAIL_PATH)/../libHSAIL
CPP_FILES := $(wildcard *.cpp)
OBJ_FILES := $(addprefix obj/, $(notdir $(CPP_FILES:.cpp=.o)))

COMMON_CPP_FILES := $(wildcard ../common/*.cpp)
COMMON_OBJ_FILES := $(addprefix ../common/obj/, $(notdir $(COMMON_CPP_FILES:.cpp=.o)))

all: check-build-env $(TEST_NAME) $(TEST_NAME).brig

$(TEST_NAME): $(OBJ_FILES) $(COMMON_OBJ_FILES)
	$(CC) $(LFLAGS) $(COMMON_OBJ_FILES) $(OBJ_FILES) -lelf -L$(HSA_RUNTIME_PATH)/lib/x86_64 -lhsa-runtime64 -o $(TEST_NAME)

$(TEST_NAME).brig :
	$(HSA_CLOC_PATH)/cloc $(TEST_NAME).cl

obj/%.o: %.cpp
	$(CC) -c $(CFLAGS) $(INCS) -o $@ $< -std=c++11

clean:
	rm -rf obj/*o *.brig $(TEST_NAME)


test:	check-test-env
	export LD_LIBRARY_PATH=$(HSA_THUNK_PATH):$(HSA_RUNTIME_PATH)/lib/x86_64
	./$(TEST_NAME)


check-test-env:
ifndef HSA_THUNK_PATH
	$(error HSA_THUNK_PATH is undefined)
endif
ifndef HSA_RUNTIME_PATH
	$(error HSA_RUNTIME_PATH is undefined)
endif
  
check-build-env:
ifndef HSA_RUNTIME_PATH
	$(error HSA_RUNTIME_PATH is undefined)
endif
ifndef HSA_CLOC_PATH
	$(error HSA_CLOC_PATH is undefined)
endif
ifndef HSA_LIBHSAIL_PATH
	$(error HSA_LIBHSAIL_PATH is undefined)
endif


