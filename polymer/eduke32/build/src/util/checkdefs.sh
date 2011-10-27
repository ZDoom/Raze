#!/bin/bash

if [ -z "$1" ]; then
    echo 'Usage: checkdefs.sh <some.def> [[<some_dir>] -patch]'
fi
deffn="$1"

if [ -z "$2" ]; then
    thedir=.
else
    thedir="$2"
fi

if [ -n "$3" ]; then
    if [ "$3" != "-patch" ]; then
        echo 'Usage: checkdefs.sh <some.def> [[<some_dir>] -patch]'
    fi
fi

files=$(grep -E "\".*\..*\"" "$deffn" | sed 's/.*"\(.*\..*\)".*/\1/g')

exfiles=$(find "$thedir" -type f)

sedcmd=""

for i in $files; do
    fn="$thedir/$i"
    if [ ! -f "$fn" ]; then
        # try finding a matching file
        match=$(echo "$exfiles" | grep -i "^$fn$")

        if [ -z "$match" ]; then
            echo "$fn"
        else
            echo "$fn --> $match"
            sedcmd="$sedcmd;s|\"$i\"|\"${match#$thedir/}\"|g"
        fi
    fi
done

if [ -n "$3" ]; then
    if [ -n "$sedcmd" ]; then
        echo "Patching $deffn"
#        echo "$sedcmd"
        sed -i "$sedcmd" "$deffn"
    fi
fi
