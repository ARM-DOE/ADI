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
*    $Revision: 57858 $
*    $Author: ermold $
*    $Date: 2014-10-27 19:56:33 +0000 (Mon, 27 Oct 2014) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file dbconn_wspc.c
 *  Web Service Procedure Call Backend.
 */

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>

#include "dbconn_wspc.h"

/**
 *  Macro for setting WSPC Errors.
 */
#define WSPC_ERROR(errnum, ...) \
    _wspc_error(__func__, __FILE__, __LINE__, errnum, __VA_ARGS__)

/*******************************************************************************
 *  Private Data and Functions
 */

static char _CurlErrorString[CURL_ERROR_SIZE];

static void _wspc_error(
    const char *func,
    const char *file,
    int         line,
    CURLcode    errnum,
    const char *format, ...)
{
    const char *curl_error = (const char *)NULL;
    char       *format_str;
    va_list     args;

    if (errnum) {
        if (_CurlErrorString[0] == '\0') {
            curl_error = curl_easy_strerror(errnum);
        }
        else {
            curl_error = _CurlErrorString;
        }
    }

    if (format) {

        va_start(args, format);
        format_str = msngr_format_va_list(format, args);
        va_end(args);

        if (format_str) {

            if (curl_error) {

                msngr_send( DBCONN_LIB_NAME, func, file, line, MSNGR_ERROR,
                    "DB Web Service Error: %s -> %s",
                    format_str, curl_error);
            }
            else {

                msngr_send( DBCONN_LIB_NAME, func, file, line, MSNGR_ERROR,
                    "DB Web Service Error: %s",
                    format_str);
            }
        }
        else {

            msngr_send( DBCONN_LIB_NAME, func, file, line, MSNGR_ERROR,
                "DB Web Service Error: Memory allocation error\n");
        }
    }
    else if (curl_error) {

        msngr_send( DBCONN_LIB_NAME, func, file, line, MSNGR_ERROR,
            "DB Web Service Error: %s\n",
            curl_error);
    }

    if (errnum) {
        _CurlErrorString[0] = '\0';
    }
}

static void _wspc_free_curlres(CurlResult *curlres)
{
    if (curlres) {
        if (curlres->buffer) free(curlres->buffer);
        free(curlres);
    }
}

static void _wspc_free_dbres(DBResult *dbres)
{
    if (dbres) {
        if (dbres->data)  free(dbres->data);
        if (dbres->dbres) _wspc_free_curlres((CurlResult *)dbres->dbres);
        free(dbres);
    }
}

