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
*    $Revision: 6689 $
*    $Author: ermold $
*    $Date: 2011-05-16 19:14:17 +0000 (Mon, 16 May 2011) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file dir_utils.c
 *  Directory Utilities
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
 *  Free all memory used by a directory list.
 *
 *  @param  dirlist - pointer to the DirList.
 */
void dirlist_free(DirList *dirlist)
{
    int fi;

    if (dirlist) {

        if (dirlist->path)     free(dirlist->path);
        if (dirlist->patterns) relist_free(dirlist->patterns);

        if (dirlist->file_list) {
            for (fi = 0; fi < dirlist->nfiles; fi++) {
                free(dirlist->file_list[fi]);
            }

            free(dirlist->file_list);
        }

        free(dirlist);
    }
}

/**
 *  Create a new directory list.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  path  - full path to the directory
 *  @param  flags - control flags
 *
 *  <b>Control Flags</b>
 *
 *    - DL_SHOW_DOT_FILES =
 *        Incude files starting with '.' in the file list. Note: the '.' and
 *        '..' directories will always be excluded from the file list.
 *
 *  @return
 *    - pointer to the new DirList
 *    - NULL if an error occurred
 */
DirList *dirlist_create(
    const char  *path,
    int          flags)
{
    DirList *dirlist;

    /* Create the new DirList */

    dirlist = (DirList *)calloc(1, sizeof(DirList));
    if (!dirlist) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not create directory list for: %s\n",
            " -> memory allocation error\n", path);

        return((DirList *)NULL);
    }

    dirlist->flags         = flags;
    dirlist->qsort_compare = qsort_strcmp;
    dirlist->path          = strdup(path);

    if (!dirlist->path) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not create directory list for: %s\n",
            " -> memory allocation error\n", path);

        dirlist_free(dirlist);
        return((DirList *)NULL);
    }

    /* Initialize the file list */

    dirlist->nalloced = 128;
    dirlist->nfiles   = 0;

    dirlist->file_list = (char **)calloc(dirlist->nalloced, sizeof(char *));
    if (!dirlist->file_list) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not create directory list for: %s\n",
            " -> memory allocation error\n", path);

        dirlist_free(dirlist);
        return((DirList *)NULL);
    }

    return(dirlist);
}

/**
 *  Add file patterns to a directory list.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  dirlist     - pointer to the DirList
 *  @param  npatterns   - number of file patterns
 *  @param  patterns    - list of extended regex file patterns (man regcomp)
 *  @param  ignore_case - ingnore case in file patterns
 *
 *  @return
 *    - 1 if succesful
 *    - 0 if an error occurred
 */
int dirlist_add_patterns(
    DirList     *dirlist,
    int          npatterns,
    const char **patterns,
    int          ignore_case)
{
    REList *new_list;
    int     cflags;

    if (!npatterns || !patterns) {
        return(1);
    }

    cflags = REG_EXTENDED | REG_NOSUB;

    if (ignore_case) {
        cflags |= REG_ICASE;
    }

    new_list = relist_compile(
        dirlist->patterns, npatterns, patterns, cflags);

    if (!new_list) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not add file patterns for directory: %s\n",
            " -> regular expression error\n",
            dirlist->path);

        return(0);
    }

    dirlist->patterns       = new_list;
    dirlist->stats.st_mtime = 0;

    return(1);
}

/**
 *  Get the list of files in a directory.
 *
 *  By default the returned list will be sorted alphanumerically using
 *  the qsort_strcmp() function.  A different file name compare function
 *  can be set using dirlist_set_qsort_compare().
 *
 *  The memory used by the returned file list belongs to the DirList
 *  structure and must *not* be freed by the calling process.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  dirlist   - pointer to the DirList
 *  @param  file_list - output: pointer to the file list
 *
 *  @return
 *    -  number of files
 *    - -1 if an error occurred
 */
