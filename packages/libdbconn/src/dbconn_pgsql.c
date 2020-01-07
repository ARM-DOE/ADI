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
*    $Revision: 65910 $
*    $Author: ermold $
*    $Date: 2015-11-17 00:12:02 +0000 (Tue, 17 Nov 2015) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file dbconn_pgsql.c
 *  Postgres backend.
 */

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include "../config.h"

#ifdef HAVE_POSTGRESQL

#include "dbconn_pgsql.h"

/**
 *  Macro for setting PGSQL Errors.
 */
#define PGSQL_ERROR(dbconn, pgconn, pgres, ...) \
    _pgsql_error(__func__, __FILE__, __LINE__, dbconn, pgconn, pgres, __VA_ARGS__)

/*******************************************************************************
 *  Private Functions
 */

static void _pgsql_error(
    const char *func,
    const char *file,
    int         line,
    DBConn     *dbconn,
    PGconn     *pgconn,
    PGresult   *pgres,
    const char *format, ...)
{
    char    *pgerr = (char *)NULL;
    char    *format_str;
    va_list  args;

    if (pgres) {
        pgerr = PQresultErrorMessage(pgres);
    }
    else if (pgconn) {
        pgerr = PQerrorMessage(pgconn);
    }

    if (pgerr && pgerr[0] != '\0') {

        if (strncmp(pgerr, "ERROR:  ", 8) == 0) {
            pgerr += 8;
        }

        if (!format) {

            msngr_send( DBCONN_LIB_NAME, func, file, line, MSNGR_ERROR,
                "%s@%s: %s\n",
                dbconn->db_name, dbconn->db_host, pgerr);

            return;
        }
    }

    if (format) {

        va_start(args, format);
        format_str = msngr_format_va_list(format, args);
        va_end(args);

        if (format_str) {

            if (pgerr && pgerr[0] != '\0') {

                msngr_send( DBCONN_LIB_NAME, func, file, line, MSNGR_ERROR,
                    "%s@%s: %s -> %s",
                    dbconn->db_name, dbconn->db_host, format_str, pgerr);
            }
            else {

                msngr_send( DBCONN_LIB_NAME, func, file, line, MSNGR_ERROR,
                    "%s@%s: %s",
                    dbconn->db_name, dbconn->db_host, format_str);
            }

            free(format_str);
        }
        else {

            msngr_send( DBCONN_LIB_NAME, func, file, line, MSNGR_ERROR,
                "%s@%s: Memory allocation error\n",
                dbconn->db_name, dbconn->db_host);
        }
    }
}

static void _pgsql_free_dbres(DBResult *dbres)
{
    if (dbres) {
        if (dbres->data) free(dbres->data);
        if (dbres->dbres) PQclear((PGresult *)dbres->dbres);
        free(dbres);
    }
}

static DBStatus _pgres_result_dbres(
    PGresult  *pgres,
    DBResult **result)
{
    DBResult *dbres;
    int       nrows;
    int       ncols;
    int       row;
    int       col;

    *result = (DBResult *)NULL;
    nrows   = PQntuples(pgres);
    ncols   = PQnfields(pgres);

    if (!nrows || !ncols) {
        PQclear(pgres);
        return(DB_NULL_RESULT);
    }

    dbres = (DBResult *)malloc(sizeof(DBResult));
    if (!dbres) {
        PQclear(pgres);
        return(DB_MEM_ERROR);
    }

    dbres->nrows = nrows;
    dbres->ncols = ncols;
    dbres->dbres = (void *)pgres;
    dbres->free  = _pgsql_free_dbres;
    dbres->data  = (char **)malloc(nrows * ncols * sizeof(char *));

    if (!dbres->data) {
        PQclear(pgres);
        free(dbres);
        return(DB_MEM_ERROR);
    }

    for (row = 0; row < nrows; row++) {
        for (col = 0; col < ncols; col++) {
            if (PQgetisnull(pgres, row, col)) {
                dbres->data[row*ncols + col] = NULL;
            }
            else {
                dbres->data[row*ncols + col] = PQgetvalue(pgres, row, col);
            }
        }
    }

    *result = dbres;

    return(DB_NO_ERROR);
}

