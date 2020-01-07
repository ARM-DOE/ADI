#!/bin/sh

# Print usage

script_name=$0

usage()
{
    cat <<EOM

DESCRIPTION

  Build script to configure, make, and install this package.

SYNOPSIS

  $script_name [--prefix=path] [--clean] [--conf] [--debug]

OPTIONS

  --prefix=path   absolute path to installation directory
                  default: \$ADI_HOME or \$ADI_PREFIX

  --destdir=path  absolute path prepended to prefix 
                  used to perform a staged installation

  --clean         cleanup build directory and regenerate build system files
                  This option should be used after editing build system files
                  or when enabling or disabling debug builds.

  --conf          only install conf package

  --debug         build debug version using: CFLAGS="-g -O0"

  --purify        build with purify

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
        --conf)       conf_only=1
                      ;;
        --debug)      debug=1
                      ;;
        --destdir=*)  destdir="${i#*=}"
                      ;;
        --prefix=*)   prefix="${i#*=}"
                      ;;
        --purify)     purify="true"
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

# Function to echo and run commands

run() {
    echo "> $1"
    $1 || exit 1
}

# Function to uninstall this package

uninstall() {
echo "--------------------------------------------------------------------"
echo "UNINSTALL"
echo ""
    # Prevent aclocal, autoconf, and automake from being run
    export ACLOCAL=":"
    export AUTOCONF=":"
    export AUTOMAKE=":"

    if [ $conf_only ]; then
        if [ -f ./build/conf/Makefile ]; then 
            run "cd build/conf"
            run "make -e uninstall"
            run "cd ../.."
        fi
    else
        run "cd build"
        run "make -e uninstall"
        run "cd .."
    fi

    unset ACLOCAL
    unset AUTOCONF
    unset AUTOMAKE
}

# Uninstall previous build

if [ -f ./build/Makefile ]; then
    uninstall
    if [ "$install" == "uninstall" ]; then
        exit
    fi
fi

# Configure, Make, and Install

if [ $clean ] || [ ! -f ./build/Makefile ]; then
echo "----------------------------------------------------------------------"
    if [ $destdir ]; then
        echo "DESTDIR: $destdir"
    fi
    echo "PREFIX:  $prefix"
    echo ""
fi

echo "--------------------------------------------------------------------"
echo "AUTOGEN"
echo ""

if [ $clean ] || [ ! -f ./configure ]; then
    run "./autogen.sh"
fi

echo "--------------------------------------------------------------------"
echo "CONFIGURE"
echo ""

if [ $clean ] || [ ! -d ./build ]; then
    run "rm -rf build"
    run "mkdir -p build"
fi

run "cd build"

if [ $clean ] || [ ! -f ./Makefile ]; then
    run "../configure --prefix=$prefix"
fi

if [ "$install" == "uninstall" ]; then
    uninstall
    exit
fi

echo "--------------------------------------------------------------------"
echo "BUILD"
echo ""

if [ $conf_only ]; then
    if [ -f ./conf/Makefile ]; then
        run "cd conf"
    else
        echo "No conf package files found to install."
        exit
    fi
fi

if [ $debug ] || [ $purify ]; then

    if [ $debug ]; then
        export CFLAGS="-g -O0"
        export FFLAGS="-g -O0"
    fi

    if [ $purify ]; then
        export CC="purify -cache-dir=$HOME/.purecache -windows=no gcc"
        export F77="purify -cache-dir=$HOME/.purecache -windows=no gfortran"
    fi

    install="-e $install"
    run "make clean"
    run "make -e"
else
    run "make"
fi

echo "--------------------------------------------------------------------"
echo "INSTALL"
echo ""

if [ $destdir ]; then
    run "make DESTDIR=$destdir $install"
else
    run "make $install"
fi

exit
