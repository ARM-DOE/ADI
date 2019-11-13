/*******************************************************************************
*
*  COPYRIGHT (C) 2013 Battelle Memorial Institute.  All Rights Reserved.
*
********************************************************************************
*
*  Authors:
*     name:  Brian Ermold
*     phone: (509) 375-2277
*     email: brian.ermold@pnl.gov
*
********************************************************************************
*
*  REPOSITORY INFORMATION:
*    $Revision: 71740 $
*    $Author: ermold $
*    $Date: 2016-06-30 19:55:27 +0000 (Thu, 30 Jun 2016) $
*    $State:$
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file dsproc_csv_parser.c
 *  CSV File Parsing Functions and Utilities.
 */

#include "dsproc3.h"

/*******************************************************************************
 *  Private Data and Functions
 */
/** @privatesection */

/**
 *  Private: Create the array of time column indexes in a CSVParser structure.
 *
 *  This function must be called after the names of the time columns have
 *  been set, and the header line has been parsed.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  csv  pointer to the CSVParser structure
 *
 *  @retval  1  if successful
 *  @retval  0  if an error occurred
 */
static int _csv_create_tc_index(CSVParser *csv)
{
    int ti;
    int fi;

    /* Make sure the names of the time columns have been defined */

    if (!csv->ntc || !csv->tc_names || !csv->tc_patterns) {

        ERROR( DSPROC_LIB_NAME,
            "Could not create array of time column indexes\n"
            " -> time column names have not been defined\n");

        dsproc_set_status(DSPROC_ECSVPARSER);

        return(0);
    }

    /* Make sure the column headers have been defined */

    if (!csv->headers) {

        ERROR( DSPROC_LIB_NAME,
            "Could not create array of time column indexes\n"
            " -> column headers have not been defined\n");

        dsproc_set_status(DSPROC_ECSVPARSER);

        return(0);
    }

    /* Allocate memory for the array of time column indexes */

    if (csv->tc_index) free(csv->tc_index);

    csv->tc_index = calloc(csv->ntc, sizeof(int));
    if (!csv->tc_index) {

        ERROR( DSPROC_LIB_NAME,
            "Could not create array of time column indexes\n"
            " -> memory allocation error\n");

        dsproc_set_status(DSPROC_ENOMEM);

        return(0);
    }

    /* Get time column indexes */

    for (ti = 0; ti < csv->ntc; ++ti) {

        for (fi = 0; fi < csv->nfields; ++fi) {

            if (strcmp(csv->tc_names[ti], csv->headers[fi]) == 0) {
                csv->tc_index[ti] = fi;
                break;
            }
        }

        if (fi == csv->nfields) {

            ERROR( DSPROC_LIB_NAME,
                "Could not create array of time column indexes\n"
                " -> time column '%s' not found in header fields\n",
                csv->tc_names[ti]);

            dsproc_set_status(DSPROC_ECSVPARSER);

            return(0);
        }
    }

    return(1);
}

/**
 *  Parse the time columns of a record in a CSV file and set the record time.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  csv           pointer to the CSVParser structure
 *  @param  record_index  record index
 *
 *  @retval  1  if successful
 *  @retval  0  if invalid time format
 *  @retval -1  if an error occurred
 */
