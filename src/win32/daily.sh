#!/bin/sh

# Web directory where we share these builds
WEBSHARE=$1

if [ "${WEBSHARE}x" = "x" ]; then echo "Usage: daily.sh <web share>"; fi
if [ ! -e ${WEBSHARE} ]; then echo "daily.sh: web share not found"; fi

# Generate a name for this build
TODAYIS=`date +%Y-%m-%d`
BUILDNAME="swish-e-${TODAYIS}"

# Set the proper CVSROOT
alias cvsswish="cvs -d:pserver:anonymous@cvs.SWISHE.sourceforge.net:/cvsroot/swishe"

# Checkout
cvsswish co -d $BUILDNAME swish-e 2>&1

# Build the installer
cd $BUILDNAME
src/win32/build.sh || echo "Error Building SWISH-E" 1>&2
src/win32/dist.sh  || echo "Error Creating SWISH-E Installer: Is WINE using ttydrv?" 1>&2

# SWISH-E VERSION
VERSION=`grep -x VERSION=.* configure | cut -d \= -f 2-`
DISTNAME=swish-e-${VERSION}-${TODAYIS}
echo $DISTNAME

# Share via the web
mv swishsetup.exe ${WEBSHARE}/${DISTNAME}.exe

