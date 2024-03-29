obj-m += mtcadummy.o
#for code readability the mtcadummy modue is split into the main driver
#functionality (mtcadummy_drv) and the proc file debug output (mtcadummy_proc)
mtcadummy-objs := mtcadummy_drv.o mtcadummy_proc.o mtcadummy_firmware.o mtcadummy_rw_no_struct.o

KVERSION = $(shell uname -r)

#define the package/module version
#DO NOT FORGET TO ADAPT BOTH DRIVER AND MODULE VERSION!
MTCADUMMY_MAJOR_VERSION=0
MTCADUMMY_MINOR_VERSION=10
MTCADUMMY_PACKAGE_VERSION=0.10.3

MTCADUMMY_DKMS_SOURCE_DIR=/usr/src/mtcadummy-${MTCADUMMY_PACKAGE_VERSION}

KERNEL_SRC=/lib/modules/$(KVERSION)/build
KERNEL_SRC_BASE=$(shell dirname $(KERNEL_SRC))

all: configure-source-files	
	$(MAKE) -C $(KERNEL_SRC) V=1 M=$(PWD)

modules_install:
	$(MAKE) -C $(KERNEL_SRC) V=1 M=$(PWD) modules_install

#Performs a dkms install
install: dkms-prepare
	dkms install -m mtcadummy -v ${MTCADUMMY_PACKAGE_VERSION} -k $(KVERSION)

#Performs a dmks remove
#Always returns true, so purge also works if there is no driver installed
uninstall:
	dkms remove -m mtcadummy -v ${MTCADUMMY_PACKAGE_VERSION} -k $(KVERSION) || true

clean:
	test ! -d $(KERNEL_SRC_BASE) || $(MAKE) -C $(KERNEL_SRC) V=1 M=$(PWD) clean
	rm -f version.h

#uninstall and remove udev rules
purge: uninstall
	rm -rf ${MTCADUMMY_DKMS_SOURCE_DIR} /etc/udev/rules.d/10-mtcadummy.rules /etc/modules-load.d/mtcadummy.conf

#This target will only succeed on debian machines with the debian packaging tools installed
debian_package: configure-source-files
	./make_debian_package.sh

##### Internal targets usually not called by the user: #####

#A target which replaces the version number in the source files
configure-source-files:
	sed -e "{s/@MTCADUMMY_MAJOR_VERSION@/${MTCADUMMY_MAJOR_VERSION}/}" \
	-e "{s/@MTCADUMMY_MINOR_VERSION@/${MTCADUMMY_MINOR_VERSION}/}" \
	-e "{s/@MTCADUMMY_PACKAGE_VERSION@/${MTCADUMMY_PACKAGE_VERSION}/}" version.h.in > version.h

#A target which replaces the version number in the control files for
#dkms and debian packaging
configure-package-files:
	sed "{s/@MTCADUMMY_PACKAGE_VERSION@/${MTCADUMMY_PACKAGE_VERSION}/}" dkms.conf.in > dkms.conf
#	test -d debian_from_template || mkdir debian_from_template
#	cp dkms.conf debian_from_template/mtcadummy-dkms.dkms
#	(cd debian.in; cp compat  control  copyright ../debian_from_template)
#	cat debian.in/rules.in | sed "{s/@MTCADUMMY_PACKAGE_VERSION@/${MTCADUMMY_PACKAGE_VERSION}/}" > debian_from_template/rules
#	chmod +x debian_from_template/rules

#copies the package sources to the place needed by dkms
#The udev rules also have to be placed, so they are available for all kernels and not uninstalled if
# the module for one kernel is removed.
dkms-prepare: configure-source-files configure-package-files
	test -d ${MTCADUMMY_DKMS_SOURCE_DIR} || mkdir ${MTCADUMMY_DKMS_SOURCE_DIR}
	cp *.h *.c version.h.in Makefile dkms.conf *.rules ${MTCADUMMY_DKMS_SOURCE_DIR}
	install --mode=644 *.rules /etc/udev/rules.d
	install --mode=644 mtcadummy.conf /etc/modules-load.d
