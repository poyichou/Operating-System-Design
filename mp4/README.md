To compile kernel  
```bash
sudo apt-get install git build-essential kernel-package fakeroot libncurses5-dev libssl-dev ccache libelf-dev
#clone linux kernel
git clone git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
cd linux-stable
#This code is for v4.4
git checkout v4.4
#copy existing conf
cp /boot/config-‘uname -r‘ .config
yes '' | make oldconfig
cp -r [mp4] security/
make clean
#Or you can specify 1.5 times of your cores instead of `getconf _NPROCESSORS_ONLN`
make -j `getconf _NPROCESSORS_ONLN` deb-pkg LOCALVERSION=-[whatever]
```
To install kernel
```bash
sudo dpkg -i linux-image-4.4.0-*_4.4.0-*_amd64.deb
sudo dpkg -i linux-headers-4.4.0-*_4.4.0-*_amd64.deb
```
