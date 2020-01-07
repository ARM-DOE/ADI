/*******************************************************************************
*
*  COPYRIGHT (C) 2010 Battelle Memorial Institute.  All Rights Reserved.
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
*    $Revision: 55441 $
*    $Author: ermold $
*    $Date: 2014-07-07 22:53:59 +0000 (Mon, 07 Jul 2014) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file dsproc_dsdb.c
 *  Database Functions.
 */

#include "dsproc3.h"
#include "dsproc_private.h"

extern DSProc *_DSProc;       /**< Internal DSProc structure */
extern int _DisableDBUpdates; /**< Flag indicating if DB updates are disabled */

/** @privatesection */

/*******************************************************************************
 *  Static Functions Visible Only To This Module
 */

/*******************************************************************************
 *  Private Functions Visible Only To This Library
 */

/**
 *  Private: Get the previously processed data times for an output datastream.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  ds - pointer to the DataStream structure
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int _dsproc_get_output_datastream_times(DataStream *ds)
{
    DSTimes *ds_times;
    int      status;

    DEBUG_LV1( DSPROC_LIB_NAME,
        "%s: Getting previously processed data times\n",
        ds->name);

    status = dsdb_get_process_output_ds_times(_DSProc->dsdb,
        ds->site, ds->facility, _DSProc->type, _DSProc->name,
        ds->dsc_name, ds->dsc_level, &ds_times);

    if (status > 0) {

        ds->ppdt_begin = ds_times->first;
        ds->ppdt_end   = ds_times->last;

        dsdb_free_ds_times(ds_times);
    }
    else if (status == 0) {

        ds->ppdt_begin.tv_sec  = 0;
        ds->ppdt_begin.tv_usec = 0;
        ds->ppdt_end.tv_sec    = 0;
        ds->ppdt_end.tv_usec   = 0;
    }
    else {

        ERROR( DSPROC_LIB_NAME,
            "Could not get previously processed data times for: %s\n"
            "-> database query error\n",
            ds->name);

        dsproc_set_status(DSPROC_EDBERROR);
        return(0);
    }

    if (msngr_debug_level || msngr_provenance_level) {

        char ts1[32], ts2[32];

        if (ds->ppdt_begin.tv_sec) {
            format_timeval(&(ds->ppdt_begin), ts1);
        }
        else {
            strcpy(ts1, "none");
        }

        if (ds->ppdt_end.tv_sec) {
            format_timeval(&(ds->ppdt_end), ts2);
        }
        else {
            strcpy(ts2, "none");
        }

        DEBUG_LV1( DSPROC_LIB_NAME,
            " - begin time: %s\n"
            " - end time:   %s\n", ts1, ts2);
    }

    return(1);
}

/**
 *  Store the times of all processed data in the database.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int _dsproc_store_output_datastream_times(void)
{
    DataStream *ds;
    int         retval;
    int         status;
    int         dsi;

    if (_DisableDBUpdates) {
        return(1);
    }

    /* Check if any updates are needed */

    for (dsi = 0; dsi < _DSProc->ndatastreams; dsi++) {

        ds = _DSProc->datastreams[dsi];

        if (ds->role == DSR_OUTPUT && ds->begin_time.tv_sec) {
            break;
        }
    }

    if (dsi == _DSProc->ndatastreams) {
        return(1);
    }

    /* Update the datastream times in the database */

    DEBUG_LV1( DSPROC_LIB_NAME,
        "Storing updated datastream times in database\n");

    if (!dsproc_db_connect()) {
        return(0);
    }

    retval = 1;

    for (dsi = 0; dsi < _DSProc->ndatastreams; dsi++) {

        ds = _DSProc->datastreams[dsi];

        if (ds->role != DSR_OUTPUT) {
            continue;
        }

        if (!ds->begin_time.tv_sec) {

            DEBUG_LV1( DSPROC_LIB_NAME,
                " - %s: not updated\n", ds->name);

            continue;
        }

        if (!ds->end_time.tv_sec) {
            ds->end_time = ds->begin_time;
        }

        if (msngr_debug_level || msngr_provenance_level) {

            char ts1[32], ts2[32];

            format_timeval(&(ds->begin_time), ts1);
            format_timeval(&(ds->end_time), ts2);

            DEBUG_LV1( DSPROC_LIB_NAME,
                " - %s:\n"
                "    - begin time: %s\n"
                "    - end time:   %s\n",
                ds->name, ts1, ts2);
        }

        status = dsdb_update_process_output_ds_times(_DSProc->dsdb,
            ds->site, ds->facility, _DSProc->type, _DSProc->name,
            ds->dsc_name, ds->dsc_level,
            &(ds->begin_time), &(ds->end_time));

        if (status <= 0) {

            if (status == 0) {

                ERROR( DSPROC_LIB_NAME,
                    "Could not store output datastream times for: %s\n"
                    "-> unexpected NULL result received from database query\n",
                    ds->name);
            }
            else {

                ERROR( DSPROC_LIB_NAME,
                    "Could not store output datastream times for: %s\n"
                    "-> database query error\n",
                    ds->name);
            }

            dsproc_set_status(DSPROC_EDBERROR);
            retval = 0;
        }
    }

    dsproc_db_disconnect();

    return(retval);
}

