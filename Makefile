obj-m += utcadummy.o
			    
#CFLAGS_utcautc.o = -include $(PWD)/utcadrvCfg_utc.h	    
#CFLAGS_utcaadc.o = -include $(PWD)/utcadrvCfg_sis.h

KVERSION = $(shell uname -r)

all:	
	make -C /lib/modules/$(KVERSION)/build V=1 M=$(PWD) modules
clean:
	test ! -d /lib/modules/$(KVERSION) || make -C /lib/modules/$(KVERSION)/build V=1 M=$(PWD) clean
