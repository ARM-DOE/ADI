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
*    $Revision: 8058 $
*    $Author: ermold $
*    $Date: 2011-09-22 03:14:35 +0000 (Thu, 22 Sep 2011) $
*    $Version: $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file dqrdb.c
 *  Data Quality Report Database Interface.
 */

#include <unistd.h>

#include "dsdb3.h"

/**
 *  @defgroup DQRDB DQR Database
 */
/*@{*/

/*******************************************************************************
 *  Private Functions
 */
/** @privatesection */

static void _dqrdb_destroy_dqr(DQR *dqr)
{
    if (dqr) {
        if (dqr->id)        free((void *)dqr->id);
        if (dqr->desc)      free((void *)dqr->desc);
        if (dqr->ds_name)   free((void *)dqr->ds_name);
        if (dqr->var_name)  free((void *)dqr->var_name);
        if (dqr->color)     free((void *)dqr->color);
        if (dqr->code_desc) free((void *)dqr->code_desc);
        free(dqr);
    }
}

static DQR *_dqrdb_create_dqr(
    DQRDB       *dqrdb,
    const char  *id,
    const char  *desc,
    const char  *ds_name,
    const char  *var_name,
    const char  *code,
    const char  *color,
    const char  *code_desc,
    const char  *start,
    const char  *end)
{
    DQR *dqr = (DQR *)calloc(1, sizeof(DQR));

    if (!dqr) {
        return((DQR *)NULL);
    }

    if (!id)        id        = "NULL";
    if (!desc)      desc      = "NULL";
    if (!ds_name)   ds_name   = "NULL";
    if (!var_name)  var_name  = "NULL";
    if (!color)     color     = "NULL";
    if (!code_desc) code_desc = "NULL";

    if (!(dqr->id        = strdup(id))       ||
        !(dqr->desc      = strdup(desc))     ||
        !(dqr->ds_name   = strdup(ds_name))  ||
        !(dqr->var_name  = strdup(var_name)) ||
        !(dqr->color     = strdup(color))    ||
        !(dqr->code_desc = strdup(code_desc))) {

        _dqrdb_destroy_dqr(dqr);
        return((DQR *)NULL);
    }

    if (code) {
        dqr->code = atoi(code);
    }
    else {
        dqr->code = 0;
    }

    if (start) {
        dbconn_text_to_time(dqrdb->dbconn, start, &dqr->start);
    }
    else {
        dqr->start = 0;
    }

    if (end) {
        dbconn_text_to_time(dqrdb->dbconn, end, &dqr->end);
    }
    else {
        dqr->end = 0;
    }

    return(dqr);
}

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Create a new DQRDB connection.
 *
 *  This function will first check the current working directory
 *  and then the users home directory for the .db_connect file.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  db_alias - database connection alias from the .db_connect file
 *                     (default: dqrdb)
 *
 *  @return
 *    - pointer to the database connection
 *    - NULL if an error occurred
 */
DQRDB *dqrdb_create(const char *db_alias)
{
    DQRDB *dqrdb;

    /* Allocate memory for the new DQRDB struct */

    dqrdb = (DQRDB *)calloc(1, sizeof(DQRDB));
    if (!dqrdb) {

        ERROR( DSDB_LIB_NAME,
            "Could not create DQRDB connection\n"
            " -> memory allocation error\n");

        return((DQRDB *)NULL);
    }

    /* Create the database connection */

    if (!db_alias) {
        db_alias = "dqrdb";
    }

    dqrdb->dbconn = dbconn_create(db_alias);

    if (!dqrdb->dbconn) {
        free(dqrdb);
        return((DQRDB *)NULL);
    }

    /* Set default connection retry values */

    dqrdb->max_retries    = 12;
    dqrdb->retry_interval = 5;

    return(dqrdb);
}

/**
 *  Destroy a DQRDB connection.
 *
 *  This function will close the database connection and
 *  free all memory associated with the DQRDB structure.
 *
 *  @param  dqrdb - pointer to the database connection
 */
void dqrdb_destroy(DQRDB *dqrdb)
{
    if (dqrdb) {
        if (dqrdb->dbconn) dbconn_destroy(dqrdb->dbconn);
        free(dqrdb);
    }
}

/**
 *  Connect to the DQRDB.
 *
 *  If the database connection has already been opened, this function
 *  will only increment the connection counter. This allows nested
 *  functions to repeatedly call this function without actually
 *  reconnecting to the database.
 *
 *  To insure the database connection is not held open longer than
 *  necessary it is important that every call to dqrdb_connect() is
 *  followed by a call to dqrdb_disconnect().
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  dqrdb - pointer to the database connection
 *
 *  @return
 *    - the number of attempts it took to connect to the database
 *    - 0 if an error occurred
 *
 *  @see  dqrdb_disconnect()
 */
