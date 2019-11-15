/*******************************************************************************
*
*  COPYRIGHT (C) 2012 Battelle Memorial Institute.  All Rights Reserved.
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
*    $Revision: 56490 $
*    $Author: ermold $
*    $Date: 2014-09-15 19:46:07 +0000 (Mon, 15 Sep 2014) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file dsproc_dataset_fetch.c
 *  Dataset Fetch Functions.
 */

#include "dsproc3.h"
#include "dsproc_private.h"

extern DSProc *_DSProc; /**< Internal DSProc structure */

/** @privatesection */

/*******************************************************************************
 *  Static Functions Visible Only To This Module
 */

/**
 *  Static: Get the time range of the previously fetched data.
 *
 *  @param   ds     pointer to the DataStream structure
 *  @param   begin  output: begin time
 *  @param   end    output: end time
 *
 *  @retval  1      previously fetched data exists
 *  @retval  0      previously fetched data does not exist
 */
static int _dsproc_get_fetched_range(
    DataStream *ds,
    timeval_t  *begin,
    timeval_t  *end)
{
    CDSGroup *dataset;
    CDSVar   *var;
    size_t    count;

    /* Initialize variables */

    memset(begin, 0, sizeof(timeval_t));
    memset(end, 0, sizeof(timeval_t));

    /* Check if any previously fetched data exists */

    if (!ds ||
        !ds->fetched_cds ||
        !ds->fetched_cds->ngroups) {

        return(0);
    }

    /* Get start time */

    dataset = ds->fetched_cds->groups[0];
    var     = cds_find_time_var(dataset);
    if (!var) return(0);

    count = 1;
    cds_get_sample_timevals(var, 0, &count, begin);

    /* Get end time */

    dataset = ds->fetched_cds->groups[ds->fetched_cds->ngroups - 1];
    var     = cds_find_time_var(dataset);
    if (!var) return(0);

    count = 1;
    cds_get_sample_timevals(var, var->sample_count - 1, &count, end);

    return(1);
}

/*******************************************************************************
 *  Private Functions Visible Only To This Library
 */

/**
 *  Private: Fetch previously stored data from a datastream file.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  dsfile        - pointer to the DSFile structure
 *  @param  start         - index of the first record to retrieve
 *  @param  count         - number of records to retrieve
 *  @param  nvars         - the number of variables to retrieve
 *                          (or 0 to retrieve all variables)
 *  @param  var_names     - the list if variable names to retrieve
 *                          (or NULL to retrieve all variables)
 *  @param  parent        - pointer to the parent dataset to store the
 *                          observation in or NULL for no parent group.
 *
 *  @retval obs   pointer to the retrieved observation
 *  @retval NULL  if an error occurred
 */
