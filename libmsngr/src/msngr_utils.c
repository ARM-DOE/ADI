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
*    $Revision: 67791 $
*    $Author: ermold $
*    $Date: 2016-02-29 23:41:35 +0000 (Mon, 29 Feb 2016) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file msngr_utils.c
 *  Utility Functions.
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

#include "msngr.h"

/**
 *  @defgroup MSNGRUtils Utility Functions
 */
/*@{*/

/*******************************************************************************
 *  Private Functions
 */
/** @privatesection */

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Create a dynamically allocated copy of a string.
 *
 *  The memory used by the returned string is dynamically allocated.
 *  It is the responsibility of the calling process to free this
 *  memory when it is no longer needed.
 *
 *  @param  string - pointer to the string to copy
 *
 *  @return
 *    - pointer to the dynamically allocated copy of the string
 *    - NULL if a memory allocation error occurred
 */
char *msngr_copy_string(const char *string)
{
    size_t  length = strlen(string) + 1;
    char   *copy   = (char *)malloc(length * sizeof(char));

    if (copy) {
        return((char *)memcpy(copy, string, length));
    }

    return((char *)NULL);
}

/**
 *  Create a new text string.
 *
 *  This functions creates a new text string under the control of
 *  the format argument.
 *
 *  The memory used by the returned string is dynamically allocated.
 *  It is the responsibility of the calling process to free this
 *  memory when it is no longer needed.
 *
 *  @param  format - format string (see printf)
 *  @param  ...    - arguments for the format string
 *
 *  @return
 *    - pointer to the dynamically allocated text string
 *    - NULL if a memory allocation error occurred
 */
char *msngr_create_string(const char *format, ...)
{
    va_list  args;
    char    *string;

    va_start(args, format);
    string = msngr_format_va_list(format, args);
    va_end(args);

    return(string);
}

/**
 *  Convert seconds since 1970 to a formatted time string.
 *
 *  This function will create a time string of the form:
 *
 *      'YYYY-MM-DD hh:mm:ss'.
 *
 *  The string argument must be large enough to hold at
 *  least 20 characters (19 plus the null terminator).
 *
 *  If an error occurs in this function, the message
 *  'FORMATTING ERROR' will be copied into the text string.
 *
 *  @param  secs1970 - seconds since 1970
 *  @param  string   - time string to store the result in
 *
 *  @return pointer to the string argument
 */
char *msngr_format_time(time_t secs1970, char *string)
{
    struct tm tm_time;

    memset(&tm_time, 0, sizeof(tm_time));

    if (!gmtime_r(&secs1970, &tm_time)) {
        strcpy(string, "FORMATTING ERROR");
    }
    else {
        sprintf(string,
            "%04d-%02d-%02d %02d:%02d:%02d",
            tm_time.tm_year + 1900,
            tm_time.tm_mon  + 1,
            tm_time.tm_mday,
            tm_time.tm_hour,
            tm_time.tm_min,
            tm_time.tm_sec);
    }

    return(string);
}

/**
 *  Create a text string from a format string and va_list.
 *
 *  The memory used by the returned string is dynamically allocated.
 *  It is the responsibility of the calling process to free this
 *  memory when it is no longer needed.
 *
 *  @param  format - format string (see printf)
 *  @param  args   - arguments for the format string
 *
 *  @return
 *    - pointer to the dynamically allocated text string
 *    - NULL if a memory allocation error occurred
 */
char *msngr_format_va_list(
    const char *format,
    va_list     args)
{
    int     length;
    char   *string;

    length = msngr_vsnprintf((char *)NULL, 0, format, args) + 1;

    if (length <= 0) {
        return((char *)NULL);
    }

    string = (char *)malloc(length * sizeof(char));
    if (!string) {
        return((char *)NULL);
    }

    msngr_vsprintf(string, format, args);

    return(string);
}

/**
 *  Get the start time of a process.
 *
 *  @param  pid - process ID
 *
 *  @return
 *    - process start time in seconds since 1970
 *    - 0 if the process does not exist
 */
