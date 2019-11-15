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
*    $Revision: 6424 $
*    $Author: ermold $
*    $Date: 2011-04-26 01:42:06 +0000 (Tue, 26 Apr 2011) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file dbconn_private.h
 *  Private header file for libdbconn.
 */

#ifndef _DBCONN_PRIVATE_H_
#define _DBCONN_PRIVATE_H_ 1

#include "dbconn.h"

/** @privatesection */

/**
 *  Macro for DBI cast.
 */
#define DBI(dbconn) ((_DBI *)dbconn->dbi)

/**
 *  Private Database Interface.
 */
typedef struct _DBI
{
    /**************************************************************************
     * Connection Functions
     */

    /** connect to the database */
    DBStatus (*connect)(DBConn *dbconn);

    /** disconnect from the database */
    void (*disconnect)(DBConn *dbconn);

    /** check the database connection */
    int (*is_connected)(DBConn *dbconn);

    /** reset the database connection */
    DBStatus (*reset)(DBConn *dbconn);

    /**************************************************************************
     * Command Functions
     */

    /** execute a database command that has no result */
    DBStatus (*exec)(
        DBConn      *dbconn,
        const char  *command,
        int          nparams,
        const char **params);

    /** execute a database command that returns a result */
    DBStatus (*query)(
        DBConn      *dbconn,
        const char  *command,
        int          nparams,
        const char **params,
        DBResult   **result);

    /** execute a database command that returns a boolean value */
    DBStatus (*query_bool)(
        DBConn      *dbconn,
        const char  *command,
        int          nparams,
        const char **params,
        int         *result);

    /** execute a database command that returns an integer value */
    DBStatus (*query_int)(
        DBConn      *dbconn,
        const char  *command,
        int          nparams,
        const char **params,
        int         *result);

    /** execute a database command that returns an integer value */
    DBStatus (*query_long)(
        DBConn      *dbconn,
        const char  *command,
        int          nparams,
        const char **params,
        long        *result);

    /** execute a database command that returns a real number */
    DBStatus (*query_float)(
        DBConn      *dbconn,
        const char  *command,
        int          nparams,
        const char **params,
        float       *result);

    /** execute a database command that returns a real number */
    DBStatus (*query_double)(
        DBConn      *dbconn,
        const char  *command,
        int          nparams,
        const char **params,
        double      *result);

    /** execute a stored procedure that returns a single text string */
    DBStatus (*query_text)(
        DBConn      *dbconn,
        const char  *command,
        int          nparams,
        const char **params,
        char       **result);

    /**************************************************************************
     * Utility Functions
     */

    /** convert boolean to database specific text string */
    char *(*bool_to_text)(int bval, char *text);

    /** convert database specific text string to boolean */
    int *(*text_to_bool)(const char *text, int *bval);

    /** convert seconds since 1970 to database specific time string */
    char *(*time_to_text)(time_t time, char *text);

    /** convert database specific time string to seconds since 1970 */
    time_t *(*text_to_time)(const char *text, time_t *time);

    /** convert timeval to database specific time string */
    char *(*timeval_to_text)(const timeval_t *tval, char *text);

    /** convert database specific time string to timeval */
    timeval_t *(*text_to_timeval)(const char *text, timeval_t *tval);

} _DBI;

#endif /* _DBCONN_PRIVATE_H_ */