static DBStatus _wspc_create_dbres(
    const char  *url,
    CurlResult  *curlres,
    DBResult   **dbres)
{
    char **data     = (char **)NULL;
    int    nalloced = 0;
    int    done     = 0;
    int    nrows    = 0;
    int    ncols    = 0;
    int    col      = 0;
    int    index    = 0;

    char **new_data;
    char  *bufp;
    char  *outp;

    *dbres = (DBResult *)NULL;

    /* Make sure we have a valid result buffer */

    if (!curlres->buflen) {
        return(DB_NULL_RESULT);
    }

    if (curlres->buffer[0] != '[') {

        ERROR( DBCONN_LIB_NAME,
            "Could not create result for: '%s'\n"
            " -> invalid response from server\n",
            url);

        return(DB_ERROR);
    }

    /* Parse the result buffer */

    bufp = curlres->buffer + 1;

    while (!done && *bufp != '\0') {

        switch (*bufp) {

            /* Check for row start */

            case '[':

                nrows++;
                col = 0;

                break;

            /* Check for row end or result end */

            case ']':

                if (!col) {
                    done = 1;
                }
                else if (!ncols) {
                    ncols = col;
                }
                else if (col != ncols) {

                    ERROR( DBCONN_LIB_NAME,
                        "Could not create result for: '%s'\n"
                        " -> invalid response from server\n",
                        url);

                    if (data) free(data);
                    return(DB_ERROR);
                }

                col = 0;

                *bufp = '\0';

                break;

            /* Skip spaces and commas */

            case ',':
            case ' ':
                break;

            /* Start of next value */

            default:

                col++;

                /* Allocate more memory for the data array if necessary */

                if (index == nalloced) {

                    nalloced = (nalloced) ? (nalloced * 2) : 8;
                    new_data = (char **)realloc(data, nalloced * sizeof(char *));

                    if (!new_data) {

                        ERROR( DBCONN_LIB_NAME,
                            "Could not create result for: '%s'\n"
                            " -> memory allocation error\n",
                            url);

                        if (data) free(data);
                        return(DB_MEM_ERROR);
                    }

                    data = new_data;
                }

                /* Check for null value */

                if (*bufp == 'n' || *bufp == 'N') {

                    if ((strncmp(bufp, "null", 4) == 0) ||
                        (strncmp(bufp, "NULL", 4) == 0)) {

                        data[index++] = (char *)NULL;
                        bufp += 3;
                        break;
                    }
                }

                /* Check for quote */

                if (*bufp == '"') {

                    data[index++] = ++bufp;

                    outp = bufp;

                    while (*bufp != '"' && *bufp != '\0') {

                        if (*bufp == '\\') {

                            if (*(++bufp) == '\0') break;

                            switch (*bufp) {
                                case '"':  *outp = '"';   break;
                                case '\\': *outp = '\\';  break;
                                case 'n':  *outp = '\n';  break;
                                case 'r':  *outp = '\r';  break;
                                case 't':  *outp = '\t';  break;
                                default:   *outp = *bufp; break;
                            }
                        }
                        else {
                            *outp = *bufp;
                        }

                        outp++;
                        bufp++;
                    }
                }
                else {
                    data[index++] = bufp;

                    outp = bufp;

                    while (*bufp != ',' &&
                           *bufp != ']' &&
                           *bufp != '\0') {

                        if (*bufp == '\\') {

                            if (*(++bufp) == '\0') break;

                            switch (*bufp) {
                                case ',':  *outp = ',';   break;
                                case '\\': *outp = '\\';  break;
                                case 'n':  *outp = '\n';  break;
                                case 'r':  *outp = '\r';  break;
                                case 't':  *outp = '\t';  break;
                                default:   *outp = *bufp; break;
                            }
                        }
                        else {
                            *outp = *bufp;
                        }

                        outp++;
                        bufp++;
                    }
                }

                if (*bufp == '\0') {

                    ERROR( DBCONN_LIB_NAME,
                        "Could not create result for: '%s'\n"
                        " -> invalid response from server\n",
                        url);

                    free(data);
                    return(DB_ERROR);
                }

                if (*outp == ']') {
                    bufp--;
                }
                else {
                    *outp = '\0';
                }

                break;
        }

        bufp++;

    } /* end: while (*bufp != '\0') */

    /* Check for error or NULL result */

    if (!done && *bufp == '\0') {

        ERROR( DBCONN_LIB_NAME,
            "Could not create result for: '%s'\n"
            " -> invalid response from server\n",
            url);

        if (data) free(data);
        return(DB_ERROR);
    }
    else if (!nrows || !ncols) {
        if (data) free(data);
        return(DB_NULL_RESULT);
    }

    /* Create the DBResult structure */

    *dbres = (DBResult *)malloc(sizeof(DBResult));
    if (!(*dbres)) {

        ERROR( DBCONN_LIB_NAME,
            "Could not create result for: '%s'\n"
            " -> memory allocation error\n",
            url);

        free(data);
        return(DB_MEM_ERROR);
    }

    (*dbres)->nrows = nrows;
    (*dbres)->ncols = ncols;
    (*dbres)->data  = data;
    (*dbres)->dbres = (void *)curlres;
    (*dbres)->free  = _wspc_free_dbres;

    return(DB_NO_ERROR);
}

static size_t _wspc_write_callback(
    void   *ptr,
    size_t  size,
    size_t  nmemb,
    void   *data)
{
    CurlResult *curlres  = (CurlResult *)data;
    size_t      realsize = size * nmemb;

    if (realsize == 0) {
        return(0);
    }

    curlres->buffer = realloc(curlres->buffer, curlres->buflen + realsize + 1);

    if (!curlres->buffer) {

        ERROR( DBCONN_LIB_NAME,
            "Could not get web service response\n"
            " -> memory allocation error\n");

        return(0);
    }

    memcpy(&(curlres->buffer[curlres->buflen]), ptr, realsize);
    curlres->buflen += realsize;
    curlres->buffer[curlres->buflen] = 0;

    return(realsize);
}

