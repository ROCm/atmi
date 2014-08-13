To build, set the following environment variables

HSA_RUNTIME_PATH to the root folder of HSA-Runtime from github.com/HSAFoundation
HSA_OKRA_PATH to the root folder of the okra distribution from github.com/HSAFoundation
HSA_CLOC_PATH to the root folder of CLOC form github.com/HSAFoundation

HSA_LIBHSAIL_PATH to the build folder of HSAIL-Tools from github.com/HSAFoundation - HSAIL-Tools/libHSAIL/libHSAIL/build
HSA_LLVM_PATH to the build folder of the HSAIL_HLC_Development from github.com/HSAFoundation (Old name-HSAIL_LLVM_BACKEND)

To run the test case, set the following environment variables
HSA_THUNK_PATH Path to libhsakmt.so


#For building HSA examples. 
cd hsa 
make all
make test

#For building OKRA examples
cd okra
make all
make test

You can also build and run individually eah test case
