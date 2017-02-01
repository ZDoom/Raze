#!/bin/bash

# build debug/release x86/x86_64/ppc versions of mapster32 and eduke32 on OS X

cd ..

# Variable presets:
buildppc=1
build86=1
build64=1
buildmain=1
buildtools=0
installtools=0
builddebug=0
buildrelease=1
pack=1
iamhelix=0
dummy=0
doclean=0
package=package
lastrevision=

# Enforce OS:
if [ `uname -s` != Darwin ]; then
    echo This script is for OS X only.
    exit 1
fi

# Detect and account for OS X version:
darwinversion=`uname -r | cut -d . -f 1`

if [ `expr $darwinversion \< 9` == 1 ]; then
    echo OS X 10.5 is the minimum requirement for building.
    exit 1
fi
if [ `expr $darwinversion \< 10` == 1 ]; then
    build64=0
    # x86_64 is disabled by default on Leopard to avoid the additional hassles of getting libvpx installed into a PowerPC environment.
fi
if [ `expr $darwinversion \> 9` == 1 ]; then
    buildppc=0
    # PPC is disabled by default on Snow Leopard for ease of installation.
fi
# All defaults can be overridden with the command-line parameters.

# Parse arguments:
for i in $*; do
    case $i in
        # onlyzip, helix: For Helixhorned's convenience.
        onlyzip)
            buildmain=0
            buildtools=0
            pack=1
        ;;
        helix)
            iamhelix=1
            buildppc=0
            build86=1
            build64=1
            buildmain=1
            buildtools=1
            builddebug=0
            pack=1
        ;;
        debughelix)
            iamhelix=1
            buildppc=0
            build86=0
            build64=1
            buildmain=1
            buildtools=0
            builddebug=1
            buildrelease=0
            pack=1
        ;;
        dummyhelix)
            iamhelix=1
            buildppc=0
            build86=0
            build64=0
            buildmain=0
            buildtools=0
            builddebug=0
            buildrelease=0
            pack=0
            dummy=1
        ;;
        clean)
            buildppc=0
            build86=0
            build64=0
            buildmain=0
            buildtools=0
            builddebug=0
            buildrelease=0
            pack=0
            doclean=1
        ;;
        # For the convenience of universal distributors:
        dist)
            buildppc=1
            build86=1
            build64=1
            buildmain=1
            buildtools=0
            builddebug=1
            pack=1
        ;;
        disttools)
            buildppc=1
            build86=1
            build64=1
            buildmain=0
            buildtools=1
            builddebug=1
            pack=0
        ;;
        full)
            buildppc=1
            build86=1
            build64=1
            buildmain=1
            buildtools=1
            builddebug=1
            pack=1
        ;;

        # misc.
        installtools)
            buildppc=0
            build86=0
            build64=0
            buildmain=0
            buildtools=0
            builddebug=0
            pack=0
            installtools=1
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

        --main=*)
            buildmain=${i#*=}
        ;;
        --tools=*)
            buildtools=${i#*=}
        ;;

        --debug=*)
        builddebug=${i#*=}
        ;;

        --pack=*)
            pack=${i#*=}
        ;;

        --lastrev=*)
            lastrevision=${i#*=}
        ;;

        *)
            echo "usage: ./osxbuild.sh [options]"
            echo "options:"
            echo "    [--buildppc=<0|1>] [--build86=<0|1>] [--build64=<0|1>]"
            echo "    [--debug=<0|1>]"
            echo "    [--main=<0|1>] [--tools=<0|1>]"
            echo "    [--pack=<0|1>]"
            echo "presets:"
            echo "    [onlyzip] [dist] [disttools] [full] [installtools]"
            exit 1
        ;;
    esac
done

# Mandatory disabled settings enforced:
if [ `expr $darwinversion \< 9` == 1 ]; then
    build64=0
    # x86_64 support did not exist before Leopard.
fi
if [ `expr $darwinversion \> 10` == 1 ]; then
    buildppc=0
    # PPC is disabled on Lion and up because support has been removed from the SDKs.
fi

# Detect versioning systems and pull the revision number:
export VC_REV=$(svn info 2> /dev/null | grep Revision | awk '{ print $2 }')
vc=svn
if [ -z "$VC_REV" ]; then
    vc=git
    export VC_REV=$(git svn info 2> /dev/null | grep Revision | awk '{ print $2 }')
