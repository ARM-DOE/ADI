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
*    $Revision: 54207 $
*    $Author: ermold $
*    $Date: 2014-05-02 20:50:34 +0000 (Fri, 02 May 2014) $
*
*******************************************************************************/

/** @file dbog_dod.c
 *  DOD object group functions.
 */

#include <stdlib.h>
#include <string.h>
#include "dbconn.h"

/** @privatesection */

/*******************************************************************************
*  Highest DOD Version
*/

DBStatus dodog_get_highest_dod_version(
    DBConn      *dbconn,
    const char  *dsc_name,
    const char  *dsc_level,
    char       **result)
{
    const char *command = "SELECT * FROM get_highest_dod_version($1,$2)";
    const char *params[2];

    params[0] = dsc_name;
    params[1] = dsc_level;

    return(dbconn_query_text(dbconn, command, 2, params, result));
}

/*******************************************************************************
*  DOD Dimensions
*/

DBStatus dodog_get_dod_dims(
    DBConn      *dbconn,
    const char  *dsc_name,
    const char  *dsc_level,
    const char  *dod_version,
    DBResult   **result)
{
    const char *command = "SELECT * FROM get_dod_dims($1,$2,$3)";
    const char *params[3];

    params[0] = dsc_name;
    params[1] = dsc_level;
    params[2] = dod_version;

    return(dbconn_query(dbconn, command, 3, params, result));
}

/*******************************************************************************
*  DOD Attributes
*/

DBStatus dodog_get_dod_atts(
    DBConn      *dbconn,
    const char  *dsc_name,
    const char  *dsc_level,
    const char  *dod_version,
    DBResult   **result)
{
    const char *command = "SELECT * FROM get_dod_atts($1,$2,$3)";
    const char *params[3];

    params[0] = dsc_name;
    params[1] = dsc_level;
    params[2] = dod_version;

    return(dbconn_query(dbconn, command, 3, params, result));
}

/*******************************************************************************
*  DOD Variables
*/

DBStatus dodog_get_dod_vars(
    DBConn      *dbconn,
    const char  *dsc_name,
    const char  *dsc_level,
    const char  *dod_version,
    DBResult   **result)
{
    const char *command = "SELECT * FROM get_dod_vars($1,$2,$3)";
    const char *params[3];

    params[0] = dsc_name;
    params[1] = dsc_level;
    params[2] = dod_version;

    return(dbconn_query(dbconn, command, 3, params, result));
}

/*******************************************************************************
*  DOD Variable Dimensions
*/

DBStatus dodog_get_dod_var_dims(
    DBConn      *dbconn,
    const char  *dsc_name,
    const char  *dsc_level,
    const char  *dod_version,
    const char  *var_name,
    DBResult   **result)
{
    const char *command = "SELECT * FROM get_dod_var_dims($1,$2,$3,$4)";
    const char *params[4];

    params[0] = dsc_name;
    params[1] = dsc_level;
    params[2] = dod_version;
    params[3] = var_name;

    return(dbconn_query(dbconn, command, 4, params, result));
}

/*******************************************************************************
*  DOD Variable Attributes
*/

DBStatus dodog_get_dod_var_atts(
    DBConn      *dbconn,
    const char  *dsc_name,
    const char  *dsc_level,
    const char  *dod_version,
    const char  *var_name,
    DBResult   **result)
{
    const char *command = "SELECT * FROM get_dod_var_atts($1,$2,$3,$4)";
    const char *params[4];

    params[0] = dsc_name;
    params[1] = dsc_level;
    params[2] = dod_version;
    params[3] = var_name;

    return(dbconn_query(dbconn, command, 4, params, result));
}

/*******************************************************************************
*  Datastream DODs
*/

void dodog_free_highest_version_dbres(DBResult *dbres)
{
    if (dbres) {
        if (dbres->data) {
            if (dbres->data[0]) free(dbres->data[0]);
            if (dbres->data[1]) free(dbres->data[1]);
            free(dbres->data);
        }
        free(dbres);
    }
}

DBStatus dodog_get_ds_dod_versions(
    DBConn      *dbconn,
    const char  *site,
    const char  *facility,
    const char  *dsc_name,
    const char  *dsc_level,
    DBResult   **result)
{
    const char *command = "SELECT * FROM get_ds_dod_versions($1,$2,$3,$4)";
    const char *params[4];
    DBStatus    status;
    char       *highest_version;

    params[0] = site;
    params[1] = facility;
    params[2] = dsc_name;
    params[3] = dsc_level;

    status = dbconn_query(dbconn, command, 4, params, result);

    /* SQLite doesn't provide a way to return the highest version number for
     * the specified datastream if there are no dsdods found, so we have to
     * implement this logic here. */

    if (dbconn->db_type == DB_SQLITE &&
        status == DB_NULL_RESULT) {

        *result         = (DBResult *)NULL;
        highest_version = (char *)NULL;

        status = dodog_get_highest_dod_version(
            dbconn, dsc_name, dsc_level, &highest_version);

        if (status == DB_NO_ERROR && highest_version) { 

            *result = (DBResult *)malloc(sizeof(DBResult));
            if (!*result) return(DB_MEM_ERROR);

            (*result)->nrows   = 1;
            (*result)->ncols   = 2;
            (*result)->dbres   = (void *)NULL;
            (*result)->free    = dodog_free_highest_version_dbres;
            (*result)->data    = (char **)malloc(2 * sizeof(char *));
            (*result)->data[0] = strdup("1970-01-01 00:00:00");
            (*result)->data[1] = highest_version;
        }
    }

    return(status);
}

