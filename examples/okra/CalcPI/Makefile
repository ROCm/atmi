#ifndef HSA_RUNTIME_PATH
	HSA_RUNTIME_PATH=/opt/hsa
#endif
#ifndef HSA_OKRA_PATH
	HSA_OKRA_PATH=/opt/amd/okra
#endif
CC=g++
TEST_NAME=CalcPI
LFLAGS= -g  -Wl,--unresolved-symbols=ignore-in-shared-libs
INCS += -I $(HSA_RUNTIME_PATH)/include -I $(HSA_OKRA_PATH)/dist/include
CPP_FILES := $(wildcard *.cpp)
OBJ_FILES := $(addprefix obj/, $(notdir $(CPP_FILES:.cpp=.o)))

all: $(TEST_NAME) $(TEST_NAME).hsail

$(TEST_NAME): $(OBJ_FILES)
	$(CC) $(LFLAGS) $(OBJ_FILES) -lelf -L$(HSA_RUNTIME_PATH)/lib -lokra_x86_64 -o $(TEST_NAME)

$(TEST_NAME).hsail :
	cloc.sh -hsail $(TEST_NAME).cl

obj/%.o: %.cpp
	$(CC) -c $(CFLAGS) $(INCS) -o $@ $< 

clean:
	rm -rf obj/*o *.hsail $(TEST_NAME)


test:	
	echo "export LD_LIBRARY_PATH=$(HSA_RUNTIME_PATH)/lib" > test.sh 
	echo "./$(TEST_NAME)" >> test.sh
	bash test.sh
	rm test.sh