time_t msngr_get_process_start_time(pid_t pid)
{
    char        pid_dir[32];
    struct stat pid_dir_stats;

    snprintf(pid_dir, 32, "/proc/%d", (int)pid);

    if (stat(pid_dir, &pid_dir_stats) == 0) {
        return(pid_dir_stats.st_mtime);
    }

    return(0);
}

/**
 *  Make the full path to a directory.
 *
 *  This function will create the specified path if it does
 *  not already exist.
 *
 *  The space pointed to by errstr should be large enough to
 *  hold MAX_LOG_ERROR bytes. Any less than that and the error
 *  message could be truncated.
 *
 *  If errlen is 0 then no error message is written to errstr
 *  and errstr can be NULL.
 *
 *  @param  path   - directory path to create
 *  @param  mode   - mode of the new directory
 *  @param  errlen - length of the error message buffer
 *  @param  errstr - output: error message
 *
 *  @return
 *    - 1 if the path exists or was created
 *    - 0 if an error occurred
 */
int msngr_make_path(
    const char *path,
    mode_t      mode,
    size_t      errlen,
    char       *errstr)
{
    char *buffer;
    char *chrp;
    char  chr;

    if (access(path, F_OK) != 0) {

        buffer = malloc((strlen(path) + 1) * sizeof(char));
        if (!buffer) {

            snprintf(errstr, errlen,
                "Could not make path: %s\n"
                " -> memory allocation error\n", path);

            return(0);
        }

        strcpy(buffer, path);

        for (chrp = buffer+1; ; chrp++) {

            if (*chrp == '/' || *chrp == '\0') {

                chr   = *chrp;
                *chrp = '\0';

                if (access(buffer, F_OK) != 0) {

                    if (mkdir(buffer, mode) < 0) {

                        if (errno != EEXIST) {
                            snprintf(errstr, errlen,
                                "Could not make path: %s\n"
                                " -> mkdir error: %s\n",
                                buffer, strerror(errno));

                            free(buffer);
                            return(0);
                        }
                    }
                }

                *chrp = chr;
                if (chr == '\0') break;
            }
        }

        free(buffer);
    }

    return(1);
}

/**
 *  Wrapper to vprintf that preserves the argument list.
 *
 *  @param  format - format string
 *  @param  args   - arguments for the format string
 *
 *  @return  the number of bytes written to stdout
 */
int msngr_vprintf(const char *format, va_list args)
{
    va_list copy;
    int     retval;

    va_copy(copy, args);
    retval = vprintf(format, copy);
    va_end(copy);

    return(retval);
}

/**
 *  Wrapper to vfprintf that preserves the argument list.
 *
 *  @param  stream - pointer to the output stream
 *  @param  format - format string
 *  @param  args   - arguments for the format string
 *
 *  @return  the number of bytes written to the output stream
 */
int msngr_vfprintf(FILE *stream, const char *format, va_list args)
{
    va_list copy;
    int     retval;

    va_copy(copy, args);
    retval = vfprintf(stream, format, copy);
    va_end(copy);

    return(retval);
}

/**
 *  Wrapper to vsprintf that preserves the argument list.
 *
 *  @param  string - output string
 *  @param  format - format string
 *  @param  args   - arguments for the format string
 *
 *  @return  the number of bytes written to the output string
 *           (excluding the terminating null byte)
 */
int msngr_vsprintf(char *string, const char *format, va_list args)
{
    va_list copy;
    int     retval;

    va_copy(copy, args);
    retval = vsprintf(string, format, copy);
    va_end(copy);

    return(retval);
}

/**
 *  Wrapper to vsnprintf that preserves the argument list.
 *
 *  @param  string - output string
 *  @param  nbytes - the length of the output string
 *  @param  format - format string
 *  @param  args   - arguments for the format string
 *
 *  @return  the number of bytes that would have been written to the
 *           output string if its length had been sufficiently large
 *           (excluding the terminating null byte.)
 */
int msngr_vsnprintf(
    char       *string,
    size_t      nbytes,
    const char *format,
    va_list     args)
{
    va_list copy;
    int     retval;

    va_copy(copy, args);
    retval = vsnprintf(string, nbytes, format, copy);
    va_end(copy);

    return(retval);
}

/*@}*/
