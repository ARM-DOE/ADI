/*******************************************************************************
*
*  Copyright © 2014, Battelle Memorial Institute
*  All rights reserved.
*
********************************************************************************
*
*  Authors:
*     name:  Brian Ermold
*     phone: (509) 375-2277
*     email: brian.ermold@pnl.gov
*
*     name:  Krista Gaustad
*     phone: (509) 375-5950
*     email: krista.gaustad@pnl.gov
*
*******************************************************************************/

/** @file dsproc_retriever.c
 *  Retriever Functions.
 */

#include "dsproc3.h"
#include "dsproc_private.h"

extern DSProc *_DSProc; /**< Internal DSProc structure */

/** @privatesection */

/*******************************************************************************
 *  Static Data and Functions Visible Only To This Module
 */

static time_t    _RetData_BaseTime;
static timeval_t _RetData_EndTime;
static char      _RetData_TimeDesc[64];
static char      _RetData_TimeUnits[64];

/**
 *  Static: Add a CDS variable to a CDS variable group.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  group_name - name of the variable group
 *  @param  array_name - name of variable array
 *  @param  var        - pointer to the variable to add
 *
 *  @return
 *    -  pointer to the CDSVarGroup structure
 *    -  NULL if an error occurrs
 */
static CDSVarGroup* _dsproc_add_var_to_vargroup(
    const char *group_name,
    const char *array_name,
    CDSVar     *var)
{
    CDSVarGroup *var_group;
    CDSVarArray *var_array;

    var_group = cds_define_vargroup(_DSProc->ret_data, group_name);
    if(!var_group) {
        dsproc_set_status(DSPROC_ERETRIEVER);
        return((CDSVarGroup *)NULL);
    }

    var_array = cds_add_vargroup_vars(var_group, array_name, 1, &var);
    if(!var_array) {
        dsproc_set_status(DSPROC_ERETRIEVER);
        return((CDSVarGroup *)NULL);
    }

    return(var_group);
}

/**
 *  Static: Free all memory used by a RetDsFile structure.
 */
static void _dsproc_free_ret_ds_file(RetDsFile *file)
{
    if (file) {

        if (file->version_string) free(file->version_string);
        if (file->dimids)         free(file->dimids);
        if (file->dim_names)      ncds_free_list(file->ndims, file->dim_names);
        if (file->dim_lengths)    free(file->dim_lengths);
        if (file->is_unlimdim)    free(file->is_unlimdim);

        free(file);
    }
}

/**
 *  Static: Cleanup input data loaded by the retriever.
 *
 *  This function will cleanup all data loaded by the retriever
 *  and prepare it to load data for the next processing interval.
 */
static void _dsproc_cleanup_retrieved_data(void)
{
    DataStream *in_ds;
    RetDsCache *cache;
    int         in_dsid;
    int         fi;

    /* Cleanup old retriever data and references in the input datastreams */

    for (in_dsid = 0; in_dsid < _DSProc->ndatastreams; in_dsid++) {

        in_ds = _DSProc->datastreams[in_dsid];

        if (!in_ds->ret_cache) {
            continue;
        }

        cache = in_ds->ret_cache;

        for (fi = 0; fi < cache->nfiles; fi++) {
            _dsproc_free_ret_ds_file(cache->files[fi]);
        }

        if (cache->files) free(cache->files);

        cache->begin_time = 0;
        cache->end_time   = 0;
        cache->ds_group   = (CDSGroup *)NULL;
        cache->files      = (RetDsFile **)NULL;
        cache->nfiles     = -1;
    }

    /* Cleanup the ret_data */

    if (_DSProc->ret_data) {
        cds_set_definition_lock(_DSProc->ret_data, 0);
        cds_delete_group(_DSProc->ret_data);
        _DSProc->ret_data = (CDSGroup *)NULL;
    }
}

/**
 *  Static: Initialize a retriever datastream.
 *
 *  This function will:
 *
 *    - create the input datastream entry in the _DSProc->datastreams
 *    - find and set the largest start and end time offsets
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  ret_group - pointer to the RetDsGroup
 *  @param  ret_ds    - pointer to the RetDataStream
 *
 *  @return
 *    -  1 if successful
 *    -  0 if an error occurred
 */
static int _dsproc_init_ret_datastream(
    RetDsGroup    *ret_group,
    RetDataStream *ret_ds)
{
    DataStream  *in_ds;
    RetDsCache  *cache;
    int          in_dsid;
    RetVariable *var;
    int          init_offsets;
    int          rvi;

    /* This will define the input datastream if it does not already exist,
     * or return the id of the existing datastream. */

    in_dsid = dsproc_init_datastream(
        ret_ds->site,
        ret_ds->facility,
        ret_ds->name,
        ret_ds->level,
        DSR_INPUT, NULL, 0, -1);

    if (in_dsid < 0) {
        return(0);
    }

    in_ds = _DSProc->datastreams[in_dsid];

    /* Initialize the retriever datastream cache */

    if (in_ds->ret_cache) {
        init_offsets = 0;
    }
    else {

        cache = (RetDsCache *)calloc(1, sizeof(RetDsCache));

        if (!cache) {

            ERROR( DSPROC_LIB_NAME,
                "Could not intialize datastream retriever cache for: %s"
                " -> memory allocation error\n",
                in_ds->name);

            dsproc_set_status(DSPROC_ENOMEM);
            return(0);
        }

        cache->nfiles    = -1;
        in_ds->ret_cache = cache;
        init_offsets     = 1;
    }

    cache = in_ds->ret_cache;

    /* Set the max start time and end time offsets */

    for (rvi = 0; rvi < ret_group->nvars; rvi++) {

        var = ret_group->vars[rvi];

        if (init_offsets) {
            cache->begin_offset = var->start_offset;
            cache->end_offset   = var->end_offset;
            init_offsets = 0;
        }
        else {

            if (cache->begin_offset < var->start_offset) {
                cache->begin_offset = var->start_offset;
            }

            if (cache->end_offset < var->end_offset) {
                cache->end_offset = var->end_offset;
            }
        }
    }

    /* Set the begin and end date dependencies */

    if (ret_ds->dep_begin_date) {

        if (cache->dep_begin_date &&
            cache->dep_begin_date != ret_ds->dep_begin_date) {

            ERROR( DSPROC_LIB_NAME,
                "Could not intialize retriever datastream: %s"
                " -> found conflicting begin date dependencies\n",
                in_ds->name);

            dsproc_set_status(DSPROC_EBADRETRIEVER);
            return(0);
        }

        cache->dep_begin_date = ret_ds->dep_begin_date;
    }

    if (ret_ds->dep_end_date) {

        if (cache->dep_end_date &&
            cache->dep_end_date != ret_ds->dep_end_date) {

            ERROR( DSPROC_LIB_NAME,
                "Could not intialize retriever datastream: %s"
                " -> found conflicting end date dependencies in retriever\n",
                in_ds->name);

            dsproc_set_status(DSPROC_EBADRETRIEVER);
            return(0);
        }

        cache->dep_end_date = ret_ds->dep_end_date;
    }

    return(1);
}

/**
 *  Static: Initialize an input file.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  in_ds  - pointer to the input datastream in _DSProc
 *  @param  dsfile - pointer to the DSFile structure.
 *
 *  @return
 *    -  1 if successful
 *    -  0 if no data was found for the desired time range
 *    - -1 if an error occurred
 */
