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
*    $Revision: 18200 $
*    $Author: ermold $
*    $Date: 2013-07-16 22:14:00 +0000 (Tue, 16 Jul 2013) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file time_utils.c
 *  Time Functions.
 */

#include "armutils.h"

/*******************************************************************************
 *  Private Functions
 */
/** @privatesection */

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Get the number of days in a month.
 *
 *  @param  year  - four digit year
 *  @param  month - month [1..12]
 *
 *  @returns number of days in month
 */
int days_in_month(int year, int month)
{
    int month_days[12] = {31,28,31,30,31,30,31,31,30,31,30,31};

    if (IS_LEAP_YEAR(year)) {
        month_days[1] = 29;
    }

    return(month_days[month-1]);
}

/**
 *  Create formatted time string.
 *
 *  This function will create a time string of the form:
 *
 *      'YYYY-MM-DD hh:mm:ss'.
 *
 *  The output time_string argument must be large enough to hold at least
 *  27 characters (see format_time_values()).
 *
 *  If an error occurs in this function, the message 'FORMATTING ERROR' will
 *  be copied into the time_string.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  secs1970    - seconds since 1970
 *  @param  time_string - output: formated time string
 *
 *  @return pointer to the time_string
 */
char *format_secs1970(time_t secs1970, char *time_string)
{
    struct tm tm_time;

    memset(&tm_time, 0, sizeof(struct tm));

    if (!gmtime_r(&secs1970, &tm_time)) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not format seconds since 1970: %d\n"
            " -> gmtime error: %s\n",
            (int)secs1970, strerror(errno));

        strcpy(time_string, "FORMATTING ERROR");
    }
    else {

        format_time_values(
            tm_time.tm_year + 1900,
            tm_time.tm_mon  + 1,
            tm_time.tm_mday,
            tm_time.tm_hour,
            tm_time.tm_min,
            tm_time.tm_sec,
            0, time_string);
    }

    return(time_string);
}

/**
 *  Create formatted time string.
 *
 *  This function will create a time string of the form:
 *
 *      'YYYY-MM-DD hh:mm:ss[.ssssss]'.
 *
 *  The output time_string argument must be large enough to hold at least
 *  27 characters (26 plus the null terminator).
 *
 *  If an error occurs in this function, the message 'FORMATTING ERROR' will
 *  be copied into the time_string.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  year        - four digit year
 *  @param  mon         - month        [1..12]
 *  @param  day         - day          [1..month days]
 *  @param  hour        - hour         [0..23]
 *  @param  min         - minute       [0..59]
 *  @param  sec         - second       [0..59]
 *  @param  usec        - microseconds [0...999999]
 *  @param  time_string - output: formated time string
 *
 *  @return pointer to the time_string
 */
char *format_time_values(
    int year, int mon, int day,
    int hour, int min, int sec, int usec,
    char *time_string)
{
    char *chrp;

    sprintf(time_string,
        "%04d-%02d-%02d %02d:%02d:%02d.%06d",
        year, mon, day, hour, min, sec, usec);

    chrp = time_string + strlen(time_string) - 1;

    for (; *chrp == '0'; chrp--) *chrp = '\0';

    if (*chrp == '.') *chrp = '\0';

    return(time_string);
}

/**
 *  Create formatted time string.
 *
 *  This function will create a time string of the form:
 *
 *      'YYYY-MM-DD hh:mm:ss[.ssssss]'.
 *
 *  The output time_string argument must be large enough to hold at least
 *  27 characters (26 plus the null terminator).
 *
 *  If an error occurs in this function, the message 'FORMATTING ERROR' will
 *  be copied into the time_string.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  tval        - timeval structure
 *  @param  time_string - output: formated time string
 *
 *  @return pointer to the time_string
 */
char *format_timeval(const timeval_t *tval, char *time_string)
{
    timeval_t tv_time = {0, 0};
    struct tm tm_time;

    memset(&tm_time, 0, sizeof(struct tm));

    if (tval) tv_time = *tval;

    if (!gmtime_r(&tv_time.tv_sec, &tm_time)) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not format timeval: %d s, %d us\n"
            " -> gmtime error: %s\n",
            (int)tv_time.tv_sec, (int)tv_time.tv_usec, strerror(errno));

        strcpy(time_string, "FORMATTING ERROR");
    }
    else {

        format_time_values(
            tm_time.tm_year + 1900,
            tm_time.tm_mon  + 1,
            tm_time.tm_mday,
            tm_time.tm_hour,
            tm_time.tm_min,
            tm_time.tm_sec,
            tv_time.tv_usec, time_string);
    }

    return(time_string);
}

/**
 *  Get the 4 digit year.
 *
 *  This function will take a 2 digit year or years since 1900 and convert
 *  it to a four digit year. For 2 digit years this function works from 1990
 *  to 2089. For years since 1900 this function works from 1990 to 3889. If
 *  the value of the specified year is greater than 1989 it will be returned
 *  unchanged.
 *
 *  @param  year - two digit year, years since 1900, or a four digit year
 *
 *  @returns four digit year
 */
