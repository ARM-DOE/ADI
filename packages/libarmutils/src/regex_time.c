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
*    $Revision: 75245 $
*    $Author: ermold $
*    $Date: 2016-12-06 01:25:47 +0000 (Tue, 06 Dec 2016) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file regex_time.c
 *  Regex Time Utilities
 */

#include "armutils.h"

/*******************************************************************************
 *  Private Data and Functions
 */
/** @privatesection */

/*

Add Time Zone (Z | +hh:mm | -hh:mm | time_zone_code)

   'Z' = Z or time zone abbreviation
   'z' = Z or +hh:mm or -hh:mm

 -> (Z| +[[:alpha:]{3}]|[+-][[:digit:]]{2}:[[[:digit:]]{2}])

Add microseconds 'u' (this is not fractional seconds)

    0-999999

Support for AM/PM (see strftime and strptime for format code)

*/

typedef struct DateTimeCode {
    char        chr;
    const char *str;
} DateTimeCode;

static DateTimeCode _DateTimeCodes[] = {

    { 'C', "([[:digit:]]{2})"    }, // century number (year/100) as a 2-digit integer
    { 'd', "([[:digit:]]{1,2})"  }, // day number in the month (1-31).
    { 'e', "([[:digit:]]{1,2})"  }, // day number in the month (1-31).
    { 'h', "([[:digit:]]{1,4})"  }, // hour and minute (0-2359)
    { 'H', "([[:digit:]]{1,2})"  }, // hour (0-23)
    { 'j', "([[:digit:]]{1,3})"  }, // day number in the year (1-366).
    { 'm', "([[:digit:]]{1,2})"  }, // month number (1-12)
    { 'M', "([[:digit:]]{1,2})"  }, // minute (0-59)
    { 'n', "[[:space:]]+"        }, // arbitrary whitespace
    // time offset in seconds
    { 'o', "([+-]*[[:digit:]]*\\.[[:digit:]]+|[+-]*[[:digit:]]+)" },
    { 'p', "([aApP][mM])"        }, // AM or PM (not case sensitive)
    // Mac-Time: seconds since 1904-01-01 00:00:00 +0000 (UTC) with optional fractional seconds
    { 'q', "([[:digit:]]+\\.[[:digit:]]+|[[:digit:]]+)" },
    // seconds since Epoch, 1970-01-01 00:00:00 +0000 (UTC) with optional fractional seconds
    { 's', "([[:digit:]]+\\.[[:digit:]]+|[[:digit:]]+)" },
    // second (0-60; 60 may occur for leap seconds) with optional fractional seconds
    { 'S', "([[:digit:]]{1,2}\\.[[:digit:]]+|[[:digit:]]{1,2})" },
    { 't', "[[:space:]]+"        }, // arbitrary whitespace
    { 'y', "([[:digit:]]{1,2})"  }, // year within century (0-99)
    { 'Y', "([[:digit:]]{4})"    }, // year with century as a 4-digit integer
    { '%', "%"                   }, // a literal "%" character
    { '\0', NULL                 }
};