CDSGroup *_dsproc_fetch_dsfile_dataset(
    DSFile      *dsfile,
    size_t       start,
    size_t       count,
    size_t       nvars,
    const char **var_names,
    CDSGroup    *parent)
{
    size_t      tmp_count;
    CDSGroup   *dataset;
    CDSVar     *var;
    int         varid;
    int         status;
    size_t      vi;

    /* Create the dataset group for this file */

    dataset = cds_define_group(parent, dsfile->name);

    if (!dataset) {
        dsproc_set_status(DSPROC_ENOMEM);
        return((CDSGroup *)NULL);
    }

    /* Load the data */

    if (!_dsproc_open_dsfile(dsfile, 0)) {
        cds_delete_group(dataset);
        return((CDSGroup *)NULL);
    }

    if (!nvars) {

        /* Read in the NetCDF header */

        if (!ncds_read_group(dsfile->ncid, 0, dataset)) {

            ERROR( DSPROC_LIB_NAME,
                "Could not read in netcdf header from: %s\n",
                dsfile->full_path);

            dsproc_set_status(DSPROC_ENCREAD);
            cds_delete_group(dataset);
            return((CDSGroup *)NULL);
        }

        /* Read in the NetCDF data */

        if (!ncds_read_group_data(
            dsfile->ncid, start, count, 0, dataset, 0)) {

            ERROR( DSPROC_LIB_NAME,
                "Could not read in netcdf data from: %s\n",
                dsfile->full_path);

            dsproc_set_status(DSPROC_ENCREAD);
            cds_delete_group(dataset);
            return((CDSGroup *)NULL);
        }
    }
    else {

        /* Read in all global attributes */

        status = ncds_read_atts(dsfile->ncid, dataset);

        if (status < 0) {

            ERROR( DSPROC_LIB_NAME,
                "Could not read in global attributes from: %s\n",
                dsfile->full_path);

            dsproc_set_status(DSPROC_ENCREAD);
            cds_delete_group(dataset);
            return((CDSGroup *)NULL);
        }

        /* Read in the time variable */

        status = ncds_get_time_info(
            dsfile->ncid, NULL, &varid, NULL, NULL, NULL, NULL);

        if (status <= 0) {

            ERROR( DSPROC_LIB_NAME,
                "Could not get time variable id from: %s\n",
                dsfile->full_path);

            dsproc_set_status(DSPROC_ENCREAD);
            cds_delete_group(dataset);
            return((CDSGroup *)NULL);
        }

        tmp_count = count;

        var = ncds_get_var_by_id(
            dsfile->ncid, varid, start, &tmp_count, dataset,
            NULL, 0, NULL, 0, 0, NULL, NULL, NULL, NULL);

        if (!var) {

            ERROR( DSPROC_LIB_NAME,
                "Could not read in time variable data from: %s\n",
                dsfile->full_path);

            dsproc_set_status(DSPROC_ENCREAD);
            cds_delete_group(dataset);
            return((CDSGroup *)NULL);
        }

        /* Read in all requested variables */

        for (vi = 0; vi < nvars; vi++) {

            tmp_count = count;
            var = ncds_get_var(
                dsfile->ncid, var_names[vi], start, &tmp_count, dataset,
                NULL, 0, NULL, 0, 0, NULL, NULL, NULL, NULL);

            if (var == (CDSVar *)-1) {

                ERROR( DSPROC_LIB_NAME,
                    "Could not read in %s variable data from: %s\n",
                    var_names[vi], dsfile->full_path);

                dsproc_set_status(DSPROC_ENCREAD);
                cds_delete_group(dataset);
                return((CDSGroup *)NULL);
            }

            if (var == (CDSVar *)NULL) {

                WARNING( DSPROC_LIB_NAME,
                    "Requested variable %s not found in: %s\n",
                    var_names[vi], dsfile->full_path);
            }
        }
    }

    return(dataset);
}

/**
 *  Private: Get the DOD for a datastream file.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  file - pointer to the DSFile structure.
 *
 *  @return
 *    - pointer to the CDSGroup containing the DOD
 *    - NULL if an error occurred
 */
CDSGroup *_dsproc_fetch_dsfile_dod(DSFile *file)
{
    if (file->dod) {
        return(file->dod);
    }

    if (!_dsproc_open_dsfile(file, 0)) {
        return((CDSGroup *)NULL);
    }

    file->dod = cds_define_group(NULL, file->name);

    if (!file->dod) {

        ERROR( DSPROC_LIB_NAME,
            "Could not get DOD for file: %s\n"
            " -> memory allocation error",
            file->full_path);

        dsproc_set_status(DSPROC_ENOMEM);
        return((CDSGroup *)NULL);
    }

    if (!ncds_read_group(file->ncid, 0, file->dod)) {

        ERROR( DSPROC_LIB_NAME,
            "Could not get DOD for file: %s\n",
            file->full_path);

        dsproc_set_status(DSPROC_ENCREAD);
        return((CDSGroup *)NULL);
    }

    if (!ncds_read_static_data(file->ncid, file->dod)) {

        ERROR( DSPROC_LIB_NAME,
            "Could not get DOD for file: %s\n",
            file->full_path);

        dsproc_set_status(DSPROC_ENCREAD);
        return((CDSGroup *)NULL);
    }

    return(file->dod);
}