static int _wspc_setopts(DBConn *dbconn)
{
    CURL     *curl = (CURL *)dbconn->dbh;
    CURLcode  errnum;

    errnum = curl_easy_setopt(
        curl, CURLOPT_ERRORBUFFER, _CurlErrorString);

    if (errnum != 0) {

        WSPC_ERROR(errnum,
            "Could not set CURLOPT_ERRORBUFFER\n");

        return(0);
    }

    errnum = curl_easy_setopt(
        curl, CURLOPT_WRITEFUNCTION, _wspc_write_callback);

    if (errnum != 0) {

        WSPC_ERROR(errnum,
            "Could not set CURLOPT_WRITEFUNCTION\n");

        return(0);
    }

    return(1);
}

static const char *_wspc_get_url(DBConn *dbconn)
{
    CURL     *curl = (CURL *)dbconn->dbh;
    CURLcode  errnum;
    char     *url;

    errnum = curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &url);
    if (errnum != 0) {
        return((const char *)NULL);
    }

    return((const char *)url);
}

static DBStatus _wspc_set_url(
    DBConn      *dbconn,
    const char  *command,
    int          nparams,
    const char **params,
    char       **url)
{
    CURL       *curl = (CURL *)dbconn->dbh;
    CURLcode    errnum;
    char       *cmd_copy;
    const char *sp_name;
    char       *strp;
    size_t      length;
    char       *params_string;
    char       *encoded_params;
    size_t      encoded_strlen;
    int         i;

    *url = (char *)NULL;

    /* Check if this is an SQL SELECT command */

    cmd_copy = strdup(command);

    if (!cmd_copy) {

        ERROR( DBCONN_LIB_NAME,
            "Could not create URL for: '%s'\n"
            " -> memory allocation error\n",
            command);

        return(DB_MEM_ERROR);
    }

    if (strncasecmp(cmd_copy, "SELECT ", 7) != 0) {
        sp_name = cmd_copy;
    }
    else {

        strp = cmd_copy + 7;

        while (*strp == ' ' || *strp == '*') strp++;

        if (strncasecmp(strp, "FROM ", 5) == 0) {
            strp += 5;
            while (*strp == ' ') strp++;
        }

        sp_name = strp;

        while (*strp != ' ' && *strp != '(') {

            if (*strp == '\0') {

                ERROR( DBCONN_LIB_NAME,
                    "Could not create URL for: '%s'\n"
                    " -> stored procedure name not found in SQL statement\n",
                    command);

                free(cmd_copy);
                return(DB_ERROR);
            }

            strp++;
        }

        *strp = '\0';
    }

    /* Create parameter string */

    length = 8;

    for (i = 0; i < nparams; i++) {
        if (params[i]) {
            length += strlen(params[i]) + 3;
        }
        else {
            length += 5;
        }
    }

    params_string = (char *)malloc(length * sizeof(char));

    if (!params_string) {

        ERROR( DBCONN_LIB_NAME,
            "Could not create URL for: '%s'\n"
            " -> memory allocation error\n",
            command);

        free(cmd_copy);
        return(DB_MEM_ERROR);
    }

    strp  = params_string;
    strp += sprintf(strp, "[");

    for (i = 0; i < nparams; i++) {

        if (i > 0) {
            strp += sprintf(strp, ",");
        }

        if (params[i]) {
            strp += sprintf(strp, "\"%s\"", params[i]);
        }
        else {
            strp += sprintf(strp, "null");
        }
    }

    strp += sprintf(strp, "]");

    /* Create encoded parameter string */

    encoded_params = curl_easy_escape(curl, params_string, 0);

    if (!encoded_params) {

        ERROR( DBCONN_LIB_NAME,
            "Could not encode parameter string: '%s'\n",
            params_string);

        free(cmd_copy);
        free(params_string);
        return(DB_ERROR);
    }

    free(params_string);

    encoded_strlen = strlen(encoded_params);

    /* Create the URL string
     *
     * <host>?func=<sp_name>&args=["arg1","arg2",null]
     */

    length = strlen(dbconn->db_host)
           + strlen(sp_name) + 16
           + encoded_strlen;

    *url = (char *)malloc(length * sizeof(char));

    if (!(*url)) {

        ERROR( DBCONN_LIB_NAME,
            "Could not create URL for: '%s'\n"
            " -> memory allocation error\n",
            command);

        free(cmd_copy);
        curl_free(encoded_params);
        return(DB_MEM_ERROR);
    }

    if (strchr(dbconn->db_host, '?')) {
        length = sprintf(*url, "%s&func=%s&args=%s",
            dbconn->db_host, sp_name, encoded_params);
    }
    else {
        length = sprintf(*url, "%s?func=%s&args=%s",
            dbconn->db_host, sp_name, encoded_params);
    }

    free(cmd_copy);
    curl_free(encoded_params);

    /* Set the URL string */

    errnum = curl_easy_setopt(curl, CURLOPT_URL, *url);

    if (errnum != 0) {

        WSPC_ERROR(errnum,
            "Could not set CURLOPT_URL to: '%s'\n",
            *url);

        free(*url);
        *url = (char *)NULL;
        return(DB_ERROR);
    }

    return(DB_NO_ERROR);
}

