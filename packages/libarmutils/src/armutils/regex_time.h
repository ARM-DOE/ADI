/*******************************************************************************
*
*  COPYRIGHT (C) 2015 Battelle Memorial Institute.  All Rights Reserved.
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
*    $Revision: 67752 $
*    $Author: ermold $
*    $Date: 2016-02-25 20:02:46 +0000 (Thu, 25 Feb 2016) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file regex_time.h
 *  Regex Time Utilities
 */

#include "armutils/time_utils.h"

#ifndef _REGEX_TIME_H
#define _REGEX_TIME_H

#define RETIME_MAX_SUBSTR_LENGTH 128 /**< maximum length of a parsed substring */
#define RETIME_MAX_NSUBS         32  /**< maximum number of subexpressions     */

/**
 *  @defgroup ARMUTILS_REGEX_TIME Regex Time Utils
 */
/*@{*/

/**
 *  Regular Expression with Time Format Codes.
 */
typedef struct {

    char        *tspattern; /**< original time string pattern         */
    char        *pattern;   /**< regex pattern string                 */
    size_t       nsubs;     /**< number of subexpressions             */
    char        *codes;     /**< date-time codes of subexpressions    */
    regex_t      preg;      /**< compiled regular expression          */
    int          flags;     /**< reserved for control flags           */

} RETime;

/**
 *  Result RETime pattern match.
 */
typedef struct {

    int          year;      /**< year with century as a 4-digit integer         */
    int          month;     /**< month number (1-12)                            */
    int          mday;      /**< day number in the month (1-31)                 */
    int          hour;      /**< hour (0-23)                                    */
    int          min;       /**< minute (0-59)                                  */
    int          sec;       /**< second (0-60; 60 may occur for leap seconds)   */
    int          usec;      /**< micro-seconds                                  */
    int          century;   /**< century number (year/100) as a 2-digit integer */
    int          yy;        /**< year number in century as a 2-digit integer    */
    int          yday;      /**< day number in the year (1-366)                 */
    int          hhmm;      /**< hour * 100 + minute                            */
    time_t       secs1970;  /**< seconds since Epoch, 1970-01-01 00:00:00       */
    timeval_t    offset;    /**< time offset from %o match                      */

    /* used by retime_get_* functions */
    time_t       res_time;  /**< result in seconds since Epoch, 1970-01-01      */
    timeval_t    res_tv;    /**< result in seconds since Epoch, 1970-01-01      */

    /** pointer to the RETime that matched */
    RETime      *retime;

    /** arrary of matching substring offsets */
    regmatch_t   pmatch[RETIME_MAX_NSUBS];

} RETimeRes;

/**
 *  List of Regular Expression with Time Format Codes.
 */
typedef struct {

    int          npatterns; /**< number of RETime patterns in the list  */
    RETime     **retimes;   /**< list of RETime patterns                */

} RETimeList;

RETime     *retime_compile(
                const char *pattern,
                int         flags);

int         retime_execute(
                RETime     *retime,
                const char *string,
                RETimeRes  *res);

void        retime_free(
                RETime *retime);

time_t      retime_get_secs1970(
                RETimeRes *res);

timeval_t   retime_get_timeval(
                RETimeRes *res);

RETimeList *retime_list_compile(
                int          npatterns,
                const char **patterns,
                int          flags);

int         retime_list_execute(
                RETimeList  *retime_list,
                const char  *string,
                RETimeRes   *res);

void        retime_list_free(
                RETimeList *retime_list);

/*@}*/

#endif /* _REGEX_TIME_H */