static int _dsproc_init_ret_dsfile(
    DataStream *in_ds,
    DSFile     *dsfile)
{
    RetDsCache  *cache = in_ds->ret_cache;
    timeval_t    ret_begin_time = { 0, 0};
    timeval_t    ret_end_time   = { 0, 0};
    timeval_t    file_begin_time;
    timeval_t    file_end_time;

    RetDsFile   *ret_file;
    char        *last_dot;
    CDSGroup    *obs_group;

    CDSVar      *time_var;
    size_t       nsamples;
    CDSAtt      *units_att;
    time_t       bt_status;
    char         units_string[64];
    double      *time_offsets;
    int          start_index;
    int          end_index;
    int          count;

    CDSVar      *time_bounds_var;
    int          tb_start;
    int          tb_count;
    double      *time_bounds;

//    int          qc_varid;
//    CDSVar      *qc_var;

    CDSAtt      *att;
    int          status;
    int          ai, t1, t2;

    int          skip_file;
    char         ts1[32], ts2[32];

    if (dsfile->ntimes <= 0) {
        return(0);
    }

    /* Open the input file if it is not already open */

    if (!_dsproc_open_dsfile(dsfile, 0)) {
        return(-1);
    }

    /* Check if the times are within the current processing interval */

    ret_begin_time.tv_sec = cache->begin_time;
    ret_end_time.tv_sec   = cache->end_time;
    file_begin_time       = dsfile->timevals[0];
    file_end_time         = dsfile->timevals[dsfile->ntimes-1];
    skip_file             = 0;

    if (in_ds->flags & DS_PRESERVE_OBS) {

        /* For observation based retrievals we want all complete observations
         * that begin within the current processing interval. */

        if (TV_LT  (file_begin_time, ret_begin_time) ||
            TV_GTEQ(file_begin_time, ret_end_time  )) {

            skip_file = 1;
        }
        else {

            /* Adjust the _RetData_EndTime value if necessary */

            if (TV_LT(_RetData_EndTime, file_end_time)) {
                _RetData_EndTime = file_end_time;
                _RetData_EndTime.tv_usec += 1;
                if (_RetData_EndTime.tv_usec >= 1e6) {
                    _RetData_EndTime.tv_sec  += 1;
                    _RetData_EndTime.tv_usec -= 1e6;
                }
            }
        }
    }
    else {

        if (TV_LT(ret_end_time, _RetData_EndTime)) {
            ret_end_time = _RetData_EndTime;
        }

        if (TV_GTEQ(file_begin_time, ret_end_time) ||
            TV_LT  (file_end_time,   ret_begin_time)) {

            skip_file = 1;
        }
    }

    if (!skip_file) {

        /* Get the start and end time indexes for the current processing,
         * interval, and verify it does not fit within a gap in the file. */

        start_index = cds_find_timeval_index(
            dsfile->ntimes, dsfile->timevals, ret_begin_time, CDS_GTEQ);

        end_index = cds_find_timeval_index(
            dsfile->ntimes, dsfile->timevals, ret_end_time, CDS_LT);

        count = end_index - start_index + 1;

        if (end_index < start_index) {

            DEBUG_LV1( DSPROC_LIB_NAME,
                " - found gap:  ['%s', '%s']\n",
                format_timeval(&dsfile->timevals[end_index],   ts1),
                format_timeval(&dsfile->timevals[start_index], ts2));

            skip_file = 1;
        }
    }

    if (skip_file) {

        DEBUG_LV1( DSPROC_LIB_NAME,
            " - skipping:   %s ['%s', '%s']\n",
            dsfile->name,
            format_timeval(&file_begin_time, ts1),
            format_timeval(&file_end_time,   ts2));

        return(0);
    }

    DEBUG_LV1( DSPROC_LIB_NAME,
        " - found:      %s ['%s', '%s']\n",
        dsfile->name,
        format_timeval(&file_begin_time, ts1),
        format_timeval(&file_end_time,   ts2));

    /* Create the RetDsFile structure for this file */

    ret_file = (RetDsFile *)calloc(1, sizeof(RetDsFile));
    if (!ret_file) {

        ERROR( DSPROC_LIB_NAME,
            "Could not get file list for input datastream: %s\n"
            " -> memory allocation error",
            in_ds->name);

        dsproc_set_status(DSPROC_ENOMEM);
        return(-1);
    }

    ret_file->dsfile = dsfile;

    cache->files[cache->nfiles] = ret_file;
    cache->nfiles += 1;

    /* Create the CDS "observation" group for this file */

    last_dot = strrchr(dsfile->name, '.');
    if (last_dot) *last_dot = '\0';

    obs_group = cds_define_group(cache->ds_group, dsfile->name);

    if (last_dot) *last_dot = '.';

    if (!obs_group) {

        ERROR( DSPROC_LIB_NAME,
            "Could not create observation group for input file: %s\n",
            dsfile->name);

        dsproc_set_status(DSPROC_ERETRIEVER);
        return(-1);
    }

    ret_file->obs_group = obs_group;

    /* Read in the time variable */

    nsamples = 0;
    time_var = ncds_get_var_by_id(
        dsfile->ncid,
        dsfile->time_varid,
        0,
        &nsamples,
        obs_group,
        "time",
        CDS_DOUBLE,
        NULL, 0, 0, NULL, NULL, NULL, NULL);

    if (!time_var) {

        ERROR( DSPROC_LIB_NAME,
            "Could not read time variable from input file: %s\n",
            dsfile->name);

        dsproc_set_status(DSPROC_ENCREAD);
        return(-1);
    }

    if (nsamples != (size_t)dsfile->ntimes) {

        ERROR( DSPROC_LIB_NAME,
            "Could not read time variable from input file: %s\n"
            " -> number of times in DSFile struct: %d\n"
            " -> sample_count of time variable:    %d\n",
            dsfile->name, dsfile->ntimes, (int)nsamples);

        dsproc_set_status(DSPROC_ENCREAD);
        return(-1);
    }

    /* Fix time units that are not recognized by UDUNITS  */

    bt_status = -1;
    units_att = cds_get_att(time_var, "units");

    if (units_att &&
        units_att->type == CDS_CHAR) {

        bt_status = cds_validate_time_units(units_att->value.cp);
        if (bt_status < -1) {

            ERROR( DSPROC_LIB_NAME,
                "Could not validate time units in input file: %s\n",
                dsfile->name);

            dsproc_set_status("Could Not Validate Time Units");
            return(-1);
        }
    }

    if (bt_status < 0) {

        /* Get units string using base_time */

        if (!cds_base_time_to_units_string(dsfile->base_time, units_string)) {

            ERROR( DSPROC_LIB_NAME,
                "Could not fix time units in input file: %s\n",
                dsfile->name);

            dsproc_set_status("Could Not Fix Time Units");
            return(-1);
        }

        if (units_att) {

            /* Update units attribute */

            DEBUG_LV1( DSPROC_LIB_NAME,
                " - fixing invalid time variable units:\n"
                "    - from: '%s'\n"
                "    - to:   '%s'\n",
                units_att->value.cp, units_string);

            if (!cds_change_att_text(units_att, "%s", units_string)) {

                ERROR( DSPROC_LIB_NAME,
                    "Could not fix time units in input file: %s\n"
                    " -> memory allocation error",
                    dsfile->name);

                dsproc_set_status(DSPROC_ENOMEM);
                return(-1);
            }
        }
        else {

            /* Create units attribute */

            DEBUG_LV1( DSPROC_LIB_NAME,
                " - adding missing time variable units: '%s'\n",
                units_string);

            if (!cds_define_att_text(time_var, "units", units_string)) {

                ERROR( DSPROC_LIB_NAME,
                    "Could not fix time units in input file: %s\n"
                    " -> memory allocation error",
                    dsfile->name);

                dsproc_set_status(DSPROC_ENOMEM);
                return(-1);
            }
        }
    }

    /* Get the start and end indexes of the time_offsets
     * within the current processing interval. */

    if (in_ds->flags & DS_PRESERVE_OBS) {
        ret_file->sample_start = 0;
        ret_file->sample_count = nsamples;
    }
    else {

        /* Shift time offsets */

        time_offsets = time_var->data.dp;

        for (t1 = 0, t2 = start_index; t1 < count; t1++, t2++) {
            time_offsets[t1] = time_offsets[t2];
        }

        /* Shift time_bounds offsets if they exists */

        time_bounds_var = cds_get_bounds_var(time_var);
        if (time_bounds_var) {

            tb_start = start_index * 2;
            tb_count = count * 2;

            time_bounds = time_bounds_var->data.dp;

            for (t1 = 0, t2 = tb_start; t1 < tb_count; t1++, t2++) {
                time_bounds[t1] = time_bounds[t2];
            }

            time_bounds_var->sample_count = (size_t)count;
        }

        time_var->dims[0]->length = (size_t)count;
        time_var->sample_count    = (size_t)count;
        ret_file->sample_start    = (size_t)start_index;
        ret_file->sample_count    = (size_t)count;
    }

    /* Adjust the base time to be consistent with all retrieved data. */

    if (dsfile->base_time != _RetData_BaseTime) {

        if (!cds_set_base_time(
            time_var, _RetData_TimeDesc, _RetData_BaseTime)) {

            ERROR( DSPROC_LIB_NAME,
                "Could not set base time value for data read from input file: %s\n",
                dsfile->name);

            dsproc_set_status(DSPROC_ERETRIEVER);
            return(-1);
        }
    }

    /* Cache the start and end times of the records loaded from this file */

    dsproc_get_time_range(
        obs_group,
        &(ret_file->start_time),
        &(ret_file->end_time));

    /* Load qc_time variable */

/* BDE: removing depricated qc_time variable
*
*    status = ncds_inq_varid(dsfile->ncid, "qc_time", &qc_varid);
*
*    if (status < 0) {
*
*        ERROR( DSPROC_LIB_NAME,
*            "Could not get qc_time varid from input file: %s\n",
*            dsfile->name);
*
*        dsproc_set_status(DSPROC_ENCREAD);
*        return(-1);
*    }
*
*    if (status > 0) {
*
*        nsamples = ret_file->sample_count;
*
*        qc_var = ncds_get_var_by_id(
*            dsfile->ncid,
*            qc_varid,
*            ret_file->sample_start,
*            &nsamples,
*            obs_group,
*            NULL, CDS_NAT, NULL, 0, 0, NULL, NULL, NULL, NULL);
*
*        if (!qc_var) {
*
*            ERROR( DSPROC_LIB_NAME,
*                "Could not get qc_time values from input file: %s\n",
*                dsfile->name);
*
*            dsproc_set_status(DSPROC_ENCREAD);
*            return(-1);
*        }
*    }
*/

    /* Load global attributes */

    status = ncds_read_atts(dsfile->ncid, obs_group);

    if (status < 0) {

        ERROR( DSPROC_LIB_NAME,
            "Could not get global attributes from input file: %s\n",
            dsfile->name);

        dsproc_set_status(DSPROC_ENCREAD);
        return(-1);
    }

    /* Get the dimension information for this file */

    ret_file->ndims = ncds_get_group_dim_info(
        dsfile->ncid, 0,
        &(ret_file->dimids),
        &(ret_file->dim_names),
        &(ret_file->dim_lengths),
        &(ret_file->is_unlimdim));

    if (ret_file->ndims < 0) {

        ERROR( DSPROC_LIB_NAME,
            "Could not get dimension information from input file: %s\n",
            dsfile->name);

        dsproc_set_status(DSPROC_ENCREAD);
        return(-1);
    }

    /* Attempt to find and cache a global version attribute */

    ret_file->version_string = (char *)NULL;

    for (ai = 0; ai < obs_group->natts; ++ai) {

        att = obs_group->atts[ai];

        if (att->type == CDS_CHAR) {

            if ((strcmp(att->name, "process_version") == 0) ||
                (strcmp(att->name, "ingest_version")  == 0) ||
                (strcmp(att->name, "Version")         == 0) ||
                (strcmp(att->name, "ingest_software") == 0) ||
                (strcmp(att->name, "ingest-software") == 0)) {

                ret_file->version_string = strdup(att->value.cp);
                break;
            }
        }
    }

    return(1);
}