static DateTimeCode _DateTimeCodes0[] = {

    { 'C', "([[:digit:]]{2})"    }, // century number (year/100) as a 2-digit integer
    { 'd', "([[:digit:]]{2})"    }, // day number in the month (range 01 to 31).
    { 'e', "([[:digit:]]{2})"    }, // day number in the month (range 01 to 31).
    { 'h', "([[:digit:]]{4})"    }, // hour and minute (0000-2359)
    { 'H', "([[:digit:]]{2})"    }, // hour (00-23)
    { 'j', "([[:digit:]]{3})"    }, // day number in the year (001-366).
    { 'm', "([[:digit:]]{2})"    }, // month number (01-12)
    { 'M', "([[:digit:]]{2})"    }, // minute (00-59)
    { 'n', "[[:space:]]+"        }, // arbitrary whitespace
    { 'o', "([+-]*[[:digit:]]+)" }, // time offset in seconds
    { 'p', "([aApP][mM])"        }, // AM or PM (not case sensitive)
    { 'q', "([[:digit:]]+)"      }, // Mac-Time: seconds since 1904-01-01 00:00:00 +0000 (UTC)
    { 's', "([[:digit:]]+)"      }, // seconds since Epoch, 1970-01-01 00:00:00 +0000 (UTC)
    { 'S', "([[:digit:]]{2})"    }, // second (00-60; 60 may occur for leap seconds)
    { 't', "[[:space:]]+"        }, // arbitrary whitespace
    { 'y', "([[:digit:]]{2})"    }, // year within century (00-99)
    { 'Y', "([[:digit:]]{4})"    }, // year with century as a 4-digit integer
    { '%', "%"                   }, // a literal "%" character
    { '\0', NULL                 }
};

/**
 *  PRIVATE: Parse a regex-time format string
 *
 *  The memory used by the returned array is dynamically allocated and must be
 *  freed by the calling process when it is no longer needed.
 *
 *  @param  retime   pointer to the RETime structure to use.
 *
 *  @param  pattern  the time string pattern containing a mixture of regex and
 *                   time format characters similar to the strptime function.
 *
 *  @retval  1  if succesful
 *  @retval  0  if an error occurred
 */
static int __retime_parse(RETime *retime, const char *pattern)
{
    char       *regex_pattern;
    const char *sp;
    char       *rp;
    int         length;
    int         mi, dti;

    DateTimeCode *dtc;

    if (strchr(pattern, '(')) {

// BDE: we can deal with this if/when we need to...

        ERROR( ARMUTILS_LIB_NAME,
            "Invalid regex/time pattern: '%s'\n"
            " -> regex/time pattern can not contain the '(' character.\n",
            pattern);

        return(0);
    }

    length        = strlen(pattern) + 512;
    regex_pattern = calloc(length, sizeof(char));

    if (!regex_pattern) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not parse time string pattern: '%s'\n"
            " -> memory allocation error\n",
            pattern);

        return(0);
    }

    sp = pattern;
    rp = regex_pattern;
    mi = 0;

    retime->codes[0] = -1;

    while (*sp != '\0') {

        if (*sp == '%') {

            ++sp;

            if (*sp == '0') {
                dtc = _DateTimeCodes0;
                ++sp;
            }
            else {
                dtc = _DateTimeCodes;
            }

            for (dti = 0; dtc[dti].chr; ++dti) {

                if (*sp == dtc[dti].chr) {

                    strcpy(rp, dtc[dti].str);
                    if (*rp == '(') retime->codes[++mi] = *sp;

                    rp = strchr(rp, '\0');

                    break;
                }
            }

            if (!dtc[dti].chr) {

                if (dtc == _DateTimeCodes0) {
                    ERROR( ARMUTILS_LIB_NAME,
                        "Invalid time format code '%%0%c' found in: '%s'\n",
                        *sp, pattern);
                }
                else {
                    ERROR( ARMUTILS_LIB_NAME,
                        "Invalid time format code '%%%c' found in: '%s'\n",
                        *sp, pattern);
                }

                free(regex_pattern);
                return(0);
            }

            ++sp;
        }
        else {
            *rp++ = *sp++;
        }
    }

    retime->pattern = regex_pattern;
    retime->nsubs   = mi;

    return(1);
}

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Compile a regex/time pattern.
 *
 *  This function will compile a time string pattern containing a mixture of
 *  regex and time format codes similar to the strptime function.  The time
 *  format codes recognized by this function begin with a % and are followed
 *  by one of the following characters:
 *
 *    - 'C' century number (year/100) as a 2-digit integer
 *    - 'd' day number in the month (1-31).
 *    - 'e' day number in the month (1-31).
 *    - 'h' hour * 100 + minute (0-2359)
 *    - 'H' hour (0-23)
 *    - 'j' day number in the year (1-366).
 *    - 'm' month number (1-12)
 *    - 'M' minute (0-59)
 *    - 'n' arbitrary whitespace
 *    - 'o' time offset in seconds
 *    - 'p' AM or PM
 *    - 'q' Mac-Time: seconds since 1904-01-01 00:00:00 +0000 (UTC)
 *    - 's' seconds since Epoch, 1970-01-01 00:00:00 +0000 (UTC)
 *    - 'S' second (0-60; 60 may occur for leap seconds)
 *    - 't' arbitrary whitespace
 *    - 'y' year within century (0-99)
 *    - 'Y' year with century as a 4-digit integer
 *    - '%' a literal "%" character
 *
 *  An optional 0 character can be used between the % and format code to
 *  specify that the number must be zero padded. For example, '%0d' specifies
 *  that the day range is 01 to 31.
 *
 *  See the regex(7) man page for the descriptions of the regex patterns.
 *
 *  The memory used by the returned RETime structure is dynamically allocated
 *  and must be freed by the calling process using the retime_free() function.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  pattern  - the time string pattern containing a mixture of regex and
 *                     time format characters similar to the strptime function.
 *
 *  @param  flags    - reserved for control flags
 *
 *  @return
 *    - pointer to the RETime structure
 *    - NULL if an error occurred
 */
