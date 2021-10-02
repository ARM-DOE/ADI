from distutils.core import setup
from distutils.extension import Extension
from Cython.Build import cythonize
from subprocess import check_output, CalledProcessError
from os import getenv
import numpy
import sys
import re

# Get package name
version = "{0}{1}".format(*sys.version_info[:2])
name    = getenv('BUILD_PACKAGE_NAME', "adi_py"+version)

# Get package version
version = getenv('BUILD_PACKAGE_VERSION')
if version:
    version = version.replace('-', '.', 1) # change x.y-z to x.y.z
else:
    command = 'git describe --tags'
    try:
        version = check_output(command.split()).decode('utf-8').strip()
        version = re.search('.*v([0-9]+\.[0-9]+\.[0-9]+)', version).group(1)
    except CalledProcessError:
        version = '0.0.0'

# Get include_dirs, library_dirs, and libraries

def pkgconfig(lib, opt):
    res = check_output(["pkg-config", opt, lib]).decode('utf-8').strip()
    return [re.compile(r'^-[ILl]').sub('', m) for m in res.split()]

cds3_incdirs = pkgconfig("cds3", '--cflags-only-I')
cds3_libdirs = pkgconfig("cds3", '--libs-only-L')
cds3_libs    = pkgconfig("cds3", '--libs-only-l')

dsproc3_incdirs = pkgconfig("dsproc3", '--cflags-only-I')
dsproc3_libdirs = pkgconfig("dsproc3", '--libs-only-L')
dsproc3_libs    = pkgconfig("dsproc3", '--libs-only-l')

numpy_incdir = numpy.get_include()
cds3_incdirs.append(numpy_incdir)
dsproc3_incdirs.append(numpy_incdir)

# Extension Modules

cds3 = Extension(
    name            = 'cds3.core',
    sources         = ['cds3/core.pyx'],
    include_dirs    = cds3_incdirs,
    library_dirs    = cds3_libdirs,
    libraries       = cds3_libs,
    runtime_library_dirs = cds3_libdirs
)

cds3_enums = Extension(
    name            = 'cds3.enums',
    sources         = ['cds3/enums.pyx'],
    include_dirs    = cds3_incdirs,
    library_dirs    = cds3_libdirs,
    libraries       = cds3_libs,
    runtime_library_dirs = cds3_libdirs
)

dsproc3 = Extension(
    name            = 'dsproc3.core',
    sources         = ['dsproc3/core.pyx'],
    include_dirs    = dsproc3_incdirs,
    library_dirs    = dsproc3_libdirs,
    libraries       = dsproc3_libs,
    runtime_library_dirs = dsproc3_libdirs
)

dsproc3_enums = Extension(
    name            = 'dsproc3.enums',
    sources         = ['dsproc3/enums.pyx'],
    include_dirs    = dsproc3_incdirs,
    library_dirs    = dsproc3_libdirs,
    libraries       = dsproc3_libs,
    runtime_library_dirs = dsproc3_libdirs
)

# Setup

setup(
    name        = name,
    version     = version,
    ext_modules = cythonize([cds3,cds3_enums,dsproc3,dsproc3_enums]),
    packages    = ['cds3', 'dsproc3', 'adi_py']
)

