#!/bin/sh
# This script documents how to build SWISH-E for Win32 under Linux

# To jumpstart your development here are pcre, libxml2, and zlib:
# http://www.webaugur.com/wares/files/swish-e/builddir.zip

# Host Arch
HA=i386

# cross-compiler: gcc 3.3.1 20030804-1, binutils 2.15.91 20040904, mingw-runtime 3.3
PATH=/opt/mingw/bin:/opt/mingw/${HA}-mingw32msvc:$PATH

# Remove the cache for our configure script else we will have problems.
rm -f config.cross.cache

# Take note of the host, target and build options.  If you're building
# on another OS you will want change these.
#   libxml2, zlib, pcre are the build directory for each.
./configure --cache-file=config.cross.cache \
        --host=${HA}-mingw32msvc \
        --target=${HA}-mingw32msvc \
        --build=i686-linux \
        --with-libxml2=$PWD/../libxml2 \
        --with-zlib=$PWD/../zlib \
        --with-pcre=$PWD/../pcre

# Build docs
pushd doc
make
popd

# Build Binaries; pushd in case something else fails
make

