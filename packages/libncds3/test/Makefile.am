TESTS = run_test
EXTRA_DIST = \
	ref \
	run_test.in

check_PROGRAMS = libncds3_test
libncds3_test_SOURCES = \
	cds3_factory.c \
	cds3_factory.h \
	libncds3_test.c \
	libncds3_test.h

libncds3_test_CFLAGS  = -Wall -Wextra -I${includedir} $(CDS3_CFLAGS) $(NETCDF_CFLAGS)
libncds3_test_LDFLAGS = -L${libdir} $(CDS3_LIBS) $(NETCDF_LIBS) -lncds3

CLEANFILES = run_test
MAINTAINERCLEANFILES = \
	Makefile.in 

