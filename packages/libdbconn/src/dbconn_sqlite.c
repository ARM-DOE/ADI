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
*ƒ
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

/** @file dbconn_sqlite.c
 *  sqlite3 backend.
 */

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>

#include "dbconn_sqlite.h"

/**
 *  Macro for setting sqlite Errors.
 */
#define sqlite_ERROR(dbconn, slconn, ...) \
    _sqlite_error(__func__, __FILE__, __LINE__, dbconn, slconn, __VA_ARGS__)

/*******************************************************************************
 *  Private Functions
 */

static void _sqlite_error(
    const char *func,
    const char *file,
    int         line,
    DBConn     *dbconn,
    sqlite3    *slconn,
    const char *format, ...)
{
    const char *slerr = (const char *)NULL;
    char       *format_str;
    va_list     args;

    if (slconn) {
        slerr = sqlite3_errmsg(slconn);
    }

    if (slerr && slerr[0] != '\0') {

        if (strncmp(slerr, "ERROR:  ", 8) == 0) {
            slerr += 8;
        }

        if (!format) {

            msngr_send( DBCONN_LIB_NAME, func, file, line, MSNGR_ERROR,
                "%s@%s: %s\n",
                dbconn->db_name, dbconn->db_host, slerr);

            return;
        }
    }

    if (format) {

        va_start(args, format);
        format_str = msngr_format_va_list(format, args);
        va_end(args);

        if (format_str) {

            if (slerr && slerr[0] != '\0') {

                msngr_send( DBCONN_LIB_NAME, func, file, line, MSNGR_ERROR,
                    "%s@%s: %s -> %s\n",
                    dbconn->db_name, dbconn->db_host, format_str, slerr);
            }
            else {

                msngr_send( DBCONN_LIB_NAME, func, file, line, MSNGR_ERROR,
                    "%s@%s: %s\n",
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

static void _sqlite_free_dbres(DBResult *dbres)
{
    if (dbres) {
        if (dbres->data) {
            dbres->data = NULL;
        }
        if (dbres->dbres) {
            sqlite3_free_table((char **)dbres->dbres);
            dbres->dbres = NULL;
        }
        free(dbres);
    }
}

static char *_sqlite_expand_command(
    DBConn      *dbconn,
    const char  *command,
    int          nparams,
    const char **params)
{
    sqlite3 *slconn = (sqlite3 *)dbconn->dbh;
    char     *sprocs_query;
    char     *sproc_query_base = "SELECT sp_query FROM stored_procedures WHERE sp_command = ";
    int      slres;
    int      nrows, ncols;
    char   **sp_query;
    const char    *sqlcmd = command;
    char    *expcmd;
    
    /* Get stored procedure data from the database */
    
    sprocs_query = malloc(strlen(command)+strlen(sproc_query_base) + 20);
    sprintf(sprocs_query, "%s\"%s\";", sproc_query_base, command);
    
    slres = sqlite3_get_table(
        slconn,
        sprocs_query,
        &sp_query,
        &nrows,
        &ncols,
        NULL);
    
    free(sprocs_query);
    
    if (slres == SQLITE_OK) {
        if (nrows == 1 && ncols == 1)
            sqlcmd = sp_query[1];
    }
    else {
        sqlite_ERROR(dbconn, slconn,
             "Could not retreive stored procedures from the database\n"
             "Continuing with assumption '%s' isn't a stored procedure\n",
             command);
    }

    /* Substitute parameters into the command */
    expcmd = dbconn_expand_command(sqlcmd, nparams, params);
    
    /* Ensure expcmd is a newly allocated string */
    if (expcmd && expcmd == sqlcmd) {
        expcmd = strdup(sqlcmd);
    }
    
    if (sp_query) {sqlite3_free_table(sp_query);}
    return expcmd;
}


static int _sqlite_get_bool(
    void   *result, 
    int    argc, 
    char **argv,
    char **azColName) 
{
    azColName = azColName; // suppress warning
    if (argc == 0) {
        *((int *)result) = (int)DB_NULL_RESULT;
        return(1);
    }
    
    else if (argc == 1 && argv && argv[0]) {
        
        if (argv[0][0] == '\0') {
            *((int *)result) = (int)DB_NULL_RESULT;
            return(1);
        }
        
        if (argv[0][0] == '1') {
            *((int *)result) = 1;
            return(0);
        }
    
        if (argv[0][0] == '0') {
            *((int *)result) = 0;
            return(0);
        }
    }
    *((int *)result) = (int)DB_BAD_RESULT;
    return (1);
}

static int _sqlite_get_long(
    void *result, 
    int    argc, 
    char **argv,
    char **azColName) 
{
    char *endptr;
    long  value;
    
    azColName = azColName; // suppress warning
    
    if (argc == 0) {
        *((long *)result) = (long)DB_NULL_RESULT;
        return(1);
    }
    
    else if (argc == 1 && argv && argv[0]) {
        
        if (argv[0][0] == '\0') {
            *((long *)result) = (long)DB_NULL_RESULT;
            return(1);
        }
        
        value = strtol(argv[0], &endptr, 10);
        
        if (argv[0] == endptr) {
            *((long *)result) = (long)DB_BAD_RESULT;
            return(1);
        }
        
        *((long *)result) = value;
        return(0);
    }
    *((long *)result) = (long)DB_BAD_RESULT;
    return (1);
}

static int _sqlite_get_double(
    void  *result, 
    int    argc, 
    char **argv,
    char **azColName)
{
    
    char *endptr;
    double  value;
    
    azColName = azColName; // suppress warning
    
    if (argc == 0) {
        *((double *)result) = (double)DB_NULL_RESULT;
        return(1);
    }
    
    else if (argc == 1 && argv && argv[0]) {
        
        if (argv[0][0] == '\0') {
            *((double *)result) = (double)DB_NULL_RESULT;
            return(1);
        }
        
        value = strtod(argv[0], &endptr);
        
        if (argv[0] == endptr) {
            *((double *)result) = (double)DB_BAD_RESULT;
            return(1);
        }
        
        *((double *)result) = value;
        return(0);
    }
    
    *((double *)result) = (double)DB_BAD_RESULT;
    return (1);
}

static int _sqlite_get_text(
    void *result, 
    int    argc, 
    char **argv,
    char **azColName)
{
    azColName = azColName; // suppress warning
    if (argc == 0) {
        *((char **)result) = (char *)DB_NULL_RESULT;
        return(1);
    }
    
    else if (argc == 1 && argv && argv[0]) {
        
        if (argv[0][0] == '\0') {
            *((char **)result) = (char *)DB_NULL_RESULT;
            return(1);
        }
        
        *((char **)result) = strdup(argv[0]);
        return(0);
    }
    
    *((char **)result) = (char *)DB_BAD_RESULT;
    return (1);
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
DBStatus sqlite_connect(DBConn *dbconn)
{
    sqlite3 *slconn;
    int slres;
    DBStatus status;

    /* Make sure any previous connection has been closed */

    if (dbconn->dbh) {
        sqlite_disconnect(dbconn);
    }
    
    /* Make a connection to the database */
    
    slres = sqlite3_open(dbconn->db_name, &slconn);

    /* Check to see that the backend connection was successfully made */

    if (!slconn) {

        sqlite_ERROR(dbconn, slconn,
            "Memory allocation error\n");

        return(DB_MEM_ERROR);
    }
    
    if (slres != SQLITE_OK) {
        sqlite_ERROR(dbconn, slconn,
            "Database connection unsuccessful\n");
        return(DB_ERROR);
    }

    dbconn->dbh = (void *)slconn;
    
    /* Set the "busy timeout" interval in ms */

    sqlite3_busy_timeout(slconn, 15000);

    /* Enable foreign key constraints */
    
    status = sqlite_exec(dbconn, "PRAGMA foreign_keys = ON;", 0, NULL);
    if (status != DB_NO_ERROR) {
        sqlite_ERROR(dbconn, slconn,
            "Database connection unsuccessful\n"
            "Could not enable foreign key constraints\n");
        
        sqlite_disconnect(dbconn);
    }

    return(status);
}

/**
 *  Disconnect from the database.
 *
 *  @param  dbconn - pointer to the database connection
 */
void sqlite_disconnect(DBConn *dbconn)
{
    int slres;
    
    if (dbconn->dbh) {

        sqlite3 *slconn = (sqlite3 *)dbconn->dbh;

        slres = sqlite3_close(slconn);
        
        if (slres != SQLITE_OK) {
            sqlite_ERROR(dbconn, slconn,
                "Database disconnection unsucessful\n");
        }

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
int sqlite_is_connected(DBConn *dbconn)
{
    if (dbconn->dbh) {
        
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
DBStatus sqlite_reset(DBConn *dbconn)
{
    sqlite_disconnect(dbconn);
    
    return(sqlite_connect(dbconn));
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
 *    - DB_MEM_ERROR
 *    - DB_ERROR
 *
 *  @see DBStatus
 */
DBStatus sqlite_exec(
    DBConn      *dbconn,
    const char  *command,
    int          nparams,
    const char **params)
{
    sqlite3  *slconn = (sqlite3 *)dbconn->dbh;
    int       slres;
    char     *slcmd;
    
    /* Expand the command's stored procedures and parameters */
    slcmd = _sqlite_expand_command(dbconn, command, nparams, params);
    
    if (!slcmd) {
        return(DB_ERROR);
    }
    
    slres = sqlite3_exec(slconn, slcmd, NULL, NULL, NULL);

    if (slres != SQLITE_OK) {
        
        sqlite_ERROR(dbconn, slconn,
            "FAILED: %s\n",
            slcmd);
        
        free(slcmd);
        if (slres == SQLITE_NOMEM) {
            return(DB_MEM_ERROR);
        }
        
        return(DB_ERROR);
    }

    free(slcmd);
    return(DB_NO_ERROR);
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

DBStatus sqlite_query(
    DBConn      *dbconn,
    const char  *command,
    int          nparams,
    const char **params,
    DBResult   **result)
{
    sqlite3  *slconn = (sqlite3 *)dbconn->dbh;
    char *slcmd;
    int   slres;
    
    *result = (DBResult *)malloc(sizeof(DBResult));
    (*result)->dbres = NULL;
    
    /* Convert postgres style command into an sqlite query */
    slcmd = _sqlite_expand_command(dbconn, command, nparams, params);
    
    if (!slcmd) {
        return(DB_ERROR);
    }
    
    /* Query the database and get the result */
    slres = sqlite3_get_table(slconn, slcmd, 
        (char ***)(&((*result)->dbres)), 
        &((*result)->nrows), 
        &((*result)->ncols),
        NULL);
    
    /* Check that data was successfully retreived */
    
    if (slres != SQLITE_OK) {
        
        sqlite_ERROR(dbconn, slconn,
            "FAILED: %s\n",
            slcmd);
        
        free(slcmd);
        _sqlite_free_dbres(*result);
        *result = NULL;
        
        if (slres == SQLITE_NOMEM) {
            return(DB_MEM_ERROR);
        }
        return DB_ERROR;
    }
    
    if (!(*result)->nrows || !(*result)->ncols) {
        free(slcmd);
        _sqlite_free_dbres(*result);
        return(DB_NULL_RESULT);
    }
    
    /* The first ncols elements of the get_table result are column names, skip them */
    
    (*result)->data = (char **)((*result)->dbres) + (*result)->ncols;
    
    (*result)->free = _sqlite_free_dbres;
    
    free(slcmd);
    return(DB_NO_ERROR);
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
DBStatus sqlite_query_bool(
    DBConn      *dbconn,
    const char  *command,
    int          nparams,
    const char **params,
    int         *result)
{
    sqlite3  *slconn = (sqlite3 *)dbconn->dbh;
    char     *slcmd;
    int       slres;
    
    *result = -1;
        
    /* Expand the command's stored procedures and parameters */
    
    slcmd = _sqlite_expand_command(dbconn, command, nparams, params);
    
    if (!slcmd) {
        return(DB_ERROR);
    }

    /* Query the database and get the result */
    
    slres = sqlite3_exec(slconn, slcmd, _sqlite_get_bool, (void *)result, NULL);
    
    /* Check that data was successfully retreived */
    
    if (*result == -1) {
        free (slcmd);
        return (DB_NULL_RESULT);
    }
    
    if (slres != SQLITE_OK) {
        if (slres == SQLITE_ABORT) {
            if ((DBStatus)(*result) == DB_NULL_RESULT) {
                *result = 0;
                free (slcmd);
                return (DB_NULL_RESULT);
            }
            else {
                sqlite_ERROR(dbconn, NULL,
                    "FAILED: %s\n"
                    " -> query returned non-boolean value\n",
                    slcmd);
                
                free(slcmd);
                return ((DBStatus)(*result));
            }
        }
        
        else {
            sqlite_ERROR(dbconn, slconn,
                "FAILED: %s\n",
                slcmd);
            
            free(slcmd);
            return(DB_ERROR);
        }
    }
    
    free(slcmd);
    return(DB_NO_ERROR);
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
DBStatus sqlite_query_int(
    DBConn      *dbconn,
    const char  *command,
    int          nparams,
    const char **params,
    int         *result)
{
    DBStatus status;
    long     value;
    
    status = sqlite_query_long(dbconn, command, nparams, params, &value);

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
DBStatus sqlite_query_long(
    DBConn      *dbconn,
    const char  *command,
    int          nparams,
    const char **params,
    long        *result)
{
    sqlite3  *slconn = (sqlite3 *)dbconn->dbh;
    char     *slcmd;
    int       slres;
    
    *result = LONG_MIN; // an improbable value
    
    /* Expand the command's stored procedures and parameters */
    
    slcmd = _sqlite_expand_command(dbconn, command, nparams, params);
    
    if (!slcmd) {
        return(DB_ERROR);
    }

    /* Query the database and get the result */
    
    slres = sqlite3_exec(slconn, slcmd, _sqlite_get_long, (void *)result, NULL);
    
    /* Check that data was successfully retreived */

    if (*result == LONG_MIN) {
        *result = 0;
        free (slcmd);
        return (DB_NULL_RESULT);
    }
    
    if (slres != SQLITE_OK) {
        if (slres == SQLITE_ABORT) {
            if ((DBStatus)(*result) == DB_NULL_RESULT) {
                *result = 0;
                free (slcmd);
                return (DB_NULL_RESULT);
            }
            else {
                sqlite_ERROR(dbconn, NULL,
                    "FAILED: %s\n"
                    " -> query returned non-integer value\n",
                    slcmd);
                
                free(slcmd);
                return ((DBStatus)(*result));
            }
        }
        else {
            sqlite_ERROR(dbconn, slconn,
                "FAILED: %s\n",
                slcmd);
            
            free(slcmd);
            return(DB_ERROR);
        }
    }

    free(slcmd);
    return(DB_NO_ERROR);
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
DBStatus sqlite_query_float(
    DBConn      *dbconn,
    const char  *command,
    int          nparams,
    const char **params,
    float       *result)
{
    DBStatus status;
    double   value;
    
    status = sqlite_query_double(dbconn, command, nparams, params, &value);
    
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
DBStatus sqlite_query_double(
    DBConn      *dbconn,
    const char  *command,
    int          nparams,
    const char **params,
    double      *result)
{
    sqlite3  *slconn = (sqlite3 *)dbconn->dbh;
    char     *slcmd;
    int       slres;
    
    *result = -9847.4321946; // an improbable value
    
    /* Expand the command's stored procedures and parameters */
    
    slcmd = _sqlite_expand_command(dbconn, command, nparams, params);
    
    if (!slcmd) {
        return(DB_ERROR);
    }
    
    /* Query the database and get the result */
    
    slres = sqlite3_exec(slconn, slcmd, _sqlite_get_double, (void *)result, NULL);
    
    /* Check that data was successfully retreived */
    
    if (*result == -9847.4321946) {
        *result = 0;
        free (slcmd);
        return (DB_NULL_RESULT);
    }
    
    if (slres != SQLITE_OK) {
            if (slres == SQLITE_ABORT) {
                
                if ((DBStatus)(*result) == DB_NULL_RESULT) {
                    *result = 0;
                    free (slcmd);
                    return (DB_NULL_RESULT);
                }
                else {
                    sqlite_ERROR(dbconn, NULL,
                        "FAILED: %s\n"
                        " -> query returned non-float value\n",
                        slcmd);
                    
                    free(slcmd);
                    return ((DBStatus)(*result));
                }
        }
        else {
            sqlite_ERROR(dbconn, slconn,
                 "FAILED: %s\n",
                 slcmd);
            
            free(slcmd);
            return(DB_ERROR);
        }
    }
 
    free(slcmd);
    return(DB_NO_ERROR);
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
DBStatus sqlite_query_text(
    DBConn      *dbconn,
    const char  *command,
    int          nparams,
    const char **params,
    char       **result)
{
    sqlite3  *slconn = (sqlite3 *)dbconn->dbh;
    char     *slcmd;
    int       slres;
    
    *result = NULL;
    
    /* Expand the command's stored procedures and parameters */
    
    slcmd = _sqlite_expand_command(dbconn, command, nparams, params);
 
    if (!slcmd) {
        return(DB_ERROR);
    }
   
    /* Query the database and get the result */
    
    slres = sqlite3_exec(slconn, slcmd, _sqlite_get_text, (void *)result, NULL);
    
    /* Check that data was successfully retreived */
    
    if (!(*result)) {
        free (slcmd);
        return (DB_NULL_RESULT);
    }
    
    if (slres != SQLITE_OK) {
        
        if (slres == SQLITE_ABORT) {
            if ((DBStatus)(*result) == DB_NULL_RESULT) {
                *result = (char *)NULL;
                free (slcmd);
                return (DB_NULL_RESULT);
            }
            else {
                sqlite_ERROR(dbconn, NULL,
                     "FAILED: %s\n"
                     " -> query returned non-text value\n",
                     slcmd);
                
                free(slcmd);
                return ((DBStatus)(*result));
            }
        }
        else {
            sqlite_ERROR(dbconn, slconn,
                 "FAILED: %s\n",
                 slcmd);
            
            free(slcmd);
            return(DB_ERROR);
        }
    }
    
    free(slcmd);
    return(DB_NO_ERROR);
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
char *sqlite_bool_to_text(int bval, char *text)
{
    if (bval) {
        strcpy(text, "1");
    }
    else {
        strcpy(text, "0");
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
int *sqlite_text_to_bool(const char *text, int *bval)
{
    if (*text == '1') {
        *bval = 1;
    }
    else if (*text == '0') {
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
char *sqlite_time_to_text(time_t time, char *text)
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
time_t *sqlite_text_to_time(const char *text, time_t *time)
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
char *sqlite_timeval_to_text(const timeval_t *tval, char *text)
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
timeval_t *sqlite_text_to_timeval(
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
