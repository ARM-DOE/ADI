TESTS = run_test
EXTRA_DIST = \
	ref \
	run_test.in \
	transform_params.cfg \
	transform_params.include

check_PROGRAMS = libcds3_test
libcds3_test_SOURCES = \
	libcds3_test.h \
	libcds3_test.c \
	libcds3_test_att_values.c \
	libcds3_test_copy.c \
	libcds3_test_struct_defs.c \
	libcds3_test_time_data.c \
	libcds3_test_transform_params.c \
	libcds3_test_units.c \
	libcds3_test_utils.c \
	libcds3_test_var_data.c

libcds3_test_CFLAGS  = -Wall -Wextra -I${includedir}
libcds3_test_LDFLAGS = -L${libdir} -lcds3

CLEANFILES = run_test
MAINTAINERCLEANFILES = \
	Makefile.in 

