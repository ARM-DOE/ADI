#!/bin/sh

# Print usage

script_name=$0

usage()
{
    cat <<EOM

DESCRIPTION

  Build script to install perl scripts.

SYNOPSIS

  $script_name [--prefix=path [--destdir=path]]

OPTIONS

  --prefix=path   absolute path to installation directory
                  default: \$DSDB_HOME

  --destdir=path  absolute path prepended to prefix 
                  used to perform a staged installation

  --uninstall     uninstall locally installed package

  -h, --help      display this help message

EOM
}

# Parse command line 

for i in "$@"
do
    case $i in
        --destdir=*)  destdir="${i#*=}"
                      ;;
        --prefix=*)   prefix="${i#*=}"
                      ;;
        --uninstall)  uninstall=true
                      ;;
        -h | --help)  usage
                      exit
                      ;;
        *)            usage
                      exit 1
                      ;;
    esac
done

if [ $destdir ] && [ ! $prefix ]; then
   usage
   exit 1
fi

# Get prefix from environemnt variables if necessary

if [ ! $prefix ]; then
    if [ $DSDB_HOME ]; then
        prefix=$DSDB_HOME
    else
        usage
        exit 1
    fi
fi

# Get version 

version=`./get_version.sh`

echo "----------------------------------------------------------------------"

if [ $destdir ]; then
    echo "destdir: $destdir"
fi

echo "prefix:  $prefix"
echo "version: $version"

# Function to echo and run commands

run() {
    echo "> $1"
    $1 || exit 1
}

# Install PERL modules

if [ -d lib ]; then
    echo "------------------------------------------------------------------"
    libdir="$destdir$prefix/lib"
    echo "libdir:  $libdir"

    cd lib
    if [ $uninstall ]; then
        for f in *pm; do
            rm -f $libdir/$f
            echo "uninstalled: $libdir/$f"
        done
    else
        mkdir -p $libdir
        for f in *pm; do
            run "perl -c $f"
            sed -e "s,@VERSION@,$version,g" < $f > $libdir/$f
            chmod 0644 $libdir/$f
            echo "installed: $libdir/$f"
        done
    fi
    cd ..
fi

# Install PERL scripts

if [ -d bin ]; then
    echo "------------------------------------------------------------------"
    bindir="$destdir$prefix/bin"
    echo "bindir:  $bindir"

    cd bin
    if [ $uninstall ]; then
        for f in *pl; do
            outfile=${f%.pl}
            rm -f $bindir/$outfile
            echo "uninstalled: $bindir/$outfile"
        done
    else
        mkdir -p $bindir
        for f in *pl; do
            run "perl -c $f"
            outfile=${f%.pl}
            sed -e "s,@VERSION@,$version,g" < $f > $bindir/$outfile
            chmod 0755 $bindir/$outfile
            echo "installed: $bindir/$outfile"
        done
    fi
    cd ..
fi

# Install DSDB defs files

if [ -d conf/dbog ]; then
    echo "------------------------------------------------------------------"
    confdir="$destdir$prefix/conf/dbog"
    echo "confdir: $confdir"

    cd conf/dbog
    if [ $uninstall ]; then
        for f in *defs; do
            rm -f $confdir/$f
            echo "uninstalled: $confdir/$f"
        done
    else
        mkdir -p $confdir
        for f in *defs; do
            cp $f $confdir/$f
            chmod 0644 $confdir/$f
            echo "installed: $confdir/$f"
        done
    fi
    cd ../..
fi

exit 0
