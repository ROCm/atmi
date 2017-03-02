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
sudo apt-get install atmi
```

## Install the libatmi-runtime package (ATMI-RT) from source package
Source package may sometimes be more updated than the ROCm apt server.

```
mkdir -p ~/git
cd ~/git
git clone https://github.com/RadeonOpenCompute/atmi.git
sudo dpkg -i ~git/atmi/packages/atmi-*.deb
```

## Build ATMI-RT and ATMI-C from source

```
mkdir -p ~/git
cd ~/git
git clone https://github.com/RadeonOpenCompute/atmi.git
mkdir ~/git/atmi/src/build
cd ~/git/atmi/src/build
# ensure you have cmake (version >= 2.8)
cmake -DCMAKE_INSTALL_PREFIX=/path/to/install ..
# make all components (RT and the C Plugin)
make
make install
export LD_LIBRARY_FLAGS=/path/to/install/lib:$LD_LIBRARY_FLAGS # (optional)
```

