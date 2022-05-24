obj-m += mtcadummy.o
#for code readability the mtcadummy modue is split into the main driver
#functionality (mtcadummy_drv) and the proc file debug output (mtcadummy_proc)
mtcadummy-objs := mtcadummy_drv.o mtcadummy_proc.o mtcadummy_firmware.o mtcadummy_rw_no_struct.o

KVERSION := $(shell uname -r)

#define the package/module version
#DO NOT FORGET TO ADAPT BOTH DRIVER AND MODULE VERSION!
MTCADUMMY_MAJOR_VERSION=0
MTCADUMMY_MINOR_VERSION=10
MTCADUMMY_PACKAGE_VERSION=0.10.2

ifeq ($(PACKAGING_INSTALL),1)
install: packaging_install
else
install: kernel_install
endif

KERNEL_SRC ?= /lib/modules/$(KVERSION)/build
KERNEL_SRC_BASE=$(shell dirname $(KERNEL_SRC))

all: configure-source-files	
	$(MAKE) -C $(KERNEL_SRC) V=1 M=$(PWD)

modules_install:
	$(MAKE) -C $(KERNEL_SRC) V=1 M=$(PWD) modules_install

#Performs a dkms install
kernel_install:
	$(MAKE) -C $(KERNEL_SRC) V=1 M=$(PWD) install

packaging_install: configure-source-files
	install -m 644 -D *.h.in *.h *.c Makefile -t $(DESTDIR)/usr/src/$(DKMS_PKG_NAME)-$(DKMS_PKG_VERSION)/
	install -m 644 -D mtcadummy.conf -t $(DESTDIR)/etc/modules-load.d/
	install -m 644 -D 10-mtcadummy.rules -t $(DESTDIR)/etc/udev/rules.d

clean:
	test ! -d $(KERNEL_SRC_BASE) || $(MAKE) -C $(KERNEL_SRC) V=1 M=$(PWD) clean
	rm -f version.h

##### Internal targets usually not called by the user: #####

#A target which replaces the version number in the source files
configure-source-files:
	sed -e "{s/@MTCADUMMY_MAJOR_VERSION@/${MTCADUMMY_MAJOR_VERSION}/}" \
	-e "{s/@MTCADUMMY_MINOR_VERSION@/${MTCADUMMY_MINOR_VERSION}/}" \
	-e "{s/@MTCADUMMY_PACKAGE_VERSION@/${MTCADUMMY_PACKAGE_VERSION}/}" version.h.in > version.h

