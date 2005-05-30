#!/bin/sh

DIR="${BASE_DIR:=$HOME/swish}"


swish-daily.pl \
    --topdir=$DIR/swish_daily_build \
    --tardir=$DIR/swish-daily || exit 1;

