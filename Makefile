# Makefile for the template kernel module

DRIVERNAME=template

ifneq ($(KERNELRELEASE),)
    obj-m := ${DRIVERNAME}.o
else
    PWD := $(shell pwd)

default:
ifeq ($(strip $(KERNELDIR)),)
	$(error "KERNELDIR is undefined!")
else
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules 
endif


clean:
	rm -rf *~ *.ko *.o *.mod.c modules.order Module.symvers .${DRIVERNAME}* .tmp_versions

endif

