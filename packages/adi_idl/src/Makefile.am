moduledir             = @libdir@
module_LTLIBRARIES    = idl_dsproc.la

idl_dsproc_la_SOURCES = idl_dsproc.c

idl_dsproc_la_CFLAGS  = -Wall -Wextra $(DSPROC3_CFLAGS)
idl_dsproc_la_LDFLAGS = -module -avoid-version -no-undefined
idl_dsproc_la_LIBADD  = $(DSPROC3_LIBS)

prolibdir = @libdir@
prolib_DATA = \
	cdsatt__define.pro \
	cdsdim__define.pro \
	cdsgroup__define.pro \
	cdsobject__define.pro \
	cds_obj_typename.pro \
	cdsvararray__define.pro \
	cdsvar__define.pro \
	cdsvargroup__define.pro \
	dsproc__define.pro \
	dsproc__main.pro \
	idl_dsproc.pro \
	idl_dsproc.dlm

MAINTAINERCLEANFILES = Makefile.in
