/*******************************************************************************
*
*  Copyright © 2014, Battelle Memorial Institute
*  All rights reserved.
*
********************************************************************************
*
*  Author:
*     name:  Brian Ermold
*     phone: (509) 375-2277
*     email: brian.ermold@pnl.gov
*
*******************************************************************************/

/** @file dsproc_datastream_files.c
 *  Datastream Files Functions.
 */

#include "dsproc3.h"
#include "dsproc_private.h"

extern DSProc *_DSProc; /**< Internal DSProc structure */

/** @privatesection */

/*******************************************************************************
 *  Static Functions Visible Only To This Module
 */

/** List of input files specified on the command line.   */
static char **_InputFiles = (char **)NULL;

/** Parsed command line string containing the input
 *  files specified on the command line.                 */
static char  *_InputFilesString = (char *)NULL;

/** Number of input files specified on the command line. */
static int    _NumInputFiles    = 0;

/** Structure used to pass data to the _dsproc_file_name_compare via qsort. */
typedef struct QsortData {
    DSDir *dir;
    int    err_count;
} QsortData;

/** Global qsort data used when sorting raw files when a user specifies
 *  a file name time pattern or function. */
static QsortData _QsortData;

/**
 *  Qsort compare function used to sort files in chronological order.
 *
 *  @param  str1  void pointer to pointer to file name 1
 *  @param  str2  void pointer to pointer to file name 2
 *
 *  @retval  1  str1 >  str2
 *  @retval  0  str1 == str2
 *  @retval -1  str1 <  str2
 */
static int _dsproc_file_name_compare(
    const void *str1,
    const void *str2)
{
    const char *s1  = *(const char **)str1;
    const char *s2  = *(const char **)str2;
    time_t      t1  = _dsproc_get_file_name_time(_QsortData.dir, s1);
    time_t      t2  = _dsproc_get_file_name_time(_QsortData.dir, s2);

    if (t1 <= 0 || t2 <= 0) {

        /* sort invalid file names at the end of the list */

        if (t1 > 0) return(-1);
        if (t2 > 0) return(1);

        return(strcmp(s1, s2));
    }

    if (t1 < t2) { return(-1); }
    if (t1 > t2) { return(1);  }

    return(strcmp(s1, s2));
}

/**
 *  Static: Close all datastream files.
 *
 *  @param  file - pointer to the DSFile structure.
 */
static void _dsproc_close_dsfile(DSFile *file)
{
    DSDir *dir;

    if (!file) return;

    dir = file->dir;

    if (file->ncid) {
        ncds_close(file->ncid);
        file->ncid  = 0;
        dir->nopen -= 1;
    }

    file->touched = 0;
}

/**
 *  Static: Free all memory used by a datastream file structure.
 *
 *  @param  dsfile - pointer to the DSFile structure.
 */
static void _dsproc_free_dsfile(DSFile *dsfile)
{
    if (dsfile) {

        if (dsfile->ncid) {
            _dsproc_close_dsfile(dsfile);
        }

        if (dsfile->name)      free(dsfile->name);
        if (dsfile->full_path) free(dsfile->full_path);
        if (dsfile->timevals)  free(dsfile->timevals);
        if (dsfile->dod)       cds_delete_group(dsfile->dod);

        free(dsfile);
    }
}

/**
 *  Static: Create a new datastream file structure.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  dir   - pointer to the parent DSDir structure
 *  @param  name  - name of the file
 *
 *  @return
 *    - pointer to the new DSFile structure
 *    - NULL if an error occurred
 */
static DSFile *_dsproc_create_dsfile(DSDir *dir, const char *name)
{
    DSFile *file;
    size_t  length;

    file = (DSFile *)calloc(1, sizeof(DSFile));
    if (!file) {
        goto MEMORY_ERROR;
    }

    file->name = strdup(name);
    if (!file->name) {
        goto MEMORY_ERROR;
    }

    length = strlen(dir->path) + strlen(name) + 2;

    file->full_path = malloc(length * sizeof(char));
    if (!file->full_path) {
        goto MEMORY_ERROR;
    }

    sprintf(file->full_path, "%s/%s", dir->path, name);

    file->dir = dir;

    return(file);

MEMORY_ERROR:

    if (file) _dsproc_free_dsfile(file);

    ERROR( DSPROC_LIB_NAME,
        "Could not create DSFile structure for: %s\n"
        " -> memory allocation error\n", name);

    dsproc_set_status(DSPROC_ENOMEM);
    return((DSFile *)NULL);
}

/**
 *  Static: Get or create a cached datastream file.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  dir  - pointer to the DSDir structure.
 *  @param  name - name of the file
 *
 *  @return
 *    - pointer to the DSFile
 *    - NULL if an error occurred
 */
static DSFile *_dsproc_get_dsfile(DSDir *dir, const char *name)
{
    size_t       new_size;
    DSFile     **new_list;
    DSFile      *file;
    int          fi;

    /* Check if this file is already in the cache */

    for (fi = 0; fi < dir->ndsfiles; fi++) {
        file = dir->dsfiles[fi];
        if (strcmp(file->name, name) == 0) {
            return(file);
        }
    }

    /* Check if we need to increase the length of the dsfiles list */

    if (dir->ndsfiles == dir->max_dsfiles - 1) {

        new_size = dir->max_dsfiles * 2;
        new_list = (DSFile **)realloc(
            dir->dsfiles, new_size * sizeof(DSFile *));

        if (!new_list) {

            ERROR( DSPROC_LIB_NAME,
                "Could not get DSFile structure for: %s/%s\n"
                " -> memory allocation error\n", dir->path, name);

            dsproc_set_status(DSPROC_ENOMEM);
            return((DSFile *)NULL);
        }

        dir->max_dsfiles = new_size;
        dir->dsfiles     = new_list;

        for (fi = dir->ndsfiles; fi < dir->max_dsfiles; fi++) {
            dir->dsfiles[fi] = (DSFile *)NULL;
        }
    }

    file = _dsproc_create_dsfile(dir, name);
    if (!file) {
        return((DSFile *)NULL);
    }

    dir->dsfiles[dir->ndsfiles] = file;
    dir->ndsfiles++;

    return(file);
}

/**
 *  Static: Find the index of a file in a file list.
 *
 *  This function requires that the file list is sorted in chronological order.
 *
 *  @param  time      - the time to search for
 *  @param  mode      - search mode:
 *                       - 0: find the index of the last file in the
 *                            array with a timestamp < the search time
 *                       - 1: find the index of the first file in the
 *                            array with a timestamp > the search time
 *  @param  nfiles    - number of files in the file list;
 *  @param  files     - list of files
 *  @param  file_time - function used to parse the time from a file name
 *
 *  @retval index  of the file in the array
 *  @retval -1     if not found.
 */
static int _dsproc_find_file_index(
    time_t   time,
    int      mode,
    size_t   nfiles,
    char   **files,
    time_t (*file_time)(const char *))
{
    int    bi, ei, mi;
    time_t t0, tn;

    if (nfiles == 0) return(-1);

    /* Handle edges */

    bi = 0;
    t0 = file_time(files[bi]);

    ei = nfiles - 1;
    tn = file_time(files[ei]);

    if (mode == 0) {
        if (time <= t0) return(-1);
        if (time >  tn) return(ei);
        if (time == tn) {
            for (--ei; time == file_time(files[ei]); --ei);
            return(ei);
        }
    }
    else {
        if (time >= tn) return(-1);
        if (time <  t0) return(bi);
        if (time == t0) {
            for (++bi; time == file_time(files[bi]); ++bi);
            return(bi);
        }
    }

    /* Find bi and ei such that:
     *
     * file_time(files[bi]) <= time < file_time(files[ei]) */

    while (ei > bi + 1) {

        mi = (bi + ei)/2;

        if (time < file_time(files[mi])) {
            ei = mi;
        }
        else {
            bi = mi;
        }
    }

    /* Return the requested index */

    if (mode == 0) {
        while(time == file_time(files[bi])) --bi;
        return(bi);
    }
    else {
        return(ei);
    }
}

