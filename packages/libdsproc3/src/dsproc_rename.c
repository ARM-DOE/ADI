/*******************************************************************************
*
*  Copyright © 2014, Battelle Memorial Institute
*  All rights reserved.
*
********************************************************************************
*
*  Author:
*     name:  Brian Ermold
*     phone: (509) 375-2277
*     email: brian.ermold@pnl.gov
*
*******************************************************************************/

/** @file dsproc_rename.c
 *  File Renaming Functions.
 */

#include "dsproc3.h"
#include "dsproc_private.h"

extern DSProc *_DSProc; /**< Internal DSProc structure */

/** @privatesection */

/*******************************************************************************
 *  Static Functions Visible Only To This Module
 */


/**
 *  Static: Get a unique destination file name.
 *
 *  This function will incrementally append ',#' to the end of the destination
 *  file name until a unique name is found.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  src_file  - full path to the input file
 *  @param  dest_file - input/output: full path to the output file
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
static int _dsproc_get_unique_file_name(
    const char *src_file,
    char       *dest_file)
{
    char *vp = dest_file + strlen(dest_file);
    int   v;

    for (v = 1; ; ++v) {

        sprintf(vp, ",%d", v);

        if (access(dest_file, F_OK) != 0) {

            if (errno == ENOENT) {
                break;
            }
            else {

                ERROR( DSPROC_LIB_NAME,
                    "Could not determine unique name for destination file.\n"
                    "Access error checking for existance of file: %s\n"
                    " -> %s\n", dest_file, strerror(errno));

                dsproc_set_status(DSPROC_EACCESS);
                return(0);
            }
        }
    }

    return(1);
}

/**
 *  Static: Rename a data file.
 *
 *  This function will rename a raw data file into the datastream directory
 *  using the datastream name and begin_time to give it a fully qualified
 *  ARM name.
 *
 *  The begin_time will be validated using:
 *
 *      dsproc_validate_datastream_data_time()
 *
 *  If the end_time is specified, this function will verify that it is greater
 *  than the begin_time and is not in the future. If only one record was found
 *  in the raw file, the end_time argument must be set to NULL.
 *
 *  If the output file exists and has the same MD5 as the input file,
 *  the input file will be removed and a warning message will be generated.
 *
 *  If the output file exists and has a different MD5 than the input
 *  file, the rename will fail.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  ds_id      - datastream ID
 *  @param  file_path  - path to the directory the file is in
 *  @param  file_name  - name of the file to rename
 *  @param  begin_time - time of the first record in the file
 *  @param  end_time   - time of the last record in the file
 *  @param  extension  - extension to use when renaming the file, or NULL to
 *                       use the default extension for the datastream format.
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
static int _dsproc_rename(
    int              ds_id,
    const char      *file_path,
    const char      *file_name,
    const timeval_t *begin_time,
    const timeval_t *end_time,
    const char      *extension)
{
    DataStream *ds         = _DSProc->datastreams[ds_id];
    time_t      now        = time(NULL);
    int         force_mode = dsproc_get_force_mode();
    char        src_file[PATH_MAX];
    struct stat src_stats;
    char        src_md5[64];
    char        done_dir[PATH_MAX];
    char       *dest_path;
    char        dest_name[256];
    char        dest_file[PATH_MAX];
    char        dest_md5[64];
    struct tm   time_stamp_tm;
    char        time_stamp[32];
    char        time_string1[32];
    char        time_string2[32];
    const char *preserve;
    int         dot_count;
    char        rename_error[2*PATH_MAX];
    int         rename_file;
    int         force_rename = 0;

    memset(&time_stamp_tm, 0, sizeof(struct tm));

    DEBUG_LV1( DSPROC_LIB_NAME,
        "%s: Renaming file: %s/%s\n",
        ds->name, file_path, file_name);

    /************************************************************
    *  Get source file stats
    *************************************************************/

    snprintf(src_file, PATH_MAX, "%s/%s", file_path, file_name);

    if (access(src_file, F_OK) != 0) {

        if (errno == ENOENT) {

            ERROR( DSPROC_LIB_NAME,
                "Could not rename file: %s\n"
                " -> file does not exist\n", src_file);

            dsproc_set_status(DSPROC_ENOSRCFILE);
        }
        else {

            ERROR( DSPROC_LIB_NAME,
                "Could not access file: %s\n"
                " -> %s\n", src_file, strerror(errno));

            dsproc_set_status(DSPROC_EACCESS);
        }

        return(0);
    }

    if (stat(src_file, &src_stats) < 0) {

        ERROR( DSPROC_LIB_NAME,
            "Could not rename file: %s\n"
            " -> stat error: %s\n", src_file, strerror(errno));

        dsproc_set_status(DSPROC_EFILESTATS);
        return(0);
    }

    /************************************************************
    *  Validate datastream times
    *************************************************************/

    /* validate begin time */

    if (!force_mode) {

        if (!begin_time || !begin_time->tv_sec) {

            ERROR( DSPROC_LIB_NAME,
                "Could not rename file: %s\n"
                " -> file time not specified\n", src_file);

            dsproc_set_status(DSPROC_ENOFILETIME);
            return(0);
        }

        if (!dsproc_validate_datastream_data_time(ds_id, begin_time)) {

            ERROR( DSPROC_LIB_NAME,
                "Could not rename file: %s\n"
                " -> time validation error\n", src_file);

            return(0);
        }

        /* validate end time */

        if (end_time && end_time->tv_sec) {

            if (TV_LTEQ(*end_time, *begin_time)) {

                format_timeval(begin_time, time_string2);
                format_timeval(end_time,   time_string1);

                ERROR( DSPROC_LIB_NAME,
                    "Could not rename file: %s\n"
                    " -> end time '%s' <= begin time '%s'\n",
                    src_file, time_string1, time_string2);

                dsproc_set_status(DSPROC_ETIMEORDER);
                return(0);
            }
            else if (end_time->tv_sec > now) {

                format_timeval(end_time, time_string1);
                format_secs1970(now, time_string2);

                ERROR( DSPROC_LIB_NAME,
                    "Could not rename file: %s\n"
                    " -> data time '%s' is in the future (current time is: %s)\n",
                    src_file, time_string1, time_string2);

                dsproc_disable(DSPROC_EFUTURETIME);
                return(0);
            }
        }
    }

    /************************************************************
    *  Determine the portion of the original file name to preserve
    *************************************************************/

    preserve = (const char *)NULL;

    if (ds->preserve_dots > 0) {

        dot_count = 0;

        for (preserve = file_name + strlen(file_name) - 1;
             preserve != file_name;
             preserve--) {

            if (*preserve == '.') {
                dot_count++;
                if (dot_count == ds->preserve_dots) {
                    preserve++;
                    break;
                }
            }
        }
    }

    /************************************************************
    *  Get the full path to the destination directory
    *************************************************************/

    snprintf(done_dir, PATH_MAX, "%s/.done", file_path);

    if (access(done_dir, F_OK) == 0) {
        dest_path = done_dir;

        /* When moving files to the .done directory we can force the rename
         * if the destination file with a different MD5 exists. */

        force_rename = 1;
    }
    else if (errno != ENOENT) {

        ERROR( DSPROC_LIB_NAME,
            "Could not access directory: %s\n"
            " -> %s\n", done_dir, strerror(errno));

        dsproc_set_status(DSPROC_EACCESS);
        return(0);
    }
    else {

        dest_path = ds->dir->path;

        /* Make sure the destination directory exists */

        if (!make_path(dest_path, 0775)) {

            ERROR( DSPROC_LIB_NAME,
                "Could not rename file: %s\n"
                " -> make_path failed for: %s\n",
                src_file, dest_path);

            dsproc_set_status(DSPROC_EDESTDIRMAKE);
            return(0);
        }
    }

    /************************************************************
    *  Create the time stamp for the destination file
    *************************************************************/

    if (!gmtime_r(&begin_time->tv_sec, &time_stamp_tm)) {

        ERROR( DSPROC_LIB_NAME,
            "Could not rename file: %s\n"
            " -> gmtime error: %s\n",
            src_file, strerror(errno));

        dsproc_set_status(DSPROC_ETIMECALC);
        return(0);
    }

    strftime(time_stamp, 32, "%Y%m%d.%H%M%S", &time_stamp_tm);

    /************************************************************
    *  Create the output file name
    *************************************************************/

    if (!extension) {
        extension = ds->extension;
    }

    if (preserve) {
        snprintf(dest_name, 256, "%s.%s.%s.%s",
            ds->name, time_stamp, extension, preserve);
    }
    else {
        snprintf(dest_name, 256, "%s.%s.%s",
            ds->name, time_stamp, extension);
    }

    snprintf(dest_file, PATH_MAX, "%s/%s", dest_path, dest_name);

    /************************************************************
    *  Check if the output file already exists
    *************************************************************/

    rename_file = 1;

    if (access(dest_file, F_OK) == 0) {

        sprintf(rename_error,
            "Found existing destination file while renaming source file:\n"
            " -> from: %s\n"
            " -> to:   %s\n",
            src_file, dest_file);

        /* Check the MD5s */

        if (!file_get_md5(src_file, src_md5)) {

            ERROR( DSPROC_LIB_NAME,
                "%s -> could not get source file MD5\n",
                rename_error);

            dsproc_set_status(DSPROC_EFILEMD5);
            return(0);
        }

        if (!file_get_md5(dest_file, dest_md5)) {

            ERROR( DSPROC_LIB_NAME,
                "%s -> could not get destination file MD5\n",
                rename_error);

            dsproc_set_status(DSPROC_EFILEMD5);
            return(0);
        }

        if (strcmp(src_md5, dest_md5) == 0) {

            /* The MD5s match so delete the input file */

            if (unlink(src_file) < 0) {

                ERROR( DSPROC_LIB_NAME,
                    "%s"
                    " -> source and destination files have matching MD5s\n"
                    " -> could not delete source file: %s\n",
                    rename_error, strerror(errno));

                dsproc_set_status(DSPROC_EUNLINK);
                return(0);
            }

            WARNING( DSPROC_LIB_NAME,
                "%s"
                " -> source and destination files have matching MD5s\n"
                " -> deleted source file\n",
                rename_error);

            rename_file = 0;
        }
        else {

            /* The MD5s do not match */

            if (!force_mode && !force_rename) {
                ERROR( DSPROC_LIB_NAME,
                    "%s"
                    " -> source and destination files have different MD5s\n",
                    rename_error);

                dsproc_set_status(DSPROC_EMD5CHECK);
                return(0);
            }
            else {

                WARNING( DSPROC_LIB_NAME,
                    "%s"
                    " -> source and destination files have different MD5s\n"
                    " -> determining unique file name\n",
                    rename_error);

                if (!_dsproc_get_unique_file_name(src_file, dest_file)) {
                    return(0);
                }
            }
        }
    }
    else if (errno != ENOENT) {

        ERROR( DSPROC_LIB_NAME,
            "Could not access file: %s\n"
            " -> %s\n", dest_file, strerror(errno));

        dsproc_set_status(DSPROC_EACCESS);
        return(0);
    }

    /************************************************************
    *  Rename the file
    *************************************************************/

    if (rename_file) {

        LOG( DSPROC_LIB_NAME,
            "Renaming:   %s\n"
            " -> to:     %s\n",
            src_file, dest_file);

        if (!file_move(src_file, dest_file, FC_CHECK_MD5)) {
            dsproc_set_status(DSPROC_EFILEMOVE);
            return(0);
        }

        /* Update the access and modification times */

        utime(dest_file, NULL);
    }

    /************************************************************
    *  Update the datastream file stats
    *************************************************************/

    dsproc_update_datastream_file_stats(
        ds_id, (double)src_stats.st_size, begin_time, end_time);

    return(1);
}

