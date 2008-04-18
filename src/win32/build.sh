#!/bin/bash
# This script documents how to build SWISH-E for Win32 under Linux

# To jumpstart your development here are pcre, libxml2, and zlib:
# http://www.webaugur.com/wares/files/swish-e/builddir.zip

# You also need the following from Debian (unstable?):
# apt-get install mingw32 mingw32-binutils mingw32-runtime

# Host Arch
HA=i586-mingw32msvc

# Build System Arch
BA=i686-linux

# Remove the cache for our configure script else we will have problems.
rm -f config.cross.cache

# Take note of the host, target and build options.  If you're building
# on another OS you will want change these.
#   libxml2, zlib, pcre are the build directory for each.
./configure --prefix=${PWD}/../prefix \
        --cache-file=config.cross.cache \
	--disable-docs \
        --host=${HA} \
        --target=${HA} \
        --build=${BA} \
        --with-libxml2=$PWD/../prefix \
        --with-zlib=$PWD/../prefix \
        --with-pcre=$PWD/../prefix \
        --with-db=$PWD/../prefix \
        --enable-shared \
        CFLAGS=$PWD/../prefix/include \
        LDFLAGS=$PWD/../prefix/lib \
        LIBS=-ldb-4.5

# Build Binaries
make

# Build SWISH::API
pushd perl
make -f Makefile.mingw
popd