RETime *retime_compile(const char *pattern, int flags)
{
    RETime  *retime;
    regex_t *preg;
    size_t   nsubs;
    int      cflags;

    flags  = flags; // prevent compiler warning

    cflags = REG_EXTENDED;

    /* Create the RETime structure */

    retime = (RETime *)calloc(1, sizeof(RETime));
    if (!retime) goto MEMORY_ERROR;

    /* Store a copy of the original time string pattern */

    retime->tspattern = strdup(pattern);
    if (!retime->tspattern) goto MEMORY_ERROR;

    /* Allocate memory to store the match codes */

    retime->codes = calloc(32, sizeof(char));

    /* Parse the time string pattern */

    if (!__retime_parse(retime, pattern)) {
        retime_free(retime);
        return((RETime *)NULL);
    }

    /* Compile the regular expression */

    preg = &(retime->preg);

    if (!re_compile(preg, retime->pattern, cflags)) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not compile regex/time pattern\n"
            " -> time string pattern:  '%s'\n"
            " -> regex string pattern: '%s'\n",
            pattern, retime->pattern);

        retime_free(retime);
        return((RETime *)NULL);
    }

    /* Create array to store substring offsets */

    nsubs = retime->nsubs;

    if (!nsubs) {

        ERROR( ARMUTILS_LIB_NAME,
            "Invalid regex/time pattern: '%s'\n"
            " -> no time format codes found in pattern\n",
            pattern);

        retime_free(retime);
        return((RETime *)NULL);
    }

    if (nsubs > RETIME_MAX_NSUBS) {

        ERROR( ARMUTILS_LIB_NAME,
            "Invalid regex/time pattern: '%s'\n"
            " -> number of subexpressions '%d' exceeds the maximum number allowed '%d'\n",
            pattern, nsubs, RETIME_MAX_NSUBS);

        retime_free(retime);
        return((RETime *)NULL);
    }

    return(retime);

MEMORY_ERROR:

    ERROR( ARMUTILS_LIB_NAME,
        "Could not compile regex/time pattern: '%s'\n"
        " -> memory allocation error\n",
        pattern);

    if (retime) retime_free(retime);
    return((RETime *)NULL);
}

