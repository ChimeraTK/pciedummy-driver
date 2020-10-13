obj-m += mtcadummy.o
#for code readability the mtcadummy modue is split into the main driver
#functionality (mtcadummy_drv) and the proc file debug output (mtcadummy_proc)
mtcadummy-objs := mtcadummy_drv.o mtcadummy_proc.o mtcadummy_firmware.o mtcadummy_rw_no_struct.o

KVERSION = $(shell uname -r)

all:	
	make -C /lib/modules/$(KVERSION)/build V=1 M=$(PWD) modules

install: all
	make -C /lib/modules/$(KVERSION)/build V=1 M=$(PWD) modules_install
	cp 10-mtcadummy.rules /etc/udev/rules.d
	cp mtcadummy.modules-conf /etc/modules-load.d/mtcadummy.conf
	depmod

clean:
	test ! -d /lib/modules/$(KVERSION) || make -C /lib/modules/$(KVERSION)/build V=1 M=$(PWD) clean

debian_package:
	./make_debian_package.sh