int _csv_parse_record_time(
    CSVParser *csv,
    int        record_index)
{
    char       *time_string;
    int         status;
    int         tci, fi;
    RETimeList *tc_patterns;
    RETimeRes   match;
    RETimeRes   result;

    int         used_file_date;
    timeval_t   rec_time;
    timeval_t   prev_time;
    int         tro_interval;
    double      delta_t;
    struct tm   gmt;


    /* Get the time column indexes */

    if (!csv->tc_index) {
        if (!_csv_create_tc_index(csv)) {
            return(-1);
        }
    }

    /* Parse time strings and merge results */

    for (tci = 0; tci < csv->ntc; ++tci) {

        fi          = csv->tc_index[tci];
        tc_patterns = csv->tc_patterns[tci];

        /* Make sure this is a valid field index */

        if (fi < 0 || fi > csv->nfields) {

            ERROR( DSPROC_LIB_NAME,
                "Time column index '%d' is out of range [0, %d].\n",
                fi, csv->nfields);

            dsproc_set_status(DSPROC_ECSVPARSER);

            return(-1);
        }

        time_string = csv->values[fi][record_index];

        /* Parse time string */

        status = retime_list_execute(tc_patterns, time_string, &match);

        if (status < 0) {

            ERROR( DSPROC_LIB_NAME,
                "Time string pattern match failed for record %d.\n",
                record_index + 1);

            dsproc_set_status(DSPROC_ECSVPARSER);

            return(-1);
        }
        else if (status == 0) {

            DSPROC_BAD_RECORD_WARNING(csv->file_name, csv->nrecs,
                "Record time format '%s' does not match '%s'\n",
                time_string,
                tc_patterns->retimes[tc_patterns->npatterns - 1]->tspattern);

            return(0);
        }

        /* Merge results */

        if (tci == 0) {
            result = match;
        }
        else {
            if (match.year     != -1) result.year     = match.year;
            if (match.month    != -1) result.month    = match.month;
            if (match.mday     != -1) result.mday     = match.mday;
            if (match.hour     != -1) result.hour     = match.hour;
            if (match.min      != -1) result.min      = match.min;
            if (match.sec      != -1) result.sec      = match.sec;
            if (match.usec     != -1) result.usec     = match.usec;
            if (match.century  != -1) result.century  = match.century;
            if (match.yy       != -1) result.yy       = match.yy;
            if (match.yday     != -1) result.yday     = match.yday;
            if (match.secs1970 != -1) result.secs1970 = match.secs1970;

            if (match.offset.tv_sec != 0)
                result.offset.tv_sec = match.offset.tv_sec;

            if (match.offset.tv_usec != 0)
                result.offset.tv_usec = match.offset.tv_usec;
        }
    }

    /* Check if the time is already in seconds since 1970 */

    if (result.secs1970 != -1) {

        rec_time         = retime_get_timeval(&result);
        rec_time.tv_sec += csv->time_offset;
        rec_time.tv_sec += csv->tro_offset;

        csv->tvs[record_index] = rec_time;

        return(1);
    }

    /* Check if a base time was set by the user */

    if (csv->base_tm.tm_year) {

        result.year = csv->base_tm.tm_year + 1900;

        if (csv->base_tm.tm_mon)  result.month = csv->base_tm.tm_mon + 1;
        if (csv->base_tm.tm_mday) result.mday  = csv->base_tm.tm_mday;
        if (csv->base_tm.tm_hour) result.hour  = csv->base_tm.tm_hour;
        if (csv->base_tm.tm_min)  result.min   = csv->base_tm.tm_min;
        if (csv->base_tm.tm_sec)  result.sec   = csv->base_tm.tm_sec;
    }

    /* Check if we need to use the date from the file name */

    used_file_date = 0;

    if (csv->ft_patterns) {

        if (!csv->ft_result) {

            csv->ft_result = calloc(1, sizeof(RETimeRes));
            if (!csv->ft_result) {

                ERROR( DSPROC_LIB_NAME,
                    "Memory allocation error creating file name RETimeRes structure\n");

                dsproc_set_status(DSPROC_ECSVPARSER);

                return(-1);
            }

            status = dsproc_get_csv_file_name_time(csv, csv->file_name, csv->ft_result);
            if (status < 0) return(-1);
        }

        if (result.year == -1) {

            if (csv->ft_result->year != -1) {

                result.year = csv->ft_result->year;
                used_file_date = 1;
            }
            else {

                ERROR( DSPROC_LIB_NAME,
                    "Could not determine record time\n"
                    " -> year not found in record or file name time patterns\n");

                dsproc_set_status(DSPROC_ECSVPARSER);

                return(-1);
            }
        }

        if (result.month == -1) {

            if (result.yday != -1) {

                yday_to_mday(result.yday,
                    &(result.year), &(result.month), &(result.mday));
            }
            else if (csv->ft_result->month != -1) {

                result.month = csv->ft_result->month;
                used_file_date = 2;
            }
            else if (csv->ft_result->yday != -1) {

                yday_to_mday(csv->ft_result->yday,
                    &(result.year), &(result.month), &(result.mday));

                used_file_date = 3;
            }
        }

        if (result.mday == -1) {

            if (csv->ft_result->mday != -1) {
                result.mday = csv->ft_result->mday;
                used_file_date = 3;
            }
        }
    }
    else {

        /* Verify that the year was found */

        if (result.year == -1) {

            ERROR( DSPROC_LIB_NAME,
                "Could not determine record time\n"
                " -> year not found in record time pattern\n");

            dsproc_set_status(DSPROC_ECSVPARSER);

            return(-1);
        }
    }

    rec_time         = retime_get_timeval(&result);
    rec_time.tv_sec += csv->time_offset;
    rec_time.tv_sec += csv->tro_offset;

    /* Check for time rollovers if the file date was used */

    if (used_file_date && record_index > 0) {

        prev_time = csv->tvs[record_index - 1];

        if (TV_LT(rec_time, prev_time)) {

            switch (used_file_date) {

                case 1: /* yearly  */

                    gmtime_r(&(prev_time.tv_sec), &gmt);

                    if (IS_LEAP_YEAR(gmt.tm_year + 1900)) {
                        tro_interval = 366 * 86400;
                    }
                    else {
                        tro_interval = 365 * 86400;
                    }

                    if (!csv->tro_threshold) {
                        csv->tro_threshold = 86400;
                    }

                    break;

                case 2: /* monthly */

                    gmtime_r(&(prev_time.tv_sec), &gmt);

                    tro_interval = days_in_month(gmt.tm_year + 1900, gmt.tm_mon + 1)
                                 * 86400;

                    if (!csv->tro_threshold) {
                        csv->tro_threshold = 43200;
                    }

                    break;

                default: /* daily   */

                    tro_interval = 86400;

                    if (!csv->tro_threshold) {
                        csv->tro_threshold = 3600;
                    }
            }

            delta_t = (TV_DOUBLE(rec_time) + tro_interval) - TV_DOUBLE(prev_time);

            if ((delta_t > 0) && (delta_t < csv->tro_threshold)) {

                rec_time.tv_sec += tro_interval;
                csv->tro_offset += tro_interval;
            }
        }
    }

    csv->tvs[record_index] = rec_time;

    return(1);
}

/**
 *  Private: Allocate memory for the header and field pointers.
 *
 *  @param  csv      pointer to the CSVParser structure
 *  @param  nfields  number of fields
 *  @param  nrecs    number of records
 *
 *  @retval  1  if successful
 *  @retval  0  if a memory allocation error occurs
 */
