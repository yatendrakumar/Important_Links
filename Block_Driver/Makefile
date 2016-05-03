ifeq ($(KERNELRELEASE),)

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

.PHONY: build clean insert remove

build:
	sudo $(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	sudo rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions modules.order Module.symvers

i:
	$(info *** Inserting module "$m" ***)
	sudo insmod $(m).ko

r:
	$(info *** Removing module "$m" ***)
	sudo rmmod $(m)

else

$(info Building with KERNELRELEASE = ${KERNELRELEASE})
obj-m :=    Block_Driver.o

endif
