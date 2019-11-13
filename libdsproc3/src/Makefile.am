lib_LTLIBRARIES = libdsproc3.la

include_HEADERS       = dsproc3.h dsproc3_internal.h
libdsproc3_la_SOURCES = \
	dsproc.c \
	dsproc_csv2cds.c \
	dsproc_csv_ingest_config.c \
	dsproc_csv_parser.c \
	dsproc_dataset_atts.c \
	dsproc_dataset_compare.c \
	dsproc_dataset_dims.c \
	dsproc_dataset_fetch.c \
	dsproc_dataset_filters.c \
	dsproc_datasets.c \
	dsproc_dataset_store.c \
	dsproc_dataset_times.c \
	dsproc_dataset_vars.c \
	dsproc_datastream_dod.c \
	dsproc_datastream_files.c \
	dsproc_datastreams.c \
	dsproc_deprecated.c \
	dsproc_dqrdb.c \
	dsproc_dsdb.c \
	dsproc_exec.c \
	dsproc_file_utils.c \
	dsproc_hooks.c \
	dsproc_main.c \
	dsproc_map_data.c \
	dsproc_merge_obs.c \
	dsproc_messages.c \
	dsproc_outlier_filters.c \
	dsproc_parse_args.c \
	dsproc_print.c \
	dsproc_private.h \
	dsproc_qc_utils.c \
	dsproc_rename.c \
	dsproc_retriever.c \
        dsproc_solar_position.c \
	dsproc_standard_qc.c \
	dsproc_station_view_hook.c \
	dsproc_time_utils.c \
	dsproc_transform.c \
	dsproc_trans_params.c \
	dsproc_update_stored_metadata.c \
	dsproc_var_tag.c \
	dsproc_version.c

libdsproc3_la_CFLAGS  = -Wall -Wextra -Wno-unused-parameter -std=gnu99 $(DSDB3_CFLAGS) $(TRANS_CFLAGS) $(NCDS3_CFLAGS) $(ARMUTILS_CFLAGS)
libdsproc3_la_LDFLAGS = -avoid-version -no-undefined
libdsproc3_la_LIBADD = $(DSDB3_LIBS) $(TRANS_LIBS) $(NCDS3_LIBS) $(ARMUTILS_LIBS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = dsproc3.pc

MAINTAINERCLEANFILES = Makefile.in