static DBStatus _wspc_query(
    DBConn      *dbconn,
    const char  *command,
    int          nparams,
    const char **params,
    DBResult   **dbres)
{
    CURL       *curl = (CURL *)dbconn->dbh;
    CurlResult *curlres;
    int         errnum;
    DBStatus    status;
    char       *url;
    long        response_code;

    if (dbres) {
        *dbres = (DBResult *)NULL;
    }

    /* Set the URL */

    status = _wspc_set_url(dbconn, command, nparams, params, &url);

    if (status != DB_NO_ERROR) {
        return(status);
    }

    /* Create the result structure */

    curlres = (CurlResult *)calloc(1, sizeof(CurlResult));
    if (!curlres) {

        ERROR( DBCONN_LIB_NAME,
            "Could not create result for: '%s'\n"
            " -> memory allocation error\n",
            url);

        free(url);
        return(DB_MEM_ERROR);
    }

    errnum = curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)curlres);
    if (errnum != 0) {

        WSPC_ERROR(errnum,
            "Could not set CURLOPT_WRITEDATA for: '%s'\n",
            url);

        free(url);
        _wspc_free_curlres(curlres);
        return(DB_ERROR);
    }

    /* Perform the query */

    errnum = curl_easy_perform(curl);
    if (errnum != 0) {

        WSPC_ERROR(errnum,
            "Could not perform query for: '%s'\n",
            url);

        free(url);
        _wspc_free_curlres(curlres);
        return(DB_ERROR);
    }

    errnum = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    if (errnum != 0) {

        WSPC_ERROR(errnum,
            "Could not get http response code for: '%s'\n",
            url);

        free(url);
        _wspc_free_curlres(curlres);
        return(DB_ERROR);
    }

    if (response_code != 200L) {

        ERROR( DBCONN_LIB_NAME,
            "Could not perform query for: '%s'\n"
            " -> http request returned response code %ld\n",
            url, response_code);

        free(url);
        _wspc_free_curlres(curlres);
        return(DB_ERROR);
    }

    /* Create the DBResult */

    if (dbres) {

        status = _wspc_create_dbres(url, curlres, dbres);

        if (status != DB_NO_ERROR) {
            _wspc_free_curlres(curlres);
        }
    }
    else {

        status = DB_NO_ERROR;

        _wspc_free_curlres(curlres);
    }

    free(url);

    return(status);
}

/*******************************************************************************
*  Connection Functions
*/

/**
 *  Initialize the database web service session.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  dbconn - pointer to the database web service session
 *
 *  @return database status:
 *    - DB_NO_ERROR
 *    - DB_MEM_ERROR
 *    - DB_ERROR
 *
 *  @see DBStatus
 */
DBStatus wspc_connect(DBConn *dbconn)
{
    CURLcode errnum;

    /* Cleanup the previous session if one has already been initialized */

    if (dbconn->dbh) {
        wspc_disconnect(dbconn);
    }

    /* Initialize libcurl */

    errnum = curl_global_init(CURL_GLOBAL_ALL);
    if (errnum != 0) {
        WSPC_ERROR(errnum, "Could not initialize libcurl\n");
        return(DB_ERROR);
    }

    /* Initialize the libcurl session */

    dbconn->dbh = (void *)curl_easy_init();

    if (!dbconn->dbh) {
        WSPC_ERROR(errnum, "Could not initialize libcurl session\n");
        curl_global_cleanup();
        return(DB_ERROR);
    }

    /* Set libcurl session options */

    if (!_wspc_setopts(dbconn)) {
        curl_easy_cleanup((CURL *)dbconn->dbh);
        curl_global_cleanup();
        return(DB_ERROR);
    }

    return(DB_NO_ERROR);
}

