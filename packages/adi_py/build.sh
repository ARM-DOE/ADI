#!/bin/sh

script_name=$0

# Set default pyprefix

pyprefix=$( which python3 )
if [ ! $pyprefix ]; then
    pyprefix="/apps/base/python3"
else
    pyprefix=${pyprefix%/bin/*}
fi

if [ ! $pyprefix ] || [ $pyprefix == "/apps/base/python3" ]; then
    pyprefix=/apps/base/python3.6
fi

# Print usage

usage()
{
    cat <<EOM

DESCRIPTION

  Build script used to install this package.

SYNOPSIS

  $script_name [--pyprefix] [--prefix=path]

OPTIONS

  --pyprefix=path   installation prefix of the instance of python to use.
                    default: $pyprefix

  --prefix=path     absolute path to the installation directory
                    default if destdir not specified: \$HOME/.local
                    default if destdir is specified:  \$pyprefix

  --destdir=path    absolute path to prepended to the prefix,
                    used to perform a staged installation

  --uninstall       uninstall all package files

  -h, --help        display this help message

EOM
}

# Parse command line

for i in "$@"
do
    case $i in
        --destdir=*)      destdir="${i#*=}"
                          ;;
        --prefix=*)       prefix="${i#*=}"
                          ;;
        --pyprefix=*)     pyprefix="${i#*=}"
                          ;;
        --uninstall)      uninstall=1
                          ;;
        -h | --help)      usage
                          exit 0
                          ;;
        *)                usage
                          exit 1
                          ;;
    esac
done

# Make sure the Python installation directory exists

if [ ! -d "$pyprefix" ]; then
    echo "Python installation directory does not exist: $pyprefix"
    echo "Please specify the Python installation prefix."
    usage
    exit 1
fi

# Get Python version

if [ -e $pyprefix/bin/python3 ]; then
    python="$pyprefix/bin/python3"
else
    python="$pyprefix/bin/python"
fi

if [ -e $pyprefix/bin/pip3 ]; then
    pip="$pyprefix/bin/pip3"
else
    pip="$pyprefix/bin/pip"
fi

version=`$python -c 'import sys; print("%s%s" % sys.version_info[:2])'`
package="adi_py$version"

# Functions to echo and run commands

run() {
    echo "> $1"
    $1 || exit 1
}

# Install Python Package

echo "------------------------------------------------------------------"
if [ $destdir ]; then
    echo "DESTDIR:   $destdir"
fi

if [ $prefix ]; then
    echo "PREFIX   = $prefix"
elif [ $destdir ]; then
    echo "PREFIX   = $pyprefix"
else
    echo "PREFIX   = $HOME/.local"
fi

echo "PYPREFIX = $pyprefix"
echo "VERSION  = $version"
echo "PACKAGE  = $package"

echo "----------------------------------------------------------------------"
if [ $uninstall ]; then
    echo "UNINSTALL"
else
    echo "INSTALL"
fi
echo ""

if [ ! $prefix ] && [ ! $destdir ]; then
    # local install/uninstall
    if [ $uninstall ]; then
        run "$pip uninstall $package"
    else
        run "$pip install . --user --upgrade"
    fi
else

    install_args='--upgrade --ignore-installed --no-deps'
    install_root=''

    if [ $destdir ]; then
        install_args+=" --root $destdir" 
        install_root+=$destdir;
    fi

    if [ $prefix ]; then
        install_args+=" --prefix $prefix" 
        install_root+=$prefix;
    else
        install_root+=$pyprefix;
    fi

    version=`$python -c 'import sys; print("%s.%s" % sys.version_info[:2])'`

    export PYTHONPATH="$install_root/lib/python$version/site-packages"

    if [ $uninstall ]; then
        run "$pip uninstall $package"
    else
        run "$pip install . $install_args"
    fi
fi

exit
