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
AC_CONFIG_SRCDIR([src/dsproc_version.c])

AC_SUBST(PACKAGE_VERSION)

GROUP=m4_esyscmd([./build-aux/get_group_name.sh])
PROJECT=m4_esyscmd([./build-aux/get_project_name.sh])
AC_SUBST(GROUP)
AC_SUBST(PROJECT)

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

# Checks for libraries.

PKG_CHECK_MODULES([NCDS3], [ncds3], [],
    [AC_MSG_ERROR([ncds3.pc not found in PKG_CONFIG_PATH])])

PKG_CHECK_MODULES([ARMUTILS], [armutils], [],
    [AC_MSG_ERROR([armutils.pc not found in PKG_CONFIG_PATH])])

PKG_CHECK_MODULES([TRANS], [trans], [],
    [AC_MSG_ERROR([trans.pc not found in PKG_CONFIG_PATH])])

PKG_CHECK_MODULES([DSDB3], [dsdb3], [],
    [AC_MSG_ERROR([dsdb3.pc not found in PKG_CONFIG_PATH])])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([Makefile
                 Doxyfile
                 src/Makefile
                 src/dsproc3.pc])
AC_OUTPUT
