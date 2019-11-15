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
*    $Revision: 6421 $
*    $Author: ermold $
*    $Date: 2011-04-26 01:23:44 +0000 (Tue, 26 Apr 2011) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file time_utils.h
 *  Time Utilities
 */

#ifndef _TIME_UTILS_H
#define _TIME_UTILS_H

/**
 *  @defgroup ARMUTILS_TIME_UTILS Time Utils
 */
/*@{*/

/*******************************************************************************
*  Timeval typedef and macros
*/

#ifndef _TIMEVAL_T
#define _TIMEVAL_T
/**
 *  typedef for: struct timeval.
 *
 *  Structure Members:
 *
 *    - tv_sec  - seconds
 *    - tv_usec - microseconds
 */
typedef struct timeval timeval_t;
#endif  /* _TIMEVAL_T */

#ifndef _TIMEVAL_MACROS
#define _TIMEVAL_MACROS

/** Cast timeval to double. */
#define TV_DOUBLE(tv) \
    ( (double)(tv).tv_sec + (1E-6 * (double)(tv).tv_usec) )

/** Check if timeval 1 is equal to timeval 2. */
#define TV_EQ(tv1,tv2) \
    ( ( (tv1).tv_sec  == (tv2).tv_sec) && \
      ( (tv1).tv_usec == (tv2).tv_usec) )

/** Check if timeval 1 is not equal to timeval 2. */
#define TV_NEQ(tv1,tv2) \
    ( ( (tv1).tv_sec  != (tv2).tv_sec) || \
      ( (tv1).tv_usec != (tv2).tv_usec) )

/** Check if timeval 1 is greater than timeval 2. */
#define TV_GT(tv1,tv2) \
    ( ( (tv1).tv_sec == (tv2).tv_sec)  ? \
      ( (tv1).tv_usec > (tv2).tv_usec) : \
      ( (tv1).tv_sec  > (tv2).tv_sec) )

/** Check if timeval 1 is greater than or equal to timeval 2. */
#define TV_GTEQ(tv1,tv2) \
    ( ( (tv1).tv_sec == (tv2).tv_sec)  ? \
      ( (tv1).tv_usec >= (tv2).tv_usec) : \
      ( (tv1).tv_sec  > (tv2).tv_sec) )

/** Check if timeval 1 is less than timeval 2. */
#define TV_LT(tv1,tv2) \
    ( ( (tv1).tv_sec == (tv2).tv_sec)  ? \
      ( (tv1).tv_usec < (tv2).tv_usec) : \
      ( (tv1).tv_sec  < (tv2).tv_sec) )

/** Check if timeval 1 is less than or equal to timeval 2. */
#define TV_LTEQ(tv1,tv2) \
    ( ( (tv1).tv_sec == (tv2).tv_sec)  ? \
      ( (tv1).tv_usec <= (tv2).tv_usec) : \
      ( (tv1).tv_sec  < (tv2).tv_sec) )

#endif  /* _TIMEVAL_MACROS */

/** Check if a 4 digit year is a leap year. */
#define IS_LEAP_YEAR(year) \
    ( ( (year % 4) == 0 && (year % 100) != 0 ) || (year % 400) == 0 )

int days_in_month(int year, int month);

char *format_secs1970(time_t secs1970, char *time_string);

char *format_time_values(
    int year, int mon, int day,
    int hour, int min, int sec, int usec,
    char *time_string);

char *format_timeval(const timeval_t *tval, char *time_string);

int four_digit_year(int year);

time_t get_secs1970(
    int year, int mon, int day, int hour, int min, int sec);

time_t normalize_time_values(
    int *year, int *mon, int *day, int *hour, int *min, int *sec, int *usec);

void normalize_timeval(timeval_t *tval);

int time_values_check(
    int year, int mon, int day, int hour, int min, int sec, int usec);

#ifndef __GNUC__
time_t timegm(const struct tm *tm_time);
#endif

void timeval_add(timeval_t *tv1, const timeval_t *tv2);
void timeval_subtract(timeval_t *tv1, const timeval_t *tv2);

void yday_to_mday(
    int yday, int *year, int *month, int *mday);

/*@}*/

#endif /* _TIME_UTILS_H */