/**
 *  Private: Fetch previously stored datasets.
 *
 *  This function will search the specified datastream files and retrieve
 *  all data for the specified time range. The _dsproc_find_dsfiles()
 *  function should be used to get the dsfiles list.
 *
 *  If the begin_timeval is not specified, data for the time just prior to
 *  the end_timeval will be retrieved.
 *
 *  If the end_timeval is not specified, data for the time just after the
 *  begin_timeval will be retrieved.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  ndsfiles      - number of datastream Files
 *  @param  dsfiles       - list of datastream Files to search
 *  @param  begin_timeval - beginning of the time range to search
 *  @param  end_timeval   - end of the time range to search
 *  @param  nvars         - the number of variables to retrieve
 *                          (or 0 to retrieve all variables)
 *  @param  var_names     - the list if variable names to retrieve
 *                          (or NULL to retrieve all variables)
 *  @param  merge_obs     - flag specifying if multiple observations should
 *                          be merged when possible (0 == false, 1 == true)
 *  @param  parent        - pointer to the parent dataset to store the
 *                          retrieved observations in.
 *
 *  @retval nobs  the number of observations retrieved
 *  @retval -1    if an error occurred
 */
int _dsproc_fetch_dataset(
    int          ndsfiles,
    DSFile     **dsfiles,
    timeval_t   *begin_timeval,
    timeval_t   *end_timeval,
    size_t       nvars,
    const char **var_names,
    int          merge_obs,
    CDSGroup    *parent)
{
    DSFile   *dsfile;
    DSFile   *prev_dsfile;
    int       prev_start;
    int       start;
    int       end;
    size_t    count;
    int       nobs;
    int       fi;

    if (!ndsfiles) {
        return(0);
    }

    if ((!begin_timeval || !begin_timeval->tv_sec) &&
        (!end_timeval   || !end_timeval->tv_sec)) {

        return(0);
    }

    /* Loop over all datastream files */

    prev_dsfile = (DSFile *)NULL;
    prev_start  = 0;
    nobs        = 0;

    for (fi = 0; fi < ndsfiles; fi++) {

        dsfile = dsfiles[fi];

        if (!dsfile->ntimes) {
            continue;
        }

        if (!begin_timeval || !begin_timeval->tv_sec) {

            /* We want the dataset for the time just prior to the end_timeval */

            start = cds_find_timeval_index(
                dsfile->ntimes, dsfile->timevals, *end_timeval, CDS_LT);

            if (start >= 0) {
                prev_dsfile = dsfile;
                prev_start  = start;
            }
            else {
                break;
            }
        }
        else if (!end_timeval || !end_timeval->tv_sec) {

            /* We want the dataset for the time just after the begin_timeval */

            start = cds_find_timeval_index(
                dsfile->ntimes, dsfile->timevals, *begin_timeval, CDS_GT);

            if (start >= 0) {
                prev_dsfile = dsfile;
                prev_start  = start;
                break;
            }
        }
        else {

            /* We want the datasets for all times in the specified range */

            start = cds_find_timeval_index(
                dsfile->ntimes, dsfile->timevals, *begin_timeval, CDS_GTEQ);

            end = cds_find_timeval_index(
                dsfile->ntimes, dsfile->timevals, *end_timeval, CDS_LTEQ);

            if ((start >= 0) &&
                (end   >= start)) {

                count = end - start + 1;

                if (!_dsproc_fetch_dsfile_dataset(
                    dsfile, start, count, nvars, var_names, parent)) {

                    return(-1);
                }

                nobs++;
            }
        }

    } /* end loop over all datastream files */

    if (prev_dsfile) {

        if (!_dsproc_fetch_dsfile_dataset(
            dsfile, prev_start, 1, nvars, var_names, parent)) {

            return(-1);
        }

        nobs++;
    }

    if (merge_obs &&
        nobs > 1) {

        nobs = _dsproc_merge_obs(parent);
    }

    return(nobs);
}

