#!/bin/bash

# The extension for executables.
exe=.exe

# some paths
top=/var/www/synthesis/eduke32
lockfile=$top/synthesis_building
source=eduke32
output=/var/www/dukeworld.duke4.net/eduke32/synthesis
make=( make SYNTHESIS=1 PLATFORM=WINDOWS CROSS='i686-w64-mingw32-' CC='i686-w64-mingw32-gcc-4.8.0' AS='nasm' PRETTY_OUTPUT=0 )
make64=( make SYNTHESIS=1 PLATFORM=WINDOWS CROSS='x86_64-w64-mingw32-' CC='x86_64-w64-mingw32-gcc-4.8.0' AS='nasm' PRETTY_OUTPUT=0 )
clean=veryclean

# the following file paths are relative to $source
targets=( eduke32$exe mapster32$exe )
package=package
not_src_packaged=( $package/ebacktrace1.dll $package/ebacktrace1-64.dll )

# group that owns the resulting packages
group=dukeworld

# some variables
dobuild=

# controls resulting package filenames... will support linux later
basename=eduke32
platform=win32

# if the output dir doesn't exist, create it
if [ ! -e $output ]
then
    mkdir -p $output
fi

if [ -f $lockfile ]
then
    echo "Build already in progress!"
    exit
else
    touch $lockfile
fi

cd $top

# update the code repository and get the last revision number from SVN
head=`svn update | tail -n1 | awk '{ print $NF }' | cut -d. -f1`
echo "HEAD is revision $head."
headlog=`svn log -r $head`

lastrevision=`ls -A1 $output/????????-???? | tail -n1 | cut -d- -f2 | cut -d. -f1`

# If the log of HEAD contains DONT_BUILD, obey.
if [ -n "`echo $headlog | grep DONT_BUILD`" ]; then
    echo "HEAD requested to not build. Bailing out..."
    rm -f $lockfile
    exit
fi

# if the output dir is empty, we build no matter what
if [ ! $lastrevision ]
then
    echo "No builds yet."
    dobuild=1
else
    echo "Last built revision is $lastrevision."
    
    # if the last built revision is less than HEAD, we also build
    if [ $lastrevision -lt $head ]
    then
        echo "Need a new build."
        dobuild=1
    fi
fi

function verifytargets ()
{
    for i in "${targets[@]}"; do
        if [ ! -e $i ]
        then
            echo "Build failed! Bailing out..."
            rm -f $lockfile
            exit
        fi
    done
}

