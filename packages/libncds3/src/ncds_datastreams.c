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
*    $Revision: 12565 $
*    $Author: ermold $
*    $Date: 2012-02-05 03:04:25 +0000 (Sun, 05 Feb 2012) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file ncds_datastreams.c
 *  NetCDF Datastream Functions.
 */

#include <unistd.h>

#include "ncds3.h"
#include "ncds_private.h"

/*******************************************************************************
 *  Private Data and Functions
 */
/** @privatesection */

static int            gNDatastreams;
static NCDatastream **gDatastreams; /**< Arrary of NetCDF Datastream entries */

/**
 *  PRIVATE: Create a new NCDatastream structure.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  name      - datastream name
 *  @param  path      - full path to the datastream directory
 *  @param  extension - file extension
 *
 *  @return
 *    - pointer to the new NCDatastream structure
 *    - NULL if an error occurred
 */
NCDatastream *_ncds_create_datastream(
    const char *name,
    const char *path,
    const char *extension)
{
    NCDatastream *datastream;

    datastream = (NCDatastream *)calloc(1, sizeof(NCDatastream));

    if (!datastream) {

        ERROR( NCDS_LIB_NAME,
            "Could not create new NCDatastream structure for: %s\n",
            " -> memory allocation error\n", name);

        return((NCDatastream *)NULL);
    }

    datastream->name      = msngr_copy_string(name);
    datastream->path      = msngr_copy_string(path);
    datastream->extension = msngr_copy_string(extension);

    if (!datastream->name ||
        !datastream->path ||
        !datastream->extension) {

        ERROR( NCDS_LIB_NAME,
            "Could not create new NCDatastream structure for: %s\n",
            " -> memory allocation error\n", name);

        _ncds_destroy_datastream(datastream);

        return((NCDatastream *)NULL);
    }

    return(datastream);
}

/**
 *  PRIVATE: Destroy a NCDatastream structure.
 *
 *  @param  datastream - pointer to the NCDatastream structure
 */
void _ncds_destroy_datastream(NCDatastream *datastream)
{
    if (datastream) {

        if (datastream->name)        free(datastream->name);
        if (datastream->path)        free(datastream->path);
        if (datastream->extension)   free(datastream->extension);
        if (datastream->split_hours) free(datastream->split_hours);
        if (datastream->split_days)  free(datastream->split_days);

        free(datastream);
    }
}

/**
 *  PRIVATE: Get the next time to split a NetCDF file.
 *
 *  @param  ds_index  - datastream index
 *  @param  prev_time - time of the previous data record
 *
 *  @return
 *    - next split time (in seconds since 1970) or prev_time if there are
 *      no split times defined for the datastream
 *    - 0 if an error occurred
 */
time_t _ncds_get_split_time(
    int    ds_index,
    time_t prev_time)
{
    int       split_nhours = gDatastreams[ds_index]->split_nhours;
    int      *split_hours  = gDatastreams[ds_index]->split_hours;
    int       split_ndays  = gDatastreams[ds_index]->split_ndays;
    int      *split_days   = gDatastreams[ds_index]->split_days;
    int       split_hour;
    int       split_day;
    time_t    split_time;
    int       hi;
    int       di;
    struct tm gmt;

    memset(&gmt, 0, sizeof(struct tm));

    if (!split_nhours && !split_ndays) {
        return(prev_time);
    }

    /* Convert prev_time to struct tm */

    if (!gmtime_r(&prev_time, &gmt)) {

        ERROR( NCDS_LIB_NAME,
            "Could not determine next split time\n"
            " -> gmtime error: %s\n", strerror(errno));

        return(0);
    }

    /* Find the next split time */

    if (split_ndays) {

        split_day = split_days[0];

        for (di = 0; di < split_ndays; di++) {
            if (split_days[di] > gmt.tm_mday) {
                split_day = split_days[di];
                break;
            }
        }

        if (split_day <= gmt.tm_mday) {
            gmt.tm_mon++;
        }

        gmt.tm_mday = split_day;
        gmt.tm_hour = (split_nhours) ? split_hours[0] : 0;
        gmt.tm_min  = 0;
        gmt.tm_sec  = 0;
    }
    else {

        split_hour = split_hours[0];

        for (hi = 0; hi < split_nhours; hi++) {
            if (split_hours[hi] > gmt.tm_hour) {
                split_hour = split_hours[hi];
                break;
            }
        }

        if (split_hour <= gmt.tm_hour) {
            gmt.tm_mday++;
        }

        gmt.tm_hour = split_hour;
        gmt.tm_min  = 0;
        gmt.tm_sec  = 0;
    }

    /* Convert struct tm to seconds since 1970 */

    split_time = mktime(&gmt);

    if (split_time == (time_t)-1) {

        ERROR( NCDS_LIB_NAME,
            "Could not determine next split time\n"
            " -> mktime error: %s\n", strerror(errno));

        return(0);
    }

    split_time -= timezone;

    return(split_time);
}

