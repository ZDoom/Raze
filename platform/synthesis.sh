#!/bin/bash

# The extension for executables.
exe=.exe

# some paths
top=/var/www/synthesis
lockfile=$top/synthesis_building
source=$top/eduke32
output=/var/www/dukeworld.duke4.net/eduke32/synthesis
make=( make SYNTHESIS=1 PLATFORM=WINDOWS CROSS='i686-w64-mingw32-' CC='i686-w64-mingw32-gcc-5.4-win32' CXX='i686-w64-mingw32-g++-5.4-win32' AS='nasm' PRETTY_OUTPUT=0 SDLCONFIG='' )
make64=( make SYNTHESIS=1 PLATFORM=WINDOWS CROSS='x86_64-w64-mingw32-' CC='x86_64-w64-mingw32-gcc-5.4-win32' CXX='x86_64-w64-mingw32-g++-5.4-win32' AS='nasm' PRETTY_OUTPUT=0 SDLCONFIG='' )
clean=veryclean

# the following file paths are relative to $source  
targets=( eduke32$exe mapster32$exe )
package=$top/package
not_src_packaged=( package/debug/win32/ebacktrace1.dll package/debug/win64/ebacktrace1-64.dll )

# group that owns the resulting packages
group=dukeworld

# some variables
dobuild=

# controls resulting package filenames... will support linux later
basename=eduke32

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

cd $source

# update the code repository and get the last revision number from SVN
head=`svn update | tail -n1 | awk '{ print $NF }' | cut -d. -f1`
echo "HEAD is revision $head."
headlog=`svn log -r $head`

lastrevision=`ls -A1 $output/????????-???? | grep ${basename}_ - | tail -n1 | cut -d- -f2 | cut -d. -f1`

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

    # create the output directory
    mkdir -p $output/$date-$head
}

function package_start ()
{
    mkdir -p $package

    cd $package

    cp -R $source/package/common/* ./
    cp -R $source/package/sdk/* ./
}

function package_debug ()
{
    cp -R $source/package/debug/${*}/* ./
}

function package_game_lunatic ()
{
    # Package some Lunatic test and demo files.
    mkdir -p ./lunatic/test
    cp $source/source/duke3d/src/lunatic/test.lua ./lunatic/
    cp $source/source/duke3d/src/lunatic/test/test_{bitar,geom,rotspr}.lua ./lunatic/test/
    cp $source/source/duke3d/src/lunatic/test/{damagehplane,delmusicsfx,helixspawner,shadexfog}.lua ./lunatic/test/
}

function package_execute ()
{
    echo 7zr a -mx9 -ms=on -t7z $output/$date-$head/${*}_$date-$head.7z *
    7zr a -mx9 -ms=on -t7z $output/$date-$head/${*}_$date-$head.7z *

    cd $source

    rm -rf $package
}

if [ $dobuild ]
then
    echo "Launching a build..."

    cd $source

    # get the date in the YYYYMMDD format (ex: 20091001)
    date=`date +%Y%m%d`


    # 32-bit debug

    # clean the tree and build
    echo "${make[@]}" RELEASE=0 $clean all
    "${make[@]}" RELEASE=0 $clean all

    # make sure all the targets were produced
    verifytargets

    # package
    package_start
    package_debug win32
    mv -f $source/eduke32$exe "$package/eduke32.debug$exe"
    mv -f $source/mapster32$exe "$package/mapster32.debug$exe"
    package_execute ${basename}_win32_debug


    # 64-bit debug

    # clean the tree and build
    echo "${make64[@]}" RELEASE=0 $clean all
    "${make64[@]}" RELEASE=0 $clean all

    # make sure all the targets were produced
    verifytargets

    # package
    package_start
    package_debug win64
    mv -f $source/eduke32$exe "$package/eduke32.debug$exe"
    mv -f $source/mapster32$exe "$package/mapster32.debug$exe"
    package_execute ${basename}_win64_debug


    # 32-bit Lunatic (pre-)release

    BUILD_LUNATIC="`echo $headlog | grep BUILD_LUNATIC`"

    if [ -n "$BUILD_LUNATIC" ]; then
        # clean the tree and build
        echo "${make[@]}" LUNATIC=1 $clean all
        "${make[@]}" LUNATIC=1 $clean all

        # make sure all the targets were produced
        verifytargets

        # package
        package_start
        package_game_lunatic
        mv -f $source/eduke32$exe "$package/leduke32_PREVIEW$exe"
        mv -f $source/mapster32$exe "$package/lmapster32_PREVIEW$exe"
        package_execute l${basename}_lunatic_PREVIEW_win32
    fi


    # 64-bit Lunatic (pre-)release

    if [ -n "$BUILD_LUNATIC" ]; then
        # clean the tree and build
        echo "${make64[@]}" LUNATIC=1 $clean all
        "${make64[@]}" LUNATIC=1 $clean all

        # make sure all the targets were produced
        verifytargets

        # package
        package_start
        package_game_lunatic
        mv -f $source/eduke32$exe "$package/leduke32_PREVIEW$exe"
        mv -f $source/mapster32$exe "$package/lmapster32_PREVIEW$exe"
        package_execute l${basename}_lunatic_PREVIEW_win64
    fi


    # 32-bit release

    # clean the tree and build
    echo "${make[@]}" $clean all
    "${make[@]}" $clean all

    # make sure all the targets were produced
    verifytargets

    # package
    package_start
    mv -f $source/eduke32$exe "$package/eduke32$exe"
    mv -f $source/mapster32$exe "$package/mapster32$exe"
    package_execute ${basename}_win32


    # 64-bit release

    # clean the tree and build
    echo "${make64[@]}" $clean all
    "${make64[@]}" $clean all

    # make sure all the targets were produced
    verifytargets

    # package
    package_start
    mv -f $source/eduke32$exe "$package/eduke32$exe"
    mv -f $source/mapster32$exe "$package/mapster32$exe"
    package_execute ${basename}_win64


    # clean up

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

    # throw the svn revision into a file the Makefile will read
    echo "$head" > ${basename}_$date-$head/EDUKE32_REVISION

    echo tar cJf ${basename}_src_$date-$head.tar.xz ${basename}_$date-$head
    tar cJf ${basename}_src_$date-$head.tar.xz ${basename}_$date-$head
    rm -r ${basename}_$date-$head

    cd $source

    # output the changelog since last snapshot in the output directory
    if [ $lastrevision ]
    then
        # add one so that we only include what is new to this update
        let lastrevision+=1
        svn log -r $head:$lastrevision > $output/$date-$head/ChangeLog.txt

        echo "See http://svn.eduke32.com/listing.php?repname=eduke32 for more details." >> $output/$date-$head/ChangeLog.txt
    fi

    # hack for our served directory structure... really belongs elsewhere,
    # like in whatever script executes this one
    chmod -R g+w $output/$date-$head
    chown -R :$group $output/$date-$head

    # link eduke32_latest.zip to the new archive
    ln -sf $output/$date-$head/${basename}_win32_$date-$head.7z $output/eduke32_latest.7z

    rm -f $lockfile
else
    echo "Nothing to do."
    rm -f $lockfile
fi
