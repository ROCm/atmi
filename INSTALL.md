ATMI Install Instructions
=========================

- [1. Prepare System for ATMI Installation](#Prepare)
- [2. Install/Build ATMI](#ATMI)
- [3. Build/Test ATMI Examples](#Examples)

<A Name="Prepare">

## Prepare System for ATMI Installation

ATMI works on all platforms that are supported by ROCm, but has been tested mainly for the Ubuntu 16.04.4 LTS ("xenial") platform.
See [here](https://github.com/RadeonOpenCompute/ROCm) for details on all supported hardware/OS configurations and instructions on how to install ROCm for your system.

<A Name="ATMI">

## Install/Build ATMI 
There are multiple ways to install ATMI (ATMI-RT, ATMI-DEVRT and ATMI-C): from the ROCm apt server, or from the package files on GitHub or build from the source.

#### Install from the ROCm apt server

```
sudo apt-get install atmi
```

#### Install from the package files in the source tree
Source package may sometimes be more up-to-date than the ROCm apt server.

```
mkdir -p ~/git
cd ~/git
git clone https://github.com/RadeonOpenCompute/atmi.git
sudo dpkg -i ~git/atmi/packages/atmi-*.deb
```

#### Build from the source

```
mkdir -p ~/git
cd ~/git
git clone https://github.com/RadeonOpenCompute/atmi.git
mkdir ~/git/atmi/src/build
cd ~/git/atmi/src/build
# export all GFX target architectures for the ATMI device runtime
export GFXLIST="gfx801 gfx803 gfx900" # e.g.: gfx900 is for AMD Vega GPUs
# ensure you have cmake (version >= 2.8)
cmake \
    -DCMAKE_INSTALL_PREFIX=/path/to/install \
    -DCMAKE_BUILD_TYPE={Debug|Release} \
    -DLLVM_DIR=/path/to/llvm \ # compiler to build ATMI device runtime and user GPU kernels
    -DDEVICE_LIB_DIR=<path>  \ # root of rocm device library to link
    -DHSA_DIR=/path/to/hsa   \ # root of ROCm/HSA runtime (default: /opt/rocm)
    -DATMI_HSA_INTEROP=ON    \ # optional to build ATMI with HSA interop functionality
    -DATMI_DEVICE_RUNTIME=ON \ # optional to build ATMI device runtime
    -DATMI_C_EXTENSION=ON    \ # optional to build ATMI with C compiler extension
    ..
# make all components (RT, Device RT and the C Plugin)
make
make install
export LD_LIBRARY_FLAGS=/path/to/install/lib:$LD_LIBRARY_FLAGS # (optional)
```

<A Name="Examples">

## Build/Test ATMI Examples

ATMI is a dual source programming model (similar to OpenCL), where the host code and device code are compiled separately,
and the ATMI host runtime loads the device module before launching tasks.
The ATMI host runtime currently supports loading both AMD GCN (HSA code object) or BRIG/HSAIL, but support for BRIG/HSAIL will go away in future releases.
The device language that is currently supported is CL kernel language and can be compiled to AMD GCN by using the LLVM/Clang toolchain.
ATMI ships with it the CLOC (CL Offline Compiler) utility script, which is a thin wrapper around Clang to help compile CL kernels.

```
# Building a simple helloworld example on a two Vega GPU system
cd /path/to/atmi/examples/runtime/helloworld_dGPU
make
# or run cloc.sh with the following options
# /opt/rocm/atmi/bin/cloc.sh -hcc2 /opt/rocm/llvm -triple amdgcn-amd-amdhsa -libgcn /opt/rocm -clopts "-I/opt/rocm/atmi/include -I/opt/rocm/hsa/include -I. -O2 -v"  -opt 2 hw.cl
make test
env LD_LIBRARY_PATH=/home/rocm-user/git/build/atmi/lib:/opt/rocm/hsa/lib: ./hello
Choosing GPU 0/2
Output from the GPU: Hello HSA World
Output from the CPU: Hello HSA World
```

