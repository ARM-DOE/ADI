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

/** @file dsproc_csv_ingest_config.c
 *  Functions for Reading CSV Ingest Configuration Files.
 */

#include "dsproc3.h"

/*******************************************************************************
 *  Private Data and Functions
 */
/** @privatesection */

/**
 *  List of config file key words.
 */
const char *_ConfKeys[] =
{
    "FILE_NAME_PATTERNS",
    "FILE_TIME_PATTERNS",
    "DELIMITER",
    "HEADER_LINE",
    "HEADER_LINE_TAG",
    "HEADER_LINE_NUMBER",
    "NUMBER_OF_HEADER_LINES",
    "NUMBER_OF_COLUMNS",
    "TIME_COLUMN_PATTERNS",
    "SPLIT_INTERVAL",
    "FIELD_MAP",
    NULL
};

/**
 *  Get the time from a CSV Ingest configuration file name.
 *
 *  @param  file_name  the name of the file
 *
 *  @retval  time  seconds since 1970
 *  @retval  0     if an error occurred
 */
static time_t _csv_get_conf_file_name_time(const char *file_name)
{
    int   YYYY, MM, DD, hh, mm, ss;
    int   count;
    char *chrp;

    /* File names look like:
     *
     *     SSSnameF#.YYYYMMDD.hhmmss.csv_conf
     *
     * or:
     *
     *     SSSnameF#.dl.YYYYMMDD.hhmmss.csv_conf
     */

    chrp = strrchr(file_name, '.');
    if (!chrp) return(0);

    for (count = 0; count < 2; ++count) {
        if (chrp != file_name) {
            for (--chrp; *chrp != '.' && chrp != file_name; --chrp);
        }
    }

    if (*chrp == '.') ++chrp;

    YYYY = MM = DD = hh = mm = ss = 0;

    sscanf(chrp, "%4d%2d%2d.%2d%2d%2d", &YYYY, &MM, &DD, &hh, &mm, &ss);

    return(get_secs1970(YYYY, MM, DD, hh, mm, ss));
}

/**
 *  Get the list of search paths for CSV Ingest configuration files.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  conf    pointer to the CSVConf structure.
 *
 *  @param  flags   control flags:
 *
 *                    - CSV_CHECK_DATA_CONF  check for config files under the root directory
 *                                           defined by the CONF_DATA environment variable.
 *
 *  @param  npaths  output: number of search paths.
 *  @param  paths   output: list of search paths.
 *
 *  @retval   1  if successful
 *  @retval   0  if error occurred
 */
static int _csv_get_conf_search_paths(CSVConf *conf, int flags, int *npaths, const char ***paths)
{
    const char  *proc_name = conf->proc;
    const char  *conf_data = (const char *)NULL;
    const char  *ingest_home;

    int    search_npaths;
    char **search_paths;
    char   path[PATH_MAX];
    int    pi;

    /* Check if the conf->file_path has already been set */

    if (conf->file_path) {

        *paths  = &(conf->file_path);
        *npaths = 1;

        return(1);
    }

    /* Check if the search paths have already been set */

    if (conf->search_npaths) {

        *npaths = conf->search_npaths;
        *paths  = conf->search_paths;

        return(1);
    }

    /* Create list of default search paths */

    ingest_home = getenv("INGEST_HOME");

    if (flags & CSV_CHECK_DATA_CONF) {

        conf_data = getenv("CONF_DATA");

        if (!conf_data && !ingest_home) {

            ERROR( DSPROC_LIB_NAME,
                "Could not create configuration file search paths:\n"
                " -> environment variables CONF_DATA and INGEST_HOME do not exist");

            dsproc_set_status(DSPROC_ECSVCONF);

            return(0);
        }
    }
    else if (!ingest_home) {

        ERROR( DSPROC_LIB_NAME,
            "Could not create configuration file search paths:\n"
            " -> environment variable INGEST_HOME does not exist");

        dsproc_set_status(DSPROC_ECSVCONF);

        return(0);
    }

    search_paths = (char **)calloc(2, sizeof(char *));
    if (!search_paths) goto MEMORY_ERROR;

    search_npaths = 0;

    for (pi = 0; pi < 2; ++pi) {

        if (pi == 0) {
            if (!conf_data) continue;
            snprintf(path, PATH_MAX, "%s/%s", conf_data, proc_name);
        }
        else {
            if (!ingest_home) continue;
            snprintf(path, PATH_MAX, "%s/conf/ingest/%s", ingest_home, proc_name);
        }

        search_paths[search_npaths] = strdup(path);
        if (!search_paths[search_npaths]) goto MEMORY_ERROR;
        search_npaths++;
    }

    conf->search_npaths = search_npaths;
    conf->search_paths  = (const char **)search_paths;

    *npaths = conf->search_npaths;
    *paths  = (const char **)conf->search_paths;

    return(1);

MEMORY_ERROR:

    if (search_paths) {

        for (pi = 0; pi < search_npaths; ++pi) {
            free(search_paths[search_npaths]);
        }

        free(search_paths);
    }

    ERROR( DSPROC_LIB_NAME,
        "Memory allocation error creating list of configuration file search paths\n");

    dsproc_set_status(DSPROC_ENOMEM);

    return(0);
}

/**
 *  Find a CSV Ingest configuration file.
 *
 *  The frist time this function is called the data_time argument must be
 *  set to 0.  This will find the main conf file containing the the file
 *  name patterns and all default configuration settings. It will also set
 *  the path to look for time varying conf files in subsequent calls to
 *  this function.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  conf       Pointer to the CSVConf structure. The file name and
 *                     path will be stored in the file_name and file_path
 *                     structure members.
 *
 *  @param  data_time  The start time of the data being processed, this should
 *                     be determined from the file name. Specify 0 to find the
 *                     main conf file containing the the file name patterns
 *                     and all default configuration settings.
 *
 *  @param  flags      Control Flags:
 *
 *                       - CSV_CHECK_DATA_CONF  check for config files under the root directory
 *                                              defined by the CONF_DATA environment variable.
 *
 *  @param  path       output buffer for the file path,
 *
 *  @param  name       output buffer for the file name
 *
 *  @retval   1  if a file was found
 *  @retval   0  if a file was not found
 *  @retval  -1  if error occurred
 */
