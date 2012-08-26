#!/bin/sh

DIFF="git diff --no-index --color-words"
CMD="/usr/bin/env luajit ./map2text.lua"

if [ `uname -s` != "Linux" ]; then
    # I think 'tempfile' isn't in POSIX. Feel free to use 'mktemp' or something
    # but absolutely test it before.
    echo "This helper script is for Linux only."
    return 1
fi

if [ -z "$1" -o -z "$2" ]; then
    echo "Usage: ./mapdiff.sh <file.map> <file2.map>"
    exit 1
fi

tf1=`tempfile`
tf2=`tempfile`

$CMD "$1" > "$tf1"
$CMD "$2" > "$tf2"

$DIFF "$tf1" "$tf2"

rm "$tf1"
rm "$tf2"
