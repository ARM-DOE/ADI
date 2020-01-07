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

/** @file dbconn_sqlite.h
 *  Header file for sqlite backend.
 */

#ifndef _DBCONN_sqlite_H_
#define _DBCONN_sqlite_H_ 1

#include "dbconn.h"

#include "sqlite3.h"

/***************************************
* Connection Functions
*/

DBStatus    sqlite_connect(DBConn *dbconn);
void        sqlite_disconnect(DBConn *dbconn);
int         sqlite_is_connected(DBConn *dbconn);
DBStatus    sqlite_reset(DBConn *dbconn);

/***************************************
* Command Functions
*/

DBStatus    sqlite_exec(
                DBConn      *dbconn,
                const char  *command,
                int          nparams,
                const char **params);

DBStatus    sqlite_query(
                DBConn      *dbconn,
                const char  *command,
                int          nparams,
                const char **params,
                DBResult   **result);

DBStatus    sqlite_query_bool(
                DBConn      *dbconn,
                const char  *command,
                int          nparams,
                const char **params,
                int         *result);

DBStatus    sqlite_query_int(
                DBConn      *dbconn,
                const char  *command,
                int          nparams,
                const char **params,
                int         *result);

DBStatus    sqlite_query_long(
                DBConn      *dbconn,
                const char  *command,
                int          nparams,
                const char **params,
                long        *result);

DBStatus    sqlite_query_float(
                DBConn      *dbconn,
                const char  *command,
                int          nparams,
                const char **params,
                float       *result);

DBStatus    sqlite_query_double(
                DBConn      *dbconn,
                const char  *command,
                int          nparams,
                const char **params,
                double      *result);

DBStatus    sqlite_query_text(
                DBConn      *dbconn,
                const char  *command,
                int          nparams,
                const char **params,
                char       **result);

/***************************************
*  Utility Functions
*/

char       *sqlite_bool_to_text(int bval, char *text);
int        *sqlite_text_to_bool(const char *text, int *bval);

char       *sqlite_time_to_text(time_t time, char *text);
time_t     *sqlite_text_to_time(const char *text, time_t *time);

char       *sqlite_timeval_to_text(const timeval_t *tval, char *text);
timeval_t  *sqlite_text_to_timeval(const char *text, timeval_t *tval);

#endif /* _DBCONN_sqlite_H_ */
