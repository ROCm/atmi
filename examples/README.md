To build, clone the following githubs 

* git clone https://github.com/HSAFoundation/HSA-Runtime-AMD
* git clone https://github.com/HSAFoundation/HSA-Drivers-Linux-AMD
* git clone https://github.com/HSAFoundation/Okra-Interface-to-HSA-Device
* git clone https://github.com/HSAFoundation/HSAIL-HLC-Development
* git clone https://github.com/HSAFoundation/HSAIL-Tools and build HSAIL-Tools

Set the environment variables
* HSA_RUNTIME_PATH= Path to HSA-Runtime-AMD
* HSA_KMT_PATH= Path to HSA-Drivers-Linux-AMD/kfd-0.8/libhsakmt/
* HSA_OKRA_PATH= Path to Okra-Interface-to-HSA-Device/okra/
* HSA_LLVM_PATH= Path to HSAIL-HLC-Developement/bin
* HSA_LIBHSAIL_PATH= Path to HSAIL-Tool/libHSAIL/build_linux


Note- If HSAIL-HLC-Stable is used. The tests have to be compiled with CFLAGS=-DDUMMY_ARGS=1. Example make all -DDUMMY_ARGS=1

#For building HSA examples. 
cd hsa 
make all
make test

#For building OKRA examples
cd okra
make all
make test

You can also build and run individually each test case