static DBStatus _pgsql_result_bool(
    PGresult *pgres,
    int      *result)
{
    char *text;

    *result = 0;

    if (PQntuples(pgres) == 1) {

        if (PQgetisnull(pgres, 0, 0)) {
            return(DB_NULL_RESULT);
        }
        else {
            text = PQgetvalue(pgres, 0, 0);

            if (*text == 't') {
                *result = 1;
                return(DB_NO_ERROR);
            }
            else if (*text == 'f') {
                *result = 0;
                return(DB_NO_ERROR);
            }
        }
    }

    return(DB_BAD_RESULT);
}

static DBStatus _pgsql_result_long(
    PGresult *pgres,
    long     *result)
{
    char *text;
    char *endptr;
    long  value;

    *result = 0;

    if (PQntuples(pgres) == 1) {

        if (PQgetisnull(pgres, 0, 0)) {
            return(DB_NULL_RESULT);
        }
        else {
            text  = PQgetvalue(pgres, 0, 0);
            errno = 0;
            value = strtol(text, &endptr, 10);

            if (errno || text == endptr) {
                return(DB_BAD_RESULT);
            }
            else {
                *result = value;
                return(DB_NO_ERROR);
            }
        }
    }

    return(DB_BAD_RESULT);
}

static DBStatus _pgsql_result_double(
    PGresult *pgres,
    double   *result)
{
    char   *text;
    char   *endptr;
    double  value;

    *result = 0;

    if (PQntuples(pgres) == 1) {

        if (PQgetisnull(pgres, 0, 0)) {
            return(DB_NULL_RESULT);
        }
        else {
            text  = PQgetvalue(pgres, 0, 0);
            errno = 0;
            value = strtod(text, &endptr);

            if (errno || text == endptr) {
                return(DB_BAD_RESULT);
            }
            else {
                *result = value;
                return(DB_NO_ERROR);
            }
        }
    }

    return(DB_BAD_RESULT);
}

static DBStatus _pgsql_result_text(
    PGresult  *pgres,
    char     **result)
{
    char *text;
    char *value;

    *result = (char *)NULL;
    
    if (PQntuples(pgres) == 1) {

        if (PQgetisnull(pgres, 0, 0)) {
            return(DB_NULL_RESULT);
        }
        else {
            text  = PQgetvalue(pgres, 0, 0);
            value = (char *)malloc((strlen(text) + 1) * sizeof(char));

            if (value) {
                strcpy(value, text);
                *result = value;
                return(DB_NO_ERROR);
            }
            else {
                return(DB_MEM_ERROR);
            }
        }
    }

    return(DB_BAD_RESULT);
}

static int _null_row_bug(PGconn *pgconn, PGresult *pgres)
{
    char *pgbug = "ERROR:  function returning row cannot return null value\n";
    char *pgerr;

    if (pgres) {
        pgerr = PQresultErrorMessage(pgres);
    }
    else {
        pgerr = PQerrorMessage(pgconn);
    }

    if (pgerr) {
        if (strcmp(pgerr, pgbug) == 0) {
            return(1);
        }
    }

    return(0);
}

/*******************************************************************************
*  Connection Functions
*/

/**
 *  Connect to the database.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  dbconn - pointer to the database connection
 *
 *  @return database status:
 *    - DB_NO_ERROR
 *    - DB_MEM_ERROR
 *    - DB_ERROR
 *
 *  @see DBStatus
 */
DBStatus pgsql_connect(DBConn *dbconn)
{
    PGconn *pgconn;
    char    conninfo[512];
    char    host[256];
    char   *port;

    /* Make sure any previous connection has been closed */

    if (dbconn->dbh) {
        pgsql_disconnect(dbconn);
    }

    /* Build the connection info parameter string */

    strcpy(host, dbconn->db_host);

    if ( (port = strchr(host, ':')) ) {
        *port++ = '\0';
        sprintf(conninfo, "host='%s' port='%s' dbname='%s' user='%s' password='%s' ",
            host, port, dbconn->db_name, dbconn->db_user, dbconn->db_pass);
    }
    else {
        sprintf(conninfo, "host='%s' dbname='%s' user='%s' password='%s' ",
            dbconn->db_host, dbconn->db_name, dbconn->db_user, dbconn->db_pass);
    }

    /* Make a connection to the database */

    pgconn = PQconnectdb(conninfo);

    /* Check to see that the backend connection was successfully made */

    if (!pgconn) {

        PGSQL_ERROR(dbconn, NULL, NULL,
            "Memory allocation error\n");

        return(DB_MEM_ERROR);
    }
    else if (PQstatus(pgconn) != CONNECTION_OK) {

        PGSQL_ERROR(dbconn, pgconn, NULL, NULL);

        PQfinish(pgconn);
        return(DB_ERROR);
    }

    dbconn->dbh = (void *)pgconn;

    return(DB_NO_ERROR);
}

