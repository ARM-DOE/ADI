/*******************************************************************************
*
*  COPYRIGHT (C) 2011 Battelle Memorial Institute.  All Rights Reserved.
*
********************************************************************************
*
*  Author:
*     name:  Brian Ermold
*     phone: (509) 375-2277
*     email: brian.ermold@pnl.gov
*
********************************************************************************
*
*  REPOSITORY INFORMATION:
*    $Revision: 57305 $
*    $Author: ermold $
*    $Date: 2014-10-06 20:26:36 +0000 (Mon, 06 Oct 2014) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file dsproc_dqrdb.c
 *  DQR Database Functions.
 */

#include "dsproc3.h"
#include "dsproc_private.h"

extern DSProc *_DSProc;       /**< Internal DSProc structure */

/** @privatesection */

/*******************************************************************************
 *  Static Functions And Data Visible Only To This Module
 */

/**
 *  Static: Get an entry in a DSVarDQRs linked list.
 *
 *  @param ds       - pointer to the DataStream.
 *  @param var_name - name of the variable.
 *
 *  @return
 *    - pointer to the DSVarDQRs structure for the specified variable.
 *    - NULL if not found
 */
static DSVarDQRs *_dsproc_get_dsvar_dqrs(
    DataStream *ds,
    const char *var_name)
{
    DSVarDQRs *dsvar_dqrs;

    for (dsvar_dqrs = ds->dsvar_dqrs;
         dsvar_dqrs;
         dsvar_dqrs = dsvar_dqrs->next) {

        if (strcmp(dsvar_dqrs->var_name,  var_name) == 0) {
            return(dsvar_dqrs);
        }
    }

    return((DSVarDQRs *)NULL);
}

/**
 *  Static: Load the DQRs for a datastream variable.
 *
 *  This function will add a new entry to the ds->dsvar_dqrs list and load
 *  all DQRs for this variable for the entire range of data processing.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param ds       - pointer to the DataStream.
 *  @param var_name - name of the variable.
 *
 *  @return
 *    - pointer to the DSVarDQRs structure for the specified variable.
 *    - NULL if an error occurred
 */
static DSVarDQRs *_dsproc_load_dsvar_dqrs(
    DataStream *ds,
    const char *var_name)
{
    DSVarDQRs *dsvar_dqrs;
    time_t     start_time;
    time_t     end_time;

    /* Create the DSVarDQRs structure for this variable. */

    dsvar_dqrs = (DSVarDQRs *)calloc(1, sizeof(DSVarDQRs));
    if (!dsvar_dqrs) {

        ERROR( DSPROC_LIB_NAME,
            "Could not get DQRs for variable: %s->%s\n"
            " -> memory allocation error\n",
            ds->name, var_name);

        dsproc_set_status(DSPROC_ENOMEM);
        return((DSVarDQRs *)NULL);
    }

    if (!(dsvar_dqrs->var_name = strdup(var_name))) {

        ERROR( DSPROC_LIB_NAME,
            "Could not get DQRs for variable: %s->%s\n"
            " -> memory allocation error\n",
            ds->name, var_name);

        dsproc_set_status(DSPROC_ENOMEM);
        free(dsvar_dqrs);
        return((DSVarDQRs *)NULL);
    }

    dsvar_dqrs->next = ds->dsvar_dqrs;
    ds->dsvar_dqrs   = dsvar_dqrs;

    /* Load all DQRs for this variable for the entire range of
     * data processing adjusted for the start and end offsets. */

    start_time = _DSProc->period_begin;
    end_time   = _DSProc->period_end;

    if (ds->ret_cache) {
        start_time -= ds->ret_cache->begin_offset;
        end_time   += ds->ret_cache->end_offset;
    }

    dsvar_dqrs->ndqrs = dsproc_get_dqrs(
        ds->site,
        ds->facility,
        ds->dsc_name,
        ds->dsc_level,
        var_name,
        start_time,
        end_time,
        &(dsvar_dqrs->dqrs));

    if (dsvar_dqrs->ndqrs < 0) {
        return((DSVarDQRs *)NULL);
    }

    return(dsvar_dqrs);
}

