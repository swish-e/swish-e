#!/bin/sh
# This script documents how to build SWISH-E for Win32 under Linux

# Let's just set the PATH once for the whole script
PATH=/usr/local/cross-tools/bin:/usr/local/cross-tools/i586-mingw32msvc/bin:$PATH

# Remove the cache for our configure script else we will have problems.
rm -f config.cross.cache

# Take note of the host, target and build options.  If you're building
# on another OS you will want change these.
#   libxml2, zlib, pcre are the build directory for each.
./configure --cache-file=config.cross.cache \
        --host=i586-mingw32msvc \
        --target=i586-mingw32msvc \
        --build=i686-linux \
        --with-libxml2=$PWD/../libxml2 \
        --with-zlib=$PWD/../zlib \
        --with-pcre=$PWD/../pcre

make 

