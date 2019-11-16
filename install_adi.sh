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

python=$( which python3 )
if [ -z "$python" ]; then
    python=$( which python )
fi

if [ -z "$python" ]; then
    pyprefix="python not found"
    echo "python not found - skipping installation of python bindings"
else
    pyprefix=${python%/bin/*}
    # hack for old ARM system
    if [ "$pyprefix" = "/apps/base/python3" ]; then
        pyprefix="/apps/base/python3.6"
    fi
fi

# Set default idlprefix

idlprefix=$( which idl )
if [ -z "$idlprefix" ]; then
    idlprefix="IDL not found"
    echo "IDL not found - skipping installation of IDL bindings"
else
    idlprefix=${idlprefix%/bin/idl}
    # hack for old ARM system
    if [ "$idlprefix" = "/apps/base/rsi/idl82" ]; then
        idlprefix="/apps/base/idl/idl86"
    fi
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
                    this does not uninstall the database or etc files

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
        --pyprefix=*) pyprefix="${i#*=}"
                      ;;
        --uninstall)  uninstall=1
                      ;;
        --reinstall)  reinstall=1
                      ;;
        -h | --help)  usage
                      exit
                      ;;
        *)            usage
                      exit
                      ;;
    esac
done

if [ "$pyprefix" = "python not found" ]; then
    pyprefix=""
fi

if [ "$idlprefix" = "IDL not found" ]; then
    idlprefix=""
fi

# Set libdir

kernel=`uname`
if [ "$kernel" = "Darwin" ]; then
    libdir="lib"
else
    libdir="lib64"
fi

###############################################################################
# Functions

sep1="======================================================================"
sep2="----------------------------------------------------------------------"

function exit_fail {
    echo ""
    echo "***** FAILED *****"
    echo ""
    exit 1;
}

function exit_success {
    echo ""
    echo "***** SUCCESS *****"
    echo ""
    exit 0;
}

function run() {
    echo "> $1"
    eval $1 || exit_fail
}

function strip_path () {
    echo `echo $1 | tr ":" "\n" | grep -v ^$2$ | tr "\n" ":" | sed ''s/:\$//''`
}

function get_idlversion() {
    if [ -z "$idlprefix" ]; then
        idlversion=""
    else
        idlversion=${idlprefix##*/}
    fi
    echo $idlversion
}

function get_pyversion() {
    if [ -z "$pyprefix" ]; then
        idlversion=""
        pyversion=""
    else
        if [ -e "$pyprefix/bin/python3" ]; then
            python="$pyprefix/bin/python3"
        else
            python="$pyprefix/bin/python"
        fi
        pyv=$($python -c 'import sys; print("%d.%d" % sys.version_info[:2])')
        pyversion="python$pyv"
    fi
    echo $pyversion
}