static int _csv_find_conf_file(CSVConf *conf, time_t data_time, int flags, char *path, char *name)
{
    const char  *site      = conf->site;
    const char  *fac       = conf->fac;
    const char  *base_name = conf->name;
    const char  *level     = conf->level;

    int          search_npaths;
    const char **search_paths;

    char         full_path[PATH_MAX];
    char         pattern[PATH_MAX];
    const char  *patternp;
    int          found_file;
    int          pi, ni;

    DirList     *dirlist;
    int          nfiles;
    char       **file_list;
    time_t       file_time;
    char         ts1[32];
    int          fi;

    found_file = 0;

    if (data_time == 0) {

        /* Looking for main configuration file */

        if (!_csv_get_conf_search_paths(conf, flags, &search_npaths, &search_paths)) {
            return(-1);
        }

        /* Loop over possible configuration file directories */

        for (pi = 0; pi < search_npaths; ++pi) {

            strncpy(path, search_paths[pi], PATH_MAX);

            DSPROC_DEBUG_LV1(
                "Checking for main csv_conf file in: %s\n", path);

            if (access(path, F_OK) != 0) {

                if (errno != ENOENT) {

                    ERROR( DSPROC_LIB_NAME,
                        "Could not access directory: %s\n"
                        " -> %s\n", path, strerror(errno));

                    dsproc_set_status(DSPROC_EACCESS);

                    return(-1);
                }

                DSPROC_DEBUG_LV1(
                    " - path does not exist\n");

                continue;
            }

            /* Loop over possible names of configuration files */

            for (ni = 0; ni < 2; ++ni) {

                if (level) {
                    if (ni == 0) {
                        snprintf(name, PATH_MAX, "%s%s%s.%s.csv_conf", site, base_name, fac, level);
                    }
                    else {
                        snprintf(name, PATH_MAX, "%s.%s.csv_conf", base_name, level);
                    }
                }
                else {
                    if (ni == 0) {
                        snprintf(name, PATH_MAX, "%s%s%s.csv_conf", site, base_name, fac);
                    }
                    else {
                        snprintf(name, PATH_MAX, "%s.csv_conf", base_name);
                    }
                }

                DSPROC_DEBUG_LV1(" - checking for file: %s\n", name);

                snprintf(full_path, PATH_MAX, "%s/%s", path, name);

                if (access(full_path, F_OK) == 0) {
                    DSPROC_DEBUG_LV1(" - found\n");
                    found_file = 1;
                    break;
                }
                else if (errno == ENOENT) {
                    DSPROC_DEBUG_LV1(" - not found\n");
                }
                else {

                    ERROR( DSPROC_LIB_NAME,
                        "Could not access file: %s\n"
                        " -> %s\n", full_path, strerror(errno));

                    dsproc_set_status(DSPROC_EACCESS);

                    return(-1);
                }

            } // end loop over file name search lists

            if (found_file) break;

        } // end loop over possible configuration file directories
    }
    else { // data_time > 0

        /* Looking for time specific configuration file names */

        nfiles  = 0;
        dirlist = (DirList *)NULL;

        if (conf->dirlist) {

            dirlist = conf->dirlist;
            nfiles  = dirlist_get_file_list(dirlist, &file_list);
            if (nfiles < 0) {

                ERROR( DSPROC_LIB_NAME,
                    "Could not get list of configuration files in: %s\n",
                    path);

                dsproc_set_status(DSPROC_EDIRLIST);

                return(-1);
            }

            strncpy(path, dirlist->path, PATH_MAX);
        }
        else {

            if (level) {
                sprintf(pattern, "^%s%s%s\\.%s\\.[0-9]{8}\\.[0-9]{6}\\.csv_conf", site, base_name, fac, level);
            }
            else {
                sprintf(pattern, "^%s%s%s\\.[0-9]{8}\\.[0-9]{6}\\.csv_conf", site, base_name, fac);
            }

            patternp = &(pattern[0]);

            if (!_csv_get_conf_search_paths(conf, flags, &search_npaths, &search_paths)) {
                return(-1);
            }

            /* Loop over possible configuration file directories */

            for (pi = 0; pi < search_npaths; ++pi) {

                strncpy(path, search_paths[pi], PATH_MAX);

                DSPROC_DEBUG_LV1(
                    "Checking for time varying csv_conf files in: %s\n", path);

                if (access(path, F_OK) != 0) {

                    if (errno != ENOENT) {

                        ERROR( DSPROC_LIB_NAME,
                            "Could not access directory: %s\n"
                            " -> %s\n", path, strerror(errno));

                        dsproc_set_status(DSPROC_EACCESS);

                        return(-1);
                    }

                    DSPROC_DEBUG_LV1(
                        " - path does not exist\n");

                    continue;
                }

                /* Check for time varying configuration files */

                if (dirlist) dirlist_free(dirlist);

                dirlist = dirlist_create(path, 0);
                if (!dirlist ||
                    !dirlist_add_patterns(dirlist, 1, &patternp, 0)) {

                    ERROR( DSPROC_LIB_NAME,
                        "Could not create configuration files list for: %s\n",
                        path);

                    dsproc_set_status(DSPROC_EDIRLIST);

                    if (dirlist) dirlist_free(dirlist);
                    return(-1);
                }

                nfiles = dirlist_get_file_list(dirlist, &file_list);
                if (nfiles < 0) {

                    ERROR( DSPROC_LIB_NAME,
                        "Could not get configuration files list for: %s\n",
                        path);

                    dsproc_set_status(DSPROC_EDIRLIST);

                    dirlist_free(dirlist);
                    return(-1);
                }
                else if (nfiles == 0) {
                    DSPROC_DEBUG_LV1(" - none found\n");
                }
                else {
                    DSPROC_DEBUG_LV1(" - found\n");
                    break;
                }

            } /* end loop over possible configuration file directories */

            conf->dirlist = dirlist;

        } // end if !conf->dirlist

        /* Now look for the file for the specified data time */

        if (nfiles > 0) {

            DSPROC_DEBUG_LV1(
                "Looking for csv_conf file for data time: %s\n",
                format_secs1970(data_time, ts1));

            for (fi = nfiles - 1; fi > -1; --fi) {
                file_time = _csv_get_conf_file_name_time(file_list[fi]);
                if (data_time >= file_time) break;
            }

            if (fi < 0) {
                DSPROC_DEBUG_LV1(" - not found\n");
            }
            else {
                found_file = 1;
                strncpy(name, file_list[fi], PATH_MAX);
                DSPROC_DEBUG_LV1(" - found: %s\n", name);
            }
        }

    }  // end if data_time > 0

    return(found_file);
}

/**
 *  Split a string on the next delimiter.
 *
 *  This function will return a pointer to the string following the next
 *  delimiter in the input string. The input string will also be terminated
 *  at the delimiter and trailing whitespace will be removed.
 *
 *  The returned pointer will only be valid as long as the memory used by the
 *  input string is not altered or freed.
 *
 *  @param  strp    pointer to the delimited string
 *  @param  delim   delimiter character
 *
 *  @retval strp    pointer to the beginning of the next string
 *  @retval NULL    if the delimiter was not found
 */
