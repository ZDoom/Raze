#!/bin/bash

# build debug/release x86/x86_64/ppc versions of mapster32 and eduke32 on OS X

# Variable presets:
buildppc=1
build86=1
build64=1
buildtools=0
builddebug=0
onlyzip=0

# Enforce OS:
if [ `uname -s` != Darwin ]; then
    echo This script is for OS X only.
    exit 1
fi

# Detect and account for OS X version:
darwinversion=`uname -r | cut -d . -f 1`

if [ `expr $darwinversion \< 9` == 1 ]; then
    build64=0
    echo OS X 10.5 is the minimum requirement for building.
    exit 1
fi
if [ `expr $darwinversion \> 9` == 1 ]; then
    buildppc=0
fi


# Parse arguments:
for i in $*; do
    case $i in
        onlyzip)
            onlyzip=1
        ;;

        --buildppc=*)
            buildppc=${i#*=}
        ;;
        --build86=*)
            build86=${i#*=}
        ;;
        --build64=*)
            build64=${i#*=}
        ;;

        --tools=*)
            buildtools=${i#*=}
        ;;

        --debug=*)
        builddebug=${i#*=}
        ;;

        *)
            echo usage: osxbuild [onlyzip] [--buildppc=\<0\|1\>] [--build86=\<0\|1\>] [--build64=\<0\|1\>] [--tools=\<0\|1\>] [--debug=\<0\|1\>]
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

# A little factoring:
commonargs="OSX_STARTUPWINDOW=1 WITHOUT_GTK=1"
if [ $buildppc == 1 ]; then
    commonargs="$commonargs DARWIN9=1"
fi

# Building the buildtools:
if [ $buildtools -eq 1 ] && [ -d "build" ]; then
    cd build

    rm -f *{.x86,.x64,.ppc}
    make veryclean
    EXESUFFIX_OVERRIDE=.debug make veryclean

    if [ $build64 == 1 ]; then
        if [ $builddebug == 1 ]; then
            make veryclean
            cmdline="ARCH='-arch x86_64' EXESUFFIX_OVERRIDE=.debug.x64 $commonargs RELEASE=0 BUILD32_ON_64=0 USE_LIBVPX=1 make -k utils"
            eval $cmdline
            if [ $? ]; then
                echo buildtools: x86_64 debug build succeeded.
            else
                echo buildtools: x86_64 debug build failed.
            fi
        fi

        make veryclean
        cmdline="ARCH='-arch x86_64' EXESUFFIX_OVERRIDE=.x64 $commonargs RELEASE=1 BUILD32_ON_64=0 USE_LIBVPX=1 make -k utils"
        eval $cmdline
        if [ $? ]; then
            echo buildtools: x86_64 release build succeeded.
        else
            echo buildtools: x86_64 release build failed.
        fi
    fi

    if [ $build86 == 1 ]; then
        if [ $builddebug == 1 ]; then
            make veryclean
            cmdline="EXESUFFIX_OVERRIDE=.debug.x86 $commonargs RELEASE=0 BUILD32_ON_64=1 USE_LIBVPX=0 make -k utils"
            eval $cmdline
            if [ $? ]; then
                echo buildtools: x86 debug build succeeded.
            else
                echo buildtools: x86 debug build failed.
            fi
        fi

        make veryclean
        cmdline="EXESUFFIX_OVERRIDE=.x86 $commonargs RELEASE=1 BUILD32_ON_64=1 USE_LIBVPX=0 make -k utils"
        eval $cmdline
        if [ $? ]; then
            echo buildtools: x86 release build succeeded.
        else
            echo buildtools: x86 release build failed.
        fi
    fi

    if [ $buildppc == 1 ]; then
        if [ $builddebug == 1 ]; then
            make veryclean
            cmdline="ARCH='-arch ppc' EXESUFFIX_OVERRIDE=.debug.ppc $commonargs RELEASE=0 BUILD32_ON_64=0 USE_LIBVPX=0 make -k utils"
            eval $cmdline
            if [ $? ]; then
                echo buildtools: PowerPC debug build succeeded.
            else
                echo buildtools: PowerPC debug build failed.
            fi
        fi

        make veryclean
        cmdline="ARCH='-arch ppc' EXESUFFIX_OVERRIDE=.ppc $commonargs RELEASE=1 BUILD32_ON_64=0 USE_LIBVPX=0 make -k utils"
        eval $cmdline
        if [ $? ]; then
            echo buildtools: PowerPC release build succeeded.
        else
            echo buildtools: PowerPC release build failed.
        fi
    fi

    echo buildtools: Creating fat binaries.
    utils=`make printutils` `EXESUFFIX_OVERRIDE=.debug make printutils`
    for i in $utils; do
        binaries=
        for j in ${i}.{x86,x64,ppc}; do
            if [ -f "$j" ]; then
                binaries="$binaries $j"
            fi
        done
        if [ -n "$binaries" ]; then
            lipo -create $binaries -output $i
        fi
    done

    if [ -d "/opt/local/bin" ]; then
        echo buildtools: Installing to MacPorts search path.
        for i in $utils; do
            if [ -f "$i" ]; then
                cp -f "$i" "/opt/local/bin/"
            fi
        done
    fi

    if [ -d "/usr/local/bin" ]; then
        echo buildtools: Installing to Homebrew search path.
        for i in $utils; do
            if [ -f "$i" ]; then
                cp -f "$i" "/usr/local/bin/"
            fi
        done
    fi

    cd ..
