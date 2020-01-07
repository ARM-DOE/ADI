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
*    $Revision: 60943 $
*    $Author: ermold $
*    $Date: 2015-04-01 19:47:36 +0000 (Wed, 01 Apr 2015) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file regex_utils.c
 *  Regular Expression Utilities
 */

#include "armutils.h"

/*******************************************************************************
 *  Private Functions
 */
/** @privatesection */

/**
 *  PRIVATE: Free the results from the last regular expression match.
 *
 *  @param  re_list - pointer to the regular expressions list
 */
static void __relist_free_results(REList *re_list)
{
    if (re_list->string)  free(re_list->string);
    if (re_list->offsets) free(re_list->offsets);

    if (re_list->substrs)
        re_free_substrings((re_list->nsubs + 1), re_list->substrs);

    re_list->string  = (char *)NULL;
    re_list->eflags  =  0;
    re_list->mindex  = -1;
    re_list->nsubs   =  0;
    re_list->offsets = (regmatch_t *)NULL;
    re_list->substrs = (char **)NULL;
}

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Wrapper function for regcomp().
 *
 *  See the regcomp man page for more detailed argument descriptions.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  preg    - pointer to the regex structure to use
 *  @param  pattern - pattern string to compile
 *  @param  cflags  - compile flags
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int re_compile(regex_t *preg, const char *pattern, int cflags)
{
    int   errcode;
    char *errstr;

    errcode = regcomp(preg, pattern, cflags);
    if (errcode) {

        errstr = re_error(errcode, preg);

        if (errstr) {

            ERROR( ARMUTILS_LIB_NAME,
                "Could not compile regular expression: '%s'\n"
                " -> %s\n", pattern, errstr);

            free(errstr);
            return(0);
        }
    }

    return(1);
}

/**
 *  Wrapper function for regerror().
 *
 *  The memory used by the returned string is dynamically allocated.
 *  It is the responsibility of the calling process to free this memory.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  errcode - error code returned by re_comp() or re_exec()
 *  @param  preg    - pointer to the regular expression that failed
 *
 *  @return
 *    - regex error message
 *    - NULL if an error occurred
 */
char *re_error(int errcode, regex_t *preg)
{
    size_t  errlen;
    char   *errstr;

    errlen = regerror(errcode, preg, NULL, 0);
    if (errlen == 0) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not create regular expression error message\n"
            " -> regerror not implemented\n");

        return((char *)NULL);
    }

    errlen++;

    errstr = (char *)malloc(errlen * sizeof(char));
    if (!errstr) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not create regular expression error message\n"
            " -> memory allocation error\n");

        return((char *)NULL);
    }

    regerror(errcode, preg, errstr, errlen);

    return(errstr);
}

/**
 *  Wrapper function for regexec().
 *
 *  See the regexec man page for more detailed argument descriptions.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  preg   - pointer to the compiled regular expression
 *  @param  string - string to compare with the regular expression
 *  @param  nmatch - number of substrings to match
 *  @param  pmatch - arrary to store the matching substring offsets
 *  @param  eflags - execute flags
 *
 *  @return
 *    -  1 if match
 *    -  0 if no match
 *    - -1 if an error occurred
 */
int re_execute(
    regex_t    *preg,
    const char *string,
    size_t      nmatch,
    regmatch_t  pmatch[],
    int         eflags)
{
    int   errcode;
    char *errstr;

    errcode = regexec(preg, string, nmatch, pmatch, eflags);

    if (errcode == 0) {
        return(1);
    }
    else if (errcode == REG_NOMATCH) {
        return(0);
    }

    errstr = re_error(errcode, preg);

    if (errstr) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not execute regular expression for string: '%s'\n"
            " -> %s\n", string, errstr);

        free(errstr);
    }

    return(-1);
}

/**
 *  Wrapper function for regfree().
 *
 *  @param  preg - pointer to the compiled regular expression
 */
void re_free(regex_t *preg)
{
    if (preg) regfree(preg);
}

/**
 *  Free the substring list returned by re_substrings().
 *
 *  @param  nmatch     - number of substrings in match request
 *  @param  substrings - array of substrings returned by re_substrings()
 */
void re_free_substrings(size_t nmatch, char **substrings)
{
    size_t si;

    if (substrings) {
        for (si = 0; si < nmatch; si++) {
            if (substrings[si]) free(substrings[si]);
        }
        free(substrings);
    }
}

/**
 *  Extract the substrings from a regular expression match.
 *
 *  The array of substrings returned by this function must me freed
 *  with re_free_substrings().
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  string - string that matched the regular expression
 *  @param  nmatch - number of substrings in match request
 *  @param  pmatch - arrary that the matching substring offsets were stored in
 *
 *  @return
 *    - pointer to array of substrings
 *    - NULL if an error occurred
 */