int four_digit_year(int year)
{
    if (year < 1990) {
        year = (year < 90) ? year + 2000 : year + 1900;
    }

    return(year);
}

/**
 *  Convert time values to seconds since 1970.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  year - four digit year
 *  @param  mon  - month  [1..12]
 *  @param  day  - day    [1..month days]
 *  @param  hour - hour   [0..23]
 *  @param  min  - minute [0..59]
 *  @param  sec  - second [0..59]
 *
 *  @return
 *    - seconds since 1970
 *    - 0 if an error occured
 */
time_t get_secs1970(
    int year, int mon, int day, int hour, int min, int sec)
{
    struct tm tm_time;
    time_t    secs1970;

    memset(&tm_time, 0, sizeof(struct tm));

    tm_time.tm_year = year - 1900;
    tm_time.tm_mon  = mon - 1;
    tm_time.tm_mday = day;
    tm_time.tm_hour = hour;
    tm_time.tm_min  = min;
    tm_time.tm_sec  = sec;

    secs1970 = mktime(&tm_time);

    if (secs1970 == (time_t)-1) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not calculate seconds since 1970 for: %d-%d-%d %d:%d:%d\n"
            " -> mktime error: %s\n",
            year, mon, day, hour, min, sec, strerror(errno));

        return(0);
    }

    secs1970 -= timezone;

    return(secs1970);
}

/**
 *  Normalize time values.
 *
 *  This function will normailize the specified time values into
 *  their valid ranges.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  year - year        see four_digit_year()
 *  @param  mon  - month       [1..12]
 *  @param  day  - day         [1..month days]
 *  @param  hour - hour        [0..23]
 *  @param  min  - minute      [0..59]
 *  @param  sec  - second      [0..59]
 *  @param  usec - microsecond [0..999999]
 *
 *  @return
 *      - seconds since 1970
 *      - 0 if an error occurred
 */
time_t normalize_time_values(
    int *year, int *mon, int *day, int *hour, int *min, int *sec, int *usec)
{
    struct tm tm_time;
    time_t    secs1970;

    memset(&tm_time, 0, sizeof(struct tm));

    *year  = four_digit_year(*year);
    *sec  += *usec/1000000;
    *usec %= 1000000;

    if (*usec < 0) {
        *sec  -= 1;
        *usec += 1000000;
    }

    tm_time.tm_year = *year - 1900;
    tm_time.tm_mon  = *mon - 1;
    tm_time.tm_mday = *day;
    tm_time.tm_hour = *hour;
    tm_time.tm_min  = *min;
    tm_time.tm_sec  = *sec;

    secs1970 = mktime(&tm_time);

    if (secs1970 == (time_t)-1) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not normalize time values for: %d-%d-%d %d:%d:%d\n"
            " -> mktime error: %s\n",
            *year, *mon, *day, *hour, *min, *sec, strerror(errno));

        return(0);
    }

    secs1970 -= timezone;

    *year = tm_time.tm_year + 1900;
    *mon  = tm_time.tm_mon + 1;
    *day  = tm_time.tm_mday;
    *hour = tm_time.tm_hour;
    *min  = tm_time.tm_min;
    *sec  = tm_time.tm_sec;

    return(secs1970);
}

/**
 *  Normalize a timeval.
 *
 *  This function will normalize the specified timeval so
 *  that the tv_usec member is within the range [0..999999].
 *
 *  @param  tval - pointer to the timeval
 */
void normalize_timeval(timeval_t *tval)
{
    tval->tv_sec  += tval->tv_usec/1000000;
    tval->tv_usec %= 1000000;

    if (tval->tv_usec < 0) {
        tval->tv_sec--;
        tval->tv_usec += 1000000;
    }
}

/**
 *  Check time values.
 *
 *  This function will verify that all date and time values are
 *  within their expected ranges.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  year - four digit year (>= 1990)
 *  @param  mon  - month       [1..12]
 *  @param  day  - day         [1..month days]
 *  @param  hour - hour        [0..23]
 *  @param  min  - minute      [0..59]
 *  @param  sec  - second      [0..59]
 *  @param  usec - microsecond [0..999999]
 *
 *  @return
 *      - 1 if the time values are ok
 *      - 0 if one or more of the time values were out of range
 */