int dirlist_get_file_list(DirList *dirlist, char ***file_list)
{
    struct stat     dir_stats;
    DIR            *dirp;
    struct dirent  *direntp;
    char          **new_list;
    size_t          new_size;
    int             status;
    int             fi;

    /* Initialize output */

    *file_list = (char **)NULL;

    /* Check to see if the directory exists */

    if (access(dirlist->path, F_OK) != 0) {

        if (errno == ENOENT) {
            return(0);
        }
        else {

            ERROR( ARMUTILS_LIB_NAME,
                "Could not access directory: %s\n",
                " -> %s\n", dirlist->path, strerror(errno));

            return(-1);
        }
    }

    /* Check if the directory has been modified */

    if (stat(dirlist->path, &dir_stats) != 0 ) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not stat directory: %s\n",
            " -> %s\n", dirlist->path, strerror(errno));

        return(-1);
    }

    if (dirlist->stats.st_mtime == dir_stats.st_mtime) {
        dirlist->stats = dir_stats;
        *file_list     = dirlist->file_list;
        return(dirlist->nfiles);
    }

    /* Free memory used by the file names in the previous list */

    for (fi = 0; fi < dirlist->nfiles; fi++) {
        free(dirlist->file_list[fi]);
        dirlist->file_list[fi] = (char *)NULL;
    }

    dirlist->nfiles = 0;

    /* Open the directory */

    dirp = opendir(dirlist->path);
    if (!dirp) {

        ERROR( ARMUTILS_LIB_NAME,
            "Could not open directory: %s\n",
            " -> %s\n", dirlist->path, strerror(errno));

        return(-1);
    }

    /* Loop over directory entries */

    dirlist->nfiles = 0;

    for (;;) {

        /* Get the next directory entry */

        errno = 0;
        if (!(direntp = readdir(dirp))) {

            if (errno) {

                ERROR( ARMUTILS_LIB_NAME,
                    "Could not read directory: %s\n",
                    " -> %s\n", dirlist->path, strerror(errno));

                closedir(dirp);
                return(-1);
            }

            break;
        }

        /* Skip dot files unless the DL_SHOW_DOT_FILES flag is set.
         * Always skip the . and .. directories */

        if (direntp->d_name[0] == '.') {

            if (!(dirlist->flags & DL_SHOW_DOT_FILES)) {
                continue;
            }

            if ((direntp->d_name[1] == '\0') ||
                (direntp->d_name[1] == '.' && direntp->d_name[2] == '\0')) {

                continue;
            }
        }

        /* Check if this file matches one of the specified patterns */

        if (dirlist->patterns) {

            status = relist_execute(
                dirlist->patterns, direntp->d_name, 0, NULL, NULL, NULL, NULL);

            if (status == 0) {
                continue;
            }

            if (status < 0) {

                ERROR( ARMUTILS_LIB_NAME,
                    "Could not get file list for directory: %s\n",
                    " -> regular expression error\n",
                    dirlist->path);

                closedir(dirp);
                return(-1);
            }
        }

        /* Check if we need to increase the length of the file list */

        if (dirlist->nfiles == dirlist->nalloced - 1) {

            new_size = dirlist->nalloced * 2;
            new_list = (char **)realloc(
                dirlist->file_list, new_size * sizeof(char *));

            if (!new_list) {

                ERROR( ARMUTILS_LIB_NAME,
                    "Could not get file list for directory: %s\n",
                    " -> memory allocation error\n",
                    dirlist->path);

                closedir(dirp);
                return(-1);
            }

            dirlist->nalloced  = new_size;
            dirlist->file_list = new_list;

            for (fi = dirlist->nfiles; fi < dirlist->nalloced; fi++) {
                dirlist->file_list[fi] = (char *)NULL;
            }
        }

        /* Add this file to the list */

        dirlist->file_list[dirlist->nfiles] = strdup(direntp->d_name);

        if (!dirlist->file_list[dirlist->nfiles]) {

            ERROR( ARMUTILS_LIB_NAME,
                "Could not get file list for directory: %s\n",
                " -> memory allocation error\n",
                dirlist->path);

            closedir(dirp);
            return(-1);
        }

        dirlist->nfiles++;
    }

    closedir(dirp);

    dirlist->stats  = dir_stats;
    *file_list      = dirlist->file_list;

    /* Check if any files were found */

    if (dirlist->nfiles) {

        dirlist->file_list[dirlist->nfiles] = (char *)NULL;

        /* Sort the file list */

        if (dirlist->qsort_compare) {

            qsort(dirlist->file_list, dirlist->nfiles, sizeof(char *),
                dirlist->qsort_compare);
        }
    }

    return(dirlist->nfiles);
}

/**
 *  Set the file name compare function.
 *
 *  The file name compare function will be passed to qsort to sort the
 *  file list.  By default the qsort_strcmp() function will be used. The
 *  qsort_numeric_strcmp() function is also provided in the "String Utils"
 *  module.
 *
 *  @param  dirlist       - pointer to the DirList
 *  @param  qsort_compare - file name compare function
 */
void dirlist_set_qsort_compare(
    DirList     *dirlist,
    int  (*qsort_compare)(const void *, const void *))
{
    dirlist->qsort_compare  = qsort_compare;
    dirlist->stats.st_mtime = 0;
}

/**
 *  Make the full path to a directory.
 *
 *  This function will create the specified path if it does not already exist.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  path   - directory path to create
 *  @param  mode   - mode of the new directory
 *
 *  @return
 *    - 1 if the path exists or was created
 *    - 0 if an error occurred
 */
int make_path(
    const char *path,
    mode_t      mode)
{
    char errstr[MAX_LOG_ERROR];
    int  status;

    status = msngr_make_path(path, mode, MAX_LOG_ERROR, errstr);

    if (!status) {
        ERROR( ARMUTILS_LIB_NAME, errstr);
    }

    return(status);
}
