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
*    $Revision: 55592 $
*    $Author: ermold $
*    $Date: 2014-07-19 23:21:50 +0000 (Sat, 19 Jul 2014) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file dsenv.c
 *  DataSystem Environment Functions.
 */

#include "armutils.h"

/*******************************************************************************
 *  Private Functions
 */
/** @privatesection */

/**
 *  Create the full path to a datastream directory.
 *
 *  This function will return a dynamically allocated string of the form:
 *
 *      $(root_dir)/$(site)/$(site)$(name)$(facility).$(level)
 *
 *  The calling process is responsible for freeing this value when it is
 *  no longer needed.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  root_dir - the root directory
 *  @param  site     - site name
 *  @param  facility - facility name
 *  @param  name     - datastream class name or process name
 *  @param  level    - datastream class level, or NULL to exclude the level
 *                     from the datastream directory name.
 *
 *  @return
 *    -  full path to the datastream directory
 *    -  0 if a memory allocation error occurred
 */
static char *dsenv_create_full_path(
    const char  *root_dir,
    const char  *site,
    const char  *facility,
    const char  *name,
    const char  *level)
{
    char *path;

    if (level) {

        path = msngr_create_string("%s/%s/%s%s%s.%s",
            root_dir, site, site, name, facility, level);
    }
    else {

        path = msngr_create_string("%s/%s/%s%s%s",
            root_dir, site, site, name, facility);
    }

    if (!path) {

        if (level) {

            ERROR( ARMUTILS_LIB_NAME,
                "Could not get datastream path for: %s, %s%s%s.%s\n"
                " -> memory allocation error\n",
                root_dir, site, name, facility, level);
        }
        else {

            ERROR( ARMUTILS_LIB_NAME,
                "Could not get datastream path for: %s, %s%s%s\n"
                " -> memory allocation error\n",
                root_dir, site, name, facility);
        }

        return((char *)NULL);
    }

    return(path);
}

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Get hostname.
 *
 *  The returned value is statically defined in this function and must
 *  *not* be freed by the calling process.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @return
 *    - pointer to the hostname
 *    - NULL if an error occurred
 */
char *dsenv_get_hostname(void)
{
    static char hostname[256];

    if (!hostname[0]) {
        if (gethostname(hostname, 255) == -1) {

            ERROR( ARMUTILS_LIB_NAME,
                "Could not get hostname: %s\n", strerror(errno));

            return((char *)NULL);
        }
    }

    return(hostname);
}

/**
 *  Get environment variable.
 *
 *  The output value is dynamically allocated and must be freed by the
 *  calling process.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  name  - name of the environment variable
 *  @param  value - output: pointer to the value of the environment variable
 *
 *  @return
 *    -  1 if the environment variable was found
 *    -  0 if the environment variable was not found
 *    - -1 if a memory allocation error occurred
 */
int dsenv_getenv(const char *name, char **value)
{
    const char *env_value = getenv(name);

    if (!env_value) {
        *value = (char *)NULL;
        return(0);
    }

    *value = strdup(env_value);
    if (!(*value)) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not get environment variable: %s\n"
            " -> memory allocation error\n", name);

        return(-1);
    }

    return(1);
}

/**
 *  Set environment variable.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  name   - name of the environment variable
 *  @param  format - format string (see printf)
 *  @param  ...    - arguments for the format string
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int dsenv_setenv(const char *name, const char *format, ...)
{
    va_list  args;
    char    *value;
    int      status;

    va_start(args, format);

    value = msngr_format_va_list(format, args);

    if (!value) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not set environment variable: %s\n"
            " -> memory allocation error\n", name);

        return(0);
    }

    va_end(args);

    status = setenv(name, value, 1);
    free(value);

    if (status < 0) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not set environment variable: %s\n"
            " -> %s\n", name, strerror(errno));

        return(0);
    }

    return(1);
}

/**
 *  Get the root path of the apps conf directory.
 *
 *  This function will return a dynamically allocated string of the form:
 *
 *      $(PROC_TYPE_HOME)/conf/$(proc_type)/$(proc_name)
 *
 *  The calling process is responsible for freeing this string when it is
 *  no longer needed.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  proc_name - the name of the process
 *  @param  proc_type - the process type (i.e. Ingest, VAP)
 *  @param  path      - output: pointer to the root path of the
 *                              apps conf directory
 *
 *  @return
 *    -  1 if successful
 *    -  0 if the PROC_TYPE_HOME environment variable was not found
 *    - -1 if a memory allocation error occurred
 */
