#! /bin/sh

#
# Version of INSTALL that will work on HPUX and other unpleasant systems
# Copyright (c) 1993 by the University of Southern California
# For copying and distribution information, please see the file
# <usc-copyr.h>.
#
# Author: Steven Seger Augart (swa@isi.edu)


PATH=$PATH:/etc:/usr/etc

sourcefile=         # first argument    
targetfile=          # 2nd argument
owner=              # -o flag
group=              # -g flag
mode=0755           # -m flag

while [ $# -gt 0 ]; do
    if [ $1 = '-o' ]; then
        shift
        owner=$1
        shift
        continue
    fi
    # old flag meaning 'copy'; ignore it.
    if [ $1 = '-c' ]; then
        shift
        continue
    fi
    if [ $1 = '-g' ]; then
        shift
        group=$1
        shift
        continue
    fi
    if [ $1 = -m ] ; then
        shift   
        mode=$1
        shift
        continue
    fi
    if [ -z "$sourcefile" ]; then
        sourcefile=$1
        shift
    elif [ -z "$targetfile" ]; then
        targetfile=$1
        shift
    else
        echo Install: too many arguments or argument not understood
        exit 1
    fi
done
set -e # exit on error
if [ -d $targetfile ]; then
    echo Target of install must be a file name.
    exit 1
fi
cp $sourcefile $targetfile
if [ -n "$owner" ]; then
    chown $owner $targetfile
fi
if [ -n "$group" ]; then
    chgrp $group $targetfile
fi
if [ -n "$mode" ]; then
    chmod $mode $targetfile
fi
