#!/bin/sh
# Adobe PDF filter
# see: http://www.foolabs.com/xpdf/ 

pdftotext "$1" - 2>/dev/null