int dqrdb_connect(DQRDB *dqrdb)
{
    int attempts;

    /* Check if the database connection is already open */

    if (dqrdb->nreconnect) {

        if (dbconn_is_connected(dqrdb->dbconn)) {
            dqrdb->nreconnect++;
            return(1);
        }
        else if (dbconn_reset(dqrdb->dbconn) == DB_NO_ERROR) {
            dqrdb->nreconnect++;
            return(1);
        }
    }

    /* Connect to the database */

    for (attempts = 1;; attempts++) {

        if (dbconn_connect(dqrdb->dbconn) == DB_NO_ERROR) {
            break;
        }
        else if (attempts > dqrdb->max_retries) {

            ERROR( DSDB_LIB_NAME,
                "Could not connect to DQRDB\n"
                " -> exceeded maximum number of retry attempts: %d\n",
                dqrdb->max_retries);

            return(0);
        }

        sleep(dqrdb->retry_interval);
    }

    dqrdb->nreconnect++;

    return(attempts);
}

/**
 *  Disconnect from the database.
 *
 *  This function will only decrement the connection counter until it
 *  reaches 0. Once the connection counter reaches zero the database
 *  connection will be closed.
 *
 *  To insure the database connection is not held open longer than
 *  necessary it is important that every call to dqrdb_connect() is
 *  followed by a call to dqrdb_disconnect().
 *
 *  @param  dqrdb - pointer to the database connection
 *
 *  @see dqrdb_connect()
 */
void dqrdb_disconnect(DQRDB *dqrdb)
{
    if (dqrdb->nreconnect > 0) {

        dqrdb->nreconnect--;

        if (dqrdb->nreconnect == 0) {
            dbconn_disconnect(dqrdb->dbconn);
        }
    }
}

/**
 *  Check the database connection.
 *
 *  @param  dqrdb - pointer to the database connection
 *
 *  @return
 *    - 1 if connected
 *    - 0 if not connected
 */
int dqrdb_is_connected(DQRDB *dqrdb)
{
    return(dbconn_is_connected(dqrdb->dbconn));
}

/**
 *  Set the maximum number of times to retry a failed database connection.
 *
 *  If this value is not set the default value of 5 will be used.
 *
 *  @param  dqrdb       - pointer to the database connection
 *  @param  max_retries - number of times to retry a database connection
 */
void dqrdb_set_max_retries(DQRDB *dqrdb, int max_retries)
{
    dqrdb->max_retries = max_retries;
}

/**
 *  Set the retry interval for a failed database connection.
 *
 *  If this value is not set the default value of 5 will be used.
 *
 *  @param  dqrdb          - pointer to the database connection
 *  @param  retry_interval - retry interval in seconds
 */
void dqrdb_set_retry_interval(DQRDB *dqrdb, int retry_interval)
{
    dqrdb->retry_interval = retry_interval;
}

/**
 *  Free all memory used by an array of pointers to DQR structures.
 *
 *  @param  dqrs - pointer to the array of pointers to DQR structures
 */
void dqrdb_free_dqrs(DQR **dqrs)
{
    int i;

    if (dqrs) {
        for (i = 0; dqrs[i]; i++) {
            _dqrdb_destroy_dqr(dqrs[i]);
        }
        free(dqrs);
    }
}