/**
 *  Private: Fetch the times of previously stored data.
 *
 *  This function will search the specified datastream files and retrieve
 *  all times within the specified time range. The _dsproc_find_dsfiles()
 *  function should be used to get the dsfiles list.
 *
 *  If the begin_timeval is not specified, the time just prior to
 *  the end_timeval will be returned.
 *
 *  If the end_timeval is not specified, the time just after
 *  the begin_timeval will be returned.
 *
 *  Memory will be allocated for the returned array of times if the output
 *  array is NULL. In this case the calling process is responsible for
 *  freeing the allocated memory.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  ds            - pointer to the DataStream structure
 *  @param  ndsfiles      - number of datastream Files
 *  @param  dsfiles       - list of datastream Files to search
 *  @param  begin_timeval - beginning of the time range to search
 *  @param  end_timeval   - end of the time range to search
 *  @param  ntimevals     - pointer to the number of timevals
 *                            - input:
 *                                - length of the output array
 *                                - ignored if the output array is NULL
 *                            - output:
 *                                - number of timevals returned
 *                                - 0 if no times were found in the specified range
 *                                - (size_t)-1 if a memory allocation error occurs
 *  @param  timevals      - pointer to the output array
 *                          or NULL to dynamically allocate the memory needed.
 *
 *  @return
 *    - pointer to the array of timevals
 *    - NULL if:
 *        - no times were found in the specified range (ntimevals == 0)
 *        - an error occurred (ntimevals == (size_t)-1)
 */
timeval_t *_dsproc_fetch_timevals(
    DataStream  *ds,
    int          ndsfiles,
    DSFile     **dsfiles,
    timeval_t   *begin_timeval,
    timeval_t   *end_timeval,
    size_t      *ntimevals,
    timeval_t   *timevals)
{
    DSFile     *dsfile;
    size_t      max_ntimes;
    int         free_timevals;
    int         start;
    int         end;
    int         fi, fti;
    size_t      oti;

    if (!ndsfiles) {
        *ntimevals = 0;
        return((timeval_t *)NULL);
    }

    if ((!begin_timeval || !begin_timeval->tv_sec) &&
        (!end_timeval   || !end_timeval->tv_sec)) {

        *ntimevals = 0;
        return((timeval_t *)NULL);
    }

    /* Determine the maximum number of times to get */

    if (timevals) {
        free_timevals = 0;
        max_ntimes    = *ntimevals;
    }
    else {

        free_timevals = 1;
        max_ntimes    = dsfiles[0]->ntimes;

        for (fi = 1; fi < ndsfiles; fi++) {
            max_ntimes += dsfiles[fi]->ntimes;
        }

        timevals = (timeval_t *)calloc(max_ntimes, sizeof(timeval_t));
        if (!timevals) {

            ERROR( DSPROC_LIB_NAME,
                "Could not fetch times from datastream: %s\n"
                " -> memory allocation error",
                ds->name);

            dsproc_set_status(DSPROC_ENOMEM);
            *ntimevals = (size_t)-1;
            return((timeval_t *)NULL);
        }
    }

    /* Loop over all datastream files */

    oti = 0;

    for (fi = 0; fi < ndsfiles; fi++) {

        dsfile = dsfiles[fi];

        if (!dsfile->ntimes) {
            continue;
        }

        if (!begin_timeval || !begin_timeval->tv_sec) {

            /* We want the time just prior to the end_timeval */

            start = cds_find_timeval_index(
                dsfile->ntimes, dsfile->timevals, *end_timeval, CDS_LT);

            if (start >= 0) {
                timevals[0] = dsfile->timevals[start];
                oti         = 1;
            }
            else {
                break;
            }
        }
        else if (!end_timeval || !end_timeval->tv_sec) {

            /* We want the time just after the begin_timeval */

            start = cds_find_timeval_index(
                dsfile->ntimes, dsfile->timevals, *begin_timeval, CDS_GT);

            if (start >= 0) {
                timevals[0] = dsfile->timevals[start];
                oti         = 1;
                break;
            }
        }
        else {

            /* We want all times in the specified range */

            start = cds_find_timeval_index(
                dsfile->ntimes, dsfile->timevals, *begin_timeval, CDS_GTEQ);

            end = cds_find_timeval_index(
                dsfile->ntimes, dsfile->timevals, *end_timeval, CDS_LTEQ);

            if ((start >= 0) &&
                (end   >= start)) {

                for (fti = start; fti <= end; ) {
                    timevals[oti++] = dsfile->timevals[fti++];
                    if (oti == max_ntimes) break;
                }

                if (oti == max_ntimes) break;
            }
        }

    } /* end loop over all datastream files */

    if (!oti) {
        if (free_timevals) free(timevals);
        timevals = (timeval_t *)NULL;
    }

    *ntimevals = oti;

    return(timevals);
}

