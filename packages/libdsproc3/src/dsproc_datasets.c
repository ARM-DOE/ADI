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

/** \file dsproc_datasets.c
 *  Dataset Functions.
 */

#include "dsproc3.h"
#include "dsproc_private.h"

extern DSProc *_DSProc; /**< Internal DSProc structure */

/** \privatesection */

/*******************************************************************************
 *  Static Functions Visible Only To This Module
 */

/*******************************************************************************
 *  Private Functions Visible Only To This Library
 */

/** \publicsection */

/*******************************************************************************
 *  Internal Functions Visible To The Public
 */

/**
 *  Create all output datasets.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  \retval  1  if successful
 *  \retval  0  if an error occurred
 */
int dsproc_create_output_datasets()
{
    DataStream *ds;
    int         ds_id;

    for (ds_id = 0; ds_id < _DSProc->ndatastreams; ds_id++) {

        ds = _DSProc->datastreams[ds_id];

        if (ds->role == DSR_OUTPUT) {

            if (ds->out_cds) {
                _dsproc_free_datastream_out_cds(ds);
            }

            if (!dsproc_create_output_dataset(
                ds_id, _DSProc->interval_begin, 1)) {

                return(0);
            }
        }
    }

    if (_DSProc->trans_data) {
        if (!dsproc_map_datasets(_DSProc->trans_data, NULL, 0)) {
            return(0);
        }
    }
    else if (_DSProc->ret_data) {
        if (!dsproc_map_datasets(_DSProc->ret_data, NULL, 0)) {
            return(0);
        }
    }

    return(1);
}

/**
 *  Create an output dataset.
 *
 *  The memory used by the returned dataset is managed internally
 *  and should not be freed by the calling process.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  \param   ds_id         datastream ID
 *
 *  \param   data_time     time of the data being processed, this is used to
 *                         determine the DOD version and set the time varying
 *                         attribute values
 *
 *  \param   set_location  specifies if the location should be set using
 *                         the process location defined in the database,
 *                         (1 = true, 0 = false)
 *
 *  \retval  dataset  pointer to the new dataset
 *  \retval  NULL     if an error occurred
 */
CDSGroup *dsproc_create_output_dataset(
    int      ds_id,
    time_t   data_time,
    int      set_location)
{
    DataStream *ds = _DSProc->datastreams[ds_id];
    int         copy_flags;
    int         status;

    if (dsproc_get_dynamic_dods_mode()) {
        copy_flags = 0;
    }
    else {
        copy_flags = CDS_COPY_LOCKS;
    }

    /* Free the current dataset if it has already been created */

    if (ds->out_cds) {
        _dsproc_free_datastream_out_cds(ds);
    }

    DEBUG_LV1( DSPROC_LIB_NAME,
        "%s: Creating dataset\n", ds->name);

    /* Load the datastream DOD for this data time */

    status = _dsproc_get_dsdod(ds, data_time);

    if (status < 0) {
        return((CDSGroup *)NULL);
    }

    if (status == 0) {

        if (dsproc_get_dynamic_dods_mode()) {

            ds->out_cds = cds_define_group(NULL, ds->name);
            if (!ds->out_cds) {
                dsproc_set_status(DSPROC_ENOMEM);
                return((CDSGroup *)NULL);
            }
        }
        else {

            ERROR( DSPROC_LIB_NAME,
                "Could not create dataset for: %s\n"
                " -> DOD not defined in database\n",
                ds->name);

            dsproc_set_status(DSPROC_ENODOD);
            return((CDSGroup *)NULL);
        }
    }
    else {

        /* Create the dataset by cloning the dsdod dataset */

        status = cds_copy_group(
            ds->dsdod->cds_group, NULL, ds->name,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0,
            copy_flags, &(ds->out_cds));

        if (status <= 0) {

            if (status == 0) {

                ERROR( DSPROC_LIB_NAME,
                    "Could not create dataset for: %s\n"
                    " -> DOD could not be cloned\n",
                    ds->name);
            }

            dsproc_set_status(DSPROC_ECDSCOPY);
            return((CDSGroup *)NULL);
        }
    }

    /* Add the runtime metadata */

    if (!dsproc_set_runtime_metadata(ds_id, ds->out_cds)) {
        return((CDSGroup *)NULL);
    }

    /* Set the location */

    if (set_location) {
        if (!dsproc_set_dataset_location(ds->out_cds)) {
            return((CDSGroup *)NULL);
        }
    }

    return(ds->out_cds);
}

