To build, set the following environment variables

* git clone https://github.com/HSAFoundation/HSA-Runtime-AMD
* git clone https://github.com/HSAFoundation/HSA-Drivers-Linux-AMD
* git clone https://github.com/HSAFoundation/Okra-Interface-to-HSA-Device
* https://github.com/HSAFoundation/CLOC

Set the environment variables
* HSA_RUNTIME_PATH= Path to HSA-Runtime-AMD
* HSA_KMT_PATH= Path to HSA-Drivers-Linux-AMD/kfd-0.8/libhsakmt/
* HSA_OKRA_PATH= Path to Okra-Interface-to-HSA-Device/okra/

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