/**
 *  Static: Load the DQRs for all tagged variables in a dataset.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  dataset - pointer to the dataset
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
static int _dsproc_load_dataset_dqrs(CDSGroup *dataset)
{
    size_t      ds_ntimes;
    time_t     *ds_times;
    time_t      ds_start;
    time_t      ds_end;
    CDSVar     *var;
    VarTag     *tag;
    DSVarDQRs  *dsvar_dqrs;
    VarDQR     *var_dqr;
    DQR        *dqr;
    int         db_connected;
    int         di, vi;

    /* Get the sample times from the dataset */

    ds_times = dsproc_get_sample_times(dataset, 0, &ds_ntimes, NULL);

    if (!ds_times) {

        if (ds_ntimes == 0) {

            WARNING( DSPROC_LIB_NAME,
                "Could not load DQRs for dataset: %s\n"
                " -> no time data found in dataset",
                cds_get_object_path(dataset));

            return(1);
        }

        return(0);
    }

    ds_start = ds_times[0];
    ds_end   = ds_times[ds_ntimes-1];

    /* Load the DQRs for all variables in the dataset */

    db_connected = 0;

    for (vi = 0; vi < dataset->nvars; vi++) {

        var = dataset->vars[vi];
        tag = cds_get_user_data(var, "DSProcVarTag");

        /* Skip variables that have not been tagged, or do not
         * have the required information to look up the DQRs. */

        if (!tag ||
            !tag->in_ds ||
            !tag->in_var_name) {

            continue;
        }

        /* Skip variables that have already had their DQRs loaded. */

        if (tag->ndqrs != 0) {
            continue;
        }

        /* Get the DSVarDQRs structure for this variable. */

        dsvar_dqrs = _dsproc_get_dsvar_dqrs(tag->in_ds, tag->in_var_name);

        if (!dsvar_dqrs) {

            /* Connect to the database if we haven't already done so. */

            if (!db_connected) {
                if (!dsproc_dqrdb_connect()) {
                    return(0);
                }
            }

            /* Load the DQRs for this variable. */

            dsvar_dqrs = _dsproc_load_dsvar_dqrs(tag->in_ds, tag->in_var_name);

            if (!dsvar_dqrs) {
                dsproc_dqrdb_disconnect();
                return(0);
            }
        }

        /* Check if any DQRs exist for this variable */

        if (dsvar_dqrs->ndqrs <= 0) {
            tag->ndqrs = -1; /* no DQRs available */
            continue;
        }

        /* Create the VarDQR array for this variable. */

        tag->ndqrs = 0;
        tag->dqrs  = (VarDQR **)calloc(dsvar_dqrs->ndqrs + 1, sizeof(VarDQR *));
        if (!tag->dqrs) {
            goto MEMORY_ERROR;
        }

        for (di = 0; di < dsvar_dqrs->ndqrs; di++) {

            dqr = dsvar_dqrs->dqrs[di];

            /* Check if this DQR is within the range of the dataset */

            if (dqr->start > ds_end ||
                dqr->end   < ds_start) {

                continue;
            }

            /* Create the VarDQR */

            tag->dqrs[tag->ndqrs] = (VarDQR *)calloc(1, sizeof(VarDQR));
            if (!tag->dqrs[tag->ndqrs]) {
                goto MEMORY_ERROR;
            }

            var_dqr = tag->dqrs[tag->ndqrs];

            /* Set the values and pointers for the structure members */

            var_dqr->id         = dqr->id;
            var_dqr->desc       = dqr->desc;
            var_dqr->ds_name    = dqr->ds_name;
            var_dqr->var_name   = dqr->var_name;
            var_dqr->code       = dqr->code;
            var_dqr->color      = dqr->color;
            var_dqr->code_desc  = dqr->code_desc;
            var_dqr->start_time = dqr->start;
            var_dqr->end_time   = dqr->end;

            var_dqr->start_index = cds_find_time_index(
                ds_ntimes, ds_times, dqr->start, CDS_GTEQ);

            var_dqr->end_index = cds_find_time_index(
                ds_ntimes, ds_times, dqr->end, CDS_LTEQ);

            tag->ndqrs += 1;

        } /* end loop over DQRs */

        /* Check if any DQRs were found within the range of the dataset */

        if (tag->ndqrs == 0) {
            tag->ndqrs = -1; /* no DQRs available */
        }

    } /* end loop over dataset variables */

    /* Disconnect from the database if necessary */

    if (db_connected) {
        dsproc_dqrdb_disconnect();
    }

    return(1);

