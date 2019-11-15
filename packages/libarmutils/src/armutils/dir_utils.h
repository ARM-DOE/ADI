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

/** @file dir_utils.h
 *  Directory Utilities
 */

#ifndef _DIR_UTILS_H
#define _DIR_UTILS_H

#include "armutils/regex_utils.h"

/**
 *  @defgroup ARMUTILS_DIR_UTILS Directory Utils
 */
/*@{*/

#define DL_SHOW_DOT_FILES  0x1 /**< include files starting with a '.' */

/**
 *  Directory List.
 */
typedef struct {

    char       *path;        /**< path to the directory              */
    int         flags;       /**< control flags                      */

    struct stat stats;       /**< directory stats                    */

    REList     *patterns;    /**< list of file patterns to look for  */

    int         nalloced;    /**< allocated length of the file list  */
    int         nfiles;      /**< number of files in the file list   */
    char      **file_list;   /**< list of files in the directory     */

    /** function used by qsort to sort the file list */
    int  (*qsort_compare)(const void *, const void *);

} DirList;

/*****  DirList functions and flags  *****/

DirList *dirlist_create(
            const char  *path,
            int          flags);

void    dirlist_free(DirList *dirlist);

int     dirlist_add_patterns(
            DirList     *dirlist,
            int          npatterns,
            const char **patterns,
            int          ignore_case);

int     dirlist_get_file_list(
            DirList   *dirlist,
            char    ***file_list);

void    dirlist_set_qsort_compare(
            DirList     *dirlist,
            int  (*qsort_compare)(const void *, const void *));

/*****  Utility functions  *****/

int make_path(const char *path, mode_t mode);

/*@}*/

#endif /* _DIR_UTILS_H */
