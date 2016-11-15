ATMI Install Instructions
==============================

- [1. Prepare System for Package Installation](#Prepare)
- [2. Install ROCm Software Stack](#ROCM)
- [3. Install CLOC (CL Offline Compiler)](#CLOC)
- [4. Install/Build ATMI](#ATMI)

<A Name="Prepare">
1. Prepare System for Package Installation
==========================================

ATMI has been tested for the Ubuntu 14.04 LTS ("trusty") platform. But, it should generally work for platforms that are supported by ROCm. 
In addition to Linux, you must have an HSA compatible system such as a Kaveri processor, a Carrizo processor, or a Fiji card. 
After you install Ubuntu, add two additional repositories with these root-authorized commands:
```
sudo su - 
add-apt-repository ppa:ubuntu-toolchain-r/test
wget -qO - http://packages.amd.com/rocm/apt/debian/rocm.gpg.key | apt-key add -
echo 'deb [arch=amd64] http://packages.amd.com/rocm/apt/debian/ trusty main' > /etc/apt/sources.list.d/rocm.list
apt-get update
apt-get upgrade
```

<A Name="ROCM">
2. Install ROCm Software Stack
==============================

Install rocm software packages, kernel drivers, HSA/ROCm Runtime and reboot your system with these commands:

```
sudo apt-get install rocm
sudo reboot
```

<A Name="CLOC">
3. Install CLOC (CL Offline Compiler) 
=====================================

The ATMI runtime currently supports loading BRIG (.brig or .hsail) or HSA code objects (.hsaco). The LLVM/Clang toolchaing supports compiling CL language to either BRIG 
or directly to HSA code objects (AMD GCN ISA). We recommend installing the CLOC utility, which is a nifty, thin wrapper script around clang, to perform 
offline compilation of CL kernels. However, you may choose to install your favorite Linux toolchain to compile CL kernels to be used with ATMI. 
Please refer to https://github.com/HSAFoundation/CLOC for further details about the usage of CLOC. 

```
sudo apt-get install amdcloc
```

<A Name="ATMI">
4. Install/Build ATMI
========================
## Install the libatmi-runtime package (ATMI-RT) from the ROCm apt server. 

```
sudo apt-get install libatmi-runtime
```

## Install the libatmi-runtime package (ATMI-RT) from source package
Source package may sometimes be more updated than the ROCm apt server.

```
mkdir -p ~/git
cd ~/git
git clone https://github.com/RadeonOpenCompute/atmi.git
sudo dpkg -i ~git/atmi/packages/libatmi-runtime_*_amd64.deb
```

## Build ATMI-RT from source

```
mkdir -p ~/git
cd ~/git
git clone https://github.com/RadeonOpenCompute/atmi.git
cd ~/git/atmi/src/runtime/
# edit Makefile to set path of ROCM_RUNTIME_PATH if ROCm has not been installed to the default location
make
# libatmi_runtime.so should have been created as ~/git/atmi/lib/libatmi_runtime.so
# copy it to any other location that is already under LD_LIBRARY_FLAGS or do the below
export LD_LIBRARY_FLAGS=~/git/atmi/lib:$LD_LIBRARY_FLAGS # (optional)
```

## (Optional) Build ATMI-C language extension from source. 
This feature is not available as part of the ATMI debian packages, so you have to install it from the source. 
Please see the examples and their Makefile under /path/to/atmi-source/examples/c_extension for details about 
the usage of the ATMI-C language extenstion feature. 

ATMI-C language extension relies on GCC plugins to create custom function attributes. 
```
sudo apt-get install gcc-4.8-plugin-dev # (For Ubuntu "trusty"; version may vary for different Ubuntu flavors.)
```
Check if GCC plugins are installed correctly.
```
$ g++ -print-file-name=plugin
/usr/lib/gcc/x86_64-linux-gnu/4.8/plugin
```

Build ATMI-C language extensions.
```
mkdir -p ~/git
cd ~/git
git clone https://github.com/RadeonOpenCompute/atmi.git
cd ~/git/atmi/src/cmopiler/
# edit Makefile to set path of ROCM_RUNTIME_PATH if ROCm has not been installed to the default location
make
# atmi_pifgen.so should have been created as ~/git/atmi/lib/atmi_pifgen.so
# copy it to any other location that is already under LD_LIBRARY_FLAGS or do the below
export LD_LIBRARY_FLAGS=~/git/atmi/lib:$LD_LIBRARY_FLAGS # (optional)
```