/** @publicsection */

/*******************************************************************************
 *  Internal Functions Visible To The Public
 */

/*******************************************************************************
 *  Public Functions
 */

/**
 *  Fetch a dataset from previously stored data.
 *
 *  This function will retrieve a dataset from the previously stored data
 *  for the specified datastream and time range.
 *
 *  If the begin_timeval is not specified, data for the time just prior to
 *  the end_timeval will be retrieved.
 *
 *  If the end_timeval is not specified, data for the time just after the
 *  begin_timeval will be retrieved.
 *
 *  If both the begin and end times are not specified, the data previously
 *  retrieved by this function will be returned.
 *
 *  This memory used by the returned dataset belongs to the internal datastream
 *  structure and must not be freed by the calling process.  This dataset will
 *  remain valid until the next call to this function using a different time
 *  range and/or different variable names.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  ds_id         - datastream ID
 *  @param  begin_timeval - beginning of the time range to search
 *  @param  end_timeval   - end of the time range to search
 *  @param  nvars         - the number of variables to retrieve
 *                          (or 0 to retrieve all variables)
 *  @param  var_names     - the list if variable names to retrieve
 *                          (or NULL to retrieve all variables)
 *  @param  merge_obs     - flag specifying if multiple observations should
 *                          be merged when possible (0 == false, 1 == true)
 *  @param  dataset       - pointer to the retrieved dataset
 *
 *  @retval nobs  the number of observations in the returned dataset
 *  @retval -1    if an error occurred
 */
