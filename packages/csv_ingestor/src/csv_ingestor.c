/*****************************************************************************
* Copyright (C) 2014, Battelle Memorial Institute
* All rights reserved.
*
* Author:
*    name:  Brian Ermold
*    email: brian.ermold@pnnl.gov
*
******************************************************************************/

/** @file csv_ingestor.c
 *  CSV Ingestor.
 */

#include "csv_ingestor.h"

#include "../config.h"
static const char *_Version = PACKAGE_NAME"-"PACKAGE_VERSION;

/**
 *  Initialize the CSV Ingestor process.
 *
 *  This function will:
 *
 *    - create the UserData structure
 *    - load the CSV configuration file
 *    - initialize the CSV parser
 *    - add the raw data file patterns
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @retval  data  void pointer to the UserData structure
 *  @retval  NULL  if an error occurred
 */
void *csv_ingestor_init(void)
{
    UserData    *data  = (UserData *)NULL;
    CSVConf     *conf  = (CSVConf *)NULL;
    int         *dsids = (int *)NULL;
    int          ndsids;
    DsData     **dsp;
    const char  *name;
    const char  *level;
    int          dsi;

    /************************************************************
    *  Create the UserData structure
    *************************************************************/

    DSPROC_DEBUG_LV1(
        "Initializing csv_ingestor process\n");

    data = (UserData *)calloc(1, sizeof(UserData));

    if (!data) {
        DSPROC_ERROR( DSPROC_ENOMEM,
            "Memory allocation error initializing process\n");
        return((void *)data);
    }

    data->proc_name = dsproc_get_name();
    data->site      = dsproc_get_site();
    data->fac       = dsproc_get_facility();

    /*********************************************************************
    * Get the input datastream ID.
    **********************************************************************/

    ndsids = dsproc_get_input_datastream_ids(&dsids);

    if (ndsids != 1) {

        if (ndsids > 1) {

            DSPROC_ERROR( NULL,
                "Too many input datastreams defined for process: %s"
                " -> found %d, but only expected one",
                data->proc_name, ndsids);
        }
        else if (ndsids == 0) {

            DSPROC_ERROR( NULL,
                "No input datastreams defined for process: %s",
                data->proc_name);
        }

        if (dsids) free(dsids);
        goto ERROR_EXIT;
    }

    data->raw_in_dsid = dsids[0];
    free(dsids);
    dsids = (int *)NULL;

    /***********************************************************
    *  Get the output datastream IDs
    **************************************************************/

    ndsids = dsproc_get_output_datastream_ids(&dsids);
    if (ndsids < 2) {

        if (ndsids == 1) {

            level = dsproc_datastream_class_level(dsids[0]);

            if (*level == '0') {

                DSPROC_ERROR( NULL,
                    "Not enough output datastreams defined for process: %s"
                    " -> missing output datastream for processed data",
                    data->proc_name);
            }
            else {

                DSPROC_ERROR( NULL,
                    "Not enough output datastreams defined for process: %s"
                    " -> missing output datastream for raw data",
                    data->proc_name);
            }
        }
        else if (ndsids == 0) {

            DSPROC_ERROR( NULL,
                "No output datastreams defined for process: %s",
                data->proc_name);
        }

        if (dsids) free(dsids);
        goto ERROR_EXIT;
    }

    /***********************************************************
    *  Initialize the Datastream Data structures
    **************************************************************/

    dsp = calloc(ndsids, sizeof(DsData *));
    if (!dsp) {
        DSPROC_ERROR( DSPROC_ENOMEM,
            "Memory allocation error initializing process\n");
        goto ERROR_EXIT;
    }

    data->dsp  = dsp;
    data->ndsp = 0;
    data->raw_out_dsid = -1;

    for (dsi = 0; dsi < ndsids; ++dsi) {

        name  = dsproc_datastream_class_name(dsids[dsi]);
        level = dsproc_datastream_class_level(dsids[dsi]);

        if (*level == '0') {

            if (data->raw_out_dsid == -1) {
                data->raw_out_dsid = dsids[dsi];
            }
            else {

                DSPROC_ERROR( NULL,
                    "Invalid output datastream: %s.%s"
                    " -> a process can only have one output 00 level datastream for raw data",
                    name, level);

                free(dsids);
                goto ERROR_EXIT;
            }
        }
        else {
            dsp[data->ndsp] = csv_ingestor_init_dsdata(name, level);
            if (!dsp[data->ndsp]) goto ERROR_EXIT;
            data->ndsp += 1;
        }
    }

    free(dsids);
    dsids = (int *)NULL;

    if (data->raw_out_dsid == -1) {

        DSPROC_ERROR( NULL,
            "Not enough output datastreams defined for process: %s"
            " -> a process must have one output 00 level datastream for raw data",
            data->proc_name);

        goto ERROR_EXIT;
    }

    /************************************************************
    *  Add the input file patterns to look for
    *************************************************************/

    for (dsi = 0; dsi < data->ndsp; ++dsi) {

        conf = data->dsp[dsi]->conf;

        if (!conf->fn_npatterns) {

            DSPROC_ERROR( NULL,
                "No input file name patterns found in configuration file: %s/%s",
                conf->file_path, conf->file_name);

            goto ERROR_EXIT;
        }

        if (!dsproc_add_datastream_file_patterns(
            data->raw_in_dsid, conf->fn_npatterns, conf->fn_patterns, 0)) {

            goto ERROR_EXIT;
        }
    }

    return((void *)data);

ERROR_EXIT:

    csv_ingestor_finish(data);

    return((void *)NULL);
}