/**
 *  Static: Open all files for a datastream that are within the current
 *  processing interval.
 *
 *  The first time this function is called for a datastream it will create
 *  the list of files that are within the current processing interval, and
 *  then create the observation group for each file and load the time data
 *  and global attributes. Subsequent calls to this function will return
 *  the cached list.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  in_ds     - pointer to the input datastream in _DSProc
 *  @param  ret_files - output: pointer to the RetDsFile list.
 *
 *  @return
 *    -  1 if successful
 *    -  0 if no data files were found for the desired time range
 *    - -1 if an error occurred
 */
static int _dsproc_open_ret_ds_files(
    DataStream *in_ds, RetDsFile ***ret_files)
{
    timeval_t    begin_timeval = { 0, 0 };
    timeval_t    end_timeval   = { 0, 0 };
    RetDsCache  *cache;
    int          ndsfiles;
    DSFile     **dsfiles;
    DSFile      *dsfile;
    RetDsFile   *ret_file;
    int          status;
    int          fi;
    char         ts1[32], ts2[32];

    /* Check if the file list is already cached */

    cache = in_ds->ret_cache;

    if (cache->nfiles >= 0) {

        DEBUG_LV1( DSPROC_LIB_NAME,
            "%s: Using cached input files list\n",
            in_ds->name);

        if (cache->nfiles == 0) {
            DEBUG_LV1( DSPROC_LIB_NAME,
                " - no files previously found\n");
            *ret_files = (RetDsFile **)NULL;
            return(0);
        }

        for (fi = 0; fi < cache->nfiles; fi++) {

            ret_file = cache->files[fi];
            dsfile   = ret_file->dsfile;

            DEBUG_LV1( DSPROC_LIB_NAME,
                " - %s\n", dsfile->full_path);
        }

        *ret_files = cache->files;
        return(cache->nfiles);
    }

    *ret_files = (RetDsFile **)NULL;

    /* Find all input files within the specified range
     *
     * For observation based retrievals we want all complete observations
     * that begin within the current processing interval.
     *
     * For all other retrievals we want all data up through the end time
     * of any previous observation base retrievals. */

    begin_timeval.tv_sec = cache->begin_time;

    if ((in_ds->flags & DS_PRESERVE_OBS) ||
        (cache->end_time > _RetData_EndTime.tv_sec)) {

        end_timeval.tv_sec = cache->end_time;
    }
    else {
        end_timeval = _RetData_EndTime;
    }

    DEBUG_LV1( DSPROC_LIB_NAME,
        "%s: Checking for input files\n"
        " - path:       %s\n"
        " - interval:   ['%s', '%s')\n",
        in_ds->name, in_ds->dir->path,
        format_secs1970(begin_timeval.tv_sec, ts1),
        format_secs1970(end_timeval.tv_sec,   ts2));

    ndsfiles = _dsproc_find_dsfiles(
        in_ds->dir,
        &begin_timeval,
        &end_timeval,
        &dsfiles);

    if (ndsfiles < 0) {
        return(-1);
    }
    else if (ndsfiles == 0) {

        DEBUG_LV1( DSPROC_LIB_NAME,
            " - no input files found\n");

        cache->nfiles = 0;
        return(0);
    }

    /* Create the RetDsFile list */

    cache->nfiles = 0;
    cache->files  = (RetDsFile **)calloc(ndsfiles, sizeof(RetDsFile *));

    if (!cache->files) {

        ERROR( DSPROC_LIB_NAME,
            "Could not get file list for input datastream: %s\n"
            " -> memory allocation error",
            in_ds->name);

        free(dsfiles);
        dsproc_set_status(DSPROC_ENOMEM);
        return(-1);
    }

    *ret_files = cache->files;

    /* Loop over all files and create the RetDsFile structures
     * and "observation" CDSGroup structures */

    for (fi = 0; fi < ndsfiles; fi++) {

        status = _dsproc_init_ret_dsfile(in_ds, dsfiles[fi]);

        if (status < 0) {
            free(dsfiles);
            return(-1);
        }

    } /* end files loop */

    free(dsfiles);

    if (cache->nfiles == 0) {
        DEBUG_LV1( DSPROC_LIB_NAME,
            " - no input data found\n");
    }

    return(cache->nfiles);
}

/**
 *  Static: Get information about a variable in an input file.
 *
 *  Specify NULL for the output arguments that are not required.
 *
 *  The dimids, dim_names, dim_lengths, and is_unlimdim arguments must point
 *  to arrays that are large enough to hold the number of values that will be
 *  returned. It is recommended that NC_MAX_DIMS be used to define the length
 *  of these arrays in the calling function.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  ret_file    - pointer to the RetDsFile
 *  @param  var_name    - name of the variable
 *  @param  varid       - output: variable ID
 *  @param  data_type   - output: variable data type
 *  @param  ndims       - output: number of variable dimensions
 *  @param  dimids      - output: IDs of the variable's dimensions
 *  @param  dim_names   - output: names of the variable's dimensions
 *  @param  dim_lengths - output: lengths of the variable's dimensions
 *  @param  is_unlimdim - output: flags indicating if a dimension is unlimited
 *                                (1 == TRUE, 0 == FALSE)
 *
 *  @return
 *    -  1 if the variable was found
 *    -  0 if the variable was not found
 *    - -1 if an error occurred
 */
static int _dsproc_get_ret_file_var_info(
    RetDsFile   *ret_file,
    const char  *var_name,
    int         *varid,
    CDSDataType *data_type,
    int         *ndims,
    int         *dimids,
    const char **dim_names,
    size_t      *dim_lengths,
    int         *is_unlimdim)
{
    DSFile *dsfile = ret_file->dsfile;
    int     var_id;
    nc_type var_type;
    int     var_ndims;
    int     var_dimids[NC_MAX_DIMS];
    int     status;
    int     vdi, fdi;

    if (varid) *varid = 0;
    if (ndims) *ndims = 0;

    /* Get the varid. */

    status = ncds_inq_varid(dsfile->ncid, var_name, &var_id);

    if (status < 0) {
        dsproc_set_status(DSPROC_ENCREAD);
        return(-1);
    }

    if (status == 0) {
        return(0);
    }

    if (varid) *varid = var_id;

    /* Get the variable data type. */

    if (data_type) {

        if (!ncds_inq_vartype(dsfile->ncid, var_id, &var_type)) {

            ERROR( DSPROC_LIB_NAME,
                "Could not get variable data type for: %s->%s\n",
                dsfile->name, var_name);

            dsproc_set_status(DSPROC_ENCREAD);
            return(-1);
        }

        *data_type = ncds_cds_type(var_type);
    }

    /* Get the number of dimensions. */

    if (!ncds_inq_varndims(dsfile->ncid, var_id, &var_ndims)) {

        ERROR( DSPROC_LIB_NAME,
            "Could not get number of variable dimensions for: %s->%s\n",
            dsfile->name, var_name);

        dsproc_set_status(DSPROC_ENCREAD);
        return(-1);
    }

    if (var_ndims <= 0) {
        return(1);
    }

    if (ndims) *ndims = var_ndims;

    /* Get the dimension IDs. */

    if (!dimids && !dim_names && !dim_lengths && !is_unlimdim) {
        return(1);
    }

    if (!ncds_inq_vardimids(dsfile->ncid, var_id, var_dimids)) {

        ERROR( DSPROC_LIB_NAME,
            "Could not get variable dimension ids for: %s->%s\n",
            dsfile->name, var_name);

        dsproc_set_status(DSPROC_ENCREAD);
        return(-1);
    }

    if (dimids) memcpy(dimids, var_dimids, var_ndims * sizeof(int));

    /* Get the dimension names and lengths. */

    if (!dim_names && !dim_lengths && !is_unlimdim) {
        return(1);
    }

    for (vdi = 0; vdi < var_ndims; vdi++) {

        for (fdi = 0; fdi < ret_file->ndims; fdi++) {

            if (var_dimids[vdi] == ret_file->dimids[fdi]) {

                if (dim_names)   dim_names[vdi]   = ret_file->dim_names[fdi];
                if (dim_lengths) dim_lengths[vdi] = ret_file->dim_lengths[fdi];
                if (is_unlimdim) is_unlimdim[vdi] = ret_file->is_unlimdim[fdi];

                break;
            }
        }

        if (fdi == ret_file->ndims) {

            /* This should never happen */

            ERROR( DSPROC_LIB_NAME,
                "Could not find variable dimension ID (%d) in file cache for: %s->%s\n",
                var_dimids[vdi], dsfile->name, var_name);

            dsproc_set_status(DSPROC_ERETRIEVER);
            return(-1);
        }

    } /* end loop over var_ndims */

    return(1);
}

