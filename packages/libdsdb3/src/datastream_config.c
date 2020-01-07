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

/** @file datastream_config.c
 *  Datastream Config Functions.
 */

#include "dsdb3.h"
#include "dbog_dsdb.h"

/**
 *  @defgroup DSDB_DSCONF Datastream Config
 */
/*@{*/

/*******************************************************************************
 *  Private Functions
 */
/** @privatesection */

static void _dsdb_destroy_ds_conf(DSConf *ds_conf)
{
    if (ds_conf) {
        if (ds_conf->site)     free((void *)ds_conf->site);
        if (ds_conf->facility) free((void *)ds_conf->facility);
        if (ds_conf->name)     free((void *)ds_conf->name);
        if (ds_conf->level)    free((void *)ds_conf->level);
        if (ds_conf->key)      free((void *)ds_conf->key);
        if (ds_conf->value)    free((void *)ds_conf->value);
        free(ds_conf);
    }
}

static DSConf *_dsdb_create_ds_conf(
    const char *name,
    const char *level,
    const char *site,
    const char *facility,
    const char *key,
    const char *value)
{
    DSConf *ds_conf = (DSConf *)calloc(1, sizeof(DSConf));

    if (!ds_conf) {
        return((DSConf *)NULL);
    }

    /* Site Name */

    if (site) {
        ds_conf->site = msngr_copy_string(site);
        if (!ds_conf->site) {
            free(ds_conf);
            return((DSConf *)NULL);
        }
    }
    else {
        ds_conf->site = (char *)NULL;
    }

    /* Facility Name */

    if (facility) {
        ds_conf->facility = msngr_copy_string(facility);
        if (!ds_conf->facility) {
            _dsdb_destroy_ds_conf(ds_conf);
            return((DSConf *)NULL);
        }
    }
    else {
        ds_conf->facility = (char *)NULL;
    }

    /* Datastream Name */

    if (name) {
        ds_conf->name = msngr_copy_string(name);
        if (!ds_conf->name) {
            _dsdb_destroy_ds_conf(ds_conf);
            return((DSConf *)NULL);
        }
    }
    else {
        ds_conf->name = (char *)NULL;
    }

    /* Datastream Level */

    if (level) {
        ds_conf->level = msngr_copy_string(level);
        if (!ds_conf->level) {
            _dsdb_destroy_ds_conf(ds_conf);
            return((DSConf *)NULL);
        }
    }
    else {
        ds_conf->level = (char *)NULL;
    }

    /* Config Key */

    if (key) {
        ds_conf->key = msngr_copy_string(key);
        if (!ds_conf->key) {
            _dsdb_destroy_ds_conf(ds_conf);
            return((DSConf *)NULL);
        }
    }
    else {
        ds_conf->key = (char *)NULL;
    }

    /* Config Value */

    if (value) {
        ds_conf->value = msngr_copy_string(value);
        if (!ds_conf->value) {
            _dsdb_destroy_ds_conf(ds_conf);
            return((DSConf *)NULL);
        }
    }
    else {
        ds_conf->value = (char *)NULL;
    }

    return(ds_conf);
}

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Free all memory used by an array of DSConf structures.
 *
 *  @param  ds_conf - pointer to the array of DSConf structure pointers
 */
void dsdb_free_datastream_config_values(DSConf **ds_conf)
{
    int row;

    if (ds_conf) {
        for (row = 0; ds_conf[row]; row++) {
            _dsdb_destroy_ds_conf(ds_conf[row]);
        }
        free(ds_conf);
    }
}

/**
 *  Get datastream config values from the database.
 *
 *  The nature of this function requires that NULL column values in the
 *  datastream_config table will match any argument value. A SQL reqular
 *  expression can be used for the key argument.
 *
 *  The memory used by the output array is dynamically allocated.
 *  It is the responsibility of the calling process to free this memory
 *  when it is no longer needed (see dsdb_free_datastream_config_values()).
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  Null results from the database are not reported as errors.
 *  It is the responsibility of the calling process to report
 *  these as errors if necessary.
 *
 *  @param  dsdb     - pointer to the open database connection
 *  @param  site     - site name
 *  @param  facility - facility name
 *  @param  ds_name  - datastream name
 *  @param  ds_level - datastream level
 *  @param  key      - config key
 *  @param  ds_conf  - output: pointer to the NULL terminated array
 *                             of DSConf structure pointers
 *
 *  @return
 *    -  1 if successful
 *    -  0 if the database returned a NULL result
 *    - -1 if an error occurred
 *
 *  @see  dsdb_free_datastream_config_values()
 */
int dsdb_get_datastream_config_values(
    DSDB         *dsdb,
    const char   *site,
    const char   *facility,
    const char   *ds_name,
    const char   *ds_level,
    const char   *key,
    DSConf     ***ds_conf)
{
    DBStatus  status;
    DBResult *dbres;
    int       row;

    *ds_conf = (DSConf **)NULL;

    status = dsdbog_get_datastream_config_values(
        dsdb->dbconn, ds_name, ds_level, site, facility, key, &dbres);

    if (status == DB_NO_ERROR) {

        *ds_conf = (DSConf **)calloc(dbres->nrows + 1, sizeof(DSConf *));
        if (!*ds_conf) {

            ERROR( DSDB_LIB_NAME,
                "Could not get datastream config values\n"
                " -> memory allocation error\n");

            dbres->free(dbres);
            return(-1);
        }

        for (row = 0; row < dbres->nrows; row++) {

            (*ds_conf)[row] = _dsdb_create_ds_conf(
                DSConfName(dbres,row),
                DSConfLevel(dbres,row),
                DSConfSite(dbres,row),
                DSConfFac(dbres,row),
                DSConfKey(dbres,row),
                DSConfValue(dbres,row));

            if (!(*ds_conf)[row]) {

                ERROR( DSDB_LIB_NAME,
                    "Could not get datastream config values\n"
                    " -> memory allocation error\n");

                dsdb_free_datastream_config_values(*ds_conf);
                *ds_conf = (DSConf **)NULL;

                dbres->free(dbres);
                return(-1);
            }
        }

        (*ds_conf)[dbres->nrows] = (DSConf *)NULL;

        dbres->free(dbres);
        return(1);
    }
    else if (status == DB_NULL_RESULT) {
        return(0);
    }

    return(-1);
}

/*@}*/
