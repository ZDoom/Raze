#!/bin/bash

ok=yes
if [ -z "$1" ]; then
    ok=
fi
if [ -z "$2" ]; then
    ok=
fi

if [ -z "$ok" ]; then
    echo "Usage: $0 <dir> <code for foreachmap.lua -e>"
    exit 1
fi

LOPT=-L
idx=$(expr match `uname -s` '[mM][iI][nN][gG][wW]')
if [ "$idx" != 0 ]; then
    LOPT=
fi

find $LOPT "$1" -iname '*.map' -print0 | xargs -0 ./foreachmap.lua "-e$2"
