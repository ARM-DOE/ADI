#!/bin/sh

script_name=$0

# Set default idlprefix

idlprefix=$( which idl )
if [ $idlprefix ]; then
    idlprefix=${idlprefix%/bin/idl}
fi

if [ ! $idlprefix ] || [ $idlprefix == "/apps/base/rsi/idl82" ]; then
    idlprefix=/apps/base/idl/idl86
fi

# Print usage

usage()
{
    cat <<EOM

DESCRIPTION

  Build script to configure, build, and install this package.

SYNOPSIS

  $script_name [--idldir=path] [--prefix=path] [--clean]

OPTIONS

  --idlprefix=path  absolute path to the IDL installation directory
                    default: $idlprefix

  --prefix=path     absolute path to the installation directory
                    default: \$ADI_HOME or \$ADI_PREFIX

  --destdir=path    absolute path prepended to prefix 
                    used to perform a staged installation

  --clean           cleanup build directory and regenerate build system files
                    This option should be used after editing build system files
                    or when enabling or disabling debug builds.

  --debug           build debug version using: CFLAGS="-g -O0"
                    to enable debug builds use:  --clean --debug
                    to disable debug builds use: --clean

  --strip           strip debugging symbols
                    This option should be used for production releases.

  --uninstall       uninstall all package files

  -h, --help        display this help message

EOM
}

# Parse command line 

install="install"

for i in "$@"
do
    case $i in
        --clean)       clean=1
                       ;;
        --destdir=*)   destdir="${i#*=}"
                       ;;
        --idlprefix=*) idlprefix="${i#*=}"
                       ;;
        --prefix=*)    prefix="${i#*=}"
                       ;;
        --debug)       debug=1
                       ;;
        --strip)       install="install-strip"
                       ;;
        --uninstall)   install="uninstall"
                       ;;
        -h | --help)   usage
                       exit
                       ;;
        *)             usage
                       exit 1
                       ;;
    esac
done

# Make sure the IDL installation directory exists

if [ ! -d "$idlprefix" ]; then
    echo "IDL installation directory does not exist: $idlprefix"
    echo "Please specify the IDL installation prefix."
    usage
    exit 1
fi

idl_version=$(echo $idlprefix | grep -Eo '[0-9]+$')

# Set default prefix if not specified

if [ ! $prefix ]; then
    if [ $ADI_HOME ]; then
        prefix=$ADI_HOME
    elif [ $ADI_PREFIX ]; then
        prefix=$ADI_PREFIX
    else
        echo "Please specify the installation prefix."
        usage
        exit 1
    fi
fi

# Set libdir

libdir="$prefix/lib64/idl$idl_version"

# Function to echo and run commands

run() {
    echo "> $1"
    $1 || exit 1
}

# Configure, Build, and Install

if [ $clean ] || [ ! -f ./build/Makefile ]; then
echo "----------------------------------------------------------------------"
    if [ $destdir ]; then
        echo "DESTDIR:   $destdir"
    fi
    echo "PREFIX:    $prefix"
    echo "LIBDIR:    $libdir"
    echo "IDLPREFIX: $idlprefix"
    echo ""
fi

echo "----------------------------------------------------------------------"
echo "AUTOGEN"
echo ""

if [ $clean ] || [ ! -f ./configure ]; then
    run "./autogen.sh"
fi

echo "----------------------------------------------------------------------"
echo "CONFIGURE"
echo ""

if [ $clean ] || [ ! -d ./build ]; then
    run "rm -rf build"
    run "mkdir -p build"
fi

run "cd build"

if [ $clean ] || [ ! -f ./Makefile ]; then
    if [ $debug ]; then
        export CFLAGS="-g -O0"
    fi
    export CPPFLAGS="-I$idlprefix/external/include"
    export LDFLAGS="-L$idlprefix/bin/bin.linux.x86_64"
    run "../configure --prefix=$prefix --libdir=$libdir"
fi

echo "----------------------------------------------------------------------"
echo "BUILD"
echo ""

if [ $install != "uninstall" ]; then
    run "make"
fi

echo "----------------------------------------------------------------------"
if [ $install == "uninstall" ]; then
    echo "UNINSTALL"
else
    echo "INSTALL"
fi
echo ""

if [ $destdir ]; then
    run "make DESTDIR=$destdir $install"
else
    run "make $install"
fi

exit
