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

/** @file dbconn.c
 *  Database Connection Interface.
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>
#include <unistd.h>

#include "dbconn_private.h"
#include "dbconn_pgsql.h"
#include "dbconn_wspc.h"
#include "dbconn_sqlite.h"
#include "../config.h"

/**
 *  @defgroup DBCONN_INTERFACE Database Interface
 */
/*@{*/

/*******************************************************************************
 *  Private Functions
 */
/** @privatesection */

/**
 *  Find the .db_connect file.
 *
 *  This function will first check for the DB_CONNECT_PATH environment
 *  variable and search all specified paths for a .db_connect file.
 *  If one is not found it will then check in the user's home directory.
 *
 *  If the .db_connect file could not be found an error message
 *  will be sent to the message handler.
 *
 *  @param  dbconn_file - output: path to the .db_connect file
 *
 *  @return
 *    - 1 if found
 *    - 0 if not found
 */
static int _dbconn_find_db_connect_file(char *dbconn_file)
{
    const char *db_connect_path = getenv("DB_CONNECT_PATH");
    const char *home_dir;
    const char *end;
    size_t      length;

    /* Check the DB_CONNECT_PATH */

    while (db_connect_path && *db_connect_path != '\0') {

        end = strchr(db_connect_path, ':');
        if (!end) {
            end = strchr(db_connect_path, '\0');
        }

        length = end - db_connect_path;

        if (length > PATH_MAX - 16) {

            ERROR( DBCONN_LIB_NAME,
                "Could not find .db_connect file\n"
                " -> A path length in DB_CONNECT_PATH is too long\n");

            return(0);
        }

        snprintf(dbconn_file, length + 1, "%s", db_connect_path);
        strcpy(dbconn_file + length, "/.db_connect");

        if (access(dbconn_file, F_OK) == 0) {
            return(1);
        }

        db_connect_path = end;
        while (*db_connect_path == ':') ++db_connect_path;
    }

    /* Check the user's home directory */

    home_dir = getenv("HOME");
    if (!home_dir) {

        ERROR( DBCONN_LIB_NAME,
            "Could not find .db_connect file\n"
            " -> HOME environment variable not found\n");

        return(0);
    }

    snprintf(dbconn_file, PATH_MAX, "%s/.db_connect", home_dir);

    if (access(dbconn_file, F_OK) == 0) {
        return(1);
    }

    ERROR( DBCONN_LIB_NAME,
        "Could not find .db_connect file: %s\n",
        dbconn_file);

    return(0);
}