static char *_csv_split_delim(char *strp, char delim)
{
    char *delimp;
    char *endp;

    delimp = dsproc_find_csv_delim(strp, delim);
    if (!delimp) return((char *)NULL);

    /* Trim trailing white-space from the previous substring */

    endp = delimp - 1;
    while (endp > strp && isspace(*endp)) *endp-- = '\0';

    /* Terminate field */

    *delimp = '\0';

    /* Skip leading white-space in the next substring */

    strp = dsproc_skip_csv_whitespace(delimp+1, delim);

    return(strp);
}

/**
 *  Strip comments from in memory copy of conf file.
 *
 *  @param  file_data  pointer to start of the config file data
 */
static void _csv_strip_comments(char *file_data)
{
    char *cp1   = file_data;
    char *cp2   = file_data;
    char  quote = '\0';

    while (*cp2 != '\0') {

        if (*cp2 == '"' || *cp2 == '\'') {

            /* quoted strings */

            quote = *cp2;

            *cp1++ = *cp2++;

            while (*cp2 != '\0') {

                if (*cp2 == quote) {

                    if (*(cp2+1) == quote) {
                        *cp1++ = *cp2++;
                    }
                    else {
                        break;
                    }
                }

                *cp1++ = *cp2++;
            }
        }
        else if (*cp2 == '#') {

            /* comments */

            for (++cp2; *cp2 != '\n' && *cp2 != '\0'; ++cp2);
        }

        *cp1++ = *cp2++;
    }

    *cp1++ = *cp2++;
}

/**
 *  Trim end of line whitespace.
 *
 *  @param  linep  pointer to the line to trim
 */
static void _csv_trim_eol(char *linep)
{
    char *eol = linep + strlen(linep) - 1;
    while ((eol >= linep) && isspace(*eol)) *eol-- = '\0';
}

/**
 *  Trim beginning and ending quotes from a string.
 *
 *  @param  linep  pointer to the line to trim
 *
 *  @retval  linep  pointer to the beging of the line
 */
static char *_csv_trim_quotes(char *linep)
{
    size_t length = strlen(linep);

    if (length > 1) {

        if ((*linep == '"' || *linep == '\'') &&
            linep[length-1] == *linep) {

            linep[length-1] = '\0';
            linep += 1;
        }
    }

    return(linep);
}

/**
 *  Load a CSV Configuration file into a CVSConf structure.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  conf   pointer to CSVConf structure to populate
 *  @param  path   path to the csv configuration file
 *  @param  name   name of the csv configuration file
 *  @param  flags  reserved for control flags
 *
 *  @retval  1  if successful
 *  @retval  0  if an error occurred
 */
