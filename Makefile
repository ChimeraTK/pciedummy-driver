obj-m += llrfdummy.o
			    
#CFLAGS_llrfutc.o = -include $(PWD)/llrfdrvCfg_utc.h	    
#CFLAGS_llrfadc.o = -include $(PWD)/llrfdrvCfg_sis.h

KVERSION = $(shell uname -r)

all:	
	make -C /lib/modules/$(KVERSION)/build V=1 M=$(PWD) modules
clean:
	test ! -d /lib/modules/$(KVERSION) || make -C /lib/modules/$(KVERSION)/build V=1 M=$(PWD) clean