/**
 *  Static: Retrieve variable data from a NetCDF file.
 *
 *  This function retrieves and populates a cds group with the specified
 *  variable data. All dimensions and coordinate variables that have not
 *  already been retrieved will also be loaded.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  in_ds     - pointer to the input datastream in _DSProc
 *  @param  ret_file  - pointer to the RetDsFile
 *  @param  ret_group - pointer to the RetDsGroup
 *  @param  ret_ds    - pointer to the RetDataStream
 *  @param  ret_var   - pointer to the RetVariable
 *
 *  @return
 *    -  1 if the variable and QC variable (if required) were found
 *    -  0 if the variable or required qc variable was not found
 *    - -1 if an error occurred
 */
static int _dsproc_retrieve_variable(
    DataStream     *in_ds,
    RetDsFile      *ret_file,
    RetDsGroup     *ret_group,
    RetDataStream  *ret_ds,
    RetVariable    *ret_var)
{
    int             dynamic_dod = dsproc_get_dynamic_dods_mode();
    DSFile         *dsfile      = ret_file->dsfile;
    RetDsVarMap    *varmap;
    RetCoordSystem *coordsys;
    RetCoordDim    *coorddim;

    const char     *var_name;
    int             varid;
    int             var_ndims;
    const char     *var_dim_names[NC_MAX_DIMS];
    size_t          var_dim_lengths[NC_MAX_DIMS];

    char            qc_var_name[NC_MAX_NAME];
    int             qc_varid;
    CDSDataType     qc_var_type;
    int             qc_var_ndims;
    const char     *qc_var_dim_names[NC_MAX_DIMS];
    int             retrieve_qc;

    CDSDataType     ret_var_type;
    const char     *ret_dim_names[NC_MAX_DIMS];
    CDSDataType     ret_dim_types[NC_MAX_DIMS];
    const char     *ret_dim_units[NC_MAX_DIMS];
    char            ret_qc_var_name[NC_MAX_NAME];

    CDSGroup       *obs_group;
    CDSVar         *obs_var;
    CDSVar         *obs_qc_var;
    char            obs_qc_var_name[NC_MAX_NAME];

    size_t          sample_start;
    size_t          sample_count;

    int             status;
    int             di, mi, ni, csdi;

    /* Prevent compiler warning */

    var_name = NULL;

    /**********************************************************************
    * Get the list of all possible names this variable can
    * have for this input datastream.
    **********************************************************************/

    varmap = (RetDsVarMap *)NULL;

    for (mi = 0; mi < ret_var->nvarmaps; mi++) {
        if (ret_var->varmaps[mi]->ds == ret_ds) {
            varmap = ret_var->varmaps[mi];
            break;
        }
    }

    if (!varmap) {

        /* This should never happen */

        ERROR( DSPROC_LIB_NAME,
            "Could not find variable names for %s->%s in datastream: %s%s%s.%s\n",
            ret_group->name, ret_var->name,
            ret_ds->site, ret_ds->name, ret_ds->facility, ret_ds->level);

        dsproc_set_status(DSPROC_EBADRETRIEVER);
        return(-1);
    }

    /* Open the input file if it is not already open */

    if (!_dsproc_open_dsfile(dsfile, 0)) {
        return(-1);
    }

    /**********************************************************************
    * Check if this variable and QC variable (if required)
    * can be found in this file.
    **********************************************************************/

    retrieve_qc = 0;

    for (ni = 0; ni < varmap->nnames; ni++) {

        var_name = varmap->names[ni];

        /* Check if this variable exists in the input file */

        status = _dsproc_get_ret_file_var_info(
            ret_file, var_name,
            &varid, NULL,
            &var_ndims, NULL, var_dim_names, var_dim_lengths, NULL);

        if (status < 0) {
            return(-1);
        }

        if (status == 0) {
            continue;
        }

        /* Check if the QC variable exists if it was requested */

        if (ret_var->retrieve_qc) {

            snprintf(qc_var_name, NC_MAX_NAME, "qc_%s", var_name);

            status = _dsproc_get_ret_file_var_info(
                ret_file, qc_var_name,
                &qc_varid, &qc_var_type,
                &qc_var_ndims, NULL, qc_var_dim_names, NULL, NULL);

            if (status < 0) {
                return(-1);
            }

            if (status == 0) {
                if (ret_var->qc_req_to_run) {
                    continue;
                }
                break;
            }

            /* Make sure the QC variable has an integer data type */

/* BDE: I think this check should be done in the QC logic... This will give
   the user the chance to fix the variable in the post_retrieval_hook().

            if (qc_var_type != CDS_INT) {

                ERROR( DSPROC_LIB_NAME,
                    "Found unsupported QC variable data type '%s' for: %s->%s\n",
                    cds_data_type_name(qc_var_type),
                    dsfile->name, qc_var_name);

                dsproc_set_status(DSPROC_ERETRIEVER);
                return(-1);
            }
*/
            /* Make sure this QC variable has the correct dimensionality */

            if (var_ndims > 0) {

                if ((qc_var_ndims <= 0) ||
                    (strcmp(var_dim_names[0], qc_var_dim_names[0]) != 0)) {

                    ERROR( DSPROC_LIB_NAME,
                        "Dimensionality of QC variable %s does not match variable: %s->%s\n",
                        qc_var_name, dsfile->name, var_name);

                    dsproc_set_status(DSPROC_ERETRIEVER);
                    return(-1);
                }
            }
            else if (qc_var_ndims > 0) {

                ERROR( DSPROC_LIB_NAME,
                    "Dimensionality of QC variable %s does not match variable: %s->%s\n",
                    qc_var_name, dsfile->name, var_name);

                dsproc_set_status(DSPROC_ERETRIEVER);
                return(-1);
            }

            retrieve_qc = 1;
        }

        break;

    } /* end loop over varmap names */

    if (ni == varmap->nnames) {

        DEBUG_LV1( DSPROC_LIB_NAME,
            "%s\n"
            " - not found:  %s\n",
            ret_var->name, dsfile->name);

        return(0);
    }

    /**********************************************************************
    * If we get here we found the variable, and QC variable if applicable.
    **********************************************************************/

    if (msngr_debug_level || msngr_provenance_level) {

        if (strlen(ret_var->name) < 7) {
            DEBUG_LV1( DSPROC_LIB_NAME,
                "%s:\t\t %s->%s\n",
                ret_var->name, dsfile->name, var_name);
        }
        else {
            DEBUG_LV1( DSPROC_LIB_NAME,
                "%s:\t %s->%s\n",
                ret_var->name, dsfile->name, var_name);
        }
    }

    /**********************************************************************
    * Check if a variable with this name already exists in the obs_group.
    * This can happen if a coordinate variable with this name was auto-loaded.
    *
    * In this case we want to replace the existing variable with the one
    * explicitly requested by the user in the retriever definition.
    *
    **********************************************************************/

    obs_group = ret_file->obs_group;
    obs_var   = cds_get_var(obs_group, ret_var->name);

    if (obs_var) {

        /* Make sure the dimensionality of the variable in the obs_group
         * matches the dimensionality of the variable we are reading in.
         * If they don't match then there is a dimension/variable name
         * conflict in the retriever definition. */

        if (obs_var->ndims != var_ndims) {

            ERROR( DSPROC_LIB_NAME,
                "Dimension name conflicts with variable name in retriever definition\n"
                " -> number of dimensions do not match for: %s->%s",
                ret_group->name, ret_var->name);

            dsproc_set_status(DSPROC_EBADRETRIEVER);
            return(-1);
        }

        for (di = 0; di < var_ndims; di++) {

            if (var_dim_lengths[di] != obs_var->dims[di]->length) {

                ERROR( DSPROC_LIB_NAME,
                    "Dimension name conflicts with variable name in retriever definition\n"
                    " -> dimension lengths do not match for: %s->%s",
                    ret_group->name, ret_var->name);

                dsproc_set_status(DSPROC_EBADRETRIEVER);
                return(-1);
            }
        }

        /* If we get here it is safe to replace the previously
         * loaded variable with the new one. */

        cds_delete_var(obs_var);

        /* Remove the companion QC variable also because it will no longer
         * be valid, and will also be replaced if it was requested. */

        snprintf(obs_qc_var_name, NC_MAX_NAME, "qc_%s", ret_var->name);

        obs_qc_var = cds_get_var(obs_group, obs_qc_var_name);

        if (obs_qc_var) {
            cds_delete_var(obs_qc_var);
        }

    } /* end if obs_var exists */

    /**********************************************************************
    * Get the user defined dimension names from the retriever definition.
    * Dimension names that are not specified in the retriever will default
    * to the names found in the input file.
    *
    * With the introduction of the Caracena transformation method, it is
    * now possible for the dimensionality of the transformed variable to
    * be different that the retrieved variable.  In these cases it is not
    * currently possible to rename the dimensions from the input file.
    * We can detect these cases by checking if the number of ret_var
    * dimensions is greater than the number of input variable dimensions.
    *
    **********************************************************************/

    di = 0;

    if (ret_var->ndim_names <= var_ndims) {

        for (; di < ret_var->ndim_names; di++) {
            ret_dim_names[di] = ret_var->dim_names[di];
            ret_dim_types[di] = 0;
            ret_dim_units[di] = (char *)NULL;
        }
    }

    for (; di < var_ndims; di++) {
        ret_dim_names[di] = var_dim_names[di];
        ret_dim_types[di] = 0;
        ret_dim_units[di] = (char *)NULL;
    }

    /**********************************************************************
    * Search the coordinate system dimensions for any
    * coordinate variable data type and/or unit conversions.
    **********************************************************************/

    if (ret_var->coord_system) {

        coordsys = ret_var->coord_system;

        for (di = 0; di < var_ndims; di++) {

            if (strcmp(ret_dim_names[di], "time") == 0) {
                continue;
            }

            for (csdi = 0; csdi < coordsys->ndims; csdi++) {

                coorddim = coordsys->dims[csdi];

                if (strcmp(coorddim->name, ret_dim_names[di]) == 0) {

                    if (coorddim->data_type) {
                        ret_dim_types[di] = cds_data_type(coorddim->data_type);
                    }

                    if (coorddim->units) {
                        ret_dim_units[di] = coorddim->units;
                    }
                }

            } /* end loop over coordinate system dimensions */

        } /* end loop over variable dimensions */

    } /* end if coordinate system defined */

    /**********************************************************************
    * Load the variable and all associated coordinate variables that have
    * not already been loaded. The ncds_get_var_by_id function will also
    * do all necessary data type and unit conversions.
    **********************************************************************/

    /* Set sample start and sample count values */

    if (var_ndims > 0 &&
        strcmp(ret_dim_names[0], "time") == 0) {

        sample_start = ret_file->sample_start;
        sample_count = ret_file->sample_count;
    }
    else {
        sample_start = 0;
        sample_count = 0;
    }

    /* Set variable data type from retriever definition */

    if (ret_var->data_type) {
        ret_var_type = cds_data_type(ret_var->data_type);
    }
    else {
        ret_var_type = CDS_NAT;
    }

    /* Read in the data from the input file */

    obs_var = ncds_get_var_by_id(
        dsfile->ncid,
        varid,
        sample_start,
        &sample_count,
        ret_file->obs_group,
        ret_var->name,
        ret_var_type,
        ret_var->units,
        0,
        var_ndims,
        var_dim_names,
        ret_dim_names,
        ret_dim_types,
        ret_dim_units);

    if (!obs_var) {
        dsproc_set_status(DSPROC_ERETRIEVER);
        return(-1);
    }

    if (!_dsproc_add_var_to_vargroup(
        ret_var->name, obs_var->name, obs_var)) {

        return(-1);
    }

    if (!_dsproc_create_ret_var_tag(
        obs_var, ret_group, ret_var, in_ds, var_name)) {

        return(-1);
    }

    /**********************************************************************
    * Create the missing_value attribute if we are in dynamic DOD mode
    * and it is not already defined.  We also want to be careful *not*
    * to create a missing_value attribute for qc and coordinate variables.
    *
    * This has been updated to only create the missing_value attribute if a
    * non-standard missing value attribute was found, and will no longer create
    * a missing_value attribute with a value equal to the default fill value.
    *
    * This logic is only enabled when we are in dynamic DOD mode.
    * Otherwise, missing values should be mapped to the correct
    * value specified in the output DODs.
    **********************************************************************/

    if (dynamic_dod) {

        if (!cds_get_dim(ret_file->obs_group, obs_var->name) &&
            strncmp(obs_var->name, "qc_", 3)     != 0        &&
            strcmp(obs_var->name, "base_time")   != 0        &&
            strcmp(obs_var->name, "time_offset") != 0) {

            if (!cds_create_missing_value_att(obs_var, 1)) {

                ERROR( DSPROC_LIB_NAME,
                    "Could not create missing value attribute for: %s\n",
                    cds_get_object_path(obs_var));

                dsproc_set_status(DSPROC_ERETRIEVER);
                return(-1);
            }
        }
    }

    /**********************************************************************
    * Load the QC variable if it was requested, or create it if
    * there are any QC limits specified in the retriever definition.
    **********************************************************************/

    obs_qc_var = (CDSVar *)NULL;
    snprintf(ret_qc_var_name, NC_MAX_NAME, "qc_%s", ret_var->name);

    if (retrieve_qc) {

        if (msngr_debug_level || msngr_provenance_level) {

            if (strlen(ret_qc_var_name) < 7) {
                DEBUG_LV1( DSPROC_LIB_NAME,
                    "%s:\t\t %s->%s\n",
                    ret_qc_var_name, dsfile->name, qc_var_name);
            }
            else {
                DEBUG_LV1( DSPROC_LIB_NAME,
                    "%s:\t %s->%s\n",
                    ret_qc_var_name, dsfile->name, qc_var_name);
            }
        }

        /* Set sample start and sample count values */

        if (qc_var_ndims > 0 &&
            strcmp(ret_dim_names[0], "time") == 0) {

            sample_start = ret_file->sample_start;
            sample_count = ret_file->sample_count;
        }
        else {
            sample_start = 0;
            sample_count = 0;
        }

        /* Read in the data from the input file */

        obs_qc_var = ncds_get_var_by_id(
            dsfile->ncid,
            qc_varid,
            sample_start,
            &sample_count,
            ret_file->obs_group,
            ret_qc_var_name,
            CDS_NAT,
            NULL,
            0,
            var_ndims,
            var_dim_names,
            ret_dim_names,
            ret_dim_types,
            ret_dim_units);

        if (!obs_qc_var) {
            dsproc_set_status(DSPROC_ERETRIEVER);
            return(-1);
        }

        if (!_dsproc_add_var_to_vargroup(
            ret_var->name, obs_qc_var->name, obs_qc_var)) {

            return(-1);
        }
    }

/* BDE: This should be done in the QC logic that should be run after
   the post_retrieval_hook() and before the pre_transform_hook. This
   will give the user the opportunity to fix QC variables that are read
   in but are not type int.

    else if (ret_var->min || ret_var->max || ret_var->delta) {

        DEBUG_LV1( DSPROC_LIB_NAME,
            "Creating QC variable: %s->%s\n",
            ret_group->name, ret_qc_var_name);
    }
*/
    /**********************************************************************
    * BDE: We could apply the QC checks defined in retriever definition here.
    **********************************************************************/
/*
    if (ret_var->min || ret_var->max || ret_var->delta) {

        DEBUG_LV1( DSPROC_LIB_NAME,
            "Applying QC check to variable: %s->%s\n",
            ret_group->name, ret_var->name);

    }
*/
    return(1);
}