/**
 *  Compare a string with a compiled regex/time pattern.
 *
 *  Results from the pattern match are stored in the following RETimeRes
 *  structure members:
 *
 *    - year:     year with century as a 4-digit integer
 *    - month:    month number (1-12)
 *    - mday:     day number in the month (1-31)
 *    - hour:     hour (0-23)
 *    - min:      minute (0-59)
 *    - sec:      second (0-60; 60 may occur for leap seconds)
 *    - usec:     micro-seconds
 *    - century:  century number (year/100) as a 2-digit integer
 *    - yy:       year number in century as a 2-digit integer
 *    - yday:     day number in the year (1-366)
 *    - hhmm:     hour * 100 + minute
 *    - secs1970: seconds since Epoch, 1970-01-01 00:00:00
 *    - offset:   time offset in seconds
 *
 *  All values that have not been set by the pattern match will be set to -1,
 *  except for the offset values which will be set to 0.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  retime  - pointer to the compiled RETime structure
 *  @param  string  - string to compare with the regular expression
 *  @param  res     - pointer to the structure to store the results of
 *                    the pattern match
 *
 *  @return
 *    -  1  if match
 *    -  0  if no match
 *    - -1  if an error occurred
 */
int retime_execute(RETime *retime, const char *string, RETimeRes *res)
{
    regex_t    *preg    = &(retime->preg);
    size_t      nsubs   = retime->nsubs;
    regmatch_t *pmatch  = res->pmatch;
    size_t      nmatch  = nsubs + 1;

    char        substr[RETIME_MAX_SUBSTR_LENGTH];
    int         negative;
    int         status;
    int         length;
    char       *chrp;
    size_t      mi;
    char        am_pm[8];

    /* Clear previous result */

    res->year           = -1;
    res->month          = -1;
    res->mday           = -1;
    res->hour           = -1;
    res->min            = -1;
    res->sec            = -1;
    res->usec           = -1;
    res->century        = -1;
    res->yy             = -1;
    res->yday           = -1;
    res->hhmm           = -1;
    res->secs1970       = -1;
    res->offset.tv_sec  =  0;
    res->offset.tv_usec =  0;

    res->res_time       = -1;
    res->res_tv.tv_sec  = -1;
    res->res_tv.tv_usec = -1;

    /* Check for match */

    status = re_execute(preg, string, nmatch, pmatch, 0);
    if (status  < 0) return(-1);
    if (status == 0) return (0);

    /* Set the results in the RETimeRes structure */

    am_pm[0] = '\0';

    for (mi = 1; mi < nmatch; mi++) {

        if (pmatch[mi].rm_so == -1) return(0);

        length = pmatch[mi].rm_eo - pmatch[mi].rm_so;

        if (length >= RETIME_MAX_SUBSTR_LENGTH) {

            ERROR( ARMUTILS_LIB_NAME,
                "Invalid time string: '%s'\n"
                " -> length of subexpression '%d' exceeds the maximum substring length '%d'\n",
                string, mi, RETIME_MAX_SUBSTR_LENGTH);

            return(-1);
        }

        strncpy(substr, (string + pmatch[mi].rm_so), length);
        substr[length] = '\0';

        switch (retime->codes[mi]) {

            case 'C': // century number (year/100) as a 2-digit integer
                res->century = atoi(substr);
                break;
            case 'd': // day number in the month (1-31).
            case 'e': // day number in the month (1-31).
                res->mday = atoi(substr);
                break;
            case 'h': // hour * 100 + minute (0-2400; 2400 is used by CDLs)
                res->hhmm = atoi(substr);
                break;
            case 'H': // hour (0-23)
                res->hour = atoi(substr);
                break;
            case 'j': // day number in the year (1-366).
                res->yday = atoi(substr);
                break;
            case 'm': // month number (1-12)
                res->month = atoi(substr);
                break;
            case 'M': // minute (0-59)
                res->min = atoi(substr);
                break;
            case 'o': // time offset in seconds

                negative = (substr[0] == '-') ? 1 : 0;

                res->offset.tv_sec = atoi(substr);

                chrp = strchr(substr, '.');
                if (chrp) {

                    res->offset.tv_usec = (int)(atof(chrp) * 1.0E6 + 0.5);

                    if (res->offset.tv_usec == 1.0E6) {

                        if (negative) {
                            res->offset.tv_sec -= 1;
                        }
                        else {
                            res->offset.tv_sec += 1;
                        }

                        res->offset.tv_usec  = 0;
                    }

                    if (negative) {
                        res->offset.tv_usec *= -1;
                    }
                }

                break;
            case 'p': // month number (1-12)
                strcpy(am_pm, substr);
                break;
            case 'q': // seconds since Epoch, 1904-01-01 00:00:00 +0000 (UTC)

                res->secs1970 = (time_t)(atoll(substr) - 2082844800LL);

                chrp = strchr(substr, '.');
                if (chrp) {
                    res->usec = (int)(atof(chrp) * 1.0E6 + 0.5);
                    if (res->usec == 1.0E6) {
                        res->secs1970 += 1;
                        res->usec      = 0;
                    }
                }

                break;

            case 's': // seconds since Epoch, 1970-01-01 00:00:00 +0000 (UTC)

                res->secs1970 = atoi(substr);

                chrp = strchr(substr, '.');
                if (chrp) {
                    res->usec = (int)(atof(chrp) * 1.0E6 + 0.5);
                    if (res->usec == 1.0E6) {
                        res->secs1970 += 1;
                        res->usec      = 0;
                    }
                }

                break;

            case 'S': // second (0-60; 60 may occur for leap seconds)

                res->sec = atoi(substr);

                chrp = strchr(substr, '.');
                if (chrp) {
                    res->usec = (int)(atof(chrp) * 1.0E6 + 0.5);
                    if (res->usec == 1.0E6) {
                        res->sec  += 1;
                        res->usec  = 0;
                    }
                }

                break;
            case 'y': // year within century (0-99)
                res->yy = atoi(substr);
                break;
            case 'Y': // year with century as a 4-digit integer
                res->year = atoi(substr);
                break;
            default:

                ERROR( ARMUTILS_LIB_NAME,
                    "Internal error in retime_execute() function\n"
                    " -> unsupported match code found: '%c'\n",
                    retime->codes[mi]);

                return(-1);
        }
    }

    /* Verify ranges and compute missing values where possible */

    if ((am_pm[0] != '\0') && (res->hour != -1)) {

        if (res->hour < 1 || res->hour > 12) return(0);

        if (strcasecmp(am_pm, "AM") == 0) {
            if (res->hour == 12) res->hour = 0;
        }
        else { // PM
            if (res->hour != 12) res->hour += 12;
        }
    }

    if ((res->month != -1) && (res->month >  12)) return(0);
    if ((res->mday  != -1) && (res->mday  >  31)) return(0);
    if ((res->hour  != -1) && (res->hour  >  23)) return(0);
    if ((res->min   != -1) && (res->min   >  59)) return(0);
    if ((res->sec   != -1) && (res->sec   >  60)) return(0);

    /* Compute year from century and/or year within century */

    if (res->year == -1) {

        if (res->yy != -1) {

            if (res->century != -1) {
                res->year = (res->century * 100);
            }
            else {
                res->year = (res->yy < 69) ? 2000 : 1900;
            }

            res->year += res->yy;
        }
    }

    /* Compute month and day from day number in year */

    if (res->yday != -1) {

        if (res->yday > 366) return(0);

        if (res->year != -1) {

            yday_to_mday(res->yday,
                &(res->year), &(res->month), &(res->mday));
        }
    }

    /* Compute hour and minute from hhmm format */

    if (res->hhmm != -1) {

        if (res->hhmm > 2400) return(0);

        res->hour = (int)(res->hhmm/100);
        res->min  = (int)(res->hhmm%100);

        if (res->min > 59) return(0);
    }

    res->retime = retime;

    return(1);
}

