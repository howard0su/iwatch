#!/bin/sh
PATH=$PATH:/usr/local/bin
VERSION=`date +%Y%m%d`
TARGETDIR=/home/junsu/public_html/${VERSION}
mkdir -p  ${TARGETDIR}
echo Build for ${TARGETDIR}

cd /home/junsu/iwatch
git checkout develop
git pull 

# output changes
LASTDAY=`date -v-1d +%Y/%m/%d`
git log --since=$LASTDAY --no-color  > ${TARGETDIR}/changes.log

WINEPREFIX=/mnt/wine;export WINEPREFIX
WINEDEBUG=-all;export WINEDEBUG
# wine /mnt/wine/drive_c/Program\ Files/IAR\ Systems/Embedded\ Workbench\ 6.4\ Evaluation/common/bin/IarBuild.exe x:/build/watch.ewp -build Retail -log all > ${TARGETDIR}/buildlog.log
cd /home/junsu/iwatch
rm -Rf objs.*
gmake -f Makefile.msp430 WINE=wine 1> ${TARGETDIR}/summary.log 2> ${TARGETDIR}/compile.log
gmake -f Makefile.efm32 CC=arm-eabi-gcc
 
cp objs.msp430/watch.txt ${TARGETDIR}
cp objs.efm32/IWATCH_W002.bin ${TARGETDIR}
cp objs.efm32/IWATCH_W002.elf ${TARGETDIR}

# build convert tool
cd /home/junsu/iwatch/tools/convertbin
gmake CC=clang
./convert ${TARGETDIR}/watch.txt ${TARGETDIR}/firmware-${VERSION}.bin

# run unittest
cd /home/junsu/iwatch
gmake CC=clang
mkdir -p unittest/screen/
./iwatch > ${TARGETDIR}/unittest.log

echo "Done."