static int _csv_realloc_data(CSVParser *csv, int nfields, int nrecs)
{
    int    prev_nfields = csv->nfields_alloced;
    int    prev_nrecs   = csv->nrecs_alloced;
    size_t nelems;
    int    fi;

    /* Increase the maximum number of records if necessary */

    if (nrecs > prev_nrecs) {

        nelems = nrecs - prev_nrecs;

        /* realloc memory for record values */

        for (fi = 0; fi < prev_nfields; ++fi) {

            csv->values[fi] = (char **)realloc(csv->values[fi], nrecs * sizeof(char *));
            if (!csv->values[fi]) return(0);

            memset(csv->values[fi] + prev_nrecs, 0, nelems * sizeof(char *));
        }

        /* realloc memory for time vector if applicable */

        if (csv->ntc) {

            csv->tvs = (timeval_t *)realloc(csv->tvs, nrecs * sizeof(timeval_t));
            if (!csv->tvs) return(0);

            memset(csv->tvs + prev_nrecs, 0, nelems * sizeof(timeval_t));
        }

        csv->nrecs_alloced = nrecs;
    }

    /* Increase the maximum number of fields if necessary */

    if (nfields > prev_nfields) {

        nelems = nfields - prev_nfields;

        /* realloc memory for record buffer */

        csv->rec_buff = (char **)realloc(csv->rec_buff, nfields * sizeof(char *));
        if (!csv->rec_buff) return(0);

        /* realloc memory for header pointers */

        csv->headers = (char **)realloc(csv->headers, nfields * sizeof(char *));
        if (!csv->headers) return(0);

        memset(csv->headers + prev_nfields, 0, nelems * sizeof(char *));

        /* realloc memory for free_header flags */

        csv->free_header = (int *)realloc(csv->free_header, nfields * sizeof(int));
        if (!csv->free_header) return(0);

        memset(csv->free_header + prev_nfields, 0, nelems * sizeof(int));

        /* realloc memory for additional field columns and values */

        csv->values = (char ***)realloc(csv->values, nfields * sizeof(char **));
        if (!csv->values) return(0);

        for (fi = prev_nfields; fi < nfields; ++fi) {

            csv->values[fi] = (char **)calloc(csv->nrecs_alloced, sizeof(char *));
            if (!csv->values[fi]) return(0);
        }

        csv->nfields_alloced = nfields;
    }

    return(1);
}

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Free all memory used by a CSVParser structure.
 *
 *  @param  csv  pointer to the CSVParser structure
 */
void dsproc_free_csv_parser(CSVParser *csv)
{
    int i;

    if (csv) {

        if (csv->file_name) free(csv->file_name);
        if (csv->file_path) free(csv->file_path);
        if (csv->file_data) free(csv->file_data);
        if (csv->lines)     free(csv->lines);

        if (csv->headers && csv->free_header) {
            for (i = 0; i < csv->nfields_alloced; ++i) {
                if (csv->free_header[i]) free(csv->headers[i]);
            }
        }

        if (csv->header_data) free(csv->header_data);
        if (csv->headers)     free(csv->headers);
        if (csv->free_header) free(csv->free_header);

        if (csv->rec_buff)    free(csv->rec_buff);

        if (csv->values) {
            for (i = 0; i < csv->nfields_alloced; ++i) {
                free(csv->values[i]);
            }
            free(csv->values);
        }

        if (csv->ft_patterns) {
            retime_list_free(csv->ft_patterns);
        }

        if (csv->ft_result) {
            free(csv->ft_result);
        }

        if (csv->tc_names) {
            for (i = 0; i < csv->ntc; ++i) {
                free(csv->tc_names[i]);
            }
            free(csv->tc_names);
        }

        if (csv->tc_patterns) {
            for (i = 0; i < csv->ntc; ++i) {
                retime_list_free(csv->tc_patterns[i]);
            }
            free(csv->tc_patterns);
        }

        if (csv->tc_index) free(csv->tc_index);

        if (csv->tvs) free(csv->tvs);

        free(csv);
    }
}

/**
 *  Get the time from a CSV file name.
 *
 *  This function must be called after the CSVParser structure has been
 *  initialized using the dsproc_init_csv_parser() function and the file name
 *  time string patterns have been set using dsproc_set_csv_file_time_patterns().
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  csv     pointer to the CSVParser structure
 *  @param  name    CSV file name
 *  @param  result  if not NULL the complete result from the pattern match
 *                  will be stored in this RETimeRes structure.
 *
 *  @retval  time  time in seconds since 1970
 *  @retval  -1    if an error occurred
 */
time_t dsproc_get_csv_file_name_time(
    CSVParser  *csv,
    const char *name,
    RETimeRes  *result)
{
    RETimeRes  result_buffer;
    RETimeRes *resultp;
    int        status;
    time_t     secs1970;

    resultp = (result) ? result : &result_buffer;

    if (!csv->ft_patterns) {

        ERROR( DSPROC_LIB_NAME,
            "Could not get time from CSV file name: %s\n"
            " -> no time string patterns have been defined\n",
            name);

        dsproc_set_status(DSPROC_ECSVPARSER);

        return(-1);
    }

    status = retime_list_execute(csv->ft_patterns, name, resultp);

    if (status < 0) {

        ERROR( DSPROC_LIB_NAME,
            "Could not get time from CSV file name: %s\n"
            " -> time string pattern matching error occured\n",
            name);

        dsproc_set_status(DSPROC_ECSVPARSER);

        return(-1);
    }
    else if (status == 0) {

        ERROR( DSPROC_LIB_NAME,
            "Could not get time from CSV file name: %s\n"
            " -> file name format does not match time string pattern: '%s'\n",
            name,
            csv->ft_patterns->retimes[csv->ft_patterns->npatterns - 1]->tspattern);

        dsproc_set_status(DSPROC_ECSVPARSER);

        return(-1);
    }

    secs1970 = retime_get_secs1970(resultp);

    if (secs1970 == -1) {

        ERROR( DSPROC_LIB_NAME,
            "Could not get time from CSV file name: %s\n"
            " -> year not found in time string pattern\n",
            name);

        dsproc_set_status(DSPROC_ECSVPARSER);

        return(-1);
    }

    return(secs1970);
}