/**
 *  Free a RETime structure.
 *
 *  @param  retime - pointer to the RETime structure
 */
void retime_free(RETime *retime)
{
    if (retime) {
        if (retime->tspattern) free(retime->tspattern);
        if (retime->codes)     free(retime->codes);
        if (retime->pattern)   free(retime->pattern);
        regfree(&(retime->preg));
        free(retime);
    }
}

/**
 *  Get the result from a regex/time pattern match.
 *
 *  This function will use the result from retime_execute() stored in the
 *  RETime structure to compute the time in seconds since 1970.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  res - pointer to the RETimeRes structure
 *
 *  @return
 *      - seconds since 1970
 *      - -1 if the year was not set in the structure
 */
time_t retime_get_secs1970(RETimeRes *res)
{
    struct tm tm_time;
    int       usec;

    /* Don't compute result again */

    if (res->res_time != -1) {
        return(res->res_time);
    }

    /* Get seconds since 1970 */

    if (res->secs1970 != -1) {
        res->res_time = res->secs1970;
    }
    else if (res->year == -1) {
        return(-1);
    }
    else {

        memset(&tm_time, 0, sizeof(struct tm));

        tm_time.tm_year = res->year - 1900;

        if (res->month != -1) tm_time.tm_mon  = res->month - 1;
        if (res->mday  != -1) tm_time.tm_mday = res->mday;
        if (res->hour  != -1) tm_time.tm_hour = res->hour;
        if (res->min   != -1) tm_time.tm_min  = res->min;
        if (res->sec   != -1) tm_time.tm_sec  = res->sec;

        res->res_time = timegm(&tm_time);
    }

    /* Adjust time for offset and microseconds */

    usec = (res->usec == -1) ? 0: res->usec;

    if (res->offset.tv_sec || res->offset.tv_usec) {

        res->res_time += res->offset.tv_sec;
        usec          += res->offset.tv_usec;

        if (usec > 1.0E6) {
            res->res_time += 1;
            usec          -= 1.0E6;
        }
    }

    if (usec) {
        if (usec >= 0.5E6) {
            res->res_time += 1;
        }
        else if (usec <= -0.5E6) {
            res->res_time -= 1;
        }
    }

    return(res->res_time);
}

