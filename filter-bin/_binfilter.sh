#!/bin/sh

# Example of a shell script filter

# Simple filter for binary files (eg: exe files) 

# Note: This is just an example of a shell script.  In general you would not
# use a shell script to just call a program -- rather call the program directly from
# swish using a FileFilter command.
# e.g. FileFilter .exe strings "'%p'"

strings "$1" - 2>/dev/null