/**
 *  Set the location variables for a dataset.
 *
 *  This function will set the lat, lon, and alt variable data using the
 *  process location defined in the database.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  \param   dataset  pointer to the dataset
 *
 *  \retval  1  if successful
 *  \retval  0  if an error occurred
 */
int dsproc_set_dataset_location(CDSGroup *dataset)
{
    CDSVar  *var;
    ProcLoc *proc_loc;
    int      status;

    DEBUG_LV1( DSPROC_LIB_NAME,
        "%s: Setting location data\n",
        dataset->name);

    status = dsproc_get_location(&proc_loc);
    if (status <= 0) {

        if (status == 0) {

            ERROR( DSPROC_LIB_NAME,
                "Could not get process location from database\n"
                " -> unexpected NULL result from database query");

            dsproc_set_status(DSPROC_EDBERROR);
        }

        return(0);
    }

    var = cds_get_var(dataset, "lat");
    if (var && (var->ndims == 0)) {
        if (!cds_set_var_data(
            var, CDS_FLOAT, 0, 1, NULL, (void *)&proc_loc->lat)) {

            dsproc_set_status(DSPROC_ECDSSETDATA);
        }
    }

    var = cds_get_var(dataset, "lon");
    if (var && (var->ndims == 0)) {
        if (!cds_set_var_data(
            var, CDS_FLOAT, 0, 1, NULL, (void *)&proc_loc->lon)) {

            dsproc_set_status(DSPROC_ECDSSETDATA);
        }
    }

    var = cds_get_var(dataset, "alt");
    if (var && (var->ndims == 0)) {
        if (!cds_set_var_data(
            var, CDS_FLOAT, 0, 1, NULL, (void *)&proc_loc->alt)) {

            dsproc_set_status(DSPROC_ECDSSETDATA);
        }
    }

    return(1);
}

/**
 *  Pass data from one dataset to another.
 *
 *  This function will only copy data from the input dataset to the output
 *  dataset for objects already defined in the output dataset that do not
 *  already have data or values defined.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  \param   in_dataset   pointer to the input dataset
 *  \param   out_dataset  pointer to the output dataset
 *  \param   flags        reserved for control flags
 *
 *  \retval  1  if successful
 *  \retval  0  if an error occurred
 */
int dsproc_dataset_pass_through(
    CDSGroup *in_dataset,
    CDSGroup *out_dataset,
    int       flags)
{
    CDSVar    *time_var;
    timeval_t *sample_times;
    size_t     ntimes;

    /* Check if we need to copy the time data */

    time_var = cds_find_time_var(out_dataset);

    if (time_var && time_var->sample_count == 0) {

        /* Get times from input dataset */

        sample_times = cds_get_sample_timevals(in_dataset, 0, &ntimes, NULL);

        if (sample_times) {

            /* Set times in output dataset */

            if (!cds_set_sample_timevals(out_dataset, 0, ntimes, sample_times)) {
                dsproc_set_status(DSPROC_ECDSSETTIME);
                return(0);
            }

            free(sample_times);
        }
        else if (ntimes != 0) {
            dsproc_set_status(DSPROC_ECDSGETTIME);
            return(0);
        }
    }

    /* Copy dimension, global attributes, and variables */

    if (!cds_copy_dims(in_dataset, out_dataset, NULL, NULL, CDS_EXCLUSIVE) ||
        !cds_copy_atts(in_dataset, out_dataset, NULL, NULL, CDS_EXCLUSIVE) ||
        !cds_copy_vars(in_dataset, out_dataset, NULL, NULL, NULL, NULL, 0, 0, 0, CDS_EXCLUSIVE)) {

        dsproc_set_status(DSPROC_ECDSCOPY);
        return(0);
    }

    return(1);
}

/*******************************************************************************
 *  Public Functions
 */