/**
 *  Static: Retrieve the variables for a datastream group from a datastream.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  in_ds           - pointer to the input datastream in _DSProc
 *  @param  ret_group       - pointer to the RetDsGroup
 *  @param  ret_ds          - pointer to the RetDataStream
 *  @param  found_var_flags - flags to track if a variable has been found
 *
 *  @return
 *    -  1 if successful
 *    -  0 if no data files were found for the desired time range
 *    - -1 if an error occurred
 */
static int _dsproc_retrieve_variables(
    DataStream    *in_ds,
    RetDsGroup    *ret_group,
    RetDataStream *ret_ds,
    int           *found_var_flags)
{
    RetDsFile  **ret_files;
    RetDsFile   *ret_file;
    RetVariable *ret_var;
    int          nfiles;
    int         *var_count;
    int          status;
    int          fi, vi;

    /* Allocate memory for the var_count array used to track if
     * a variable was found in this datastream. */

    var_count = (int *)calloc(ret_group->nvars, sizeof(int));

    if (!var_count) {

        ERROR( DSPROC_LIB_NAME,
            "Could not retrieve data from datastream: %s\n"
            " -> memory allocation error\n", in_ds->name);

        return(-1);
    }

    /* Loop over all files and load data */

    nfiles = _dsproc_open_ret_ds_files(in_ds, &ret_files);

    if (nfiles <= 0) {
        free(var_count);
        return(nfiles);
    }

    for (fi = 0; fi < nfiles; fi++) {

        ret_file = ret_files[fi];

        /* Loop over the variable groups and retrieve the data */

        for (vi = 0; vi < ret_group->nvars; vi++) {

            if (found_var_flags[vi]) {
                continue;
            }

            ret_var = ret_group->vars[vi];

            status = _dsproc_retrieve_variable(
                in_ds, ret_file, ret_group, ret_ds, ret_var);

            if (status < 0) {

                ERROR( DSPROC_LIB_NAME,
                    "Could not retrieve data for variable: %s->%s\n",
                    ret_group->name, ret_var->name);

                free(var_count);
                dsproc_set_status(DSPROC_ERETRIEVER);
                return(-1);
            }

            if (status == 1) {
                var_count[vi] += 1;
                ret_file->var_count += 1;
            }

        } /* end loop over variables */

    } /* end loop over files */

    /* Set the found_var_flags for the entire group */

    for (vi = 0; vi < ret_group->nvars; vi++) {
        if (var_count[vi] > 0) found_var_flags[vi] = 1;
    }

    free(var_count);

    return(1);
}

