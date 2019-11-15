#!/bin/sh
kernel=`uname`
rel=$kernel

if [ "$kernel" == "Linux" ]; then
    release=`uname -r`
    if [[ $release = *".el"* ]]; then
        rel=`echo $release | sed 's/.*\(el[0-9]\).*/\1/'`
    fi
fi

printf "2.5-1.$rel"
exit 0
