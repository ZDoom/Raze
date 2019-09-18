#!/bin/bash

ok=yes
if [ -z "$1" ]; then
    ok=
fi
if [ -z "$2" ]; then
    ok=
fi

if [ -z "$ok" ]; then
    echo "Usage: $0 <dir> <some_foreachmap_module.lua | code for foreachmap.lua -e>"
    exit 1
fi

LOPT=-L
idx=$(expr match `uname -s` '[mM][iI][nN][gG][wW]')
if [ "$idx" != 0 ]; then
    LOPT=
fi

FN="$1"
ARG="$2"

idx=$(expr match "$ARG" '.*lua$')
if [ "$idx" == 0 ]; then
    ARG="-e$ARG"
    find $LOPT "$FN" -iname '*.map' -print0 | xargs -0 ./foreachmap.lua "$ARG"
else
    shift
    # So that you can e.g. do
    # ./findmaps.sh ~/.eduke32 ./colenemy.lua -u
    find $LOPT "$FN" -iname '*.map' -print0 | xargs -0 ./foreachmap.lua "$@"
fi