/**
 *  Get the array of string pointers to the column headers in a CSV file.
 *
 *  The memory used by the returned array of strings belongs to the
 *  CSVParser structure and must not be freed by the calling process.
 *
 *  @param  csv      pointer to the CSVParser structure
 *  @param  nfields  output: number of header fields.
 *
 *  @retval  headers  array of string pointers to the column headers
 *  @retval  NULL     if the column headers have not been parsed
 */
char **dsproc_get_csv_column_headers(CSVParser *csv, int *nfields)
{
    if (nfields) *nfields = csv->nfields;
    if (csv->nfields > 0) return(csv->headers);
    return((char **)NULL);
}

/**
 *  Get the array of string pointers for a field in a CSV file.
 *
 *  The memory used by the returned array of strings belongs to the
 *  CSVParser structure and must not be freed by the calling process.
 *
 *  @param  csv   pointer to the CSVParser structure
 *  @param  name  column name as specified in the header.
 *
 *  @retval  vals  array of string pointers to the column values
 *  @retval  NULL  if the column name was not found
 */
char **dsproc_get_csv_field_strvals(CSVParser *csv, const char *name)
{
    int fi;

    for (fi = 0; fi < csv->nfields; ++fi) {

        if (strcmp(csv->headers[fi], name) == 0) {
            return((char **)csv->values[fi]);
        }
    }

    return((char **)NULL);
}

/**
 *  Get the array of record times after parsing a CSV file.
 *
 *  The memory used by the returned array of timevals belongs to the
 *  CSVParser structure and must not be freed by the calling process.
 *
 *  @param  csv    pointer to the CSVParser structure
 *  @param  nrecs  output: the number of records found in the file
 *
 *  @retval  times  array of timevals in seconds since 1970
 *  @retval  NULL   if no records have been parsed from the file
 */
timeval_t *dsproc_get_csv_timevals(CSVParser *csv, int *nrecs)
{
    if (nrecs) *nrecs = csv->nrecs;
    if (csv->nrecs) return(csv->tvs);
    return((timeval_t *)NULL);
}

/**
 *  Get the next line from the file loaded into the CSVParser structure.
 *
 *  The memory used by the returned line belongs to the CSVParser structure
 *  and must not be freed by the calling process.
 *
 *  @param  csv   pointer to the CSVParser structure
 *
 *  @retval  line  the next line in the file loaded into memory
 *  @retval  NULL  if the end of file was reached
 */
char *dsproc_get_next_csv_line(CSVParser *csv)
{
    if (csv->linenum >= csv->nlines) {
        return((char *)NULL);
    }

    csv->linep = csv->lines[csv->linenum];
    csv->linenum += 1;

    return(csv->linep);
}

/**
 *  Initialize a CSVParser structure.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  csv   pointer to the CSV Data structure to initialize,
 *                or NULL to create a new CSVParser structure.
 *
 *  @retval  csv   pointer to the CSVParser structure
 *  @retval  NULL  if an error occurred
 */
CSVParser *dsproc_init_csv_parser(CSVParser *csv)
{
    if (csv) {

        /* Initialize an existing CSVParser structure */

        if (csv->file_path) {
            free(csv->file_path);
            csv->file_path  = (char *)NULL;
        }

        if (csv->file_name) {
            free(csv->file_name);
            csv->file_name  = (char *)NULL;
        }

        csv->nlines     = 0;
        csv->linep      = (char *)NULL;
        csv->linenum    = 0;

        csv->nfields    = 0;
        csv->nrecs      = 0;

        if (csv->tc_index) {
            free(csv->tc_index);
            csv->tc_index = (int *)NULL;
        }

        csv->tro_offset = 0;
    }
    else {

        /* Create a new CSVParser structure */

        csv = (CSVParser *)calloc(1, sizeof(CSVParser));
        if (!csv) {

            ERROR( DSPROC_LIB_NAME,
                "Memory allocation error loading CSV file: %s\n",
                csv->file_name);

            dsproc_set_status(DSPROC_ENOMEM);

            return((CSVParser *)NULL);
        }

        csv->delim         = ',';
        csv->nlines_guess  = 4096;
        csv->nfields_guess = 32;
    }

    return(csv);
}

/**
 *  Load a CSV data file into a CSVParser structure.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  csv   pointer to the CSVParser structure created by dsproc_init_csv_parser()
 *  @param  path  path to the location of the file
 *  @param  name  the name of the file
 *
 *  @retval  nlines  number of lines read from the file
 *  @retval  -1      if an error occurred
 */
