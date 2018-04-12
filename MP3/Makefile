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
node:
	mknod node c $(cat /proc/devices | grep mp3_chrdev | awk '{print $1}') 0

clean:
	$(RM) -f userapp *~ *.ko *.o *.mod.c Module.symvers modules.order .mp3.* -r .tmp_versions
