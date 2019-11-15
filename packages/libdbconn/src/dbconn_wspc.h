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
*    $Revision: 6425 $
*    $Author: ermold $
*    $Date: 2011-04-26 01:43:47 +0000 (Tue, 26 Apr 2011) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file dbconn_wspc.h
 *  Header file for Web Service Procedure Call backend.
 */

#ifndef _DBCONN_WSPC_H_
#define _DBCONN_WSPC_H_ 1

#include "dbconn.h"

#include <curl/curl.h>

typedef struct CurlResult {

  size_t  buflen;
  char   *buffer;

} CurlResult;

/***************************************
* Connection Functions
*/

DBStatus    wspc_connect(DBConn *dbconn);
void        wspc_disconnect(DBConn *dbconn);
int         wspc_is_connected(DBConn *dbconn);
DBStatus    wspc_reset(DBConn *dbconn);

/***************************************
* Command Functions
*/

DBStatus    wspc_exec(
                DBConn      *dbconn,
                const char  *command,
                int          nparams,
                const char **params);

DBStatus    wspc_query(
                DBConn      *dbconn,
                const char  *command,
                int          nparams,
                const char **params,
                DBResult   **result);

DBStatus    wspc_query_bool(
                DBConn      *dbconn,
                const char  *command,
                int          nparams,
                const char **params,
                int         *result);

DBStatus    wspc_query_int(
                DBConn      *dbconn,
                const char  *command,
                int          nparams,
                const char **params,
                int         *result);

DBStatus    wspc_query_long(
                DBConn      *dbconn,
                const char  *command,
                int          nparams,
                const char **params,
                long        *result);

DBStatus    wspc_query_float(
                DBConn      *dbconn,
                const char  *command,
                int          nparams,
                const char **params,
                float       *result);

DBStatus    wspc_query_double(
                DBConn      *dbconn,
                const char  *command,
                int          nparams,
                const char **params,
                double      *result);

DBStatus    wspc_query_text(
                DBConn      *dbconn,
                const char  *command,
                int          nparams,
                const char **params,
                char       **result);

/***************************************
*  Utility Functions
*/

char       *wspc_bool_to_text(int bval, char *text);
int        *wspc_text_to_bool(const char *text, int *bval);

char       *wspc_time_to_text(time_t time, char *text);
time_t     *wspc_text_to_time(const char *text, time_t *time);

char       *wspc_timeval_to_text(const timeval_t *tval, char *text);
timeval_t  *wspc_text_to_timeval(const char *text, timeval_t *tval);

#endif /* _DBCONN_WSPC_H_ */
