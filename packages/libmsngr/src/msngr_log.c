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
*    $Revision: 63477 $
*    $Author: ermold $
*    $Date: 2015-08-26 22:47:45 +0000 (Wed, 26 Aug 2015) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file msngr_log.c
 *  Log File Functions.
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "msngr.h"

/**
 *  @defgroup LOG_FILES Log Files
 */
/*@{*/

/*******************************************************************************
 *  Private Functions
 */
/** @privatesection */

/**
 *  PRIVATE: Free all memory used by a LogFile.
 *
 *  This function will also close the log file if log->fp
 *  is not NULL, and it is not stdout or stderr.
 *
 *  @param  log - pointer to the LogFile
 */
static void _log_free(LogFile *log)
{
    if (log) {

        if (log->fp && (log->fp != stdout) && (log->fp != stderr)) {
            fclose(log->fp);
        }

        if (log->path)      free(log->path);
        if (log->name)      free(log->name);
        if (log->full_path) free(log->full_path);
        if (log->errstr)    free(log->errstr);

        free(log);
    }
}

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Close a LogFile.
 *
 *  This function will close a log file and free all memory
 *  used by the LogFile structure.
 *
 *  If the pointer to the LogFile is NULL this function will
 *  do nothing and return successfully.
 *
 *  If the the log file is not open (log->fp == NULL),
 *  all messeges will be printed to stdout.
 *
 *  If the LOG_STATS flag is set, the process stats will be
 *  printed to the log file before it is closed.
 *
 *  If the LOG_TAGS flag is set, the run time and the
 *  "**** CLOSED: YYYY-MM-DD hh:mm:ss" lines will be printd
 *  to the log file before it is closed.
 *
 *  The space pointed to by errstr should be large enough to
 *  hold MAX_LOG_ERROR bytes. Any less than that and the error
 *  message could be truncated.
 *
 *  If errlen is 0 then no error message is written to errstr
 *  and errstr can be NULL.
 *
 *  @param  log    - pointer to the LogFile
 *  @param  errlen - length of the error message buffer
 *  @param  errstr - output: error message
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int log_close(
    LogFile *log,
    size_t   errlen,
    char    *errstr)
{
    FILE      *log_fp;
    ProcStats *proc_stats;
    time_t     close_time;
    time_t     run_time;
    char       time_string[32];
    int        nbytes;
    int        log_errno;
    char      *full_path;

    if (!log) {
        return(1);
    }

    log_fp    = (log->fp) ? log->fp : stdout;
    log_errno = 0;
    run_time  = 0;

    /* Log process stats */

    if (log->flags & LOG_STATS) {

        proc_stats = procstats_get();

        if (proc_stats->errstr[0] != '\0') {

            nbytes = fprintf(log_fp,
                "\n%s", proc_stats->errstr);

            if (nbytes < 0) {
                log_errno = errno;
            }
        }

        nbytes = fprintf(log_fp,
            "\n"
            "Executable File Name:  %s\n"
            "Process Image Size:    %u Kbytes\n"
            "Resident Set Size:     %u Kbytes\n"
            "Total CPU Time:        %g seconds\n",
            proc_stats->exe_name,
            proc_stats->image_size,
            proc_stats->rss_size,
            proc_stats->cpu_time);

        if (nbytes < 0) {
            log_errno = errno;
        }

        if (proc_stats->total_rw_io > 0) {

            nbytes = fprintf(log_fp,
                "Total read/write IO:   %lu bytes\n",
                proc_stats->total_rw_io);

            if (nbytes < 0) {
                log_errno = errno;
            }
        }

        if (proc_stats->run_time > 0) {
            run_time = (time_t)(proc_stats->run_time + 0.5);
        }
    }

    /* Log run time and closed tag */

    if (log->flags & LOG_TAGS) {

        close_time = time(NULL);

        if (!run_time && log->open_time) {
            run_time = close_time - log->open_time;
        }

        nbytes = fprintf(log_fp,
            "\nRun time: %d seconds\n"
            "**** CLOSED: %s\n",
            (int)run_time,
            msngr_format_time(close_time, time_string));

        if (nbytes < 0) {
            log_errno = errno;
        }
    }

    /* Close the log file */

    if (log_fp != stdout &&
        log_fp != stderr) {

        if (log->flags & LOG_LOCKF) {
            lockf(fileno(log_fp), F_ULOCK, 0);
        }

        if (fclose(log_fp) != 0) {
            log_errno = errno;
        }
    }

    /* Check for errors */

    if (log_errno) {

        if (log->full_path) {
            full_path = log->full_path;
        }
        else if (log_fp == stdout) {
            full_path = "stdout";
        }
        else if (log_fp == stderr) {
            full_path = "stderr";
        }
        else {
            full_path = "unknown";
        }

        snprintf(errstr, errlen,
            "Could not write to log file: %s\n"
            " -> %s\n",
            full_path, strerror(log_errno));
    }

    /* Free the LogFile structure */

    log->fp = (FILE *)NULL;

    _log_free(log);

    return((log_errno) ? 0 : 1 );
}

