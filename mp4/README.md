# Linux Security Module  

## To compile kernel (might fail on Ubuntu 14.04 because old version of gcc)  
```bash
sudo apt-get install git build-essential kernel-package fakeroot libncurses5-dev libssl-dev ccache libelf-dev
sudo apt install -y libncurses5-dev attr
#clone linux kernel
git clone git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
cd linux-stable
#This code is for v4.4
git checkout v4.4
#copy existing conf
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
#add `source security/mp4/Kconfig` to line 125
vim security/Kconfig
#add `subdir-$(CONFIG_SECURITY_MP4_LSM) += mp4` to line 11
#add `obj-$(CONFIG_SECURITY_MP4_LSM) += mp4/` to line 27
vim security/Makefile
make clean
#Or you can specify 1.5 times of your cores instead of `getconf _NPROCESSORS_ONLN`
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
#change `GRUB_CMDLINE_LINUX_DEFAULT="quiet nosplash"` to `GRUB_CMDLINE_LINUX_DEFAULT="quiet nosplash security=mp4"`
vim /etc/default/grub
```