/**
 *  Static: Retrieve the data for a RetDsGroup.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  ret_group - pointer to the RetDsGroup
 *
 *  @return
 *    -  1 if successful
 *    -  0 if a required variable could not be found but data for the input
 *         datastream exists past the end of the current processing interval.
 *         If this is the case we can assume there is a gap in the data and
 *         the process should continue to the next processing interval.
 *    - -1 if an error occurred, or a required variable could not be found and
 *         data for the input datastream does not exist past the end of the
 *         current processing interval. If this is the case the process should
 *         wait for more input data to be created.
 */
static int _dsproc_retrieve_group(
    RetDsGroup    *ret_group)
{
    RetDsSubGroup *ret_subgroup;
    RetDataStream *ret_ds;
    RetDsCache    *cache;
    int           *found_var_flags;
    int            found_files;
    int            in_dsid;
    DataStream    *in_ds;
    int            status;
    int            rdsi, rvi;

    Mail          *warning_mail  = msngr_get_mail(MSNGR_WARNING);
    int            warning_count = 0;
    DataStream    *last_ds       = (DataStream *)NULL;
    int            ds_count      = 0;
    int            scan_mode     = 0;
    time_t         begin_time    = 0;
    time_t         end_time      = 0;
    char           ts1[32], ts2[32];

    timeval_t      search_begin    = { 0, 0 };
    timeval_t      fetched_timeval = { 0, 0 };
    int            wait_for_data   = 0;
    size_t         len;

    /* Allocate memory for a flags array to indicate if a variable was found */

    found_var_flags = (int *)calloc(ret_group->nvars, sizeof(int));
    found_files     = 0;

    if (!found_var_flags) {

        ERROR( DSPROC_LIB_NAME,
            "Could not retrieve data for group: %s\n"
            " -> memory allocation error\n", ret_group->name);

        dsproc_set_status(DSPROC_ENOMEM);
        return(-1);
    }

    /* Currently we only support one subgroup per group */

    ret_subgroup = ret_group->subgroups[0];

    for (rdsi = 0; rdsi < ret_subgroup->ndatastreams; rdsi++) {

        ret_ds = ret_subgroup->datastreams[rdsi];

        /* Get the _DSProc input datastream structure
         * or continue if the datastream should be skipped */

        in_dsid = dsproc_get_datastream_id(
            ret_ds->site,
            ret_ds->facility,
            ret_ds->name,
            ret_ds->level,
            DSR_INPUT);

        if (in_dsid < 0) {
            continue;
        }

        in_ds = _DSProc->datastreams[in_dsid];
        cache = in_ds->ret_cache;

        if (!cache ||
            !cache->ds_group) {

            continue;
        }

        /* Track information used for reporting log/warning messages */
        
        if (!begin_time || cache->begin_time < begin_time) {
            begin_time = cache->begin_time;
        }

        if ((in_ds->flags & DS_PRESERVE_OBS) ||
            (cache->end_time > end_time)) {

            end_time = cache->end_time;
        }
        else {
            end_time = _RetData_EndTime.tv_sec;
        }

        last_ds   = in_ds;
        ds_count += 1;

        if (in_ds->flags & DS_SCAN_MODE) {
            scan_mode = 1;
        }

        /* Retrieve data from this datastream for all variables that
         * have not yet been found. */

        status = _dsproc_retrieve_variables(
            in_ds, ret_group, ret_ds, found_var_flags);

        if (status < 0) {
            free(found_var_flags);
            return(-1);
        }

        if (status == 0) {

            /* No data files were found for this processing interval
             * so we need to check if this is a gap in the input data
             * or if we need to wait for the input data to be created. */

            search_begin.tv_sec = end_time;
            len = 1;

            if (!dsproc_fetch_timevals(in_dsid,
                &search_begin, NULL,
                &len, &fetched_timeval)) {

                wait_for_data = 1;
            }
        }
        else { // status > 0

            found_files = 1;

            /* Check if all variables have been found
            * before moving on to the next datastream */

            if (rdsi < ret_subgroup->ndatastreams - 1) {

                for (rvi = 0; rvi < ret_group->nvars; rvi++) {
                    if (!found_var_flags[rvi]) {
                        break;
                    }
                }

                if (rvi == ret_group->nvars) {
                    break;
                }
            }
        }

    } /* end loop over datastreams */

    /* Check if all required variables were found */

    status = 1;

    for (rvi = 0; rvi < ret_group->nvars; rvi++) {

        if (!found_var_flags[rvi] &&
            ret_group->vars[rvi]->req_to_run) {

            if (found_files) {

                if (warning_count == 0) {

                    if (warning_mail) {
                        mail_unset_flags(warning_mail, MAIL_ADD_NEWLINE);
                    }

                    WARNING( DSPROC_LIB_NAME,
                        "%s -> %s: Could not find data for required variables:\n",
                        format_secs1970(begin_time, ts1),
                        format_secs1970(end_time,   ts2));
                }

                warning_count++;

                if (ds_count == 1) {
                    WARNING( DSPROC_LIB_NAME,
                        " - %s->%s\n",
                        last_ds->name, ret_group->vars[rvi]->name);
                }
                else {
                    WARNING( DSPROC_LIB_NAME,
                        " - %s->%s\n",
                        ret_group->name, ret_group->vars[rvi]->name);
                }
            }

            status = 0;
        }
    }

    if (warning_count && warning_mail) {
        WARNING( DSPROC_LIB_NAME, "\n");
        mail_set_flags(warning_mail, MAIL_ADD_NEWLINE);
    }

    if (status == 0 && wait_for_data) {

        // We need to wait for more input data to be created.

        if (ds_count == 1) {

            LOG( DSPROC_LIB_NAME,
                "No data found for required datastream %s after %s\n"
                " -> waiting for input data before continuing",
                last_ds->name,
                format_secs1970(begin_time, ts1));
        }
        else {
            LOG( DSPROC_LIB_NAME,
                "No data found for required datastream group %s after %s\n"
                " -> waiting for input data before continuing",
                ret_group->name,
                format_secs1970(begin_time, ts1));
        }

        dsproc_set_status(DSPROC_ENODATA);
        status = -1;
    }
    else if (found_files == 0) {

        if (status == 1) {
            // no files found for optional data
            if (ds_count == 1) {
                LOG( DSPROC_LIB_NAME,
                    "Missing:    %s (optional)\n",
                    last_ds->name);
            }
            else {
                LOG( DSPROC_LIB_NAME,
                    "Missing:    %s (optional datastreams group)\n",
                    ret_group->name);
            }
        }
        else {
            // no files found for required datastream
            if (ds_count == 1) {

                if (scan_mode) {
                    LOG( DSPROC_LIB_NAME,
                        "Skipping:   No data found within processing interval for: %s\n",
                        last_ds->name);
                }
                else {
                    WARNING( DSPROC_LIB_NAME,
                        "%s -> %s: Could not find required data for: %s\n",
                        format_secs1970(begin_time, ts1),
                        format_secs1970(end_time,   ts2),
                        last_ds->name);
                }
            }
            else {
                if (scan_mode) {
                    LOG( DSPROC_LIB_NAME,
                        "Skipping:   No data found within processing interval for datastreams group: %s\n",
                        ret_group->name);
                }
                else {
                    WARNING( DSPROC_LIB_NAME,
                        "%s -> %s: Could not find required data for datastream group: %s\n",
                        format_secs1970(begin_time, ts1),
                        format_secs1970(end_time,   ts2),
                        ret_group->name);
                }
            }
        }
    }

    free(found_var_flags);

    return(status);
}

/*******************************************************************************
 *  Private Functions Visible Only To This Library
 */

/**
 *  Private: Free all memory used by a RetDsCache structure.
 */
void _dsproc_free_ret_ds_cache(RetDsCache *cache)
{
    int fi;

    if (cache) {

        for (fi = 0; fi < cache->nfiles; fi++) {
            _dsproc_free_ret_ds_file(cache->files[fi]);
        }

        if (cache->files) free(cache->files);

        free(cache);
    }
}

