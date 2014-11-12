cloc Install Instructions
=========================

The cloc utility consists of two bash scripts with file names "cloc" and "cloc_genw". These are found in the bin directory of this repository. Copy these files to a bin directory in your Linux environment PATH such as /usr/local/bin.  To update to a new version of cloc simply replace cloc and cloc_genw in that directory.

In addition to the bash scripts, cloc requires HSA software and the LLVM compiler. This set of instructions can be used to install much of the HSA software stack and cloc for Ubuntu.  In addition to Linux, you must have an HSA compatible system such as a Kaveri processor. 


HSA Software Install Instructions
=================================


- Make sure Ubuntu 14.04 64-bit version or above has been installed.  Then install these dependencies:
```
sudo apt-get install git
sudo apt-get install make
sudo apt-get install g++
sudo apt-get install libstdc++-4.8-dev
sudo apt-get install llvm-3.4-dev
sudo apt-get install libelf-dev
sudo apt-get install libtinfo-dev
sudo apt-get install re2c
sudo apt-get install libbsd-dev
sudo apt-get install gfortran
sudo apt-get install build-essential 
```


- Install HSA Runtime
```
mkdir ~/git
cd ~/git
git clone https://github.com/HSAfoundation/HSA-Runtime-AMD.git
cd HSA-Runtime-AMD 
sudo mkdir -p /opt/hsa/lib
sudo cp -R include /opt/hsa
sudo cp lib/* /opt/hsa/lib
```


- Install HSA Linux Kernel Drivers and Reboot
```
cd ~/git
git clone https://github.com/HSAfoundation/HSA-Drivers-Linux-AMD.git
sudo dpkg -i HSA-Drivers-Linux-AMD/kfd-0.9/ubuntu/*.deb
echo "KERNEL==\"kfd\", MODE=\"0666\"" | sudo tee /etc/udev/rules.d/kfd.rules
sudo cp HSA-Drivers-Linux-AMD/kfd-0.9/libhsakmt/lnx64a/libhsakmt.so.1 /opt/hsa/lib
sudo reboot
```


- Use "kfd_check_installation.sh" in HSA Linux driver to verify installation.
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


- Install HSAIL Compiler
```
cd ~/git
git clone https://github.com/HSAfoundation/HSAIL-HLC-Stable.git
sudo mkdir -p /opt/amd
sudo cp -R HSAIL-HLC-Stable/bin /opt/amd
```


- Build and Install libdwarf & libHSAIL
```
cd ~/git
wget http://pkgs.fedoraproject.org/repo/pkgs/libdwarf/libdwarf-20130729.tar.gz/4cc5e48693f7b93b7aa0261e63c0e21d/libdwarf-20130729.tar.gz
tar -xvzf libdwarf-20130729
cd dwarf-20130729
./configure
make
sudo cp libdwarf/libdwarf.a /usr/local/lib
sudo cp libdwarf/libdwarf.h /usr/local/include
sudo chmod 444 /usr/local/include/libdwarf.h
cd ..
git clone https://github.com/HSAfoundation/HSAIL-Tools.git
cd HSAIL-Tools/libHSAIL
make -j LLVM_CONFIG=llvm-config-3.4 _OPT=1 _M64=1
sudo mkdir -p /opt/hsa/lib
sudo mkdir -p /opt/hsa/include
sudo cp build*/libhsail.a /opt/hsa/lib
sudo cp libHSAIL/*.h* /opt/hsa/include
sudo cp libHSAIL/generated/*.h* /opt/hsa/include

```


- Install and test cloc
```
cd ~/git
git clone https://github.com/HSAfoundation/CLOC.git
sudo cp CLOC/bin/cloc /usr/local/bin/.
sudo cp CLOC/bin/cloc_genw /usr/local/bin/.
cd 
cp -r git/CLOC/examples .
cd examples/snack/helloworld
./buildrun.sh
```


- Install C++AMP (optional, not needed for cloc)
```
mkdir ~/git/deb
cd ~/git/deb
wget https://bitbucket.org/multicoreware/cppamp-driver-ng/downloads/clamp-0.4.0-hsa-milestone3-Linux.deb
wget https://bitbucket.org/multicoreware/cppamp-driver-ng/downloads/libcxxamp-0.4.0-hsa-milestone3-Linux.deb
wget https://bitbucket.org/multicoreware/cppamp-driver-ng/downloads/clamp-bolt-1.2.0-hsa-milestone3-Linux.deb
wget https://bitbucket.org/multicoreware/cppamp-driver-ng/downloads/boost_1_55_0-hsa-milestone3.deb
sudo dpkg -i *.deb
```


- Install Okra (optional, not needed for cloc)
```
cd ~/git
git clone https://github.com/HSAfoundation/Okra-Interface-to-HSA-Device
sudo mkdir /opt/amd/okra
sudo cp -r Okra-Interface-to-HSA-Device/okra /opt/amd
sudo cp Okra-Interface-to-HSA-Device/okra/dist/bin/libokra_x86_64.so /opt/hsa/lib/.
```


- Set HSA environment variables
```
export HSA_LLVM_PATH=/opt/amd/bin
export HSA_RUNTIME_PATH=/opt/hsa
export HSA_LIBHSAIL_PATH=/opt/hsa/lib
export HSA_OKRA_PATH=/opt/amd/okra
export PATH=$PATH:/opt/amd/bin
export LD_LIBRARY_PATH=/opt/hsa/lib
```
