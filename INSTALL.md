cloc Installation Instructions
==============================

The cloc utility consists of two bash shells with file names "cloc" and "cloc_genw".   
These are found in the bin directory of this repository.
Copy these files to a bin directory somewhere in your Linux environment PATH.  
Since cloc depends on the HSAIL compiler, you can copy these two files to its bin directory which is usually in /opt/amd/bin.  
To update to a new version of cloc simply replace these two files in that directory.  

In addition to the backend compiler, cloc requires HSA software and the LLVM compiler.  
This set of instructions can be used to install HSA and cloc for Ubuntu.


# Setting up Ubuntu and HSA Linux Driver
----------------------------------------

- Make sure Ubuntu 14.04 64-bit version or above has been installed and then install these dependencies
```
sudo apt-get install g++
sudo apt-get install libstdc++-4.8-dev
sudo apt-get install llvm-3.4-dev
sudo apt-get install libelf-dev
sudo apt-get install libdwarf-dev
sudo apt-get install libtinfo-dev
sudo apt-get install re2c
sudo apt-get install libbsd-dev
sudo apt-get install gfortran
```

- Install HSA Runtime
```
cd git
git clone https://github.com/HSAfoundation/HSA-Runtime-AMD.git
cd HSA-Runtime-AMD 
sudo mkdir -p /opt/hsa/lib
sudo cp -R include /opt/hsa; 
sudo cp lib/* /opt/hsa/lib
```

- Install HSA Linux Kernel Drivers and Reboot
```
mkdir git
cd git
git clone https://github.com/HSAfoundation/HSA-Drivers-Linux-AMD.git
sudo dpkg -i HSA-Drivers-Linux-AMD/kfd-0.9/ubuntu/linux*.deb
echo "KERNEL==\"kfd\", MODE=\"0666\"" | sudo tee /etc/udev/rules.d/kfd.rules
sudo cp kfd-0.9/libhsakmt/lnx64a/libhsakmt.so.1 /opt/hsa/lib
sudo reboot
```

- Use "kfd_check_installation.sh" in HSA Linux driver to verify installation is correct.
``` 
cd git/HSA-Ddrivers-Linux-AMD
./kfd_check_installation.sh
``` 

Output of above should look like this.

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

# Setting up HSAIL Compiler
---------------------------

```
cd git
git clone https://github.com/HSAfoundation/HSAIL-HLC-Stable.git
sudo mkdir -p /opt/amd
sudo cp -R HSAIL-HLC-Stable/bin /opt/amd
```

# Build and Install libHSAIL
----------------------------

```
cd git
git clone https://github.com/HSAfoundation/HSAIL-Tools.git
cd HSAIL-Tools/libHSAIL
make -j LLVM_CONFIG=llvm-config-3.4 _OPT=1 _M64=1
sudo mkdir -p /opt/hsa/lib
sudo mkdir -p /opt/hsa/include
sudo cp build*/libhsail.a /opt/hsa/lib
sudo cp libHSAIL/*.h* /opt/hsa/include
sudo cp libHSAIL/generated/*.h* /opt/hsa/include

```

# Copy cloc and cloc_genw to /opt/amd/bin
-----------------------------------------

```
cd git
git clone https://github.com/HSAfoundation/CLOC.git
sudo cp CLOC/bin/cloc /opt/amd/bin/.
sudo cp CLOC/bin/cloc_genw /opt/amd/bin/.
```

# Set HSA environment variables
-------------------------------
```
export HSA_LLVM_PATH=/opt/amd/bin
export HSA_RUNTIME_PATH=/opt/hsa
export HSA_LIBHSAIL_PATH=/opt/hsa/lib
export PATH=$PATH:/opt/amd/bin
export LD_LIBRARY_PATH=/opt/hsa/lib
```