static int _csv_load_conf_file(
    CSVConf    *conf,
    const char *path,
    const char *name,
    int         flags)
{
    char        full_path[PATH_MAX];
    struct stat file_stats;
    size_t      nbytes;
    size_t      nread;
    FILE       *fp;
    char       *file_data;
    char       *linep;
    int         linenum;
    char       *eol;
    char        chr;

    const char *key;
    size_t      keylen;
    size_t      linelen;

    int         count;
    int         buflen;
    char      **buffer;

    char       *tc_name;
    char       *out_name;
    int         reload;
    int         ki;

    DSPROC_DEBUG_LV1("Reading Configuration File: %s/%s\n", path, name);

    /* Set the full path to the conf file */

    snprintf(full_path, PATH_MAX, "%s/%s", path, name);

    /* Get the file status */

    if (stat(full_path, &file_stats) < 0) {

        ERROR( DSPROC_LIB_NAME,
            "Could not get file stats for conf file: %s\n"
            " -> %s\n", full_path, strerror(errno));

        dsproc_set_status(DSPROC_EFILESTATS);

        return(0);
    }

    /* Read in the entire file */

    nbytes = file_stats.st_size;
    if (nbytes == 0) return(1);

    file_data = (char *)malloc((nbytes + 1) * sizeof(char));
    if (!file_data) {

         ERROR( DSPROC_LIB_NAME,
            "Memory allocation error loading conf file: %s\n",
            full_path);

        dsproc_set_status(DSPROC_ENOMEM);

        return(0);
    }

    fp = fopen(full_path, "r");
    if (!fp) {

        ERROR( DSPROC_LIB_NAME,
            "Could not open file: %s\n"
            " -> %s\n", full_path, strerror(errno));

        dsproc_set_status(DSPROC_EFILEOPEN);

        free(file_data);
        return(0);
    }

    nread = fread(file_data, 1, nbytes, fp);
    fclose(fp);

    if (nread != nbytes) {

        ERROR( DSPROC_LIB_NAME,
            "Could not read conf file: %s\n"
            " -> %s\n", full_path, strerror(errno));

        dsproc_set_status(DSPROC_EFILEREAD);

        free(file_data);
        return(0);
    }

    file_data[nbytes] = '\0';

    /* Remove comments */

    _csv_strip_comments(file_data);

    /* Allocate memory for the buffer */

    buflen = 64;
    buffer = calloc(buflen, sizeof(char *));
    if (!buffer) goto MEMORY_ERROR;

    /* Loop over lines from the conf file */

    key     = (char *)NULL;
    linenum = 0;
    reload  = 0;

    for (linep = file_data; *linep != '\0'; linep = eol + 1) {

        linenum += 1;

        /* Find end-of-line */

        eol = dsproc_find_csv_delim(linep, '\n');
        if (eol) {

            /* Skip blank lines */

            if (eol == linep) {
                continue;
            }

            /* Handle carriage returns */

            if (*(eol - 1) == '\r') {
                *(eol - 1) = '\0';
            }

            /* Null terminate the line */

            *eol = '\0';
        }

        /* Trim end-of-line whitespace */

        _csv_trim_eol(linep);

        /* Skip blank lines */

        if (*linep == '\0') {
            if (eol) continue;
            else     break;
        }

        /* Check if this is a key word. */
        if (isalpha(*linep)) {

            linelen = strlen(linep);

            for (ki = 0; _ConfKeys[ki]; ++ki) {

                keylen = strlen(_ConfKeys[ki]);
                if (keylen > linelen) continue;

                chr = linep[keylen];
                if (isspace(chr) || chr == ':' || chr == '=' || chr == '\0') {
                    if (strncmp(linep, _ConfKeys[ki], keylen) == 0) {
                        break;
                    }
                }
            }

            if (_ConfKeys[ki]) {

                key     = _ConfKeys[ki];
                linep  += keylen;
                reload  = 1;

                /* skip whitespace, colons, and = signs */

                while (isspace(*linep) || *linep == ':' || *linep == '=') ++linep;
            }
            else {

                ERROR( DSPROC_LIB_NAME,
                    "Invalid keyword found on line %d in file: %s\n"
                    " -> '%s'\n",
                    linenum, full_path, linep);

                dsproc_set_status(DSPROC_ECSVCONF);

                free(file_data);
                return(0);
            }
        }
        else {

            /* skip whitespace */

            while (isspace(*linep)) ++linep;
        }

        /* Skip blank lines */

        if (*linep == '\0') {
            if (eol) continue;
            else     break;
        }

        /* Make sure we have found a keyword */

        if (!key) {

            ERROR( DSPROC_LIB_NAME,
                "Invalid configuration file: %s\n"
                " -> keyword not found before first line of text\n",
                full_path);

            dsproc_set_status(DSPROC_ECSVCONF);

            free(file_data);
            return(0);
        }

        /* Set the configuration value */

        if (strcmp(key, "FILE_NAME_PATTERNS") == 0) {

            if (reload) dsproc_clear_csv_file_name_patterns(conf);

            count = dsproc_count_csv_delims(linep, ',') + 1;

            if (buflen < count) {
                buffer = realloc(buffer, count * sizeof(char *));
                if (!buffer) goto MEMORY_ERROR;
                buflen = count;
            }

            count = dsproc_split_csv_string(linep, ',', buflen, buffer);

            if (!dsproc_add_csv_file_name_patterns(conf, count, (const char **)buffer)) {
                free(file_data);
                return(0);
            }
        }
        else if (strcmp(key, "FILE_TIME_PATTERNS") == 0) {

            if (reload) dsproc_clear_csv_file_time_patterns(conf);

            count = dsproc_count_csv_delims(linep, ',') + 1;
            if (buflen < count) {
                buffer = realloc(buffer, count * sizeof(char *));
                if (!buffer) goto MEMORY_ERROR;
                buflen = count;
            }

            count = dsproc_split_csv_string(linep, ',', buflen, buffer);

            if (!dsproc_add_csv_file_time_patterns(conf, count, (const char **)buffer)) {
                free(file_data);
                return(0);
            }
        }
        else if (strcmp(key, "DELIMITER") == 0) {

            linep = _csv_trim_quotes(linep);
            if (*linep == '\\' && *(linep+1) == 't') {
                conf->delim = '\t';
            }
            else {
                conf->delim = *linep;
            }
        }
        else if (strcmp(key, "HEADER_LINE") == 0) {

            if (reload) {
                if (conf->header_line) {
                    free((void *)conf->header_line);
                    conf->header_line = (char *)NULL;
                }
            }

            if (!dsproc_append_csv_header_line(conf, linep)) {
                free(file_data);
                return(0);
            }
        }
        else if (strcmp(key, "HEADER_LINE_TAG") == 0) {

            linep = _csv_trim_quotes(linep);

            if (conf->header_tag) free((void *)conf->header_tag);
            conf->header_tag = strdup(linep);
            if (!conf->header_tag) goto MEMORY_ERROR;
        }
        else if (strcmp(key, "HEADER_LINE_NUMBER") == 0) {

            linep = _csv_trim_quotes(linep);
            conf->header_linenum = atoi(linep);
        }
        else if (strcmp(key, "NUMBER_OF_HEADER_LINES") == 0) {

            linep = _csv_trim_quotes(linep);
            conf->header_nlines = atoi(linep);
        }
        else if (strcmp(key, "NUMBER_OF_COLUMNS") == 0) {

            linep = _csv_trim_quotes(linep);
            conf->exp_ncols = atoi(linep);
        }
        else if (strcmp(key, "TIME_COLUMN_PATTERNS") == 0) {

            if (reload) dsproc_clear_csv_time_column_patterns(conf);

            tc_name = linep;
            linep   = _csv_split_delim(linep, ':');

            if (!linep || *linep == '\0') {

                ERROR( DSPROC_LIB_NAME,
                    "Invalid time column format found on line %d in file: %s\n"
                    " -> expected format: name: pattern(s)\n",
                    linenum, full_path);

                dsproc_set_status(DSPROC_ECSVCONF);

                free(file_data);
                return(0);
            }

            count = dsproc_count_csv_delims(linep, ',') + 1;

            if (buflen < count) {
                buffer = realloc(buffer, count * sizeof(char *));
                if (!buffer) goto MEMORY_ERROR;
                buflen = count;
            }

            count = dsproc_split_csv_string(linep, ',', buflen, buffer);

            if (!dsproc_add_csv_time_column_patterns(conf, tc_name, count, (const char **)buffer)) {
                free(file_data);
                return(0);
            }
        }
        else if (strcmp(key, "SPLIT_INTERVAL") == 0) {

            linep = _csv_trim_quotes(linep);

            if (conf->split_interval) free((void *)conf->split_interval);
            conf->split_interval = strdup(linep);
            if (!conf->split_interval) goto MEMORY_ERROR;
        }
        else if (strcmp(key, "FIELD_MAP") == 0) {

            if (reload) dsproc_clear_csv_field_maps(conf);

            out_name = linep;
            linep    = _csv_split_delim(linep, ':');

            if (!linep || *linep == '\0') {

                ERROR( DSPROC_LIB_NAME,
                    "Invalid field map format found on line %d in file: %s\n"
                    " -> expected format: dod_var_name:  csv column name [, csv units [, csv missing value string]]\n",
                    linenum, full_path);

                dsproc_set_status(DSPROC_ECSVCONF);

                free(file_data);
                return(0);
            }

            count = dsproc_count_csv_delims(linep, ',') + 1;
            if (buflen < count) {
                buffer = realloc(buffer, count * sizeof(char *));
                if (!buffer) goto MEMORY_ERROR;
                buflen = count;
            }

            count = dsproc_split_csv_string(linep, ',', buflen, buffer);

            if (!dsproc_add_csv_field_map(
                conf, out_name, buffer[0], count - 1, (const char **)(buffer + 1))) {

                free(file_data);
                return(0);
            }
        }

        reload = 0;

        if (!eol) break;

    } /* end loop reading file */

    free(file_data);
    if (buffer) free(buffer);

    return(1);

MEMORY_ERROR:

    if (buffer) free(buffer);

    ERROR( DSPROC_LIB_NAME,
        "Memory allocation error loading CSV configuration file: %s/%s\n",
        path, name);

    dsproc_set_status(DSPROC_ENOMEM);

    return(0);
}

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Add an entry to the field map
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  conf      pointer to CSVConf structure to populate
 *  @param  out_name  name of the output variable, or NULL to use the column name
 *  @param  col_name  name of the CSV column
 *  @param  nargs     length of args list
 *  @param  args      list of additional arguments in the following order
 *                    (specify NULL or empty string for no value):
 *                      - units
 *                      - comma separated list of missing value strings
 *
 *  @retval  1  if successful
 *  @retval  0  if an error occurred
 */