/*******************************************************************************
 *  Private Functions Visible Only To This Library
 */

/** @publicsection */

/*******************************************************************************
 *  Internal Functions Visible To The Public
 */

/**
 *  Force a file to be renamed as bad using 19700101 as the timestamp.
 *
 *  This function should only be used as a last resort if the force option is
 *  enabled and all other attempts to convince the process to continue fail.
 *  It also has no idea which level 0 output datastream to use if more than
 *  one are defined for the process, so the first one found will be used.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  file_path  - path to the directory the file is in
 *  @param  file_name  - name of the file to rename
 *
 *  @return
 *    - 1 if successful
 *    - 0 if a fatal error occurred
 */
int dsproc_force_rename_bad(
    const char *file_path,
    const char *file_name)
{
    timeval_t    begin = {0, 0};
    char         full_path[PATH_MAX];
    int          ndsids;
    int         *dsids;
    int          dsid;
    const char  *level;
    int          dsi;

    /* Check if the file still exists, the ingest may have moved it already */

    snprintf(full_path, PATH_MAX, "%s/%s", file_path, file_name);

    if (access(full_path, F_OK) != 0) {

        if (errno != ENOENT) {

            ERROR( DSPROC_LIB_NAME,
                "Could not access file: %s\n"
                " -> %s\n", full_path, strerror(errno));

            dsproc_set_status(DSPROC_EACCESS);
            return(0);
        }

        return(1);
    }

    LOG( DSPROC_LIB_NAME,
        "FORCE: Forcing rename of raw file: %s\n",
        file_name);

    /* Find the first '0' level output datastream. Some ingests may have
     * more than one, but at this point we have no idea which one to use. */

    ndsids = dsproc_get_output_datastream_ids(&dsids);

    if (ndsids == 0) {

        ERROR( DSPROC_LIB_NAME,
            "FORCE: Could not rename raw file: %s\n"
            " -> no 0 level output datastreams defined\n",
            file_path, file_name);

        dsproc_set_status(DSPROC_EFORCE);
        return(0);
    }

    dsid = -1;

    for (dsi = 0; dsi < ndsids; dsi++) {

        level = dsproc_datastream_class_level(dsids[dsi]);
        if (!level) continue;

        if (level[0] == '0') {
            dsid = dsids[dsi];
            break;
        }
    }

    free(dsids);

    if (dsid < 0) {

        ERROR( DSPROC_LIB_NAME,
            "FORCE: Could not rename raw file: %s\n"
            " -> no 0 level output datastreams defined\n",
            file_path, file_name);

        dsproc_set_status(DSPROC_EFORCE);
        return(0);
    }

    return(_dsproc_rename(
        dsid, file_path, file_name, &begin, NULL, "bad"));
}