int time_values_check(
    int year, int mon, int day, int hour, int min, int sec, int usec)
{
    int month_days;
    char ts[32];

    if (year < 1990) {

        ERROR( ARMUTILS_LIB_NAME,
            "Invalid time: %s\n"
            " -> year %d is less than 1990\n",
            format_time_values(year, mon, day, hour, min, sec, usec, ts), year);

        return(0);
    }
    else if ((mon < 1) || (mon > 12)) {

        ERROR( ARMUTILS_LIB_NAME,
            "Invalid time: %s\n"
            " -> month %d is not between 1 and 12\n",
            format_time_values(year, mon, day, hour, min, sec, usec, ts), mon);

        return(0);
    }

    month_days = days_in_month(year, mon);

    if ((day < 1) || (day > month_days)) {

        ERROR( ARMUTILS_LIB_NAME,
            "Invalid time: %s\n"
            " -> day %d is not between 0 and %d\n",
            format_time_values(year, mon, day, hour, min, sec, usec, ts),
            day, month_days);

        return(0);
    }
    else if ((hour < 0) || (hour > 23)) {

        ERROR( ARMUTILS_LIB_NAME,
            "Invalid Time: %s\n"
            " -> hour %d is not between 0 and 23\n",
            format_time_values(year, mon, day, hour, min, sec, usec, ts), hour);

        return(0);
    }
    else if ((min < 0) || (min > 59)) {

        ERROR( ARMUTILS_LIB_NAME,
            "Invalid Time: %s\n"
            " -> minute %d is not between 0 and 59\n",
            format_time_values(year, mon, day, hour, min, sec, usec, ts), min);

        return(0);
    }
    else if ((sec < 0) || (sec > 59)) {

        ERROR( ARMUTILS_LIB_NAME,
            "Invalid Time: %s\n"
            " -> second %d is not between 0 and 59\n",
            format_time_values(year, mon, day, hour, min, sec, usec, ts), sec);

        return(0);
    }
    else if ((usec < 0) || (usec > 999999)) {

        ERROR( ARMUTILS_LIB_NAME,
            "Invalid Time: %s\n"
            " -> microsecond %d is not between 0 and 999999\n",
            format_time_values(year, mon, day, hour, min, sec, usec, ts), usec);

        return(0);
    }

    return(1);
}

#ifndef __GNUC__
/**
 *  Convert a struct tm time to seconds since 1970.
 *
 *  This function does the reverse of gmtime.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  tm_time - pointer to the tm structure.
 *
 *  @return
 *    - seconds since 1970
 *    - 0 if an error occurred
 */
time_t timegm(const struct tm *tm_time)
{
    struct tm gmt = *tm_time;
    time_t    secs1970;

    gmt.tm_isdst = 0;

    secs1970 = mktime(&gmt);

    if (secs1970 == (time_t)-1) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not convert struct tm to seconds since 1970\n"
            " -> mktime error: %s\n", strerror(errno));

        return(0);
    }

    secs1970 -= timezone;

    return(secs1970);
}
#endif

/**
 *  Add two timevals.
 *
 *  This function adds timeval 2 to timeval 1
 *
 *  @param  tv1 - timeval 1
 *  @param  tv2 - timeval 2
 */
void timeval_add(timeval_t *tv1, const timeval_t *tv2)
{
    tv1->tv_sec  += tv2->tv_sec;
    tv1->tv_usec += tv2->tv_usec;

    tv1->tv_sec  += tv1->tv_usec/1000000;
    tv1->tv_usec %= 1000000;

    if (tv1->tv_usec < 0) {
        tv1->tv_sec--;
        tv1->tv_usec += 1000000;
    }
}

/**
 *  Subtract two timevals.
 *
 *  This function subtracts timeval 2 from timeval 1.
 *
 *  @param  tv1 - timeval 1
 *  @param  tv2 - timeval 2
 */
void timeval_subtract(timeval_t *tv1, const timeval_t *tv2)
{
    tv1->tv_sec  -= tv2->tv_sec;
    tv1->tv_usec -= tv2->tv_usec;

    tv1->tv_sec  += tv1->tv_usec/1000000;
    tv1->tv_usec %= 1000000;

    if (tv1->tv_usec < 0) {
        tv1->tv_sec--;
        tv1->tv_usec += 1000000;
    }
}

/**
 *  Convert day of year to month and day of month.
 *
 *  This function will also increment the year value if yday
 *  extends beyond the number of days in the specified year.
 *
 *  @param  yday  - day of year          [1..year days]
 *  @param  year  - four digit year
 *  @param  month - output: month        [1..12]
 *  @param  mday  - output: day of month [1..month days]
 */
void yday_to_mday(int yday, int *year, int *month, int *mday)
{
    int month_days[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    int mon;

    if (IS_LEAP_YEAR(*year)) {
        month_days[1] = 29;
    }

    mon = 0;
    while (month_days[mon] < yday) {

        yday -= month_days[mon];
        mon  += 1;

        if (mon == 12) {
            mon = 0;
            (*year)++;
            month_days[1] = (IS_LEAP_YEAR(*year)) ? 29 : 28;
        }
    }

    *month = mon + 1;
    *mday  = yday;
}

/*@}*/