/*******************************************************************************
*  Datastream Attributes
*/

DBStatus dodog_get_ds_atts(
    DBConn      *dbconn,
    const char  *site,
    const char  *facility,
    const char  *dsc_name,
    const char  *dsc_level,
    DBResult   **result)
{
    const char *command = "SELECT * FROM get_ds_atts($1,$2,$3,$4)";
    const char *params[4];

    params[0] = site;
    params[1] = facility;
    params[2] = dsc_name;
    params[3] = dsc_level;

    return(dbconn_query(dbconn, command, 4, params, result));
}

/*******************************************************************************
*  Time Varying Datastream Attributes
*/

DBStatus dodog_get_ds_att_times(
    DBConn      *dbconn,
    const char  *site,
    const char  *facility,
    const char  *dsc_name,
    const char  *dsc_level,
    const char  *att_name,
    DBResult   **result)
{
    const char *command = "SELECT * FROM get_ds_att_times($1,$2,$3,$4,$5)";
    const char *params[5];

    params[0] = site;
    params[1] = facility;
    params[2] = dsc_name;
    params[3] = dsc_level;
    params[4] = att_name;

    return(dbconn_query(dbconn, command, 5, params, result));
}

DBStatus dodog_get_ds_time_atts(
    DBConn           *dbconn,
    const char       *site,
    const char       *facility,
    const char       *dsc_name,
    const char       *dsc_level,
    time_t            att_time,
    DBResult        **result)
{
    const char *command = "SELECT * FROM get_ds_time_atts($1,$2,$3,$4,$5)";
    const char *params[5];
    char        time_string[32];

    params[0] = site;
    params[1] = facility;
    params[2] = dsc_name;
    params[3] = dsc_level;

    if (att_time) {
        params[4] = dbconn_time_to_text(dbconn, att_time, time_string);
        if (!params[4]) {
            *result = (DBResult *)NULL;
            return(DB_ERROR);
        }
    }
    else {
        params[4] = (const char *)NULL;
    }

    return(dbconn_query(dbconn, command, 5, params, result));
}

/*******************************************************************************
*  Datastream Variable Attributes
*/

DBStatus dodog_get_ds_var_atts(
    DBConn      *dbconn,
    const char  *site,
    const char  *facility,
    const char  *dsc_name,
    const char  *dsc_level,
    const char  *var_name,
    DBResult   **result)
{
    const char *command = "SELECT * FROM get_ds_var_atts($1,$2,$3,$4,$5)";
    const char *params[5];

    params[0] = site;
    params[1] = facility;
    params[2] = dsc_name;
    params[3] = dsc_level;
    params[4] = var_name;

    return(dbconn_query(dbconn, command, 5, params, result));
}

/*******************************************************************************
*  Time Varying Datastream Variable Attributes
*/

DBStatus dodog_get_ds_var_att_times(
    DBConn      *dbconn,
    const char  *site,
    const char  *facility,
    const char  *dsc_name,
    const char  *dsc_level,
    const char  *var_name,
    const char  *att_name,
    DBResult   **result)
{
    const char *command = "SELECT * FROM get_ds_var_att_times($1,$2,$3,$4,$5,$6)";
    const char *params[6];

    params[0] = site;
    params[1] = facility;
    params[2] = dsc_name;
    params[3] = dsc_level;
    params[4] = var_name;
    params[5] = att_name;

    return(dbconn_query(dbconn, command, 6, params, result));
}

DBStatus dodog_get_ds_var_time_atts(
    DBConn           *dbconn,
    const char       *site,
    const char       *facility,
    const char       *dsc_name,
    const char       *dsc_level,
    const char       *var_name,
    time_t            att_time,
    DBResult        **result)
{
    const char *command = "SELECT * FROM get_ds_var_time_atts($1,$2,$3,$4,$5,$6)";
    const char *params[6];
    char        time_string[32];

    params[0] = site;
    params[1] = facility;
    params[2] = dsc_name;
    params[3] = dsc_level;
    params[4] = var_name;

    if (att_time) {
        params[5] = dbconn_time_to_text(dbconn, att_time, time_string);
        if (!params[5]) {
            *result = (DBResult *)NULL;
            return(DB_ERROR);
        }
    }
    else {
        params[5] = (const char *)NULL;
    }

    return(dbconn_query(dbconn, command, 6, params, result));
}

/*******************************************************************************
*  Datastream Properties
*/

DBStatus dodog_get_ds_properties(
    DBConn     *dbconn,
    const char *dsc_name,
    const char *dsc_level,
    const char *site,
    const char *facility,
    const char *var_name,
    const char *prop_name,
    DBResult  **result)
{
    const char *command = "SELECT * FROM get_ds_properties($1,$2,$3,$4,$5,$6)";
    const char *params[6];

    params[0] = dsc_name;
    params[1] = dsc_level;
    params[2] = site;
    params[3] = facility;
    params[4] = var_name;
    params[5] = prop_name;

    return(dbconn_query(dbconn, command, 6, params, result));
}
