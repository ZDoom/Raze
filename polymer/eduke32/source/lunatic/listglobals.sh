#!/bin/sh

if [ -z "$1" ]; then
    # S or G can be used to display SETGLOBALs or GETGLOBALs, respectively
    echo "Usage: $0 <file.lua> [S|G]"
    exit 1;
fi

# Strip LuaJIT specific syntax
sed -r -e "s/[0-9]+U?LL/0/g" "$1" | luac -p -l - | grep "$2ETGLOBAL" |
    # mark where the new module environment starts
    sed -e "s/; module/; module ____________________/"
