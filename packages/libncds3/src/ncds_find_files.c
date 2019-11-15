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
*    $Revision: 12565 $
*    $Author: ermold $
*    $Date: 2012-02-05 03:04:25 +0000 (Sun, 05 Feb 2012) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file ncds_find_files.c
 *  NCDS Find Files Functions.
 */

#include <errno.h>
#include <dirent.h>
#include <regex.h>
#include <string.h>
#include <unistd.h>

#include "ncds3.h"
#include "ncds_private.h"

/*******************************************************************************
 *  Private Functions
 */
/** @privatesection */

/**
 *  PRIVATE: Get regex error string.
 *
 *  This is a wrapper for the regerror() function. The returned string
 *  is dynamically allocated and must be freed by the calling process.
 *
 *  @param  re_errcode - error code returned by regcomp() or regexec()
 *  @param  re         - pointer to the regular expression that failed
 *
 *  @return
 *    - regex error message
 *    - NULL if a memory allocation error occurred
 */
char *_ncds_regex_errstr(int re_errcode, regex_t *re)
{
    size_t  errlen;
    char   *errstr;

    errlen = regerror(re_errcode, re, NULL, 0);
    errlen++;

    errstr = (char *)malloc(errlen * sizeof(char));
    if (!errstr) {
        return((char *)NULL);
    }

    regerror(re_errcode, re, errstr, errlen);

    return(errstr);
}

/**
 *  PRIVATE: Qsort string compare function.
 *
 *  This function is used by qsort to sort an array of string pointers.
 *
 *  @param  str1 - pointer to pointer to string 1
 *  @param  str2 - pointer to pointer to string 2
 */
int _ncds_qsort_strcmp(const void *str1, const void *str2)
{
    return(strcmp(*(const char **)str1, *(const char **)str2));
}

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Free a NULL terminated list of files.
 *
 *  This function can be used to free a file list
 *  created by the ncds_find_files() function.
 *
 *  @param  file_list - NULL terminated list of files.
 */
void ncds_free_file_list(char **file_list)
{
    int i;

    if (file_list) {
        for (i = 0; file_list[i]; i++) {
            free(file_list[i]);
        }
        free(file_list);
    }
}

/**
 *  Find files containing data for a given date range.
 *
 *  This function requires that file names have the following format:
 *
 *    prefix.YYYYMMDD.hhmmss.extension
 *
 *  where prefix and extension are passed into this function.
 *  The timestamp in the file names must also identify the time
 *  of the first data record.
 *
 *  The first entry in the returned list will be the file with
 *  a timestamp equal to or just prior to the start time.
 *  The last entry in the returned list will be the file with
 *  a timestamp equal to or just prior to the end time.
 *
 *  The returned list of files and file names are dynamically
 *  allocated and must be freed by the calling process
 *  (see ncds_free_file_list()).
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  path       - full path to the directory to look in
 *  @param  prefix     - prefix of files to look for
 *  @param  extension  - extension of files to look for
 *  @param  start_time - start time (seconds since 1970)
 *  @param  end_time   - end time   (seconds since 1970)
 *  @param  file_list  - output: pointer to the file list.
 *
 *  @return
 *    - number of files found
 *    - -1 if an error occurred
 *
 *  @see  ncds_free_file_list()
 */