/**
 *  Returns the dataset name.
 *
 *  The returned name belongs to the dataset structure and must not be freed
 *  or altered by the calling process.
 *
 *  @param   dataset  pointer to the dataset
 *
 *  @retval  name  pointer to the dataset name
 *  @retval  NULL  if the specified dataset is NULL
 */
const char *dsproc_dataset_name(CDSGroup *dataset)
{
    if (!dataset) {
        return((const char *)NULL);
    }

    return(dataset->name);
}

/**
 *  Get file creation information from the history attribute.
 *
 *  This function will parse the first line of the history attribute
 *  and return the time the file was created, the host that the file
 *  was created on, the user that ran the process that created
 *  the file, and the name of the process that created the file.
 *
 *  Memory will be allocated for the returned history, host, user,
 *  and process strings, and must be freed by the calling process.
 *
 *  Output arguments can be NULL if they are not needed.
 *
 *  \param   dataset  pointer to the dataset
 *  \param   history  output: copy of the history attribute value
 *  \param   time     output: creation time of the file the dataset came from
 *  \param   host     output: host the file was created on
 *  \param   user     output: name of the user that created the file
 *  \param   process  output: name of the process that created the file
 *
 *  \retval   1  if successful
 *  \retval   0  history attribute does not exist or has invalid format
 */
int dsproc_get_dataset_creation_info(
    CDSGroup  *dataset,
    char     **history,
    time_t    *time,
    char     **host,
    char     **user,
    char     **process)
{
    CDSAtt *att; 
    int     count;
    int     YYYY, MM, DD, hh, mm, ss;
    char    _host[256];
    char    _user[256];
    char    _proc[256];
    char   *chrp;

    att = cds_get_att(dataset, "history");
    if (!att || !att->value.cp || att->length <= 0) {
        return(0);
    }

    if (history) {
        *history = strdup(att->value.cp);
        // trim trailing newline character
        chrp = *history + strlen(*history) - 1;
        while (chrp >= *history && isspace(*chrp)) --chrp;
        chrp++;
        if (*chrp == '\n') *chrp = '\0';
    }

    _host[0] = _user[0] = _proc[0] = '\0';
    YYYY = MM = DD = hh = mm = ss = 0;

    count = sscanf(att->value.cp,
        "created by user %s on machine %s at %d-%d-%d %d:%d:%d, using %s",
        _user, _host, &YYYY, &MM, &DD, &hh, &mm, &ss, _proc);

    if (count < 5) return(0);

    if (time)    *time    = get_secs1970(YYYY, MM, DD, hh, mm, ss);
    if (host)    *host    = strdup(_host);
    if (user)    *user    = strdup(_user);
    if (process) *process = strdup(_proc);

    return(1);
}

/**
 *  Get the values of the lat, lon, and alt variables in a dataset.
 *
 *  All output arguments can be NULL if the values are not needed.
 * 
 *  An error message will be generated if an output argument is not NULL
 *  and the associated variable does not exist in the dataset.
 *
 *  \param   dataset  pointer to the dataset
 *  \param   lat      output: latitude
 *  \param   lon      output: longitude
 *  \param   alt      output: altitude
 *
 *  \retval  1  if successful
 *  \retval  0  if an error occurred
 */
int dsproc_get_dataset_location(
    CDSGroup *dataset,
    double   *lat,
    double   *lon,
    double   *alt)
{
    CDSVar *var;
    size_t  length;

    /* Get latitude */

    if (lat) {

        if (!(var = dsproc_get_var(dataset, "lat"))) {

            ERROR( DSPROC_LIB_NAME,
                "Could not get latitude from dataset: %s\n"
                " -> 'lat' variable does not exist\n",
                dataset->name);

            dsproc_set_status(DSPROC_EREQVAR);
            return(0);
        }

        length = 1;
        if (!dsproc_get_var_data(var, CDS_DOUBLE, 0, &length, NULL, lat)) {
            return(0);
        }
    }

    /* Get longitude */

    if (lon) {

        if (!(var = dsproc_get_var(dataset, "lon"))) {

            ERROR( DSPROC_LIB_NAME,
                "Could not get longitude from dataset: %s\n"
                " -> 'lon' variable does not exist\n",
                dataset->name);

            dsproc_set_status(DSPROC_EREQVAR);
            return(0);
        }

        length = 1;
        if (!dsproc_get_var_data(var, CDS_DOUBLE, 0, &length, NULL, lon)) {
            return(0);
        }
    }

    /* Get altitude */

    if (alt) {

        if (!(var = dsproc_get_var(dataset, "alt"))) {

            ERROR( DSPROC_LIB_NAME,
                "Could not get altitude from dataset: %s\n"
                " -> 'alt' variable does not exist\n",
                dataset->name);

            dsproc_set_status(DSPROC_EREQVAR);
            return(0);
        }

        length = 1;
        if (!dsproc_get_var_data(var, CDS_DOUBLE, 0, &length, NULL, alt)) {
            return(0);
        }
    }

    return(1);
}