/**
 *  Disconnect from the database.
 *
 *  @param  dbconn - pointer to the database connection
 */
void pgsql_disconnect(DBConn *dbconn)
{
    PGconn *pgconn;

    if (dbconn->dbh) {

        pgconn = (PGconn *)dbconn->dbh;

        PQfinish(pgconn);

        dbconn->dbh = (void *)NULL;
    }
}

/**
 *  Check the database connection.
 *
 *  @param  dbconn - pointer to the database connection
 *
 *  @return
 *    - 1 if connected
 *    - 0 if not connected
 */
int pgsql_is_connected(DBConn *dbconn)
{
    PGconn *pgconn;

    if (dbconn->dbh) {

        pgconn = (PGconn *)dbconn->dbh;

        if (PQstatus(pgconn) != CONNECTION_OK) {
            return(0);
        }

        return(1);
    }

    return(0);
}

/**
 *  Reset the database conenction.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  dbconn - pointer to the database connection
 *
 *  @return database status:
 *    - DB_NO_ERROR
 *    - DB_MEM_ERROR
 *    - DB_ERROR
 *
 *  @see DBStatus
 */
DBStatus pgsql_reset(DBConn *dbconn)
{
    PGconn *pgconn;

    if (dbconn->dbh) {

        pgconn = (PGconn *)dbconn->dbh;

        PQreset(pgconn);

        if (PQstatus(pgconn) == CONNECTION_OK) {
            return(DB_NO_ERROR);
        }
    }

    return(pgsql_connect(dbconn));
}

/*******************************************************************************
*  Command Functions
*/

/**
 *  Execute a database command that has no result.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  dbconn  - pointer to the database connection
 *  @param  command - command string
 *  @param  nparams - number of $1, $2, ... parameters in the command
 *  @param  params  - parameters to substitute in the command
 *
 *  @return database status:
 *    - DB_NO_ERROR
 *    - DB_ERROR
 *
 *  @see DBStatus
 */
DBStatus pgsql_exec(
    DBConn      *dbconn,
    const char  *command,
    int          nparams,
    const char **params)
{
    PGconn   *pgconn = (PGconn *)dbconn->dbh;
    PGresult *pgres;
    DBStatus  status;

    if (nparams > 0) {
        pgres = PQexecParams(
            pgconn, command, nparams, NULL, params, NULL, NULL, 0);
    }
    else {
        pgres = PQexec(pgconn, command);
    }

    if (!pgres || PQresultStatus(pgres) != PGRES_COMMAND_OK) {

        PGSQL_ERROR(dbconn, pgconn, pgres,
            "FAILED: %s\n", command);

        status = DB_ERROR;
    }
    else {
        status = DB_NO_ERROR;
    }

    if (pgres) {
        PQclear(pgres);
    }

    return(status);
}

/**
 *  Execute a database command that returns a result.
 *
 *  The memory used by the database result is dynamically allocated.
 *  It is the responsibility of the calling process to free this
 *  memory using the free method of the DBResult structure.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  Null results from the database are not reported as errors.
 *  It is the responsibility of the calling process to check for
 *  DB_NULL_RESULT and report the error if necessary.
 *
 *  @param  dbconn  - pointer to the database connection
 *  @param  command - command string
 *  @param  nparams - number of $1, $2, ... parameters in the command
 *  @param  params  - parameters to substitute in the command
 *  @param  result  - output: pointer to the database result
 *
 *  @return database status:
 *    - DB_NO_ERROR
 *    - DB_NULL_RESULT
 *    - DB_MEM_ERROR
 *    - DB_ERROR
 *
 *  @see DBStatus
 */