/**
 *  Static: Get the timestamp from an ARM datastream file name.
 *
 *  This function assumes the file name format is:
 *
 *  name.level.YYYYMMDD.hhmmss. ...
 *
 *  @param  file_name - the file name
 *
 *  @retval time  timestamp from the file name
 *  @retval 0     invalid file name format
 */
static time_t _dsproc_get_ARM_file_name_time(const char *file_name)
{
    struct tm  gmt;
    char      *timestamp;
    int        nscanned;
    time_t     file_time;

    if (!(timestamp = strchr(file_name,   '.'))) return(0);
    if (!(timestamp = strchr(++timestamp, '.'))) return(0);

    timestamp++;

    memset(&gmt, 0, sizeof(struct tm));

    nscanned = sscanf(timestamp,
        "%4d%2d%2d.%2d%2d%2d",
        &gmt.tm_year, &gmt.tm_mon, &gmt.tm_mday,
        &gmt.tm_hour, &gmt.tm_min, &gmt.tm_sec);

    if (nscanned != 6) return(0);

    gmt.tm_year -= 1900;
    gmt.tm_mon--;

    file_time = mktime(&gmt);
    if (file_time == (time_t)-1) {
        return(0);
    }

    file_time -= timezone;

    return(file_time);
}

/**
 *  Static: Refresh information cached in the DSFile structure.
 *
 *  If this function returns successfully the following members of the
 *  DSFile structure will be populated with the most current values:
 *
 *    int        time_dimid
 *    int        time_varid
 *    time_t     base_time
 *    size_t     ntimes
 *    timeval_t *timevals
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  dsfile  - pointer to the DSFile structure.
 *
 *  @return
 *    -  1 if successful
 *    -  0 if an error occurred
 */
static int _dsproc_refresh_dsfile_info(DSFile *dsfile)
{
    struct stat file_stats;
    int         status;
    int         sync;

    /* Get file stats */

    if (stat(dsfile->full_path, &file_stats) != 0 ) {

        ERROR( DSPROC_LIB_NAME,
            "Could not stat data file: %s\n"
            " -> %s\n", dsfile->full_path, strerror(errno));

        dsproc_set_status(DSPROC_EFILESTATS);
        return(0);
    }

    /* Check if the file has been updated */

    if (dsfile->stats.st_mtime        != file_stats.st_mtime       ||
#ifdef __APPLE__
        dsfile->stats.st_mtimespec.tv_sec  != file_stats.st_mtimespec.tv_sec ||
        dsfile->stats.st_mtimespec.tv_nsec != file_stats.st_mtimespec.tv_nsec) {
#else
        dsfile->stats.st_mtim.tv_sec  != file_stats.st_mtim.tv_sec ||
        dsfile->stats.st_mtim.tv_nsec != file_stats.st_mtim.tv_nsec) {
#endif

        sync = (dsfile->ncid && !(dsfile->mode & NC_WRITE)) ? 1 : 0;

        if (!_dsproc_open_dsfile(dsfile, 0)) {
            return(0);
        }

        if (sync) {

            if (!ncds_sync(dsfile->ncid)) {

                ERROR( DSPROC_LIB_NAME,
                    "Could not sync with netcdf file: %s\n",
                    dsfile->full_path);

                dsproc_set_status(DSPROC_ENCSYNC);
                return(0);
            }
        }

        /* Read in the time information */

        status = ncds_get_time_info(
            dsfile->ncid,
            &dsfile->time_dimid,
            &dsfile->time_varid,
            NULL,
            &dsfile->base_time,
            NULL, NULL);

        if (status < 0) {

            ERROR( DSPROC_LIB_NAME,
                "Could not read times from data file: %s\n",
                dsfile->full_path);

            dsproc_set_status(DSPROC_ENCREAD);
            return(0);
        }

        /* Read in the time values */

        if (dsfile->timevals) {
            free(dsfile->timevals);
            dsfile->timevals = (timeval_t *)NULL;
        }

        dsfile->ntimes = ncds_get_timevals(
            dsfile->ncid, 0, 0, &(dsfile->timevals));

        if (dsfile->ntimes < 0) {

            ERROR( DSPROC_LIB_NAME,
                "Could not read times from data file: %s\n",
                dsfile->full_path);

            dsproc_set_status(DSPROC_ENCREAD);
            return(0);
        }
    }

    dsfile->stats = file_stats;

    return(1);
}

/**
 *  Static: Get version number of a file with an optional .v# extension.
 *
 *  @param  name - name of the file
 *  @param  extp - output: pointer to the '.' in the file extension,
 *                 or NULL if the file does not have a .v# extension.
 *
 *  @return
 *    - version number from .v# file extension
 *    - -1 if the file does not have a .v# extension
 */
static int _dsproc_get_file_version(char *name, char **extp)
{
    char *chrp;
    char *endp;
    int   version;

    version = -1;
    *extp = (char *)NULL;

    chrp = strrchr(name, '.');
    if (chrp && chrp[1] == 'v' && strlen(chrp) > 2) {
        *extp = chrp;
        chrp += 2;

        version = (int)strtol(chrp, &endp, 10);
        if (endp == chrp || *endp != '\0') {
            version = -1;
            *extp = (char *)NULL;
        }
    }

    return(version);
}

/**
 *  Static: Filter a list of files containing optional .v# extensions.
 *
 *  This function will filter out all lower version files from a list of files
 *  containing optional .v# extensions.  Files without a .v# extension will be
 *  assumed to be the highest version of the file. 
 * 
 *  @param  nfiles    - number of files in the list
 *  @param  file_list - input:  list of files containing optional .v# extensions.
 *                    - output: filtered list
 *
 *  @return
 *    - number of files in the filtered list
 */
static int _dsproc_filter_versioned_files(int nfiles, char **file_list)
{
    int   i1, i2;
    int   v1, v2;
    char *ext1, *ext2;

    if (nfiles <= 0) return(0);

    i1 = 0;
    v1 = _dsproc_get_file_version(file_list[i1], &ext1);
    if (ext1) *ext1 = '\0';

    for (i2 = 1; i2 < nfiles; ++i2) {

        v2 = _dsproc_get_file_version(file_list[i2], &ext2);
        if (ext2) *ext2 = '\0';

        if (strcmp(file_list[i1], file_list[i2]) == 0) {

            /* names match, check version numbers */
            if (v1 == -1 || (v1 > v2 && v2 != -1)) {
                /* v1 is highest version, skip i2 */
                ext2 = (char *)NULL;
                free(file_list[i2]);
                file_list[i2] = (char *)NULL;
                continue;
            }
            else {
                /* v2 is highest version, replace i1 with i2 */
                ext1 = (char *)NULL;
                free(file_list[i1]);
                file_list[i1] = file_list[i2];
                file_list[i2] = (char *)NULL;
            }
        }
        else {
            /* names do not match, add i2 to the list */
            i1 += 1;
            if (i1 != i2) {
                file_list[i1] = file_list[i2];
                file_list[i2] = (char *)NULL;
            }
            if (ext1) *ext1 = '.';
        }

        ext1 = ext2;
        v1 = v2;
    }

    if (ext1) *ext1 = '.';

    return(i1+1);
}

/*******************************************************************************
 *  Private Functions Visible Only To This Library
 */