MEMORY_ERROR:

    ERROR( DSPROC_LIB_NAME,
        "Could not load DQRs for dataset: %s\n"
        " -> memory allocation error\n",
        dataset->name);

    dsproc_set_status(DSPROC_ENOMEM);
    return(0);
}

/*******************************************************************************
 *  Private Functions Visible Only To This Library
 */

/**
 *  Private: Free all memory used by a DSVarDQRs linked list.
 *
 *  @param dsvar_dqrs - pointer to the head of the linked list.
 */
void _dsproc_free_dsvar_dqrs(DSVarDQRs *dsvar_dqrs)
{
    DSVarDQRs *next;

    for (; dsvar_dqrs; dsvar_dqrs = next) {

        next = dsvar_dqrs->next;

        if (dsvar_dqrs->var_name) free((void *)dsvar_dqrs->var_name);
        if (dsvar_dqrs->dqrs)     dqrdb_free_dqrs(dsvar_dqrs->dqrs);

        free(dsvar_dqrs);
    }
}

/**
 *  Private:  Free all memory used by an array of pointers to VarDQR structures.
 *
 *  @param var_dqrs - pointer to the array of pointers to VarDQR structures.
 */
void _dsproc_free_var_dqrs(VarDQR **var_dqrs)
{
    int i;

    if (var_dqrs) {

        for (i = 0; var_dqrs[i]; i++) {
            free(var_dqrs[i]);
        }

        free(var_dqrs);
    }
}

/** @publicsection */

/*******************************************************************************
 *  Internal Functions Visible To The Public
 */

/**
 *  Connect to the DQR database.
 *
 *  To insure the database connection is not held open longer than necessary
 *  it is important that every call to dsproc_dqrdb_connect() is followed by a
 *  call to dsproc_dqrdb_disconnect().
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @return
 *    - 1 if connected to the database
 *    - 0 if an error occurred
 *
 *  @see dsproc_dqrdb_disconnect()
 */
int dsproc_dqrdb_connect(void)
{
    int nattempts;

    if (!_DSProc->dqrdb) {
        _DSProc->dqrdb = dqrdb_create("dqrdb");
        if (!_DSProc->dqrdb) {
            dsproc_set_status(DSPROC_EDQRDBCONNECT);
            return(0);
        }
    }

    if (msngr_debug_level || msngr_provenance_level) {

        if (!dqrdb_is_connected(_DSProc->dqrdb)) {
            DEBUG_LV1( DSPROC_LIB_NAME,
                "----- OPENING DQR DATABASE CONNECTION -----\n");
        }
    }

    nattempts = dqrdb_connect(_DSProc->dqrdb);

    if (nattempts == 0) {
        dsproc_set_status(DSPROC_EDQRDBCONNECT);
        return(0);
    }

    if (nattempts > 1) {

        LOG( DSPROC_LIB_NAME,
            "\nDQRDB_RETRIES: It took %d retries to connect to the DQR database.\n",
            nattempts);
    }

    return(1);
}

/**
 *  Disconnect from the DQR database.
 *
 *  To insure the database connection is not held open longer than necessary
 *  it is important that every call to dsproc_dqrdb_connect() is followed by a
 *  call to dsproc_dqrdb_disconnect().
 *
 *  @see dsproc_dqrdb_connect()
 */
void dsproc_dqrdb_disconnect(void)
{
    dqrdb_disconnect(_DSProc->dqrdb);

    if (msngr_debug_level || msngr_provenance_level) {

        if (!dqrdb_is_connected(_DSProc->dqrdb)) {
            DEBUG_LV1( DSPROC_LIB_NAME,
                "----- CLOSED DQR DATABASE CONNECTION ------\n");
        }
    }
}

/**
 *  Free all memory used by an array of pointers to DQR structures.
 *
 *  @param  dqrs - pointer to the array of pointers to DQR structures
 */
void dsproc_free_dqrs(DQR **dqrs)
{
    dqrdb_free_dqrs(dqrs);
}

