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
*    $Revision: 6688 $
*    $Author: ermold $
*    $Date: 2011-05-16 19:03:08 +0000 (Mon, 16 May 2011) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file msngr_procstats.c
 *  Process Stats Functions.
 */

#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/times.h>

#include "msngr.h"

#if defined(SOLARIS)
#include <procfs.h>
#endif

/**
 *  @defgroup PROCESS_STATS Process Stats
 */
/*@{*/

/*******************************************************************************
 *  Private Data and Functions
 */
/** @privatesection */

static ProcStats gProcStats;

#if defined(SOLARIS)

/**
 *  PRIVATE: Get stats from the /proc/pid/usage file.
 *
 *  @param  pid - process id
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
static int _get_prusage(pid_t pid)
{
    char      prusage_file[64];
    prusage_t pru;
    int       fd;
    int       nread;
    double    run_time;

    memset((void *)&pru, 0, sizeof(prusage_t));

    sprintf(prusage_file, "/proc/%ld/usage", (long)pid);

    fd = open(prusage_file, O_RDONLY);

    if (fd < 0) {

        snprintf(gProcStats.errstr, MAX_STATS_ERROR,
            "Could not open process usage file: %s\n"
            " -> %s\n", prusage_file, strerror(errno));

        return(0);
    }

    nread = read(fd, &pru, sizeof(prusage_t));

    if (nread == -1) {

        snprintf(gProcStats.errstr, MAX_STATS_ERROR,
            "Could not read process usage file: %s\n"
            " -> %s\n", prusage_file, strerror(errno));

        close (fd);
        return(0);
    }
    else if (nread != sizeof(prusage_t)) {

        snprintf(gProcStats.errstr, MAX_STATS_ERROR,
            "Could not read process usage file: %s\n"
            " -> bytes read (%d) does not match expected size (%d)\n",
            prusage_file, nread, sizeof(prusage_t));

        close (fd);
        return(0);
    }

    close (fd);

    if (pru.pr_ioch > gProcStats.total_rw_io) {
        gProcStats.total_rw_io = (unsigned long)pru.pr_ioch;
    }

    run_time = (double)pru.pr_rtime.tv_sec
             + ((double)pru.pr_rtime.tv_nsec/1e9);

    if (run_time > gProcStats.run_time) {
        gProcStats.run_time = run_time;
    }

    return(1);
}

/**
 *  PRIVATE: Get stats from the /proc/pid/psinfo file.
 *
 *  @param  pid - process id
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
static int _get_psinfo(pid_t pid)
{
    char     psinfo_file[64];
    psinfo_t psi;
    int      fd;
    int      nread;

    memset((void *)&psi, 0, sizeof(psinfo_t));

    sprintf(psinfo_file, "/proc/%ld/psinfo", (long)pid);

    fd = open(psinfo_file, O_RDONLY);

    if (fd < 0) {

        snprintf(gProcStats.errstr, MAX_STATS_ERROR,
            "Could not open process info file: %s\n"
            " -> %s\n", psinfo_file, strerror(errno));

        return(0);
    }

    nread = read(fd, &psi, sizeof(psinfo_t));

    if (nread == -1) {

        snprintf(gProcStats.errstr, MAX_STATS_ERROR,
            "Could not read process info file: %s\n"
            " -> %s\n", psinfo_file, strerror(errno));

        close (fd);
        return(0);
    }
    else if (nread != sizeof(psinfo_t)) {

        snprintf(gProcStats.errstr, MAX_STATS_ERROR,
            "Could not read process info file: %s\n"
            " -> bytes read (%d) does not match expected size (%d)\n",
            psinfo_file, nread, sizeof(psinfo_t));

        close (fd);
        return(0);
    }

    close (fd);

    strncpy(gProcStats.exe_name, psi.pr_fname, 127);

    if (psi.pr_size > gProcStats.image_size) {
        gProcStats.image_size = (unsigned int)psi.pr_size;
    }

    if (psi.pr_rssize > gProcStats.rss_size) {
        gProcStats.rss_size = (unsigned int)psi.pr_rssize;
    }

    return(1);
}

#elif defined(LINUX)

/**
 *  PRIVATE: Get stats from the /proc/pid/status file.
 *
 *  @param  pid - process id
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
static int _get_process_status(pid_t pid)
{
    char          status_file[64];
    char          line[512];
    FILE         *fp;
    unsigned int  image_size;
    unsigned int  rss_size;

    sprintf(status_file, "/proc/%d/status", pid);

    fp = fopen(status_file, "r");

    if (!fp) {

        snprintf(gProcStats.errstr, MAX_STATS_ERROR,
            "Could not open process status file: %s\n"
            " -> %s\n", status_file, strerror(errno));

        return(0);
    }

    while (fgets(line, 512, fp) != NULL) {

        if (strstr(line, "Name")) {
            sscanf(line, "%*s %s", gProcStats.exe_name);
        }
        else if (strstr(line, "VmSize")) {
            sscanf(line, "%*s %u", &image_size);
            if (gProcStats.image_size < image_size) {
                gProcStats.image_size = image_size;
            }
        }
        else if (strstr(line, "VmRSS")) {
            sscanf(line, "%*s %u", &rss_size);
            if (gProcStats.rss_size < rss_size) {
                gProcStats.rss_size = rss_size;
            }
        }
    }

    if (ferror(fp)) {

        snprintf(gProcStats.errstr, MAX_STATS_ERROR,
            "Could not read process status file: %s\n"
            " -> %s\n", status_file, strerror(errno));

        fclose(fp);
        return(0);
    }

    fclose(fp);

    return(1);
}

#endif

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Get process stats.
 *
 *  The return value of this function points to the internal ProcStats
 *  structure and must not be modified or freed by the calling process.
 *
 *  @return  pointer to the internal ProcStats structure
 */
