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
*    $Revision: 16166 $
*    $Author: ermold $
*    $Date: 2012-12-07 22:39:18 +0000 (Fri, 07 Dec 2012) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file string_utils.c
 *  String Functions.
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
 *  Extract the major, minor, and micro values from a version string.
 *
 *  This function will look for the first occurrence of "%d.%d" in the specified
 *  version string.  These values will be returned as the major and minor version
 *  numbers respectively.  It will then check for an optional ".%d" or "-%d"
 *  following the version number.  If found, this value will be returned as the
 *  micro version number.
 *
 *  @param  string  pointer to the version string
 *  @param  major   output: major version number
 *  @param  minor   output: minor version number
 *  @param  micro   output: micro version number
 *
 *  @return
 *    - 3 if major, minor, and micro version numbers were found
 *    - 2 if major and minor version numbers were found
 *    - 0 if a valid version number was not found
 */
int parse_version_string(
    const char *string,
    int        *major,
    int        *minor,
    int        *micro)
{
    const char *strp   = string;
    int         nfound = 0;
    int         version[3];

    if (major) *major = 0;
    if (minor) *minor = 0;
    if (micro) *micro = 0;

    for (; *strp != '\0'; ++strp) {

        while ((*strp != '\0') && !isdigit(*strp)) ++strp;

        if (*strp == '\0') break;

        nfound = sscanf(strp, "%d.%d.%d",
            &version[0], &version[1], &version[2]);

        if (nfound == 2) {
            nfound = sscanf(strp, "%d.%d-%d",
                &version[0], &version[1], &version[2]);
        }

        if (nfound >= 2) break;
    }

    if (nfound < 2) {
        return(0);
    }

    if (major && (nfound >= 1)) *major = version[0];
    if (minor && (nfound >= 2)) *minor = version[1];
    if (micro && (nfound >= 3)) *micro = version[2];

    return(nfound);
}

/**
 *  Qsort numeric string compare function.
 *
 *  This function can be used by qsort to sort an array of string
 *  pointers by using the numeric values found within the strings.
 *  The first numbers found in the strings will be compared, if those
 *  are equal the next numbers found will be used, and so on and so
 *  fourth, until no more numbers are found in the strings. If all
 *  numeric values are equal, the strcmp result will be returned.
 *
 *  @param  str1 - pointer to pointer to string 1
 *  @param  str2 - pointer to pointer to string 2
 */
int qsort_numeric_strcmp(const void *str1, const void *str2)
{
    const char *s1 = *(const char **)str1;
    const char *s2 = *(const char **)str2;
    long n1, n2;

    while (1) {

        while (*s1 && !isdigit(*s1)) s1++;
        while (*s2 && !isdigit(*s2)) s2++;

        if (*s1 && *s2) {

            n1 = strtol(s1, (char **)&s1, 10);
            n2 = strtol(s2, (char **)&s2, 10);

            if      (n1 > n2) return(1);
            else if (n1 < n2) return(-1);
        }
        else if (*s1) return(1);
        else if (*s2) return(-1);
        else {
            return(strcmp(*(const char **)str1, *(const char **)str2));
        }
    }
}

/**
 *  Qsort string compare function.
 *
 *  This function can be used by qsort to sort an array of string pointers.
 *
 *  @param  str1 - pointer to pointer to string 1
 *  @param  str2 - pointer to pointer to string 2
 */
int qsort_strcmp(const void *str1, const void *str2)
{
    return(strcmp(*(const char **)str1, *(const char **)str2));
}

/**
 *  Copy a string.
 *
 *  The returned string is dynamically allocated and must be freed by
 *  the calling process.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  string - the string to copy
 *
 *  @return
 *    - pointer to the copy of the string
 *    - NULL if a memory allocation error occurred
 */
char *string_copy(const char *string)
{
    char *copy = strdup(string);

    if (!copy) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not copy string: '%s'\n"
            " -> memory allocation error\n", string);
    }

    return(copy);
}

/**
 *  Create a new string.
 *
 *  This functions creates a new string under the control of the format
 *  argument. The returned string is dynamically allocated and must be
 *  freed by the calling process.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  format - format string (see printf)
 *  @param  ...    - arguments for the format string
 *
 *  @return
 *    - pointer to the new string
 *    - NULL if a memory allocation error occurred
 */
char *string_create(const char *format, ...)
{
    va_list  args;
    char    *string;

    va_start(args, format);
    string = msngr_format_va_list(format, args);
    va_end(args);

    if (!string) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not create string for format string: '%s'\n"
            " -> memory allocation error\n", format);
    }

    return(string);
}

/**
 *  Create a new string.
 *
 *  This functions creates a new string under the control of the format
 *  argument. The returned string is dynamically allocated and must be
 *  freed by the calling process.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  format - format string (see printf)
 *  @param  args   - arguments for the format string
 *
 *  @return
 *    - pointer to the new string
 *    - NULL if a memory allocation error occurred
 */
char *string_create_va_list(const char *format, va_list args)
{
    char *string = msngr_format_va_list(format, args);

    if (!string) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not create string for format string: '%s'\n"
            " -> memory allocation error\n", format);
    }

    return(string);
}

/**
 *  Read numeric values from a string.
 *
 *  This function will read all the numerical values from a string
 *  and store them in the specified buffer. All strings of * characters
 *  will also be extracted as -9999.
 *
 *  @param  string - pointer to the string
 *  @param  buflen - the maximum number of values the buffer can hold
 *  @param  buffer - output buffer
 *
 *  @return
 *    The number of values read from the string or the number of
 *    values that would have been read from the string if the
 *    buffer had been large enough.
 */
