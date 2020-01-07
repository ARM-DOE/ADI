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

/** @file dsdb.c
 *  Connection and Utility functions for libdsdb3.
 */

#include <unistd.h>

#include "dsdb3.h"

/*******************************************************************************
*  DSDB Connection Functions
*/

/**
 *  @defgroup DSDB_CONNECTION DSDB Connection
 */
/*@{*/

/**
 *  Create a new database connection.
 *
 *  This function will first check the current working directory
 *  and then the users home directory for the .db_connect file.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  db_alias - database connection alias from the .db_connect file
 *
 *  @return
 *    - pointer to the database connection
 *    - NULL if an error occurred
 */
DSDB *dsdb_create(const char *db_alias)
{
    DSDB *dsdb;

    /* Allocate memory for the new DSDB struct */

    dsdb = (DSDB *)calloc(1, sizeof(DSDB));
    if (!dsdb) {

        ERROR( DSDB_LIB_NAME,
            "Could not create database connection\n"
            " -> memory allocation error\n");

        return((DSDB *)NULL);
    }

    /* Create the database connection */

    dsdb->dbconn = dbconn_create(db_alias);

    if (!dsdb->dbconn) {
        free(dsdb);
        return((DSDB *)NULL);
    }

    /* Set default connection retry values */

    dsdb->max_retries    = 12;
    dsdb->retry_interval = 5;

    return(dsdb);
}

/**
 *  Destroy a database connection.
 *
 *  This function will close the database connection and
 *  free all memory associated with the DSDB structure.
 *
 *  @param  dsdb - pointer to the database connection
 */
void dsdb_destroy(DSDB *dsdb)
{
    if (dsdb) {
        if (dsdb->dbconn) dbconn_destroy(dsdb->dbconn);
        free(dsdb);
    }
}

/**
 *  Connect to the database.
 *
 *  If the database connection has already been opened, this function
 *  will only increment the connection counter. This allows nested
 *  functions to repeatedly call this function without actually
 *  reconnecting to the database.
 *
 *  To insure the database connection is not held open longer than
 *  necessary it is important that every call to dsdb_connect() is
 *  followed by a call to dsdb_disconnect().
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  dsdb - pointer to the database connection
 *
 *  @return
 *    - the number of attempts it took to connect to the database
 *    - 0 if an error occurred
 *
 *  @see  dsdb_disconnect()
 */
