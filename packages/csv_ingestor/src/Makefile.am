bin_PROGRAMS = csv_ingestor

csv_ingestor_SOURCES = \
	csv_ingestor.c \
	csv_ingestor.h \
	csv_ingestor_read_data.c \
	csv_ingestor_store_data.c

csv_ingestor_CFLAGS  = -Wall -Wextra -Wno-unused-parameter $(DSPROC3_CFLAGS)
csv_ingestor_LDFLAGS = $(DSPROC3_LIBS)

MAINTAINERCLEANFILES = \
	Makefile.in
