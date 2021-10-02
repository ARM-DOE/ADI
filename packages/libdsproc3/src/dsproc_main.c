/*******************************************************************************
*
*  Copyright © 2014, Battelle Memorial Institute
*  All rights reserved.
*
********************************************************************************
*
*  Author:
*     name:  Brian Ermold
*     phone: (509) 375-2277
*     email: brian.ermold@pnl.gov
*
*******************************************************************************/

/** @file dsproc_main.c
 *  DSProc Main Entry Functions.
 */

#include "dsproc3.h"
#include "dsproc_private.h"

/** @privatesection */

/*******************************************************************************
 *  Static Data and Functions Visible Only To This Module
 */

/** Quicklook mode flag */
static int _QuicklookMode = 0;

/**
 *  Static: Main Ingest file processing loop.
 */
static void _dsproc_ingest_main_loop()
{
    int          ndsids;
    int         *dsids;
    int          dsid;
    const char  *level;
    int          nfiles;
    char       **files;
    const char  *input_dir;
    time_t       loop_start;
    time_t       loop_end;
    time_t       time_remaining;
    int          status;
    int          dsi, fi;

    /* Get the list of input datastream IDs */

    ndsids = dsproc_get_input_datastream_ids(&dsids);

    if (ndsids == 0) {

        ERROR( DSPROC_LIB_NAME,
            "Could not find an input datastream defined in the database\n");

        dsproc_set_status(DSPROC_ENOINDSC);
        return;
    }

    /* For now make sure that only one level 0 input datastream class was
     * specified in the database. I have ideas to allow for more than
     * one but there are a few caveats to this that will require some
     * more thought and work. */

    dsid = -1;

    for (dsi = 0; dsi < ndsids; dsi++) {

        level = dsproc_datastream_class_level(dsids[dsi]);
        if (!level) continue;

        if (level[0] == '0') {

            if (dsid == -1) {
                dsid = dsids[dsi];
            }
            else {

                ERROR( DSPROC_LIB_NAME,
                    "Too many level 0 input datastreams defined in database\n"
                    "  -> ingest framework only supports one level 0 input datastream\n");

                dsproc_set_status(DSPROC_ETOOMANYINDSC);

                free(dsids);
                return;
            }
        }
    }

    if (dsid == -1) {

        if (ndsids == 1) {
            dsid = dsids[0];
        }
        else {

            ERROR( DSPROC_LIB_NAME,
                "Too many input datastreams defined in database\n"
                "  -> ingest framework only supports one input datastream\n");

            dsproc_set_status(DSPROC_ETOOMANYINDSC);

            free(dsids);
            return;
        }
    }

    free(dsids);

    /* Get the list of input files */

    nfiles = dsproc_get_datastream_files(dsid, &files);

    if (nfiles <= 0) {

        if (nfiles == 0) {

            LOG( DSPROC_LIB_NAME,
                "No data files found to process in: %s\n",
                dsproc_datastream_path(dsid));

            dsproc_set_status(DSPROC_ENODATA);
        }

        return;
    }

    input_dir = dsproc_datastream_path(dsid);

    /* Loop over all input files */

    loop_start  = 0;
    loop_end    = 0;

    for (fi = 0; fi < nfiles; fi++) {

        /* Check the run time */

        time_remaining = dsproc_get_time_remaining();

        if (time_remaining >= 0) {

            if (time_remaining == 0) {
                break;
            }
            else if (loop_end - loop_start > time_remaining) {

                LOG( DSPROC_LIB_NAME,
                    "\nStopping ingest before max run time of %d seconds is exceeded\n",
                    (int)dsproc_get_max_run_time());

                dsproc_set_status(DSPROC_ERUNTIME);
                break;
            }
        }

        /* Process the file */

        DEBUG_LV1_BANNER( DSPROC_LIB_NAME,
            "PROCESSING FILE #%d: %s\n", fi + 1, files[fi]);

        LOG( DSPROC_LIB_NAME,
            "\nProcessing: %s/%s\n", input_dir, files[fi]);

        loop_start = time(NULL);

        dsproc_set_input_dir(input_dir);
        dsproc_set_input_source(files[fi]);

        status = _dsproc_run_process_file_hook(input_dir, files[fi]);
        if (status == -1) break;

        loop_end = time(NULL);
    }

    dsproc_free_file_list(files);
}