/**
 *  Private: Add new datastream directory file patterns.
 *
 *  This function adds file patterns to look for when creating the list of
 *  files in the datastream directory. By default all files in the directory
 *  will be listed.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  dir         - pointer to the DSDir structure.
 *  @param  npatterns   - number of file patterns
 *  @param  patterns    - list of extended regex file patterns (man regcomp)
 *  @param  ignore_case - ingnore case in file patterns
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int _dsproc_add_dsdir_patterns(
    DSDir       *dir,
    int          npatterns,
    const char **patterns,
    int          ignore_case)
{
    int     cflags = REG_EXTENDED | REG_NOSUB;
    REList *new_list;

    if (ignore_case) cflags |= REG_ICASE;

    new_list = relist_compile(dir->patterns, npatterns, patterns, cflags);
    if (!new_list) {

        ERROR( DSPROC_LIB_NAME,
            "Could not add file pattern(s) for directory: %s\n"
            " -> regular expression error\n",
            dir->path);

        dsproc_set_status(DSPROC_EREGEX);
        return(0);
    }

    dir->patterns       = new_list;
    dir->stats.st_mtime = 0;

    return(1);
}

/**
 *  Private: Create a dynamically allocated copy of a file list.
 *
 *  The memory used by the returned file list is dynamically allocated and
 *  and must be freed using the dsproc_free_file_list() function.
 *
 *  @param  nfiles    - number of files
 *  @param  file_list - list of file names
 *
 *  @return
 *    - dynamically allocated and NULL terminated copy of the file list
 *    - NULL if a memory allocation error occurred
 */
char **_dsproc_clone_file_list(int nfiles, char **file_list)
{
    char **clone;
    int    fi;

    clone = (char **)calloc(nfiles + 1, sizeof(char *));
    if (!clone) {
        return((char **)NULL);
    }

    for (fi = 0; fi < nfiles; ++fi) {
        if (!(clone[fi] = strdup(file_list[fi]))) {
            dsproc_free_file_list(clone);
            return((char **)NULL);
        }
    }

    return(clone);
}

/**
 *  Private: Create a new datastream directory structure.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  path  - full path to the directory
 *
 *  @return
 *    - pointer to the new DSDir structure
 *    - NULL if an error occurred
 */
DSDir *_dsproc_create_dsdir(const char *path)
{
    DSDir *dir;

    /* Create the new DSDir */

    dir = (DSDir *)calloc(1, sizeof(DSDir));
    if (!dir) {
        goto MEMORY_ERROR;
    }

    dir->path = strdup(path);
    if (!dir->path) {
        goto MEMORY_ERROR;
    }

    /* Initialize the files list */

    dir->max_files = 128;
    dir->nfiles    = 0;

    dir->files = (char **)calloc(dir->max_files, sizeof(char *));
    if (!dir->files) {
        goto MEMORY_ERROR;
    }

    dir->nopen    = 0;
    dir->max_open = 64;

    /* Initialize the dsfiles list */

    dir->max_dsfiles = 128;
    dir->ndsfiles    = 0;

    dir->dsfiles = (DSFile **)calloc(dir->max_dsfiles, sizeof(DSFile *));
    if (!dir->dsfiles) {
        goto MEMORY_ERROR;
    }

    return(dir);

MEMORY_ERROR:

    if (dir) _dsproc_free_dsdir(dir);

    ERROR( DSPROC_LIB_NAME,
        "Could not create DSDir structure for: %s\n"
        " -> memory allocation error\n", path);

    dsproc_set_status(DSPROC_ENOMEM);
    return((DSDir *)NULL);
}

/**
 *  Private: Free all memory used by a datastream directory structure.
 *
 *  @param  dir - pointer to the DSDir structure.
 */
void _dsproc_free_dsdir(DSDir *dir)
{
    int fi;

    if (dir) {

        if (dir->dsfiles) {

            for (fi = 0; fi < dir->ndsfiles; fi++) {
                if (dir->dsfiles[fi]) _dsproc_free_dsfile(dir->dsfiles[fi]);
            }

            free(dir->dsfiles);
        }

        if (dir->files) {

            for (fi = 0; fi < dir->nfiles; fi++) {
                if (dir->files[fi]) free(dir->files[fi]);
            }

            free(dir->files);
        }

        if (dir->path)     free(dir->path);
        if (dir->patterns) relist_free(dir->patterns);

        if (dir->file_name_time_patterns) {
            retime_list_free(dir->file_name_time_patterns);
        }

        free(dir);
    }
}

/**
 *  Private: Find files in a datastream directory for a specified time range.
 *
 *  The returned file list will bracket the file with a timestamp just prior
 *  to the begin time, and the file with a timestamp just after the end time.
 *  That is it will return an extra file on either side of the search range.
 *  This is done to prevent newly created files containing only header
 *  information from hiding existing data in forward and/or backward searches.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  dir        - pointer to the DSDir
 *  @param  begin_time - beginning of the time range to search
 *  @param  end_time   - end of the time range to search
 *  @param  file_list  - output: pointer to the first file in the internal
 *                       file list maintained by the DSDir structure.
 *
 *  @retval nfiles  the number of files found
 *  @retval -1      if an error occurred
 */
int _dsproc_find_dsdir_files(
    DSDir    *dir,
    time_t    begin_time,
    time_t    end_time,
    char   ***file_list)
{
    int      nfiles;
    char   **files;
    int      bi, ei;

    /* Initialize variables */

    *file_list = ((char **)NULL);

    if (!begin_time && !end_time) return(0);

    if      (!begin_time) begin_time = end_time;
    else if (!end_time)   end_time   = begin_time;

    /* Get the file list and find the indexes of the begin and end files */

    nfiles = _dsproc_get_dsdir_files(dir, &files);

    if (!nfiles) return(0);

    bi = _dsproc_find_file_index(
        begin_time, 0, nfiles, files, dir->file_name_time);

    ei = _dsproc_find_file_index(
        end_time,   1, nfiles, files, dir->file_name_time);

    if (bi < 0) bi = 0;
    if (ei < 0) ei = nfiles - 1;

    /* Return an extra file on both sides to prevent newly created files
     * containing only header information from hiding existing data in
     * forward and/or backward searches.
     */

    if (bi > 0) bi -= 1;
    if (ei < nfiles - 1) ei += 1;

    /* Output the pointer to the first file and
     * return the number of files */

    *file_list = &(files[bi]);

    return(ei - bi + 1);
}

/**
 *  Private: Find all files in a datastream directory for a specified time range.
 *
 *  This function will return an array of pointers to the DSFile structures
 *  for all files in a datastream directory containing data for the specified
 *  time range.
 *
 *  If the begin_timeval is not specified, the file containing data for the
 *  time just prior to the end_timeval will be opened and returned.
 *
 *  If the end_timeval is not specified, the file containing data for the
 *  time just after the begin_timeval will be opened and returned.
 *
 *  The memory used by the output array is dynamically allocated and must
 *  be freed by the calling process when it is no longer needed. The DSFile
 *  structures pointed to by the array values, however, belong to the DSDir
 *  structure and must *not* be freed by the calling process.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  dir           - pointer to the DSDir
 *  @param  begin_timeval - beginning of the time range
 *  @param  end_timeval   - end of the time range
 *  @param  dsfile_list   - output: pointer to the array of pointers to the
 *                          requested DSFile structures.
 *
 *  @retval ndsfiles  the number of files found
 *  @retval -1        if an error occurred
 */
