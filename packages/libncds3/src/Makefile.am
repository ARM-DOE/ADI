lib_LTLIBRARIES = libncds3.la

include_HEADERS     = ncds3.h
libncds3_la_SOURCES = \
	ncds_data_types.c \
	ncds_datastreams.c \
	ncds_find_files.c \
	ncds_get.c \
	ncds_private.h \
	ncds_read.c \
	ncds_utils.c \
	ncds_version.c \
	ncds_write.c \
	ncwrap_dataset.c \
	ncwrap_inquire.c

libncds3_la_CFLAGS  = -Wall -Wextra $(CDS3_CFLAGS) $(NETCDF_CFLAGS)
libncds3_la_LDFLAGS = -avoid-version -no-undefined
libncds3_la_LIBADD  = $(CDS3_LIBS) $(NETCDF_LIBS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = ncds3.pc

MAINTAINERCLEANFILES = Makefile.in
