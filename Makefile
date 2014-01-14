obj-m += mtcadummy.o
#for code readability the mtcadummy modue is split into the main driver
#functionality (mtcadummy_drv) and the proc file debug output (mtcadummy_proc)
mtcadummy-objs := mtcadummy_drv.o mtcadummy_proc.o mtcadummy_firmware.o

#CFLAGS_mtcautc.o = -include $(PWD)/mtcadrvCfg_utc.h	    
#CFLAGS_mtcaadc.o = -include $(PWD)/mtcadrvCfg_sis.h

KVERSION = $(shell uname -r)

all:	
	make -C /lib/modules/$(KVERSION)/build V=1 M=$(PWD) modules

install: all
	make -C /lib/modules/$(KVERSION)/build V=1 M=$(PWD) modules_install
	depmod
	#cp mtcadrv_io.h /usr/local/include

clean:
	test ! -d /lib/modules/$(KVERSION) || make -C /lib/modules/$(KVERSION)/build V=1 M=$(PWD) clean
