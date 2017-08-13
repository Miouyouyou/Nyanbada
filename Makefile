MYY_KERNEL_DIR ?= ../RockMyy/linux
ARCH ?= arm
CROSS_COMPILE ?= armv7a-hardfloat-linux-gnueabi-

ccflags-y += -DMYY_TESTS

myy-vpu-objs := dummy.o

# Careful, obj-y is for elements built into the kernel !
# obj-m is for elements built as modules
# We're building a module, so obj-m is required !
obj-m += myy-vpu.o

all:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) M=$(PWD) -C $(MYY_KERNEL_DIR) modules

clean:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) M=$(PWD) -C $(MYY_KERNEL_DIR) clean

install:
	scp myy-vpu.ko 10.100.0.55:/tmp
