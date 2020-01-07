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

/** @file regex_utils.h
 *  Regular Expression Utilities
 */

#ifndef _REGEX_UTILS_H
#define _REGEX_UTILS_H

/**
 *  @defgroup ARMUTILS_REGEX_UTILS Regex Utils
 */
/*@{*/

/**
 *  Regular Expressions List.
 */
typedef struct {

    int          nregs;    /**< number of regular expressions in the list */
    char       **patterns; /**< list of regular expression patterns       */
    int         *cflags;   /**< list of compile flags used                */
    regex_t    **regs;     /**< list of compiled regular expressions      */

    char        *string;   /**< string compared to regular expressions    */
    int          eflags;   /**< execute flags used                        */
    int          mindex;   /**< index of the expression that was matched  */
    size_t       nsubs;    /**< number of parenthesised subexpressions    */
    regmatch_t  *offsets;  /**< arrary of substring offsets               */
    char       **substrs;  /**< array of substrings                       */

} REList;

/*****  Wrappers to regex funtions  *****/

int     re_compile(
            regex_t    *preg,
            const char *pattern,
            int         cflags);

char   *re_error(
            int      errcode,
            regex_t *preg);

int     re_execute(
            regex_t    *preg,
            const char *string,
            size_t      nmatch,
            regmatch_t  pmatch[],
            int         eflags);

void    re_free(
            regex_t *preg);

/*****  Extract substrings from regex match  *****/

void    re_free_substrings(
            size_t nmatch,
            char **substrings);

char  **re_substrings(
            const char *string,
            size_t      nmatch,
            regmatch_t *pmatch);

/*****  REList functions  *****/

REList *relist_compile(
            REList      *re_list,
            int          npatterns,
            const char **patterns,
            int          cflags);

int     relist_execute(
            REList       *re_list,
            const char   *string,
            int           eflags,
            int          *mindex,
            size_t       *nsubs,
            regmatch_t  **pmatch,
            char       ***substrings);

void    relist_free(
            REList *re_list);

/*@}*/

#endif /* _REGEX_UTILS_H */