int dsproc_fetch_dataset(
    int          ds_id,
    timeval_t   *begin_timeval,
    timeval_t   *end_timeval,
    size_t       nvars,
    const char **var_names,
    int          merge_obs,
    CDSGroup    **dataset)
{
    DataStream *ds           = _DSProc->datastreams[ds_id];
    timeval_t   search_begin = {0, 0};
    timeval_t   search_end   = {0, 0};
    timeval_t   data_begin   = {0, 0};
    timeval_t   data_end     = {0, 0};
    int         ndsfiles;
    DSFile    **dsfiles;
    int         new_request;
    int         nobs;
    char        ts1[32], ts2[32];
    size_t      vi;

    /************************************************************
    *  Initialize Variables
    *************************************************************/

    *dataset = (CDSGroup *)NULL;

    if (begin_timeval) search_begin = *begin_timeval;
    if (end_timeval)   search_end   = *end_timeval;

    if (msngr_debug_level || msngr_provenance_level) {

        if (search_begin.tv_sec) format_timeval(&search_begin, ts1);
        else                     strcpy(ts1, "N/A");

        if (search_end.tv_sec)   format_timeval(&search_end, ts2);
        else                     strcpy(ts2, "N/A");

        DEBUG_LV1( DSPROC_LIB_NAME,
            "%s: Fetching previously stored datasets\n"
            " - search begin: %s\n"
            " - search end:   %s\n", ds->name, ts1, ts2);
    }

    /************************************************************
    *  If the begin and end times were not specified, return the
    *  dataset previously retrieved by this function.
    *************************************************************/

    if (!search_begin.tv_sec &&
        !search_end.tv_sec) {

        if (ds->fetched_cds) {

            DEBUG_LV1( DSPROC_LIB_NAME,
                " - returning dataset from previous request\n");

            *dataset = ds->fetched_cds;
            return(ds->fetched_cds->ngroups);
        }
        else {

            DEBUG_LV1( DSPROC_LIB_NAME,
                " - no previous dataset to return\n");

            return(0);
        }
    }

    /************************************************************
    *  Check if we have already retrieved the dataset for this
    *  request, or clear the results from the previous request.
    *************************************************************/

    new_request = 0;

    if (!_dsproc_get_fetched_range(ds, &data_begin, &data_end)) {
        new_request = 1;
    }
    else if (
        (search_begin.tv_sec == 0 || ds->fetch_begin.tv_sec == 0) &&
        (search_begin.tv_sec != ds->fetch_begin.tv_sec) ) {

        new_request = 1;
    }
    else if (
        (search_begin.tv_sec != 0) &&
        (TV_LT(search_begin, ds->fetch_begin) ||
         TV_GT(search_begin, data_begin)) ) {

        new_request = 1;
    }
    else if (
        (search_end.tv_sec == 0 || ds->fetch_end.tv_sec == 0) &&
        (search_end.tv_sec != ds->fetch_end.tv_sec) ) {

        new_request = 1;
    }
    else if (
        (search_end.tv_sec != 0) &&
        (TV_LT(search_end, data_end) ||
         TV_GT(search_end, ds->fetch_end)) ) {

        new_request = 1;
    }
    else if (ds->fetch_nvars != 0) {

        if (nvars == 0) {
            new_request = 1;
        }
        else {
            for (vi = 0; vi < nvars; vi++) {
                if (!cds_get_var(ds->fetched_cds, var_names[vi])) break;
            }

            if (vi != nvars) {
                new_request = 1;
            }
        }
    }

    if (new_request) {
        _dsproc_free_datastream_fetched_cds(ds);
    }
    else {

        DEBUG_LV1( DSPROC_LIB_NAME,
            " - returning dataset from previous request\n");

        *dataset = ds->fetched_cds;
        return(ds->fetched_cds->ngroups);
    }

    /************************************************************
    *  Get the list of datastream files in the requested range
    *************************************************************/

    ndsfiles = _dsproc_find_dsfiles(
        ds->dir, &search_begin, &search_end, &dsfiles);

    if (ndsfiles <= 0) {

        if (ndsfiles == 0) {
            DEBUG_LV1( DSPROC_LIB_NAME,
                " - no stored data found for requested range\n");
        }

        return(ndsfiles);
    }

    /************************************************************
    *  Fetch the data.
    *************************************************************/

    ds->fetched_cds = cds_define_group(NULL, ds->name);
    if (!ds->fetched_cds) {
        dsproc_set_status(DSPROC_ENOMEM);
        return(-1);
    }

    nobs = _dsproc_fetch_dataset(
        ndsfiles, dsfiles, &search_begin, &search_end,
        nvars, var_names, merge_obs, ds->fetched_cds);

    free(dsfiles);

    if (nobs <= 0) {

        if (nobs == 0) {
            DEBUG_LV1( DSPROC_LIB_NAME,
                " - no stored data found for requested range\n");
        }

        _dsproc_free_datastream_fetched_cds(ds);
        return(nobs);
    }

    if (msngr_debug_level || msngr_provenance_level) {

        _dsproc_get_fetched_range(ds, &data_begin, &data_end);

        format_timeval(&data_begin, ts1);
        format_timeval(&data_end, ts2);

        DEBUG_LV1( DSPROC_LIB_NAME,
            " - data begin:   %s\n"
            " - data end:     %s\n", ts1, ts2);
    }

    ds->fetch_begin = search_begin;
    ds->fetch_end   = search_end;
    ds->fetch_nvars = nvars;

    *dataset = ds->fetched_cds;

    return(ds->fetched_cds->ngroups);
}

