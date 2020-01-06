#!/bin/sh

# Check if the test is being run by APR
if [ "$APR_TOPDIR" ]; then
    export PATH="$APR_TOPDIR/package/$APR_PREFIX/bin:$PATH"
    export ADI_HOME="$APR_TOPDIR/package/$APR_PREFIX"
    cd $APR_TOPDIR/test
fi

/apps/ds/bin/dsproc_test

if [ $? != 0 ]; then
    echo "***** FAILED TEST *****"
    exit 1
fi

echo "***** PASSED TEST *****"
exit