int dsdb_connect(DSDB *dsdb)
{
    int attempts;

    /* Check if the database connection is already open */

    if (dsdb->nreconnect) {

        if (dbconn_is_connected(dsdb->dbconn)) {
            dsdb->nreconnect++;
            return(1);
        }
        else if (dbconn_reset(dsdb->dbconn) == DB_NO_ERROR) {
            dsdb->nreconnect++;
            return(1);
        }
    }

    /* Connect to the database */

    for (attempts = 1;; attempts++) {

        if (dbconn_connect(dsdb->dbconn) == DB_NO_ERROR) {
            break;
        }
        else if (attempts > dsdb->max_retries) {

            ERROR( DSDB_LIB_NAME,
                "%s@%s: Could not connect to database\n"
                " -> exceeded maximum number of retry attempts: %d\n",
                dsdb->dbconn->db_name, dsdb->dbconn->db_host,
                dsdb->max_retries);

            return(0);
        }

        sleep(dsdb->retry_interval);
    }

    dsdb->nreconnect++;

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
 *  necessary it is important that every call to dsdb_connect() is
 *  followed by a call to dsdb_disconnect().
 *
 *  @param  dsdb - pointer to the database connection
 *
 *  @see dsdb_connect()
 */
void dsdb_disconnect(DSDB *dsdb)
{
    if (dsdb->nreconnect > 0) {

        dsdb->nreconnect--;

        if (dsdb->nreconnect == 0) {
            dbconn_disconnect(dsdb->dbconn);
        }
    }
}

/**
 *  Check the database connection.
 *
 *  @param  dsdb - pointer to the database connection
 *
 *  @return
 *    - 1 if connected
 *    - 0 if not connected
 */
int dsdb_is_connected(DSDB *dsdb)
{
    return(dbconn_is_connected(dsdb->dbconn));
}

/**
 *  Set the maximum number of times to retry a failed database connection.
 *
 *  If this value is not set the default value of 5 will be used.
 *
 *  @param  dsdb        - pointer to the database connection
 *  @param  max_retries - number of times to retry a database connection
 */
void dsdb_set_max_retries(DSDB *dsdb, int max_retries)
{
    dsdb->max_retries = max_retries;
}

/**
 *  Set the retry interval for a failed database connection.
 *
 *  If this value is not set the default value of 5 will be used.
 *
 *  @param  dsdb           - pointer to the database connection
 *  @param  retry_interval - retry interval in seconds
 */
void dsdb_set_retry_interval(DSDB *dsdb, int retry_interval)
{
    dsdb->retry_interval = retry_interval;
}

/*@}*/

/*******************************************************************************
*  Utility Functions:
*/

/**
 *  @defgroup DSDB_UTILITIES Utility Functions
 */
/*@{*/

/**
 *  Convert a boolean value to a database specific text string.
 *
 *  This function will convert a boolean value into a string that
 *  can be used in database queries.
 *
 *  @param  dsdb - pointer to the database connection
 *  @param  bval - boolean value (1 = TRUE, 0 = FALSE)
 *  @param  text - output: text string
 *
 *  @return  the text argument
 */
char *dsdb_bool_to_text(DSDB *dsdb, int bval, char *text)
{
    return(dbconn_bool_to_text(dsdb->dbconn, bval, text));
}

/**
 *  Convert a database specific text string to a boolean value.
 *
 *  This function will convert a boolean string returned by
 *  a database query into an integer (1 = TRUE, 0 = FALSE).
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  dsdb - pointer to the database connection
 *  @param  text - text string
 *  @param  bval - output: boolean value (1 = TRUE, 0 = FALSE)
 *
 *  @return
 *    - the bval argument
 *    - NULL if the text string is not a valid boolean value
 */
int *dsdb_text_to_bool(DSDB *dsdb, const char *text, int *bval)
{
    return(dbconn_text_to_bool(dsdb->dbconn, text, bval));
}

/**
 *  Convert seconds since 1970 to a database specific time string.
 *
 *  This function will convert seconds since 1970 into a time
 *  string that can be used in database queries.
 *
 *  The text argument must point to a string that can hold
 *  at least 32 characters.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  dsdb - pointer to the database connection
 *  @param  time - seconds since 1970
 *  @param  text - output: time string
 *
 *  @return
 *    - the text argument
 *    - NULL if an error occurred
 */
char *dsdb_time_to_text(
    DSDB   *dsdb,
    time_t  time,
    char   *text)
{
    return(dbconn_time_to_text(dsdb->dbconn, time, text));
}

/**
 *  Convert a database specific time string to seconds since 1970.
 *
 *  This function will convert a time string returned by a
 *  database query into seconds since 1970.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  dsdb - pointer to the database connection
 *  @param  text - time string
 *  @param  time - output: seconds since 1970
 *
 *  @return
 *    - the time argument
 *    - NULL if an error occurred
 */
time_t *dsdb_text_to_time(
    DSDB       *dsdb,
    const char *text,
    time_t     *time)
{
    return(dbconn_text_to_time(dsdb->dbconn, text, time));
}

/**
 *  Convert a timeval to a database specific time string.
 *
 *  This function will convert a timeval into a time string
 *  that can be used in database queries.
 *
 *  The text argument must point to a string that can hold
 *  at least 32 characters.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  dsdb - pointer to the database connection
 *  @param  tval - pointer to the timeval (in seconds since 1970)
 *  @param  text - output: time string
 *
 *  @return
 *    - the text argument
 *    - NULL if an error occurred
 */
char *dsdb_timeval_to_text(
    DSDB      *dsdb,
    timeval_t *tval,
    char      *text)
{
    return(dbconn_timeval_to_text(dsdb->dbconn, tval, text));
}

/**
 *  Convert a database specific time string to a timeval.
 *
 *  This function will convert a time string returned by
 *  a database query into a timeval.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  dsdb - pointer to the database connection
 *  @param  text - time string
 *  @param  tval - output: timeval (in seconds since 1970)
 *
 *  @return
 *    - the tval argument
 *    - NULL if an error occurred
 */
timeval_t *dsdb_text_to_timeval(
    DSDB       *dsdb,
    const char *text,
    timeval_t  *tval)
{
    return(dbconn_text_to_timeval(dsdb->dbconn, text, tval));
}

/*@}*/