/**
 *  Clear the last error message for a LogFile.
 *
 *  @param  log - pointer to the LogFile
 */
void log_clear_error(LogFile *log)
{
    if (log) {
        log->errstr[0] = '\0';
    }
}

/**
 *  Get the last error message for a LogFile.
 *
 *  @param  log - pointer to the LogFile
 *
 *  @return
 *    - the last error message that occurred
 *    - NULL if no errors have occurred
 */
const char *log_get_error(LogFile *log)
{
    if (log->errstr[0] == '\0') {
        return((const char *)NULL);
    }
    else {
        return((const char *)log->errstr);
    }
}

/**
 *  Open a LogFile.
 *
 *  This function will create the log file path with permissions 00775
 *  if it does not already exist. It will then open a log file and place
 *  an advisory lock on it using lockf(). If the log cannot be locked
 *  it probably means another process is using it.
 *
 *  If the LOG_TAGS flag is set, the "**** OPENED: YYYY-MM-DD hh:mm:ss"
 *  line will be printd to the log file after it is opened.
 *
 *  The space pointed to by errstr should be large enough to
 *  hold MAX_LOG_ERROR bytes. Any less than that and the error
 *  message could be truncated.
 *
 *  If errlen is 0 then no error message is written to errstr
 *  and errstr can be NULL.
 *
 *  @param  path   - path to the directory to open the log file in
 *  @param  name   - log file name
 *  @param  flags  - control flags
 *  @param  errlen - length of the error message buffer
 *  @param  errstr - output: error message
 *
 *  Control Flags:
 *
 *    - LOG_TAGS  - Print the "**** OPENED: " line when the log is opened,
 *                  and the "**** CLOSED: " line when the log is closed.
 *
 *    - LOG_STATS - Log process stats before closing the log file.
 *
 *    - LOG_LOCKF - Place an advisory lock on the log file using lockf().
 *
 *  @return
 *    - pointer to the open LogFile
 *    - NULL if:
 *        - a memory allocation error occurred
 *        - the log path did not exist and it could not be created
 *        - the log file could not be opened
 *        - the log file could not be locked
 *        - the log file could not be written to
 */
LogFile *log_open(
    const char *path,
    const char *name,
    int         flags,
    size_t      errlen,
    char       *errstr)
{
    LogFile *log;
    char     time_string[32];
    int      nbytes;

    if (!msngr_make_path(path, 00775, errlen, errstr)) {
        return(0);
    }

    log = (LogFile *)calloc(1, sizeof(LogFile));

    if (!log) {

        snprintf(errstr, errlen,
            "Could not open log file: %s/%s\n"
            " -> memory allocation error\n",
            path, name);

        return((LogFile *)NULL);
    }

    log->name      = msngr_copy_string(name);
    log->path      = msngr_copy_string(path);
    log->full_path = msngr_create_string("%s/%s", path, name);
    log->errstr    = (char *)calloc(MAX_LOG_ERROR, sizeof(char));

    if (!log->name || !log->path || !log->full_path || !log->errstr) {

        _log_free(log);

        snprintf(errstr, errlen,
            "Could not open log file: %s/%s\n"
            " -> memory allocation error\n",
            path, name);

        return((LogFile *)NULL);
    }

    log->fp = fopen(log->full_path, "a");

    if (!log->fp) {

        _log_free(log);

        snprintf(errstr, errlen,
            "Could not open log file: %s/%s\n"
            " -> %s\n",
            path, name, strerror(errno));

        return((LogFile *)NULL);
    }

    if (flags & LOG_LOCKF) {

        if (lockf(fileno(log->fp), F_TLOCK, 0) == -1) {

            _log_free(log);

            snprintf(errstr, errlen,
                "Could not get lock on log file: %s/%s\n"
                " -> %s\n",
                path, name, strerror(errno));

            return((LogFile *)NULL);
        }
    }

    log->flags     = flags;
    log->open_time = time(NULL);

    if (log->flags & LOG_TAGS) {

        nbytes = fprintf(log->fp,
            "**** OPENED: %s\n",
            msngr_format_time(log->open_time, time_string));

        if (nbytes < 0) {

            _log_free(log);

            snprintf(errstr, errlen,
                "Could not write to log file: %s/%s\n"
                " -> %s\n",
                path, name, strerror(errno));

            return((LogFile *)NULL);
        }
    }

#if LINUX /* Update Process Stats */
    if (log->flags & LOG_STATS) {
        procstats_get();
    }
#endif

    return(log);
}