/**
 *  Get the DOD version of a dataset.
 *
 *  The memory used by the return string belongs to the global dod_version
 *  attribute in the dataset and must not be altered or freed.
 *
 *  @param  dataset  pointer to the dataset
 *  @param  major    output: major version number
 *  @param  minor    output: minor version number
 *  @param  micro    output: reserved for micro version number (this not currently
 *                   implemented but I can see a need for it coming soon...)
 *
 *  @return
 *    - DOD version of the dataset (as a character string)
 *    - NULL if the dod_version attribute was not found
 */
const char *dsproc_get_dataset_version(
    CDSGroup *dataset,
    int      *major,
    int      *minor,
    int      *micro)
{
    CDSAtt *att;

    if (major) *major = 0;
    if (minor) *minor = 0;
    if (micro) *micro = 0;

    att = cds_get_att(dataset, "dod_version");
    if (!att || att->type != CDS_CHAR) {
        return((const char *)NULL);
    }

    parse_version_string(att->value.cp, major, minor, micro);

    return(att->value.cp);
}

/**
 *  Get an output dataset.
 *
 *  This function will return a pointer to the output dataset for the
 *  specifed datastream and observation. The obs_index should always be zero
 *  unless observation based processing is being used. This is because all
 *  input observations should have been merged into a single observation
 *  in the output datasets.
 *
 *  \param   ds_id      output datastream ID
 *  \param   obs_index  the index of the obervation to get the dataset for
 *
 *  \retval  dataset    pointer to the output dataset
 *  \retval  NULL       if it does not exist
 *
 *  \b Example: Get the dataset for an output datastream
 *  \code
 *
 *  const char *dsc_name  = "example";
 *  const char *dsc_level = "c1";
 *  int         ds_id;
 *  CDSGroup   *dataset;
 *
 *  ds_id   = dsproc_get_output_datastream_id(dsc_name, dsc_level);
 *  dataset = dsproc_get_output_dataset(ds_id, 0);
 *
 *  \endcode
 */
CDSGroup *dsproc_get_output_dataset(
    int ds_id,
    int obs_index)
{
    DataStream *ds;
    CDSGroup   *dataset;

    if (obs_index < 0 ||
        ds_id     < 0 ||
        ds_id     >= _DSProc->ndatastreams) {

        return((CDSGroup *)NULL);
    }

    ds = _DSProc->datastreams[ds_id];

    if (ds->role != DSR_OUTPUT ||
        !ds->out_cds) {

        return((CDSGroup *)NULL);
    }

/* BDE OBS UPDATE:
 * For now just return ds->out_cds if obs_index is zero. This will need to be
 * updated when multiple observations in the output datasets are supported.

    if (obs_index < ds->out_cds->ngroups) {
        dataset = ds->out_cds->groups[obs_index];
    }
    else {
        dataset = (CDSGroup *)NULL;
    }
*/

    if (obs_index == 0) {
        dataset = ds->out_cds;
    }
    else {
        dataset = (CDSGroup *)NULL;
    }

/* END BDE OBS UPDATE */

    return(dataset);
}