fi

# The build process itself:
if [ $onlyzip -eq 0 ]; then
    rm -f {eduke32,mapster32}{.debug,}{.x86,.x64,.ppc,}
    rm -rf {EDuke32,Mapster32}{.debug,}.app

    if [ $build64 == 1 ]; then
        if [ $builddebug == 1 ]; then
            make veryclean
            cmdline="ARCH='-arch x86_64' $commonargs RELEASE=0 BUILD32_ON_64=0 USE_LIBVPX=1 make"
            eval $cmdline
            if [ $? ]; then
                echo x86_64 debug build succeeded.
                cp "Mapster32.app/Contents/MacOS/mapster32" mapster32.debug.x64
                cp "EDuke32.app/Contents/MacOS/eduke32" eduke32.debug.x64
            else
                echo x86_64 debug build failed.
            fi
        fi

        make veryclean
        cmdline="ARCH='-arch x86_64' $commonargs RELEASE=1 BUILD32_ON_64=0 USE_LIBVPX=1 make"
        eval $cmdline
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
            cmdline="$commonargs RELEASE=0 BUILD32_ON_64=1 USE_LIBVPX=0 make"
            eval $cmdline
            if [ $? ]; then
                echo x86 debug build succeeded.
                cp "Mapster32.app/Contents/MacOS/mapster32" mapster32.debug.x86
                cp "EDuke32.app/Contents/MacOS/eduke32" eduke32.debug.x86
            else
                echo x86 debug build failed.
            fi
        fi

        make veryclean
        cmdline="$commonargs RELEASE=1 BUILD32_ON_64=1 USE_LIBVPX=0 make"
        eval $cmdline
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
            cmdline="ARCH='-arch ppc' $commonargs RELEASE=0 BUILD32_ON_64=0 USE_LIBVPX=0 make"
            eval $cmdline
            if [ $? ]; then
                echo PowerPC debug build succeeded.
                cp "Mapster32.app/Contents/MacOS/mapster32" mapster32.debug.ppc
                cp "EDuke32.app/Contents/MacOS/eduke32" eduke32.debug.ppc
            else
                echo PowerPC debug build failed.
            fi
        fi

        make veryclean
        cmdline="ARCH='-arch ppc' $commonargs RELEASE=1 BUILD32_ON_64=0 USE_LIBVPX=0 make"
        eval $cmdline
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
arcontents="README.OSX Changelog.txt"

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

    # Generate Changelog:
    lastrevision=`ls -A1 eduke32-osx* | tail -n1 | cut -d- -f3 | cut -d. -f1`

    if [ -z $lastrevision ]; then
        let lastrevision=rev-1
    elif [ $lastrevision -lt $rev ]; then
        let lastrevision+=1
    else
        let lastrevision=rev-1
    fi

    if [ "$vc" == "svn" ]; then
            svn log -r $rev:$lastrevision > Changelog.txt
    elif [ "$vc" == "git" ]; then
            git svn log -r $rev:$lastrevision > Changelog.txt
    fi

    # Package
    arfilename="eduke32-osx-$rev.zip"
    rm -f "$arfilename"
    zip -r -y "$arfilename" $arcontents -x "*.svn*" "*.git*"
fi