/**
 *  Cleanup the database web service session.
 *
 *  @param  dbconn - pointer to the database connection
 */
void wspc_disconnect(DBConn *dbconn)
{
    if (dbconn->dbh) {
        curl_easy_cleanup((CURL *)dbconn->dbh);
        curl_global_cleanup();
        dbconn->dbh = (void *)NULL;
    }
}

/**
 *  Check if the database web service session has been initialized.
 *
 *  @param  dbconn - pointer to the database connection
 *
 *  @return
 *    - 1 if connected
 *    - 0 if not connected
 */
int wspc_is_connected(DBConn *dbconn)
{
    if (dbconn->dbh) {
        return(1);
    }

    return(0);
}

/**
 *  Reset the database web service session.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
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
DBStatus wspc_reset(DBConn *dbconn)
{
    if (dbconn->dbh) {

        curl_easy_reset((CURL *)dbconn->dbh);

        if (!_wspc_setopts(dbconn)) {
            return(DB_ERROR);
        }
    }

    return(DB_NO_ERROR);
}

/*******************************************************************************
*  Command Functions
*/

/**
 *  Execute a database stored procedure that has no result.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  dbconn  - pointer to the database connection
 *  @param  command - name of the stored procedure, or the SQL command
 *                    containing the name of the stored procedure
 *  @param  nparams - number of stored procedure arguments
 *  @param  params  - list of stored procedure arguments
 *
 *  @return database status:
 *    - DB_NO_ERROR
 *    - DB_MEM_ERROR
 *    - DB_ERROR
 *
 *  @see DBStatus
 */
DBStatus wspc_exec(
    DBConn      *dbconn,
    const char  *command,
    int          nparams,
    const char **params)
{
    DBStatus status;

    status = _wspc_query(dbconn, command, nparams, params, NULL);

    return(status);
}