int dsproc_add_csv_field_map(
    CSVConf     *conf,
    const char  *out_name,
    const char  *col_name,
    int          nargs,
    const char **args)
{
    CSVFieldMap  *map;
    int           length;
    CSVFieldMap  *list;
    const char   *namep;
    int           count;
    int           i;

    map = (CSVFieldMap *)NULL;

    /* Check if we already have an entry for this output name */

    if (out_name && *out_name != '\0') {
        for (i = 0; i < conf->field_nmaps; ++i) {

            map = &(conf->field_maps[i]);
            if (strcmp(map->out_name, out_name) == 0) {
                break;
            }
        }

        if (i == conf->field_nmaps) {
            map = (CSVFieldMap *)NULL;
        }
        else {
            if (map->col_name) free((void *)map->col_name);
            if (map->units)    free((void *)map->units);
            if (map->missings) free(map->missings);
            if (map->missbuf)  free(map->missbuf);

            namep = map->out_name;
            memset(map, 0, sizeof(CSVFieldMap));
            map->out_name = namep;
        }
    }

    /* Add a new field map entry if an existing one was not found */

    if (!map) {
        length = conf->field_nmaps + 1;
        list   = (CSVFieldMap *)realloc(
            conf->field_maps, length * sizeof(CSVFieldMap));

        if (!list) goto MEMORY_ERROR;

        conf->field_maps = list;

        map = &(conf->field_maps[conf->field_nmaps]);
        memset(map, 0, sizeof(CSVFieldMap));

        if (out_name && *out_name != '\0') {
            map->out_name = strdup(out_name);
            if (!map->out_name) goto MEMORY_ERROR;
        }

        conf->field_nmaps += 1;
    }

    /* Set CSV column name */

    map->col_name = strdup(col_name);

    /* Set CSV units */

    if (nargs >= 1) {
        if (args[0] && *args[0] != '\0') {
            map->units = strdup(args[0]);
        }
    }

    /* Set CSV units and missing value strings */

    if (nargs >= 2) {

        if (args[1] && *args[1] != '\0') {

            map->missbuf = strdup(args[1]);

            count = dsproc_count_csv_delims(map->missbuf, ',') + 1;

            map->missings = calloc(count, sizeof(char *));
            if (!map->missings) goto MEMORY_ERROR;

            map->nmissings = dsproc_split_csv_string(
                map->missbuf, ',', count, (char **)map->missings);
        }
    }

    return(1);

MEMORY_ERROR:

    ERROR( DSPROC_LIB_NAME,
        "Memory allocation error appending an entry to the CSV field map\n");

    dsproc_set_status(DSPROC_ENOMEM);

    return(0);
}

/**
 *  Add file name patterns to a CSVConf structure
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  conf       pointer to CSVConf structure to populate
 *  @param  npatterns  number of patterns
 *  @param  patterns   list of extended regex patterns, see man regex(7)
 *
 *  @retval  1  if successful
 *  @retval  0  if an error occurred
 */
int dsproc_add_csv_file_name_patterns(
    CSVConf     *conf,
    int          npatterns,
    const char **patterns)
{
    int          length;
    const char **list;
    int          i, j;

    length = conf->fn_npatterns + npatterns;
    list   = (const char **)realloc(
        conf->fn_patterns, length * sizeof(const char *));

    if (!list) goto MEMORY_ERROR;

    conf->fn_patterns = list;

    for (i = conf->fn_npatterns, j = 0; j < npatterns; ++i, ++j) {

        if (patterns[j] && *patterns[j] != '\0') {
            conf->fn_patterns[i] = strdup(patterns[j]);
            if (!conf->fn_patterns[i]) goto MEMORY_ERROR;
            conf->fn_npatterns += 1;
        }
    }

    return(1);

MEMORY_ERROR:

    ERROR( DSPROC_LIB_NAME,
        "Memory allocation error adding CSV file name patterns to configuration structure\n");

    dsproc_set_status(DSPROC_ENOMEM);

    return(0);
}

/**
 *  Add file time patterns to a CSVConf structure
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  conf       pointer to CSVConf structure to populate
 *  @param  npatterns  number of patterns
 *  @param  patterns   list of extended regex patterns, see man regex(7)
 *
 *  @retval  1  if successful
 *  @retval  0  if an error occurred
 */
int dsproc_add_csv_file_time_patterns(
    CSVConf     *conf,
    int          npatterns,
    const char **patterns)
{
    int          length;
    const char **list;
    int          i, j;

    /* Add the new patterns to the list */

    length = conf->ft_npatterns + npatterns;
    list   = (const char **)realloc(
        conf->ft_patterns, length * sizeof(const char *));

    if (!list) goto MEMORY_ERROR;

    conf->ft_patterns = list;

    for (i = conf->ft_npatterns, j = 0; j < npatterns; ++i, ++j) {

        if (patterns[j] && *patterns[j] != '\0') {
            conf->ft_patterns[i] = strdup(patterns[j]);
            if (!conf->ft_patterns[i]) goto MEMORY_ERROR;
            conf->ft_npatterns += 1;
        }
    }

    return(1);

MEMORY_ERROR:

    ERROR( DSPROC_LIB_NAME,
        "Memory allocation error adding CSV file time patterns to configuration structure\n");

    dsproc_set_status(DSPROC_ENOMEM);

    return(0);
}

/**
 *  Add time column patterns to a CSVConf structure
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  conf       pointer to CSVConf structure to populate
 *  @param  name       name of the time column
 *  @param  npatterns  number of patterns
 *  @param  patterns   list of time patterns
 *
 *  @retval  1  if successful
 *  @retval  0  if an error occurred
 */
int dsproc_add_csv_time_column_patterns(
    CSVConf     *conf,
    const char  *name,
    int          npatterns,
    const char **patterns)
{
    CSVTimeCol  *time_col;
    int          length;
    CSVTimeCol  *list;
    const char **strlist;
    int          i, j;

    time_col = (CSVTimeCol *)NULL;

    /* Check if we already have an entry for this time column */

    for (i = 0; i < conf->time_ncols; ++i) {

        time_col = &(conf->time_cols[i]);

        if (strcmp(time_col->name, name) == 0) {
            break;
        }
    }

    if (i == conf->time_ncols) {

        /* Add a new time column */

        length = conf->time_ncols + 1;
        list   = (CSVTimeCol *)realloc(
            conf->time_cols, length * sizeof(CSVTimeCol));

        if (!list) goto MEMORY_ERROR;

        conf->time_cols = list;

        time_col = &(conf->time_cols[conf->time_ncols]);
        memset(time_col, 0, sizeof(CSVTimeCol));

        time_col->name = strdup(name);
        if (!time_col->name) goto MEMORY_ERROR;

        conf->time_ncols   += 1;
    }

    /* Add time string patterns */

    length  = time_col->npatterns + npatterns;
    strlist = (const char **)realloc(
        time_col->patterns, length * sizeof(const char *));

    if (!strlist) goto MEMORY_ERROR;

    time_col->patterns = strlist;

    for (i = time_col->npatterns, j = 0; j < npatterns; ++i, ++j) {

        if (patterns[j] && *patterns[j] != '\0') {
            time_col->patterns[i] = strdup(patterns[j]);
            if (!time_col->patterns[i]) goto MEMORY_ERROR;
            time_col->npatterns += 1;
        }
    }

    return(1);

MEMORY_ERROR:

    ERROR( DSPROC_LIB_NAME,
        "Memory allocation error adding CSV time column patterns to configuration structure\n");

    dsproc_set_status(DSPROC_ENOMEM);

    return(0);
}