/*******************************************************************************
 *  Public Functions
 */

/**
 *  Rename a data file.
 *
 *  This function will rename a raw data file into the datastream directory
 *  using the datastream name and begin_time to give it a fully qualified
 *  ARM name.
 *
 *  The begin_time will be validated using:
 *
 *      dsproc_validate_datastream_data_time()
 *
 *  If the end_time is specified, this function will verify that it is greater
 *  than the begin_time and is not in the future. If only one record was found
 *  in the raw file, the end_time argument must be set to NULL.
 *
 *  If the output file exists and has the same MD5 as the input file,
 *  the input file will be removed and a warning message will be generated.
 *
 *  If the output file exists and has a different MD5 than the input
 *  file, the rename will fail.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  ds_id      - datastream ID
 *  @param  file_path  - path to the directory the file is in
 *  @param  file_name  - name of the file to rename
 *  @param  begin_time - time of the first record in the file
 *  @param  end_time   - time of the last record in the file
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int dsproc_rename(
    int         ds_id,
    const char *file_path,
    const char *file_name,
    time_t      begin_time,
    time_t      end_time)
{
    timeval_t begin = {0, 0};
    timeval_t end   = {0, 0};

    begin.tv_sec = begin_time;
    end.tv_sec   = end_time;

    return(_dsproc_rename(
        ds_id, file_path, file_name, &begin, &end, NULL));
}

/**
 *  Rename a data file.
 *
 *  This function is the same as dsproc_rename() except that it
 *  takes timevals for the begin and end times.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  ds_id      - datastream ID
 *  @param  file_path  - path to the directory the file is in
 *  @param  file_name  - name of the file to rename
 *  @param  begin_time - time of the first record in the file
 *  @param  end_time   - time of the last record in the file
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int dsproc_rename_tv(
    int              ds_id,
    const char      *file_path,
    const char      *file_name,
    const timeval_t *begin_time,
    const timeval_t *end_time)
{
    return(_dsproc_rename(
        ds_id, file_path, file_name, begin_time, end_time, NULL));
}

/**
 *  Rename a bad data file.
 *
 *  This function works the same as dsproc_rename() except that the
 *  extension "bad" will be used in the output file name.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  ds_id      - datastream ID
 *  @param  file_path  - path to the directory the file is in
 *  @param  file_name  - name of the file to rename
 *  @param  file_time  - the time used to rename the file
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int dsproc_rename_bad(
    int         ds_id,
    const char *file_path,
    const char *file_name,
    time_t      file_time)
{
    timeval_t begin = {0, 0};

    begin.tv_sec = file_time;

    return(_dsproc_rename(
        ds_id, file_path, file_name, &begin, NULL, "bad"));
}

/**
 *  Determine the portion of the original file name to preserve.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  ds_id      datastream ID
 *  @param  file_name  name of the raw data file
 *
 *  @retval  1  if successful
 *  @retval  0  if a pattern matching error occurred
 */