/**
 *  Private: Free the retriever and all associated data and structures.
 */
void _dsproc_free_retriever()
{
    DataStream *in_ds;
    int         in_dsid;

    /* Free old retriever data and references in the input datastreams */

    for (in_dsid = 0; in_dsid < _DSProc->ndatastreams; in_dsid++) {

        in_ds = _DSProc->datastreams[in_dsid];

        if (!in_ds->ret_cache) {
            continue;
        }

        _dsproc_free_ret_ds_cache(in_ds->ret_cache);

        in_ds->ret_cache = (RetDsCache *)NULL;
    }

    /* Free the retrieved data */

    if (_DSProc->ret_data) {
        cds_set_definition_lock(_DSProc->ret_data, 0);
        cds_delete_group(_DSProc->ret_data);
        _DSProc->ret_data = (CDSGroup *)NULL;
    }

    /* Free the retriever definition structure */

    if (_DSProc->retriever) {
        dsdb_free_retriever(_DSProc->retriever);
        _DSProc->retriever = (Retriever *)NULL;
    }
}

/**
 *  Private: Get a coordinate system from the retriever definition.
 *
 *  @param name - name of the coordinate system
 *
 *  @return
 *    - pointer to the RetCoordSystem in the retriever definition.
 *    - NULL if not found
 */
RetCoordSystem *_dsproc_get_ret_coordsys(const char *name)
{
    Retriever *ret = _DSProc->retriever;
    int        csi;

    if (name) {
        for (csi = 0; csi < ret->ncoord_systems; csi++) {
            if (strcmp(ret->coord_systems[csi]->name, name) == 0) {
                return(ret->coord_systems[csi]);
            }
        }
    }

    return((RetCoordSystem *)NULL);
}

/**
 *  Private: Get the base time used for retrieved data.
 *
 *  @return base_time used for retrieved data
 */
time_t _dsproc_get_ret_data_base_time(void)
{
    return(_RetData_BaseTime);
}

/**
 *  Private: Get the end time used for retrieved data.
 *
 *  @return end_time used for retrieved data
 */
timeval_t _dsproc_get_ret_data_end_time(void)
{
    return(_RetData_EndTime);
}

/**
 *  Private: Get the time long_name used for retrieved data.
 *
 *  @return time long_name used for retrieved data
 */
const char *_dsproc_get_ret_data_time_desc(void)
{
    return(_RetData_TimeDesc);
}

/**
 *  Private: Get the time units used for retrieved data.
 *
 *  @return time units used for retrieved data
 */
const char *_dsproc_get_ret_data_time_units(void)
{
    return(_RetData_TimeUnits);
}

/**
 *  Private: Get the input datastream ID for a retrieved datastream group.
 *
 *  @param ret_ds_group - pointer to the retrieved datastream group
 *
 *  @return
 *    - input datastream ID
 *    - -1 if this is not a retrieved datastream group
 */
int _dsproc_get_ret_group_ds_id(CDSGroup *ret_ds_group)
{
    CDSAtt *dsid_att;
    size_t  length;
    int     in_dsid;

    dsid_att = cds_get_att(ret_ds_group, "datastream_id");
    if (dsid_att) {
        length = 1;
        if (cds_get_att_value(dsid_att, CDS_INT, &length, &in_dsid)) {
            return(in_dsid);
        }
    }

    return(-1);
}

/**
 *  Private: Initialize Retriever.
 *
 *  This function will load the retriever definition from the database
 *  and initialize the input datastreams.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @return
 *    -  1 if successful
 *    -  0 if an error occurred
 */
int _dsproc_init_retriever()
{
    Retriever     *ret;
    RetDsGroup    *ret_group;
    RetDsSubGroup *ret_subgroup;
    int            status;
    int            rgi, rdsi;

    /* Cleanup any previously loaded retriever data */

    _dsproc_free_retriever();

    /* Load the retriever information from the database */

    DEBUG_LV1( DSPROC_LIB_NAME,
        "Loading retriever definition from database\n");

    status = dsdb_get_retriever(
        _DSProc->dsdb, _DSProc->type, _DSProc->name, &ret);

    if (status < 0) {
        dsproc_set_status(DSPROC_EDBERROR);
        return(0);
    }
    else if (status == 0) {

        DEBUG_LV1( DSPROC_LIB_NAME,
            " - retriever definition was not found in the database\n");

        return(1);
    }

    /* Set the retriever location */

    if (!dsdb_set_retriever_location(ret, _DSProc->site, _DSProc->facility)) {
        dsproc_set_status(DSPROC_EBADRETRIEVER);
        return(0);
    }

    /**********************************************************************
    * BDE: We need a dsdb_validate_retriever function here...
    * Primarily to verify that data types and units match for dimensions
    * and variables with the same name.
    **********************************************************************/

    _DSProc->retriever = ret;

    /* Print the retriever definition if we are in debug mode */

    if (msngr_debug_level) {

        fprintf(stdout,
        "\n"
        "================================================================================\n"
        "Retriever Definition:\n"
        "================================================================================\n"
        "\n");

        dsdb_print_retriever(stdout, ret);

        fprintf(stdout,
        "\n"
        "================================================================================\n"
        "\n");
    }

    /* Initialize the input datastreams */

    for (rgi = 0; rgi < ret->ngroups; rgi++) {

        ret_group    = ret->groups[rgi];
        ret_subgroup = ret->subgroups[0];

        if (ret_group->nsubgroups <= 0) {

            WARNING( DSPROC_LIB_NAME,
                "No subgroups found in retriever definition for group: %s\n",
                ret_group->name);

            continue;
        }

        if (ret_group->nsubgroups > 1) {

            WARNING( DSPROC_LIB_NAME,
                "Found multiple subgroups in retriever definition for group: %s\n"
                " -> multiple subgroups are not currently supported\n"
                " -> only the first subgroup will be processed\n",
                ret_group->name);
        }

        ret_subgroup = ret_group->subgroups[0];

        for (rdsi = 0; rdsi < ret_subgroup->ndatastreams; rdsi++) {

            if (!_dsproc_init_ret_datastream(
                ret_group, ret_subgroup->datastreams[rdsi])) {

                return(0);
            }
        }
    }

    return(1);
}

/** @publicsection */

/*******************************************************************************
 *  Internal Functions Visible To The Public
 */

/**
 *  Get input data using retriever information.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  begin_time - begin time of the data processing interval
 *  @param  end_time   - end time of the data processing interval
 *  @param  ret_data   - output: pointer to the ret_data CDSGroup
 *
 *  @return
 *    -  1 if successful or a retriever definition is not defined
 *    -  0 if a required variable could not be found but data for the input
 *         datastream exists past the end of the current processing interval.
 *         If this is the case we can assume there is a gap in the data and
 *         the process should continue to the next processing interval.
 *    - -1 if an error occurred, or a required variable could not be found and
 *         data for the input datastream does not exist past the end of the
 *         current processing interval. If this is the case the process should
 *         wait for more input data to be created.
 */
