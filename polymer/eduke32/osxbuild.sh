#!/bin/bash

# build debug/release x86/x86_64/ppc versions of mapster32 and eduke32 on OS X

# Variable presets:
buildppc=1
build86=1
build64=1
builddebug=0
onlyzip=0

# Enforce OS:
if [ `uname -s` != Darwin ]; then
    echo This script is for OS X only.
    exit 1
fi

# Detect and account for OS X version:
darwinversion=`uname -r | cut -d . -f 1`

if [ `expr $darwinversion \< 8` == 1 ]; then
    echo OS X 10.4 is the minimum requirement for building.
    exit 1
fi
if [ `expr $darwinversion \< 9` == 1 ]; then
    build64=0
fi
if [ `expr $darwinversion \> 10` == 1 ]; then
    buildppc=0
fi

# Parse arguments:
for i in $*; do
    case $i in
        onlyzip)
            onlyzip=1
        ;;

        noppc)
            buildppc=0
        ;;
        no86)
            build86=0
        ;;
        no64)
            build64=0
        ;;

        --debug=*)
        builddebug=${i#*=}
        ;;

        *)
            echo usage: osxbuild [onlyzip] [noppc] [no86] [no64] [--debug=\<0|1\>]
            exit 1
        ;;
    esac
done

# Detect versioning systems and pull the revision number:
rev=`svn info | grep Revision | awk '{ print $2 }'`
vc=svn
if [ -z "$rev" ]; then
    vc=git
    rev=`git svn info | grep 'Revision' | awk '{ print $2 }'`
fi

if [ -n "$rev" ]; then
    # throw the svn revision into a header.  this is ugly.
    echo "const char *s_buildRev = \"r$rev\";" > source/rev.h
else
    rev=unknown
    vc=none
fi

# The build process itself:
if [ $onlyzip -eq 0 ]; then
    rm -f {eduke32,mapster32}{.debug,}{.x86,.x64,.ppc,}
    rm -rf {EDuke32,Mapster32}{.debug,}.app

    if [ $build64 == 1 ]; then
        if [ $builddebug == 1 ]; then
            make veryclean
            ARCH='-arch x86_64' OSX_STARTUPWINDOW=1 WITHOUT_GTK=1 RELEASE=0 BUILD32_ON_64=0 USE_LIBVPX=1 make -j 3
            if [ $? ]; then
                echo x86_64 debug build succeeded.
                cp "Mapster32.app/Contents/MacOS/mapster32" mapster32.debug.x64
                cp "EDuke32.app/Contents/MacOS/eduke32" eduke32.debug.x64
            else
                echo x86_64 debug build failed.
            fi
        fi

        make veryclean
        ARCH='-arch x86_64' OSX_STARTUPWINDOW=1 WITHOUT_GTK=1 RELEASE=1 BUILD32_ON_64=0 USE_LIBVPX=1 make -j 3
        if [ $? ]; then
            echo x86_64 release build succeeded.
            cp "Mapster32.app/Contents/MacOS/mapster32" mapster32.x64
            cp "EDuke32.app/Contents/MacOS/eduke32" eduke32.x64
        else
            echo x86_64 release build failed.
        fi
    fi

    if [ $build86 == 1 ]; then
        if [ $builddebug == 1 ]; then
            make veryclean
            OSX_STARTUPWINDOW=1 WITHOUT_GTK=1 RELEASE=0 BUILD32_ON_64=1 USE_LIBVPX=0 make -j 3
            if [ $? ]; then
                echo x86 debug build succeeded.
                cp "Mapster32.app/Contents/MacOS/mapster32" mapster32.debug.x86
                cp "EDuke32.app/Contents/MacOS/eduke32" eduke32.debug.x86
            else
                echo x86 debug build failed.
            fi
        fi

        make veryclean
        OSX_STARTUPWINDOW=1 WITHOUT_GTK=1 RELEASE=1 BUILD32_ON_64=1 USE_LIBVPX=0 make -j 3
        if [ $? ]; then
            echo x86 release build succeeded.
            cp "Mapster32.app/Contents/MacOS/mapster32" mapster32.x86
            cp "EDuke32.app/Contents/MacOS/eduke32" eduke32.x86
        else
            echo x86 release build failed.
        fi
    fi

    if [ $buildppc == 1 ]; then
        if [ $builddebug == 1 ]; then
            make veryclean
            ARCH='-arch ppc' OSX_STARTUPWINDOW=1 WITHOUT_GTK=1 RELEASE=0 BUILD32_ON_64=0 USE_LIBVPX=0 make -j 3
            if [ $? ]; then
                echo PowerPC debug build succeeded.
                cp "Mapster32.app/Contents/MacOS/mapster32" mapster32.debug.ppc
                cp "EDuke32.app/Contents/MacOS/eduke32" eduke32.debug.ppc
            else
                echo PowerPC debug build failed.
            fi
        fi

        make veryclean
        ARCH='-arch ppc' OSX_STARTUPWINDOW=1 WITHOUT_GTK=1 RELEASE=1 BUILD32_ON_64=0 USE_LIBVPX=0 make -j 3
        if [ $? ]; then
            echo PowerPC release build succeeded.
            cp "Mapster32.app/Contents/MacOS/mapster32" mapster32.ppc
            cp "EDuke32.app/Contents/MacOS/eduke32" eduke32.ppc
        else
            echo PowerPC release build failed.
        fi
    fi
fi

# clean up, clean up, everybody everywhere, clean up, clean up, everybody do your share
if [ "$vc" == "svn" ]; then
    svn revert "source/rev.h"
elif [ "$vc" == "git" ]; then
    git checkout "source/rev.h"
fi

# Duplicating .app bundles for debug build:
if [ $builddebug == 1 ]; then
    for i in Mapster32 EDuke32; do
        if [ -d "$i.app" ]; then
            cp -RP "$i.app" "$i.debug.app"
        fi
    done
fi

# Begin assembling archive contents:
arcontents="README.OSX"

echo Creating fat binaries.
success=0
for i in {eduke32,mapster32}{.debug,}; do
    binaries=
    for j in ${i}.{x86,x64,ppc}; do
        if [ -f "$j" ]; then
            binaries="$binaries $j"
            success=1
        fi
    done
    if [ -n "$binaries" ]; then
        lipo -create $binaries -output $i
        app=${i//eduke32/EDuke32}
        app=${app//mapster32/Mapster32}.app
        cp -f $i "$app/Contents/MacOS/${i%.debug}"
        arcontents="$arcontents $app"
    fi
done

# Almost done...
if [ $success == 1 ]; then

    # Output README.OSX:
    let "darwinversion -= 4"
    echo "This archive was produced from revision $rev by the osxbuild.sh script." > README.OSX
    echo "Built on: Mac OS X 10.$darwinversion" >> README.OSX
    echo "EDuke32 home: http://www.eduke32.com" >> README.OSX
    echo "OS X build discussion on Duke4.net: http://forums.duke4.net/topic/4242-building-eduke-on-mac-os-x/" >> README.OSX
    echo "The 64-bit build in this archive has LibVPX (http://www.webmproject.org/code/)" >> README.OSX
    echo "from MacPorts (http://www.macports.org/) statically linked into it." >> README.OSX

    arfilename="eduke32-osx-$rev.zip"
    rm -f "$arfilename"
    zip -r -y "$arfilename" $arcontents -x "*.svn*" "*.git*"
fi