int dsproc_load_csv_file(CSVParser *csv, const char *path, const char *name)
{
    char     full_path[PATH_MAX];
    size_t   nread;
    size_t   nbytes;
    FILE    *fp;
    char    *chrp;
    int      nlines;
    int      li;

    /* Initialize the CSVParser structure if necessary */

    if (csv->nlines != 0) {
        if (!dsproc_init_csv_parser(csv)) {
            return(-1);
        }
    }

    /* Initialize the array of CSVParser line pointers if necessary */

    if (csv->nlines_alloced == 0) {

        nlines = csv->nlines_guess;

        csv->lines = (char **)malloc(nlines * sizeof(char *));

        if (!csv->lines) {

            ERROR( DSPROC_LIB_NAME,
                "Memory allocation error loading CSV file: %s\n",
                csv->file_name);

            dsproc_set_status(DSPROC_ENOMEM);

            return(-1);
        }

        csv->nlines_alloced = nlines;
    }

    /* Set the file name and path in the CSVParser structure */

    if (!(csv->file_path = strdup(path)) ||
        !(csv->file_name = strdup(name))) {

        ERROR( DSPROC_LIB_NAME,
            "Memory allocation error loading CSV file: %s\n",
            csv->file_name);

        dsproc_set_status(DSPROC_ENOMEM);

        return(-1);
    }

    snprintf(full_path, PATH_MAX, "%s/%s", path, name);

    if (csv->ft_result) {
        free(csv->ft_result);
        csv->ft_result = (RETimeRes *)NULL;
    }

    /* Get the CSV file status */

    if (stat(full_path, &csv->file_stats) < 0) {

        ERROR( DSPROC_LIB_NAME,
            "Could not get file status for: %s\n"
            " -> %s\n", full_path, strerror(errno));

        dsproc_set_status(DSPROC_EFILESTATS);

        return(-1);
    }

    /* Read in the entire CSV file */

    nbytes = csv->file_stats.st_size;
    if (nbytes == 0) return(0);

    if (nbytes > (size_t)csv->nbytes_alloced) {

        csv->file_data = (char *)realloc(csv->file_data, (nbytes + 1) * sizeof(char));
        if (!csv->file_data) {

            ERROR( DSPROC_LIB_NAME,
                "Memory allocation error loading CSV file: %s\n",
                csv->file_name);

            dsproc_set_status(DSPROC_ENOMEM);

            return(-1);
        }

        csv->nbytes_alloced = nbytes;
    }

    fp = fopen(full_path, "r");
    if (!fp) {

        ERROR( DSPROC_LIB_NAME,
            "Could not open file: %s\n"
            " -> %s\n", csv->file_name, strerror(errno));

        dsproc_set_status(DSPROC_EFILEOPEN);

        return(-1);
    }

    nread = fread(csv->file_data, 1, nbytes, fp);
    fclose(fp);

    if (nread != nbytes) {

        ERROR( DSPROC_LIB_NAME,
            "Could not read CSV file: %s\n"
            " -> %s\n", csv->file_name, strerror(errno));

        dsproc_set_status(DSPROC_EFILEREAD);

        return(-1);
    }

    csv->file_data[nbytes] = '\0';

    /* Set the line pointers */

    chrp = csv->file_data;
    csv->lines[0] = chrp;

    li = 1;
    while ((chrp = dsproc_find_csv_delim(chrp, '\n'))) {

        if (chrp != csv->file_data) {

            /* Handle carriage returns */

            if (*(chrp - 1) == '\r') {
                *(chrp - 1) = '\0';
            }
        }

        *chrp++ = '\0';
        if (*chrp == '\0') break; // end of file

        if (li >= csv->nlines_alloced) {

            /* Estimate the total number of lines we will need:
             *
             *  = (sizeof_file / nbytes_read) * nlines_read
             */

            nlines = (nbytes / (chrp - csv->file_data)) * li
                   + csv->nlines_guess;

            csv->lines   = (char **)realloc(csv->lines, nlines * sizeof(char *));

            if (!csv->lines) {

                ERROR( DSPROC_LIB_NAME,
                    "Memory allocation error loading CSV file: %s\n",
                    csv->file_name);

                dsproc_set_status(DSPROC_ENOMEM);

                return(-1);
            }

            csv->nlines_alloced = nlines;
        }

        csv->lines[li++] = chrp;
    }

    csv->nlines = li;

    return(csv->nlines);
}

/**
 *  Parse a header line.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  csv    pointer to the CSVParser structure
 *  @param  linep  pointer to the line to parse,
 *                 or NULL to parse the current line.
 *
 *  @retval nfields  number of header fields
 *  @retval -1       if a memory allocation error occurs
 */
int dsproc_parse_csv_header(CSVParser *csv, const char *linep)
{
    char delim = csv->delim;
    int  nfields;
    int  count;
    int  fi;

    /* Get pointer to the current line if linep is NULL */

    if (!linep) {

        if (csv->linenum == 0) {
            csv->linenum += 1;
        }

        linep = csv->lines[csv->linenum - 1];
    }

    /* Count the number of header fields */

    count = dsproc_count_csv_delims(linep, delim) + 1;

    /* Make a copy of the header line */

    if (csv->header_data) free(csv->header_data);

    csv->header_data = strdup(linep);

    if (!csv->header_data) {

        ERROR( DSPROC_LIB_NAME,
            "Memory allocation error parsing CSV header for file: %s\n",
            csv->file_name);

        dsproc_set_status(DSPROC_ENOMEM);

        return(-1);
    }

    /* Allocate memory for the header and field pointers */

    if (!_csv_realloc_data(csv, count, csv->nlines)) {

        ERROR( DSPROC_LIB_NAME,
            "Memory allocation error parsing CSV header for file: %s\n",
            csv->file_name);

        dsproc_set_status(DSPROC_ENOMEM);

        return(-1);
    }

    /* Free any header strings that have been added manually */

    if (csv->headers && csv->free_header) {
        for (fi = 0; fi < csv->nfields_alloced; ++fi) {
            if (csv->free_header[fi]) {
                free(csv->headers[fi]);
                csv->headers[fi]     = (char *)NULL;
                csv->free_header[fi] = 0;
            }
        }
    }

    /* Parse the header line */

    nfields = dsproc_split_csv_string(csv->header_data, delim, count, csv->headers);

    if (nfields != count) {

        if (nfields == 0) {
            /* input line was zero length string or all white-space */
            return(0);
        }

        /* This should never happen */

        ERROR( DSPROC_LIB_NAME,
            "Unknown error parsing CSV header line for file: %s\n",
            csv->file_name);

        dsproc_set_status(DSPROC_ECSVPARSER);

        return(-1);
    }

    csv->nfields = nfields;

    return(nfields);
}

