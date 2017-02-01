#!/bin/bash

function usage ()
{
    echo 'Usage: checkdefs.sh <some.def> [[<search_path_base_dir>] {-patch,-con,-conpatch}]'
    exit 1
}

if [ -z "$1" ]; then
    usage
fi
deffn="$1"

if [ ! -f "$deffn" ]; then
    echo "Error: First argument must be a name of an existing DEF or CON file."
    usage
fi

if [ -z "$2" ]; then
    thedir=.
else
    thedir="$2"
fi

if [ ! -d "$thedir" ]; then
    echo "Error: Second argument must be a name of an existing directory."
    usage
fi

dopatch=""
docon=""
if [ -n "$3" ]; then
    if [ "$3" != "-patch" -a "$3" != "-con" -a "$3" != "-conpatch" ]; then
        echo 'Usage: checkdefs.sh <some.def> [[<search_path_base_dir>] {-patch,-con,-conpatch}]'
        exit 1
    fi
    if [ "$3" == "-patch" ]; then
        dopatch=1
    elif [ "$3" == "-con" ]; then
        docon=1
    else
        docon=1
        dopatch=1
    fi
fi

if [ -z $docon ]; then
    # def
    files=$(grep -E "\"[^ ]*\..*\"" "$deffn" | sed 's/.*"\(.*\..*\)".*/\1/g')
    files2=$(grep -E '^include ' "$deffn" | sed 's/^include \(.*\)\r/\1/g')  # XXX: \r

    files="$files $files2"
else
    # con... this is awful
    files=$(grep -E -i '(include *.*\.con)|(definesound.*(voc|wav|ogg))|(definelevelname.*\.map)' "$deffn" |
        sed -r 's/.*include[ \t]+([^ \t]+\.[cC][oO][nN]).*/\1/g;
                s/.*[ \t]([^ \t].+\.([wW][aA][vV]|[vV][oO][cC]|[oO][gG][gG])).*/\1/g;
                s/.*[ \t]([^ \t].+\.([mM][aA][pP])).*/\1/g;')
fi

exfiles=$(find -L "$thedir" -type f)

sedcmd=""

for i in $files; do
    fn="$thedir/$i"
    if [ ! -f "$fn" ]; then
        # try finding a matching file
        match=$(echo "$exfiles" | grep -i "^$fn$")

        if [ -z "$match" ]; then
            if [ -z "$docon" ]; then  # avoid spamming files not found that are in GRPs...
                echo "$fn"
            fi
        else
            echo "$fn --> $match"
            if [ -n "$dopatch" ]; then
                if [ -z "$docon" ]; then
                    sedcmd="$sedcmd;s|\"$i\"|\"${match#$thedir/}\"|g"
                else
                    sedcmd="$sedcmd;s|$i|${match#$thedir/}|g"
                fi
            fi
        fi
    fi
done

if [ -n "$dopatch" ]; then
    if [ -n "$sedcmd" ]; then
        echo "Patching $deffn"
#        echo "$sedcmd"
        sed -i "$sedcmd" "$deffn"
    fi
fi