/** @publicsection */

/*******************************************************************************
 *  Internal Functions Visible To The Public
 */

/**
 *  Connect to the database.
 *
 *  To insure the database connection is not held open longer than necessary
 *  it is important that every call to dsproc_db_connect() is followed by a
 *  call to dsproc_db_disconnect().
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @return
 *    - 1 if connected to the database
 *    - 0 if an error occurred
 *
 *  @see dsproc_db_disconnect()
 */
int dsproc_db_connect(void)
{
    int nattempts;

    if (msngr_debug_level || msngr_provenance_level) {

        if (!dsdb_is_connected(_DSProc->dsdb)) {
            DEBUG_LV1( DSPROC_LIB_NAME,
                "----- OPENING DATABASE CONNECTION -----\n");
        }

    }

    nattempts = dsdb_connect(_DSProc->dsdb);

    if (nattempts == 0) {
        dsproc_set_status(DSPROC_EDBCONNECT);
        return(0);
    }

    if (nattempts > 1) {

        LOG( DSPROC_LIB_NAME,
            "\nDB_RETRIES: It took %d retries to connect to the database.\n",
            nattempts);
    }

    return(1);
}

/**
 *  Disconnect from the database.
 *
 *  To insure the database connection is not held open longer than necessary
 *  it is important that every call to dsproc_db_connect() is followed by a
 *  call to dsproc_db_disconnect().
 *
 *  @see dsproc_db_connect()
 */
void dsproc_db_disconnect(void)
{
    dsdb_disconnect(_DSProc->dsdb);

    if (msngr_debug_level || msngr_provenance_level) {

        if (!dsdb_is_connected(_DSProc->dsdb)) {
            DEBUG_LV1( DSPROC_LIB_NAME,
                "----- CLOSED DATABASE CONNECTION ------\n");
        }
    }
}

/**
 *  Get datastream properties.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  ds_id   - datastream ID
 *  @param  dsprops - output: pointer to the NULL terminated array
 *                            of DSProp structure pointers
 *
 *  @return
 *    -  number of datastream properties
 *    -  0 if the database returned a NULL result
 *    - -1 if an error occurred
 */
int dsproc_get_datastream_properties(int ds_id, DSProp ***dsprops)
{
    DataStream *ds = _DSProc->datastreams[ds_id];
    int   ndsprops;
    char *site;
    char *fac;
    char *var;
    char  ts[32];
    int   i;

    *dsprops = (DSProp **)NULL;

    /* Check if the datastream properties have already been loaded */

    if (ds->ndsprops < 0) return(0);

    if (ds->dsprops) {
        *dsprops = ds->dsprops;
        return(ds->ndsprops);
    }

    /* Get the datastream properties from the database */

    DEBUG_LV1( DSPROC_LIB_NAME,
        "%s.%s: Getting datastream properties from database\n",
        ds->dsc_name, ds->dsc_level);

    if (!dsproc_db_connect()) {
        return(-1);
    }

    ndsprops = dsdb_get_ds_properties(
        _DSProc->dsdb, ds->dsc_name, ds->dsc_level, ds->site, ds->facility,
        "%", "%", dsprops);

    dsproc_db_disconnect();

    if (ndsprops > 0) {

        ds->ndsprops = ndsprops;
        ds->dsprops  = *dsprops;

        if (msngr_debug_level || msngr_provenance_level) {

            for (i = 0; i < ndsprops; i++) {

                site = ((*dsprops)[i]->site)     ? (*dsprops)[i]->site     : "null";
                fac  = ((*dsprops)[i]->facility) ? (*dsprops)[i]->facility : "null";
                var  = ((*dsprops)[i]->var_name) ? (*dsprops)[i]->var_name : "" ;

                format_secs1970((*dsprops)[i]->time, ts);

                DEBUG_LV1( DSPROC_LIB_NAME,
                    " - %s %s\t%s:%s\t'%s'\t'%s'\n",
                    site, fac, var,
                    (*dsprops)[i]->name, ts, (*dsprops)[i]->value);
            }
        }
    }
    else if (ndsprops < 0) {
        dsproc_set_status(DSPROC_EDBERROR);
    }
    else {
        DEBUG_LV1( DSPROC_LIB_NAME,
            " - no datastream properties defined in database\n");
        ds->ndsprops = -1;
    }

    return(ndsprops);
}