/**
 *  Append a string to the end of the header line
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  conf    pointer to CSVConf structure to populate
 *  @param  string  string to append to the header line
 *
 *  @retval  1  if successful
 *  @retval  0  if an error occurred
 */
int dsproc_append_csv_header_line(
    CSVConf    *conf,
    const char *string)
{
    int   length;
    char *header;

    if (conf->header_line) {

        length = strlen(conf->header_line) + strlen(string) + 1;
        header = (char *)realloc(
            (void *)conf->header_line, length * sizeof(char));

        if (!header) goto MEMORY_ERROR;

        conf->header_line = header;

        strcat(header, string);
    }
    else {
        conf->header_line = strdup(string);
        if (!conf->header_line) goto MEMORY_ERROR;
    }

    return(1);

MEMORY_ERROR:

    ERROR( DSPROC_LIB_NAME,
        "Memory allocation error appending a string to the CSV header line\n");

    dsproc_set_status(DSPROC_ENOMEM);

    return(0);
}

/**
 *  Clear the time column patterns in a CSVConf structure
 *
 *  @param  conf  pointer to CSVConf structure to populate
 */
void dsproc_clear_csv_field_maps(CSVConf *conf)
{
    CSVFieldMap *map;
    int          i;

    if (conf->field_maps) {

        for (i = 0; i < conf->field_nmaps; ++i) {

            map = &(conf->field_maps[i]);

            if (map->out_name) free((void *)map->out_name);
            if (map->col_name) free((void *)map->col_name);
            if (map->units)    free((void *)map->units);
            if (map->missings) free((void *)map->missings);
            if (map->missbuf)  free(map->missbuf);
        }

        free(conf->field_maps);
    }

    conf->field_nmaps = 0;
    conf->field_maps  = (CSVFieldMap *)NULL;
}

/**
 *  Clear the file name patterns in a CSVConf structure
 *
 *  @param  conf       pointer to CSVConf structure to populate
 */
void dsproc_clear_csv_file_name_patterns(CSVConf *conf)
{
    int i;

    if (conf->fn_patterns) {

        for (i = 0; i < conf->fn_npatterns; ++i) {
            if (conf->fn_patterns[i]) free((void *)conf->fn_patterns[i]);
        }

        free((void *)conf->fn_patterns);
    }

    conf->fn_npatterns = 0;
    conf->fn_patterns  = (const char **)NULL;
}

/**
 *  Clear the file name patterns in a CSVConf structure
 *
 *  @param  conf       pointer to CSVConf structure to populate
 */
void dsproc_clear_csv_file_time_patterns(CSVConf *conf)
{
    int i;

    if (conf->ft_patterns) {

        for (i = 0; i < conf->ft_npatterns; ++i) {
            if (conf->ft_patterns[i]) free((void *)conf->ft_patterns[i]);
        }

        free((void *)conf->ft_patterns);
    }

    conf->ft_npatterns = 0;
    conf->ft_patterns  = (const char **)NULL;
}

/**
 *  Clear the time column patterns in a CSVConf structure
 *
 *  @param  conf  pointer to CSVConf structure to populate
 */
void dsproc_clear_csv_time_column_patterns(CSVConf *conf)
{
    CSVTimeCol *time_col;
    int         i, j;

    if (conf->time_cols) {

        for (i = 0; i < conf->time_ncols; ++i) {

            time_col = &(conf->time_cols[i]);

            if (time_col->name) free((void *)time_col->name);

            if (time_col->patterns) {

                for (j = 0; j < time_col->npatterns; ++j) {
                    if (time_col->patterns[j]) free((void *)time_col->patterns[j]);
                }

                free(time_col->patterns);
            }
        }

        free(conf->time_cols);
    }

    conf->time_ncols = 0;
    conf->time_cols  = (CSVTimeCol *)NULL;
}

/**
 *  Configure file time and time column patterns for a CSVParser.
 *
 *  @param  conf  pointer to CSVConf structure
 *  @param  csv   pointer to CSVParser
 *
 *  @retval  1  if successful
 *  @retval  0  if an error occured
 */
int dsproc_configure_csv_parser(CSVConf *conf, CSVParser *csv)
{
    CSVTimeCol *tc;
    int         status;
    int         i;

    if (conf->delim) {
        dsproc_set_csv_delimiter(csv, conf->delim);
    }

    if (conf->ft_npatterns) {

        status = dsproc_set_csv_file_time_patterns(
            csv, conf->ft_npatterns, conf->ft_patterns);

        if (status <= 0) {
            return(0);
        }
    }

    if (conf->time_ncols) {

        dsproc_reset_csv_time_patterns(csv);

        for (i = 0; i < conf->time_ncols; ++i) {

            tc = &(conf->time_cols[i]);

            status = dsproc_set_csv_time_patterns(
                csv, tc->name, tc->npatterns, tc->patterns);

             if (status <= 0) {
                 return(0);
             }
        }
    }

    return(1);
}

/**
 *  Create a CSV2CDS Map.
 *
 *  The memory used by the returned CSV2CDS Map is dynamically allocated
 *  and must be freed using the dsproc_free_csv_to_cds_map().
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param   conf   pointer to the CSVConf structure
 *  @param   csv    pointer to the CSVParser structure
 *  @param   cds    pointer to the CDSGroup structure
 *  @param   flags  reserved for control flags
 *
 *  @retval  map   pointer to the CSV2CDS Map structure
 *  @retval  NULL  if an error occurred
 */
