lib_LTLIBRARIES = libcds3.la

include_HEADERS    = cds3.h
libcds3_la_SOURCES = \
	cds_atts.c \
	cds_converter.c \
	cds_copy.c \
	cds_data_types.c \
	cds_dims.c \
	cds_groups.c \
	cds_objects.c \
	cds_print.c \
	cds_private.h \
	cds_times.c \
	cds_transform_params.c \
	cds_units_map.h \
	cds_units.c \
	cds_utils.c \
	cds_vararrays.c \
	cds_var_data.c \
	cds_vargroups.c \
	cds_vars.c \
	cds_version.c

libcds3_la_CFLAGS  = -Wall -Wextra -Wno-unused-parameter $(MSNGR_CFLAGS)
libcds3_la_LDFLAGS = -avoid-version -no-undefined
libcds3_la_LIBADD  = $(MSNGR_LIBS) -ludunits2

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = cds3.pc

MAINTAINERCLEANFILES = Makefile.in