/**
 *  Static: Main VAP data processing loop.
 */
static void _dsproc_vap_main_loop(int proc_model)
{
    time_t    interval_begin;
    time_t    interval_end;
    CDSGroup *ret_data;
    CDSGroup *trans_data;
    int       status;

    while (dsproc_start_processing_loop(&interval_begin, &interval_end)) {

        ret_data   = (CDSGroup *)NULL;
        trans_data = (CDSGroup *)NULL;

        /* Run the pre_retrieval_hook function */

        status = _dsproc_run_pre_retrieval_hook(
            interval_begin, interval_end);

        if (status == -1) break;
        if (status ==  0) continue;

        dsproc_get_processing_interval(&interval_begin, &interval_end);

        if (_QuicklookMode != QUICKLOOK_ONLY) {

            /* Retrieve the data for the current processing interval */

            if (proc_model & DSP_RETRIEVER) {

                status = dsproc_retrieve_data(
                    interval_begin, interval_end, &ret_data);

                if (status == -1) break;
                if (status ==  0) continue;
            }

            /* Run the post_retrieval_hook function */

            status = _dsproc_run_post_retrieval_hook(
                interval_begin, interval_end, ret_data);

            if (status == -1) break;
            if (status ==  0) continue;

            /* Merge the observations in the retrieved data */

            if (!dsproc_merge_retrieved_data()) {
                break;
            }

            /* Run the pre_transform_hook function */

            status = _dsproc_run_pre_transform_hook(
                interval_begin, interval_end, ret_data);

            if (status == -1) break;
            if (status ==  0) continue;

            /* Perform the data transformations for transform VAPs */

            if (proc_model & DSP_TRANSFORM) {

                status = dsproc_transform_data(&trans_data);
                if (status == -1) break;
                if (status ==  0) continue;
            }

            /* Run the post_transform_hook function */

            status = _dsproc_run_post_transform_hook(
                interval_begin, interval_end, trans_data);

            if (status == -1) break;
            if (status ==  0) continue;

            /* Create output datasets */

            if (!dsproc_create_output_datasets()) {
                break;
            }

            /* Run the user's data processing function */

            if (trans_data) {
                status = _dsproc_run_process_data_hook(
                    interval_begin, interval_end, trans_data);
            }
            else {
                status = _dsproc_run_process_data_hook(
                    interval_begin, interval_end, ret_data);
            }

            if (status == -1) break;
            if (status ==  0) continue;

            /* Store all output datasets */

            if (!dsproc_store_output_datasets()) {
                break;
            }
        }

        /* Run the quicklook_hook function */

        if (_QuicklookMode != QUICKLOOK_DISABLE) {

            status = _dsproc_run_quicklook_hook(
                interval_begin, interval_end);

            if (status == -1) break;
            if (status ==  0) continue;
        }
    }

    return;
}

/** @publicsection */

/*******************************************************************************
 *  Public Functions
 */

/**
 *  Datasystem Process Main Function.
 *
 *  @param  argc         - command line argument count
 *  @param  argv         - command line argument vector
 *  @param  proc_model   - processing model to use
 *  @param  proc_version - process version
 *  @param  nproc_names  - number of valid process names
 *  @param  proc_names   - list of valid process names
 *
 *  @return
 *    - suggested program exit value
 *      (0 = successful, 1 = failure)
 */
