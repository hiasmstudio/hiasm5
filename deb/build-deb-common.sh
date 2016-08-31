#!/bin/sh

PNAME=hiasm5-common

cp -R hiasm5-common-data $PNAME

wget https://raw.githubusercontent.com/hiasmstudio/hiasm5/master/src/share.h -o /dev/null
VERSION1=`cat share.h | grep "HIASM_VERSION_MAJOR " | cut -c 29-31 -`
VERSION2=`cat share.h | grep "HIASM_VERSION_MINOR " | cut -c 29-31 -`
VERSION3=`cat share.h | grep "HIASM_VERSION_BUILD " | cut -c 29-31 -`
VERSION=$VERSION1.$VERSION2.$VERSION3
rm share.h

sed -i "s/%version%$/$VERSION/" $PNAME/DEBIAN/control

cd $PNAME
md5deep -r usr > DEBIAN/md5sums
cd ../

fakeroot dpkg-deb --build $PNAME
rm -rf $PNAME