DBStatus pgsql_query(
    DBConn      *dbconn,
    const char  *command,
    int          nparams,
    const char **params,
    DBResult   **result)
{
    PGconn   *pgconn = (PGconn *)dbconn->dbh;
    PGresult *pgres;
    DBStatus  status;

    *result = (DBResult *)NULL;

    if (nparams > 0) {
        pgres = PQexecParams(
            pgconn, command, nparams, NULL, params, NULL, NULL, 0);
    }
    else {
        pgres = PQexec(pgconn, command);
    }

    if (!pgres || PQresultStatus(pgres) != PGRES_TUPLES_OK) {

        if (_null_row_bug(pgconn, pgres)) {
            status = DB_NULL_RESULT;
        }
        else {

            PGSQL_ERROR(dbconn, pgconn, pgres,
                "FAILED: %s\n", command);

            status = DB_ERROR;
        }

        if (pgres) {
            PQclear(pgres);
        }
    }
    else {
        status = _pgres_result_dbres(pgres, result);

        if (status == DB_MEM_ERROR) {

            PGSQL_ERROR(dbconn, NULL, NULL,
                "FAILED: %s\n"
                " -> memory allocation error",
                command);
        }
    }

    return(status);
}

/**
 *  Execute a database command that returns a boolean value.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  Null results from the database are not reported as errors.
 *  It is the responsibility of the calling process to check for
 *  DB_NULL_RESULT and report the error if necessary.
 *
 *  @param  dbconn  - pointer to the database connection
 *  @param  command - command string
 *  @param  nparams - number of $1, $2, ... parameters in the command
 *  @param  params  - parameters to substitute in the command
 *  @param  result  - output: (1 = TRUE, 0 = FALSE)
 *
 *  @return database status:
 *    - DB_NO_ERROR
 *    - DB_NULL_RESULT
 *    - DB_BAD_RESULT
 *    - DB_ERROR
 *
 *  @see DBStatus
 */
DBStatus pgsql_query_bool(
    DBConn      *dbconn,
    const char  *command,
    int          nparams,
    const char **params,
    int         *result)
{
    PGconn   *pgconn = (PGconn *)dbconn->dbh;
    PGresult *pgres;
    DBStatus  status;

    *result = 0;

    if (nparams > 0) {
        pgres = PQexecParams(
            pgconn, command, nparams, NULL, params, NULL, NULL, 0);
    }
    else {
        pgres = PQexec(pgconn, command);
    }

    if (!pgres || PQresultStatus(pgres) != PGRES_TUPLES_OK) {

        PGSQL_ERROR(dbconn, pgconn, pgres,
            "FAILED: %s\n", command);

        status = DB_ERROR;
    }
    else {
        status = _pgsql_result_bool(pgres, result);

        if (status == DB_BAD_RESULT) {

            PGSQL_ERROR(dbconn, pgconn, pgres,
                "FAILED: %s\n"
                " -> bad result received from boolean query",
                command);
        }
    }

    if (pgres) {
        PQclear(pgres);
    }

    return(status);
}

/**
 *  Execute a database command that returns an integer value.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  Null results from the database are not reported as errors.
 *  It is the responsibility of the calling process to check for
 *  DB_NULL_RESULT and report the error if necessary.
 *
 *  @param  dbconn  - pointer to the database connection
 *  @param  command - command string
 *  @param  nparams - number of $1, $2, ... parameters in the command
 *  @param  params  - parameters to substitute in the command
 *  @param  result  - output: result of the database query
 *
 *  @return database status:
 *    - DB_NO_ERROR
 *    - DB_NULL_RESULT
 *    - DB_BAD_RESULT
 *    - DB_ERROR
 *
 *  @see DBStatus
 */
DBStatus pgsql_query_int(
    DBConn      *dbconn,
    const char  *command,
    int          nparams,
    const char **params,
    int         *result)
{
    DBStatus status;
    long     value;
    
    status = pgsql_query_long(dbconn, command, nparams, params, &value);

    *result = (int)value;
    
    return(status);
}

/**
 *  Execute a database command that returns an integer value.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  Null results from the database are not reported as errors.
 *  It is the responsibility of the calling process to check for
 *  DB_NULL_RESULT and report the error if necessary.
 *
 *  @param  dbconn  - pointer to the database connection
 *  @param  command - command string
 *  @param  nparams - number of $1, $2, ... parameters in the command
 *  @param  params  - parameters to substitute in the command
 *  @param  result  - output: result of the database query
 *
 *  @return database status:
 *    - DB_NO_ERROR
 *    - DB_NULL_RESULT
 *    - DB_BAD_RESULT
 *    - DB_ERROR
 *
 *  @see DBStatus
 */
