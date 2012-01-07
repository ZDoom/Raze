#!/bin/sh

# build debug/release x86/x64/ppc versions of mapster32 and eduke32 on OSX

if [ `uname -s` != Darwin ]; then
    echo This script is for OSX only.
    exit 1
fi

onlyzip=0

if [ $1 ]; then
    if [ $1 == onlyzip ]; then
        onlyzip=1
    else
        echo usage: osxbuild [onlyzip]
        exit 1
    fi
fi

if [ $onlyzip -eq 0 ]; then
    # make veryclean
    # WITHOUT_GTK=1 RELEASE=0 BUILD32_ON_64=0 make -j 3
    # if [ $? ]; then
    #     echo 64-bit debug build succeeded.
    #     cp mapster32 mapster32.debug.x64
    #     cp eduke32 eduke32.debug.x64
    # else
    #     echo 64-bit debug build failed.
    # fi

    make veryclean
    WITHOUT_GTK=1 RELEASE=1 BUILD32_ON_64=0 USE_LIBVPX=1 make -j 3
    if [ $? ]; then
        echo 64-bit release build succeeded.
        cp mapster32 mapster32.x64
        cp eduke32 eduke32.x64
    else
        echo 64-bit release build failed.
    fi

    # make veryclean
    # WITHOUT_GTK=1 RELEASE=0 BUILD32_ON_64=1 make -j 3
    # if [ $? ]; then
    #     echo 32-bit debug build succeeded.
    #     cp mapster32 mapster32.debug.x86
    #     cp eduke32 eduke32.debug.x86
    # else
    #     echo 32-bit debug build failed.
    # fi

    make veryclean
    WITHOUT_GTK=1 RELEASE=1 BUILD32_ON_64=1 USE_LIBVPX=0 make -j 3
    if [ $? ]; then
        echo 32-bit release build succeeded.
        cp mapster32 mapster32.x86
        cp eduke32 eduke32.x86
    else
        echo 32-bit release build failed.
    fi

    # make veryclean
    # ARCH='-arch ppc' WITHOUT_GTK=1 RELEASE=0 BUILD32_ON_64=0 make -j 3
    # if [ $? ]; then
    #     echo PowerPC debug build succeeded.
    #     cp mapster32 mapster32.debug.ppc
    #     cp eduke32 eduke32.debug.ppc
    # else
    #     echo PowerPC debug build failed.
    # fi

    # make veryclean
    # ARCH='-arch ppc' WITHOUT_GTK=1 RELEASE=1 BUILD32_ON_64=0 USE_LIBVPX=0 make -j 3
    # if [ $? ]; then
    #     echo PowerPC release build succeeded.
    #     cp mapster32 mapster32.ppc
    #     cp eduke32 eduke32.ppc
    # else
    #     echo PowerPC release build failed.
    # fi
fi

# Almost done...
if [ -f mapster32.x64 ] && [ -f eduke32.x86 ] && [ -f eduke32.ppc ]; then
    echo Creating fat binaries.
    lipo -create mapster32.x64 mapster32.x86  -output mapster32
#    lipo -create mapster32.debug.x64 mapster32.debug.x86 -output mapster32.debug
    lipo -create eduke32.x64 eduke32.x86  -output eduke32
#    lipo -create eduke32.debug.x64 eduke32.debug.x86 -output eduke32.debug
    rev=`svn info | grep Revision | awk '{ print $2 }'`
    arfilename="eduke32-osx-$rev.zip"
    echo "This archive was produced from revision $rev by the osxbuild.sh script." > README.OSX
    echo "EDuke32 home: http://www.eduke32.com" >> README.OSX
    echo "OSX build discussion on Duke4.net: http://forums.duke4.net/topic/4242-building-eduke-on-mac-os-x/" >> README.OSX
    echo "The 64-bit build in this archive has LibVPX (http://www.webmproject.org/code/)" >> README.OSX
    echo "from MacPorts (http://www.macports.org/) statically linked into it." >> README.OSX
    rm -f "$arfilename"
    zip "$arfilename" mapster32 eduke32 README.OSX
fi