fi

if [ -z "$VC_REV" ]; then
    export VC_REV=Unknown
    vc=none
fi

function dobuildtools ()
{
    make veryclean
    local cmdline="$2"
    eval $cmdline
    if [ $? ]; then
        echo buildtools: "$1" build succeeded.
    else
        echo buildtools: "$1" build failed.
        exit 1
    fi
}

function dobuildem()  # build EDuke32 and Mapster32
{
    make veryclean
    local cmdline="$2"
    eval $cmdline
    if [ $? ]; then
        echo $1 build succeeded.
        cp "Mapster32.app/Contents/MacOS/mapster32" mapster32.$1
        cp "EDuke32.app/Contents/MacOS/eduke32" eduke32.$1
    else
        echo $1 build failed.
        exit 1
    fi
}

# A little factoring:
commonargs="WITHOUT_GTK=1"
if [ $buildppc == 1 ] || [ `expr $darwinversion = 9` == 1 ]; then
    commonargs="$commonargs DARWIN9=1"
fi
if [ `expr $darwinversion = 10` == 1 ]; then
    commonargs="$commonargs DARWIN10=1"
fi

if [ $doclean == 1 ]; then
    cd build
    rm -f *{.x86,.x64,.ppc}
    cd ..
fi

# Building the buildtools:
if [ $buildtools$installtools != 00 ] && [ -d "build" ]; then

    makecmd="make -k"

    if [ $buildtools == 1 ]; then
        rm -f *{.x86,.x64,.ppc}
        make veryclean
        EXESUFFIX_OVERRIDE=.debug make veryclean

        if [ $build64 == 1 ]; then
            if [ $builddebug == 1 ]; then
                dobuildtools "x86_64 debug" \
                    "ARCH=x86_64 EXESUFFIX_OVERRIDE=.debug.x64 $commonargs RELEASE=0 USE_LIBVPX=1 $makecmd utils"
            fi

            dobuildtools "x86_64 release" \
                "ARCH=x86_64 EXESUFFIX_OVERRIDE=.x64 $commonargs RELEASE=1 USE_LIBVPX=1 $makecmd utils"
        fi

        if [ $build86 == 1 ]; then
            if [ $builddebug == 1 ]; then
                dobuildtools "x86 debug" \
                    "ARCH=i386 EXESUFFIX_OVERRIDE=.debug.x86 $commonargs RELEASE=0 USE_LIBVPX=0 $makecmd utils"
            fi

            dobuildtools "x86 release" \
                "ARCH=i386 EXESUFFIX_OVERRIDE=.x86 $commonargs RELEASE=1 USE_LIBVPX=0 $makecmd utils"
        fi

        if [ $buildppc == 1 ]; then
            if [ $builddebug == 1 ]; then
                dobuildtools "PowerPC debug" \
                    "ARCH=ppc EXESUFFIX_OVERRIDE=.debug.ppc $commonargs RELEASE=0 USE_LIBVPX=0 $makecmd utils"
            fi

            dobuildtools "PowerPC release" \
                "ARCH=ppc EXESUFFIX_OVERRIDE=.ppc $commonargs RELEASE=1 USE_LIBVPX=0 $makecmd utils"
        fi

        mkdir -p tools

        echo buildtools: Creating fat binaries.
        utils=`make printutils && EXESUFFIX_OVERRIDE=.debug make printutils`
        for i in $utils; do
            binaries=
            for j in ${i}.{x86,x64,ppc}; do
                if [ -f "$j" ]; then
                    binaries="$binaries $j"
                fi
            done
            if [ -n "$binaries" ]; then
                lipo -create $binaries -output $i || exit 1
#                ln -f -s $i tools/$i || exit 1
                cp -f $i tools/$i || exit 1
            fi
        done
    fi

    if [ $installtools == 1 ]; then
        if [ -d "/opt/local/bin" ]; then
            echo buildtools: Installing to MacPorts search path.
            for i in $utils; do
                if [ -f "$i" ]; then
                    cp -f "$i" "/opt/local/bin/" || exit 1
                fi
            done
        fi

        if [ -d "/usr/local/bin" ]; then
            echo buildtools: Installing to Homebrew search path.
            for i in $utils; do
                if [ -f "$i" ]; then
                    cp -f "$i" "/usr/local/bin/" || exit 1
                fi
            done
        fi
    fi

    cd ..
