#!/bin/sh

# List of packages to install

pkgs=( \
    libmsngr \
    libarmutils \
    libcds3 \
    libncds3 \
    libdbconn \
    libtrans \
    libdsdb3 \
    db_utils \
    libdsproc3 \
    adi_idl \
    adi_py \
)

# Default installation prefix

prefix="/usr/local"

# Set default pyprefix

pyprefix=$( which python3 )
if [ $pyprefix ]; then
    pyprefix=${pyprefix%/bin/*}
    if [ "$pyprefix" = "/apps/base/python3" ]; then
        pyprefix=/apps/base/python3.6
    fi
else
    pyprefix="python3 not found"
    echo "python3 not found - skipping installation of python bindings"
fi

# Set default idlprefix

idlprefix=$( which idl )
if [ $idlprefix ]; then
    idlprefix=${idlprefix%/bin/idl}
    if [ "$idlprefix" = "/apps/base/rsi/idl82" ]; then
        idlprefix=/apps/base/idl/idl86
    fi
else
    idlprefix="IDL not found"
    echo "IDL not found - skipping installation of IDL bindings"
fi

# Print usage

script_name=$0

usage()
{
    cat <<EOM

DESCRIPTION

    Build and install all packages in the ARM-DOE/ADI GitHub repository.

    Each package will be built and installed into /usr/local unless otherwise
    otherwise specified by the --prefix option, and a package.installed file
    will be created. These files will be used to skip packages that have been
    successfully installed in the event that a build fails and this script
    needs to be run multiple times. Removing a package.installed file will
    force the package to be built and installed again.

SYNOPSIS

    $script_name --prefix=path [--reinstall]

OPTIONS

  --prefix=path     absolute path to the installation directory
                    default: $prefix

  --destdir=path    absolute path prepended to prefix 
                    used to perform a staged installation

  --idlprefix=path  absolute path to the IDL installation directory
                    default: $idlprefix

  --pyprefix=path   installation prefix of the instance of python to use
                    default: $pyprefix

  --debug           install debug versions of packages using:
                    CFLAGS="-g -O0"

  --uninstall       uninstall all packages

  --reinstall       remove all package.installed files and reinstall
                    all packages

  -h, --help        display this help message

EOM
}

# Parse command line

for i in "$@"
do
    case $i in
        --debug)      debug=1
                      ;;
        --destdir=*)  destdir="${i#*=}"
                      ;;
        --prefix=*)   prefix="${i#*=}"
                      ;;
        --uninstall)  uninstall=1
                      ;;
        --reinstall)  reinstall=1
                      ;;
        -h | --help)  usage
                      exit
                      ;;
        *)            pkglist="${i#*=}"
                      ;;
    esac
done

# Set libdir

kernel=`uname`
if [ "$kernel" = "Darwin" ]; then
    libdir="$prefix/lib"
else
    libdir="$prefix/lib64"
fi

##############################################################################
# Functions

sep1="======================================================================"
sep2="----------------------------------------------------------------------"

function exit_fail {
    echo "***** FAILED *****"
    exit 1;
}

function exit_success {
    echo "***** SUCCESS *****"
    exit 0;
}

function run {
    echo "> $1"
    $1 || exit_fail
}

function strip_path () {
    echo `echo $1 | tr ":" "\n" | grep -v ^$2$ | tr "\n" ":" | sed ''s/:\$//''`
}

function build_and_install {
    local package=$1

    echo ""
    echo $sep1
    echo "PACKAGE = $package"
    echo $sep1
    echo ""

    # Make sure the package directory exists

    if [ ! -d "$package" ]; then

        echo "Skipping - package directory not found"
        echo ""
        installed=""
        return
    fi

    run "cd $package"

    # Make sure the build.sh script exists

    if [ ! -f ./build.sh ]; then

        echo "Skipping - missing ./build.sh script"
        echo ""
        installed=""
        run "cd .."
        return
    fi

    cmd="./build.sh"

    # Check if this package requires IDL

    if grep -Fq -- '--idlprefix=path' ./build.sh; then
        if [ "$idlprefix" = "IDL not found" ]; then
            echo "Skipping - IDL not found"
            echo ""
            installed=""
            run "cd .."
            return
        else
            cmd+=" --idlprefix=$idlprefix"
        fi
    fi

    # Check if thus package requires Python

    if grep -Fq -- '--pyprefix=path' ./build.sh; then
        if [ "$pyprefix" = "python3 not found" ]; then
            echo "Skipping - python3 not found"
            echo ""
            installed=""
            run "cd .."
            return
        else
            cmd+=" --pyprefix=$pyprefix"
        fi
    fi

    # Set build.sh options

    if [ $destdir ]; then
        cmd+=" --destdir=$destdir"
    fi

    cmd+=" --prefix=$prefix"

    if grep -Fq -- '--libdir=path' ./build.sh; then
        cmd+=" --libdir=$libdir"
    fi

    if [ $uninstall ]; then
        if grep -Fq -- '--uninstall' ./build.sh; then
            cmd+=" --uninstall"
        else
            echo "Could not uninstall: $package"
            echo "Build script does not support the uninstall option."
            run "cd .."
            return
        fi
    else
        if [ $debug ]; then
            if grep -Fq -- '--debug' ./build.sh; then
                cmd+=" --debug"
            else
                export CFLAGS="-g -Og"
            fi
        elif grep -Fq -- '--strip' ./build.sh; then
            cmd+=" --strip"
        fi

        if grep -Fq -- '--clean' ./build.sh; then
            cmd+=" --clean"
        fi
    fi

    run "$cmd"
    run "cd .."
}

##############################################################################
# Build and Install

# Add install location to PKG_CONFIG_PATH

ADI_PKG_CONFIG_PATH="$libdir/pkgconfig"

if [ -z "$PKG_CONFIG_PATH" ]; then
    export PKG_CONFIG_PATH=$ADI_PKG_CONFIG_PATH
else
    export PKG_CONFIG_PATH=$(strip_path $PKG_CONFIG_PATH $ADI_PKG_CONFIG_PATH)
    export PKG_CONFIG_PATH="${ADI_PKG_CONFIG_PATH}:$PKG_CONFIG_PATH"
fi

# Add install location to PERLLIB

ADI_PERLLIB="$prefix/lib"

if [ -z "$PERLLIB" ]; then
    export PERLLIB=$ADI_PERLLIB
else
    export PERLLIB=$(strip_path $PERLLIB $ADI_PERLLIB)
    export PERLLIB="${ADI_PERLLIB}:$PERLLIB"
fi

# Check if we are updating all packages

if [ $reinstall ]; then
    for pkg in ${pkgs[@]}; do
        installed="${pkg}.installed"
        run "rm -f $installed"
    done
fi

# Loop over all packages to install

for pkg in ${pkgs[@]}; do

    # Create file name used to track which packages
    # have been installed.

    installed="${pkg}.installed"

    # Build and install or uninstall the package.

    if [ ! -f $installed ] || [ $uninstall ]; then

        build_and_install ${pkg}

        if [ ! -z "$installed" ]; then

            if [ $uninstall ]; then
                run "rm -f $installed"
            else
                run "touch $installed"
            fi
        fi
    fi

done

exit_success