int dsproc_retrieve_data(
    time_t     begin_time,
    time_t     end_time,
    CDSGroup **ret_data)
{
    Retriever     *ret;
    RetDsGroup    *ret_group;
    RetDsSubGroup *ret_subgroup;
    RetDataStream *ret_ds;
    RetDsCache    *cache;
    RetDsFile     *file;
    DataStream    *in_ds;
    int           *retrieved_group;
    int            retrieve_group;
    int            in_dsid;
    int            int_begin_time;
    int            int_end_time;
    int            status;
    int            rgi, rdsi, fi;
    int            scan_mode;

    char           ts1[32], ts2[32];

    *ret_data = (CDSGroup *)NULL;

    ret = _DSProc->retriever;
    if (!ret) {
        return(1);
    }

    if (msngr_debug_level || msngr_provenance_level) {

        DEBUG_LV1( DSPROC_LIB_NAME,
            "Retrieving input data for processing interval:\n"
            " - begin time: %s\n"
            " - end time:   %s\n",
            format_secs1970(begin_time, ts1),
            format_secs1970(end_time,   ts2));
    }

    /* Clean up any previous input data loaded by the retriever */

    _dsproc_cleanup_retrieved_data();

    /* Define the parent CDSGroup used to store the retrieved data */

    _DSProc->ret_data = cds_define_group(NULL, "retrieved_data");
    if (!_DSProc->ret_data) {
        dsproc_set_status(DSPROC_ENOMEM);
        return(-1);
    }

    *ret_data = _DSProc->ret_data;

    /* Set the base_time, and time units and long_name
     * to use for retrieved data. */

    _RetData_BaseTime        = begin_time;
    _RetData_EndTime.tv_sec  = end_time;
    _RetData_EndTime.tv_usec = 0;

    if (!cds_base_time_to_units_string(_RetData_BaseTime, _RetData_TimeUnits)) {

        ERROR( DSPROC_LIB_NAME,
            "Could not create base_time string for retrieved data\n");

        dsproc_set_status(DSPROC_ERETRIEVER);
        return(-1);
    }

    strcpy(_RetData_TimeDesc, "Time offset from midnight");

    /* Initialize all input datastreams for this processing interval */

    for (in_dsid = 0; in_dsid < _DSProc->ndatastreams; in_dsid++) {

        in_ds = _DSProc->datastreams[in_dsid];

        if (!in_ds->ret_cache) {
            continue;
        }

        cache = in_ds->ret_cache;

        DEBUG_LV1( DSPROC_LIB_NAME,
            "%s: Initializing datastream for current processing interval\n",
            in_ds->name);

        /* Adjust times for the begin and end offsets */

        cache->begin_time = begin_time - cache->begin_offset;
        cache->end_time   = end_time   + cache->end_offset;

        /* Check begin date and end date dependencies */

        if (cache->dep_begin_date) {

            if (cache->dep_begin_date > cache->end_time) {
                cache->begin_time = 0;
                cache->end_time   = 0;
                continue;
            }

            if (cache->begin_time < cache->dep_begin_date)
                cache->begin_time = cache->dep_begin_date;
        }

        if (cache->dep_end_date) {

            if (cache->dep_end_date < cache->begin_time) {
                cache->begin_time = 0;
                cache->end_time   = 0;
                continue;
            }

            if (cache->end_time > cache->dep_end_date)
                cache->end_time = cache->dep_end_date;
        }

        /* Define the CDSGroup for this input datastream */

        cache->ds_group = cds_define_group(*ret_data, in_ds->name);
        if (!cache->ds_group) {
            dsproc_set_status(DSPROC_ENOMEM);
            return(-1);
        }

        /* Add global datastream attributes */

        if (!cds_define_att_text(cache->ds_group, "base_name", in_ds->dsc_name)  ||
            !cds_define_att_text(cache->ds_group, "site",      in_ds->site)      ||
            !cds_define_att_text(cache->ds_group, "facility",  in_ds->facility)  ||
            !cds_define_att_text(cache->ds_group, "level",     in_ds->dsc_level) ||
            !cds_define_att(cache->ds_group,
                "datastream_id", CDS_INT, 1, &in_dsid)) {

            dsproc_set_status(DSPROC_ENOMEM);
            return(-1);
        }

        /* Define the begin and end time global attributes */

        int_begin_time = (int)cache->begin_time;
        int_end_time   = (int)cache->end_time;

        if (!cds_define_att(cache->ds_group, "begin_time", CDS_INT, 1, &int_begin_time) ||
            !cds_define_att(cache->ds_group, "end_time",   CDS_INT, 1, &int_end_time)) {

            dsproc_set_status(DSPROC_ENOMEM);
            return(-1);
        }

        /* Load the tranformation parameters file for this datastream */

        if (_DSProc->model & DSP_TRANSFORM) {

            status = dsproc_load_transform_params(
                cache->ds_group,
                in_ds->site,
                in_ds->facility,
                in_ds->dsc_name,
                in_ds->dsc_level);

            if (status < 0) {
                return(-1);
            }
        }
    }

    /* Load data for all retriever groups whose first datastream has
     * the DS_PRESERVE_OBS flag set. This allows the end time to be
     * adjusted properly for all other datastreams. */

    retrieved_group = (int *)calloc(ret->ngroups, sizeof(int));
    if (!retrieved_group) {

        ERROR( DSPROC_LIB_NAME,
            "Could not retrieve input data.\n"
            " -> memory allocation error\n");

        dsproc_set_status(DSPROC_ENOMEM);
        return(-1);
    }

    for (rgi = 0; rgi < ret->ngroups; rgi++) {

        ret_group      = ret->groups[rgi];
        ret_subgroup   = ret_group->subgroups[0];
        retrieve_group = 0;

        for (rdsi = 0; rdsi < ret_subgroup->ndatastreams; rdsi++) {

            ret_ds = ret_subgroup->datastreams[rdsi];

            in_dsid = dsproc_get_datastream_id(
                ret_ds->site,
                ret_ds->facility,
                ret_ds->name,
                ret_ds->level,
                DSR_INPUT);

            if (in_dsid < 0) {
                continue;
            }

            in_ds = _DSProc->datastreams[in_dsid];

            if (in_ds->flags & DS_PRESERVE_OBS) {
                retrieve_group = 1;
            }

            break;
        }

        if (retrieve_group) {

            DEBUG_LV1( DSPROC_LIB_NAME,
                "---------------------------------------\n"
                "Retrieving data for group: %s\n"
                "---------------------------------------\n",
                ret_group->name);

            status = _dsproc_retrieve_group(ret_group);

            if (status <= 0) {
                free(retrieved_group);
                return(status);
            }

            retrieved_group[rgi] = 1;
        }
    }

    /* Load data for all retriever groups that haven't been loaded yet */

    for (rgi = 0; rgi < ret->ngroups; rgi++) {

        if (retrieved_group[rgi]) continue;

        ret_group = ret->groups[rgi];

        DEBUG_LV1( DSPROC_LIB_NAME,
            "---------------------------------------\n"
            "Retrieving data for group: %s\n"
            "---------------------------------------\n",
            ret_group->name);

        status = _dsproc_retrieve_group(ret_group);

        if (status <= 0) {
            free(retrieved_group);
            return(status);
        }
    }

    free(retrieved_group);

    /* Loop over all input datastreams created by the retriever and delete
     * the ones for which no input data (observations) were found. */

    DEBUG_LV1( DSPROC_LIB_NAME,
        "---------------------------------------\n"
        "Retrieval Summary:\n"
        "---------------------------------------\n");

    scan_mode = 0;

    for (in_dsid = 0; in_dsid < _DSProc->ndatastreams; in_dsid++) {

        in_ds = _DSProc->datastreams[in_dsid];

        if (!in_ds->ret_cache ||
            !in_ds->ret_cache->ds_group) {

            continue;
        }

        if (in_ds->flags & DS_SCAN_MODE) {
            scan_mode = 1;
        }

        cache = in_ds->ret_cache;

        /* Delete observations that do not have any variables defined */

        for (fi = 0; fi < cache->nfiles; fi++) {

            file = cache->files[fi];

            if (file->var_count == 0) {

                DEBUG_LV1( DSPROC_LIB_NAME,
                    "%s: removing empty observation created by retriever\n",
                    file->obs_group->name);

                cds_set_definition_lock(file->obs_group, 0);
                cds_delete_group(file->obs_group);

                file->obs_group = (CDSGroup *)NULL;
            }
            else {
                in_ds->total_records += file->sample_count;

                if (in_ds->begin_time.tv_sec == 0) {
                    in_ds->begin_time = file->start_time;
                }

                in_ds->end_time = file->end_time;

                DEBUG_LV1( DSPROC_LIB_NAME,
                    "%s: '%s' -> '%s', %d records\n",
                    file->obs_group->name,
                    format_secs1970(file->start_time.tv_sec, ts1),
                    format_secs1970(file->end_time.tv_sec,   ts2),
                    file->sample_count);
            }
        }

        /* Delete the datastream group if no observations are defined */

        if (cache->ds_group->ngroups == 0) {

            DEBUG_LV1( DSPROC_LIB_NAME,
                "%s: removing empty datastream created by retriever\n",
                cache->ds_group->name);

            /* Set lock to zero to allow it to be deleted */

            cds_set_definition_lock(cache->ds_group, 0);
            cds_delete_group(cache->ds_group);

            cache->ds_group = (CDSGroup *)NULL;
        }
    }

    /* Check if we found any data to process */

    if ((*ret_data)->ngroups == 0) {

        // All inputs are optional and none were found

        if (!scan_mode) {

            WARNING( DSPROC_LIB_NAME,
                "Could not find any data to retrieve for processing interval:\n"
                " - begin time: %s\n"
                " - end time:   %s\n",
                format_secs1970(begin_time, ts1),
                format_secs1970(end_time,   ts2));
        }
    }

    return(1);
}

/*******************************************************************************
 *  Public Functions
 */

/**
 *  Set the time offsets to use when retrieving data.
 *
 *  This function can be used to override the begin and end time offsets
 *  specified in the retriever definition and should be called from the
 *  pre-retrieval hook function.
 *
 *  @param  ds_id        - input datastream ID
 *  @param  begin_offset - time offset from beginning of processing interval
 *  @param  end_offset   - time offset from end of processing interval
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 */
void dsproc_set_retriever_time_offsets(
    int    ds_id,
    time_t begin_offset,
    time_t end_offset)
{
    DataStream *ds    = _DSProc->datastreams[ds_id];
    RetDsCache *cache = ds->ret_cache;

    if (cache) {

        DEBUG_LV1( DSPROC_LIB_NAME,
            "%s: Setting retrieval time offsets\n"
            " - begin offset: %d seconds\n"
            " - end offset:   %d seconds\n",
            ds->name, (int)begin_offset, (int)end_offset);

        cache->begin_offset = begin_offset;
        cache->end_offset   = end_offset;
    }
    else {

        WARNING( DSPROC_LIB_NAME,
            "Could not set retriever time offsets for: %s\n"
            " -> not a valid input datastream\n",
            ds->name);
    }
}