/**
 *  Get the DQRs for a datastream variable.
 *
 *  The memory used by the output array is dynamically allocated. It is the
 *  responsibility of the calling process to free this memory when it is no
 *  longer needed (see dsproc_free_dqrs()).
 *
 *  Null results from the database are not reported as errors. It is the
 *  responsibility of the calling process to report these as errors if
 *  necessary.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  site       - site name
 *  @param  facility   - facility name
 *  @param  dsc_name   - datastream class name
 *  @param  dsc_level  - datastream class level
 *  @param  var_name   - variable name or NULL for all variables
 *  @param  start_time - start time in seconds since 1970
 *  @param  end_time   - end time in seconds since 1970
 *  @param  dqrs       - output: pointer to the array of pointers
 *                               to the DQR structures.
 *
 *  @result
 *    -  number of DQRs returned
 *    -  0 if no DQRs were found
 *    - -1 if an error occurred
 */
int dsproc_get_dqrs(
    const char *site,
    const char *facility,
    const char *dsc_name,
    const char *dsc_level,
    const char *var_name,
    time_t      start_time,
    time_t      end_time,
    DQR      ***dqrs)
{
    int ndqrs;

    *dqrs = (DQR **)NULL;

    if (!dsproc_dqrdb_connect()) {
        return(-1);
    }

    if (msngr_debug_level || msngr_provenance_level) {

        const char *d_site  = (site)      ? site      : "sss";
        const char *d_fac   = (facility)  ? facility  : "Fn";
        const char *d_name  = (dsc_name)  ? dsc_name  : "xxxxx";
        const char *d_level = (dsc_level) ? dsc_level : "xx";
        const char *d_var   = (var_name)  ? var_name  : "";
        time_t      d_end   = (end_time)  ? end_time  : time(NULL);
        char        ts1[32];
        char        ts2[32];

        DEBUG_LV1( DSPROC_LIB_NAME,
            "Getting DQRs for: %s%s%s.%s:%s\n"
            " - from: %s\n"
            " - to:   %s\n",
            d_site, d_name, d_fac, d_level, d_var,
            format_secs1970(start_time, ts1),
            format_secs1970(d_end, ts2));
    }

    ndqrs = dqrdb_get_dqrs(_DSProc->dqrdb,
        site, facility, dsc_name, dsc_level, var_name,
        start_time, end_time, dqrs);

    if (msngr_debug_level) {
        if (ndqrs > 0) {
            dqrdb_print_dqrs(stdout, _DSProc->dqrdb, *dqrs);
        }
        else if (ndqrs == 0) {
            DEBUG_LV1( DSPROC_LIB_NAME,
                " -> no DQRs found\n");
        }
    }

    dsproc_dqrdb_disconnect();

    if (ndqrs < 0) {
        dsproc_set_status(DSPROC_EDQRDBERROR);
    }

    return(ndqrs);
}

/*******************************************************************************
 *  Public Functions
 */

/**
 *  Get all available DQRs for the data stored in the specified variable.
 *
 *  The memory used by the output array belongs to the internal variable
 *  tag and must not be freed by the calling process.
 *
 *  @param  var      - pointer to the variable
 *  @param  var_dqrs - output: pointer to the array of pointers
 *                             to the VarDQR structures.
 *
 *  @return
 *    -  number of DQRs found
 *    -  0 if no DQRs where found for the relevant time range,
 *         or the variable was not explicitely requested by the
 *         user in the retriever definition.
 *    - -1 if an error occurred
 */
int dsproc_get_var_dqrs(CDSVar *var, VarDQR ***var_dqrs)
{
    VarTag *tag;

    *var_dqrs = (VarDQR **)NULL;

    /* Check if this variable has a tag with the
     * information required to look up the DQRs. */

    tag = cds_get_user_data(var, "DSProcVarTag");

    if (!tag ||
        !tag->in_ds ||
        !tag->in_var_name) {

        return(0);
    }

    /* Check if the VarDQR array for this varible has already been created. */

    if (tag->ndqrs != 0) {

        if (tag->ndqrs < 0) {
            return(0); /* no DQRs available */
        }

        *var_dqrs = tag->dqrs;
        return(tag->ndqrs);
    }

    /* Load the DQRs for all variables in this dataset. */

    if (!_dsproc_load_dataset_dqrs((CDSGroup *)var->parent)) {
        return(-1);
    }

    /* Check if any DQRs were found for this variable. */

    if (tag->ndqrs < 0) {
        return(0); /* no DQRs available */
    }

    *var_dqrs = tag->dqrs;
    return(tag->ndqrs);
}