/**
 *  Parse the database connection information from the .db_connect file.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  dbconn   - pointer to the DBConn structure
 *  @param  db_alias - the database connection alias
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
static int _dbconn_parse_db_connect_file(
    DBConn     *dbconn,
    const char *db_alias)
{
    char  dbconn_file[PATH_MAX];
    FILE *fp;
    char  line[256];
    char *linep;
    char  db_type[64];
    int   nscanned;

    /* Find the .db_connect file */

    if (!_dbconn_find_db_connect_file(dbconn_file)) {
        return(0);
    }

    /* Open the .db_connect file */

    fp = fopen(dbconn_file, "r");

    if(!fp) {

        ERROR( DBCONN_LIB_NAME,
            "Could not open .db_connect file: %s\n"
            " -> %s\n", dbconn_file, strerror(errno));

        return(0);
    }

    /* Parse the .db_connect file */

    while(fgets(line, 256, fp) != NULL) {

        /* Remove end of line comments */

        linep = strchr(line, '#');
        if (linep) *linep = '\0';

        /* Skip leading white-space */

        for(linep = line; isspace(*linep); linep++);

        /* Skip blank lines */

        if (*linep == '\0') {
            continue;
        }

        /* Get the remaining .db_connect info */

        nscanned = sscanf(linep, "%s %s %s %s %s %s",
            dbconn->db_alias,
            dbconn->db_host,
            dbconn->db_name,
            dbconn->db_user,
            dbconn->db_pass,
            db_type);

        /* Make sure there are at least 2 entries on this line */

        if (nscanned < 2) {
            continue;
        }

        /* Check if this is the alias we are looking for */

        if (strcmp(db_alias, dbconn->db_alias) != 0) {
            continue;
        }

        fclose(fp);

        /* Determine connection type */

        if (strncmp(dbconn->db_host, "http", 4) == 0) {
            dbconn->db_type = DB_WSPC;
        }
        else if (strncmp(dbconn->db_host, "sqlite", 6) == 0) {
            dbconn->db_type = DB_SQLITE;

        }
        else if (nscanned < 5) {
            continue;
        }
        else {
            dbconn->db_type = DB_PGSQL;
        }

        return(1);
    }

    /* Alias not found in the .db_connect file */

    fclose(fp);

    ERROR( DBCONN_LIB_NAME,
        "Could not find alias '%s' in .db_connect file: %s\n",
        db_alias, dbconn_file);

    return(0);
}

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

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
DBConn *dbconn_create(const char *db_alias)
{
    DBConn *dbconn;

    /* Allocate memory for the DBConn structure */

    dbconn = (DBConn *)calloc(1, sizeof(DBConn));
    if (!dbconn) {

        ERROR( DBCONN_LIB_NAME,
            "Could not create database connection\n"
            " -> memory allocation error\n");

        return((DBConn *)NULL);
    }

    /* Get the database connection information from the .db_connect file */

    if (!_dbconn_parse_db_connect_file(dbconn, db_alias)) {
        free(dbconn);
        return((DBConn *)NULL);
    }

    /* Allocate memory for the database interface */

    dbconn->dbi = calloc(1, sizeof(_DBI));
    if (!dbconn->dbi) {

        ERROR( DBCONN_LIB_NAME,
            "Could not create database connection\n"
            " -> memory allocation error\n");

        free(dbconn);
        return((DBConn *)NULL);
    }

    /* Attach Interface Functions */

    switch (dbconn->db_type) {

        case DB_PGSQL:

#ifdef HAVE_POSTGRESQL
            DBI(dbconn)->connect          = pgsql_connect;
            DBI(dbconn)->disconnect       = pgsql_disconnect;
            DBI(dbconn)->reset            = pgsql_reset;
            DBI(dbconn)->is_connected     = pgsql_is_connected;
            DBI(dbconn)->exec             = pgsql_exec;
            DBI(dbconn)->query            = pgsql_query;
            DBI(dbconn)->query_bool       = pgsql_query_bool;
            DBI(dbconn)->query_int        = pgsql_query_int;
            DBI(dbconn)->query_long       = pgsql_query_long;
            DBI(dbconn)->query_float      = pgsql_query_float;
            DBI(dbconn)->query_double     = pgsql_query_double;
            DBI(dbconn)->query_text       = pgsql_query_text;
            DBI(dbconn)->bool_to_text     = pgsql_bool_to_text;
            DBI(dbconn)->text_to_bool     = pgsql_text_to_bool;
            DBI(dbconn)->time_to_text     = pgsql_time_to_text;
            DBI(dbconn)->text_to_time     = pgsql_text_to_time;
            DBI(dbconn)->timeval_to_text  = pgsql_timeval_to_text;
            DBI(dbconn)->text_to_timeval  = pgsql_text_to_timeval;
#else
            ERROR( DBCONN_LIB_NAME,
                "Could not create database connection\n"
                " -> The PostgreSQL libraries were not found when libdbconn was built.\n");
            free(dbconn->dbi);
            free(dbconn);
            return((DBConn *)NULL);
#endif
            break;

        case DB_WSPC:

            DBI(dbconn)->connect          = wspc_connect;
            DBI(dbconn)->disconnect       = wspc_disconnect;
            DBI(dbconn)->reset            = wspc_reset;
            DBI(dbconn)->is_connected     = wspc_is_connected;
            DBI(dbconn)->exec             = wspc_exec;
            DBI(dbconn)->query            = wspc_query;
            DBI(dbconn)->query_bool       = wspc_query_bool;
            DBI(dbconn)->query_int        = wspc_query_int;
            DBI(dbconn)->query_long       = wspc_query_long;
            DBI(dbconn)->query_float      = wspc_query_float;
            DBI(dbconn)->query_double     = wspc_query_double;
            DBI(dbconn)->query_text       = wspc_query_text;
            DBI(dbconn)->bool_to_text     = wspc_bool_to_text;
            DBI(dbconn)->text_to_bool     = wspc_text_to_bool;
            DBI(dbconn)->time_to_text     = wspc_time_to_text;
            DBI(dbconn)->text_to_time     = wspc_text_to_time;
            DBI(dbconn)->timeval_to_text  = wspc_timeval_to_text;
            DBI(dbconn)->text_to_timeval  = wspc_text_to_timeval;

            break;

        case DB_SQLITE:
            DBI(dbconn)->connect          = sqlite_connect;
            DBI(dbconn)->disconnect       = sqlite_disconnect;
            DBI(dbconn)->reset            = sqlite_reset;
            DBI(dbconn)->is_connected     = sqlite_is_connected;
            DBI(dbconn)->exec             = sqlite_exec;
            DBI(dbconn)->query            = sqlite_query;
            DBI(dbconn)->query_bool       = sqlite_query_bool;
            DBI(dbconn)->query_int        = sqlite_query_int;
            DBI(dbconn)->query_long       = sqlite_query_long;
            DBI(dbconn)->query_float      = sqlite_query_float;
            DBI(dbconn)->query_double     = sqlite_query_double;
            DBI(dbconn)->query_text       = sqlite_query_text;
            DBI(dbconn)->bool_to_text     = sqlite_bool_to_text;
            DBI(dbconn)->text_to_bool     = sqlite_text_to_bool;
            DBI(dbconn)->time_to_text     = sqlite_time_to_text;
            DBI(dbconn)->text_to_time     = sqlite_text_to_time;
            DBI(dbconn)->timeval_to_text  = sqlite_timeval_to_text;
            DBI(dbconn)->text_to_timeval  = sqlite_text_to_timeval;

            break;


        default:

            ERROR( DBCONN_LIB_NAME,
                "Could not create database connection\n"
                " -> invalid database type: %d\n",
                dbconn->db_type);

            free(dbconn->dbi);
            free(dbconn);
            return((DBConn *)NULL);
    }

    return(dbconn);
}