/**
 *  Finish the CSV Ingestor process.
 *
 *  This function frees all memory used by the UserData structure.
 *
 *  @param user_data  - void pointer to the UserData structure
 *                      returned by the csv_ingestor_init() function
 */
void csv_ingestor_finish(void *user_data)
{
    UserData *data = (UserData *)user_data;
    int       dsi;

    DSPROC_DEBUG_LV1(
        "Cleaning up allocated memory\n");

    if (data) {

        if (data->dsp) {

            for (dsi = 0; dsi < data->ndsp; ++dsi) {
                csv_ingestor_free_dsdata(data->dsp[dsi]);
            }

            free(data->dsp);
        }

        free(data);
    }
}

/**
 *  Process a raw CSV data file.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  user_data  void pointer to the UserData structure
 *  @param  input_dir  path to the file
 *  @param  file_name  file name
 *
 *  @retval   1  if successful
 *  @retval   0  if the current file should be skipped
 *  @retval  -1  if a fatal error occurred
 */
int csv_ingestor_process_file(
    void       *user_data,
    const char *input_dir,
    const char *file_name)
{
    UserData  *data = (UserData *)user_data;
    DsData    *ds   = (DsData *)NULL;
    CSVConf   *conf;
    CSVParser *csv;

    time_t     file_time;
    char       full_path[PATH_MAX];
    int        nrecs_loaded;
    int        nrecs_stored;
    int        status;
    int        dsi;

    /************************************************************
    *  Find the correct DsData structure to use for this file
    *************************************************************/

    for (dsi = 0; dsi < data->ndsp; ++dsi) {

        ds = data->dsp[dsi];

        status = relist_execute(
            ds->fn_relist, file_name, 0, NULL, NULL, NULL, NULL);

        if (status == 1) {
            break;
        }

        if (status < 0) {

            DSPROC_ERROR( NULL,
                "Regex error while looking for DsData structure for file: %s\n",
                file_name);

            return(-1);
        }
    }

    if (dsi == data->ndsp) {

        DSPROC_ERROR( NULL,
            "Could not find matching DsData structure for file: %s\n",
            file_name);

        return(-1);
    }

    conf = ds->conf;
    csv  = ds->csv;

    /************************************************************
    *  Initialize data structures for a new file read
    *************************************************************/

    data->input_dir  = input_dir;
    data->file_name  = file_name;
    data->begin_time = 0;
    data->end_time   = 0;

    dsproc_init_csv_parser(csv);

    /* Set the number of dots from the end of the file name to
     * preserve when the file is renamed. */

    if (!dsproc_set_preserve_dots_from_name(data->raw_out_dsid, file_name)) {
        return(-1);
    }

    /************************************************************
    *  Check for a time varying CSV configuration file
    *************************************************************/

    file_time = dsproc_get_csv_file_name_time(csv, file_name, NULL);
    if (file_time < 0) return(-1);

    status = dsproc_load_csv_conf(conf, file_time, CSV_CHECK_DATA_CONF);
    if (status < 0) return(-1);

    if (status == 1) {

        /* An updated configuration file was found. */

        if (!dsproc_configure_csv_parser(conf, csv)) {
            return(-1);
        }

        /* Free the csv to cds map so it gets recreated. */

        if (ds->map) {
            dsproc_free_csv_to_cds_map(ds->map);
            ds->map = (CSV2CDSMap *)NULL;
        }
    }

    /************************************************************
    *  Read in the raw data file
    *************************************************************/

    DSPROC_DEBUG_LV1("Loading file:   %s\n", data->file_name);

    snprintf(full_path, PATH_MAX, "%s/%s", input_dir, file_name);

    nrecs_loaded = csv_ingestor_read_data(data, ds);
    if (nrecs_loaded  < 0) return(-1);
    if (nrecs_loaded == 0) {

        DSPROC_BAD_FILE_WARNING(file_name,
            "No valid data records found in file\n"
            " -> marking file bad and continuing\n");

        if (data->begin_time == 0) {
            data->begin_time = file_time;
        }

        if (dsproc_rename_bad(data->raw_out_dsid,
            input_dir, file_name, data->begin_time)) {

            return(0);
        }
        else {
            return(-1);
        }
    }

    /************************************************************
    *  Store the data
    *************************************************************/

    nrecs_stored = csv_ingestor_store_data(data, ds);
    if (nrecs_stored < 0) return(-1);

    /************************************************************
    *  Rename the raw data file
    *************************************************************/

    if (!dsproc_rename(data->raw_out_dsid,
        input_dir, file_name, data->begin_time, data->end_time)) {

        return(-1);
    }

    return(1);
}

