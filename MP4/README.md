# Linux Security Module  
*A simple Linux Security Module(LSM)*  

## To compile kernel (might fail on Ubuntu 14.04 because old version of gcc)  
```bash
sudo apt-get install git build-essential kernel-package fakeroot libncurses5-dev libssl-dev ccache libelf-dev
sudo apt install -y libncurses5-dev attr
# Clone linux kernel
git clone git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
cd linux-stable
# This code is for v4.4
git checkout v4.4
# Copy existing conf
cp /boot/config-‘uname -r‘ .config
yes '' | make oldconfig
```
#### To speed up compiling  
```bash
make menuconfig
# Kernel Hacking -> Compile-time checks and compiler options -> Compile the kernel with debug info <-- disable
```
```bash
cp -r [mp4] security/
# Add `source security/mp4/Kconfig` to line 125
vim security/Kconfig
# Add `subdir-$(CONFIG_SECURITY_MP4_LSM) += mp4` to line 11
# Add `obj-$(CONFIG_SECURITY_MP4_LSM) += mp4/` to line 27
vim security/Makefile
make clean
# Or you can specify 1.5 times of your cores instead of `getconf _NPROCESSORS_ONLN`
make -j `getconf _NPROCESSORS_ONLN` bindeb-pkg LOCALVERSION=-[whatever]
CS423 machine problem 4 support (SECURITY_MP4_LSM) [N/y/?] (NEW) y
```
## To install kernel  
```bash
sudo dpkg -i linux-image-4.4.0-*_4.4.0-*_amd64.deb
sudo dpkg -i linux-headers-4.4.0-*_4.4.0-*_amd64.deb
```
## To add the security module as the default only major LSM  
```bash
# Change `GRUB_CMDLINE_LINUX_DEFAULT="quiet nosplash"` to `GRUB_CMDLINE_LINUX_DEFAULT="quiet nosplash security=mp4"`
vim /etc/default/grub
sudo update-grub
```

## Testing
Boost into the installed kernel  
Referring files in test/, write your own .perm, .perm.unload files  
```bash
# To get the path of the binary file of the command
which [command]
# To know which files a program would access  
strace -o [reportfile] [command]
```
```bash
# To set xattr to files
source *.perm
```
After setting the xattr to the binary file of program,  
run it and get access to other files with the process.  
```bash
# To reverse it
source *.perm.unload
```
