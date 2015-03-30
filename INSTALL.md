cloc Install Instructions
=========================

The cloc utility consists of thre bash scripts with file names "cloc.sh" ,  "snack.sh" , and "snk_genw.sh" . These are found in the bin directory of this repository. Copy these files to a bin directory in your Linux environment PATH such as /usr/local/bin.  To update to a new version of cloc simply replace cloc.sh snack.sh,  and snk_genw.sh in that directory.

In addition to the bash scripts, cloc requires HSA software and the LLVM compiler. This set of instructions can be used to install much of the HSA software stack and cloc for Ubuntu.  In addition to Linux, you must have an HSA compatible system such as a Kaveri processor. 


HSA Software Install Instructions
=================================


- Make sure Ubuntu 14.04 64-bit version or above has been installed.  Then install these dependencies:
```
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


- Install HSA Runtime
```
mkdir ~/git
cd ~/git
git clone -b release-v1.0  https://github.com/HSAfoundation/HSA-Runtime-AMD.git
cd HSA-Runtime-AMD/ubuntu
sudo dpkg -i hsa-runtime_1.0_amd64.deb
```


- Install HSA Linux Kernel Drivers and Reboot
```
cd ~/git
git clone -b kfd-v1.0.x https://github.com/HSAfoundation/HSA-Drivers-Linux-AMD.git
sudo dpkg -i HSA-Drivers-Linux-AMD/kfd-1.0/ubuntu/*.deb
echo "KERNEL==\"kfd\", MODE=\"0666\"" | sudo tee /etc/udev/rules.d/kfd.rules
sudo cp HSA-Drivers-Linux-AMD/kfd-1.0/libhsakmt/lnx64a/libhsakmt.so.1 /opt/hsa/lib
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
cd HSAIL-HLC-Stable/ubuntu
sudo dpkg -i hsail-hlc-stable_1.0_amd64.deb
```


- Install and test cloc
```
cd ~/git
git clone https://github.com/HSAfoundation/CLOC.git
# As of 0.8 the executable names are changed to cloc.sh, snack.sh and snk_genw.sh
# We will fix the debian package later
sudo cp CLOC/bin/cloc.sh /usr/local/bin/.
sudo cp CLOC/bin/snack.sh /usr/local/bin/.
sudo cp CLOC/bin/snk_genw.sh /usr/local/bin/.
cd 
cp -r git/CLOC/examples .
cd examples/snack/helloworld
./buildrun.sh
```


- Install C++AMP (optional, not needed for cloc)
```
mkdir ~/git/deb
cd ~/git/deb
wget https://bitbucket.org/multicoreware/cppamp-driver-ng/downloads/clamp-0.5.0-hsa-milestone4-Linux.deb
wget https://bitbucket.org/multicoreware/cppamp-driver-ng/downloads/libcxxamp-0.5.0-hsa-milestone4-Linux.deb
wget https://bitbucket.org/multicoreware/cppamp-driver-ng/downloads/clamp-bolt-1.2.0-hsa-milestone4-Linux.deb
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
export HSA_OKRA_PATH=/opt/amd/okra
export PATH=$PATH:/opt/amd/bin
export LD_LIBRARY_PATH=/opt/hsa/lib
```
