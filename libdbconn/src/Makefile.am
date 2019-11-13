lib_LTLIBRARIES = libdbconn.la

include_HEADERS      = dbconn.h
libdbconn_la_SOURCES = \
	dbconn.c \
	dbconn_pgsql.c \
	dbconn_pgsql.h \
	dbconn_private.h \
	dbconn_sqlite.c \
	dbconn_sqlite.h \
	dbconn_version.c \
	dbconn_wspc.c \
	dbconn_wspc.h \
	sqlite3.c \
	sqlite3.h 

libdbconn_la_CFLAGS  = -Wall -Wextra $(MSNGR_CFLAGS) $(POSTGRESQL_CFLAGS) $(CURL_CFLAGS)
libdbconn_la_LDFLAGS = -avoid-version -no-undefined
libdbconn_la_LIBADD  = $(MSNGR_LIBS) $(POSTGRESQL_LDFLAGS) $(CURL_LIBS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = dbconn.pc

MAINTAINERCLEANFILES = Makefile.in