/**
 *  Call a database stored procedure that returns a row or table.
 *
 *  The memory used by the database result is dynamically allocated.
 *  It is the responsibility of the calling process to free this
 *  memory using the free method of the DBResult structure.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  Null results from the database are not reported as errors.
 *  It is the responsibility of the calling process to check for
 *  DB_NULL_RESULT and report the error if necessary.
 *
 *  @param  dbconn  - pointer to the database connection
 *  @param  command - name of the stored procedure, or the SQL command
 *                    containing the name of the stored procedure
 *  @param  nparams - number of stored procedure arguments
 *  @param  params  - list of stored procedure arguments
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
DBStatus wspc_query(
    DBConn      *dbconn,
    const char  *command,
    int          nparams,
    const char **params,
    DBResult   **result)
{
    DBStatus status;

    *result = (DBResult *)NULL;
    status  = _wspc_query(dbconn, command, nparams, params, result);

    return(status);
}

/**
 *  Call a database stored procedure that returns a boolean value.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  Null results from the database are not reported as errors.
 *  It is the responsibility of the calling process to check for
 *  DB_NULL_RESULT and report the error if necessary.
 *
 *  @param  dbconn  - pointer to the database connection
 *  @param  command - name of the stored procedure, or the SQL command
 *                    containing the name of the stored procedure
 *  @param  nparams - number of stored procedure arguments
 *  @param  params  - list of stored procedure arguments
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
DBStatus wspc_query_bool(
    DBConn      *dbconn,
    const char  *command,
    int          nparams,
    const char **params,
    int         *result)
{
    DBResult   *dbres = (DBResult *)NULL;;
    DBStatus    status;
    char       *string;
    const char *url;

    *result = 0;

    status  = _wspc_query(dbconn, command, nparams, params, &dbres);

    if (status != DB_NO_ERROR) {
        return(status);
    }

    if (dbres->nrows != 1 || dbres->ncols != 1) {

        url = _wspc_get_url(dbconn);
        if (!url) url = command;

        ERROR( DBCONN_LIB_NAME,
            "Could not create result for: '%s'\n"
            " -> bad result received from boolean query\n",
            url);

        dbres->free(dbres);
        return(DB_BAD_RESULT);
    }

    string = dbres->data[0];

    if (*string == 't') {
        *result = 1;
    }
    else if (*string == 'f') {
        *result = 0;
    }
    else {

        url = _wspc_get_url(dbconn);
        if (!url) url = command;

        ERROR( DBCONN_LIB_NAME,
            "Could not create result for: '%s'\n"
            " -> bad result received from boolean query: '%s'\n",
            url, string);

        dbres->free(dbres);
        return(DB_BAD_RESULT);
    }

    dbres->free(dbres);
    return(DB_NO_ERROR);
}

/**
 *  Call a database stored procedure that returns an integer value.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  Null results from the database are not reported as errors.
 *  It is the responsibility of the calling process to check for
 *  DB_NULL_RESULT and report the error if necessary.
 *
 *  @param  dbconn  - pointer to the database connection
 *  @param  command - name of the stored procedure, or the SQL command
 *                    containing the name of the stored procedure
 *  @param  nparams - number of stored procedure arguments
 *  @param  params  - list of stored procedure arguments
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
DBStatus wspc_query_int(
    DBConn      *dbconn,
    const char  *command,
    int          nparams,
    const char **params,
    int         *result)
{
    DBStatus status;
    long     value;

    status = wspc_query_long(dbconn, command, nparams, params, &value);

    *result = (int)value;

    return(status);
}

/**
 *  Call a database stored procedure that returns an integer value.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  Null results from the database are not reported as errors.
 *  It is the responsibility of the calling process to check for
 *  DB_NULL_RESULT and report the error if necessary.
 *
 *  @param  dbconn  - pointer to the database connection
 *  @param  command - name of the stored procedure, or the SQL command
 *                    containing the name of the stored procedure
 *  @param  nparams - number of stored procedure arguments
 *  @param  params  - list of stored procedure arguments
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
DBStatus wspc_query_long(
    DBConn      *dbconn,
    const char  *command,
    int          nparams,
    const char **params,
    long        *result)
{
    DBResult   *dbres = (DBResult *)NULL;;
    DBStatus    status;
    char       *endptr;
    char       *string;
    const char *url;

    *result = 0;

    status  = _wspc_query(dbconn, command, nparams, params, &dbres);

    if (status != DB_NO_ERROR) {
        return(status);
    }

    if (dbres->nrows != 1 || dbres->ncols != 1) {

        url = _wspc_get_url(dbconn);
        if (!url) url = command;

        ERROR( DBCONN_LIB_NAME,
            "Could not create result for: '%s'\n"
            " -> bad result received from integer query\n",
            url);

        dbres->free(dbres);
        return(DB_BAD_RESULT);
    }

    string  = dbres->data[0];
    errno   = 0;
    *result = strtol(string, &endptr, 10);

    if (errno || endptr == string) {

        url = _wspc_get_url(dbconn);
        if (!url) url = command;

        ERROR( DBCONN_LIB_NAME,
            "Could not create result for: '%s'\n"
            " -> bad result received from integer query: '%s'\n",
            url, string);

        *result = 0;
        dbres->free(dbres);
        return(DB_BAD_RESULT);
    }

    dbres->free(dbres);
    return(DB_NO_ERROR);
}

/**
 *  Call a database stored procedure that returns a real number.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  Null results from the database are not reported as errors.
 *  It is the responsibility of the calling process to check for
 *  DB_NULL_RESULT and report the error if necessary.
 *
 *  @param  dbconn  - pointer to the database connection
 *  @param  command - name of the stored procedure, or the SQL command
 *                    containing the name of the stored procedure
 *  @param  nparams - number of stored procedure arguments
 *  @param  params  - list of stored procedure arguments
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
DBStatus wspc_query_float(
    DBConn      *dbconn,
    const char  *command,
    int          nparams,
    const char **params,
    float      *result)
{
    DBStatus status;
    double   value;

    status = wspc_query_double(dbconn, command, nparams, params, &value);

    *result = (float)value;

    return(status);
}

/**
 *  Call a database stored procedure that returns a real number.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  Null results from the database are not reported as errors.
 *  It is the responsibility of the calling process to check for
 *  DB_NULL_RESULT and report the error if necessary.
 *
 *  @param  dbconn  - pointer to the database connection
 *  @param  command - name of the stored procedure, or the SQL command
 *                    containing the name of the stored procedure
 *  @param  nparams - number of stored procedure arguments
 *  @param  params  - list of stored procedure arguments
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
DBStatus wspc_query_double(
    DBConn      *dbconn,
    const char  *command,
    int          nparams,
    const char **params,
    double      *result)
{
    DBResult   *dbres = (DBResult *)NULL;;
    DBStatus    status;
    char       *endptr;
    char       *string;
    const char *url;

    *result = 0;

    status  = _wspc_query(dbconn, command, nparams, params, &dbres);

    if (status != DB_NO_ERROR) {
        return(status);
    }

    if (dbres->nrows != 1 || dbres->ncols != 1) {

        url = _wspc_get_url(dbconn);
        if (!url) url = command;

        ERROR( DBCONN_LIB_NAME,
            "Could not create result for: '%s'\n"
            " -> bad result received from real number query\n",
            url);

        dbres->free(dbres);
        return(DB_BAD_RESULT);
    }

    string  = dbres->data[0];
    errno   = 0;
    *result = strtod(string, &endptr);

    if (errno || endptr == string) {

        url = _wspc_get_url(dbconn);
        if (!url) url = command;

        ERROR( DBCONN_LIB_NAME,
            "Could not create result for: '%s'\n"
            " -> bad result received from real number query: '%s'\n",
            url, string);

        *result = 0;
        dbres->free(dbres);
        return(DB_BAD_RESULT);
    }

    dbres->free(dbres);
    return(DB_NO_ERROR);
}

/**
 *  Call a database stored procedure that returns a text string.
 *
 *  The memory used by the result string is dynamically allocated.
 *  It is the responsibility of the calling process to free this
 *  memory when it is no longer needed.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  Null results from the database are not reported as errors.
 *  It is the responsibility of the calling process to check for
 *  DB_NULL_RESULT and report the error if necessary.
 *
 *  @param  dbconn  - pointer to the database connection
 *  @param  command - name of the stored procedure, or the SQL command
 *                    containing the name of the stored procedure
 *  @param  nparams - number of stored procedure arguments
 *  @param  params  - list of stored procedure arguments
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
DBStatus wspc_query_text(
    DBConn      *dbconn,
    const char  *command,
    int          nparams,
    const char **params,
    char       **result)
{
    DBResult   *dbres = (DBResult *)NULL;;
    DBStatus    status;
    const char *url;

    *result = (char *)NULL;

    status  = _wspc_query(dbconn, command, nparams, params, &dbres);

    if (status != DB_NO_ERROR) {
        return(status);
    }

    if (dbres->nrows != 1 || dbres->ncols != 1) {

        url = _wspc_get_url(dbconn);
        if (!url) url = command;

        ERROR( DBCONN_LIB_NAME,
            "Could not create result for: '%s'\n"
            " -> bad result received from text query\n",
            url);

        dbres->free(dbres);
        return(DB_BAD_RESULT);
    }

    *result = strdup(dbres->data[0]);

    if (!(*result)) {

        url = _wspc_get_url(dbconn);
        if (!url) url = command;

        ERROR( DBCONN_LIB_NAME,
            "Could not create result for: '%s'\n"
            " -> memory allocation error\n",
            url);

        dbres->free(dbres);
        return(DB_MEM_ERROR);
    }

    dbres->free(dbres);
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
char *wspc_bool_to_text(int bval, char *text)
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
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  text - text string
 *  @param  bval - output: boolean value (1 = TRUE, 0 = FALSE)
 *
 *  @return
 *    - the bval argument
 *    - NULL if the text string is not a valid boolean value
 */
int *wspc_text_to_bool(const char *text, int *bval)
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
 *  This function will convert seconds since 1970 into a time string
 *  that can be used in database queries.
 *
 *  The text argument must point to a string that can hold at least
 *  32 characters.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  time   - seconds since 1970
 *  @param  text   - output: time string
 *
 *  @return
 *    - the text argument
 *    - NULL if an error occurred
 */
char *wspc_time_to_text(time_t time, char *text)
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
 *  This function will convert a time string returned by a database
 *  query into seconds since 1970.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  text   - time string
 *  @param  time   - output: seconds since 1970
 *
 *  @return
 *    - the time argument
 *    - NULL if an error occurred
 */
time_t *wspc_text_to_time(const char *text, time_t *time)
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
 *  The text argument must point to a string that can hold at least
 *  32 characters.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  tval - pointer to the timeval
 *  @param  text - output: time string
 *
 *  @return
 *    - the text argument
 *    - NULL if an error occurred
 */
char *wspc_timeval_to_text(const timeval_t *tval, char *text)
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
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  text - time string
 *  @param  tval - output: timeval
 *
 *  @return
 *    - the tval argument
 *    - NULL if an error occurred
 */
timeval_t *wspc_text_to_timeval(
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
