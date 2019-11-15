#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT(
    m4_esyscmd([./build-aux/get_package_name.sh]),
    m4_esyscmd([./build-aux/get_package_version.sh]),
    [brian.ermold@pnnl.gov])

AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([foreign -Wall -Werror])
#AM_MAINTAINER_MODE

m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
LT_INIT(disable-static)

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([src/msngr_version.c])

AC_SUBST(PACKAGE_VERSION)

GROUP=m4_esyscmd([./build-aux/get_group_name.sh])
PROJECT=m4_esyscmd([./build-aux/get_project_name.sh])
AC_SUBST(GROUP)
AC_SUBST(PROJECT)

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([Makefile
                 Doxyfile
                 src/Makefile
                 src/msngr.pc])
AC_OUTPUT