CSV2CDSMap *dsproc_create_csv_to_cds_map(
    CSVConf   *conf,
    CSVParser *csv,
    CDSGroup  *cds,
    int        flags)
{
    CSV2CDSMap  *maps;
    CSV2CDSMap  *map;
    CSVFieldMap *field_map;
    int          max_fields;
    char        *strp;
    int          csv_nvars;
    int          cds_nvars;
    CDSVar     **cds_vars;
    int          nmissings;
    const char **missings;
    int          fi, mi, mvi, ti;

    /* Allocate memory for the variable data map */

    max_fields = csv->nfields;

    maps = (CSV2CDSMap *)calloc(max_fields + 1, sizeof(CSV2CDSMap));
    if (!maps) goto MEMORY_ERROR;

    /* Get the array of variables in the CDSGroup */

    cds_nvars = dsproc_get_dataset_vars(
        cds, NULL, 0, &cds_vars, NULL, NULL);

    /* Count the number of columns in the CSV file,
     * skipping the time columns. */

    csv_nvars = 0;

    for (fi = 0; fi < csv->nfields; ++fi) {

        for (ti = 0; ti < csv->ntc; ++ti) {

            if (strcmp(csv->tc_names[ti], csv->headers[fi]) == 0) {
                break;
            }
        }

        if (ti != csv->ntc) continue;

        csv_nvars += 1;
    }

    /* Check for field map entries in the conf file */

    if (conf->field_maps) {

        /* Loop over field map entries */

        mi = 0;

        for (fi = 0; fi < conf->field_nmaps; ++fi) {

            map       = &(maps[mi]);
            field_map = &(conf->field_maps[fi]);

            map->csv_name = strdup(field_map->col_name);
            if (!map->csv_name) goto MEMORY_ERROR;

            /* Check if an output variable name was specified */

            if (field_map->out_name) {
                map->cds_name = strdup(field_map->out_name);
                if (!map->cds_name) goto MEMORY_ERROR;
            }
            else if (csv_nvars <= cds_nvars) {

                /* use variable name at same map index */

                map->cds_name = strdup(cds_vars[mi]->name);
                if (!map->cds_name) goto MEMORY_ERROR;
            }
            else {

                /* use column name as output variable name */

                map->cds_name = strdup(field_map->col_name);
                if (!map->cds_name) goto MEMORY_ERROR;

// BDE: Need update to append _ to front of name if it begins with a number

                /* change all non-alphanumeric characters to underbars */

                strp = (char *)map->cds_name;
                while (*strp) {
                    if (!isalnum(*strp)) *strp = '_';
                    ++strp;
                }
            }

            /* Check units were specified */

            if (field_map->units) {
                map->csv_units = strdup(field_map->units);
                if (!map->csv_units) goto MEMORY_ERROR;
            }

            /* Check if any missing values where specified */

            if (field_map->nmissings) {

                nmissings = field_map->nmissings;
                missings  = field_map->missings;

                map->csv_missings = calloc(nmissings + 1, sizeof(const char *));

                for (mvi = 0; mvi < nmissings; ++mvi) {
                    map->csv_missings[mvi] = strdup(missings[mvi]);
                    if (!map->csv_missings[mvi]) goto MEMORY_ERROR;
                }
            }

            ++mi;
        }
    }
    else {

        /* Loop over all csv columns */

        mi = 0;

        for (fi = 0; fi < csv->nfields; ++fi) {

            /* skip time columns */

            for (ti = 0; ti < csv->ntc; ++ti) {
                if (strcmp(csv->tc_names[ti], csv->headers[fi]) == 0) {
                    break;
                }
            }

            if (ti != csv->ntc) continue;

            /* skip columns with NULL header names */

            if (csv->headers[fi]  == NULL ||
                *csv->headers[fi] == '\0') {

                continue;
            }

            /* set the csv column name */

            map = &(maps[mi]);

            map->csv_name = strdup(csv->headers[fi]);
            if (!map->csv_name) goto MEMORY_ERROR;

            /* check if we should map by index */

            if (csv_nvars <= cds_nvars) {

                /* use variable name at same map index */

                map->cds_name = strdup(cds_vars[mi]->name);
                if (!map->cds_name) goto MEMORY_ERROR;
            }
            else {

                /* use column name as output variable name */

                map->cds_name = strdup(csv->headers[fi]);
                if (!map->cds_name) goto MEMORY_ERROR;

// BDE: Need update to append _ to front of name if it begins with a number

                /* change all non-alphanumeric characters to underbars */

                strp = (char *)map->cds_name;
                while (*strp) {
                    if (!isalnum(*strp)) *strp = '_';
                    ++strp;
                }
            }

            ++mi;
        }
    }

    return(maps);

MEMORY_ERROR:

    dsproc_free_csv_to_cds_map(maps);

    ERROR( DSPROC_LIB_NAME,
        "Memory allocation error creating CSV2CDSMap structure\n");

    dsproc_set_status(DSPROC_ENOMEM);

    return((CSV2CDSMap *)NULL);
}

/**
 *  Free memory used by a CSVConf structure.
 *
 *  @param  conf  pointer to CSVConf structure
 */
void dsproc_free_csv_conf(CSVConf *conf)
{
    int i;

    if (conf) {

        if (conf->proc)      free((void *)conf->proc);
        if (conf->site)      free((void *)conf->site);
        if (conf->fac)       free((void *)conf->fac);
        if (conf->name)      free((void *)conf->name);
        if (conf->level)     free((void *)conf->level);

        if (conf->file_name) free((void *)conf->file_name);
        if (conf->file_path) free((void *)conf->file_path);

        if (conf->search_paths) {
            for (i = 0; i < conf->search_npaths; ++i) {
                if (conf->search_paths[i]) free((void *)conf->search_paths[i]);
            }
            free((void *)conf->search_paths);
        }

        if (conf->dirlist)   dirlist_free(conf->dirlist);

        dsproc_clear_csv_file_name_patterns(conf);
        dsproc_clear_csv_file_time_patterns(conf);

        if (conf->header_line) free((void *)conf->header_line);
        if (conf->header_tag)  free((void *)conf->header_tag);

        dsproc_clear_csv_time_column_patterns(conf);
        dsproc_clear_csv_field_maps(conf);

        if (conf->split_interval) free((void *)conf->split_interval);

        free(conf);
    }
}

/**
 *  Free the memory used by a CSV2CDS Map.
 *
 *  @param  map   pointer to the CSV2CDS Map structure
 */
void dsproc_free_csv_to_cds_map(CSV2CDSMap *map)
{
    int fi, mvi;

    if (map) {

        for (fi = 0; map[fi].csv_name; ++fi) {

            free((void *)map[fi].cds_name);
            free((void *)map[fi].csv_name);
            free((void *)map[fi].csv_units);

            if (map[fi].csv_missings) {

                for (mvi = 0; map[fi].csv_missings[mvi]; ++mvi) {
                    free((void *)map[fi].csv_missings[mvi]);
                }

                free(map[fi].csv_missings);
            }
        }

        free(map);
    }
}

/**
 *  Initialize a new CSVConf structure.
 *
 *  The memory used by the returned structure is dynamically allocated
 *  and must be freed using dsproc_free_csv_conf().
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  name   the base name of the conf file
 *  @param  level  the data level of the conf file, or NULL
 *
 *  @retval  conf  pointer to the CSVConf structure
 *  @retval  NULL  if an error occurred
 */
