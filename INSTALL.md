Cloc Install Instructions 1.0P
==============================

The Cloc utility consists of three bash scripts with file names "cloc.sh" ,  "snack.sh" , and "snk_genw.sh" . These are found in the bin directory of this repository. Copy these files to a bin directory in your Linux environment PATH such as /usr/local/bin.  To update to a new version of Cloc simply replace cloc.sh snack.sh,  and snk_genw.sh in that directory. 

In addition to the bash scripts, Cloc requires the HSA runtime and the HLC compiler. This set of instructions can be used to install a comprehensive HSA software stack and the Cloc utility for Ubuntu.  In addition to Linux, you must have an HSA compatible system such as a Kaveri processor. There are four major steps to this process:

- [1. Prepare for Upgrade](#Prepare)
- [2. Install Linux Kernel](#Boot)
- [3. Install HSA Software](#Install)
- [4. Install Optional Infiniband Software](#Infiniband)

<A Name="Prepare">
1. Prepare System
=================

## Install Ubuntu 14.04 LTS

Make sure Ubuntu 14.04 LTS 64-bit version has been installed.  We recommend the server package set.  The utica version of ubuntu (14.10) has not been tested with HSA.  Then install these dependencies:
```
sudo apt-get update
sudo apt-get upgrade
sudo apt-get install git
sudo apt-get install make
sudo apt-get install g++
sudo apt-get install libstdc++-4.8-dev
sudo apt-get install libelf-dev
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

Make sure you get the backlevel <b>kfd-v1.0.x</b> branch. This set of instructions is for the provisional HSA runtime.  The software stack for the new <b>finalized v1.0F</b> is not yet complete. We will update these install instructions when that is complete.  This should be sometime in June 2015.  

Execute these commands:

```
cd ~/git
git clone -b kfd-v1.0.x https://github.com/HSAfoundation/HSA-Drivers-Linux-AMD.git
sudo dpkg -i HSA-Drivers-Linux-AMD/kfd-1.0/ubuntu/*.deb
echo "KERNEL==\"kfd\", MODE=\"0666\"" | sudo tee /etc/udev/rules.d/kfd.rules
sudo cp HSA-Drivers-Linux-AMD/kfd-1.0/libhsakmt/lnx64a/libhsakmt.so.1 /opt/hsa/lib
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

## Install HSA Runtime

```
mkdir ~/git
cd ~/git
git clone -b release-v1.0  https://github.com/HSAfoundation/HSA-Runtime-AMD.git
cd HSA-Runtime-AMD/ubuntu
sudo dpkg -i hsa-runtime_1.0_amd64.deb
```

## Install HSAIL Compiler (HLC)

```
cd ~/git
git clone https://github.com/HSAfoundation/HSAIL-HLC-Stable.git
cd HSAIL-HLC-Stable/ubuntu
sudo dpkg -i hsail-hlc-stable_1.0_amd64.deb
```

## Install and Test Cloc utility

As of Cloc version 0.8 the executable shell script names are changed to cloc.sh, snack.sh and snk_genw.sh. 
These scripts need to be copied to a directory that is in users PATH.  For example /usr/local/bin is typically in PATH.
```
cd ~/git
git clone -b 0.8 https://github.com/HSAfoundation/CLOC.git
sudo cp CLOC/bin/cloc.sh /usr/local/bin/.
sudo cp CLOC/bin/snack.sh /usr/local/bin/.
sudo cp CLOC/bin/snk_genw.sh /usr/local/bin/.
cd 
cp -r git/CLOC/examples .
cd examples/snack/helloworld
./buildrun.sh
```

## Install Kalmar compiler

This was formerly known as C++AMP. This step is optional because it is not needed for Cloc.  However this is becoming a very good HSA compiler. 

```
mkdir ~/git/deb
cd ~/git/deb
wget https://bitbucket.org/multicoreware/cppamp-driver-ng/downloads/clamp-0.5.0-hsa-milestone4-Linux.deb
wget https://bitbucket.org/multicoreware/cppamp-driver-ng/downloads/libcxxamp-0.5.0-hsa-milestone4-Linux.deb
wget https://bitbucket.org/multicoreware/cppamp-driver-ng/downloads/clamp-bolt-1.2.0-hsa-milestone4-Linux.deb
wget https://bitbucket.org/multicoreware/cppamp-driver-ng/downloads/boost_1_55_0-hsa-milestone3.deb
sudo dpkg -i *.deb
```

## Install Okra 

This step is also optional.  It is not needed for Cloc.  However, it is currently needed for the experimental version of gcc that supports OpenMP accelertion in HSA. 
```
cd ~/git
git clone https://github.com/HSAfoundation/Okra-Interface-to-HSA-Device
sudo mkdir /opt/amd/okra
sudo cp -r Okra-Interface-to-HSA-Device/okra /opt/amd
sudo cp Okra-Interface-to-HSA-Device/okra/dist/bin/libokra_x86_64.so /opt/hsa/lib/.
```

## Set HSA environment variables

```
export HSA_LLVM_PATH=/opt/amd/bin
export HSA_RUNTIME_PATH=/opt/hsa
export HSA_OKRA_PATH=/opt/amd/okra
export PATH=$PATH:/opt/amd/bin
export LD_LIBRARY_PATH=/opt/hsa/lib
```


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

