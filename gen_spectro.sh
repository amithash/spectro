#!/bin/bash

NUMCPU=4

find . -type f -regextype posix-awk -iregex '.*\.(mp3|ogg|flac)' | while read i ; do

        if [ `jobs -p | wc -l` -ge $NUMCPU ] ; then
               wait
       fi

       TEMP="${i%.*}.spect"
       OUTF=`echo "$TEMP" | sed 's#\(.*\)/\([^,]*\)#\1/.\2#'`
       if [ ! -e "$OUTF" ] ; then
               spectgen -o "$OUTF" "$i" &
       fi
done

