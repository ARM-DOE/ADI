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
*    $Revision: 6689 $
*    $Author: ermold $
*    $Date: 2011-05-16 19:14:17 +0000 (Mon, 16 May 2011) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file benchmark.c
 *  Benchmark Utilities
 */

#include "armutils.h"

static time_t gStart;
static double gUTime;
static double gSTime;
static double gCUTime;
static double gCSTime;
static double gReal;

/**
 *  Initialize the benchmark function.
 *
 *  This function should be called when the calling process starts.
 *  It sets the start time used to determine the real time in the
 *  benchmark() output.
 */
void benchmark_init(void)
{
    gStart = time(NULL);
}

/**
 *  Print benchmark.
 *
 *  Before using this function the benchmark_init() function should be called.
 *
 *  The first call to this function will print the elapsed times used by the
 *  CPU since the program started. All subsequent calls to this function will
 *  print the elapsed times since the previous call, and the total times since
 *  the program started.
 *
 *  The user time is the CPU time (in seconds) used while executing
 *  instructions in the user space of the calling process.
 *
 *  The system time is the CPU time (in seconds) used by the system
 *  on behalf of the calling process.
 *
 *  The cuser time is the sum of the user times (in seconds) for the
 *  calling process and the child processes.
 *
 *  The csystem time is the sum of the system times (in seconds) for
 *  the calling process and the child processes.
 *
 *  The real time is the wall clock time (in seconds).
 *
 *  @param fp      - pointer to output file stream
 *  @param message - message to print at the top of the output
 */
void benchmark(FILE *fp, char *message)
{
    double         ticks_per_sec = (double)sysconf(_SC_CLK_TCK);
    struct tms     tms_buf;
    double utime,  utime_diff;
    double stime,  stime_diff;
    double cutime, cutime_diff;
    double cstime, cstime_diff;
    double real,   real_diff;

    times(&tms_buf);

    utime  = (double)tms_buf.tms_utime  / ticks_per_sec;
    stime  = (double)tms_buf.tms_stime  / ticks_per_sec;
    cutime = (double)tms_buf.tms_cutime / ticks_per_sec;
    cstime = (double)tms_buf.tms_cstime / ticks_per_sec;
    real   = time(NULL) - gStart;

    utime_diff  = utime  - gUTime;
    stime_diff  = stime  - gSTime;
    cutime_diff = cutime - gCUTime;
    cstime_diff = cstime - gCSTime;
    real_diff   = real   - gReal;

    if (!message) {
        message = "----- Benchmark -----";
    }

    fprintf(fp,
        "\n%s\n\n"
        "              elapsed  total\n"
        "    user:     %-8.2f %-8.2f\n"
        "    system:   %-8.2f %-8.2f\n"
        "    cuser:    %-8.2f %-8.2f\n"
        "    csystem:  %-8.2f %-8.2f\n"
        "    real:     %-8.2f %-8.2f\n",
        message,
        utime_diff,  utime,
        stime_diff,  stime,
        cutime_diff, cutime,
        cstime_diff, cstime,
        real_diff,   real);

    gUTime  = utime;
    gSTime  = stime;
    gCUTime = cutime;
    gCSTime = cstime;
    gReal   = real;
}
