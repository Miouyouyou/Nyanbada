ccflags-y += -DMYY_TESTS

myy-vpu-objs := dummy.o

# Careful, obj-y is for elements built into the kernel !
# obj-m is for elements built as modules
# We're building a module, so obj-m is required !
obj-m += myy-vpu.o