DBStatus pgsql_query_long(
    DBConn      *dbconn,
    const char  *command,
    int          nparams,
    const char **params,
    long        *result)
{
    PGconn   *pgconn = (PGconn *)dbconn->dbh;
    PGresult *pgres;
    DBStatus  status;

    *result = 0;

    if (nparams > 0) {
        pgres = PQexecParams(
            pgconn, command, nparams, NULL, params, NULL, NULL, 0);
    }
    else {
        pgres = PQexec(pgconn, command);
    }

    if (!pgres || PQresultStatus(pgres) != PGRES_TUPLES_OK) {

        PGSQL_ERROR(dbconn, pgconn, pgres,
            "FAILED: %s\n", command);

        status = DB_ERROR;
    }
    else {
        status = _pgsql_result_long(pgres, result);

        if (status == DB_BAD_RESULT) {

            PGSQL_ERROR(dbconn, pgconn, pgres,
                "FAILED: %s\n"
                " -> bad result received from integer query",
                command);
        }
    }

    if (pgres) {
        PQclear(pgres);
    }

    return(status);
}

/**
 *  Execute a database command that returns a real number.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  Null results from the database are not reported as errors.
 *  It is the responsibility of the calling process to check for
 *  DB_NULL_RESULT and report the error if necessary.
 *
 *  @param  dbconn  - pointer to the database connection
 *  @param  command - command string
 *  @param  nparams - number of $1, $2, ... parameters in the command
 *  @param  params  - parameters to substitute in the command
 *  @param  result  - output: result of the database query
 *
 *  @return database status:
 *    - DB_NO_ERROR
 *    - DB_NULL_RESULT
 *    - DB_BAD_RESULT
 *    - DB_ERROR
 *
 *  @see DBStatus
 */
DBStatus pgsql_query_float(
    DBConn      *dbconn,
    const char  *command,
    int          nparams,
    const char **params,
    float       *result)
{
    DBStatus status;
    double   value;
    
    status = pgsql_query_double(dbconn, command, nparams, params, &value);
    
    *result = (float)value;
    
    return(status);
}

/**
 *  Execute a database command that returns a real number.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  Null results from the database are not reported as errors.
 *  It is the responsibility of the calling process to check for
 *  DB_NULL_RESULT and report the error if necessary.
 *
 *  @param  dbconn  - pointer to the database connection
 *  @param  command - command string
 *  @param  nparams - number of $1, $2, ... parameters in the command
 *  @param  params  - parameters to substitute in the command
 *  @param  result  - output: result of the database query
 *
 *  @return database status:
 *    - DB_NO_ERROR
 *    - DB_NULL_RESULT
 *    - DB_BAD_RESULT
 *    - DB_ERROR
 *
 *  @see DBStatus
 */
DBStatus pgsql_query_double(
    DBConn      *dbconn,
    const char  *command,
    int          nparams,
    const char **params,
    double      *result)
{
    PGconn   *pgconn = (PGconn *)dbconn->dbh;
    PGresult *pgres;
    DBStatus  status;

    *result = 0;

    if (nparams > 0) {
        pgres = PQexecParams(
            pgconn, command, nparams, NULL, params, NULL, NULL, 0);
    }
    else {
        pgres = PQexec(pgconn, command);
    }

    if (!pgres || PQresultStatus(pgres) != PGRES_TUPLES_OK) {

        PGSQL_ERROR(dbconn, pgconn, pgres,
            "FAILED: %s\n", command);

        status = DB_ERROR;
    }
    else {
        status = _pgsql_result_double(pgres, result);

        if (status == DB_BAD_RESULT) {

            PGSQL_ERROR(dbconn, pgconn, pgres,
                "FAILED: %s\n"
                " -> bad result received from real number query",
                command);
        }
    }

    if (pgres) {
        PQclear(pgres);
    }

    return(status);
}