/**
 *  Get the result from a regex/time pattern match.
 *
 *  This function will use the result from retime_execute() stored in the
 *  RETime structure to compute the time in seconds since 1970.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  res - pointer to the RETimeRes structure
 *
 *  @return
 *      - seconds since 1970
 *      - tv.tv_sec == -1 if the year was not set in the structure
 */
timeval_t retime_get_timeval(RETimeRes *res)
{
    struct tm tm_time;
    int       usec;

    /* Don't compute result again */

    if (res->res_tv.tv_sec != -1) {
        return(res->res_tv);
    }

    /* Get seconds since 1970 */

    if (res->secs1970 != -1) {
        res->res_tv.tv_sec = res->secs1970;
    }
    else if (res->year == -1) {
        return(res->res_tv);
    }
    else {

        memset(&tm_time, 0, sizeof(struct tm));

        tm_time.tm_year = res->year - 1900;

        if (res->month != -1) tm_time.tm_mon  = res->month - 1;
        if (res->mday  != -1) tm_time.tm_mday = res->mday;
        if (res->hour  != -1) tm_time.tm_hour = res->hour;
        if (res->min   != -1) tm_time.tm_min  = res->min;
        if (res->sec   != -1) tm_time.tm_sec  = res->sec;

        res->res_tv.tv_sec = timegm(&tm_time);
    }

    /* Compute microseconds and adjust for offsets */

    usec = (res->usec == -1) ? 0: res->usec;

    if (res->offset.tv_sec || res->offset.tv_usec) {

        res->res_tv.tv_sec += res->offset.tv_sec;
        usec               += res->offset.tv_usec;

        if (usec > 1.0E6) {
            res->res_tv.tv_sec += 1;
            usec               -= 1.0E6;
        }
    }

    if (usec < 0) {
        res->res_tv.tv_sec -= 1;
        usec += 1.0E6;
    }

    res->res_tv.tv_usec = usec;

    return(res->res_tv);
}

