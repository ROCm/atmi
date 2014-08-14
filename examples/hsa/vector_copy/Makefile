
TEST_NAME=vector_copy
LFLAGS= -g  -Wl,--unresolved-symbols=ignore-in-shared-libs
INCS = -I $(HSA_RUNTIME_PATH)/include
C_FILES := $(wildcard *.c)
OBJ_FILES := $(addprefix obj/, $(notdir $(C_FILES:.c=.o)))

COMMON_C_FILES := $(wildcard ../common/*.c)
COMMON_OBJ_FILES := $(addprefix ../common/obj/, $(notdir $(COMMON_C_FILES:.c=.o)))

all: $(TEST_NAME) $(TEST_NAME).brig

$(TEST_NAME): $(OBJ_FILES) $(COMMON_OBJ_FILES) check-build-env
	$(CC) $(LFLAGS) $(COMMON_OBJ_FILES) $(OBJ_FILES) -lelf -L$(HSA_RUNTIME_PATH)/lib/x86_64 -lhsa-runtime64 -o $(TEST_NAME)

$(TEST_NAME).brig : check-cloc-env
	$(HSA_CLOC_PATH)/cloc $(TEST_NAME).cl

obj/%.o: %.c
	$(CC) -c $(CFLAGS) $(INCS) -o $@ $< -std=c99

clean:
	rm -rf obj/*o *.brig $(TEST_NAME)


test:	check-test-env
	echo "export LD_LIBRARY_PATH=$(HSA_THUNK_PATH):$(HSA_RUNTIME_PATH)/lib/x86_64" > test.sh
	echo "./$(TEST_NAME)" >> test.sh
	bash test.sh
	rm test.sh

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

check-cloc-env:
ifndef HSA_CLOC_PATH
	$(error HSA_CLOC_PATH is undefined)
endif
ifndef HSA_LIBHSAIL_PATH
	$(error HSA_LIBHSAIL_PATH is undefined - Required by CLOC)
endif
ifndef HSA_LIBHSAIL_PATH
	$(error HSA_CLOC_PATH is undefined - Required by CLOC)
endif



