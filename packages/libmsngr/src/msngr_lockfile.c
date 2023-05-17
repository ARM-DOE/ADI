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
*    $Revision: 12907 $
*    $Author: ermold $
*    $Date: 2012-02-23 07:10:44 +0000 (Thu, 23 Feb 2012) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file msngr_lockfile.c
 *  Lock File Functions.
 */

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#include "msngr.h"

/**
 *  @defgroup LOCK_FILES Lock Files
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
 *  Remove a lockfile.
 *
 *  The space pointed to by errstr should be large enough to
 *  hold MAX_LOCKFILE_ERROR bytes. Any less than that and the
 *  error message could be truncated.
 *
 *  @param  path   - path to the lockfile
 *  @param  name   - name of the lockfile
 *  @param  errlen - length of the error message buffer
 *  @param  errstr - output: error message
 *
 *  @return
 *    -  1 if successful
 *    -  0 the lockfile did not exist
 *    - -1 if an error occurred
 */
int lockfile_remove(
    const char *path,
    const char *name,
    size_t      errlen,
    char       *errstr)
{
    char lockfile[PATH_MAX];

    snprintf(lockfile, PATH_MAX, "%s/%s", path, name);

    /* Check to see if the lockfile exists */

    if (access(lockfile, F_OK) == 0) {

        if (unlink(lockfile) != 0) {

            snprintf(errstr, errlen,
                "Could not remove lockfile: %s\n"
                " -> %s\n",
                lockfile, strerror(errno));

            return(-1);
        }

        return(1);
    }
    else if (errno != ENOENT) {

        snprintf(errstr, errlen,
            "Could not access lockfile: %s\n"
            " -> %s\n",
            lockfile, strerror(errno));

        return(-1);
    }

    return(0);
}

/**
 *  Create a lockfile.
 *
 *  This function will print the time, hostname and process id
 *  into the lockfile.
 *
 *  The space pointed to by errstr should be large enough to
 *  hold MAX_LOCKFILE_ERROR bytes. Any less than that and the
 *  error message could be truncated.
 *
 *  @param  path   - path to the lockfile
 *  @param  name   - name of the lockfile
 *  @param  flags  - control flags (reserved for future use)
 *  @param  errlen - length of the error message buffer
 *  @param  errstr - output: error message
 *
 *  @return
 *    -  2 if a stale lockfile was found and removed.
 *    -  1 if successful
 *    -  0 the lockfile already exists (error message will be set)
 *    - -1 if an error occurred
 */