/**
 *  Parse a record line.
 *
 *  If the number of values found on the current line does not match
 *  the number of header fields, a warning message will be generated.
 *
 *  The records values in the internal CSVParser structure point to the
 *  substrings in the input string and have been null terminated. All
 *  leading and trailing spaces, and surrounding quotes have also been
 *  removed from the substrings.
 *
 *  The input string must not be altered or freed after calling this function
 *  until the record values are no longer needed by the calling process.
 *
 *  @param   csv    pointer to the CSVParser structure
 *  @param   linep  pointer to the line to parse, or NULL to parse
 *                  from the beginning of the current line.
 *  @param   flags  reserved for control flags, set to 0 to maintain
 *                  backward compatibility with future updates.
 *
 *  @retval  1  if successful
 *  @retval  0  if the record time has an invalid format, or the number of values
 *              found on the line does not match the number of header fields
 *  @retval -1  if an error occurred
 */
int dsproc_parse_csv_record(CSVParser *csv, char *linep, int flags)
{
    char     delim = csv->delim;
    int      nfields;
    int      fi;
    int      status;

    /* prevent unused parameter warning */

    flags = flags;

    /* Get pointer to the current line if linep is NULL */

    if (!linep) {

        if (csv->linenum == 0) {
            csv->linenum += 1;
        }

        linep = csv->lines[csv->linenum - 1];
    }

    /* Check size of csv->nrecs_alloced */

    if (csv->nrecs == csv->nrecs_alloced) {

        if (!_csv_realloc_data(csv, csv->nfields, csv->nrecs * 1.5)) {

            ERROR( DSPROC_LIB_NAME,
                "Memory allocation error parsing CSV record #%d for file: %s\n",
                csv->nrecs + 1, csv->file_name);

            dsproc_set_status(DSPROC_ENOMEM);

            return(-1);
        }
    }

    /* Parse the record line */

    nfields = dsproc_split_csv_string(linep, delim, csv->nfields, csv->rec_buff);

    /* Make sure the number of fields in the record
     * matches the number of header fields. */

    if (nfields != csv->nfields) {

        DSPROC_BAD_RECORD_WARNING(csv->file_name, csv->nrecs,
            "Expected %d values but found %d\n",
            csv->nfields, nfields);

        return(0);
    }

    /* Set the record value pointers */

    for (fi = 0; fi < nfields; ++fi) {
        csv->values[fi][csv->nrecs] = csv->rec_buff[fi];
    }

    /* Get the record time if a time format string was specified. */

    if (csv->ntc) {

        status = _csv_parse_record_time(csv, csv->nrecs);
        if (status  < 0) return(-1);
        if (status == 0) return(0);
    }

    csv->nrecs += 1;

    return(1);
}

/**
 *  Set the column delimiter.
 *
 *  @param  csv    pointer to the CSVParser structure
 *  @param  delim  column delimiter
 *
 */
void dsproc_set_csv_delimiter(CSVParser *csv, char delim)
{
    csv->delim = delim;
}

/**
 *  Set or change a column name in the header.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  csv    pointer to the CSVParser structure
 *  @param  index  column index
 *  @param  name   column name
 *
 *  @retval  1  if successful
 *  @retval  0  if a memory allocation error occurs
 */
int dsproc_set_csv_column_name(CSVParser *csv, int index, const char *name)
{
    int max_fields;

    /* Allocate/reallocate memory if necessary */

    max_fields = csv->nfields_guess;
    while (max_fields <= index) max_fields += csv->nfields_guess;

    if (!_csv_realloc_data(csv, max_fields, csv->nlines)) {

        ERROR( DSPROC_LIB_NAME,
            "Memory allocation error adding CSV column name:\n"
            " -> file name:    %s\n"
            " -> column index: %d\n"
            " -> column name:  %s\n",
            csv->file_name, index, name);

        dsproc_set_status(DSPROC_ENOMEM);

        return(0);
    }

    if (csv->headers[index] && csv->free_header[index]) {
        free(csv->headers[index]);
    }

    csv->headers[index] = strdup(name);
    if (!csv->headers[index]) {

        ERROR( DSPROC_LIB_NAME,
            "Memory allocation error adding CSV column name:\n"
            " -> file name:    %s\n"
            " -> column index: %d\n"
            " -> column name:  %s\n",
            csv->file_name, index, name);

        dsproc_set_status(DSPROC_ENOMEM);

        return(0);
    }

    csv->free_header[index] = 1;

    if (csv->nfields < index + 1) {
        csv->nfields = index + 1;
    }

    return(1);
}

/**
 *  Set the base time to use for record times.
 *
 *  This option is used when the records times are relative to a
 *  base time.
 *
 *  @param  csv        pointer to the CSVParser structure
 *  @param  base_time  base time in seconds since 1970
 *
 *  @retval  1  if successful
 *  @retval  0  if an error occurred
 */
int dsproc_set_csv_base_time(CSVParser *csv, time_t base_time)
{
    memset(&(csv->base_tm), 0, sizeof(struct tm));

    if (!gmtime_r(&base_time, &(csv->base_tm))) {

        ERROR( DSPROC_LIB_NAME,
            "Could not set base time for CSV data records\n"
            " -> gmtime error: %s\n",
            strerror(errno));

        dsproc_set_status(DSPROC_ECSVPARSER);
        return(0);
    }

    return(1);
}