/**
 *  Get a datastream property for a specified time.
 *
 *  The memory used by the output value belongs to the internal data
 *  structures and must not be freed or altered by the calling processes.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  ds_id      - datastream ID
 *  @param  var_name   - variable name (NULL for global properties)
 *  @param  prop_name  - datastream property name
 *  @param  data_time  - data time to get the propery value for.
 *  @param  prop_value - output: pointer to the datastream property value
 *
 *  @return
 *    -  1 if successful
 *    -  0 if the property was not found
 *    - -1 if an error occurred
 */
int dsproc_get_datastream_property(
    int          ds_id,
    const char  *var_name,
    const char  *prop_name,
    time_t       data_time,
    const char **prop_value)
{
    DSProp **dsprops;
    int      ndsprops;
    char    *prev_site;
    char    *site;
    char    *var;
    char    *name;
    time_t   time;
    int      ri;

    *prop_value = (const char *)NULL;

    /* Get the complete list of all datastream properties */

    ndsprops = dsproc_get_datastream_properties(ds_id, &dsprops);
    if (ndsprops <= 0) return(ndsprops);

    /* The dsprops structures are sorted by site, facility,
     * var_name, name, time in ascending order with NULLs last. */

    /* Find the first entry for the specified datastream property */

    for (ri = 0; ri < ndsprops; ri++) {

        var  = dsprops[ri]->var_name;
        name = dsprops[ri]->name;

        if ((!var && !var_name) ||
            ( var &&  var_name  && strcmp(var, var_name) == 0 )) {

            if (strcmp(name, prop_name) == 0) break;
        }
    }

    /* Check if we found an entry for the requested datastream
     * property and data time. */

    if (ri == ndsprops ||
        dsprops[ri]->time > data_time) {

        return(0);
    }

    /* Find the correct value for the specified data time */

    prev_site   = dsprops[ri]->site;
    *prop_value = dsprops[ri]->value;

    for (ri++; ri < ndsprops; ri++) {

        site = dsprops[ri]->site;
        var  = dsprops[ri]->var_name;
        name = dsprops[ri]->name;
        time = dsprops[ri]->time;

        if ((prev_site && !site)           ||
            (strcmp(name, prop_name) != 0) ||
            (time > data_time)) {

            break;
        }

        if ((!var && !var_name) ||
            ( var &&  var_name  && strcmp(var, var_name) == 0 )) {

            *prop_value = dsprops[ri]->value;
        }
        else {
            break;
        }
    }

    return(1);
}

/**
 *  Get input datastream classes.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  ds_classes - output: pointer to the NULL terminated array
 *                               of DSClass structure pointers
 *
 *  @return
 *    -  number of input datastream classes
 *    -  0 if the database returned a NULL result
 *    - -1 if an error occurred
 */