fi

if [ $doclean == 1 ] || [ $buildmain == 1 ]; then
    rm -f {eduke32,mapster32}{.debug,}{.x86,.x64,.ppc,}
    rm -rf {$package/,}{EDuke32,Mapster32}{.debug,}.app
fi

# The build process itself:
if [ $buildmain == 1 ]; then
    if [ $iamhelix == 1 ]; then
        makecmd="make -j 2"
    else
        makecmd="make"
    fi

    if [ $build64 == 1 ]; then
        if [ $builddebug == 1 ]; then
            dobuildem debug.x64 "ARCH=x86_64 $commonargs RELEASE=0 $makecmd"
        fi

        if [ $buildrelease == 1 ]; then
            dobuildem x64 "ARCH=x86_64 $commonargs RELEASE=1 $makecmd"
        fi
    fi

    if [ $build86 == 1 ]; then
        if [ $builddebug == 1 ]; then
            dobuildem debug.x86 "ARCH=i386 $commonargs RELEASE=0 USE_LIBPNG=0 USE_LIBVPX=0 $makecmd"
        fi

        dobuildem x86 "ARCH=i386 $commonargs RELEASE=1 USE_LIBPNG=0 USE_LIBVPX=0 $makecmd"
    fi

    if [ $buildppc == 1 ]; then
        if [ $builddebug == 1 ]; then
            dobuildem debug.ppc "ARCH=ppc $commonargs RELEASE=0 USE_LIBPNG=0 USE_LIBVPX=0 $makecmd"
        fi

        dobuildem ppc "ARCH=ppc $commonargs RELEASE=1 USE_LIBPNG=0 USE_LIBVPX=0 $makecmd"
    fi
fi

# Duplicating .app bundles for debug build:
if [ $builddebug == 1 ] || [ $pack == 1 ]; then
    for i in Mapster32 EDuke32; do
        if [ -d "$i.app" ]; then
            cp -RP "$i.app" "$i.debug.app"
        fi
    done
fi

# Begin assembling archive contents:
echo Creating fat binaries.

success=$dummy
if [ $dummy == 0 ]; then
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
            mv -f "$app" "$package/"
        fi
    done
fi

# Almost done...
if [ $success == 1 ]; then
    cd $package

    echo "Generating README.OSX and Changelog.txt"

    # Output README.OSX:
    let "darwinversion -= 4"
    echo "This archive was produced from revision ${VC_REV} by the osxbuild.sh script." > README.OSX
    echo "Built on: Mac OS X 10.$darwinversion" >> README.OSX
    echo "EDuke32 home: http://www.eduke32.com" >> README.OSX
    echo "OS X build discussion on Duke4.net: http://forums.duke4.net/topic/4242-building-eduke-on-mac-os-x/" >> README.OSX
    echo "The 64-bit build in this archive has LibVPX (http://www.webmproject.org/code/)" >> README.OSX
    echo "from MacPorts (http://www.macports.org/) and" >> README.OSX
    echo "LibPNG from Fink (http://www.finkproject.org/) statically linked into it." >> README.OSX

    # Generate Changelog:
    if [ -z $lastrevision ]; then
        lastrevision=$(ls -A1 eduke32-osx* | tail -n1 | cut -d- -f3 | cut -d. -f1)

        if [ -z $lastrevision ]; then
            let lastrevision=VC_REV-1
        elif [ $lastrevision -lt $VC_REV ]; then
            let lastrevision+=1
        else
            let lastrevision=VC_REV-1
        fi
    fi

    echo "Using r$lastrevision as last revision for change log"

    if [ "$vc" == "svn" ]; then
        svn log -r $VC_REV:$lastrevision > Changelog.txt
    elif [ "$vc" == "git" ]; then
        commitid=$(git log --grep="git-svn-id: .*@$lastrevision" -n 1 | grep -E '^commit ' | head -n 1 | awk '{print $2}')
        # Get the commit messages and strip the email addresses
        git log $commitid..HEAD | sed 's/<.*@.*>//g' > Changelog.txt
    fi

    # Package
    if [ $pack == 1 ]; then
        arfilename="eduke32-osx-${VC_REV}.zip"
        echo "Packing distribution into $arfilename"
        rm -f "$arfilename"
        zip -r -y "$arfilename" * -x "*.svn*" "*.git*" "*.dll"
    fi
    cd ..
fi