int _dsproc_find_dsfiles(
    DSDir     *dir,
    timeval_t *begin_timeval,
    timeval_t *end_timeval,
    DSFile  ***dsfile_list)
{
    time_t     begin_time;
    time_t     end_time;
    int        nfiles;
    char     **files;
    int        ndsfiles;
    DSFile   **dsfiles;
    DSFile    *dsfile;
    timeval_t  file_begin;
    timeval_t  file_end;
    int        fi;

    /* Initialize variables */

    *dsfile_list = ((DSFile  **)NULL);

    if ((!begin_timeval || !begin_timeval->tv_sec) &&
        (!end_timeval   || !end_timeval->tv_sec) ) {

        return(0);
    }

    /* Get the list of files that bracket and contain
     * data for the requested time range. */

    begin_time = (begin_timeval) ? begin_timeval->tv_sec : 0;
    end_time   = (end_timeval)   ? end_timeval->tv_sec   : 0;

    nfiles = _dsproc_find_dsdir_files(
        dir, begin_time, end_time, &files);

    if (nfiles <= 0) {
        return(nfiles);
    }

    /* Allocate memory for the output array */

    dsfiles = (DSFile **)calloc(nfiles, sizeof(DSFile *));
    if (!dsfiles) {

        ERROR( DSPROC_LIB_NAME,
            "Could not find DSFiles in: %s\n"
            " -> memory allocation error\n",
            dir->path);

        dsproc_set_status(DSPROC_ENOMEM);
        return(-1);
    }

    /* Loop over all files and return the ones in the requested range */

    ndsfiles = 0;

    for (fi = 0; fi < nfiles; fi++) {

        dsfile = _dsproc_get_dsfile(dir, files[fi]);
        if (!dsfile) {
            free(dsfiles);
            return(-1);
        }

        if (!_dsproc_refresh_dsfile_info(dsfile)) {
            return(-1);
        }

        if (dsfile->ntimes == 0) {
            continue;
        }

        file_begin = dsfile->timevals[0];
        file_end   = dsfile->timevals[dsfile->ntimes - 1];

        if (!begin_timeval || !begin_timeval->tv_sec) {

            /* We want the last file containing data prior to the end_timeval */

            if (TV_LT(file_begin, *end_timeval)) {
                dsfiles[0] = dsfile;
                ndsfiles   = 1;
            }
            else {
                break;
            }
        }
        else if (!end_timeval || !end_timeval->tv_sec) {

            /* We want the first file containing data after the begin_timeval */

            if (TV_GT(file_end, *begin_timeval)) {
                dsfiles[0] = dsfile;
                ndsfiles   = 1;
                break;
            }
        }
        else {

            /* We want all files that contain data for the specified range */

            if (!TV_GT(file_begin, *end_timeval) &&
                !TV_LT(file_end,   *begin_timeval)) {

                dsfiles[ndsfiles] = dsfile;
                ndsfiles++;
            }
        }

    } /* end loop over file names */

    if (ndsfiles) {
        *dsfile_list = dsfiles;
    }
    else {
        free(dsfiles);
    }

    return(ndsfiles);
}

/**
 *  Private: Find the next file that starts on or after the specified time.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  dir          - pointer to the DSDir
 *  @param  search_start - start time of search
 *  @param  dsfile       - output: pointer to the DSFile structure.
 *
 *  @retval  1  if a file was found
 *  @retval  0  if a file was not found
 *  @retval -1  if an error occurred
 */
int _dsproc_find_next_dsfile(
    DSDir     *dir,
    timeval_t *search_start,
    DSFile   **dsfile)
{
    time_t     search_begin;
    int        nfiles;
    char     **files;
    timeval_t  file_begin;
    int        fi;

    /* Initialize variables */

    *dsfile = (DSFile  *)NULL;

    if ((!search_start || !search_start->tv_sec)) {
        return(0);
    }

    /* Get the list of files that bracket and contain
     * data after the specified start time. */

    search_begin = search_start->tv_sec;

    nfiles = _dsproc_find_dsdir_files(
        dir, search_begin, search_begin, &files);

    if (nfiles <= 0) {
        return(0);
    }

    /* Loop over all files and return the one that starts after
     * the specified start time. */

    for (fi = 0; fi < nfiles; fi++) {

        *dsfile = _dsproc_get_dsfile(dir, files[fi]);
        if (!*dsfile) {
            return(-1);
        }

        if (!_dsproc_refresh_dsfile_info(*dsfile)) {
            *dsfile = (DSFile  *)NULL;
            return(-1);
        }

        if ((*dsfile)->ntimes == 0) {
            *dsfile = (DSFile  *)NULL;
            continue;
        }

        file_begin = (*dsfile)->timevals[0];

        /* We want the first file with a start time
        *  on or after the search start time */

        if (TV_GTEQ(file_begin, *search_start)) {
            return(1);
        }

    } /* end loop over file names */

    *dsfile = (DSFile  *)NULL;
    return(0);
}

/**
 *  Private: Get the list of files in a datastream directory.
 *
 *  The memory used by the returned file list belongs to the DSDir
 *  structure and must not be freed by the calling process.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  dir   - pointer to the DSDir
 *  @param  files - output: pointer to the list of files
 *
 *  @return
 *    -  number of files
 *    - -1 if an error occurred
 */
