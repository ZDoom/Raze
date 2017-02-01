#!/bin/sh

DIFF="git diff -U2 --no-index --color-words"
CMD="/usr/bin/env luajit ./map2text.lua"

opt=""

# Name of the 'tempfile' or 'mktemp' command (or full path).
tempfile_cmd=tempfile

tempfile_path=`which "$tempfile_cmd"`
if [ -z "$tempfile_path" ]; then
    echo "Error: tempfile_cmd ($tempfile_cmd) must be the name of existing 'tempfile' or 'mktemp' executable."
    exit 1
fi

if [ "$1" = "-c" -o "$1" = "-C" ]; then
    opt="$1"
    shift
fi

if [ -z "$1" -o -z "$2" ]; then
    echo "Usage: ./mapdiff.sh [-c] <file.map> <file2.map>"
    exit 1
fi

tf1=`"$tempfile_cmd"`
if [ -z "$tf1" ]; then
    echo Failed creating temp file
    exit 2
fi

tf2=`"$tempfile_cmd"`
if [ -z "$tf2" ]; then
    rm "$tf1"
    echo Failed creating temp file
    exit 2
fi

$CMD $opt "$1" > "$tf1"
$CMD $opt "$2" > "$tf2"

$DIFF "$tf1" "$tf2"

rm "$tf1"
rm "$tf2"