/**
 *  Destroy a database connection.
 *
 *  This function will close the database connection and
 *  free all memory associated with the DBConn structure.
 *
 *  @param  dbconn - pointer to the database connection
 */
void dbconn_destroy(DBConn *dbconn)
{
    if (dbconn) {

        if (dbconn->dbh) {
            DBI(dbconn)->disconnect(dbconn);
        }

        free(dbconn->dbi);
        free(dbconn);
    }
}

/**************************************************************************
 * Connection Functions
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
DBStatus dbconn_connect(DBConn *dbconn)
{
    return(DBI(dbconn)->connect(dbconn));
}

/**
 *  Disconnect from the database.
 *
 *  @param  dbconn - pointer to the database connection
 */
void dbconn_disconnect(DBConn *dbconn)
{
    DBI(dbconn)->disconnect(dbconn);
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
int dbconn_is_connected(DBConn *dbconn)
{
    return(DBI(dbconn)->is_connected(dbconn));
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
DBStatus dbconn_reset(DBConn *dbconn)
{
    return(DBI(dbconn)->reset(dbconn));
}

/**************************************************************************
 * Command Functions
 */
/**
 *  Expand all the parameters in a command string.
 *
 *  The returned string is dynamically allocated so it must be free
 *  by the calling process when it is no longer needed.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  command - command string
 *  @param  nparams - number of $1, $2, ... parameters in the command
 *  @param  params  - parameters to substitute in the command
 *
 *  @return
 *    - command string with all parameter values expanded
 *    - NULL if an error occurred
 *
 */
char *dbconn_expand_command(
                            const char  *command,
                            int          nparams,
                            const char **params)
{
    const char *cmdp;
    char       *expcmd;
    char       *expcmdp;
    size_t      length;
    const char *paramp;
    int         paramnum;

    /* Determine the length of the new command string */

    length = strlen(command) + 1;
    cmdp   = command;

    while (*cmdp != '\0') {

        if ((cmdp[0] != '$') ||
            (!isdigit(cmdp[1]))) {

            ++cmdp;
        }
        else {

            paramnum = atoi(++cmdp);

            while (isdigit(*(++cmdp)));

            if ((paramnum <= 0) ||
                (paramnum >  nparams)) {

                ERROR( DBCONN_LIB_NAME,
                      "Could not expand command paramters in: '%s'\n"
                      " -> invalide parameter number in command string: %d\n",
                      command, paramnum);

                return((char *)NULL);
            }

            length += strlen(params[paramnum - 1]);
        }
    }

    /* Expand the parameters */

    expcmd = (char *)calloc(length, sizeof(char));
    if (!expcmd) {

        ERROR( DBCONN_LIB_NAME,
              "Could not expand command paramters in: '%s'\n"
              " -> memory allocation error\n",
              command);

        return((char *)NULL);
    }

    cmdp    = command;
    expcmdp = expcmd;

    while (*cmdp != '\0') {

        if ((cmdp[0] != '$') ||
            (!isdigit(cmdp[1]))) {

            *expcmdp++ = *cmdp++;
        }
        else {

            paramnum = atoi(++cmdp);

            while (isdigit(*(++cmdp)));

            if ((paramnum <= 0) ||
                (paramnum >  nparams)) {

                ERROR( DBCONN_LIB_NAME,
                      "Could not expand command paramters in: '%s'\n"
                      " -> invalide parameter number in command string: %d\n",
                      command, paramnum);

                return((char *)NULL);
            }

            paramp = params[paramnum - 1];

            *expcmdp++ = '\'';

            while (*paramp != '\0') {
                *expcmdp++ = *paramp++;
            }

            *expcmdp++ = '\'';
        }
    }

    return(expcmd);
}

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
DBStatus dbconn_exec(
    DBConn      *dbconn,
    const char  *command,
    int          nparams,
    const char **params)
{
    return(DBI(dbconn)->exec(dbconn, command, nparams, params));
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
 *    - DB_BAD_RESULT
 *    - DB_MEM_ERROR
 *    - DB_ERROR
 *
 *  @see DBStatus
 */
DBStatus dbconn_query(
    DBConn      *dbconn,
    const char  *command,
    int          nparams,
    const char **params,
    DBResult   **result)
{
    return(DBI(dbconn)->query(dbconn, command, nparams, params, result));
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
 *    - DB_MEM_ERROR
 *    - DB_ERROR
 *
 *  @see DBStatus
 */
DBStatus dbconn_query_bool(
    DBConn      *dbconn,
    const char  *command,
    int          nparams,
    const char **params,
    int         *result)
{
    return(DBI(dbconn)->query_bool(dbconn, command, nparams, params, result));
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
 *    - DB_MEM_ERROR
 *    - DB_ERROR
 *
 *  @see DBStatus
 */
DBStatus dbconn_query_int(
    DBConn      *dbconn,
    const char  *command,
    int          nparams,
    const char **params,
    int         *result)
{
    return(DBI(dbconn)->query_int(dbconn, command, nparams, params, result));
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
 *    - DB_MEM_ERROR
 *    - DB_ERROR
 *
 *  @see DBStatus
 */
DBStatus dbconn_query_long(
    DBConn      *dbconn,
    const char  *command,
    int          nparams,
    const char **params,
    long        *result)
{
    return(DBI(dbconn)->query_long(dbconn, command, nparams, params, result));
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
 *    - DB_MEM_ERROR
 *    - DB_ERROR
 *
 *  @see DBStatus
 */
DBStatus dbconn_query_float(
    DBConn      *dbconn,
    const char  *command,
    int          nparams,
    const char **params,
    float       *result)
{
    return(DBI(dbconn)->query_float(dbconn, command, nparams, params, result));
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
 *    - DB_MEM_ERROR
 *    - DB_ERROR
 *
 *  @see DBStatus
 */
DBStatus dbconn_query_double(
    DBConn      *dbconn,
    const char  *command,
    int          nparams,
    const char **params,
    double      *result)
{
    return(DBI(dbconn)->query_double(dbconn, command, nparams, params, result));
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
DBStatus dbconn_query_text(
    DBConn      *dbconn,
    const char  *command,
    int          nparams,
    const char **params,
    char       **result)
{
    return(DBI(dbconn)->query_text(dbconn, command, nparams, params, result));
}

/**************************************************************************
 * Utility Functions
 */

/**
 *  Convert a boolean value to a database specific text string.
 *
 *  This function will convert a boolean value into a string that
 *  can be used in database queries.
 *
 *  @param  dbconn - pointer to the database connection
 *  @param  bval   - boolean value (1 = TRUE, 0 = FALSE)
 *  @param  text   - output: text string
 *
 *  @return  the text argument
 */
char *dbconn_bool_to_text(DBConn *dbconn, int bval, char *text)
{
    return(DBI(dbconn)->bool_to_text(bval, text));
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
 *  @param  dbconn - pointer to the database connection
 *  @param  text   - text string
 *  @param  bval   - output: boolean value (1 = TRUE, 0 = FALSE)
 *
 *  @return
 *    - the bval argument
 *    - NULL if the text string is not a valid boolean value
 */
int *dbconn_text_to_bool(DBConn *dbconn, const char *text, int *bval)
{
    return(DBI(dbconn)->text_to_bool(text, bval));
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
 *  @param  dbconn - pointer to the database connection
 *  @param  time   - seconds since 1970
 *  @param  text   - output: time string
 *
 *  @return
 *    - the text argument
 *    - NULL if an error occurred
 */
char *dbconn_time_to_text(
    DBConn *dbconn,
    time_t  time,
    char   *text)
{
    return(DBI(dbconn)->time_to_text(time, text));
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
 *  @param  dbconn - pointer to the database connection
 *  @param  text   - time string
 *  @param  time   - output: seconds since 1970
 *
 *  @return
 *    - the time argument
 *    - NULL if an error occurred
 */
time_t *dbconn_text_to_time(
    DBConn     *dbconn,
    const char *text,
    time_t     *time)
{
    return(DBI(dbconn)->text_to_time(text, time));
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
 *  @param  dbconn - pointer to the database connection
 *  @param  tval   - pointer to the timeval (in seconds since 1970)
 *  @param  text   - output: time string
 *
 *  @return
 *    - the text argument
 *    - NULL if an error occurred
 */
char *dbconn_timeval_to_text(
    DBConn          *dbconn,
    const timeval_t *tval,
    char            *text)
{
    return(DBI(dbconn)->timeval_to_text(tval, text));
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
 *  @param  dbconn - pointer to the database connection
 *  @param  text   - time string
 *  @param  tval   - output: timeval (in seconds since 1970)
 *
 *  @return
 *    - the tval argument
 *    - NULL if an error occurred
 */
timeval_t *dbconn_text_to_timeval(
    DBConn     *dbconn,
    const char *text,
    timeval_t  *tval)
{
    return(DBI(dbconn)->text_to_timeval(text, tval));
}

/*@}*/