int _dsproc_get_dsdir_files(DSDir *dir, char ***files)
{
    struct stat     dir_stats;
    DIR            *dirp;
    struct dirent  *direntp;
    char          **new_list;
    size_t          new_size;
    int             status;
    char           *extp;
    int             found_version;
    int             fi;
    int             err_count;

    int  (*file_name_compare)(const void *, const void *);

    /* Initialize output */

    *files = (char **)NULL;

    /* Check to see if the directory exists */

    if (access(dir->path, F_OK) != 0) {

        if (errno == ENOENT) {
            return(0);
        }
        else {

            ERROR( DSPROC_LIB_NAME,
                "Could not access directory: %s\n"
                " -> %s\n", dir->path, strerror(errno));

            dsproc_set_status(DSPROC_EACCESS);
            return(-1);
        }
    }

    /* Check if the directory has been modified */

    if (stat(dir->path, &dir_stats) != 0 ) {

        ERROR( DSPROC_LIB_NAME,
            "Could not stat directory: %s\n"
            " -> %s\n", dir->path, strerror(errno));

        dsproc_set_status(DSPROC_EDIRLIST);
        return(-1);
    }

    if (dir->stats.st_mtime        == dir_stats.st_mtime       &&
#ifdef __APPLE__
        dir->stats.st_mtimespec.tv_sec  == dir_stats.st_mtimespec.tv_sec &&
        dir->stats.st_mtimespec.tv_nsec == dir_stats.st_mtimespec.tv_nsec) {
#else
        dir->stats.st_mtim.tv_sec  == dir_stats.st_mtim.tv_sec &&
        dir->stats.st_mtim.tv_nsec == dir_stats.st_mtim.tv_nsec) {
#endif

        dir->stats = dir_stats;
        *files     = dir->files;
        return(dir->nfiles);
    }

    /* Open the directory */

    dirp = opendir(dir->path);
    if (!dirp) {

        ERROR( DSPROC_LIB_NAME,
            "Could not open directory: %s\n"
            " -> %s\n", dir->path, strerror(errno));

        dsproc_set_status(DSPROC_EDIRLIST);
        return(-1);
    }

    /* Loop over directory entries */

    dir->nfiles   = 0;
    found_version = 0;

    for (;;) {

        /* Get the next directory entry */

        errno = 0;
        if (!(direntp = readdir(dirp))) {

            if (errno) {

                ERROR( DSPROC_LIB_NAME,
                    "Could not read directory: %s\n"
                    " -> %s\n", dir->path, strerror(errno));

                dsproc_set_status(DSPROC_EDIRLIST);

                closedir(dirp);
                return(-1);
            }

            break;
        }

        /* Skip dot files and the . and .. directories */

        if (direntp->d_name[0] == '.') {
            continue;
        }

        /* Check if this file matches one of the specified patterns */

        if (dir->patterns) {

            extp = (char *)NULL;

            if (dir->filter_versioned_files) {

                /* Check for .v# version extension and remove it from the file
                 * name before checking the if the file matches the patterns. */
                if (_dsproc_get_file_version(direntp->d_name, &extp) >= 0) {
                    *extp = '\0';
                    found_version = 1;
                }
            }

            status = relist_execute(
                dir->patterns, direntp->d_name, 0, NULL, NULL, NULL, NULL);

            if (extp) {
                *extp = '.';
            }

            if (status == 0) {
                continue;
            }

            if (status < 0) {

                ERROR( DSPROC_LIB_NAME,
                    "Could not get directory listing for: %s\n"
                    " -> regular expression error\n",
                    dir->path);

                dsproc_set_status(DSPROC_EDIRLIST);

                closedir(dirp);
                return(-1);
            }
        }

        /* Check if we need to increase the length of the file list */

        if (dir->nfiles == dir->max_files - 1) {

            new_size = dir->max_files * 2;
            new_list = (char **)realloc(
                dir->files, new_size * sizeof(char *));

            if (!new_list) {

                ERROR( DSPROC_LIB_NAME,
                    "Could not get directory listing for: %s\n"
                    " -> memory allocation error\n",
                    dir->path);

                dsproc_set_status(DSPROC_ENOMEM);

                closedir(dirp);
                return(-1);
            }

            dir->max_files = new_size;
            dir->files     = new_list;

            for (fi = dir->nfiles; fi < dir->max_files; fi++) {
                dir->files[fi] = (char *)NULL;
            }
        }

        /* Set the file name */

        if (dir->files[dir->nfiles]) {
            free(dir->files[dir->nfiles]);
        }

        dir->files[dir->nfiles] = strdup(direntp->d_name);
        if (!dir->files[dir->nfiles]) {

            ERROR( DSPROC_LIB_NAME,
                "Could not get directory listing for: %s\n"
                " -> memory allocation error\n",
                dir->path);

            dsproc_set_status(DSPROC_EDIRLIST);

            closedir(dirp);
            return(-1);
        }

        dir->nfiles++;
    }

    closedir(dirp);

    dir->stats = dir_stats;
    *files     = dir->files;

    if (dir->nfiles < 2) {
        return(dir->nfiles);
    }

    /* If versioned files were found,
     * filter the lower versioned files from the list */

    if (found_version) {
        /* The _dsproc_filter_versioned_files function requires that the
         * files are sorted alphanumerically */

        qsort(dir->files, dir->nfiles, sizeof(char *), qsort_strcmp);
        dir->nfiles = _dsproc_filter_versioned_files(dir->nfiles, dir->files);
    }

    /* Determine the file_name_compare function to use to sort the file list */

    if (dir->file_name_compare) {
        /* User specified file_name_compare function */
        file_name_compare = dir->file_name_compare;
    }
    else if (dir->file_name_time_patterns) {
        /* User specified file_name_time_patterns */
        file_name_compare = _dsproc_file_name_compare;
    }
    else if (dir->file_name_time) {

        if (dir->file_name_time == _dsproc_get_ARM_file_name_time) {
            /* Default for files with standard arm names */
            file_name_compare = qsort_strcmp;
        }
        else {
            /* User specified file_name_time function */
            file_name_compare = _dsproc_file_name_compare;
        }
    }
    else {

        if ((dir->ds->role == DSR_INPUT) &&
            (dir->ds->dsc_level[0] == '0')) {

            /* Default for 0-level input raw files */
            file_name_compare = qsort_numeric_strcmp;
        }
        else {
            /* Default for files with standard arm names.
             * We should never get here because in this case the
             * dir->file_name_time function should have already
             * been set to _dsproc_get_ARM_file_name_time(). */
            file_name_compare = qsort_strcmp;
        }
    }

    /* Sort the file list if necessary */

    if (file_name_compare == _dsproc_file_name_compare) {

        _QsortData.dir = dir;
        _QsortData.err_count = 0;

        qsort(dir->files, dir->nfiles, sizeof(char *), file_name_compare);

        err_count = _QsortData.err_count;

        _QsortData.dir = (DSDir *)NULL;
        _QsortData.err_count = 0;

        if (err_count) {

            ERROR( DSPROC_LIB_NAME,
                "Could not sort file list for %s datastream '%s'\n"
                " -> could not get time for one or more file names\n",
                _dsproc_dsrole_to_name(dir->ds->role), dir->ds->name);

            dsproc_set_status("Could Not Sort File List");

            return(-1);
        }
    }
    else if (!found_version || file_name_compare != qsort_strcmp) {
        qsort(dir->files, dir->nfiles, sizeof(char *), file_name_compare);
    }

    return(dir->nfiles);
}

/**
 *  Private: Get the time from a file name.
 *
 *  For 0-level input datastreams this function requires that
 *  the file name time patterns have been specified using the
 *  dsproc_set_file_name_time_patterns() function, or a file name time
 *  function has been specified using dsproc_set_file_name_time_function().
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  dir        pointer to the datastream's DSDir structure
 *  @param  file_name  file name to parse the time from
 *
 *  @retval  time  seconds since 1970
 *  @retval   0    a file name time pattern or function has not been specified,
 *                 invalid file name format, or a regex pattern matching error occurred
 */
time_t _dsproc_get_file_name_time(
    DSDir      *dir,
    const char *file_name)
{
    time_t      secs1970 = 0;
    RETimeRes   result;
    int         status;
    const char *errmsg;

    if (dir->file_name_time_patterns) {

        status = retime_list_execute(
            dir->file_name_time_patterns, file_name, &result);

        if (status < 0) {
            errmsg = "invalid file name time pattern specified";
            goto ERROR_EXIT;
        }

        if (status == 0) {
            errmsg = "invalid file name format";
            goto ERROR_EXIT;
        }

        secs1970 = retime_get_secs1970(&result);    

        if (secs1970 <= 0) {
            errmsg = "invalid file name format";
            goto ERROR_EXIT;
        }

        return(secs1970);
    }
    else if (dir->file_name_time) {

        secs1970 = dir->file_name_time(file_name);

        if (secs1970 == 0) {
            errmsg = "invalid file name format";
            goto ERROR_EXIT;
        }
    }
    else {
        errmsg = "a file name time pattern has not been specified";
        goto ERROR_EXIT;
    }

    return(secs1970);

ERROR_EXIT:

    if (_QsortData.dir) {
        _QsortData.err_count += 1;
    }

    if (_QsortData.err_count <= 11) {

        ERROR( DSPROC_LIB_NAME,
            "Could not get time from file name: %s\n"
            " -> %s for %s datastream '%s'\n",
            file_name, errmsg, _dsproc_dsrole_to_name(dir->ds->role), dir->ds->name);

        if (_QsortData.err_count > 10) {
            ERROR( DSPROC_LIB_NAME,
                "File name compare error count > 10\n"
                " -> suppressing file name compare error messages\n");
        }

        dsproc_set_status("Could Not Get Time From File Name");
    }

    return(0);
}

/**
 *  Private: Open a datastream file.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  file - pointer to the DSFile structure.
 *  @param  mode - open mode of the file
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int _dsproc_open_dsfile(DSFile *file, int mode)
{
    DSDir  *dir = file->dir;
    DSFile *prev_file;
    int     fi;

    /* Check if the file is already open. */

    if (file->ncid) {

        /* Close the file if we are changing from read to write mode */

        if ((mode & NC_WRITE) && !(file->mode & NC_WRITE)) {
            _dsproc_close_dsfile(file);
        }
    }

    /* Check if we need to open the file. */

    if (!file->ncid) {

        /* Check if this will exceed the maximum number of open files */

        fi = 0;

        while ((dir->nopen >= dir->max_open) && (fi < dir->ndsfiles)) {

            prev_file = dir->dsfiles[fi];

            if (prev_file->ncid) {
                _dsproc_close_dsfile(prev_file);
            }

            ++fi;
        }

        /* Open the file */

        if (!ncds_open(file->full_path, mode, &(file->ncid))) {

            ERROR( DSPROC_LIB_NAME,
                "Could not open data file: %s\n",
                file->full_path);

            dsproc_set_status(DSPROC_ENCOPEN);
            return(0);
        }

        file->mode  = mode;
        dir->nopen += 1;
    }

    file->touched = 1;

    return(1);
}

