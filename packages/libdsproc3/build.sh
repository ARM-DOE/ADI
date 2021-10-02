#!/bin/sh

# Print usage

script_name=$0

# Set libdir

UNAME=`uname`
if [ "$UNAME" == "Darwin" ]; then
    default_libdir="lib"
else
    default_libdir="lib64"
fi

usage()
{
    cat <<EOM

DESCRIPTION

  Build script to configure, build, and install this package.

SYNOPSIS

  $script_name [--prefix=path] [--clean]

OPTIONS

  --prefix=path   absolute path to the installation directory
                  default: \$ADI_HOME or \$ADI_PREFIX

  --libdir=path   absolute path to the library installation directory
                  default: \$prefix/$default_libdir

  --destdir=path  absolute path prepended to prefix 
                  used to perform a staged installation

  --clean         cleanup build directory and regenerate build system files
                  This option should be used after editing build system files
                  or when enabling or disabling debug builds.

  --debug         build debug version using: CFLAGS="-g -O0"
                  to enable debug builds use:  --clean --debug
                  to disable debug builds use: --clean

  --strip         strip debugging symbols
                  This option should be used for production releases.

  --uninstall     uninstall all package files

  -h, --help      display this help message

EOM
}

# Parse command line 

install="install"

for i in "$@"
do
    case $i in
        --clean)      clean=1
                      ;;
        --destdir=*)  destdir="${i#*=}"
                      ;;
        --libdir=*)   libdir="${i#*=}"
                      ;;
        --prefix=*)   prefix="${i#*=}"
                      ;;
        --debug)      debug=1
                      ;;
        --strip)      install="install-strip"
                      ;;
        --uninstall)  install="uninstall"
                      ;;
        -h | --help)  usage
                      exit
                      ;;
        *)            usage
                      exit 1
                      ;;
    esac
done

# Set default prefix and libdir if not specified

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

if [ ! $libdir ]; then
    libdir="$prefix/$default_libdir"
fi

# Function to echo and run commands

run() {
    echo "> $1"
    $1 || exit 1
}

# Configure, Build, and Install

if [ $clean ] || [ ! -f ./build/Makefile ]; then
echo "----------------------------------------------------------------------"
    if [ $destdir ]; then
        echo "DESTDIR: $destdir"
    fi
    echo "PREFIX:  $prefix"
    echo "LIBDIR:  $libdir"
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