/**
 *  Compile a list of regex/time patterns.
 *
 *  See retime_compile() for a description of the pattern strings.
 *
 *  The memory used by the returned RETimeList structure is dynamically
 *  allocated and must be freed by the calling process using the
 *  retime_list_free() function.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  npatterns - number of pattern strings in the list
 *  @param  patterns  - list of time string patterns containing a mixture of
 *                      regex and time format characters similar to the
 *                      strptime function.
 *  @param  flags     - reserved for control flags
 *
 *  @return
 *    - pointer to the RETimeList structure
 *    - NULL if an error occurred
 */
RETimeList *retime_list_compile(
    int          npatterns,
    const char **patterns,
    int          flags)
{
    RETimeList *retime_list;
    int         pi;

    retime_list = calloc(1, sizeof(RETimeList));
    if (!retime_list) {
        ERROR( ARMUTILS_LIB_NAME,
            "Memory allocation error creating RETimeList\n");
        return((RETimeList *)NULL);
    }

    retime_list->retimes = calloc(npatterns + 1, sizeof(RETime *));
    if (!retime_list->retimes) {
        ERROR( ARMUTILS_LIB_NAME,
            "Memory allocation error creating RETimeList\n");
        return((RETimeList *)NULL);
    }

    retime_list->npatterns = npatterns;

    for (pi = 0; pi < npatterns; ++pi) {

        retime_list->retimes[pi] = retime_compile(patterns[pi], flags);
        if (!retime_list->retimes[pi]) {
            return((RETimeList *)NULL);
        }
    }

    return(retime_list);
}

/**
 *  Compare a string with a list of regex/time patterns.
 *
 *  Results from the pattern match are stored in the output RETime
 *  structure members:
 *
 *    - year:     year with century as a 4-digit integer
 *    - month:    month number (1-12)
 *    - mday:     day number in the month (1-31)
 *    - hour:     hour (0-23)
 *    - min:      minute (0-59)
 *    - sec:      second (0-60; 60 may occur for leap seconds)
 *    - usec:     micro-seconds
 *    - century:  century number (year/100) as a 2-digit integer
 *    - yy:       year number in century as a 2-digit integer
 *    - yday:     day number in the year (1-366)
 *    - hhmm:     hour * 100 + minute
 *    - secs1970: seconds since Epoch, 1970-01-01 00:00:00
 *    - offset:   time offset in seconds
 *
 *  All values that have not been set by the pattern match will be set to -1,
 *  except for the offset values which will be set to 0.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  retime_list - pointer to the RETimeList structure
 *  @param  string      - string to compare with the list of RETime patterns
 *  @param  res         - pointer to the structure to store the results of
 *                        the first pattern that was matched
 *
 *  @return
 *    -  1  if a match was found
 *    -  0  if no match was found
 *    - -1  if an error occurred
 */
int retime_list_execute(
    RETimeList  *retime_list,
    const char  *string,
    RETimeRes   *res)
{
    RETime *retime;
    int     status;
    int     pi;

    for (pi = 0; pi < retime_list->npatterns; ++pi) {

        retime = retime_list->retimes[pi];
        status = retime_execute(retime, string, res);

        if (status < 0) return(-1);
        if (status > 0) {
            return(1);
        }
    }

    return(0);
}

/**
 *  Free a RETimeList Structure
 *
 *  @param  retime_list - pointer to the RETimeList
 */
void retime_list_free(RETimeList *retime_list)
{
    int pi;

    if (retime_list) {

        if (retime_list->retimes) {

            for (pi = 0; pi < retime_list->npatterns; ++pi) {
                retime_free(retime_list->retimes[pi]);
            }

            free(retime_list->retimes);
        }

        free(retime_list);
    }
}