/**
 *  Private: Set input file list for Ingests from command line.
 *
 *  @param  cmd_line_arg - command line argument
 *
 *  @return
 *    - 1 if successful
 *    - 0 if a memory allocation error occurred
 */
int _dsproc_set_input_file_list(const char *cmd_line_arg)
{
    int count;

    _InputFilesString = strdup(cmd_line_arg);
    if (!_InputFilesString) goto MEM_ERROR;

    /* Count the number of commas in the input string */

    count = dsproc_count_csv_delims(_InputFilesString, ',');

    /* Allocate memory for the list */

    _InputFiles = (char **)calloc((count + 2), sizeof(char *));
    if (!_InputFiles) goto MEM_ERROR;

    /* Creat the file list */

    _NumInputFiles = dsproc_split_csv_string(
        _InputFilesString, ',', count + 2, _InputFiles);

    return(1);

MEM_ERROR:

    ERROR( DSPROC_LIB_NAME,
        "Could not create input file list from command line\n"
        " -> memory allocation error\n");

    dsproc_set_status(DSPROC_ENOMEM);
    return(0);
}

/**
 *  Private: Free input file list from command line.
 */
void _dsproc_free_input_file_list(void)
{
    if (_InputFiles) {
        free(_InputFiles);
        _InputFiles = (char **)NULL;
    }

    if (_InputFilesString) {
        free(_InputFilesString);
        _InputFilesString = (char *)NULL;
    }

    _NumInputFiles = 0;
}

/** @publicsection */

/*******************************************************************************
 *  Internal Functions Visible To The Public
 */

/**
 *  Close all open datastream files that haven't been touched
 *  since the last time this function was called.
 */
void dsproc_close_untouched_files(void)
{
    int         ds_id;
    DataStream *ds;
    DSDir      *dir;
    DSFile     *file;
    int         fi;

    for (ds_id = 0; ds_id < _DSProc->ndatastreams; ds_id++) {

        ds = _DSProc->datastreams[ds_id];

        if (ds && ds->dir) {

            dir = ds->dir;

            for (fi = 0; fi < dir->ndsfiles; fi++) {

                file = dir->dsfiles[fi];

                if (file->ncid && !file->touched) {
                    _dsproc_close_dsfile(file);
                }

                file->touched = 0;
            }
        }
    }
}

/**
 *  Set the maximum number of files that can be held open.
 *
 *  The default is to only allow a maximum of 64 files to be open at the same
 *  time per datastream. This function and be used to change this default.
 *
 *  @param  ds_id    - datastream ID
 *  @param  max_open - the maximum number of open files
 */
void dsproc_set_max_open_files(int ds_id, int max_open)
{
    DataStream *ds  = _DSProc->datastreams[ds_id];
    DSDir      *dir = ds->dir;

    dir->max_open = max_open;
}

/*******************************************************************************
 *  Public Functions
 */

/**
 *  Add datastream file patterns.
 *
 *  This function adds file patterns to look for when creating the list of
 *  files in the datastream directory. By default all files in the directory
 *  will be listed.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  ds_id       - datastream ID
 *  @param  npatterns   - number of file patterns
 *  @param  patterns    - list of extended regex file patterns (man regcomp)
 *  @param  ignore_case - ingnore case in file patterns
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int dsproc_add_datastream_file_patterns(
    int          ds_id,
    int          npatterns,
    const char **patterns,
    int          ignore_case)
{
    DataStream *ds  = _DSProc->datastreams[ds_id];
    DSDir      *dir = ds->dir;
    int         i;

    if (msngr_debug_level || msngr_provenance_level) {

        if (npatterns == 1) {

            DEBUG_LV1( DSPROC_LIB_NAME,
                "%s: Adding %s datastream file pattern: '%s'\n",
                ds->name, _dsproc_dsrole_to_name(ds->role), patterns[0]);
        }
        else {

            DEBUG_LV1( DSPROC_LIB_NAME,
                "%s: Adding %s datastream file patterns:\n",
                ds->name, _dsproc_dsrole_to_name(ds->role));

            for (i = 0; i < npatterns; i++) {

                DEBUG_LV1( DSPROC_LIB_NAME,
                    " - '%s'\n", patterns[i]);
            }
        }
    }

    if (!_dsproc_add_dsdir_patterns(
        dir, npatterns, patterns, ignore_case)) {

        return(0);
    }

    return(1);
}

/**
 *  Find all files in a datastream directory for a specified time range.
 *
 *  This function will return a list of all files in a datastream directory
 *  containing data for the specified time range.  This search will include
 *  the begin_time but exclude the end_time. That is, it will find files that
 *  include data greter than or equal to the begin time, and less than the
 *  end time.
 *
 *  If the begin_time is not specified, the file containing data for the
 *  time just prior to the end_time will be returned.
 *
 *  If the end_time is not specified, the file containing data for the
 *  time just after the begin_time will be returned.
 *
 *  The memory used by the returned file list is dynamically allocated and
 *  and must be freed using the dsproc_free_file_list() function.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  ds_id      - datastream ID
 *  @param  begin_time - beginning of the time range to search
 *  @param  end_time   - end of the time range to search
 *  @param  file_list  - output: pointer to the NULL terminated file list
 *
 *  @retval  nfiles  the number of files found
 *  @retval  -1      if an error occurred
 */