int dsenv_get_apps_conf_root(
    const char  *proc_name,
    const char  *proc_type,
    char       **path)
{
    char *env_var = (char *)NULL;
    char *lc_type = (char *)NULL;
    char *root_dir;
    int   typelen;
    int   i;

    typelen = strlen(proc_type);

    if (!(env_var = malloc((typelen + 6) * sizeof(char))) ||
        !(lc_type = malloc((typelen + 1) * sizeof(char)))) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not get apps conf path for: %s, %s\n"
            " -> memory allocation error\n",
            proc_name, proc_type);

        if (env_var) free(env_var);

        return(-1);
    }

    for (i = 0; i < typelen; ++i) {
        env_var[i] = toupper(proc_type[i]);
        lc_type[i] = tolower(proc_type[i]);
    }

    lc_type[i] = '\0';
    strcpy(&(env_var[i]), "_HOME");

    root_dir = getenv(env_var);
    free(env_var);

    if (!root_dir) {
        *path = (char *)NULL;
        free(lc_type);
        return(0);
    }

    *path = msngr_create_string("%s/conf/%s/%s",
        root_dir, lc_type, proc_name);

    free(lc_type);

    if (!*path) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not get apps conf path for: %s, %s\n"
            " -> memory allocation error\n",
            proc_name, proc_type);

        return(-1);
    }

    return(1);
}

/**
 *  Get the apps conf directory for a datastream.
 *
 *  This function will return a dynamically allocated string of the form:
 *
 *      $(PROC_TYPE_HOME)/conf/$(proc_type)/$(proc_name)/$(site)/$(site)$(name)$(facility).$(level)
 *
 *  The calling process is responsible for freeing this string when it is
 *  no longer needed.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  proc_name - the name of the process
 *  @param  proc_type - the process type (i.e. Ingest, VAP)
 *  @param  site      - site name
 *  @param  facility  - facility name
 *  @param  name      - datastream class name
 *  @param  level     - datastream class level, or NULL to exclude the level
 *                      from the datastream directory name.
 *  @param  path      - output: pointer to the path of the
 *                              apps conf directory
 *
 *  @return
 *    -  1 if successful
 *    -  0 if the PROC_TYPE_HOME environment variable was not found
 *    - -1 if a memory allocation error occurred
 */
int dsenv_get_apps_conf_dir(
    const char  *proc_name,
    const char  *proc_type,
    const char  *site,
    const char  *facility,
    const char  *name,
    const char  *level,
    char       **path)
{
    char *root_dir;
    int   status;

    status = dsenv_get_apps_conf_root(proc_name, proc_type, &root_dir);

    if (status <= 0) {
        *path = (char *)NULL;
        return(0);
    }

    *path = dsenv_create_full_path(
        root_dir, site, facility, name, level);

    free(root_dir);

    if (!*path) {
        return(-1);
    }

    return(1);
}

/**
 *  Get the root path of the data collection directory.
 *
 *  This function returns a dynamically allocated copy of the COLLECTION_DATA
 *  environment variable. The calling process is responsible for freeing this
 *  string when it is no longer needed.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  path - output: pointer to the root path of the
 *                         data collection directory
 *
 *  @return
 *    -  1 if successful
 *    -  0 if the COLLECTION_DATA environment variable was not found
 *    - -1 if a memory allocation error occurred
 */
int dsenv_get_collection_root(char **path)
{
    return(dsenv_getenv("COLLECTION_DATA", path));
}

/**
 *  Get the data collection directory for a datastream.
 *
 *  This function will return a dynamically allocated string of the form:
 *
 *      $(COLLECTION_DATA)/$(site)/$(site)$(name)$(facility).$(level)
 *
 *  The calling process is responsible for freeing this string when it is
 *  no longer needed.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  site     - site name
 *  @param  facility - facility name
 *  @param  name     - datastream class name
 *  @param  level    - datastream class level
 *  @param  path     - output: pointer to the data collection directory
 *
 *  @return
 *    -  1 if successful
 *    -  0 if the COLLECTION_DATA environment variable was not found
 *    - -1 if a memory allocation error occurred
 */
int dsenv_get_collection_dir(
    const char  *site,
    const char  *facility,
    const char  *name,
    const char  *level,
    char       **path)
{
    const char *root_dir = getenv("COLLECTION_DATA");

    if (!root_dir) {
        *path = (char *)NULL;
        return(0);
    }

    *path = dsenv_create_full_path(
        root_dir, site, facility, name, level);

    if (!*path) {
        return(-1);
    }

    return(1);
}