/**
 *  Specify the pattern to use to parse the date/time from the file name.
 *
 *  The time string patterns can containing a mixture of regex and time format
 *  codes similar to the strptime function. The time format codes recognized by
 *  this function begin with a % and are followed by one of the following characters:
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
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  csv        pointer to the CSVParser structure
 *  @param  npatterns  number of possible patterns to check
 *  @param  patterns   list of possible time string patterns
 *
 *  @retval  1  if successful
 *  @retval  0  if an error occurred
 */
int dsproc_set_csv_file_time_patterns(
    CSVParser   *csv,
    int          npatterns,
    const char **patterns)
{
    if (csv->ft_patterns) {
        retime_list_free(csv->ft_patterns);
    }

    csv->ft_patterns = retime_list_compile(npatterns, patterns, 0);

    if (!csv->ft_patterns) {

        ERROR( DSPROC_LIB_NAME,
            "Could not compile CSV file time pattern(s)\n");

        dsproc_set_status(DSPROC_ECSVPARSER);

        return(0);
    }

    return(1);
}

/**
 *  Set the time offset to apply to record times.
 *
 *  @param  csv          pointer to the CSVParser structure
 *  @param  time_offset  base time in seconds since 1970
 */
void dsproc_set_csv_time_offset(CSVParser *csv, time_t time_offset)
{
    csv->time_offset = time_offset;
}

/**
 *  Specify the pattern to use to parse a date/time column.
 *
 *  The time string patterns can containing a mixture of regex and time format
 *  codes similar to the strptime function. The time format codes recognized by
 *  this function begin with a % and are followed by one of the following characters:
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
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  csv        pointer to the CSVParser structure
 *  @param  name       name of the time column
 *  @param  npatterns  number of possible patterns to check
 *  @param  patterns   list of possible time string patterns
 *
 *  @retval  1  if successful
 *  @retval  0  if an error occurred
 */
int dsproc_set_csv_time_patterns(
    CSVParser   *csv,
    const char  *name,
    int          npatterns,
    const char **patterns)
{
    char       **tc_names;
    RETimeList **tc_patterns;
    int          ncols;
    int          tci;

    ncols = csv->ntc;

    /* Clear the array of time column indexes if it has already been created */

    if (csv->tc_index) {
        free(csv->tc_index);
        csv->tc_index = (int *)NULL;
    }

    /* Check if an entry already exists for this time column */

    for (tci = 0; tci < ncols; ++tci) {
        if (strcmp(csv->tc_names[tci], name) == 0) {
            break;
        }
    }

    if (tci == ncols) {

        /* Allocate memory for a new entry */

        tc_names = (char **)realloc(csv->tc_names, (ncols + 1) * sizeof(char *));
        if (!tc_names) goto MEMORY_ERROR;

        csv->tc_names = tc_names;

        tc_patterns = (RETimeList **)realloc(csv->tc_patterns, (ncols + 1) * sizeof(RETimeList *));
        if (!tc_patterns) goto MEMORY_ERROR;

        csv->tc_patterns = tc_patterns;

        /* Set the time column name */

        csv->tc_names[tci] = strdup(name);
        if (!csv->tc_names[tci]) goto MEMORY_ERROR;

        csv->ntc += 1;
    }
    else {
        retime_list_free(csv->tc_patterns[tci]);
    }

    /* Compile the list of time string patterns */

    csv->tc_patterns[tci] = retime_list_compile(npatterns, patterns, 0);
    if (!csv->tc_patterns[tci]) {
        goto MEMORY_ERROR;
    }

    return(1);

MEMORY_ERROR:

    ERROR( DSPROC_LIB_NAME,
        "Memory allocation error adding CSV time column patterns\n");

    dsproc_set_status(DSPROC_ENOMEM);

    return(0);
}

/*******************************************************************************
 *  Parsing Utilities
 */

/**
 *  Count the number of delimiters in a string.
 *
 *  Delimiters inside single or double quoted strings will not be matched.
 *
 *  @param  strp    pointer to the delimited string
 *  @param  delim   delimiter character
 *
 *  @retval nfound  number of delimiters found.
 */
int dsproc_count_csv_delims(const char *strp, char delim)
{
    int nfound = 0;

    strp = dsproc_skip_csv_whitespace(strp, delim);

    while ((strp = dsproc_find_csv_delim(strp, delim))) {
        ++strp;
        ++nfound;
        strp = dsproc_skip_csv_whitespace(strp, delim);
    }

    return(nfound);
}

/**
 *  Find the next delimiter in a string.
 *
 *  Delimiters inside single or double quoted strings will not be matched.
 *
 *  @param  strp   pointer to the string
 *  @param  delim  delimiter to search for
 *
 *  @retval  strp  pointer to the next delimiter in the string
 *  @retval  NULL  if not found
 */
char *dsproc_find_csv_delim(const char *strp, char delim)
{
    char quote = '\0';

    for (; *strp != '\0'; ++strp) {

        if (quote) {
            if (*strp == quote) {
                quote = '\0';
            }
        }
        else if (*strp == delim) {
            return((char *)strp);
        }
        else if (*strp == '"' || *strp == '\'') {
            quote = *strp;
        }
    }

    return((char *)NULL);
}

/**
 *  Skip white-space characters that do not match the delimiter.
 *
 *  If a delimiter is specified, it will not be treated as a
 *  white-space character unless it is a normal space ' '.
 *
 *  @param  strp   pointer to the first character in the string
 *  @param  delim  delimiter character
 *
 *  @retval  strp  pointer to the next non white-space character
 */
char *dsproc_skip_csv_whitespace(const char *strp, char delim)
{
    if (delim && delim != ' ') {
        while (isspace(*strp) && *strp != delim) ++strp;
    }
    else {
        while (isspace(*strp)) ++strp;
    }

    return((char *)strp);
}