if [ $dobuild ]
then
    echo "Launching a build..."

    cd $top/$source

    # throw the svn revision into a header.  this is ugly.
    echo "s_buildRev = \"r$head\";" > source/rev.h


    # 32-bit debug

    # clean the tree and build
    echo "${make[@]}" RELEASE=0 $clean all
    "${make[@]}" RELEASE=0 $clean all

    # make sure all the targets were produced
    verifytargets

    # move the targets to $package
    echo mv -f eduke32$exe "$package/eduke32.debug$exe"
    mv -f eduke32$exe "$package/eduke32.debug$exe"
    echo mv -f mapster32$exe "$package/mapster32.debug$exe"
    mv -f mapster32$exe "$package/mapster32.debug$exe"


    # 64-bit debug

    # clean the tree and build
    echo "${make64[@]}" RELEASE=0 $clean all
    "${make64[@]}" RELEASE=0 $clean all

    # make sure all the targets were produced
    verifytargets

    # move the targets to $package
    echo mv -f eduke32$exe "$package/eduke64.debug$exe"
    mv -f eduke32$exe "$package/eduke64.debug$exe"
    echo mv -f mapster32$exe "$package/mapster64.debug$exe"
    mv -f mapster32$exe "$package/mapster64.debug$exe"


    # 32-bit Lunatic (pre-)release

    BUILD_LUNATIC="`echo $headlog | grep BUILD_LUNATIC`"

    if [ -n "$BUILD_LUNATIC" ]; then
        # clean the tree and build
        echo "${make[@]}" LUNATIC=1 $clean all
        "${make[@]}" LUNATIC=1 $clean all

        # make sure all the targets were produced
        verifytargets

        # move the targets to $package
        echo mv -f eduke32$exe "$package/leduke32_PREVIEW$exe"
        mv -f eduke32$exe "$package/leduke32_PREVIEW$exe"
    fi


    # 32-bit release

    # clean the tree and build
    echo "${make[@]}" $clean all
    "${make[@]}" $clean all

    # make sure all the targets were produced
    verifytargets

    # move the targets to $package
    echo mv -f eduke32$exe "$package/eduke32$exe"
    mv -f eduke32$exe "$package/eduke32$exe"
    echo mv -f mapster32$exe "$package/mapster32$exe"
    mv -f mapster32$exe "$package/mapster32$exe"


    # 64-bit release

    # clean the tree and build
    echo "${make64[@]}" $clean all
    "${make64[@]}" $clean all

    # make sure all the targets were produced
    verifytargets

    # move the targets to $package
    echo mv -f eduke32$exe "$package/eduke64$exe"
    mv -f eduke32$exe "$package/eduke64$exe"
    echo mv -f mapster32$exe "$package/mapster64$exe"
    mv -f mapster32$exe "$package/mapster64$exe"



    # get the date in the YYYYMMDD format (ex: 20091001)
    date=`date +%Y%m%d`
    
    # create the output directory
    mkdir $output/$date-$head

    # package the binary snapshot
    cd $package

    # Package some Lunatic test and demo files.
    if [ -n "$BUILD_LUNATIC" ]; then
        mkdir -p lunatic/test
        cp $top/$source/source/lunatic/test.lua lunatic
        cp $top/$source/source/lunatic/test/test_{bitar,geom,rotspr}.lua lunatic/test
        cp $top/$source/source/lunatic/test/{delmusicsfx,helixspawner}.lua lunatic/test
    fi

    echo 7z a -mx9 -t7z $output/$date-$head/${basename}_${platform}_$date-$head.7z *
    7z a -mx9 -t7z $output/$date-$head/${basename}_${platform}_$date-$head.7z *

    # Remove the packaged Lunatic test/demo files.
    if [ -n "$BUILD_LUNATIC" ]; then
        rm lunatic/test/*.lua lunatic/test.lua
        rmdir lunatic/test
        rmdir lunatic
    fi

    cd $top/$source

    # hack to restore [e]obj/keep.me
    echo svn update -r $head
    svn update -r $head
    
    # export the source tree into the output directory
    svn export . $output/$date-$head/${basename}_$date-$head
    echo svn export . $output/$date-$head/${basename}_$date-$head
    
    # package the source
    cd $output/$date-$head
    
    # first remove the unnecessary files
    for i in "${not_src_packaged[@]}"; do
        echo rm -r ${basename}_$date-$head/$i
        rm -r ${basename}_$date-$head/$i
    done
    
    echo tar cJf ${basename}_src_$date-$head.tar.xz ${basename}_$date-$head
    tar cJf ${basename}_src_$date-$head.tar.xz ${basename}_$date-$head
    rm -r ${basename}_$date-$head

    # clean up the revision header
    cd $top/$source
    echo svn revert "source/rev.h"
    svn revert "source/rev.h"
    
    # output the changelog since last snapshot in the output directory
    if [ $lastrevision ]
    then
        # add one so that we only include what is new to this update
        let lastrevision+=1
        svn log -r $head:$lastrevision > $output/$date-$head/ChangeLog.txt
    fi
    
    # hack for our served directory structure... really belongs elsewhere,
    # like in whatever script executes this one
    chmod -R g+w $output/$date-$head
    chown -R :$group $output/$date-$head

   # link eduke32_latest.zip to the new archive
    ln -sf $output/$date-$head/${basename}_${platform}_$date-$head.7z $output/eduke32_latest.7z

    rm -f $lockfile
else
    echo "Nothing to do."
    rm -f $lockfile
fi