int dsproc_get_input_ds_classes(DSClass ***ds_classes)
{
    int nds_classes;
    int i;

    *ds_classes = (DSClass **)NULL;

    /* Check if the input datastream classes have already been loaded */

    if (_DSProc->dsc_inputs) {
        *ds_classes = _DSProc->dsc_inputs;
        return(_DSProc->ndsc_inputs);
    }

    /* Get the input datastream classes from the database */

    DEBUG_LV1( DSPROC_LIB_NAME,
        "Getting input datastream classes from database\n");

    if (!dsproc_db_connect()) {
        return(-1);
    }

    nds_classes = dsdb_get_process_dsc_inputs(
        _DSProc->dsdb, _DSProc->type, _DSProc->name, ds_classes);

    dsproc_db_disconnect();

    if (nds_classes > 0) {

        _DSProc->ndsc_inputs = nds_classes;
        _DSProc->dsc_inputs  = *ds_classes;

        if (msngr_debug_level || msngr_provenance_level) {

            for (i = 0; i < nds_classes; i++) {

                DEBUG_LV1( DSPROC_LIB_NAME,
                    " - %s.%s\n",
                    (*ds_classes)[i]->name,
                    (*ds_classes)[i]->level);
            }
        }
    }
    else if (nds_classes < 0) {
        dsproc_set_status(DSPROC_EDBERROR);
    }
    else {
        DEBUG_LV1( DSPROC_LIB_NAME,
            " - no input datastream classes defined in database\n");
    }

    return(nds_classes);
}

/**
 *  Get output datastream classes.
 *
 *  If an error occurs in this function it will be appended
 *  to the log and error mail messages, and the process status
 *  will be set appropriately.
 *
 *  @param  ds_classes - output: pointer to the NULL terminated array
 *                               of DSClass structure pointers
 *
 *  @return
 *    -  number of output datastream classes
 *    -  0 if the database returned a NULL result
 *    - -1 if an error occurred
 */
int dsproc_get_output_ds_classes(DSClass ***ds_classes)
{
    int nds_classes;
    int i;

    *ds_classes = (DSClass **)NULL;

    /* Check if the output datastream classes have already been loaded */

    if (_DSProc->dsc_outputs) {
        *ds_classes = _DSProc->dsc_outputs;
        return(_DSProc->ndsc_outputs);
    }

    /* Get the output datastream classes from the database */

    DEBUG_LV1( DSPROC_LIB_NAME,
        "Getting output datastream classes from database\n");

    if (!dsproc_db_connect()) {
        return(-1);
    }

    nds_classes = dsdb_get_process_dsc_outputs(
        _DSProc->dsdb, _DSProc->type, _DSProc->name, ds_classes);

    dsproc_db_disconnect();

    if (nds_classes > 0) {

        _DSProc->ndsc_outputs = nds_classes;
        _DSProc->dsc_outputs  = *ds_classes;

        if (msngr_debug_level || msngr_provenance_level) {

            for (i = 0; i < nds_classes; i++) {

                DEBUG_LV1( DSPROC_LIB_NAME,
                    " - %s.%s\n",
                    (*ds_classes)[i]->name,
                    (*ds_classes)[i]->level);
            }
        }
    }
    else if (nds_classes < 0) {
        dsproc_set_status(DSPROC_EDBERROR);
    }
    else {
        DEBUG_LV1( DSPROC_LIB_NAME,
            " - no output datastream classes defined in database\n");
    }

    return(nds_classes);
}

/**
 *  Get process location.
 *
 *  The memory used by the output location structure belongs to the
 *  internal structures and must not be freed or modified by the calling
 *  process.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  proc_loc - output: pointer to the ProcLoc structure
 *
 *  @return
 *    -  1 if successful
 *    -  0 if the database returned a NULL result
 *    - -1 if an error occurred
 */
