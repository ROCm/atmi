
TEST_NAME=elf_utils
LFLAGS= -g  -Wl,--unresolved-symbols=ignore-in-shared-libs
INCS += -I $(HSA_RUNTIME_PATH)/include
C_FILES := $(wildcard *.c)
OBJ_FILES := $(addprefix obj/, $(notdir $(C_FILES:.c=.o)))

all: check-build-env $(OBJ_FILES)

obj/%.o: %.c
	$(CC) -c $(CFLAGS) $(INCS) -o $@ $< -std=c99 

clean:
	rm -rf obj/*o 
test:
	@echo 'Nothing to be done here'

check-build-env:
ifndef HSA_RUNTIME_PATH
	$(error HSA_RUNTIME_PATH is undefined)
endif