/**
 *  Free the memory used by a DsData structure.
 *
 *  @param  ds  pointer to the DsData structure
 */
void csv_ingestor_free_dsdata(DsData *ds)
{
    if (ds) {

        if (ds->conf)      dsproc_free_csv_conf(ds->conf);
        if (ds->csv)       dsproc_free_csv_parser(ds->csv);
        if (ds->map)       dsproc_free_csv_to_cds_map(ds->map);
        if (ds->fn_relist) relist_free(ds->fn_relist);

        free(ds);
    }
}

/**
 *  Initialize a Datastream Data structure.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  dsname    output datastream name
 *  @param  dslevel   output datastream level
 *
 *  @retval  ds    pointer to the new DsData structure
 *  @retval  NULL  if an error occurred
 */
DsData *csv_ingestor_init_dsdata(
    const char *dsname,
    const char *dslevel)
{
    DsData    *ds        = (DsData *)NULL;
    CSVConf   *conf      = (CSVConf *)NULL;
    CSVParser *csv       = (CSVParser *)NULL;
    REList    *fn_relist = (REList *)NULL;
    int        re_cflags = REG_EXTENDED | REG_NOSUB;
    SplitMode  split_mode;
    double     split_start;
    double     split_interval;
    int        status;

    /************************************************************
    * Allocate memory for the new DsData structure
    *************************************************************/

    ds = (DsData *)calloc(1, sizeof(DsData));
    if (!ds) {
        DSPROC_ERROR( DSPROC_ENOMEM,
            "Memory allocation error creating DsData structure\n");
        return(0);
    }

    /***********************************************************
    *  Get the output datastream ID
    **************************************************************/

    ds->dsid = dsproc_get_output_datastream_id(dsname, dslevel);
    if (ds->dsid < 0) {
        goto ERROR_EXIT;
    }

    /************************************************************
    *  Find and load the CSV Ingestor configuration file
    *************************************************************/

    conf = dsproc_init_csv_conf(dsname, dslevel);
    if (!conf) {
        goto ERROR_EXIT;
    }

    status = dsproc_load_csv_conf(conf, 0, CSV_CHECK_DATA_CONF);
    if (status <= 0) {

        if (status == 0) {

            DSPROC_ERROR( NULL,
                "Could not find required configuration file: %s.%s.csv_conf",
                dsname, dslevel);
        }

        goto ERROR_EXIT;
    }

    ds->conf = conf;

    /************************************************************
    *  Initialize the CSV Parser
    *************************************************************/

    csv = dsproc_init_csv_parser(NULL);
    if (!csv) {
        goto ERROR_EXIT;
    }

    if (!dsproc_configure_csv_parser(conf, csv)) {
        goto ERROR_EXIT;
    }

    ds->csv = csv;

    /************************************************************
    *  Check if a split interval was set in the conf file
    *************************************************************/

    if (conf->split_interval) {

        if (strcmp(conf->split_interval, "DAILY") == 0) {

            split_mode     = SPLIT_ON_HOURS;
            split_start    = 0.0;
            split_interval = 24.0;
        }
        else if (strcmp(conf->split_interval, "MONTHLY") == 0) {

            split_mode     = SPLIT_ON_MONTHS;
            split_start    = 1.0;
            split_interval = 1.0;
        }
        else if (strcmp(conf->split_interval, "YEARLY") == 0) {

            split_mode     = SPLIT_ON_MONTHS;
            split_start    = 1.0;
            split_interval = 12.0;
        }
        else if (strcmp(conf->split_interval, "FILE") == 0) {

            split_mode     = SPLIT_ON_STORE;
            split_start    = 0.0;
            split_interval = 0.0;
        }
        else if (strcmp(conf->split_interval, "NONE") == 0) {

            split_mode     = SPLIT_NONE;
            split_start    = 0.0;
            split_interval = 0.0;
        }
        else {

            DSPROC_ERROR( NULL,
                "Invalid split interval '%s' found in configuration file: %s/%s",
                conf->split_interval, conf->file_path, conf->file_name);

            goto ERROR_EXIT;
        }

        dsproc_set_datastream_split_mode(
            ds->dsid, split_mode, split_start, split_interval);
    }

    /************************************************************
    *  Compile the file name patterns.  This will be used later
    *  to determine which DsData structure to use.
    *************************************************************/

    if (!conf->fn_npatterns) {

        DSPROC_ERROR( NULL,
            "No input file name patterns found in configuration file: %s/%s",
            conf->file_path, conf->file_name);

        goto ERROR_EXIT;
    }

    fn_relist = relist_compile(
        NULL, conf->fn_npatterns, conf->fn_patterns, re_cflags);

    if (!fn_relist) {

        DSPROC_ERROR( NULL,
            "Could not compile file name patterns in configuration file: %s/%s\n",
            conf->file_path, conf->file_name);

        goto ERROR_EXIT;
    }

    ds->fn_relist = fn_relist;

    return(ds);

ERROR_EXIT:

    csv_ingestor_free_dsdata(ds);
    return((DsData *)NULL);
}

/**
 *  Main CSV Ingestor entry function.
 *
 *  @param  argc - number of command line arguments
 *  @param  argv - command line arguments
 *
 *  @retval  0  successful
 *  @retval  1  failure
 */
int main(int argc, char **argv)
{
    int exit_value;

    /* Set output NetCDF file extension to be .nc */

    dsproc_use_nc_extension();

    /* Set Ingest hook functions */

    dsproc_set_init_process_hook(csv_ingestor_init);
    dsproc_set_process_file_hook(csv_ingestor_process_file);
    dsproc_set_finish_process_hook(csv_ingestor_finish);

    /* Run the ingest */

    exit_value = dsproc_main(
        argc, argv,
        PM_INGEST,
        _Version,
        0, NULL);

    return(exit_value);
}