/**
 *  PRIVATE: Get the index of the next time to split a NetCDF file.
 *
 *  @param  ds_index  - datastream index
 *  @param  prev_time - time of the previous data record
 *  @param  ntimes    - number of times in array
 *  @param  times     - array of timevals
 *
 *  @return
 *    -  index of the next split time or ntimes if no split index was found
 *    - -1 if an error occurred
 */
int _ncds_get_split_index(
    int        ds_index,
    time_t     prev_time,
    int        ntimes,
    timeval_t *times)
{
    time_t    split_time;
    timeval_t split_tv;
    int       split_index;

    /* Get split time */

    split_time = _ncds_get_split_time(ds_index, prev_time);

    if (!split_time) {
        return(-1);
    }

    if (split_time == prev_time) {
        return(0);
    }

    /* Get split index */

    split_tv.tv_sec  = split_time;
    split_tv.tv_usec = 0;

    split_index = cds_find_timeval_index(ntimes, times, split_tv, CDS_GTEQ);

    if (split_index < 0) {
        return(ntimes);
    }

    return(split_index);
}

#if 0
/**
 *  PRIVATE: Function used by _ncds_store().
 *
 *  @param  ds_index     - datastream index
 *  @param  CDSGroup     - cds group containing the data to store
 *  @param  cds_time     - time of the start index in the CDS group
 *  @param  cds_start    - start index in the CDS group
 *  @param  nc_grpid     - NetCDF group id (0 to create a new file)
 *  @param  nc_start     - start index in the NetCDF group
 *  @param  record_count - number of records to store
 *  @param  flags        - control flags
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int _ncds_store_data(
    int       ds_index,
    CDSGroup *cds_group,
    time_t    cds_time,
    size_t    cds_start,
    int       nc_grpid,
    size_t    nc_start,
    size_t    record_count,
    int       flags)
{
    char   *ds_path = gDatastreams[ds_index]->path;
    char   *ds_name = gDatastreams[ds_index]->name;
    char   *ds_ext  = gDatastreams[ds_index]->extension;

    char    nc_timestamp[32];
    char    nc_file[PATH_MAX];

    CDSVar *var;
    int     status;
    int     varid;
    int     vi;
    size_t  write_count;

    if (!nc_grpid) {

        if (!ncds_format_timestamp(cds_time, nc_timestamp)) {
            return(0);
        }

        snprintf(nc_file, PATH_MAX, "%s/%s.%s.%s",
            ds_path, ds_name, nc_timestamp, ds_ext);

        /* Create the NetCDF file */

        if (!ncds_create(nc_file, NC_NOCLOBBER, &nc_grpid)) {
            return(0);
        }

        /* Write the NetCDF header */

        if (!ncds_write_group(cds_group, nc_grpid, 0)) {
            ncds_close(nc_grpid);
            unlink(nc_file);
            return(0);
        }

        if (!ncds_enddef(nc_grpid)) {
            ncds_close(nc_grpid);
            unlink(nc_file);
            return(0);
        }

        nc_start = 0;
    }

    /* Write the data */

    for (vi = 0; vi < cds_group->nvars; vi++) {

        var = cds_group->vars[vi];

        /* Check if the variable has any data defined */

        if (!var->sample_count) {
            continue;
        }

        /* Get variable id in NetCDF file */

        status = ncds_inq_varid(nc_grpid, var->name, &varid);
        if (status <= 0) {

            if (status == 0) {

                ERROR( NCDS_LIB_NAME,
                    "Could not store variable data for: %s\n"
                    " -> variable does not exist in netcdf file\n",
                    var->name);
            }

            return(0);
        }

        /* Store the variable data */

        if (var->ndims == 0 || var->dims[0]->is_unlimited == 0) {

            /* Static Variable */

            if (!ncds_write_var_samples(var, 0, nc_grpid, varid, 0, NULL)) {
                return(0);
            }
        }
        else {

            /* Record Variable */

            write_count = record_count;

            if (!ncds_write_var_samples(
                var, cds_start, nc_grpid, varid, nc_start, &write_count)) {

                return(0);
            }
        }
    }

    return(1);
}