int lockfile_create(
    const char *path,
    const char *name,
    int         flags,
    size_t      errlen,
    char       *errstr)
{
    char   lockfile[PATH_MAX];
    FILE  *lockfile_fp;
    char   hostname[256];
    pid_t  pid;
    time_t pid_time;
    char   time_string[32];
    int    nbytes;

    char   lockfile_string[256];
    char  *chrp;
    char  *lockfile_host;
    pid_t  lockfile_pid;
    time_t lockfile_time;
    time_t current_time;

    int    retval = 1;

    snprintf(lockfile, PATH_MAX, "%s/%s", path, name);

    /* Get hostname */

    if (gethostname(hostname, 256) == -1) {

        snprintf(errstr, errlen,
            "Could not get hostname for lockfile:\n"
            " -> %s\n"
            " -> %s\n",
            lockfile, strerror(errno));

        return(-1);
    }

    /* Make sure the path to the lockfile exists */

    if (!msngr_make_path(path, 00775, errlen, errstr)) {
        return(0);
    }

    /* Check to see if the lockfile already exists */

    if (access(lockfile, F_OK) == 0) {

        /* Open the existing lockfile */

        lockfile_fp = fopen(lockfile, "r");

        if (!lockfile_fp) {

            snprintf(errstr, errlen,
                "Lockfile exists but could not be opened:\n"
                " -> %s\n"
                " -> %s\n",
                lockfile, strerror(errno));

            return(-1);
        }

        /* Read in the identifier line */

        if (!fgets(lockfile_string, 256, lockfile_fp)) {

            snprintf(errstr, errlen,
                "Lockfile exists but could not be read:\n"
                " -> %s\n"
                " -> %s\n",
                lockfile, strerror(errno));

            return(-1);
        }

        /* Duplicate the line and set pointer to hostname */

        lockfile_host = strdup(lockfile_string);
        if (!lockfile_host) {

            snprintf(errstr, errlen,
                "Lockfile exists:\n"
                " -> %s\n"
                " -> but could not parse identifier string: '%s'\n"
                " -> memory allocation error",
                lockfile, lockfile_string);

            return(-1);
        }

        /* Get the pid of the locked process */

        chrp = strchr(lockfile_host, ':');
        if (!chrp) {

            snprintf(errstr, errlen,
                "Lockfile exists:\n"
                " -> %s\n"
                " -> but has an invalid identifier string: '%s'\n",
                lockfile, lockfile_string);

            free(lockfile_host);
            return(-1);
        }
        *chrp = '\0';

        lockfile_pid = atoi(++chrp);

        /* Get the start time of the locked process */

        chrp = strchr(chrp, ':');
        if (!chrp) {

            snprintf(errstr, errlen,
                "Lockfile exists:\n"
                " -> %s\n"
                " -> but has an invalid identifier string: '%s'\n",
                lockfile, lockfile_string);

            free(lockfile_host);
            return(-1);
        }

        lockfile_time = (time_t)atol(++chrp);

        /* Check if this process was started on the same host. */

        if (strcmp(lockfile_host, hostname) == 0) {

            /* Check if this process is still running. */

            pid_time = msngr_get_process_start_time(lockfile_pid);

            if (lockfile_time == pid_time) {

                snprintf(errstr, errlen,
                    "Lockfile exists:\n"
                    " -> %s\n"
                    " -> %s\n",
                    lockfile, lockfile_string);

                free(lockfile_host);
                return(0);
            }
        }
        else {

            /* Process was started on a different host. */

            current_time = time(NULL);
            if (current_time - lockfile_time < 86400) {

                snprintf(errstr, errlen,
                    "Lockfile exists:\n"
                    " -> %s\n"
                    " -> %s\n",
                    lockfile, lockfile_string);

                free(lockfile_host);
                return(0);
            }
            else {
                /* Process was started over 1 day ago on a differnt host,
                 * so assume it is a stale lockfile */
            }
        }

        free(lockfile_host);

        /* We found a stale lockfile so clean it up and continue... */

        if (lockfile_remove(path, name, errlen, errstr) < 0) {
            return(-1);
        }

        retval = 2;
    }
    else if (errno != ENOENT) {

        snprintf(errstr, errlen,
            "Could not check if lockfile exists:\n"
            " -> %s\n"
            " -> %s\n",
            lockfile, strerror(errno));

        return(-1);
    }

    /* Get process ID and start time */

    pid      = getpid();
    pid_time = msngr_get_process_start_time(pid);
    msngr_format_time(pid_time, time_string);

    /* Create the lockfile */

    lockfile_fp = fopen(lockfile, "w");

    if (!lockfile_fp) {

        snprintf(errstr, errlen,
            "Could not create lockfile:\n"
            " -> %s\n"
            " -> %s\n",
            lockfile, strerror(errno));

        return(-1);
    }

    nbytes = fprintf(lockfile_fp, "%s:%d:%ld %s\n",
        hostname, (int)pid, (long)pid_time, time_string);

    if (nbytes < 0) {

        snprintf(errstr, errlen,
            "Could not write to lockfile:\n"
            " -> %s\n"
            " -> %s\n",
            lockfile, strerror(errno));

        fclose(lockfile_fp);
        unlink(lockfile);

        return(-1);
    }

    if (fclose(lockfile_fp) != 0) {

        snprintf(errstr, errlen,
            "Could not close lockfile:\n"
            " -> %s\n"
            " -> %s\n",
            lockfile, strerror(errno));

        unlink(lockfile);

        return(-1);
    }

    return(retval);
}

/*@}*/