int dsproc_get_location(ProcLoc **proc_loc)
{
    int status;

    *proc_loc = (ProcLoc *)NULL;

    /* Check if the process location has already been loaded */

    if (_DSProc->location) {
        *proc_loc = _DSProc->location;
        return(1);
    }

    DEBUG_LV1( DSPROC_LIB_NAME,
        "Getting process location from database\n");

    /* Get the process location from the database */

    if (!dsproc_db_connect()) {
        return(-1);
    }

    status = dsdb_get_process_location(_DSProc->dsdb,
        _DSProc->site, _DSProc->facility, _DSProc->type, _DSProc->name,
        proc_loc);

    dsproc_db_disconnect();

    if (status == 1) {

        if ((*proc_loc)->lat == 0 &&
            (*proc_loc)->lon == 0 &&
            (*proc_loc)->alt == 0) {

            (*proc_loc)->lat = -9999.0;
            (*proc_loc)->lon = -9999.0;
            (*proc_loc)->alt = -9999.0;
        }

        if ((*proc_loc)->lat < -9900) (*proc_loc)->lat = -9999.0;
        if ((*proc_loc)->lon < -9900) (*proc_loc)->lon = -9999.0;
        if ((*proc_loc)->alt < -9900) (*proc_loc)->alt = -9999.0;

        _DSProc->location = *proc_loc;

        DEBUG_LV1( DSPROC_LIB_NAME,
            " - name: %s\n"
            " - lat:  %g N\n"
            " - lon:  %g E\n"
            " - alt:  %g MSL\n",
            (*proc_loc)->name,
            (*proc_loc)->lat,
            (*proc_loc)->lon,
            (*proc_loc)->alt);
    }
    else if (status < 0) {
        dsproc_set_status(DSPROC_EDBERROR);
    }
    else {
        DEBUG_LV1( DSPROC_LIB_NAME,
            " - process location not defined in database\n");
    }

    return(status);
}

/**
 *  Get site description.
 *
 *  The memory used by the output site description string belongs to the
 *  internal structures and must not be freed or modified by the calling
 *  process.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  site_desc - output: pointer to the site description
 *
 *  @return
 *    -  1 if successful
 *    -  0 if the database returned a NULL result
 *    - -1 if an error occurred
 */
int dsproc_get_site_description(const char **site_desc)
{
    int   status;
    char *description;

    *site_desc = (const char *)NULL;

    /* Check if the site description has already been loaded */

    if (_DSProc->site_desc) {
        *site_desc = _DSProc->site_desc;
        return(1);
    }

    /* Get the site description from the database */

    DEBUG_LV1( DSPROC_LIB_NAME,
        "Getting site description from database\n");

    if (!dsproc_db_connect()) {
        return(-1);
    }

    status = dsdb_get_site_description(_DSProc->dsdb,
        _DSProc->site, &description);

    dsproc_db_disconnect();

    if (status == 1) {

        _DSProc->site_desc = description;
        *site_desc = _DSProc->site_desc;

        DEBUG_LV1( DSPROC_LIB_NAME,
            " - \"%s\"\n", *site_desc);
    }
    else if (status < 0) {
        dsproc_set_status(DSPROC_EDBERROR);
    }
    else {
        DEBUG_LV1( DSPROC_LIB_NAME,
            " - site description not defined in database\n");
    }

    return(status);
}

/**
 *  Get process config value.
 *
 *  The memory used by the output config_value is dynamically allocated.
 *  It is the responsibility of the calling process to free this memory
 *  when it is no longer needed.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  config_key   - datastream class name
 *  @param  config_value - output: pointer to the config value
 *
 *  @return
 *    -  1 if successful
 *    -  0 if the database returned a NULL result
 *    - -1 if an error occurred
 */
int dsproc_get_config_value(
    const char  *config_key,
    char       **config_value)
{
    int        status;
    ProcConf **proc_conf;

    *config_value = (char *)NULL;

    DEBUG_LV1( DSPROC_LIB_NAME,
        "Getting process configuration value\n"
        " - key:   '%s'\n", config_key);

    status = dsdb_get_process_config_values(_DSProc->dsdb,
        _DSProc->site, _DSProc->facility, _DSProc->type, _DSProc->name,
        config_key, &proc_conf);

    if (status == 1) {

        DEBUG_LV1( DSPROC_LIB_NAME,
            " - value: '%s'\n", proc_conf[0]->value);

        *config_value = msngr_copy_string(proc_conf[0]->value);

        dsdb_free_process_config_values(proc_conf);

        if (!*config_value) {

            ERROR( DSPROC_LIB_NAME,
                "Could not get process configuration value for: %s\n"
                " -> memory allocation error\n", config_key);

            dsproc_set_status(DSPROC_ENOMEM);
            return(-1);
        }
    }
    else if (status < 0) {
        dsproc_set_status(DSPROC_EDBERROR);
    }
    else {
        DEBUG_LV1( DSPROC_LIB_NAME,
            " - process configuration value not defined\n");
    }

    return(status);
}

/*******************************************************************************
 *  Public Functions
 */

