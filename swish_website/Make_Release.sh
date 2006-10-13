#!/bin/sh

DIR="${BASE_DIR:=$HOME/swish}"

if test ! -n "$1"; then
    echo "Must specify URL to fetch"
    exit 1
fi

TAR_URL="$1"

swish-daily.pl \
    --fetchtarurl="$TAR_URL" \
    --topdir=$DIR/swish_release_build \
    --noremove \
    --notimestamp \
    --verbose \
    --tardir=$DIR/swish-releases || exit 1;


# TODO FIXME ;)
# bin/build -swishsrc=/home/bmoseley/swish/swish_release_build/latest_swish_build/swish-e-2.4.4   -all

# swish-daily needs to symlink to the src directory so can do:
# bin/build -swishsrc=/home/bmoseley/swish/swish_release_build/latest_swish_build/src   -all