/**
 *  Get the DQRs for a datastream.
 *
 *  The memory used by the output array is dynamically allocated.
 *  It is the responsibility of the calling process to free this memory
 *  when it is no longer needed (see dqrdb_free_dqrs()).
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  Null results from the database are not reported as errors.
 *  It is the responsibility of the calling process to report
 *  these as errors if necessary.
 *
 *  @param  dqrdb      - pointer to the database connection
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
int dqrdb_get_dqrs(
    DQRDB      *dqrdb,
    const char *site,
    const char *facility,
    const char *dsc_name,
    const char *dsc_level,
    const char *var_name,
    time_t      start_time,
    time_t      end_time,
    DQR      ***dqrs)
{
    const char *command = "SELECT * FROM get_dqrs($1,$2,$3,$4,$5,$6,$7)";
    const char *params[7];
    char        start[32];
    char        end[32];
    DBStatus    status;
    DBResult   *dbres;
    int         ndqrs;
    int         row;

    ndqrs = 0;
    *dqrs = (DQR **)NULL;

    params[0] = dsc_name;
    params[1] = dsc_level;
    params[2] = site;
    params[3] = facility;
    params[4] = var_name;

    if (start_time) {
        params[5] = dbconn_time_to_text(dqrdb->dbconn, start_time, start);
        if (!params[5]) {
            return(-1);
        }
    }
    else {
        params[5] = (const char *)NULL;
    }

    if (end_time) {
        params[6] = dbconn_time_to_text(dqrdb->dbconn, end_time, end);
        if (!params[6]) {
            return(-1);
        }
    }
    else {
        params[6] = (const char *)NULL;
    }

    status = dbconn_query(dqrdb->dbconn, command, 7, params, &dbres);

    if (status == DB_NO_ERROR) {

        *dqrs = (DQR **)calloc(dbres->nrows + 1, sizeof(DQR *));
        if (!*dqrs) {

            ERROR( DSDB_LIB_NAME,
                "Could not get list of DQRs for: %s%s%s.%s:%s\n"
                " -> memory allocation error\n",
                site, dsc_name, facility, dsc_level, var_name);

            dbres->free(dbres);
            return(-1);
        }

        for (row = 0; row < dbres->nrows; row++) {

            (*dqrs)[row] = _dqrdb_create_dqr(
                dqrdb,
                DB_RESULT(dbres,row,0),
                DB_RESULT(dbres,row,1),
                DB_RESULT(dbres,row,2),
                DB_RESULT(dbres,row,3),
                DB_RESULT(dbres,row,4),
                DB_RESULT(dbres,row,5),
                DB_RESULT(dbres,row,6),
                DB_RESULT(dbres,row,7),
                DB_RESULT(dbres,row,8));

            if (!(*dqrs)[row]) {

                ERROR( DSDB_LIB_NAME,
                    "Could not get list of DQRs for: %s%s%s.%s:%s\n"
                    " -> memory allocation error\n",
                    site, dsc_name, facility, dsc_level, var_name);

                dqrdb_free_dqrs(*dqrs);
                *dqrs = (DQR **)NULL;

                dbres->free(dbres);
                return(-1);
            }

            ndqrs++;
        }

        (*dqrs)[dbres->nrows] = (DQR *)NULL;

        dbres->free(dbres);
        return(ndqrs);
    }
    else if (status == DB_NULL_RESULT) {
        return(0);
    }

    return(-1);
}

/**
 *  Print an arrary of DQRs.
 *
 *  @param  fp    - pointer to the output stream
 *  @param  dqrdb - pointer to the database connection
 *  @param  dqrs  - pointer to the array of DQR structures
 */
void dqrdb_print_dqrs(FILE *fp, DQRDB *dqrdb, DQR **dqrs)
{
    int  id_width    = 0;
    int  ds_width    = 0;
    int  var_width   = 0;
    int  color_width = 0;
    int  desc_width  = 0;
    int  length;
    char format1[64];
    char format2[64];
    char ts1[64];
    char ts2[64];
    int  i;

    for (i = 0; dqrs[i]; i++) {

        if (dqrs[i]->id) {
            length = strlen(dqrs[i]->id);
            if (id_width < length) { id_width = length; }
        }

        if (dqrs[i]->ds_name) {
            length = strlen(dqrs[i]->ds_name);
            if (ds_width < length) { ds_width = length; }
        }

        if (dqrs[i]->var_name) {
            length = strlen(dqrs[i]->var_name);
            if (var_width < length) { var_width = length; }
        }

        if (dqrs[i]->color) {
            length = strlen(dqrs[i]->color);
            if (color_width < length) { color_width = length; }
        }

        if (dqrs[i]->code_desc) {
            length = strlen(dqrs[i]->code_desc);
            if (desc_width < length) { desc_width = length; }
        }
    }

    sprintf(format1, "%%-%ds | %%-%ds | %%-%ds ", id_width, ds_width, var_width);
    sprintf(format2, "| %%-%ds | %%-%ds ", color_width, desc_width);

    fprintf(fp, format1, "id", "datastream", "variable");
    fprintf(fp, "| code ");
    fprintf(fp, format2, "color", "code_desc");
    fprintf(fp, "|     start time      |      end time\n");

    for (i = 0; dqrs[i]; i++) {
        fprintf(fp, format1, dqrs[i]->id, dqrs[i]->ds_name, dqrs[i]->var_name);

        if (dqrs[i]->code < 0) {
            fprintf(fp, "|  %d  ", dqrs[i]->code);
        }
        else {
            fprintf(fp, "|   %d  ", dqrs[i]->code);
        }

        fprintf(fp, format2, dqrs[i]->color, dqrs[i]->code_desc);

        fprintf(fp, "| %s | %s\n",
            dbconn_time_to_text(dqrdb->dbconn, dqrs[i]->start, ts1),
            dbconn_time_to_text(dqrdb->dbconn, dqrs[i]->end, ts2));
    }
}

/*@}*/
