# Linux Security Module  
## To compile kernel  
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
cp -r [mp4] security/
#add `source security/mp4/Kconfig` to line 125
vim security/Kconfig
#add `subdir-$(CONFIG_SECURITY_MP4_LSM) += mp4` to line 11
#add `obj-$(CONFIG_SECURITY_MP4_LSM)` += mp4/ to line 27
vim security/Makefile
make clean
#Or you can specify 1.5 times of your cores instead of `getconf _NPROCESSORS_ONLN`
make -j `getconf _NPROCESSORS_ONLN` deb-pkg LOCALVERSION=-[whatever]
```
## To install kernel  
```bash
sudo dpkg -i linux-image-4.4.0-*_4.4.0-*_amd64.deb
sudo dpkg -i linux-headers-4.4.0-*_4.4.0-*_amd64.deb
```
