#!/bin/bash

# some paths
top=/var/www/synthesis/eduke32
lockfile=$top/synthesis_building
source=eduke32
output=/var/www/dukeworld.duke4.net/eduke32/synthesis
make=( make PLATFORM=WINDOWS CC='wine gcc' CXX='wine g++' AS='wine nasm' RC='wine windres' STRIP='wine strip' AR='wine ar' RANLIB='wine ranlib' PRETTY_OUTPUT=0 NEDMALLOC=0 )
clean=veryclean

# the following file paths are relative to $source
targets=( eduke32.exe mapster32.exe )
package=package
not_src_packaged=( psd $package/ebacktrace1.dll $package/ebacktrace1-64.dll )

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

lastrevision=`ls -A1 $output/????????-???? | tail -n1 | cut -d- -f2 | cut -d. -f1`

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

if [ $dobuild ]
then
    echo "Launching a build..."

    cd $top/$source

    # throw the svn revision into a header.  this is ugly.
    echo "s_buildRev = \"r$head\";" > source/rev.h

    # clean the tree and build debug first
    echo "${make[@]}" RELEASE=0 $clean all
    "${make[@]}" RELEASE=0 $clean all

    # make sure all the targets were produced
    for i in "${targets[@]}"; do
        if [ ! -e $i ]
        then
            echo "Build failed! Bailing out..."
        	rm -r $lockfile
            exit
        fi
    done

    # move the targets to $package
    echo mv -f eduke32.exe "$package/eduke32.debug.exe"
    mv -f eduke32.exe "$package/eduke32.debug.exe"
    echo mv -f mapster32.exe "$package/mapster32.debug.exe"
    mv -f mapster32.exe "$package/mapster32.debug.exe"

    # clean the tree and build release
    echo "${make[@]}" $clean all
    "${make[@]}" $clean all

    # make sure all the targets were produced
    for i in "${targets[@]}"; do
        if [ ! -e $i ]
        then
            echo "Build failed! Bailing out..."
        	rm -r $lockfile
            exit
        fi
    done

    # move the targets to $package
    echo mv -f eduke32.exe "$package/eduke32.exe"
    mv -f eduke32.exe "$package/eduke32.exe"
    echo mv -f mapster32.exe "$package/mapster32.exe"
    mv -f mapster32.exe "$package/mapster32.exe"

    # get the date in the YYYYMMDD format (ex: 20091001)
    date=`date +%Y%m%d`
    
    # create the output directory
    mkdir $output/$date-$head
    
    # package the binary snapshot
    cd $package
    echo zip -r -y -9 $output/$date-$head/${basename}_${platform}_$date-$head.zip * -x "*.svn*"
    zip -r -y -9 $output/$date-$head/${basename}_${platform}_$date-$head.zip * -x "*.svn*"
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
    
    echo tar cjf ${basename}_src_$date-$head.tar.bz2 ${basename}_$date-$head
    tar cjf ${basename}_src_$date-$head.tar.bz2 ${basename}_$date-$head
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
    ln -sf $output/$date-$head/${basename}_${platform}_$date-$head.zip $output/eduke32_latest.zip

    rm -r $lockfile
else
    echo "Nothing to do."
    rm -r $lockfile
fi