int dsproc_find_datastream_files(
    int       ds_id,
    time_t    begin_time,
    time_t    end_time,
    char   ***file_list)
{
    DataStream *ds = _DSProc->datastreams[ds_id];
    int         nfiles;
    char      **files;
    DSFile     *dsfile;
    time_t      file_begin;
    time_t      file_end;
    int         bi, ei, fi;
    char        ts1[32], ts2[32];

    /* Initialize variables */

    *file_list = (char **)NULL;

    if (msngr_debug_level || msngr_provenance_level) {

        DEBUG_LV1( DSPROC_LIB_NAME,
            "%s: Finding datastream files containing data\n"
            " - from:  %s\n"
            " - to:    %s\n",
            ds->name,
            format_secs1970(begin_time, ts1),
            format_secs1970(end_time, ts2));
    }

    if (!begin_time && !end_time) {
        goto RETURN_NOT_FOUND;
    }

    /* Check for required DataSteram properties */

    if (!ds) {

        ERROR( DSPROC_LIB_NAME,
            "Invalid Datastreamd Id: %s\n", ds_id);

        dsproc_set_status(DSPROC_EBADDSID);
        return(-1);
    }

    if (!ds->dir || !ds->dir->path) {

        ERROR( DSPROC_LIB_NAME,
            "Datastream path has not been set for: %s\n", ds->name);

        dsproc_set_status(DSPROC_EDSPATH);
        return(-1);
    }

    if (!ds->dir->file_name_time) {

        ERROR( DSPROC_LIB_NAME,
            "Datastream file_name_time function has not been set for: %s\n",
            ds->name);

        dsproc_set_status(DSPROC_ENOFILETIME);
        return(-1);
    }

    /* Get the list of files that bracket and contain
     * data for the requested time range. */

    nfiles = _dsproc_find_dsdir_files(
        ds->dir, begin_time, end_time, &files);

    if (nfiles <= 0) {

        if (nfiles == 0) {
            goto RETURN_NOT_FOUND;
        }

        return(-1);
    }

    /* Loop over all files and return the ones in the requested range */

    bi = ei = -1;

    for (fi = 0; fi < nfiles; fi++) {

        dsfile = _dsproc_get_dsfile(ds->dir, files[fi]);
        if (!dsfile) {
            return(-1);
        }

        if (!_dsproc_refresh_dsfile_info(dsfile)) {
            return(-1);
        }

        if (dsfile->ntimes == 0) {
            continue;
        }

        file_begin = dsfile->timevals[0].tv_sec;
        file_end   = dsfile->timevals[dsfile->ntimes - 1].tv_sec;

        if (!begin_time) {

            /* We want the last file containing data prior to the end_timeval */

            if (file_begin < end_time) {
                bi = ei = fi;
            }
            else {
                break;
            }
        }
        else if (!end_time) {

            /* We want the first file containing data after the begin_timeval */

            if (file_end > begin_time) {
                bi = ei = fi;
                break;
            }
        }
        else {

            /* We want all files that contain data for the specified range,
             * this includes the begin_time but excludes the end_time */

            if ((file_begin < end_time) &&
                (file_end   >= begin_time)) {

                if (bi == -1) bi = fi;
                ei = fi;
            }
        }

    } /* end loop over file names */

    if (bi == -1) {
        goto RETURN_NOT_FOUND;
    }

    /* Create output file list */

    nfiles = ei - bi + 1;

    *file_list = _dsproc_clone_file_list(nfiles, &(files[bi]));
    if (!*file_list) {

        ERROR( DSPROC_LIB_NAME,
            "Could not create file list for datastream: %s\n"
            " -> memory allocation error\n",
            ds->name);

        dsproc_set_status(DSPROC_ENOMEM);
        return(-1);
    }

    if (msngr_debug_level || msngr_provenance_level) {

        for (fi = 0; fi < nfiles; ++fi) {
            DEBUG_LV1( DSPROC_LIB_NAME,
                " - found: %s\n", (*file_list)[fi]);
        }
    }

    return(nfiles);

RETURN_NOT_FOUND:
    DEBUG_LV1( DSPROC_LIB_NAME,
        " - no stored data found for requested range\n");
    return(0);
}

/**
 *  Free a null terminated list of file names.
 *
 *  @param  file_list - null terminated list of file names
 */
void dsproc_free_file_list(char **file_list)
{
    int fi;

    if (file_list) {
        for (fi = 0; file_list[fi]; ++fi) {
            free(file_list[fi]);
        }
        free(file_list);
    }
}

/**
 *  Get the list of files in a datastream directory.
 *
 *  By default the returned list will be sorted using the
 *  qsort_numeric_strcmp() function for 0-level input datastreams and the
 *  qsort_strcmp() function will be used for all other datastreams.
 *
 *  For 0-level input datastreams, a different file name compare function can
 *  be set using the dsproc_set_file_name_compare_function() function.
 *  Alternatively the file name time pattern(s) can be specified with the
 *  dsproc_set_file_name_time_patterns() function.  If a file name time
 *  pattern is specified it will be used to sort the files in chronological
 *  order.  If both the file name compare function and file name time patterns
 *  are specified, the file name compare function will take precedence.
 *
 *  The memory used by the returned file list is dynamically allocated and
 *  and must be freed using the dsproc_free_file_list() function.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  ds_id     - datastream ID
 *  @param  file_list - output: pointer to the NULL terminated list of file names
 *
 *  @return
 *    -  number of files
 *    - -1 if an error occurred
 */
int dsproc_get_datastream_files(
    int     ds_id,
    char ***file_list)
{
    DataStream  *ds = _DSProc->datastreams[ds_id];
    char       **dsdir_files;
    int          nfiles;

    *file_list = (char **)NULL;

    /* Check if an input file list was specified on the command line if this
     * is a level 0 datastream for an Ingest process. */

    if (_NumInputFiles &&
        _InputFiles    &&
        *(ds->dsc_level) == '0') {

        DEBUG_LV1( DSPROC_LIB_NAME,
            "%s: Using input file list from command line\n",
            ds->name);

        *file_list = _dsproc_clone_file_list(_NumInputFiles, _InputFiles);
        if (!*file_list) {

            ERROR( DSPROC_LIB_NAME,
                "Could not create input file list\n"
                " -> memory allocation error\n");

            dsproc_set_status(DSPROC_ENOMEM);
            return(-1);
        }

        return(_NumInputFiles);
    }

    /* Get the list of files in the datastream directory */

    if (!ds->dir || !ds->dir->path) {

        ERROR( DSPROC_LIB_NAME,
            "Datastream path has not been set for: %s\n", ds->name);

        dsproc_set_status(DSPROC_EDSPATH);
        return(-1);
    }

    nfiles = _dsproc_get_dsdir_files(ds->dir, &dsdir_files);

    if (nfiles > 0) {

        *file_list = _dsproc_clone_file_list(nfiles, dsdir_files);

        if (!*file_list) {

            ERROR( DSPROC_LIB_NAME,
                "Could not get file list for %s datastream: %s\n"
                " -> memory allocation error\n",
                _dsproc_dsrole_to_name(ds->role), ds->name);

            dsproc_set_status(DSPROC_ENOMEM);
            return(-1);
        }
    }

    return(nfiles);
}

/**
 *  Get the time from a file name.
 *
 *  For 0-level input datastreams this function requires that
 *  the file name time patterns have been specified using the
 *  dsproc_set_file_name_time_patterns() function, or a file name time
 *  function has been specified using dsproc_set_file_name_time_function().
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  ds_id      datastream ID
 *  @param  file_name  file name to parse the time from
 *
 *  @retval  time  seconds since 1970
 *  @retval   0    a file name time pattern or function has not been specified,
 *                 invalid file name format, or a regex pattern matching error occurred
 */
time_t dsproc_get_file_name_time(int ds_id, const char *file_name)
{
    DataStream *ds = _DSProc->datastreams[ds_id];
    return(_dsproc_get_file_name_time(ds->dir, file_name));
}

/**
 *  Set the datastream file extension.
 *
 *  @param  ds_id     - datastream ID
 *  @param  extension - file extension
 */
void dsproc_set_datastream_file_extension(
    int         ds_id,
    const char *extension)
{
    DataStream *ds   = _DSProc->datastreams[ds_id];
    const char *extp = extension;

    while (*extp == '.') extp++;

    DEBUG_LV1( DSPROC_LIB_NAME,
        "%s: Setting datastream file extension to: '%s'\n",
        ds->name, extp);

    strncpy((char *)ds->extension, extp, 63);
}

/**
 *  Set the path to the datastream directory.
 *
 *  Default datastream path set if path == NULL:
 *
 *    - dsenv_get_collection_dir() for level 0 input datastreams
 *    - dsenv_get_input_datastream_dir() for all other input datastreams
 *    - dsenv_get_output_datastream_dir() for output datastreams
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  ds_id - datastream ID
 *  @param  path  - path to the datastream directory,
 *                  or NULL to set the default datastream path
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurs
 */
