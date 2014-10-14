cloc installation and test instructions

 You may have done some of this already.
 Execute with caution. 

1.  Get repositories from github
```
cd
mkdir git
cd git 
git clone http://github.com/HSAfoundation/CLOC
git clone http://github.com/HSAfoundation/HSA-Drivers-Linux-AMD
git clone http://github.com/HSAfoundation/HSA-Runtime-AMD
git clone http://github.com/HSAfoundation/HSAIL-HLC-Stable
git clone http://github.com/HSAfoundation/HSAIL-HLC-Development
git clone http://github.com/HSAfoundation/HSAIL-Tools
```
2.  Install kernel
```
cd
sudo dpkg -i git/HSA-Drivers-Linux-AMD/kfd-0.9/ubuntu/*.deb
sudo reboot
```

3.  Make sure you installed these dependencies.
```
sudo apt-get install llvm-3.4-dev
sudo apt-get install libelf-dev
sudo apt-get install libdwarf-dev
sudo apt-get install re2c
sudo apt-get install libbsd-dev
sudo apt-get install gfortran
```

4. Build the HSAIL disassembler. 
```
cd
cd git/HSAIL-Tools/libHSAIL
make -j LLVM_CONFIG=llvm-config-3.4 _OPT=1 _M64=1
ln -sf build_linux_opt_m64 build
```

5. Put cloc and cloc_w somewhere in your path. For example,
```
cd
cp git/CLOC/bin/cloc $HOME/bin/cloc
cp git/CLOC/bin/cloc_genw $HOME/bin/cloc_genw
```

6.  Set HSA environment variables. For example
```
export HSA_LIBHSAIL_PATH=~/git/HSAIL-Tools/libHSAIL/build
export HSA_LLVM_PATH=~/git/HSAIL-HLC-Stable/bin
export HSA_RUNTIME_PATH=~/git/HSA-Runtime-AMD
export HSA_KMT_PATH=~/git/HSA-Drivers-Linux-AMD/kfd-0.9/libhsakmt
```

7. Try the 6 samples 
```
cd
cp -rp git/CLOC/examples examples

cd examples/snack/csquares
./buildrun.sh
cd ../fortran
./buildrun.sh
cd ../helloworld
./buildrun.sh
./buildrun.sh f
./buildrun.sh cpp
cd ../matmul
./buildrun.sh
cd ../multiple_cl_files
./buildrun.sh
cd ../vector_copy
./buildrun.sh
```
