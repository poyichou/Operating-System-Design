CONFIG_MODULE_SIG=n
EXTRA_CFLAGS +=
APP_EXTRA_FLAGS:= -O2 -ansi -pedantic
KERNEL_SRC:= /lib/modules/$(shell uname -r)/build
SUBDIR= $(PWD)
GCC:=gcc
RM:=rm

.PHONY : clean

all: clean modules

obj-m:= mp3.o

modules:
	$(MAKE) -C $(KERNEL_SRC) M=$(SUBDIR) modules

install: mp3.ko
	sudo insmod mp3.ko
uninstall:
	sudo rmmod mp3
node:
	sudo mknod node c $(shell cat /proc/devices | grep mp3_chrdev | awk '{print $$1}') 0
	sudo chown $(shell whoami) node 

clean:
	$(RM) -f node userapp *~ *.ko *.o *.mod.c Module.symvers modules.order .mp3.* -r .tmp_versions