int dsproc_set_datastream_path(int ds_id, const char *path)
{
    DataStream *ds = _DSProc->datastreams[ds_id];
    int     free_path = 0;
    int     status;
    size_t  length;
    char   *pattern;

    if (!path) {

        if (ds->role == DSR_INPUT) {

            if (ds->dsc_level[0] == '0') {

                status = dsenv_get_collection_dir(
                    ds->site, ds->facility, ds->dsc_name, ds->dsc_level,
                    (char **)&path);

                if (status == 0) {

                    ERROR( DSPROC_LIB_NAME,
                        "Could not set path for %s datastream: %s\n"
                        " -> the COLLECTION_DATA environment variable was not found\n",
                        _dsproc_dsrole_to_name(ds->role), ds->name);

                    dsproc_set_status(DSPROC_EDSPATH);
                    return(0);
                }
            }
            else {

                status = dsenv_get_input_datastream_dir(
                    ds->site, ds->facility, ds->dsc_name, ds->dsc_level,
                    (char **)&path);
            }
        }
        else {

            status = dsenv_get_output_datastream_dir(
                ds->site, ds->facility, ds->dsc_name, ds->dsc_level,
                (char **)&path);
        }

        if (status < 0) {
            dsproc_set_status(DSPROC_ENOMEM);
            return(0);
        }

        if (status == 0) {

            ERROR( DSPROC_LIB_NAME,
                "Could not set path for %s datastream: %s\n"
                " -> the DATASTREAM_DATA environment variable was not found\n",
                _dsproc_dsrole_to_name(ds->role), ds->name);

            dsproc_set_status(DSPROC_EDSPATH);
            return(0);
        }

        free_path = 1;
    }

    if (msngr_debug_level || msngr_provenance_level) {

        DEBUG_LV1( DSPROC_LIB_NAME,
            "%s: Setting %s datastream path: %s\n",
            ds->name, _dsproc_dsrole_to_name(ds->role), path);
    }

    if (ds->dir) {

        if (ds->dir->path && (strcmp(ds->dir->path, path) == 0)) {
            return(1);
        }

        _dsproc_free_dsdir(ds->dir);
    }

    ds->dir = _dsproc_create_dsdir(path);
    ds->dir->ds = ds;

    if (free_path) {
        free((void *)path);
    }

    if (!ds->dir) {
        return(0);
    }

    if (ds->flags & DS_FILTER_VERSIONED_FILES) {
        ds->dir->filter_versioned_files = 1;
    }

    if ((ds->role == DSR_INPUT) &&
        (ds->dsc_level[0] == '0')) {

        /* Raw input datastream */
    }
    else {

        /* Set datastream file pattern */

        length = strlen(ds->name) + 64;

        pattern = malloc(length * sizeof(char));
        if (!pattern) {

            ERROR( DSPROC_LIB_NAME,
                "Could not set path for %s datastream: %s\n"
                " -> memory allocation error\n",
                _dsproc_dsrole_to_name(ds->role), ds->name);

            dsproc_set_status(DSPROC_ENOMEM);
            return(0);
        }

        sprintf(pattern, "^%s\\.[0-9]{8}\\.[0-9]{6}\\.(cdf|nc)$", ds->name);

        _dsproc_add_dsdir_patterns(ds->dir, 1, (const char **)&pattern, 0);

        free(pattern);

        /* Set function used to get the time from the file name */

        dsproc_set_file_name_time_function(ds_id, _dsproc_get_ARM_file_name_time);
    }

    return(1);
}

/**
 *  Set the file name compare function used to sort the file list.
 *
 *  Alternatively the file name time pattern(s) can be specified using
 *  dsproc_set_file_name_time_patterns(), or a file_name_time function
 *  can be specified using dsproc_set_file_name_time_function(). If more
 *  than one are specified the order of precedence is:
 *
 *      file_name_compare function
 *      file name time patterns
 *      file_name_time function
 *
 *  @param  ds_id    - datastream ID
 *  @param  function - the file name compare function used to sort the file list
 */
void dsproc_set_file_name_compare_function(
    int     ds_id,
    int    (*function)(const void *, const void *))
{
    DataStream *ds  = _DSProc->datastreams[ds_id];
    DSDir      *dir = ds->dir;

    dir->file_name_compare = function;
    dir->stats.st_mtime = 0;
}

/**
 *  Set the function used to parse the time from a file name.
 *
 *  The file_name_time function will also be used to sort the list of files in
 *  the datastream directory. Alternatively a file_name_compare function can be
 *  specified using dsproc_set_file_name_compare_function(), or the file name
 *  time pattern(s) can be specified using dsproc_set_file_name_time_patterns().
 *  If more than one are specified the order of precedence is:
 *
 *      file_name_compare function
 *      file name time patterns
 *      file_name_time function
 *
 *  @param  ds_id    - datastream ID
 *  @param  function - The function used to parse the time from a file name.
 *                     This function must return the time in seconds since 1970,
 *                     or 0 for invalid file name format.
 */
void dsproc_set_file_name_time_function(
    int      ds_id,
    time_t (*function)(const char *))
{
    DataStream *ds  = _DSProc->datastreams[ds_id];
    DSDir      *dir = ds->dir;

    dir->file_name_time = function;
}

/**
 *  Set the file name time pattern(s) used to parse the time from a file name.
 *
 *  The file name time pattern(s) will also be used to sort the list of files in
 *  the datastream directory. Alternatively a file_name_compare function can be
 *  specified using dsproc_set_file_name_compare_function(), or a file_name_time
 *  function can be specified using dsproc_set_file_name_time_function(). If more
 *  than one are specified the order of precedence is:
 *
 *      file_name_compare function
 *      file name time patterns
 *      file_name_time function
 *
 *  The file name time pattern(s) contain a mixture of regex (see regex(7)) and
 *  time format codes similar to the strptime function. The time format codes
 *  recognized by this function begin with a % and are followed by one of the
 *  following characters:
 *
 *    - 'C'  century number (year/100) as a 2-digit integer
 *    - 'd'  day number in the month (1-31).
 *    - 'e'  day number in the month (1-31).
 *    - 'h'  hour * 100 + minute (0-2359)
 *    - 'H'  hour (0-23)
 *    - 'j'  day number in the year (1-366).
 *    - 'm'  month number (1-12)
 *    - 'M'  minute (0-59)
 *    - 'n'  arbitrary whitespace
 *    - 'o'  time offset in seconds
 *    - 'p'  AM or PM
 *    - 'q'  Mac-Time: seconds since 1904-01-01 00:00:00 +0000 (UTC)
 *    - 's'  seconds since Epoch, 1970-01-01 00:00:00 +0000 (UTC)
 *    - 'S'  second (0-60; 60 may occur for leap seconds)
 *    - 't'  arbitrary whitespace
 *    - 'y'  year within century (0-99)
 *    - 'Y'  year with century as a 4-digit integer
 *    - '%'  a literal "%" character
 * 
 *  An optional 0 character can be used between the % and format code to
 *  specify that the number must be zero padded. For example, '%0d' specifies
 *  that the day range is 01 to 31.
 *
 *  Multiple patterns can be provided and will be checked in the specified order.
 *
 *  Examples:
 *
 *    - "%Y%0m%0d\\.%0H%0M%0S\\.[a-z]$" would match *20150923.072316.csv
 *    - "%Y-%0m-%0d_%0H:%0M:%0S\\.dat"  would match *2015-09-23_07:23:16.dat
 *
 *  @param  ds_id     - datastream ID
 *  @param  npatterns - number of pattern strings in the list
 *  @param  patterns  - list of file name time patterns
 *
 *  @return
 *    - 1 if successful
 *    - 0 if a regex compile error occurred
 */
int dsproc_set_file_name_time_patterns(
    int          ds_id,
    int          npatterns,
    const char **patterns)
{
    DataStream *ds  = _DSProc->datastreams[ds_id];
    DSDir      *dir = ds->dir;

    dir->file_name_time_patterns = retime_list_compile(npatterns, patterns, 0);

    if (!dir->file_name_time_patterns) {

        ERROR( DSPROC_LIB_NAME,
            "Could not compile list of file name time patterns for %s\n",
            ds->name);

        dsproc_set_status("Could not compile list of file name time patterns");
        return(0);
    }

    return(1);
}
