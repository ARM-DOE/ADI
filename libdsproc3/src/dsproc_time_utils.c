/*******************************************************************************
*
*  COPYRIGHT (C) 2017 Battelle Memorial Institute.  All Rights Reserved.
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
*    $Revision: 80272 $
*    $Author: ermold $
*    $Date: 2017-08-28 01:47:45 +0000 (Mon, 28 Aug 2017) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file dsproc_time_utils.c
 *  Time Functions.
 */

#include "dsproc3.h"

/**
 *  Convert seconds since 1970 to a timestamp.
 *
 *  This function will create a timestamp of the form:
 *
 *      'YYYYMMDD.hhmmss'
 *
 *  The timestamp argument must be large enough to hold at least 16 characters
 *  (15 plus the null terminator).
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  secs1970  - seconds since 1970
 *  @param  timestamp - output: timestamp string in the form YYYYMMDD.hhmmss
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int dsproc_create_timestamp(time_t secs1970, char *timestamp)
{
    struct tm gmt;

    memset(&gmt, 0, sizeof(struct tm));

    if (!gmtime_r(&secs1970, &gmt)) {

        ERROR( DSPROC_LIB_NAME,
            "Could not create timestamp for: %ld\n"
            " -> gmtime error: %s\n",
            (long)secs1970, strerror(errno));

        sprintf(timestamp, "GMTIME.ERROR");
        dsproc_set_status(DSPROC_ETIMECALC);
        return(0);
    }

    sprintf(timestamp,
        "%04d%02d%02d.%02d%02d%02d",
        gmt.tm_year + 1900,
        gmt.tm_mon  + 1,
        gmt.tm_mday,
        gmt.tm_hour,
        gmt.tm_min,
        gmt.tm_sec);

    return(1);
}
