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
*    $Revision: 6726 $
*    $Author: ermold $
*    $Date: 2011-05-17 03:48:03 +0000 (Tue, 17 May 2011) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file process_config.c
 *  Process Config Functions.
 */

#include "dsdb3.h"
#include "dbog_dsdb.h"

/**
 *  @defgroup DSDB_PROCCONF Process Config
 */
/*@{*/

/*******************************************************************************
 *  Private Functions
 */
/** @privatesection */

static void _dsdb_destroy_proc_conf(ProcConf *proc_conf)
{
    if (proc_conf) {
        if (proc_conf->site)     free((void *)proc_conf->site);
        if (proc_conf->facility) free((void *)proc_conf->facility);
        if (proc_conf->type)     free((void *)proc_conf->type);
        if (proc_conf->name)     free((void *)proc_conf->name);
        if (proc_conf->key)      free((void *)proc_conf->key);
        if (proc_conf->value)    free((void *)proc_conf->value);
        free(proc_conf);
    }
}

static ProcConf *_dsdb_create_proc_conf(
    const char *type,
    const char *name,
    const char *site,
    const char *facility,
    const char *key,
    const char *value)
{
    ProcConf *proc_conf = (ProcConf *)calloc(1, sizeof(ProcConf));

    if (!proc_conf) {
        return((ProcConf *)NULL);
    }

    /* Site Name */

    if (site) {
        proc_conf->site = msngr_copy_string(site);
        if (!proc_conf->site) {
            free(proc_conf);
            return((ProcConf *)NULL);
        }
    }
    else {
        proc_conf->site = (char *)NULL;
    }

    /* Facility Name */

    if (facility) {
        proc_conf->facility = msngr_copy_string(facility);
        if (!proc_conf->facility) {
            _dsdb_destroy_proc_conf(proc_conf);
            return((ProcConf *)NULL);
        }
    }
    else {
        proc_conf->facility = (char *)NULL;
    }

    /* Process Type */

    if (type) {
        proc_conf->type = msngr_copy_string(type);
        if (!proc_conf->type) {
            _dsdb_destroy_proc_conf(proc_conf);
            return((ProcConf *)NULL);
        }
    }
    else {
        proc_conf->type = (char *)NULL;
    }

    /* Process Name */

    if (name) {
        proc_conf->name = msngr_copy_string(name);
        if (!proc_conf->name) {
            _dsdb_destroy_proc_conf(proc_conf);
            return((ProcConf *)NULL);
        }
    }
    else {
        proc_conf->name = (char *)NULL;
    }

    /* Config Key */

    if (key) {
        proc_conf->key = msngr_copy_string(key);
        if (!proc_conf->key) {
            _dsdb_destroy_proc_conf(proc_conf);
            return((ProcConf *)NULL);
        }
    }
    else {
        proc_conf->key = (char *)NULL;
    }

    /* Config Value */

    if (value) {
        proc_conf->value = msngr_copy_string(value);
        if (!proc_conf->value) {
            _dsdb_destroy_proc_conf(proc_conf);
            return((ProcConf *)NULL);
        }
    }
    else {
        proc_conf->value = (char *)NULL;
    }

    return(proc_conf);
}

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Free all memory used by an array of ProcConf structures.
 *
 *  @param  proc_conf - pointer to the array of ProcConf structure pointers
 */
void dsdb_free_process_config_values(ProcConf **proc_conf)
{
    int row;

    if (proc_conf) {
        for (row = 0; proc_conf[row]; row++) {
            _dsdb_destroy_proc_conf(proc_conf[row]);
        }
        free(proc_conf);
    }
}

/**
 *  Get process config values from the database.
 *
 *  The nature of this function requires that NULL column values in the
 *  process_config table will match any argument value. A SQL reqular
 *  expression can be used for the key argument.
 *
 *  The memory used by the output array is dynamically allocated.
 *  It is the responsibility of the calling process to free this memory
 *  when it is no longer needed (see dsdb_free_process_config_values()).
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  Null results from the database are not reported as errors.
 *  It is the responsibility of the calling process to report
 *  these as errors if necessary.
 *
 *  @param  dsdb      - pointer to the open database connection
 *  @param  site      - site name
 *  @param  facility  - facility name
 *  @param  proc_type - process type
 *  @param  proc_name - process name
 *  @param  key       - config key
 *  @param  proc_conf - output: pointer to the NULL terminated array
 *                              of ProcConf structure pointers
 *
 *  @return
 *    -  1 if successful
 *    -  0 if the database returned a NULL result
 *    - -1 if an error occurred
 *
 *  @see  dsdb_free_process_config_values()
 */
int dsdb_get_process_config_values(
    DSDB         *dsdb,
    const char   *site,
    const char   *facility,
    const char   *proc_type,
    const char   *proc_name,
    const char   *key,
    ProcConf   ***proc_conf)
{
    DBStatus  status;
    DBResult *dbres;
    int       row;

    *proc_conf = (ProcConf **)NULL;
    
    status = dsdbog_get_process_config_values(
        dsdb->dbconn, proc_type, proc_name, site, facility, key, &dbres);

    if (status == DB_NO_ERROR) {

        *proc_conf = (ProcConf **)calloc(dbres->nrows + 1, sizeof(ProcConf *));
        if (!*proc_conf) {

            ERROR( DSDB_LIB_NAME,
                "Could not get process config values\n"
                " -> memory allocation error\n");

            dbres->free(dbres);
            return(-1);
        }

        for (row = 0; row < dbres->nrows; row++) {

            (*proc_conf)[row] = _dsdb_create_proc_conf(
                ProcConfType(dbres,row),
                ProcConfName(dbres,row),
                ProcConfSite(dbres,row),
                ProcConfFac(dbres,row),
                ProcConfKey(dbres,row),
                ProcConfValue(dbres,row));

            if (!(*proc_conf)[row]) {

                ERROR( DSDB_LIB_NAME,
                    "Could not get process config values\n"
                    " -> memory allocation error\n");

                dsdb_free_process_config_values(*proc_conf);
                *proc_conf = (ProcConf **)NULL;

                dbres->free(dbres);
                return(-1);
            }
        }

        (*proc_conf)[dbres->nrows] = (ProcConf *)NULL;

        dbres->free(dbres);
        return(1);
    }
    else if (status == DB_NULL_RESULT) {
        return(0);
    }

    return(-1);
}

/*@}*/