char **re_substrings(const char *string, size_t nmatch, regmatch_t *pmatch)
{
    char **substrings;
    size_t mi;
    int    length;

    substrings = (char **)calloc(nmatch, sizeof(char *));
    if (!substrings) {
        goto MEMORY_ERROR;
    }

    for (mi = 0; mi < nmatch; mi++) {

        if (pmatch[mi].rm_so == -1) {
            substrings[mi] = (char *)NULL;
        }
        else {

            length = pmatch[mi].rm_eo - pmatch[mi].rm_so;

            substrings[mi] = (char *)malloc((length + 1) * sizeof(char));
            if (!substrings[mi]) {
                goto MEMORY_ERROR;
            }

            substrings[mi][length] = '\0';

            if (length) {
                strncpy(substrings[mi], (string + pmatch[mi].rm_so), length);
            }
        }
    }

    return(substrings);

MEMORY_ERROR:

    if (substrings) {
        for (mi--; mi > 0; mi--) {
            if (substrings[mi]) free(substrings[mi]);
        }
        if (substrings[0]) free(substrings[0]);
        free(substrings);
    }

    ERROR( ARMUTILS_LIB_NAME,
        "Could not extract subtrings from regular expression match for: '%s'\n"
        " -> memory allocation error\n", string);

    return((char **)NULL);
}

/**
 *  Compile a list of regular expression patterns.
 *
 *  See the regcomp man page for the descriptions of the pattern
 *  strings and compile flags.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  re_list   - pointer to the regular expressions list to add the
 *                      patterns to, or NULL to create a new list
 *  @param  npatterns - number of patterns to compile
 *  @param  patterns  - list of patterns to compile
 *  @param  cflags    - compile flags
 *
 *  @return
 *    - pointer to the regular expressions list
 *    - NULL if an error occurred
 */
REList *relist_compile(
    REList      *re_list,
    int          npatterns,
    const char **patterns,
    int          cflags)
{
    REList   *new_re_list = (REList *)NULL;
    int       new_nregs;
    char    **new_patterns;
    int      *new_cflags;
    regex_t **new_regs;
    regex_t  *preg;
    int       pi;

    /* Create a new REList if one was not specified */

    if (!re_list) {
        new_re_list = (REList *)calloc(1, sizeof(REList));
        if (!new_re_list) {
            goto MEMORY_ERROR;
        }
        re_list = new_re_list;
        re_list->mindex = -1;
    }

    /* Allocate space for the new patterns list */

    new_nregs = re_list->nregs + npatterns;

    new_patterns = (char **)realloc(
        re_list->patterns, new_nregs * sizeof(char *));

    if (!new_patterns) {
        goto MEMORY_ERROR;
    }

    re_list->patterns = new_patterns;

    /* Allocate space for the new cflags list */

    new_cflags = (int *)realloc(
        re_list->cflags, new_nregs * sizeof(int));

    if (!new_cflags) {
        goto MEMORY_ERROR;
    }

    re_list->cflags = new_cflags;

    /* Allocate space for the new regs list */

    new_regs = (regex_t **)realloc(
        re_list->regs, new_nregs * sizeof(regex_t *));

    if (!new_regs) {
        goto MEMORY_ERROR;
    }

    re_list->regs = new_regs;

    /* Compile the new regular expressions */

    for (pi = 0; pi < npatterns; pi++) {

        preg = (regex_t *)calloc(1, sizeof(regex_t));
        if (!preg) {
            goto MEMORY_ERROR;
        }

        if (!re_compile(preg, patterns[pi], cflags)) {
            free(preg);
            goto REGCOMP_ERROR;
        }

        re_list->patterns[re_list->nregs] = strdup(patterns[pi]);
        if (!re_list->patterns[re_list->nregs]) {
            regfree(preg);
            free(preg);
            goto MEMORY_ERROR;
        }

        re_list->cflags[re_list->nregs] = cflags;
        re_list->regs[re_list->nregs]   = preg;
        re_list->nregs++;
    }

    return(re_list);

MEMORY_ERROR:

    ERROR( ARMUTILS_LIB_NAME,
        "Could not compile list of regular expression patterns\n"
        " -> memory allocation error\n");

REGCOMP_ERROR:

    if (new_re_list) relist_free(new_re_list);

    return((REList *)NULL);
}

