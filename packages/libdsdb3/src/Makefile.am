lib_LTLIBRARIES = libdsdb3.la

include_HEADERS     = dsdb3.h
libdsdb3_la_SOURCES = \
	datastream_config.c \
	dbog_dod.c \
	dbog_dod.h \
	dbog_dsdb.c \
	dbog_dsdb.h \
	dbog_retriever.c \
	dbog_retriever.h \
	dqrdb.c \
	ds_class.c \
	dsdb3.h \
	dsdb3.pc.in \
	dsdb.c \
	dsdb_version.c \
	ds_dod.c \
	ds_properties.c \
	ds_times.c \
	facility_location.c \
	family_process.c \
	Makefile.am \
	process_config.c \
	process_location.c \
	process_state.c \
	process_status.c \
	retriever.c

libdsdb3_la_CFLAGS  = -Wall -Wextra $(DBCONN_CFLAGS) $(CDS3_CFLAGS) 
libdsdb3_la_LDFLAGS = -avoid-version -no-undefined
libdsdb3_la_LIBADD  = $(DBCONN_LIBS) $(CDS3_LIBS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = dsdb3.pc

MAINTAINERCLEANFILES = Makefile.in
