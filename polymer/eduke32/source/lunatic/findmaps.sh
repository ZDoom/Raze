#!/bin/sh

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

find -L "$1" -iname '*.map' -print0 | xargs -0 ./foreachmap.lua "-e$2"
