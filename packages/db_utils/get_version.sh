#!/bin/sh
kernel=`uname`
rel=$kernel

if [ "$kernel" == "Linux" ]; then
    release=`uname -r`
    if [[ $release = *".el"* ]]; then
        rel=`echo $release | sed 's/.*\(el[0-9]\).*/\1/'`
    fi
fi

printf "db_utils-v2.3-1.$rel"
exit 0