/**
 *  PRIVATE: Function used by ncds_store().
 *
 *  This function will store data for a datastream, splitting the
 *  files when necessary.
 *
 *  This function will also close the NetCDF file before returning.
 *
 *  @param  ds_index      - datastream index
 *  @param  prev_time     - time just prior to the cds start time
 *  @param  cds_group     - cds group containing the data to store
 *  @param  cds_ntimevals - number of timevals in array
 *  @param  cds_timevals  - array of timevals for the CDS group
 *  @param  cds_start     - start index in the CDS group
 *  @param  nc_grpid      - NetCDF group id (0 to create a new file)
 *  @param  nc_start      - start index in the NetCDF group
 *  @param  record_count  - number of records to store
 *  @param  flags         - control flags
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int _ncds_store(
    int        ds_index,
    time_t     prev_time,
    CDSGroup  *cds_group,
    int        cds_ntimevals,
    timeval_t *cds_timevals,
    size_t     cds_start,
    int        nc_grpid,
    size_t     nc_start,
    size_t     record_count,
    int        flags)
{
    char   *ds_path = gDatastreams[ds_index]->path;
    char   *ds_name = gDatastreams[ds_index]->name;
    char   *ds_ext  = gDatastreams[ds_index]->extension;

    int     split_index;

    split_index = _ncds_get_split_index(
        ds_index,
        prev_time,
        cds_ntimevals - cds_start,
        &cds_timevals[cds_start]);

    if (split_index < 0) {
        return(0);
    }

    split_index += cds_start;

    if (!_ncds_store_data(
        ds_index,
        cds_group,
        cds_timevals[cds_start].tv_sec,
        cds_start,
        nc_grpid,
        nc_start,
        (split_index - cds_start),
        flags)) {

        return(0);
    }


    return(1);
}
#endif

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Add a data stream to the internal Data streams array.
 *
 *  By default the split_interval will be set to 86400 seconds
 *  (one day) and the split hour will be set to 0. To change
 *  these values see the ncds_set_ds_split_interval() and
 *  ncds_set_ds_split_hour() functions.
 *
 *  If an entry already exists for the specified datastream, the
 *  path and extension values will be updated with the new ones.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  name      - datastream name
 *  @param  path      - full path to the datastream directory
 *  @param  extension - file extension
 *
 *  @return
 *    - datastream index
 *    - -1 if an error occurred
 */
int ncds_add_datastream(
    const char *name,
    const char *path,
    const char *extension)
{
    int            index;
    NCDatastream  *datastream;
    NCDatastream **datastreams;
    char          *path_copy;
    char          *ext_copy;

    /* Update the Datastream entry if it already exists */

    for (index = 0; index < gNDatastreams; index++) {

        if (strcmp(gDatastreams[index]->name, name) == 0) {

            path_copy = msngr_copy_string(path);
            ext_copy  = msngr_copy_string(extension);

            if (!path_copy || !ext_copy) {

                ERROR( NCDS_LIB_NAME,
                    "Could not update netcdf datastream entry for: %s\n",
                    " -> memory allocation error\n", name);

                return(-1);
            }

            datastream = gDatastreams[index];

            free(datastream->path);
            free(datastream->extension);

            datastream->path      = path_copy;
            datastream->extension = ext_copy;

            return(index);
        }
    }

    /* Create the new NCDatastream structure */

    datastream = _ncds_create_datastream(name, path, extension);

    if (!datastream) {
        return(-1);
    }

    /* Increase gDatastreams array size */

    datastreams = (NCDatastream **)realloc(
        gDatastreams, (gNDatastreams + 2) * sizeof(NCDatastream));

    if (!datastreams) {

        ERROR( NCDS_LIB_NAME,
            "Could not add netcdf datastream entry for: %s\n",
            " -> memory allocation error\n", name);

        _ncds_destroy_datastream(datastream);
        return(-1);
    }

    gDatastreams          = datastreams;
    gDatastreams[index]   = datastream;
    gDatastreams[index+1] = (NCDatastream *)NULL;

    gNDatastreams++;

    return(index);
}

/**
 *  Delete a datastream from the internal Datastreams array.
 *
 *  @param  ds_index - datastream index
 */
void ncds_delete_datastream(int ds_index)
{
    int index;

    _ncds_destroy_datastream(gDatastreams[ds_index]);

    for (index = ds_index + 1; index < gNDatastreams; index++) {
        gDatastreams[index-1] = gDatastreams[index];
    }

    gDatastreams[index-1] = (NCDatastream *)NULL;
    gNDatastreams--;
}

