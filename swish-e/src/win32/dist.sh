#!/bin/sh

INTERPRETER=wine

# Convert our documentation to DOS format
find . -regex ".*/\(README\|COPYING\)" -exec mv {} {}.txt \;
find . -regex ".*/.*\(\.\)*\(txt\|html\|pm\|pl\|html\|tt\|tmpl\|cgi\)" -exec unix2dos {} \;


$INTERPRETER c:/nsis/makensis src/win32/installer.nsi

