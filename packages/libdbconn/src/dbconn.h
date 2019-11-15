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
*    $Revision: 47369 $
*    $Author: vonderfecht $
*    $Date: 2013-08-23 23:09:23 +0000 (Fri, 23 Aug 2013) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file dbconn.h
 *  Header file for libdbconn.
 */

#ifndef _DBCONN_H_
#define _DBCONN_H_ 1

#include <time.h>
#include <sys/time.h>

#include "msngr.h"

/** DBCONN library name. */
#define DBCONN_LIB_NAME "libdbconn"

#ifndef _TIMEVAL_T
#define _TIMEVAL_T
/**
 *  typedef for: struct timeval.
 *
 *  Structure Members:
 *
 *    - tv_sec  - seconds
 *    - tv_usec - microseconds
 */
typedef struct timeval timeval_t;
#endif  /* _TIMEVAL_T */

/**
 *  @addtogroup DBCONN_INTERFACE */
/*@{*/

/**
 *  Database Types.
 */
typedef enum
{
    DB_PGSQL  = 1, /**< Postgres Backend */
    DB_WSPC   = 2, /**< Web Service Procedure Call Backend */
	DB_SQLITE = 3  /**< SQLite database Backend */

} DBType;

/**
 *  Database Status Values.
 */
typedef enum {
    DB_NO_ERROR    = 0,  /**< no database error               */
    DB_NULL_RESULT = 1,  /**< database returned null result   */
    DB_MEM_ERROR   = 2,  /**< memory allocation error         */
    DB_ERROR       = 3,  /**< database access error           */
    DB_BAD_RESULT  = 4   /**< database returned a bad result  */
} DBStatus;

/**
 *  Database Connection.
 */
typedef struct DBConn
{
    char       db_alias[128]; /**< alias in the .db_connect file      */
    char       db_host[128];  /**< database host name                 */
    char       db_name[128];  /**< database name                      */
    char       db_user[128];  /**< database user name                 */
    char       db_pass[128];  /**< database user password             */
    DBType     db_type;       /**< database type                      */
    void      *options;       /**< not implemented: database options  */
    void      *user_data;     /**< not implemented: user data         */
    void      *dbh;           /**< database connection                */
    void      *dbi;           /**< database interface                 */

} DBConn;

typedef struct DBResult DBResult;
/**
 *  Database Result.
 */
struct DBResult
{
    int    nrows;  /**< number of rows in the result                  */
    int    ncols;  /**< number of columns in the result               */
    char **data;   /**< array of pointers to the result data          */
    void  *dbres;  /**< pointer to database specific result structure */

    /** function used to free all memory used by a database result    */
    void (*free)(DBResult *);
};

/**
 *  Macro For Database Row/Column Result Value.
 */
#define DB_RESULT(dbres,row,col) dbres->data[row*dbres->ncols + col]

/*@}*/

/*******************************************************************************
*  Prototypes
*/

const char *dbconn_lib_version(void);

DBConn     *dbconn_create(const char *db_alias);
void        dbconn_destroy(DBConn *dbconn);

DBStatus    dbconn_connect(DBConn *dbconn);
void        dbconn_disconnect(DBConn *dbconn);
int         dbconn_is_connected(DBConn *dbconn);
DBStatus    dbconn_reset(DBConn *dbconn);

char       *dbconn_expand_command(
                const char  *command,
                int          nparams,
                const char **params);

DBStatus    dbconn_exec(
                DBConn      *dbconn,
                const char  *command,
                int          nparams,
                const char **params);

DBStatus    dbconn_query(
                DBConn      *dbconn,
                const char  *command,
                int          nparams,
                const char **params,
                DBResult   **result);

DBStatus    dbconn_query_bool(
                DBConn      *dbconn,
                const char  *command,
                int          nparams,
                const char **params,
                int         *result);

DBStatus    dbconn_query_int(
                DBConn      *dbconn,
                const char  *command,
                int          nparams,
                const char **params,
                int         *result);

DBStatus    dbconn_query_long(
                DBConn      *dbconn,
                const char  *command,
                int          nparams,
                const char **params,
                long        *result);

DBStatus    dbconn_query_float(
                DBConn      *dbconn,
                const char  *command,
                int          nparams,
                const char **params,
                float       *result);

DBStatus    dbconn_query_double(
                DBConn      *dbconn,
                const char  *command,
                int          nparams,
                const char **params,
                double      *result);

DBStatus    dbconn_query_text(
                DBConn      *dbconn,
                const char  *command,
                int          nparams,
                const char **params,
                char       **result);

char       *dbconn_bool_to_text(DBConn *dbconn, int bval, char *text);
int        *dbconn_text_to_bool(DBConn *dbconn, const char *text, int *bval);

char       *dbconn_time_to_text(
                DBConn *dbconn,
                time_t  time,
                char   *text);

time_t     *dbconn_text_to_time(
                DBConn     *dbconn,
                const char *text,
                time_t     *time);

char       *dbconn_timeval_to_text(
                DBConn          *dbconn,
                const timeval_t *tval,
                char            *text);

timeval_t  *dbconn_text_to_timeval(
                DBConn     *dbconn,
                const char *text,
                timeval_t  *tval);

#endif /* _DBCONN_H_ */
