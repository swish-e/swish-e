#!/bin/sh

# Web directory where we share these builds
WEBSHARE="/home/swish/public"

# Generate a name for this build
TODAYIS=`date +%Y-%m-%d`
BUILDNAME="swish-e-${TODAYIS}"

# Set the proper CVSROOT
export CVS_RSH="ssh"
#alias cvsswish="cvs -d:ext:augur@cvs.sourceforge.net:/cvsroot/swishe"
alias cvsswish="cvs -d:pserver:anonymous@cvs.sourceforge.net:/cvsroot/swishe"

# Checkout
cvsswish co -d $BUILDNAME swish-e 2>&1

# Build the installer
cd $BUILDNAME
src/win32/build.sh || echo "Error Building SWISH-E" 1>&2
# src/win32/distzip.sh  || echo "Error Creating SWISH-E ZIP File." 1>&2
src/win32/dist.sh  || echo "Error Creating SWISH-E Installer: Is WINE using ttydrv?" 1>&2

# SWISH-E VERSION
# `grep -x VERSION=.* configure | cut -d \= -f 2-`
export `grep ^MAJOR_VERSION configure`
export `grep ^MINOR_VERSION configure`
export `grep ^MICRO_VERSION configure`
VERSION=$MAJOR_VERSION.$MINOR_VERSION.$MICRO_VERSION
DISTNAME=swish-e-${VERSION}-${TODAYIS}
echo $DISTNAME

# Share via the web
mv src/win32/swishsetup.exe ${WEBSHARE}/${DISTNAME}.exe
# mv swish-e.zip ${WEBSHARE}/${DISTNAME}.zip
rm -f ${WEBSHARE}/swish-latest.exe
# rm -f ${WEBSHARE}/swish-latest.zip
ln -s ${WEBSHARE}/${DISTNAME}.exe ${WEBSHARE}/swish-latest.exe
# ln -s ${WEBSHARE}/${DISTNAME}.zip ${WEBSHARE}/swish-latest.zip
echo "${DISTNAME}" > ${WEBSHARE}/latest

