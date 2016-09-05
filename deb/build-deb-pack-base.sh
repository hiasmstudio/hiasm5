#!/bin/bash

#--------------------------------------------------
# READ VERSION
if [ -e pack-ver/base ]; then
	VERSION=`cat pack-ver/base`
	let VERSION=$VERSION+1
else
	VERSION=1
fi
echo $VERSION > pack-ver/base
#--------------------------------------------------

PNAME=hiasm5-pack-base

mkdir -p $PNAME/DEBIAN
cp hiasm5-pack/co* $PNAME/DEBIAN/
mkdir -p $PNAME/usr/share/hiasm/elements/_base

git clone https://github.com/hiasmstudio/hiasm5.git
mv ./hiasm5/packs/_base/* $PNAME/usr/share/hiasm/elements/_base/
rm -rf hiasm5
cd $PNAME/usr/share/hiasm/elements/_base/
./sql_to_db.sh
cd ../../../../../../

sed -i "s/%version%$/0.0.$VERSION/" $PNAME/DEBIAN/control
sed -i "s/%pack%$/base/" $PNAME/DEBIAN/control
sed -i "s/%arch%$/all/" $PNAME/DEBIAN/control
sed -i "s/^Depends: $/Origin: vdv/" $PNAME/DEBIAN/control

cd $PNAME
md5deep -r usr > DEBIAN/md5sums
cd ../

fakeroot dpkg-deb --build $PNAME
rm -rf $PNAME