/**
 *  Compare a string with a list of regular expressions.
 *
 *  For the outputs that are not NULL, this function will return the results
 *  for the first regular expression that matches the specified string. The
 *  returned pointers to the pmatch and substrings arrays will be valid until
 *  the next call to relist_execute() or relist_free().
 *
 *  The pmatch and substrings arrays will have an entry for every parenthesised
 *  subexpression for the pattern that was matched (starting at index 1).
 *  Entries at index 0 correspond to the entire regular expression. For
 *  subexpressions that were not matched, the offsets in the pmatch entry will
 *  be -1 and the substrings entry will be NULL.
 *
 *  See the regexec man page for more detailed descriptions of the execute
 *  flags and output pmatch array.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  re_list    - pointer to the regular expressions list
 *  @param  string     - string to compare with the regular expression
 *  @param  eflags     - execute flags
 *  @param  mindex     - output: index of the pattern that was matched
 *  @param  nsubs      - output: number of parenthesised subexpressions
 *  @param  pmatch     - output: pointer to array of substring offsets
 *  @param  substrings - output: pointer to array of substrings
 *
 *  @return
 *    -  1 if match
 *    -  0 if no match
 *    - -1 if an error occurred
 */
int relist_execute(
    REList       *re_list,
    const char   *string,
    int           eflags,
    int          *mindex,
    size_t       *nsubs,
    regmatch_t  **pmatch,
    char       ***substrings)
{
    size_t      max_nsubs;
    size_t      max_nmatch;
    regex_t    *preg;
    regmatch_t *offsets;
    int         status;
    int         ri;

    /* Initialize outputs */

    if (mindex)     *mindex     = -1;
    if (nsubs)      *nsubs      =  0;
    if (pmatch)     *pmatch     =  (regmatch_t *)NULL;
    if (substrings) *substrings =  (char **)NULL;

    /* Free results from previous match */

    __relist_free_results(re_list);

    /* Set string and eflags */

    re_list->eflags = eflags;
    re_list->string = strdup(string);
    if (!re_list->string) {
        goto MEMORY_ERROR;
    }

    /* Determine the maximum number of parenthesised subexpressions */

    max_nsubs = 0;

    for (ri = 0; ri < re_list->nregs; ri++) {

        if (re_list->cflags[ri] & REG_NOSUB) continue;

        if (max_nsubs < re_list->regs[ri]->re_nsub) {
            max_nsubs = re_list->regs[ri]->re_nsub;
        }
    }

    /* Create array to store substring offsets */

    max_nmatch = max_nsubs + 1;

    offsets = (regmatch_t *)malloc(max_nmatch * sizeof(regmatch_t));
    if (!offsets) {
        goto MEMORY_ERROR;
    }

    /* Find the first matching regular expression */

    for (ri = 0; ri < re_list->nregs; ri++) {

        preg   = re_list->regs[ri];
        status = re_execute(preg, string, max_nmatch, offsets, eflags);

        if (status >  0) break;
        if (status == 0) continue;
        if (status <  0) {
            free(offsets);
            return(-1);
        }
    }

    if (ri == re_list->nregs) {
        free(offsets);
        return(0);
    }

    /* Set the results in the REList structure */

    re_list->mindex  = ri;
    re_list->offsets = offsets;

    if (re_list->cflags[ri] & REG_NOSUB) {
        re_list->nsubs = 0;
    }
    else {
        re_list->nsubs = re_list->regs[ri]->re_nsub;
    }

    if (substrings) {

        re_list->substrs = re_substrings(
            string, (re_list->nsubs + 1), re_list->offsets);

        if (!re_list->substrs) {
            return(-1);
        }
    }

    /* Set outputs */

    if (mindex)     *mindex     = re_list->mindex;
    if (nsubs)      *nsubs      = re_list->nsubs;
    if (pmatch)     *pmatch     = re_list->offsets;
    if (substrings) *substrings = re_list->substrs;

    return(1);

MEMORY_ERROR:

    ERROR( ARMUTILS_LIB_NAME,
        "Could not compare string to regular expressions list: '%s'\n"
        " -> memory allocation error\n", string);

    return(-1);
}

/**
 *  Free a regular expressions list.
 *
 *  @param  re_list - pointer to the regular expressions list
 */
void relist_free(REList *re_list)
{
    int ri;

    if (re_list) {

        __relist_free_results(re_list);

        if (re_list->patterns) {
            for (ri = 0; ri < re_list->nregs; ri++) {
                free(re_list->patterns[ri]);
            }
            free(re_list->patterns);
        }

        if (re_list->regs) {
            for (ri = 0; ri < re_list->nregs; ri++) {
                regfree(re_list->regs[ri]);
                free(re_list->regs[ri]);
            }
            free(re_list->regs);
        }

        free(re_list->cflags);
        free(re_list);
    }
}