/**
 *  Print a message to a LogFile.
 *
 *  This function will print a message to a log file under the
 *  control of the format argument. Alternatively, an array of
 *  strings can be passed into this function by specifying
 *  MSNGR_MESSAGE_BLOCK for the format argument. In this case
 *  the next argument after format must be a pointer to a NULL
 *  terminted array of strings (char **).
 *
 *  If the pointer to the LogFile is NULL or the log file
 *  is not open, all messeges will be printed to stdout.
 *
 *  @param  log      - pointer to the LogFile
 *  @param  line_tag - line tag to print before the message is printed
 *  @param  format   - format string (see printf)
 *  @param  ...      - arguments for the format string
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 *
 *  @see  log_get_error()
 */
int log_printf(
    LogFile    *log,
    const char *line_tag,
    const char *format, ...)
{
    va_list args;
    int     retval;

    va_start(args, format);
    retval = log_vprintf(log, line_tag, format, args);
    va_end(args);

    return(retval);
}

/**
 *  Print a message to a LogFile.
 *
 *  This function will print a message to a log file under the
 *  control of the format argument. Alternatively, an array of
 *  strings can be passed into this function by specifying
 *  MSNGR_MESSAGE_BLOCK for the format argument. In this case
 *  the first argument in the args list must be a pointer to
 *  a NULL terminted array of strings (char **).
 *
 *  If the pointer to the LogFile is NULL or the log file
 *  is not open, all messeges will be printed to stdout.
 *
 *  @param  log      - pointer to the LogFile
 *  @param  line_tag - line tag to print before the message is printed
 *  @param  format   - format string (see printf)
 *  @param  args     - arguments for the format string
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 *
 *  @see  log_get_error()
 */
int log_vprintf(
    LogFile    *log,
    const char *line_tag,
    const char *format,
    va_list     args)
{
    FILE    *log_fp;
    int      nbytes;
    int      log_errno;
    char    *full_path;
    char   **msg_block;
    va_list  args_copy;
    size_t   length;
    int      i;

    log_fp    = (log && log->fp) ? log->fp : stdout;
    log_errno = 0;
    nbytes    = 0;

    /* Print the line tag if one was specified */

    if (line_tag) {
        nbytes = fprintf(log_fp, "%s", line_tag);
        if (nbytes < 0) {
            log_errno = errno;
        }
    }

    /* Print the message to the log file */

    if (strcmp(format, "MSNGR_MESSAGE_BLOCK") == 0) {

        va_copy(args_copy, args);
        msg_block = va_arg(args_copy, char **);
        va_end(args_copy);

        for (i = 0; msg_block[i] != (char *)NULL; i++) {

            length = strlen(msg_block[i]);
            nbytes = fprintf(log_fp, "%s", msg_block[i]);
            if (nbytes < 0) {
                log_errno = errno;
            }
            else {
                if ((length == 0) || (msg_block[i][length-1] != '\n')) {
                    fprintf(log_fp, "\n");
                }
            }
        }
    }
    else {
        length = strlen(format);
        nbytes = msngr_vfprintf(log_fp, format, args);
        if ((length == 0) || (format[length-1] != '\n')) {
            fprintf(log_fp, "\n");
        }
    }

    if (nbytes < 0) {
        log_errno = errno;
    }

    /* Flush the log file buffer */

    if (fflush(log_fp) != 0) {
        log_errno = errno;
    }

    /* Check for errors */

    if (log_errno) {

        if (log_fp == stdout) {
            full_path = "stdout";
        }
        else if (log_fp == stderr) {
            full_path = "stderr";
        }
        else if (log->full_path) {
            full_path = log->full_path;
        }
        else {
            full_path = "";
        }

        snprintf(log->errstr, MAX_LOG_ERROR,
            "Could not write to log file: %s\n"
            " -> %s\n",
            full_path, strerror(log_errno));
    }

#if LINUX /* Update Process Stats */
    if (log->flags & LOG_STATS) {
        procstats_get();
    }
#endif

    return((log_errno) ? 0 : 1 );
}

/*@}*/