/**
 *  Execute a database command that returns a text string.
 *
 *  The memory used by the result string is dynamically allocated.
 *  It is the responsibility of the calling process to free this
 *  memory when it is no longer needed.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  Null results from the database are not reported as errors.
 *  It is the responsibility of the calling process to check for
 *  DB_NULL_RESULT and report the error if necessary.
 *
 *  @param  dbconn  - pointer to the database connection
 *  @param  command - command string
 *  @param  nparams - number of $1, $2, ... parameters in the command
 *  @param  params  - parameters to substitute in the command
 *  @param  result  - output: pointer to the result string
 *
 *  @return database status:
 *    - DB_NO_ERROR
 *    - DB_NULL_RESULT
 *    - DB_BAD_RESULT
 *    - DB_MEM_ERROR
 *    - DB_ERROR
 *
 *  @see DBStatus
 */
DBStatus pgsql_query_text(
    DBConn      *dbconn,
    const char  *command,
    int          nparams,
    const char **params,
    char       **result)
{
    PGconn   *pgconn = (PGconn *)dbconn->dbh;
    PGresult *pgres;
    DBStatus  status;

    *result = (char *)NULL;
    
    if (nparams > 0) {
        pgres = PQexecParams(
            pgconn, command, nparams, NULL, params, NULL, NULL, 0);
    }
    else {
        pgres = PQexec(pgconn, command);
    }

    if (!pgres || PQresultStatus(pgres) != PGRES_TUPLES_OK) {

        PGSQL_ERROR(dbconn, pgconn, pgres,
            "FAILED: %s\n", command);

        status = DB_ERROR;
    }
    else {
        status = _pgsql_result_text(pgres, result);

        if (status == DB_BAD_RESULT) {

            PGSQL_ERROR(dbconn, pgconn, pgres,
                "FAILED: %s\n"
                " -> bad result received from text query",
                command);
        }
        else if (status == DB_MEM_ERROR) {

            PGSQL_ERROR(dbconn, NULL, NULL,
                "FAILED: %s\n"
                " -> memory allocation error",
                command);
        }
    }

    if (pgres) {
        PQclear(pgres);
    }

    return(status);
}

/**************************************************************************
 * Utility Functions
 */

/**
 *  Convert a boolean value to a database specific text string.
 *
 *  @param  bval - boolean value (1 = TRUE, 0 = FALSE)
 *  @param  text - output: text string
 *
 *  @return  the text argument
 */
char *pgsql_bool_to_text(int bval, char *text)
{
    if (bval) {
        strcpy(text, "t");
    }
    else {
        strcpy(text, "f");
    }

    return(text);
}

/**
 *  Convert a database specific text string to a boolean value.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  text - text string
 *  @param  bval - output: boolean value (1 = TRUE, 0 = FALSE)
 *
 *  @return
 *    - the bval argument
 *    - NULL if the text string is not a valid boolean value
 */
