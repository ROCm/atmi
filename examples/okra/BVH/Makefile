#ifndef HSA_RUNTIME_PATH
	HSA_RUNTIME_PATH=/opt/hsa
#endif
TEST_NAME=BVH
CC=g++
LFLAGS+= -g  -Wl,--unresolved-symbols=ignore-in-shared-libs
CFLAGS+= -std=c++11
INCS += -I $(HSA_RUNTIME_PATH)/include
CPP_FILES := $(wildcard *.cpp)
OBJ_FILES := $(addprefix obj/, $(notdir $(CPP_FILES:.cpp=.o)))

all: $(TEST_NAME) $(TEST_NAME).hsail

$(TEST_NAME): $(OBJ_FILES)
	$(CC) $(LFLAGS) $(OBJ_FILES) -lelf -L$(HSA_RUNTIME_PATH)/lib -lokra_x86_64 -o $(TEST_NAME)

$(TEST_NAME).hsail :
	cloc -hsail $(TEST_NAME).cl

obj/%.o: %.cpp
	$(CC) -c $(CFLAGS) $(INCS) -o $@ $< 

clean:
	rm -rf obj/*o *.hsail $(TEST_NAME)


test:	
	echo "export LD_LIBRARY_PATH=$(HSA_RUNTIME_PATH)/lib" > test.sh 
	echo "./$(TEST_NAME)" >> test.sh
	bash test.sh
	rm test.sh