/**
 *  Get the root path of the data conf directory.
 *
 *  This function returns a dynamically allocated copy of the CONF_DATA
 *  environment variable. The calling process is responsible for freeing this
 *  string when it is no longer needed.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  path - output: pointer to the root path of the
 *                         data conf directory
 *
 *  @return
 *    -  1 if successful
 *    -  0 if the CONF_DATA environment variable was not found
 *    - -1 if a memory allocation error occurred
 */
int dsenv_get_data_conf_root(char **path)
{
    return(dsenv_getenv("CONF_DATA", path));
}

/**
 *  Get the data conf directory for a datastream.
 *
 *  This function will return a dynamically allocated string of the form:
 *
 *      $(CONF_DATA)/$(site)/$(site)$(name)$(facility).$(level)
 *
 *  The calling process is responsible for freeing this string when it is
 *  no longer needed.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  site     - site name
 *  @param  facility - facility name
 *  @param  name     - datastream class name or process name
 *  @param  level    - datastream class level, or NULL to exclude the level
 *                     from the datastream directory name.
 *  @param  path     - output: pointer to the data conf directory
 *
 *  @return
 *    -  1 if successful
 *    -  0 if the CONF_DATA environment variable was not found
 *    - -1 if a memory allocation error occurred
 */
int dsenv_get_data_conf_dir(
    const char  *site,
    const char  *facility,
    const char  *name,
    const char  *level,
    char       **path)
{
    const char *root_dir = getenv("CONF_DATA");

    if (!root_dir) {
        *path = (char *)NULL;
        return(0);
    }

    *path = dsenv_create_full_path(
        root_dir, site, facility, name, level);

    if (!*path) {
        return(-1);
    }

    return(1);
}

/**
 *  Get the root path of the datastream directory.
 *
 *  This function returns a dynamically allocated copy of the DATASTREAM_DATA
 *  environment variable. The calling process is responsible for freeing this
 *  string when it is no longer needed.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  path - output: pointer to the root path of the
 *                         datastream directory
 *
 *  @return
 *    -  1 if successful
 *    -  0 if the DATASTREAM_DATA environment variable was not found
 *    - -1 if a memory allocation error occurred
 */
int dsenv_get_datastream_root(char **path)
{
    return(dsenv_getenv("DATASTREAM_DATA", path));
}

/**
 *  Get the datastream directory for a datastream.
 *
 *  This function will return a dynamically allocated string of the form:
 *
 *      $(DATASTREAM_DATA)/$(site)/$(site)$(name)$(facility).$(level)
 *
 *  The calling process is responsible for freeing this string when it is
 *  no longer needed.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  site     - site name
 *  @param  facility - facility name
 *  @param  name     - datastream class name
 *  @param  level    - datastream class level
 *  @param  path     - output: pointer to the datastream directory
 *
 *  @return
 *    -  1 if successful
 *    -  0 if the DATASTREAM_DATA environment variable was not found
 *    - -1 if a memory allocation error occurred
 */
int dsenv_get_datastream_dir(
    const char  *site,
    const char  *facility,
    const char  *name,
    const char  *level,
    char       **path)
{
    const char *root_dir = getenv("DATASTREAM_DATA");

    if (!root_dir) {
        *path = (char *)NULL;
        return(0);
    }

    *path = dsenv_create_full_path(
        root_dir, site, facility, name, level);

    if (!*path) {
        return(-1);
    }

    return(1);
}

/**
 *  Get the root path of the input datastream directory.
 *
 *  This function returns a dynamically allocated copy of the first enviroment
 *  variable found in the following search order:
 *
 *   - DATASTREAM_DATA_IN
 *   - DATASTREAM_DATA
 *
 *  The calling process is responsible for freeing the returned string when
 *  it is no longer needed.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  path - output: pointer to the root path of the
 *                         datastream directory
 *
 *  @return
 *    -  1 if successful
 *    -  0 none of the environment variables were found
 *    - -1 if a memory allocation error occurred
 */
int dsenv_get_input_datastream_root(char **path)
{
    int status = dsenv_getenv("DATASTREAM_DATA_IN", path);
    
    if (status == 0) {
        status = dsenv_getenv("DATASTREAM_DATA", path);
    }

    return(status);
}

/**
 *  Get the input datastream directory.
 *
 *  This function will return a dynamically allocated string of the form:
 *
 *      root_dir/$(site)/$(site)$(name)$(facility).$(level)
 *
 *  where root_dir is the value of the first enviroment variable found in
 *  the following search order:
 *
 *   - DATASTREAM_DATA_IN
 *   - DATASTREAM_DATA
 *
 *  The calling process is responsible for freeing this string when it is
 *  no longer needed.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  site     - site name
 *  @param  facility - facility name
 *  @param  name     - datastream class name
 *  @param  level    - datastream class level
 *  @param  path     - output: pointer to the datastream directory
 *
 *  @return
 *    -  1 if successful
 *    -  0 none of the environment variables were found
 *    - -1 if a memory allocation error occurred
 */
