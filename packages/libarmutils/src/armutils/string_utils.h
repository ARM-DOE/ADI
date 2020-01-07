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
*    $Revision: 15861 $
*    $Author: ermold $
*    $Date: 2012-11-12 01:15:27 +0000 (Mon, 12 Nov 2012) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file string_utils.h
 *  String Utilities
 */

#ifndef _STRING_UTILS_H
#define _STRING_UTILS_H

/**
 *  @defgroup ARMUTILS_STRING_UTILS String Utils
 */
/*@{*/

int parse_version_string(
        const char *string,
        int        *major,
        int        *minor,
        int        *micro);

int qsort_numeric_strcmp(const void *str1, const void *str2);
int qsort_strcmp(const void *str1, const void *str2);

char *string_copy(const char *string);

char *string_create(const char *format, ...);
char *string_create_va_list(const char *format, va_list args);

int   string_to_doubles(char *line, int buflen, double *buffer);
int   string_to_floats(char *line, int buflen, float *buffer);
int   string_to_ints(char *line, int buflen, int *buffer);
int   string_to_longs(char *line, int buflen, long *buffer);

char *trim_repository_string(const char *string, int buflen, char *buffer);

char *trim_trailing_spaces(char *string);

/*@}*/

#endif /* _STRING_UTILS_H */
