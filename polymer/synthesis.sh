#!/bin/bash

# some paths
top=/home/plagman/src/eduke32
source=polymer/eduke32
output=/var/www/dukeworld.duke4.net/eduke32/synthesis
make=( make PLATFORM=WINDOWS CC='wine gcc' CXX='wine g++' AS='wine nasm' RC='wine windres' STRIP='wine strip' AR='wine ar' RANLIB='wine ranlib' PRETTY_OUTPUT=0 )
clean=veryclean

# the following file paths are relative to $source
targets=( eduke32.exe mapster32.exe )
bin_packaged=( eduke32.exe mapster32.exe names.h tiles.cfg buildlic.txt GNU.TXT m32help.hlp ror.map a.m32 )
not_src_packaged=( rsrc/game2.psd rsrc/game3.psd source/jaudiolib/third-party/vorbis.framework/Versions/A/vorbis Apple )

# group that owns the resulting packages
group=dukeworld

# some variables
dobuild=
buildfailed=

# if the output dir doesn't exist, create it
if [ ! -e $output ]
then
    mkdir -p $output
fi

# update the code repository and get the last revision number from SVN
cd $top
head=`svn update | tail -n1 | awk '{ print $NF }' | cut -d. -f1`
echo "HEAD is revision $head."

lastrevision=`ls -A1 $output | tail -n1 | cut -d- -f2`

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
    # clean the tree
    echo "${make[@]}" $clean
    "${make[@]}" $clean
    # build
    echo "${make[@]}"
    "${make[@]}"
    # make sure all the targets were produced
    for i in "${targets[@]}"; do
        if [ ! -e $i ]
        then
            buildfailed=1
        fi
    done
    # bail out if the build is failed
    if [ $buildfailed ]
    then
        echo "Build failed! Bailing out..."
        exit
    fi
    # get the date in the YYYYMMDD format (ex: 20091001)
    date=`date +%Y%m%d`
    # create the output directory
    mkdir $output/$date-$head
    # package the binary snapshot
    echo zip -9 $output/$date-$head/eduke32_win32_$date-$head.zip ${bin_packaged[@]}
    zip -9 $output/$date-$head/eduke32_win32_$date-$head.zip ${bin_packaged[@]}
    # hack to restore [e]obj/keep.me
    echo svn update -r $head
    svn update -r $head
    # export the source tree into the output directory
    svn export . $output/$date-$head/eduke32_$date-$head
    echo svn export . $output/$date-$head/eduke32_$date-$head
    # package the source
    cd $output/$date-$head
    # first remove the unnecessary files
    for i in "${not_src_packaged[@]}"; do
        echo rm -r eduke32_$date-$head/$i
        rm -r eduke32_$date-$head/$i
    done
    echo tar cvzf eduke32_src_$date-$head.tar.gz eduke32_$date-$head
    tar cvzf eduke32_src_$date-$head.tar.gz eduke32_$date-$head
    rm -r eduke32_$date-$head
    # output the changelog since last snapshot in the output directory
    if [  $lastrevision ]
    then
        cd $top/$source
        svn log -r $head:$lastrevision > $output/$date-$head/ChangeLog.txt
    fi
    # hack for our served directory structure
    chmod -R g+w $output/$date-$head
    chown -R :$group $output/$date-$head
else
    echo "Nothing to do."
fi