CSVConf *dsproc_init_csv_conf(
    const char *name,
    const char *level)
{
    const char *proc = dsproc_get_name();
    const char *site = dsproc_get_site();
    const char *fac  = dsproc_get_facility();

    CSVConf *conf = (CSVConf *)calloc(1, sizeof(CSVConf));

    if (!conf ||
        !(conf->proc = strdup(proc)) ||
        !(conf->site = strdup(site)) ||
        !(conf->fac  = strdup(fac))  ||
        !(conf->name = strdup(name)) ||
        (level && !(conf->level = strdup(level)))) {

        if (conf) {
            dsproc_free_csv_conf(conf);
        }

        ERROR( DSPROC_LIB_NAME,
            "Memory allocation error initializing CSV configuration structure\n");

        dsproc_set_status(DSPROC_ENOMEM);

        return((CSVConf *)NULL);
    }

    return(conf);
}

/**
 *  Load the CSV Configuration file into a CVSConf structure.
 *
 *  The frist time this function is called the data_time argument must be
 *  set to 0.  This will load the main conf file containing the the file
 *  name patterns and all default configuration settings. It will also set
 *  the path to look for time varying conf files in subsequent calls to
 *  this function.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  conf       Pointer to the CSVConf structure.
 *
 *  @param  data_time  The start time of the data being processed, this should
 *                     be determined from the file name. Specify 0 to find the
 *                     main conf file containing the the file name patterns
 *                     and all default configuration settings.
 *
 *  @param  flags      Control Flags:
 *
 *                       - CSV_CHECK_DATA_CONF  check for config files under the root directory
 *                                              defined by the CONF_DATA environment variable.
 *
 *  @retval   1  if successful
 *  @retval   0  if a file was not found, or it has already been loaded
 *  @retval  -1  if error occurred
 */
int dsproc_load_csv_conf(CSVConf *conf, time_t data_time, int flags)
{
    char  path[PATH_MAX];
    char  name[PATH_MAX];
    int   status;

    status = _csv_find_conf_file(conf, data_time, flags, path, name);
    if (status <= 0) return(status);

    /* Check if this file has already been loaded */

    if ((conf->file_path && conf->file_name) &&
        (strcmp(conf->file_path, path) == 0) &&
        (strcmp(conf->file_name, name) == 0) ) {

        return(0);
    }

    /* Read in the configuration file */

    if (!_csv_load_conf_file(conf, path, name, 0)) {
        return(-1);
    }

    if (msngr_debug_level) {
        dsproc_print_csv_conf(stdout, conf);
    }

    /* Set the configuration file path and name in the structure */

    if (conf->file_path) free((void *)conf->file_path);
    conf->file_path = strdup(path);
    if (!conf->file_path) goto MEMORY_ERROR;

    if (conf->file_name) free((void *)conf->file_name);
    conf->file_name = strdup(name);
    if (!conf->file_name) goto MEMORY_ERROR;

    return(1);

MEMORY_ERROR:

    ERROR( DSPROC_LIB_NAME,
        "Memory allocation error initializing CSV configuration structure\n");

    dsproc_set_status(DSPROC_ENOMEM);

    return(-1);
}

/**
 *  Print the contents of a CSVConf structure.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  fp    pointer to the output stream
 *  @param  conf  pointer to the CSVConf structure
 *
 *  @retval  1  if successful
 *  @retval  0  if successful
 */
int dsproc_print_csv_conf(FILE *fp, CSVConf *conf)
{
    CSVTimeCol  *time_col;
    CSVFieldMap *map;
    int          i, j;

    fprintf(fp, "CSV Configuration Structure\n\n");

    if (conf->fn_patterns) {

        fprintf(fp, "FILE_NAME_PATTERNS:\n\n");

        for (i = 0; i < conf->fn_npatterns; ++i) {
            fprintf(fp, "    %s\n", conf->fn_patterns[i]);
        }

        fprintf(fp, "\n");
    }

    if (conf->ft_patterns) {

        fprintf(fp, "FILE_TIME_PATTERNS:\n\n");

        for (i = 0; i < conf->ft_npatterns; ++i) {
            fprintf(fp, "    %s\n", conf->ft_patterns[i]);
        }

        fprintf(fp, "\n");
    }

    if (conf->delim) {
        fprintf(fp, "DELIMITER:\n\n    '%c'\n\n", conf->delim);
    }

    if (conf->header_line) {
        fprintf(fp, "HEADER_LINE:\n\n    %s\n\n", conf->header_line);
    }

    if (conf->header_tag) {
        fprintf(fp, "HEADER_LINE_TAG:\n\n    %s\n\n", conf->header_tag);
    }

    if (conf->header_linenum) {
        fprintf(fp, "HEADER_LINE_NUMBER:\n\n    %d\n\n", conf->header_linenum);
    }

    if (conf->header_nlines) {
        fprintf(fp, "NUMBER_OF_HEADER_LINES:\n\n    %d\n\n", conf->header_nlines);
    }

    if (conf->exp_ncols) {
        fprintf(fp, "NUMBER_OF_COLUMNS:\n\n    %d\n\n", conf->exp_ncols);
    }

    if (conf->time_cols) {

        fprintf(fp, "TIME_COLUMN_PATTERNS:\n\n");

        for (i = 0; i < conf->time_ncols; ++i) {

            time_col = &(conf->time_cols[i]);

            fprintf(fp, "    %s:", time_col->name);

            if (time_col->npatterns) {

                fprintf(fp, " %s", time_col->patterns[0]);

                for (j = 1; j < time_col->npatterns; ++j) {
                    fprintf(fp, ", %s", time_col->patterns[j]);
                }
            }
            fprintf(fp, "\n");
        }

        fprintf(fp, "\n");
    }

    if (conf->split_interval) {
        fprintf(fp, "SPLIT_INTERVAL:\n\n    %s\n\n", conf->split_interval);
    }

    if (conf->field_maps) {

        fprintf(fp, "FIELD_MAP:\n\n");

        for (i = 0; i < conf->field_nmaps; ++i) {

            map = &(conf->field_maps[i]);

            if (map->out_name)
                fprintf(fp, "    %s:", map->out_name);
            else
                fprintf(fp, "    :");

            if (map->col_name)
                fprintf(fp, " %s", map->col_name);

            if (map->units)
                fprintf(fp, ", %s", map->units);

            if (map->nmissings == 1) {
                fprintf(fp, ", %s", map->missings[0]);
            }
            else if (map->nmissings > 1) {

                fprintf(fp, " \"%s", map->missings[0]);

                for (j = 1; j < map->nmissings; ++j) {
                    fprintf(fp, ", %s", map->missings[j]);
                }

                fprintf(fp, "\"");
            }

            fprintf(fp, "\n");
        }

        fprintf(fp, "\n");
    }

    return(1);
}
