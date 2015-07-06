ATMI 0.1 Install Instructions
=============================

- [1. Install CLOC ](#InstallCLOC)
- [2. Install ATMI e](#Install)


<A Name="InstallCLOC">
1. Install CLOC
===============

```
cd ~/git
git clone https://github.com/HSAfoundation/cloc
# Install
mkdir -p /opt/amd/cloc
sudo cp -rp ~/git/cloc /opt/amd
sudo ln -sf /opt/amd/cloc/bin/cloc.sh /usr/local/bin/cloc.sh
sudo ln -sf /opt/amd/cloc/bin/snack.sh /usr/local/bin/snack.sh
sudo ln -sf /opt/amd/cloc/bin/printhsail /usr/local/bin/printhsail
# Test
cp -r /opt/amd/cloc/examples ~
cd ~/examples/snack/helloworld
./buildrun.sh
```

<A Name="Install">
2. Install ATMI
===============

```
cd ~/git
git clone https://github.com/HSAfoundation/atmi
# Install
mkdir -p /opt/amd/atmi
sudo cp -rp ~/git/atmi /opt/amd
```

