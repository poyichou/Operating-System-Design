CONFIG_MODULE_SIG=n
EXTRA_CFLAGS +=
APP_EXTRA_FLAGS:= -O2 -ansi -pedantic
KERNEL_SRC:= /lib/modules/$(shell uname -r)/build
SUBDIR= $(PWD)
GCC:=gcc
RM:=rm
FILE:=mp3

.PHONY : clean

all: clean modules

obj-m:= $(FILE).o

modules:
	$(MAKE) -C $(KERNEL_SRC) M=$(SUBDIR) modules

install: $(FILE).ko
	sudo insmod $(FILE).ko
uninstall:
	sudo rmmod $(FILE)
node:
	sudo mknod node c $(shell cat /proc/devices | grep mp3_chrdev | awk '{print $$1}') 0
	sudo chown $(shell whoami) node 

clean:
	$(RM) -f node userapp *~ *.ko *.o *.mod.c Module.symvers modules.order .$(FILE).* -r .tmp_versions
