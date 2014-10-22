TEST_NAME=BVH
LFLAGS= -g  -Wl,--unresolved-symbols=ignore-in-shared-libs
INCS += -I $(HSA_OKRA_PATH)/dist/include
CPP_FILES := $(wildcard *.cpp)
OBJ_FILES := $(addprefix obj/, $(notdir $(CPP_FILES:.cpp=.o)))
CXXFLAGS=-std=c++11
CC=g++

all: check-build-env $(TEST_NAME) $(TEST_NAME).hsail

$(TEST_NAME): $(OBJ_FILES)
	$(CC) $(LFLAGS) $(OBJ_FILES) -lelf -L$(HSA_OKRA_PATH)/dist/bin -lokra_x86_64 -o $(TEST_NAME)

$(TEST_NAME).hsail :
	$(HSA_CLOC_PATH)/cloc -hsail $(TEST_NAME).cl

obj/%.o: %.cpp
	$(CC) -c $(CXXFLAGS) $(CFLAGS) $(INCS) -o $@ $< 

clean:
	rm -rf obj/*o *.hsail $(TEST_NAME)


test:	check-test-env
	echo "export LD_LIBRARY_PATH=$(HSA_KMT_PATH)/lnx64a:$(HSA_OKRA_PATH)/dist/bin:$(HSA_RUNTIME_PATH)/lib/x86_64" > test.sh 
	echo "./$(TEST_NAME)" >> test.sh
	bash test.sh
	rm test.sh

check-test-env:
ifndef HSA_KMT_PATH
	$(error HSA_KMT_PATH is undefined)
endif
ifndef HSA_RUNTIME_PATH
	$(error HSA_RUNTIME_PATH is undefined)
endif
ifndef HSA_OKRA_PATH
	$(error HSA_OKRA_PATH is undefined)
endif
  
check-build-env:
ifndef HSA_OKRA_PATH
	$(error HSA_OKRA_PATH is undefined)
endif
ifndef HSA_CLOC_PATH
	$(error HSA_CLOC_PATH is undefined)
endif



