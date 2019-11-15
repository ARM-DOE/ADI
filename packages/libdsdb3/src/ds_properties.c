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
*    $Revision: 54301 $
*    $Author: ermold $
*    $Date: 2014-05-13 02:28:34 +0000 (Tue, 13 May 2014) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file ds_properties.c
 *  Datastream Config Functions.
 */

#include "dsdb3.h"
#include "dbog_dod.h"

/**
 *  @defgroup DSDB_DSCONF Datastream Config
 */
/*@{*/

/*******************************************************************************
 *  Private Functions
 */
/** @privatesection */

static void _dsdb_destroy_dsprop(DSProp *dsprop)
{
    if (dsprop) {
        if (dsprop->site)      free((void *)dsprop->site);
        if (dsprop->facility)  free((void *)dsprop->facility);
        if (dsprop->dsc_name)  free((void *)dsprop->dsc_name);
        if (dsprop->dsc_level) free((void *)dsprop->dsc_level);
        if (dsprop->var_name)  free((void *)dsprop->var_name);
        if (dsprop->name)      free((void *)dsprop->name);
        if (dsprop->value)     free((void *)dsprop->value);
        free(dsprop);
    }
}

static DSProp *_dsdb_create_dsprop(
    DSDB       *dsdb,
    const char *dsc_name,
    const char *dcs_level,
    const char *site,
    const char *facility,
    const char *var_name,
    const char *name,
    const char *time,
    const char *value)
{
    DSProp *dsprop = (DSProp *)calloc(1, sizeof(DSProp));
    if (!dsprop) return((DSProp *)NULL);

    DSDB_STRDUP(dsc_name,  dsprop->dsc_name,  dsprop, (DSProp *)NULL);
    DSDB_STRDUP(dcs_level, dsprop->dsc_level, dsprop, (DSProp *)NULL);
    DSDB_STRDUP(site,      dsprop->site,      dsprop, (DSProp *)NULL);
    DSDB_STRDUP(facility,  dsprop->facility,  dsprop, (DSProp *)NULL);
    DSDB_STRDUP(var_name,  dsprop->var_name,  dsprop, (DSProp *)NULL);
    DSDB_STRDUP(name,      dsprop->name,      dsprop, (DSProp *)NULL);
    DSDB_STRDUP(value,     dsprop->value,     dsprop, (DSProp *)NULL);

    dsdb_text_to_time(dsdb, time, &(dsprop->time));

    return(dsprop);
}

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Free all memory used by an array of DSProp structures.
 *
 *  @param  dsprops - pointer to the array of DSProp structure pointers
 */
void dsdb_free_ds_properties(DSProp **dsprops)
{
    int row;

    if (dsprops) {
        for (row = 0; dsprops[row]; row++) {
            _dsdb_destroy_dsprop(dsprops[row]);
        }
        free(dsprops);
    }
}

/**
 *  Get datastream config values from the database.
 *
 *  The nature of this function requires that NULL column values in the
 *  ds_properties table will match any argument value. A SQL reqular
 *  expression can be used for the key argument.
 *
 *  The memory used by the output array is dynamically allocated.
 *  It is the responsibility of the calling process to free this memory
 *  when it is no longer needed (see dsdb_free_ds_properties()).
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  Null results from the database are not reported as errors.
 *  It is the responsibility of the calling process to report
 *  these as errors if necessary.
 *
 *  @param  dsdb      - pointer to the open database connection
 *  @param  ds_name   - datastream name
 *  @param  ds_level  - datastream level
 *  @param  site      - site name
 *  @param  facility  - facility name
 *  @param  var_name  - variable name
 *  @param  prop_name - property name
 *  @param  dsprops   - output: pointer to the NULL terminated array
 *                              of DSProp structure pointers
 *
 *  @return
 *    -  number of datastream properties
 *    -  0 if the database returned a NULL result
 *    - -1 if an error occurred
 *
 *  @see  dsdb_free_ds_properties()
 */
int dsdb_get_ds_properties(
    DSDB         *dsdb,
    const char   *ds_name,
    const char   *ds_level,
    const char   *site,
    const char   *facility,
    const char   *var_name,
    const char   *prop_name,
    DSProp     ***dsprops)
{
    DBStatus  status;
    DBResult *dbres;
    int       ndsprops;
    int       row;

    ndsprops = 0;
    *dsprops = (DSProp **)NULL;

    status = dodog_get_ds_properties(
        dsdb->dbconn, ds_name, ds_level, site, facility,
        var_name, prop_name, &dbres);

    if (status == DB_NO_ERROR) {

        *dsprops = (DSProp **)calloc(dbres->nrows + 1, sizeof(DSProp *));
        if (!*dsprops) {

            ERROR( DSDB_LIB_NAME,
                "Could not get datastream config values\n"
                " -> memory allocation error\n");

            dbres->free(dbres);
            return(-1);
        }

        for (row = 0; row < dbres->nrows; row++) {

            (*dsprops)[row] = _dsdb_create_dsprop(dsdb,
                DsPropDscName(dbres,row),
                DsPropDscLevel(dbres,row),
                DsPropSite(dbres,row),
                DsPropFac(dbres,row),
                DsPropVar(dbres,row),
                DsPropName(dbres,row),
                DsPropTime(dbres,row),
                DsPropValue(dbres,row));

            if (!(*dsprops)[row]) {

                ERROR( DSDB_LIB_NAME,
                    "Could not get datastream properties\n"
                    " -> memory allocation error\n");

                dsdb_free_ds_properties(*dsprops);
                *dsprops = (DSProp **)NULL;

                dbres->free(dbres);
                return(-1);
            }
            
            ndsprops++;
        }

        (*dsprops)[dbres->nrows] = (DSProp *)NULL;

        dbres->free(dbres);
        return(ndsprops);
    }
    else if (status == DB_NULL_RESULT) {
        return(0);
    }

    return(-1);
}

/*@}*/