/**
 *  Fetch the times of previously stored data.
 *
 *  This function will retrieve the times of previously stored data for the
 *  specified datastream and time range.
 *
 *  If the begin_timeval is not specified, the time just prior to the
 *  end_timeval will be returned.
 *
 *  If the end_timeval is not specified, the time just after the
 *  begin_timeval will be returned.
 *
 *  Memory will be allocated for the returned array of times if the output
 *  array is NULL. In this case the calling process is responsible for
 *  freeing the allocated memory.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  ds_id         - datastream ID
 *  @param  begin_timeval - beginning of the time range to search
 *  @param  end_timeval   - end of the time range to search
 *  @param  ntimevals     - pointer to the number of timevals
 *                            - input:
 *                                - length of the output array
 *                                - ignored if the output array is NULL
 *                            - output:
 *                                - number of timevals returned
 *                                - 0 if no times were found in the specified range
 *                                - (size_t)-1 if a memory allocation error occurs
 *  @param  timevals      - pointer to the output array
 *                          or NULL to dynamically allocate the memory needed.
 *
 *  @return
 *    - pointer to the array of timevals
 *    - NULL if:
 *        - no times were found in the specified range (ntimevals == 0)
 *        - an error occurred (ntimevals == (size_t)-1)
 */
timeval_t *dsproc_fetch_timevals(
    int          ds_id,
    timeval_t   *begin_timeval,
    timeval_t   *end_timeval,
    size_t      *ntimevals,
    timeval_t   *timevals)
{
    DataStream *ds = _DSProc->datastreams[ds_id];
    int         ndsfiles;
    DSFile    **dsfiles;
    char        ts1[32], ts2[32];

    if (msngr_debug_level || msngr_provenance_level) {

        if (begin_timeval && begin_timeval->tv_sec) {
            format_timeval(begin_timeval, ts1);
        }
        else {
            strcpy(ts1, "N/A");
        }

        if (end_timeval && end_timeval->tv_sec) {
            format_timeval(end_timeval, ts2);
        }
        else {
            strcpy(ts2, "N/A");
        }

        DEBUG_LV1( DSPROC_LIB_NAME,
            "%s: Fetching times of previously stored data\n"
            " - search begin: %s\n"
            " - search end:   %s\n", ds->name, ts1, ts2);
    }

    /************************************************************
    *  Get the list of datastream files in the requested range
    *************************************************************/

    ndsfiles = _dsproc_find_dsfiles(
        ds->dir, begin_timeval, end_timeval, &dsfiles);

    if (ndsfiles <= 0) {

        if (ndsfiles == 0) {
            DEBUG_LV1( DSPROC_LIB_NAME,
                " - no stored data found for requested range\n");
        }

        *ntimevals = (size_t)ndsfiles;
        return((timeval_t *)NULL);
    }

    /************************************************************
    *  Fetch the times
    *************************************************************/

    timevals = _dsproc_fetch_timevals(
        ds, ndsfiles, dsfiles, begin_timeval, end_timeval, ntimevals, timevals);

    free(dsfiles);

    if (!timevals) {

        if (*ntimevals == 0) {
            DEBUG_LV1( DSPROC_LIB_NAME,
                " - no stored data found for requested range\n");
        }

        return((timeval_t *)NULL);
    }

    if (msngr_debug_level || msngr_provenance_level) {

        format_timeval(&timevals[0],              ts1);
        format_timeval(&timevals[*ntimevals - 1], ts2);

        DEBUG_LV1( DSPROC_LIB_NAME,
            " - data begin:   %s\n"
            " - data end:     %s\n", ts1, ts2);
    }

    return(timevals);
}
