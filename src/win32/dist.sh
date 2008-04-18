#!/bin/bash

# You need a native Linux version of makensis.  This is not easy to come by.
# Here you'll find a binary tarball and Debian package:
#     http://webaugur.com/wares/files/makensis/

# You'll also need the following from Debian: 
# apt-get install sysutils



# Convert our documentation and scripts to DOS format
find . -type f -regex ".*/\(README\|COPYING\)" -exec mv {} {}.txt \;
find . -type f -regex ".*/\(.*\(\.\)\(txt\|html\|pm\|pl\|html\|tt\|tmpl\|cgi\)\|swishspider\)" -exec unix2dos {} \; 2>&1

# Build the installer executable, assumes makensis is in PATH
makensis src/win32/release.nsi

