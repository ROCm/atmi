ATMI Install Instructions
=========================

- [1. Prepare System for ATMI Installation](#Prepare)
- [2. Install/Build ATMI](#ATMI)
- [3. Build/Test ATMI Examples](#Examples)

<A Name="Prepare">

## Prepare System for ATMI Installation

ATMI works on all platforms that are supported by ROCm, but has been tested mainly for the 18.04.2 LTS (Bionic Beaver) platform.
See [here](https://github.com/RadeonOpenCompute/ROCm) for details on all supported hardware/OS configurations and instructions on how to install ROCm for your system.

<A Name="ATMI">

## Install/Build ATMI
ATMI can be installed either from the ROCm apt server or built from the source.

#### Install from the ROCm apt server

```
sudo apt-get install atmi
```

#### Build from the source

```
mkdir -p ~/git
cd ~/git
git clone https://github.com/RadeonOpenCompute/atmi.git
mkdir ~/git/atmi/src/build
cd ~/git/atmi/src/build
# export all GFX target architectures for the ATMI device runtime
export GFXLIST="gfx900 gfx906" # e.g.: gfx900 is for AMD Vega GPUs
# ensure you have cmake (version >= 2.8)
cmake \
    -DCMAKE_INSTALL_PREFIX=/path/to/install \
    -DCMAKE_BUILD_TYPE={Debug|Release} \
    -DLLVM_DIR=/path/to/llvm \ # compiler to build ATMI device runtime and user GPU kernels
    -DDEVICE_LIB_DIR=<path>  \ # root of ROCm Device Library to link
    -DATMI_DEVICE_RUNTIME=ON \ # (optional) to build ATMI device runtime (default: OFF)
    -DATMI_HSA_INTEROP=ON    \ # (optional) to build ATMI with HSA interop functionality (default: OFF)
    -DHSA_DIR=/path/to/hsa   \ # (optional) root of ROCm/HSA runtime (default: /opt/rocm)
    ..
# make all components (Host runtime and device runtime)
make
make install
export LD_LIBRARY_FLAGS=/path/to/install/lib:$LD_LIBRARY_FLAGS # (optional)
```

<A Name="Examples">

## Build/Test ATMI Examples

ATMI runtime works with any high level compiler that generates AMD GCN code objects.
The examples here use OpenCL kernel language and ATMI as the host runtime, but ATMI can also work any high level
kernel language like HIP or OpenMP as long as they are compiled to AMD GCN code objects.
In this example set, the host code and device code are compiled separately,
and the ATMI host runtime explicitly loads the device module before launching tasks.
ATMI currently supports loading AMD GCN (HSA code objects).
ATMI ships with it the CLOC (CL Offline Compiler) utility script, which is a thin wrapper around Clang to help compile CL kernels.

```
# Building a simple helloworld example on a two GPU system
cd /path/to/atmi/examples/runtime/helloworld_dGPU
make
# If make does not work, then check the different flags in make to point to the right installed locations of ROCm,
# or directly run cloc.sh with the following options
# /opt/rocm/atmi/bin/cloc.sh -aomp /opt/rocm/llvm -triple amdgcn-amd-amdhsa -libgcn /opt/rocm -clopts "-I/opt/rocm/atmi/include -I/opt/rocm/hsa/include -I. -O2 -v"  -opt 2 hw.cl
make test
env LD_LIBRARY_PATH=/opt/rocm/atmi/lib:/opt/rocm/hsa/lib: ./hello
Choosing GPU 0/2
Output from the GPU: Hello HSA World
Output from the CPU: Hello HSA World
```