int dsenv_get_input_datastream_dir(
    const char  *site,
    const char  *facility,
    const char  *name,
    const char  *level,
    char       **path)
{
    const char *root_dir = getenv("DATASTREAM_DATA_IN");

    if (!root_dir) {
        root_dir = getenv("DATASTREAM_DATA");
        if (!root_dir) {
            *path = (char *)NULL;
            return(0);
        }
    }

    *path = dsenv_create_full_path(
        root_dir, site, facility, name, level);

    if (!*path) {
        return(-1);
    }

    return(1);
}

/**
 *  Get the root path of the output datastream directory.
 *
 *  This function returns a dynamically allocated copy of the first enviroment
 *  variable found in the following search order:
 *
 *   - DATASTREAM_DATA_OUT
 *   - DATASTREAM_DATA
 *
 *  The calling process is responsible for freeing the returned string when
 *  it is no longer needed.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  path - output: pointer to the root path of the
 *                         datastream directory
 *
 *  @return
 *    -  1 if successful
 *    -  0 none of the environment variables were found
 *    - -1 if a memory allocation error occurred
 */
int dsenv_get_output_datastream_root(char **path)
{
    int status = dsenv_getenv("DATASTREAM_DATA_OUT", path);

    if (status == 0) {
        status = dsenv_getenv("DATASTREAM_DATA", path);
    }

    return(status);
}

/**
 *  Get the output datastream directory.
 *
 *  This function will return a dynamically allocated string of the form:
 *
 *      root_dir/$(site)/$(site)$(name)$(facility).$(level)
 *
 *  where root_dir is the value of the first enviroment variable found in
 *  the following search order:
 *
 *   - DATASTREAM_DATA_OUT
 *   - DATASTREAM_DATA
 *
 *  The calling process is responsible for freeing this string when it is
 *  no longer needed.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  site     - site name
 *  @param  facility - facility name
 *  @param  name     - datastream class name
 *  @param  level    - datastream class level
 *  @param  path     - output: pointer to the datastream directory
 *
 *  @return
 *    -  1 if successful
 *    -  0 none of the environment variables were found
 *    - -1 if a memory allocation error occurred
 */
int dsenv_get_output_datastream_dir(
    const char  *site,
    const char  *facility,
    const char  *name,
    const char  *level,
    char       **path)
{
    const char *root_dir = getenv("DATASTREAM_DATA_OUT");

    if (!root_dir) {
        root_dir = getenv("DATASTREAM_DATA");
        if (!root_dir) {
            *path = (char *)NULL;
            return(0);
        }
    }

    *path = dsenv_create_full_path(
        root_dir, site, facility, name, level);

    if (!*path) {
        return(-1);
    }

    return(1);
}

/**
 *  Get the root path of the data tmp directory.
 *
 *  This function returns a dynamically allocated copy of the TMP_DATA
 *  environment variable. The calling process is responsible for freeing this
 *  string when it is no longer needed.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  path - output: pointer to the root path of the
 *                         data tmp directory
 *
 *  @return
 *    -  1 if successful
 *    -  0 if the TMP_DATA environment variable was not found
 *    - -1 if a memory allocation error occurred
 */
int dsenv_get_tmp_root(char **path)
{
    return(dsenv_getenv("TMP_DATA", path));
}

/**
 *  Get the root path of the data logs directory.
 *
 *  This function returns a dynamically allocated copy of the LOGS_DATA
 *  environment variable. The calling process is responsible for freeing this
 *  string when it is no longer needed.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  path - output: pointer to the root path of the
 *                         data logs directory
 *
 *  @return
 *    -  1 if successful
 *    -  0 if the LOGS_DATA environment variable was not found
 *    - -1 if a memory allocation error occurred
 */
int dsenv_get_logs_root(char **path)
{
    return(dsenv_getenv("LOGS_DATA", path));
}

/**
 *  Get the root path of the quicklook directory.
 *
 *  This function returns a dynamically allocated copy of the QUICKLOOK_DATA
 *  environment variable. The calling process is responsible for freeing this
 *  string when it is no longer needed.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  path - output: pointer to the root path of the
 *                         quicklook directory
 *
 *  @return
 *    -  1 if successful
 *    -  0 if the QUICKLOOK_DATA environment variable was not found
 *    - -1 if a memory allocation error occurred
 */
int dsenv_get_quicklook_root(char **path)
{
    return(dsenv_getenv("QUICKLOOK_DATA", path));
}
