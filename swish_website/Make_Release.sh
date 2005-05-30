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