/**
 *  Get a retrieved dataset.
 *
 *  This function will return a pointer to the retrieved dataset for the
 *  specifed datastream and observation.
 *
 *  The obs_index is used to specify which observation to get the dataset for.
 *  This value will typically be zero unless this function is called from a
 *  post_retrieval_hook() function, or the process is using observation based
 *  processing.  In either of these cases the retrieved data will contain one
 *  "observation" for every file the data was read from on disk.
 *
 *  It is also possible to have multiple observations in the retrieved data
 *  when a pre_transform_hook() is called if a dimensionality conflict
 *  prevented all observations from being merged.
 *
 *  \param   ds_id      input datastream ID
 *  \param   obs_index  observation index (0 based indexing)
 *
 *  \retval  dataset    pointer to the retrieved dataset
 *  \retval  NULL       if it does not exist
 *
 *  \b Example: Loop over all retrieved datasets for an input datastream
 *  \code
 *
 *  const char *dsc_name  = "example";
 *  const char *dsc_level = "b1";
 *  int         ds_id;
 *  int         obs_index;
 *  CDSGroup   *dataset;
 *
 *  ds_id = dsproc_get_input_datastream_id(dsc_name, dsc_level);
 *
 *  for (obs_index = 0; ; obs_index++) {
 *      dataset = dsproc_get_retrieved_dataset(ds_id, obs_index);
 *      if (!dataset) break;
 *  }
 *
 *  \endcode
 */
CDSGroup *dsproc_get_retrieved_dataset(
    int ds_id,
    int obs_index)
{
    DataStream *ds;
    CDSGroup   *ds_group;
    CDSGroup   *dataset;

    if (obs_index < 0 ||
        ds_id     < 0 ||
        ds_id     >= _DSProc->ndatastreams) {

        return((CDSGroup *)NULL);
    }

    ds = _DSProc->datastreams[ds_id];

    if (ds->role != DSR_INPUT ||
        !ds->ret_cache        ||
        !ds->ret_cache->ds_group) {

        return((CDSGroup *)NULL);
    }

    ds_group = ds->ret_cache->ds_group;

    if (obs_index < ds_group->ngroups) {
        dataset = ds_group->groups[obs_index];
    }
    else {
        dataset = (CDSGroup *)NULL;
    }

    return(dataset);
}

/**
 *  Get a transformed dataset.
 *
 *  This function will return a pointer to the transformed dataset for the
 *  specifed coordinate system, datastream, and observation. The obs_index
 *  should always be zero unless observation based processing is being used.
 *  This is because all input observations should have been merged into a
 *  single observation in the transformed datasets.
 *
 *  \param  coordsys_name  the name of the coordinate system as specified in
 *                         the retriever definition or NULL if a coordinate
 *                         system name was not specified.
 *
 *  \param  ds_id          input datastream ID
 *
 *  \param  obs_index      the index of the obervation to get the dataset for
 *
 *  \return
 *    - pointer to the output dataset
 *    - NULL if it does not exist
 */
CDSGroup *dsproc_get_transformed_dataset(
    const char *coordsys_name,
    int         ds_id,
    int         obs_index)
{
    CDSGroup   *cs_group;
    CDSGroup   *ds_group;
    DataStream *ds;
    CDSGroup   *dataset;
    char        auto_coordsys_name[256];

    if (obs_index < 0 ||
        ds_id     < 0 ||
        ds_id     >= _DSProc->ndatastreams) {

        return((CDSGroup *)NULL);
    }

    ds = _DSProc->datastreams[ds_id];

    if (ds->role != DSR_INPUT) {
        return((CDSGroup *)NULL);
    }

    if (!coordsys_name) {

        snprintf(auto_coordsys_name, 256,
            "auto_%s_%s",
            ds->dsc_name,
            ds->dsc_level);

        coordsys_name = auto_coordsys_name;
    }

    cs_group = cds_get_group(_DSProc->trans_data, coordsys_name);
    if (!cs_group) {
        return((CDSGroup *)NULL);
    }

    ds_group = cds_get_group(cs_group, ds->name);

/* BDE OBS UPDATE:
 * For now just return ds_group if obs_index is zero. This will need to be
 * updated when multiple observations in the transformed datasets are supported.

    if (obs_index < ds_group->ngroups) {
        dataset = ds_group->groups[obs_index];
    }
    else {
        dataset = (CDSGroup *)NULL;
    }
*/

    if (obs_index == 0) {
        dataset = ds_group;
    }
    else {
        dataset = (CDSGroup *)NULL;
    }

/* END BDE OBS UPDATE */

    return(dataset);
}