int ncds_find_files(
    const char *path,
    const char *prefix,
    const char *extension,
    time_t      start_time,
    time_t      end_time,
    char     ***file_list)
{
    char           start_timestamp[16];
    char           end_timestamp[16];

    int            nfiles;
    char         **new_list;
    int            list_size;

    regex_t        re;
    int            re_errcode;
    char          *re_errstr;
    char          *re_pattern;
    size_t         re_pattern_length;
    regmatch_t     re_match[2];

    DIR           *dirp;
    struct dirent *direntp;
    const char    *d_name;
    const char    *d_timestamp;
    size_t         d_length;

    char          *ff_timestamp;
    size_t         ff_length;

    int            alloc_error;
    int            cmp_result;
    int            i;

    /* Setting these to prevent the "may be used uninitialized" warnings */

    ff_length    = 0;
    ff_timestamp = (char *)NULL;

    /* Check if the specified path exists */

    if (access(path, F_OK) != 0) {
        return(0);
    }

    /* Create the start and end timestamps */

    if (!ncds_format_timestamp(start_time, start_timestamp) ||
        !ncds_format_timestamp(end_time, end_timestamp)) {

        return(-1);
    }

    /* Allocate memory for the file list */

    list_size = 32;

    *file_list = (char **)malloc(list_size * sizeof(char *));
    if (!*file_list) {

        ERROR( NCDS_LIB_NAME,
            "Could not create file list for: %s\n"
            " -> memory allocation error\n", path);

        return(-1);
    }

    /* Open the directory */

    dirp = opendir(path);
    if (!dirp) {

        ERROR( NCDS_LIB_NAME,
            "Could not open directory: %s\n"
            " -> %s\n", path, strerror(errno));

        free(*file_list);
        return(-1);
    }

    /* Create the regular expression pattern used to locate the timestamp */

    re_pattern_length = 64;

    if (prefix) {
        re_pattern_length += strlen(prefix);
    }

    if (extension) {
        re_pattern_length += strlen(extension);
    }

    re_pattern = (char *)malloc(re_pattern_length * sizeof(char));

    if (!re_pattern) {

        ERROR( NCDS_LIB_NAME,
            "Could not create file list for: %s\n"
            " -> memory allocation error\n", path);

        closedir(dirp);
        free(*file_list);
        return(-1);
    }

    if (prefix) {
        sprintf(re_pattern, "^%s", prefix);
    }
    else {
        sprintf(re_pattern, "^.+");
    }

    strcat(re_pattern, "\\.([0-9]{8}\\.[0-9]{6})\\.");

    if (extension) {
        strcat(re_pattern, extension);
    }
    else {
        strcat(re_pattern, ".+");
    }

    strcat(re_pattern, "$");

    /* Compile the regular expression used to get the timestamp */

    re_errcode = regcomp(&re, re_pattern, REG_EXTENDED);

    if (re_errcode) {

        re_errstr = _ncds_regex_errstr(re_errcode, &re);

        if (re_errstr) {

            ERROR( NCDS_LIB_NAME,
                "Could not compile regular expression: %s\n"
                " -> %s\n", re_pattern, re_errstr);

            free(re_errstr);
        }
        else {

            ERROR( NCDS_LIB_NAME,
                "Could not compile regular expression: %s\n"
                " -> memory allocation error\n", re_pattern);
        }

        closedir(dirp);
        regfree(&re);
        free(re_pattern);
        free(*file_list);
        return(-1);
    }

    /* Get the list of files in this directory */

    errno           = 0;
    alloc_error     = 0;
    nfiles          = 1;
    (*file_list)[0] = (char *)NULL;

    while ((direntp = readdir(dirp)) != NULL ) {

        d_name = direntp->d_name;

        /* Skip '.' files */

        if (d_name[0] == '.') {
            continue;
        }

        /* Check the file pattern and find the timestamp */

        re_errcode = regexec(&re, d_name, 2, re_match, 0);

        if (re_errcode == REG_NOMATCH) {
            re_errcode = 0;
            continue;
        }
        else if (re_errcode) {
            break;
        }

        d_timestamp = d_name + re_match[1].rm_so;

        /* Skip the file if its timestamp is greater than the end time */

        if (strncmp(d_timestamp, end_timestamp, 15) > 0) {
            continue;
        }

        /* Find the file with a timestamp equal to or
         * just prior to the start time */

        cmp_result = strncmp(d_timestamp, start_timestamp, 15);

        if (cmp_result <= 0) {

            if ((*file_list)[0]) {

                if (cmp_result == 0 ||
                    strncmp(d_timestamp, ff_timestamp, 15) > 0) {

                    d_length = strlen(d_name);

                    if (d_length > ff_length) {

                        ff_length = d_length;

                        (*file_list)[0] = (char *)realloc(
                            (*file_list)[0], (ff_length+1) * sizeof(char));

                        if (!(*file_list)[0]) {
                            alloc_error = 1;
                            break;
                        }
                    }

                    strcpy((*file_list)[0], d_name);

                    ff_timestamp = (*file_list)[0] + re_match[1].rm_so;
                }
            }
            else {
                ff_length = strlen(d_name);

                (*file_list)[0] =
                    (char *)malloc((ff_length + 1) * sizeof(char));

                if (!(*file_list)[0]) {
                    alloc_error = 1;
                    break;
                }

                strcpy((*file_list)[0], d_name);

                ff_timestamp = (*file_list)[0] + re_match[1].rm_so;
            }

            continue;
        }

        /* Check if we need to increase the length of the file list */

        if (nfiles > list_size - 2) {

            list_size += 32;

            new_list =
                (char **)realloc(*file_list, list_size * sizeof(char *));

            if (!new_list) {
                alloc_error = 1;
                break;
            }

            *file_list = new_list;
        }

        /* Add this file to the list */

        (*file_list)[nfiles] =
            (char *)malloc((strlen(d_name) + 1) * sizeof(char));

        if (!(*file_list)[nfiles]) {
            alloc_error = 1;
            break;
        }

        strcpy((*file_list)[nfiles], d_name);
        nfiles++;
    }

    /* Check for errors */

    if (errno || alloc_error || re_errcode) {

        closedir(dirp);

        for (nfiles--; nfiles > 0; nfiles--) {
            free((*file_list)[nfiles]);
        }

        free((*file_list)[0]);
        free(*file_list);

        if (errno) {

            ERROR( NCDS_LIB_NAME,
                "Could not read directory: %s\n"
                " -> $s\n", path, strerror(errno));
        }
        else if (alloc_error) {

            ERROR( NCDS_LIB_NAME,
                "Could not create file list for: %s\n"
                " -> memory allocation error\n", path);
        }
        else {
            re_errstr = _ncds_regex_errstr(re_errcode, &re);

            if (re_errstr) {

                ERROR( NCDS_LIB_NAME,
                    "Could not execute regular expression: %s\n"
                    " -> %s\n", re_pattern, re_errstr);

                free(re_errstr);
            }
            else {

                ERROR( NCDS_LIB_NAME,
                    "Could not execute regular expression: %s\n"
                    " -> memory allocation error\n", re_pattern);
            }
        }

        regfree(&re);
        free(re_pattern);
        return(-1);
    }

    closedir(dirp);
    regfree(&re);

    /* Check if any files were found */

    if (nfiles == 1 && !(*file_list)[0]) {
        free(*file_list);
        free(re_pattern);
        return(0);
    }

    /* Check if a file just prior to the start time was found */

    if (!(*file_list)[0]) {

        for (i = 1; i < nfiles; i++) {
            (*file_list)[i-1] = (*file_list)[i];
        }

        nfiles--;
    }

    (*file_list)[nfiles] = (char *)NULL;

    /* Sort the file list */

    qsort(*file_list, nfiles, sizeof(char *), _ncds_qsort_strcmp);

    /* Return the file list */

    free(re_pattern);
    return(nfiles);
}
