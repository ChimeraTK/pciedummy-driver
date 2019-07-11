#!/bin/bash

#drop out if an error occurs
set -e

#Configure the debian directory.

#First determine the package version from the source code
MTCADUMMY_PACKAGE_VERSION=`grep "define MTCADUMMY_MODULE_VERSION" mtcadummy.h | sed "{s/^.*MTCADUMMY_MODULE_VERSION *\"//}" | sed "{s/\".*$//}"`

#The package versions for doocs / Ubuntu contain the codename of the distribution. Get it from the system.
CODENAME=`lsb_release -c | sed "{s/Codename:\s*//}"`

#To create the changelog we use debchange because it does the right format and the current date, user name and email automatically for us.
#Use the NAME and EMAIL environment variables to get correct values if needed (usually the email is
# user@host instead of first.last@institute, for instance killenb@mskpcx18571.desy.de instead of martin.killenberg@desy.de).
tag=$(git describe --tags --abbrev=0)
last_tag=$(git describe --tags --abbrev=0 $tag~1)

echo "Adding creating changelog with commit between tags ${tag} and ${last_tag}"

rm -rf debian/changelog
debchange --create --package mtcadummy-dkms -v ${MTCADUMMY_PACKAGE_VERSION}-${CODENAME}1 --distribution ${CODENAME} Debian package for the mtcadummy kernel module.
git log --format="%s" $last_tag.. | while read line; do
    debchange -a "${line}"
done

#Now everything is prepared and we can actually build the package.
#If you have a gpg signature you can remove the -us and -uc flags and sign the package.

[ x"$NOBUILD" == "x" ] && dpkg-buildpackage -rfakeroot -us -uc
