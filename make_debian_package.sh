#!/bin/bash

#drop out if an error occurs
set -e

#Configure the debian directory.

#First determine the package version from the source code
MTCADUMMY_PACKAGE_VERSION=`grep "define MTCADUMMY_MODULE_VERSION" mtcadummy.h | sed "{s/^.*MTCADUMMY_MODULE_VERSION *\"//}" | sed "{s/\".*$//}"`

#Configure the dkms and the rules file
cat debian/mtcadummy-dkms.dkms.in | sed "{s/@MTCADUMMY_PACKAGE_VERSION@/${MTCADUMMY_PACKAGE_VERSION}/}" > debian/mtcadummy-dkms.dkms
cat debian/rules.in | sed "{s/@MTCADUMMY_PACKAGE_VERSION@/${MTCADUMMY_PACKAGE_VERSION}/}" > debian/rules
chmod +x debian/rules

#The package versions for doocs / Ubuntu contain the codename of the distribution. Get it from the system.
CODENAME=`lsb_release -c | sed "{s/Codename:\s*//}"`

#To create the changelog we use debchange because it does the right format and the current date, user name and email automatically for us.
#Use the NAME and EMAIL environment variables to get correct values if needed (usually the email is
# user@host instead of first.last@institute, for instance killenb@mskpcx18571.desy.de instead of martin.killenberg@desy.de).
rm -rf debian/changelog
debchange --create --package mtcadummy-dkms -v ${MTCADUMMY_PACKAGE_VERSION}-${CODENAME}1 --distribution ${CODENAME} Debian package for the mtcadummy kernel module.

#Now everything is prepared and we can actually build the package.
#If you have a gpg signature you can remove the -us and -uc flags and sign the package.
dpkg-buildpackage -rfakeroot -us -uc
