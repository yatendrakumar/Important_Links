KDIR:=~/DD_Exp/Beagle_Bone_Black/Working_Source/Janki_kernel/kernel/kernel/

PWD:=$(shell pwd)
CROSS_COMPILE=/home/yatendra/DD_Exp/Beagle_Bone_Black/Working_Source/Git/gcc-linaro-arm-linux-gnueabihf-4.8-2014.04_linux/bin/arm-linux-gnueabihf-

.PHONY: build clean app

build:
	$(MAKE) ARCH=arm CROSS_COMPILE=${CROSS_COMPILE} -C $(KDIR) M=$(PWD) modules 

clean:
	sudo rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions modules.order Module.symvers .Null_Driver.c.sw* .Makefile.swp

app:
	${CROSS_COMPILE}gcc app.c

copy:
	sudo cp my_eeprom.ko a.out /var/lib/tftpboot/

obj-m :=  my_eeprom.o