ProcStats *procstats_get(void)
{
    pid_t      pid           = getpid();
    double     ticks_per_sec = (double)sysconf(_SC_CLK_TCK);
    double     clock_ticks;
    struct tms tms_buf;

    /* Clear the error message */

    gProcStats.errstr[0] = '\0';

    /* Get total CPU time */

    if (times(&tms_buf) != (clock_t)-1) {

        clock_ticks = tms_buf.tms_utime   /* user time */
                    + tms_buf.tms_stime   /* system time */
                    + tms_buf.tms_cutime  /* user time of children */
                    + tms_buf.tms_cstime; /* system time of children */

        gProcStats.cpu_time = clock_ticks / ticks_per_sec;
    }

    /* Get stats */

#if defined(SOLARIS)
    _get_prusage(pid);
    _get_psinfo(pid);
#elif defined(LINUX)
    _get_process_status(pid);
#endif

    return(&gProcStats);
}

/**
 *  Print process stats.
 *
 *  This function will print the current process information for:
 *
 *      - Executable File Name
 *      - Process Image Size
 *      - Resident Set Size
 *      - Total CPU Time
 *      - Total Read/Write IO
 *      - Run Time
 *
 *  @param fp - pointer to output file stream
 */
void procstats_print(FILE *fp)
{
    ProcStats *proc_stats = procstats_get();

    if (proc_stats->errstr[0] != '\0') {
        fprintf(fp, "%s\n", proc_stats->errstr);
    }

    fprintf(fp,
        "Executable File Name:  %s\n"
        "Process Image Size:    %u Kbytes\n"
        "Resident Set Size:     %u Kbytes\n"
        "Total CPU Time:        %g seconds\n",
        proc_stats->exe_name,
        proc_stats->image_size,
        proc_stats->rss_size,
        proc_stats->cpu_time);

    if (proc_stats->total_rw_io > 0) {
        fprintf(fp,
            "Total Read/Write IO:   %lu bytes\n",
            proc_stats->total_rw_io);
    }

    if (proc_stats->run_time > 0) {
        fprintf(fp,
            "Run Time:              %.2f seconds\n",
            proc_stats->run_time);
    }
}

/*@}*/