int string_to_doubles(char *string, int buflen, double *buffer)
{
    int     nvals = 0;
    char   *endp  = (char *)NULL;
    char   *strp;
    double  value;

    for (strp = string; *strp != '\0'; ++strp) {

        while(*strp == ' ') ++strp;
        if (*strp == '\0') break;

        if (*strp == '*') {
            endp = strp;
            while(*endp == '*') ++endp;
            value = -9999.0;
        }
        else {
            value = strtod(strp, &endp);
        }

        if (endp != strp) {

            if (nvals < buflen)
                buffer[nvals] = value;

            nvals++;
            strp = endp;
            if (*strp == '\0') break;
        }
    }

    return(nvals);
}

/**
 *  Read numeric values from a string.
 *
 *  This function will read all the numerical values from a string
 *  and store them in the specified buffer. All strings of * characters
 *  will also be extracted as -9999.
 *
 *  @param  string - pointer to the string
 *  @param  buflen - the maximum number of values the buffer can hold
 *  @param  buffer - output buffer
 *
 *  @return
 *    The number of values read from the string or the number of
 *    values that would have been read from the string if the
 *    buffer had been large enough.
 */
int string_to_floats(char *string, int buflen, float *buffer)
{
    int     nvals = 0;
    char   *endp  = (char *)NULL;
    char   *strp;
    double  value;

    for (strp = string; *strp != '\0'; ++strp) {

        while(*strp == ' ') ++strp;
        if (*strp == '\0') break;

        if (*strp == '*') {
            endp = strp;
            while(*endp == '*') ++endp;
            value = -9999.0;
        }
        else {
            value = strtod(strp, &endp);
        }

        if (endp != strp) {

            if (nvals < buflen)
                buffer[nvals] = (float)value;

            nvals++;
            strp = endp;
            if (*strp == '\0') break;
        }
    }

    return(nvals);
}

/**
 *  Read numeric values from a string.
 *
 *  This function will read all the numerical values from a string
 *  and store them in the specified buffer. All strings of * characters
 *  will also be extracted as -9999.
 *
 *  @param  string - pointer to the string
 *  @param  buflen - the maximum number of values the buffer can hold
 *  @param  buffer - output buffer
 *
 *  @return
 *    The number of values read from the string or the number of
 *    values that would have been read from the string if the
 *    buffer had been large enough.
 */
int string_to_ints(char *string, int buflen, int *buffer)
{
    int   nvals = 0;
    char *endp  = (char *)NULL;
    char *strp;
    long  value;

    for (strp = string; *strp != '\0'; ++strp) {

        while(*strp == ' ') ++strp;
        if (*strp == '\0') break;

        if (*strp == '*') {
            endp = strp;
            while(*endp == '*') ++endp;
            value = -9999;
        }
        else {
            value = strtol(strp, &endp, 0);
        }

        if (endp != strp) {

            if (nvals < buflen)
                buffer[nvals] = (int)value;

            nvals++;
            strp = endp;
            if (*strp == '\0') break;
        }
    }

    return(nvals);
}

/**
 *  Read numeric values from a string.
 *
 *  This function will read all the numerical values from a string
 *  and store them in the specified buffer. All strings of * characters
 *  will also be extracted as -9999.
 *
 *  @param  string - pointer to the string
 *  @param  buflen - the maximum number of values the buffer can hold
 *  @param  buffer - output buffer
 *
 *  @return
 *    The number of values read from the string or the number of
 *    values that would have been read from the string if the
 *    buffer had been large enough.
 */
int string_to_longs(char *string, int buflen, long *buffer)
{
    int   nvals = 0;
    char *endp  = (char *)NULL;
    char *strp;
    long  value;

    for (strp = string; *strp != '\0'; ++strp) {

        while(*strp == ' ') ++strp;
        if (*strp == '\0') break;

        if (*strp == '*') {
            endp = strp;
            while(*endp == '*') ++endp;
            value = -9999;
        }
        else {
            value = strtol(strp, &endp, 0);
        }

        if (endp != strp) {

            if (nvals < buflen)
                buffer[nvals] = value;

            nvals++;
            strp = endp;
            if (*strp == '\0') break;
        }
    }

    return(nvals);
}

/**
 *  Trim the tag from a repository string.
 *
 *  This function will extract the repository value by trimming the
 *  leading tag and trailing $ from it.
 *
 *  @param  string - repository string
 *  @param  buflen - length of the output buffer
 *  @param  buffer - pointer to the output buffer
 *
 *  @return pointer to the output buffer
 */
char *trim_repository_string(
    const char *string,
    int         buflen,
    char       *buffer)
{
    char *cp;

    buffer[0] = '\0';

    cp = (char *)strchr(string, ':');
    if (cp) {
        for (cp++; (*cp == ' ') && (*cp != '\0'); cp++);

        strncpy(buffer, cp, buflen);
        buffer[buflen-1] = '\0';

        cp = (char *)strrchr(buffer, '$');
        if (cp) {
            for (cp--; (*cp == ' ') && (cp != buffer); cp--);

            if (cp == buffer)
                *cp = '\0';
            else
                *++cp = '\0';
        }
    }

    return(buffer);
}

/**
 *  Trim all space characters from the end of a string.
 *
 *  This function replaces all isspace() characters at the
 *  end of a string with the terminating null character.
 *
 *  @param  string - the string to trim
 *
 *  @return pointer to the modified input string
 */
char *trim_trailing_spaces(char *string)
{
    char *chrp;

    if (string) {
        chrp = &string[strlen(string)-1];
        while (isspace(*chrp) && chrp != string) *chrp-- = '\0';
    }

    return(string);
}
