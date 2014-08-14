To build, set the following environment variables

HSA_RUNTIME_PATH to the root folder of HSA-Runtime from github.com/HSAFoundation
HSA_OKRA_PATH to the root folder of the okra distribution from github.com/HSAFoundation
HSA_CLOC_PATH to the root folder of CLOC form github.com/HSAFoundation

HSA_LIBHSAIL_PATH to the build folder of HSAIL-Tools from github.com/HSAFoundation - HSAIL-Tools/libHSAIL/libHSAIL/build
HSA_LLVM_PATH to the build folder of the HSAIL-HLC-Stable or HSAIL-HLC-Development from github.com/HSAFoundation
    Note- If HSAIL-HLC-Stable is used. The tests have to be compiled with CFLAGS=-DDUMMY_ARGS=1. Example make all -DDUMMY_ARGS=1

To run the test case, set the following environment variables
HSA_KMT_PATH Path to libhsakmt.so


#For building HSA examples. 
cd hsa 
make all
make test

#For building OKRA examples
cd okra
make all
make test

You can also build and run individually each test case
