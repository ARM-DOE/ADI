lib_LTLIBRARIES = libtrans.la

include_HEADERS     = trans.h
libtrans_la_SOURCES = \
	cds_transform_driver.c \
	timing.h \
	trans_bin_average.c \
	trans_caracena.c \
	trans_interpolate.c \
	trans_param_atts.c \
	trans_passthrough.c \
	trans_private.h \
	trans_subsample.c \
	trans_utils.c \
	trans_version.c

libtrans_la_CFLAGS  = -Wall -Wextra -std=gnu99 $(CDS3_CFLAGS)
libtrans_la_LDFLAGS = -avoid-version -no-undefined
libtrans_la_LIBADD  = $(CDS3_LIBS) $(LAPACK_LIBS) $(BLAS_LIBS) $(LIBS) $(FLIBS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = trans.pc

MAINTAINERCLEANFILES = Makefile.in