int *pgsql_text_to_bool(const char *text, int *bval)
{
    if (*text == 't' || *text == 'T') {
        *bval = 1;
    }
    else if (*text == 'f' || *text == 'F') {
        *bval = 0;
    }
    else {

        ERROR( DBCONN_LIB_NAME,
            "Invalid boolean text string: '%s'\n", text);

        return((int *)NULL);
    }

    return(bval);
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
 *  @param  time   - seconds since 1970
 *  @param  text   - output: time string
 *
 *  @return
 *    - the text argument
 *    - NULL if an error occurred
 */
char *pgsql_time_to_text(time_t time, char *text)
{
    struct tm tm_time;

    memset(&tm_time, 0, sizeof(struct tm));

    if (!gmtime_r(&time, &tm_time)) {

        ERROR( DBCONN_LIB_NAME,
            "Could not convert time to text: %ld\n"
            " -> gmtime error: %s\n",
            (long)time, strerror(errno));

        return((char *)NULL);
    }

    sprintf(text,
        "%d-%02d-%02d %02d:%02d:%02d",
        tm_time.tm_year + 1900,
        tm_time.tm_mon + 1,
        tm_time.tm_mday,
        tm_time.tm_hour,
        tm_time.tm_min,
        tm_time.tm_sec);

    return(text);
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
 *  @param  text   - time string
 *  @param  time   - output: seconds since 1970
 *
 *  @return
 *    - the time argument
 *    - NULL if an error occurred
 */
time_t *pgsql_text_to_time(const char *text, time_t *time)
{
    struct tm tm_time;
    int       nscanned;
    time_t    secs;

    memset((void *)&tm_time, 0, sizeof(struct tm));

    nscanned = sscanf(text,
        "%d-%d-%d %d:%d:%d",
        &(tm_time.tm_year),
        &(tm_time.tm_mon),
        &(tm_time.tm_mday),
        &(tm_time.tm_hour),
        &(tm_time.tm_min),
        &(tm_time.tm_sec));

    if (nscanned < 6) {

        ERROR( DBCONN_LIB_NAME,
            "Could not convert text to seconds since 1970: '%s'\n"
            " -> invalid time string format\n", text);

        return((time_t *)NULL);
    }

    tm_time.tm_year -= 1900;
    tm_time.tm_mon--;

    secs = mktime(&tm_time);

    if (secs == (time_t)-1) {

        ERROR( DBCONN_LIB_NAME,
            "Could not convert text to seconds since 1970: '%s'\n"
            " -> mktime error: %s\n", text, strerror(errno));

        return((time_t *)NULL);
    }

    *time = secs - timezone;

    return(time);
}

/**
 *  Convert a timeval to a database specific time string.
 *
 *  The text argument must point to a string that can hold
 *  at least 32 characters.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  tval - pointer to the timeval
 *  @param  text - output: time string
 *
 *  @return
 *    - the text argument
 *    - NULL if an error occurred
 */
char *pgsql_timeval_to_text(const timeval_t *tval, char *text)
{
    struct tm tm_time;
    time_t    secs;
    int       usec;
    char     *chrp;

    memset(&tm_time, 0, sizeof(struct tm));

    secs = tval->tv_sec + (tval->tv_usec/1000000);
    usec = tval->tv_usec % 1000000;

    if (usec < 0) {
        secs--;
        usec += 1000000;
    }

    if (!gmtime_r(&secs, &tm_time)) {

        ERROR( DBCONN_LIB_NAME,
            "Could not convert timeval to text: tv_sec = %ld, tv_usec = %ld\n"
            " -> gmtime error: %s\n",
            (long)tval->tv_sec, (long)tval->tv_usec, strerror(errno));

        return((char *)NULL);
    }

    sprintf(text,
        "%d-%02d-%02d %02d:%02d:%02d.%06d",
        tm_time.tm_year + 1900,
        tm_time.tm_mon + 1,
        tm_time.tm_mday,
        tm_time.tm_hour,
        tm_time.tm_min,
        tm_time.tm_sec,
        usec);

    for (chrp = (text + strlen(text) - 1); *chrp == '0'; chrp--) *chrp = '\0';
    if (*chrp == '.') *chrp = '\0';

    return(text);
}

/**
 *  Convert a database specific time string to a timeval.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  text - time string
 *  @param  tval - output: timeval
 *
 *  @return
 *    - the tval argument
 *    - NULL if an error occurred
 */
timeval_t *pgsql_text_to_timeval(
    const char *text,
    timeval_t  *tval)
{
    struct tm tm_time;
    int       nscanned;
    int       usec;
    time_t    secs;
    char     *chrp;
    int       factor;

    memset((void *)&tm_time, 0, sizeof(struct tm));

    nscanned = sscanf(text,
        "%d-%d-%d %d:%d:%d.%d",
        &(tm_time.tm_year),
        &(tm_time.tm_mon),
        &(tm_time.tm_mday),
        &(tm_time.tm_hour),
        &(tm_time.tm_min),
        &(tm_time.tm_sec),
        &usec);

    if (nscanned < 6) {

        ERROR( DBCONN_LIB_NAME,
            "Could not convert text to timeval: '%s'\n"
            " -> invalid time string format\n", text);

        return((timeval_t *)NULL);
    }
    else if (nscanned == 6) {
        usec = 0;
    }
    else {
        chrp = strchr(text, '.');
        for (factor = 1000000; isdigit(*++chrp); factor /= 10);
        usec *= factor;
    }

    tm_time.tm_year -= 1900;
    tm_time.tm_mon--;

    secs = mktime(&tm_time);

    if (secs == (time_t)-1) {

        ERROR( DBCONN_LIB_NAME,
            "Could not convert text to timeval: '%s'\n"
            " -> mktime error: %s\n", text, strerror(errno));

        return((timeval_t *)NULL);
    }

    secs -= timezone;

    tval->tv_sec  = secs;
    tval->tv_usec = usec;

    return(tval);
}

#endif
