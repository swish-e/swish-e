#!/bin/sh

# Web directory where we share these builds
WEBSHARE=/home/httpd/html/swish-e

# Generate a name for this build
TODAYIS=`date +%Y-%m-%d`
BUILDNAME="swish-e-${TODAYIS}"
# SWISH-E VERSION
VERSION=`grep -x VERSION=.* configure | cut -d \= -f 2-`
DISTNAME=swish-e-${VERSION}-${TODAYIS}
echo $DISTNAME

# Set the proper CVSROOT
alias cvsswish="cvs -d:pserver:anonymous@cvs.SWISHE.sourceforge.net:/cvsroot/swishe"

# Checkout
cvsswish co -d $BUILDNAME swish-e

# Build the installer
cd $BUILDNAME
src/win32/build.sh || echo "Error Building SWISH-E" 1>&2
src/win32/dist.sh  || echo "Error Creating SWISH-E Installer" 1>&2

# SWISH-E VERSION
VERSION=`grep -x VERSION=.* configure | cut -d \= -f 2-`
DISTNAME=swish-e-${VERSION}-${TODAYIS}
echo $DISTNAME

# Share via the web
mv swishsetup.exe ${WEBSHARE}/${DISTNAME}.exe

