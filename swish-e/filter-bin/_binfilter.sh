#!/bin/sh

# Simple filter for binary files (eg: exe files) 

strings "$1" - 2>/dev/null
