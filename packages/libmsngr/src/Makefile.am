lib_LTLIBRARIES     = libmsngr.la

include_HEADERS     = msngr.h messenger.h
libmsngr_la_SOURCES = \
	msngr.c \
	msngr_lockfile.c \
	msngr_log.c \
	msngr_mail.c \
	msngr_procstats.c \
	msngr_utils.c \
	msngr_version.c

libmsngr_la_CFLAGS  = -Wall -Wextra -Wno-unused-parameter
libmsngr_la_LDFLAGS = -avoid-version -no-undefined

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = msngr.pc

MAINTAINERCLEANFILES = Makefile.in
