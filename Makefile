obj-m += utcadummy.o
#for code readability the utcadummy modue is split into the main driver
#functionality (utcadummy_drv) and the proc file debug output (utcadummy_proc)
utcadummy-objs := utcadummy_drv.o utcadummy_proc.o utcadummy_firmware.o

#CFLAGS_utcautc.o = -include $(PWD)/utcadrvCfg_utc.h	    
#CFLAGS_utcaadc.o = -include $(PWD)/utcadrvCfg_sis.h

KVERSION = $(shell uname -r)

all:	
	make -C /lib/modules/$(KVERSION)/build V=1 M=$(PWD) modules

install: all
	make -C /lib/modules/$(KVERSION)/build V=1 M=$(PWD) modules_install
	depmod
	#cp utcadrv_io.h /usr/local/include

clean:
	test ! -d /lib/modules/$(KVERSION) || make -C /lib/modules/$(KVERSION)/build V=1 M=$(PWD) clean
