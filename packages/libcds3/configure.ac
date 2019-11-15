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
AC_CONFIG_SRCDIR([src/cds_version.c])

AC_SUBST(PACKAGE_VERSION)

GROUP=m4_esyscmd([./build-aux/get_group_name.sh])
PROJECT=m4_esyscmd([./build-aux/get_project_name.sh])
AC_SUBST(GROUP)
AC_SUBST(PROJECT)

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_LIBTOOL

# Checks for libraries.

PKG_CHECK_MODULES([MSNGR], [msngr], [],
    [AC_MSG_ERROR([msngr.pc not found in PKG_CONFIG_PATH])])

# Checks for header files.

AC_CHECK_HEADER([udunits2.h],
    [AC_DEFINE([UDUNITS_INCLUDE],[<udunits2.h>], [Units2 header file location])
     found_udunits=1])
AC_CHECK_HEADER([udunits2/udunits2.h],
    [AC_DEFINE([UDUNITS_INCLUDE],[<udunits2/udunits2.h>], [Units2 header file location])
     found_udunits=1])
if test -z "$found_udunits"; then
    AC_MSG_ERROR([Could not find required library header file: udunits2.h])
fi

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([Makefile
                 Doxyfile
                 src/Makefile
                 src/cds3.pc
                 test/Makefile])
AC_CONFIG_FILES([test/run_test], [chmod +x test/run_test])
AC_OUTPUT