/**
 *  Set the file split hours for a Datastream.
 *
 *  @param  ds_index - datastream index
 *  @param  nhours   - number of hours in the array
 *  @param  hours    - the hours of the day netcdf files should be split
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int ncds_set_split_hours(int ds_index, int nhours, int *hours)
{
    int *split_hours;

    split_hours = (int *)malloc((size_t)nhours * sizeof(int));
    if (!split_hours) {

        ERROR( NCDS_LIB_NAME,
            "Could not set split hours for netcdf datastream entry: %s\n",
            " -> memory allocation error\n",
            gDatastreams[ds_index]->name);

        return(0);
    }

    memcpy(split_hours, hours, nhours * sizeof(int));

    if (gDatastreams[ds_index]->split_hours) {
        free(gDatastreams[ds_index]->split_hours);
    }

    gDatastreams[ds_index]->split_nhours = nhours;
    gDatastreams[ds_index]->split_hours  = split_hours;

    return(1);
}

/**
 *  Set the file split days for a Datastream.
 *
 *  @param  ds_index - datastream index
 *  @param  ndays    - number of days in the array
 *  @param  days     - the days of the month netcdf files should be split
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int ncds_set_split_days(int ds_index, int ndays, int *days)
{
    int *split_days;

    split_days = (int *)malloc((size_t)ndays * sizeof(int));
    if (!split_days) {

        ERROR( NCDS_LIB_NAME,
            "Could not set split days for netcdf datastream entry: %s\n",
            " -> memory allocation error\n",
            gDatastreams[ds_index]->name);

        return(0);
    }

    memcpy(split_days, days, ndays * sizeof(int));

    if (gDatastreams[ds_index]->split_days) {
        free(gDatastreams[ds_index]->split_days);
    }

    gDatastreams[ds_index]->split_ndays = ndays;
    gDatastreams[ds_index]->split_days  = split_days;

    return(1);
}

/* function not completed yet */
#if 0

/**
 *  Store data for a datastream.
 *
 *  @param  ds_index - datastream index
 *  @param  CDSGroup - cds group containing the data to store
 *  @param  flags    - control flags
 *
 *  Control Flags:
 *
 *    - NCDS_FORCE_SPLIT  = force the netcdf file to split
 *
 *  @return
 *    - number of records stored
 *    - -1 if an error occurred
 */
int ncds_store(int ds_index, CDSGroup *cds_group, int flags)
{
    char      *ds_path = gDatastreams[ds_index]->path;
    char      *ds_name = gDatastreams[ds_index]->name;
    char      *ds_ext  = gDatastreams[ds_index]->extension;

    int        cds_ntimes;
    timeval_t *cds_times;
    double     cds_start;
    double     cds_end;
    int        cds_start_index;

    int        nc_nfiles;
    char     **nc_files;
    char       nc_file[PATH_MAX];
    int        nc_id;
    int        nc_ntimes;
    time_t     nc_base_time;
    double    *nc_time_offsets;
    double     nc_start;
    double     nc_end;

    char       ts1[32], ts2[32];

    /* Get time range of data to store */

    cds_ntimes = cds_get_sample_timevals(cds_group, 0, 0, &cds_times);
    if (cds_ntimes <= 0) {
        return(cds_ntimes);
    }

    cds_start = TV_DOUBLE(cds_times[0]);
    cds_end   = TV_DOUBLE(cds_times[cds_ntimes-1]);

    /* Get the list of existing data files for the cds time range */

    nc_nfiles = ncds_find_files(ds_path, ds_name, ds_ext,
        (time_t)cds_start, (time_t)cds_end, &nc_files);

    if (nc_nfiles < 0) {
        free(cds_times);
        return(-1);
    }

    /* If more than 1 file was found we may need to merge the current
     * data with the data that was previously stored. For now just fail */

    if (nc_nfiles > 1) {

        ERROR( NCDS_LIB_NAME,
            "Could not store data for: %s\n"
            " -> files exist in storage range: ['%s', '%s']\n",
            ds_name,
            msngr_format_time((time_t)cds_start, ts1),
            msngr_format_time((time_t)cds_end, ts2));

        free(cds_times);
        ncds_free_file_list(nc_files);
        return(-1);
    }

    if (nc_nfiles == 0) {
/* create new file */
    }
    else { /* nc_files == 1 */

        /* Open the NetCDF file */

        snprintf(nc_file, PATH_MAX, "%s/%s", ds_path, nc_files[0]);

        if (!ncds_open(nc_file, NC_WRITE, &nc_id)) {
            free(cds_times);
            ncds_free_file_list(nc_files);
            return(-1);
        }

        /* Get record times */

        nc_ntimes = ncds_get_time_range(
            nc_id, &nc_base_time, &nc_start, &nc_end);

        if (nc_ntimes < 0) {
            free(cds_times);
            ncds_free_file_list(nc_files);
            return(-1);
        }
        else if (nc_ntimes == 0) {
            nc_start = 0;
            nc_end   = 0;
        }
        else {
            nc_start += (double)nc_base_time;
            nc_end   += (double)nc_base_time;
        }

        /* Check for overlaps */

        if (cds_start <= nc_end) {
/* skip duplicates in cds or fail with overlap error */
/* update cds_start_index and cds_start */
        }

    }

    free(cds_times);
    ncds_free_file_list(nc_files);

}

#endif
