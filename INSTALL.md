Cloc 0.9 Install Instructions
=============================

Warning.  These instructions are for HSA 1.0F .

The Cloc utility consists of three bash scripts with file names "cloc.sh" ,  "snack.sh" , and "snk_genw.sh" . These are found in the bin directory of this repository. Copy these files to /opt/amd/cloc/bin.  To update to a new version of Cloc simply replace cloc.sh snack.sh,  and snk_genw.sh in directory /opt/amd/cloc/bin.

In addition to the bash scripts, Cloc requires the HSA runtime and the HLC compiler. This set of instructions can be used to install a comprehensive HSA software stack and the Cloc utility for Ubuntu.  In addition to Linux, you must have an HSA compatible system such as a Kaveri processor. There are four major steps to this process:

- [1. Prepare for Upgrade](#Prepare)
- [2. Install Linux Kernel](#Boot)
- [3. Install HSA Software](#Install)
- [4. Install Optional Infiniband Software](#Infiniband)

<A Name="Prepare">
1. Prepare System
=================

## Install Ubuntu 14.04 LTS

Make sure Ubuntu 14.04 LTS 64-bit version has been installed.  Ubunutu 14.04 is also known as trusty.  We recommend the server package set.  The utica version of ubuntu (14.10) has not been tested with HSA.  Then install these dependencies:
```
sudo apt-get update
sudo apt-get upgrade
sudo apt-get install git
sudo apt-get install make
sudo apt-get install g++
sudo apt-get install libstdc++-4.8-dev
sudo apt-get install libelf
sudo apt-get install libtinfo-dev
sudo apt-get install re2c
sudo apt-get install libbsd-dev
sudo apt-get install gfortran
sudo apt-get install build-essential 
```

## Uninstall Infiniband

If you have Infiniband installed, uninstall the MLNX_OFED packages. 
```
mount the appropriate MLNX_OFED iso
/<mount point>/uninstall.sh
```


<A Name="Boot">
2. Install Linux Kernel Drivers 
===============================

## Install HSA Linux Kernel Drivers 

These instructions are for HSA1.0F.

Execute these commands:

```
cd ~/git
git clone https://github.com/HSAfoundation/HSA-Drivers-Linux-AMD.git
sudo dpkg -i HSA-Drivers-Linux-AMD/kfd-1.2/ubuntu/*.deb
echo "KERNEL==\"kfd\", MODE=\"0666\"" | sudo tee /etc/udev/rules.d/kfd.rules
sudo cp HSA-Drivers-Linux-AMD/kfd-1.2/libhsakmt/lnx64a/libhsakmt.so.1 /opt/hsa/lib
```

## Reboot System

```
sudo reboot
```

## Test HSA is Active.

Use "kfd_check_installation.sh" in HSA Linux driver to verify installation.

``` 
cd ~/git/HSA-Drivers-Linux-AMD
./kfd_check_installation.sh
``` 

The output of above command should look like this.

```
Kaveri detected:............................Yes
Kaveri type supported:......................Yes
Radeon module is loaded:....................Yes
KFD module is loaded:.......................Yes
AMD IOMMU V2 module is loaded:..............Yes
KFD device exists:..........................Yes
KFD device has correct permissions:.........Yes
Valid GPU ID is detected:...................Yes
Can run HSA.................................YES
```

If it does not detect a valid GPU ID (last two entries are NO), it is possible that you need to turn the IOMMU on in the firmware.  Reboot your system and interrupt the boot process to get the firmware screen. Then find the menu to turn on IOMMU and switch from disabled to enabled.  Then select "save and exit" to boot your system.  Then rerun the test script.


<A Name="Install">
3. Install HSA Software
=======================

## Install HSA 1.0F Runtime

```
mkdir ~/git
cd ~/git
git clone https://github.com/HSAfoundation/HSA-Runtime-AMD.git
cd HSA-Runtime-AMD/ubuntu
sudo dpkg -i hsa-runtime_1.0_amd64.deb
```

## Install and Test Cloc utility

As of Cloc version 0.9 the cl frontend clc2 and supporting LLVM 3.6 executables are stored in the same directory as the cloc.sh, snack.sh and snk_genw.sh shell scripts.  These scripts need to be copied should be copied into /opt/amd/cloc/bin
```
cd ~/git
git clone -b CLOC-0.9 https://github.com/HSAfoundation/CLOC.git
# Install
mkdir -p /opt/amd/cloc
sudo cp -rp ~/git/CLOC/bin /opt/amd/cloc
sudo cp -rp ~/git/CLOC/examples /opt/amd/cloc
sudo ln -sf /opt/amd/cloc/bin/cloc.sh /usr/local/bin/cloc.sh
sudo ln -sf /opt/amd/cloc/bin/snack.sh /usr/local/bin/snack.sh
sudo ln -sf /opt/amd/cloc/bin/printhsail /usr/local/bin/printhsail
# Test
cp -r /opt/amd/cloc/examples ~
cd ~/examples/snack/helloworld
./buildrun.sh
cd ~/examples/hsa/vector_copy
make
make test
```

## Set HSA environment variables

As of Cloc version 0.9, HSA_LLVM_PATH is no longer required because cloc.sh and snack.sh expect the binaries to be in the same directory where cloc.sh and snack.sh are stored.  For testing other compilers or versions of the HSA LLVM binaries, you may set HSA_LLVM_PATH or use the -p option as noted in the help. The snack.sh script assumes HSA_RUNTIME_PATH is /opt/hsa.  However, we recommend using LD_LIBRARY_PATH to find the current version of he HSA runtime as follows:
```
export HSA_RUNTIME_PATH=/opt/hsa
export LD_LIBRARY_PATH=$HSA_RUNTIME_PATH/lib
```

We recommend that cloc.sh, snack,sh, and printhsail be available in your path.  You can symbolically link them or add to PATH as follows:
```
#
# Either put /opt/amd/cloc/bin in your PATH as follows
export PATH=$PATH:/opt/amd/cloc/bin
#
# OR symbolic link cloc.sh and snack.sh to system path
sudo ln -sf /opt/amd/cloc/bin/cloc.sh /usr/local/bin/cloc.sh
sudo ln -sf /opt/amd/cloc/bin/snack.sh /usr/local/bin/snack.sh
sudo ln -sf /opt/amd/cloc/bin/printhsail /usr/local/bin/printhsail
```

Future package installers (.deb and .rpm) will symbolically link them.

## Install Kalmar (C++AMP) HSA Compiler (OPTIONAL)

SKIP THIS STEP TILL KALMAR IS PORTED TO 1.0F

## Install gcc OpenMP for HSA Compiler (OPTIONAL)

SKIP THIS STEP TILL IT IS PORTED TO 1.0F

## Install Codeplay HSA Compiler (OPTIONAL)

SKIP THIS STEP TILL IT IS PORTED TO 1.0F

## Install Pathscale HSA Compiler (OPTIONAL)

SKIP THIS STEP TILL IT IS PORTED TO 1.0F


<A Name="Infiniband">
4. Optional Infiniband Install 
==============================

## Download OFED

Go to the Mellanox site and download the following MLNX_OFED iso package:
```
http://www.mellanox.com/page/products_dyn?product_family=26
2.4-1.0.4/Ubuntu/Ubuntu 14.10/x86_86/MLNX_OFED*.iso     
```
<b>Ubuntu 14.10</b> is the one that needs to be downloaded to be able to build and install with the 3.17 and 3.19 kernels

## Install gcc-4.9 

The gcc-4.9 compiler is needed for some of the Mellanox tools which need libstdc++6 built with g++-4.9 or greater

```
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt-get update
sudo apt-get install gcc-4.9
sudo apt-get install g++-4.9
sudo apt-get install gfortran-4.9
```

## Install MLNX_OFED 

```
Mount the MLNX_OFED iso for Ubuntu 14.10
sudo /<mount point>/mlnxofedinstall --skip-distro-check
```

## Setup Infiniband IP Networking

Edit /etc/network/interfaces to setup IPoIB (ib0)

## Run Subnet Manager

On a couple of the systems, opensm should be run
```
sudo /etc/init.d/opensmd start
```

## Load Kernel Components

Load the IB/RDMA related kernel components
```
sudo /etc/init.d/openibd restart
```

## Verify Install

Run this command
```
lsmod | egrep .ib_|mlx|rdma.  
```
It should show 14 or more IB-related modules loaded.