int dsproc_main(
    int          argc,
    char       **argv,
    ProcModel    proc_model,
    const char  *proc_version,
    int          nproc_names,
    const char **proc_names)
{
    int exit_value;

    /*****************************************************************
    *  Initialize the data system process.
    *
    *  This function will not return if the -h (help) or -v (version)
    *  option was specified on the command line, or if an error occurs
    *  (i.e. could not connect to database, the process is not defined
    *  in the database, the log file could not be opened/created, a
    *  memory allocation error, etc...).
    *
    ******************************************************************/

    dsproc_initialize(
        argc, argv, proc_model, proc_version, nproc_names, proc_names);

    /*****************************************************************
    *  Call the user's init_process() function.
    *
    *  The _dsproc_run_init_process_hook() function will call the
    *  user's init_process() function if one was set using the
    *  dsproc_set_init_process_hook() function.
    *
    *  When writing bindings for other languages the target language
    *  must implement its own methods for setting and calling the
    *  various user defined hook functions, and for storing a reference
    *  to the user defined data structure.
    *
    ******************************************************************/

    if (!_dsproc_run_init_process_hook()) {
        exit_value = dsproc_finish();
        return(exit_value);
    }

    /*****************************************************************
    *  Disconnect from the database until it is needed again
    ******************************************************************/

    dsproc_db_disconnect();

    /*****************************************************************
    *  Call the appropriate data processing loop
    ******************************************************************/

    if (proc_model == PM_INGEST) {
        _dsproc_ingest_main_loop();
    }
    else {
        _dsproc_vap_main_loop(proc_model);
    }

    /*****************************************************************
    *  Call the user's finish_process() function.
    *
    *  The _dsproc_run_finish_process_hook() function will call the
    *  user's finish_process() function if one was set using the
    *  dsproc_set_finish_process_hook() function.
    *
    *  When writing bindings for other languages the target language
    *  must implement its own methods for setting and calling the
    *  various user defined hook functions, and for storing a reference
    *  to the user defined data structure.
    *
    ******************************************************************/

    _dsproc_run_finish_process_hook();

    /*****************************************************************
    *  Finish the data system process.
    *
    *  This function will update the database and logs with process
    *  status and metrics, close the logs, send any generated mail
    *  messages, and cleanup all allocated memory.
    *
    ******************************************************************/

    exit_value = dsproc_finish();

    return(exit_value);
}

/**
 *  Get quicklook hook mode.
 *
 *  @return  mode - quicklook modes:
 *                    - QUICKLOOK_NORMAL  - run the quicklook function normally
 *                    - QUICKLOOK_ONLY    - only run quicklook function
 *                    - QUICKLOOK_DISABLE - do not run the quicklook function
 */
int dsproc_get_quicklook_mode(void)
{
    return(_QuicklookMode);
}

/**
 *  Set quicklook hook mode.
 *
 *  @param  mode - quicklook modes:
 *                    - QUICKLOOK_NORMAL  - run the quicklook function normally
 *                    - QUICKLOOK_ONLY    - only run quicklook function
 *                    - QUICKLOOK_DISABLE - do not run the quicklook function
 */
void dsproc_set_quicklook_mode(int mode)
{
    _QuicklookMode = mode;

    if (msngr_debug_level || msngr_provenance_level) {
        if (_QuicklookMode == QUICKLOOK_NORMAL) {
            DEBUG_LV1( DSPROC_LIB_NAME,
                "Setting quicklook mode to: QUICKLOOK_NORMAL\n");
        }
        else if (_QuicklookMode == QUICKLOOK_ONLY) {
            DEBUG_LV1( DSPROC_LIB_NAME,
                "Setting quicklook mode to: QUICKLOOK_ONLY\n");
        }
        else if (_QuicklookMode == QUICKLOOK_DISABLE) {
            DEBUG_LV1( DSPROC_LIB_NAME,
                "Setting quicklook mode to: QUICKLOOK_DISABLE\n");
        }
    }
}
