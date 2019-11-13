#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([adi_idl], [1.19], [krista.gaustad@pnnl.gov])

AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([foreign -Wall -Werror])
#AM_MAINTAINER_MODE

m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
LT_INIT(disable-static)

AC_CONFIG_SRCDIR([src/idl_dsproc.c])

AC_SUBST(PACKAGE_NAME)
AC_SUBST(PACKAGE_VERSION)

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

# Checks for libraries.

PKG_CHECK_MODULES([DSPROC3], [dsproc3], [],
    [AC_MSG_ERROR([dsproc3 not found in PKG_CONFIG_PATH])])

AC_SUBST(DSPROC3_CFLAGS)
AC_SUBST(DSPROC3_LIBS)

# End Hack

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