function build_and_install {
    local package=$1

    echo ""
    echo $sep1
    echo "Installing Package: $package"
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
        if [ -z "$idlprefix" ]; then
            echo "Skipping - IDL not found"
            echo ""
            installed=""
            run "cd .."
            return
        else
            cmd+=" --idlprefix='$idlprefix'"
        fi
    fi

    # Check if thus package requires Python

    if grep -Fq -- '--pyprefix=path' ./build.sh; then
        if [ -z "$pyprefix" ]; then
            echo "Skipping - python not found"
            echo ""
            installed=""
            run "cd .."
            return
        else
            cmd+=" --pyprefix='$pyprefix'"
        fi
    fi

    # Set build.sh options

    if [ $destdir ]; then
        cmd+=" --destdir='$destdir'"
    fi

    cmd+=" --prefix='$prefix'"

    if grep -Fq -- '--libdir=path' ./build.sh; then
        cmd+=" --libdir='$prefix/$libdir'"
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

function append_adirc_idl {
    local infile=$1
    local outfile=$2

    echo "updating: $outfile"
    echo " -> adding IDL environment settings"

    sed -e "s,@LIBDIR@,$libdir,g" \
        -e "s,@IDLVERSION@,$idlversion,g" \
        -e "s,@IDLPREFIX@,$idlprefix,g" \
        < "$infile" >> "$outfile"
}

function append_adirc_py {
    local infile=$1
    local outfile=$2

    echo "updating: $outfile"
    echo " -> adding Python environment settings"

    sed -e "s,@LIBDIR@,$libdir,g" \
        -e "s,@PYVERSION@,$pyversion,g" \
        < "$infile" >> "$outfile"
}

function install_in_file {
    local infile=$1
    local outfile=$2
    local mode=$3

    echo "installing: $outfile"

    sed -e "s,@ADI_PREFIX@,$prefix,g" \
        -e "s,@LIBDIR@,$libdir,g" \
        < "$infile" > "$outfile"

    run "chmod $mode '$outfile'"
}

###############################################################################
# Build and Install

# Add install location to PKG_CONFIG_PATH

ADI_PKGCONFIG="$prefix/$libdir/pkgconfig"

if [ -z "$PKG_CONFIG_PATH" ]; then
    export PKG_CONFIG_PATH="$ADI_PKGCONFIG"
else
    export PKG_CONFIG_PATH=$(strip_path $PKG_CONFIG_PATH $ADI_PKGCONFIG)
    export PKG_CONFIG_PATH="${ADI_PKGCONFIG}:$PKG_CONFIG_PATH"
fi

# Add install location to PERLLIB

ADI_PERLLIB="$prefix/lib"

if [ -z "$PERLLIB" ]; then
    export PERLLIB="$ADI_PERLLIB"
else
    export PERLLIB=$(strip_path $PERLLIB $ADI_PERLLIB)
    export PERLLIB="${ADI_PERLLIB}:$PERLLIB"
fi

# Check if we are reinstalling all packages

run "cd packages"

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
                installed=""
            else
                run "touch $installed"
            fi
        fi
    fi

    if [ "$pkg" = "adi_py" ] && [ ! -z "$installed" ]; then
        pyversion=$(get_pyversion)
    fi

    if [ "$pkg" = "adi_idl" ] && [ ! -z "$installed" ]; then
        idlversion=$(get_idlversion)
    fi

done

run "cd .."

echo ""
echo $sep1

if [ $uninstall ]; then
    echo ""
    exit_success
fi

# Create the $prefix/etc directory if it doesn't exist

outdir="$prefix/etc"
if [ ! -d "outdir" ]; then
    echo ""
    run "mkdir -p '$outdir'"
fi

# Install .adi.bashrc file

indir="etc"
infile=".adi.bashrc.in"
outfile=${infile%.in}

echo ""
if [ -e "$outdir/$outfile" ]; then
    echo "skipping: $indir/$infile"
    echo " -> previously installed file already exists: $outdir/$outfile"
else
    install_in_file "$indir/$infile" "$outdir/$outfile" 0644

    if [ ! -z "$pyversion" ]; then
        append_adirc_py "$indir/.adi.bashrc.py.in" "$outdir/$outfile"
    fi

    if [ ! -z "$idlversion" ]; then
        append_adirc_idl "$indir/.adi.bashrc.idl.in" "$outdir/$outfile"
    fi
fi

# Install .adi.cshrc file

infile=".adi.cshrc.in"
outfile=${infile%.in}

echo ""
if [ -e "$outdir/$outfile" ]; then
    echo "skipping: $indir/$infile"
    echo " -> previously installed file already exists: $outdir/$outfile"
else
    install_in_file "$indir/$infile" "$outdir/$outfile" 0644

    if [ ! -z "$pyversion" ]; then
        append_adirc_py "$indir/.adi.cshrc.py.in" "$outdir/$outfile"
    fi

    if [ ! -z "$idlversion" ]; then
        append_adirc_idl "$indir/.adi.cshrc.idl.in" "$outdir/$outfile"
    fi
fi

# Install .db_connect file

infile=".db_connect.in"
outfile=${infile%.in}

echo ""
if [ -e "$outdir/$outfile" ]; then
    echo "skipping: $indir/$infile"
    echo " -> previously installed file already exists: $outdir/$outfile"
else
    install_in_file "$indir/$infile" "$outdir/$outfile" 0644
fi

# Create the $prefix/var/adi directory if it doesn't exist

outdir="$prefix/var/adi"
if [ ! -d "$outdir" ]; then
    echo ""
    run "mkdir -p '$outdir'"
fi

# Install dsdb.sqlite

indir="database"
infile="dsdb.sqlite"
outfile="dsdb.sqlite"

echo ""
if [ -e "$outdir/$outfile" ]; then
    echo "skipping: $indir/$infile"
    echo " -> previously installed database already exists: $outdir/$outfile"
else
    echo "installing: $outdir/$outfile"
    run "cp $indir/$infile $outdir/$outfile"
    run "chmod 0664 $outdir/$outfile"
fi

exit_success