int dsproc_set_preserve_dots_from_name(int ds_id, const char *file_name)
{
    DataStream *ds = _DSProc->datastreams[ds_id];

    char        pattern[512];
    regex_t     preg;
    regmatch_t  pmatch[1];
    const char *strp;
    int         status;
    int         ndots;

    sprintf(pattern, "^%s\\.[[:digit:]]{8}\\.[[:digit:]]{6}\\.[[:alpha:]]+\\.",
        ds->name);

    if (!re_compile(&preg, pattern, REG_EXTENDED)) {

        DSPROC_ERROR( NULL,
            "Could not compile regular expression: '%s'",
            pattern);

        return(0);
    }

    status = re_execute(&preg, file_name, 1, pmatch, 0);
    if (status < 0) {

        DSPROC_ERROR( NULL,
            "Could not execute regular expression: '%s'",
            pattern);

        re_free(&preg);
        return(0);
    }

    strp = file_name;

    if (status == 1) {
         strp += pmatch[0].rm_eo;
    }

    ndots = 1;

    while ((strp = strchr(strp, '.'))) {
        ++ndots;
        ++strp;
    }

    re_free(&preg);

    DEBUG_LV1( DSPROC_LIB_NAME,
        "%s: Setting rename preserve dots value to: %d\n",
        ds->name, ndots);

    ds->preserve_dots = ndots;

    return(1);
}

/**
 *  Set the portion of file names to preserve when they are renamed.
 *
 *  This is the number of dots from the end of the file name and specifies
 *  the portion of the original file name to preserve when it is renamed.
 *
 *  Default value set if preserve_dots < 0:
 *
 *    - 2 for level '0' datastreams
 *    - 0 for all other datastreams
 *
 *  @param  ds_id         - output datastream ID
 *  @param  preserve_dots - portion of original file name to preserve
 */
void dsproc_set_rename_preserve_dots(int ds_id, int preserve_dots)
{
    DataStream *ds = _DSProc->datastreams[ds_id];

    if (preserve_dots < 0) {
        preserve_dots = (ds->dsc_level[0] == '0') ? 2: 0;
    }

    DEBUG_LV1( DSPROC_LIB_NAME,
        "%s: Setting rename preserve dots value to: %d\n",
        ds->name, preserve_dots);

    ds->preserve_dots = preserve_dots;
}

/*@}*/