/**
 *  Split a delimited string into list of strings.
 *
 *  The pointers in the output list point to the substrings in the input
 *  string and have been null terminated. All leading and trailing spaces,
 *  and surrounding quotes have also been removed from the substrings.
 *
 *  The pointers in the output list will only be valid as long as the
 *  memory used by the input string is not altered or freed.
 *
 *  @param  strp    pointer to the delimited string
 *  @param  delim   delimiter character
 *  @param  length  length of the output list
 *  @param  list    output: pointers to the substrings in the input string.
 *
 *  @retval nfound  number of substrings returned in the output list,
 *                  or the number that would have beed returned if
 *                  the output list had been big enough.
 */
int dsproc_split_csv_string(char *strp, char delim, int length, char **list)
{
    char *startp;
    char *endp;
    char *delimp;
    int   nfound;

    /* Skip leading white-space */

    startp = dsproc_skip_csv_whitespace(strp, delim);
    if (*startp == '\0') return(0);

    nfound = 0;

    while ((delimp = dsproc_find_csv_delim(startp, delim))) {

        if (delimp != startp) {

            /* Trim trailing white-space from the substring */

            endp = delimp - 1;
            while (endp > startp && isspace(*endp)) *endp-- = '\0';

            /* Remove surrounding quotes from the substring */

            if (endp > startp) {
                if (*endp == '"' || *endp == '\'') {
                    if (*startp == *endp) {
                        startp += 1;
                        *endp = '\0';
                    }
                }
            }
        }

        /* Terminate field */

        *delimp = '\0';

        /* Set substring pointer in output list */

        if (nfound < length) {
            list[nfound] = startp;
        }

        nfound += 1;

        /* Skip leading white-space in the next substring */

        startp = dsproc_skip_csv_whitespace(delimp+1, delim);
    }

    /* Trim trailing white-space from the last field */

    endp = startp + strlen(startp) - 1;
    while (endp > startp && isspace(*endp)) *endp-- = '\0';

    /* Remove surrounding quotes from the substring */

    if (endp > startp) {
        if (*endp == '"' || *endp == '\'') {
            if (*startp == *endp) {
                startp += 1;
                *endp = '\0';
            }
        }
    }

    if (nfound < length) {
        list[nfound] = startp;
    }

    nfound += 1;

    return(nfound);
}

/*******************************************************************************
 *  Print Functions
 */

/**
 *  Print CSV header and records.
 *
 *  If the number of values found on the current line does not match
 *  the number of header fields, a warning message will be generated.
 *
 *  @param   fp   pointer to the output stream
 *  @param   csv  pointer to the CSVParser structure
 *
 *  @retval   1  if successful
 *  @retval   0  if no data was found in the CSV table
 *  @retval  -1  if a write error occurred
 */
int dsproc_print_csv(FILE *fp, CSVParser *csv)
{
    int status;

    status = dsproc_print_csv_header(fp, csv);
    if (status != 1) return(status);

    fprintf(fp, "\n");

    status = dsproc_print_csv_record(fp, csv);
    if (status != 1) return(status);

    return(1);
}

/**
 *  Print CSV header data.
 *
 *  If the number of values found on the current line does not match
 *  the number of header fields, a warning message will be generated.
 *
 *  @param   fp   pointer to the output stream
 *  @param   csv  pointer to the CSVParser structure
 *
 *  @retval   1  if successful
 *  @retval   0  if no header was found in the CSV table
 *  @retval  -1  if a write error occurred
 */
int dsproc_print_csv_header(FILE *fp, CSVParser *csv)
{
    char delim = csv->delim;
    int  fi;

    if (csv->nfields <= 0) {
        fprintf(fp, "No header stored in CSV Table\n");
        return(0);
    }

    fprintf(fp, "%s", csv->headers[0]);
    for (fi = 1; fi < csv->nfields; ++fi) {
        fprintf(fp, "%c%s", delim, csv->headers[fi]);
    }

    if (ferror(fp)) {

        ERROR( DSPROC_LIB_NAME,
            "Could not write to CSV file\n"
            " -> %s\n", strerror(errno));

        dsproc_set_status(DSPROC_EFILEWRITE);

        return(-1);
    }

    return(1);
}

/**
 *  Print CSV record data.
 *
 *  If the number of values found on the current line does not match
 *  the number of header fields, a warning message will be generated.
 *
 *  @param   fp   pointer to the output stream
 *  @param   csv  pointer to the CSVParser structure
 *
 *  @retval   1  if successful
 *  @retval   0  if no data was found in the CSV table
 *  @retval  -1  if a write error occurred
 */
int dsproc_print_csv_record(FILE *fp, CSVParser *csv)
{
    char delim = csv->delim;
    int  ri, fi;

    if (csv->nfields <= 0) {
        fprintf(fp, "No fields stored in CSV Table\n");
        return(0);
    }

    if (csv->nrecs <= 0) {
        fprintf(fp, "No records stored in CSV Table\n");
        return(0);
    }

    for (ri = 0; ri < csv->nrecs; ++ri) {

        fprintf(fp, "%s", csv->values[0][ri]);
        for (fi = 1; fi < csv->nfields; ++fi) {
            fprintf(fp, "%c%s", delim, csv->values[fi][ri]);
        }
        fprintf(fp, "\n");
    }

    if (ferror(fp)) {

        ERROR( DSPROC_LIB_NAME,
            "Could not write to CSV file\n"
            " -> %s\n", strerror(errno));

        dsproc_set_status(DSPROC_EFILEWRITE);

        return(-1);
    }

    return(1);
}

