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

/** @file dsproc_standard_qc.c
 *  Standard QC Checks.
 */

#include "dsproc3.h"
#include "dsproc_private.h"
#include <math.h>

extern DSProc *_DSProc; /**< Internal DSProc structure */

/** @privatesection */

/*******************************************************************************
 *  Static Data and Functions Visible Only To This Module
 */

/** Array of variable names that should be excluded from the QC checks. */
static char **ExQcVars;

/** Number of variables that should be excluded from the QC checks. */
static int    NumExQcVars;

/**
 *  Static: Check if a variable has been exculded from the QC checks.
 *
 *  @param  var_name - the variable name
 *
 *  @return
 *    - 1 if the variable has been excluded from the QC checks
 *    - 0 if the variable has not been excluded
 */
static int _dsproc_is_excluded_from_standard_qc_checks(const char *var_name)
{
    int vi;

    for (vi = 0; vi < NumExQcVars; ++vi) {
        if (strcmp(var_name, ExQcVars[vi]) == 0) {
            return(1);
        }
    }

    return(0);
}

/**
 *  Static: Get previous DSFile and time index.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  ds         - pointer to the DataStream structure
 *  @param  cds_object - pointer to the CDS Object to get the time from
 *  @param  dsfile     - output: pointer to the DSFile
 *  @param  index      - output: index of the time in the DSFile that is just
 *                       prior to the first time in the specified CDS Object.
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
static int _dsproc_get_prev_dsfile_time_index(
    DataStream *ds,
    void       *cds_object,
    DSFile    **dsfile,
    int        *index)
{
    size_t     count         = 1;
    timeval_t  start_timeval = { 0, 0 };
    int        ndsfiles      = 0;
    DSFile   **dsfiles       = (DSFile **)NULL;

    *dsfile = (DSFile *)NULL;
    *index  = -1;

    if (!dsproc_get_sample_timevals(cds_object, 0, &count, &start_timeval)) {
        return((count == 0) ? 1 : 0);
    }

    ndsfiles = _dsproc_find_dsfiles(ds->dir, NULL, &start_timeval, &dsfiles);

    if (ndsfiles <  0) return(0);
    if (ndsfiles == 0) return(1);

    *dsfile = dsfiles[0];
    free(dsfiles);

    *index = cds_find_timeval_index(
        (*dsfile)->ntimes, (*dsfile)->timevals, start_timeval, CDS_LT);

    return(1);
}

/*******************************************************************************
 *  Private Functions Visible Only To This Library
 */

/**
 *  Private: Free all memory used by the interval ExQcVars list.
 */
void _dsproc_free_excluded_qc_vars(void)
{
    int vi;

    if (ExQcVars) {

        for (vi = 0; vi < NumExQcVars; ++vi) {
            if (ExQcVars[vi]) free(ExQcVars[vi]);
        }

        free(ExQcVars);
    }

    ExQcVars    = (char **)NULL;
    NumExQcVars = 0;
}

/** @publicsection */

/*******************************************************************************
 *  Internal Functions Visible To The Public
 */

/**
 *  Exclude a variable from the standard QC checks.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  var_name - the name of the variable to exclude
 *
 *  @return
 *    - 1 if successful
 *    - 0 if a memory allocation error occurred
 */
int dsproc_exclude_from_standard_qc_checks(const char *var_name)
{
    const char *xvar;
    char      **new_xvars;
    int         new_nxvars;

    xvar = var_name;

    if ((strlen(var_name) > 3) &&
        (strncmp(var_name, "qc_", 3) == 0)) {

         xvar = &(var_name[3]);
    }

    if (_dsproc_is_excluded_from_standard_qc_checks(xvar)) {
        return(1);
    }

    new_nxvars = NumExQcVars + 1;
    new_xvars  = (char **)realloc(
        ExQcVars, new_nxvars * sizeof(char *));

    if (!new_xvars) goto MEMORY_ERROR;

    ExQcVars = new_xvars;

    if (!(ExQcVars[NumExQcVars] = strdup(xvar))) {
        goto MEMORY_ERROR;
    }

    NumExQcVars += 1;

    return(1);

MEMORY_ERROR:

    ERROR( DSPROC_LIB_NAME,
        "Could not exclude variable from standard QC checks: %s\n"
        " -> memory allocation error\n", var_name);

    dsproc_set_status(DSPROC_ENOMEM);
    return(0);
}

/**
 *  Perform all standard QC checks.
 *
 *  This function calls dsproc_qc_limit_checks() to perform all missing value
 *  and threshold checks. The default bit values used for the missing_value,
 *  valid_min, and valid_max checks are 0x1, 0x2, and 0x4 respectively.
 * 
 *  It will also check if any solar obstruction QC checks are necessary and
 *  call dsproc_qc_solar_obstruction_check() if necessary.
 *
 *  To maintain backward compatibility with older processes and DODs, this
 *  function will also perform the qc_time and valid_delta checks. These
 *  checks are depricated and should not be used by new processes. They
 *  should also be removed from old processes when they are updated.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  ds_id   - datastream ID used to get the previous dataset
 *                    if it is needed for the QC time and delta checks.
 *  @param  dataset - pointer to the dataset
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int dsproc_standard_qc_checks(
    int         ds_id,
    CDSGroup   *dataset)
{
    DataStream *ds           = _DSProc->datastreams[ds_id];
    timeval_t   prev_timeval = { 0, 0 };
    DSFile     *dsfile       = (DSFile *)NULL;
    int         index        = -1;
    int         do_solar_obstruction_check = 0;

    CDSVar     *var;
    CDSVar     *qc_var;
    CDSAtt     *att;
    int         prior_sample_flag = 0;
    int         is_base_time;
    size_t      length;
    int         found;

    int         dc_nvars     = 0;
    CDSVar    **dc_vars      = (CDSVar **)NULL;
    CDSVar    **dc_qc_vars   = (CDSVar **)NULL;
    char      **dc_var_names = (char **)NULL;
    CDSGroup   *dc_dataset   = (CDSGroup *)NULL;

    CDSVar     *prev_var;
    CDSVar     *prev_qc_var;
    int         bad_flags;
    int         vi;

    DEBUG_LV1( DSPROC_LIB_NAME,
        "%s: Applying standard QC checks\n",
        dataset->name);

    /************************************************************
    * Apply the QC time checks
    *************************************************************/

    /* Check if we have a qc_time variable */

    if ((var    = dsproc_get_time_var(dataset)) &&
        (qc_var = dsproc_get_qc_var(var))       &&
        var->sample_count) {

        /* Check if we need the time of the previously stored sample */

        att = cds_get_att(qc_var, "prior_sample_flag");
        if (att) {

            length = 1;
            cds_get_att_value(att, CDS_INT, &length, &prior_sample_flag);

            if (length && prior_sample_flag) {

                /* Get the time of the previously stored sample */

                if (!dsfile) {

                    if (!_dsproc_get_prev_dsfile_time_index(
                        ds, var, &dsfile, &index)) {

                        return(0);
                    }
                }

                if (dsfile && index >= 0) {
                    prev_timeval = dsfile->timevals[index];
                }
            }
        }

        /* Apply the QC time checks */

        if (!dsproc_qc_time_checks(
            var, qc_var, &prev_timeval, 0x1, 0x2, 0x4)) {
            return(0);
        }
    }

    /************************************************************
    * Loop over all variables, applying the QC limit checks, and
    * looking for variables that have solar obstruction checks
    * or delta checks defined.
    *************************************************************/

    /* Check if we should run the solar obstruction checks */

    do_solar_obstruction_check = 0;
    if (dsproc_get_att(dataset, "solar_obstruction_azimuth_range") ||
        dsproc_get_att(dataset, "solar_obstruction_elevation_range")) {

        do_solar_obstruction_check = 1;
    }

    dc_nvars = 0;

    for (vi = 0; vi < dataset->nvars; ++vi) {

        var = dataset->vars[vi];

        /* Skip the time variables */

        if (cds_is_time_var(var, &is_base_time)) {
            continue;
        }

        /* Check for a companion QC variable */

        if (!(qc_var = dsproc_get_qc_var(var))) {
            continue;
        }

        /* Check if this variable has been excluded from the QC checks */

        if (_dsproc_is_excluded_from_standard_qc_checks(var->name)) {
            continue;
        }

        /* Do the QC limit checks */

        if (!dsproc_qc_limit_checks(var, qc_var, 0x1, 0x2, 0x4)) {
            return(0);
        }

        /* Check if we should run the solar obstruction checks */

        if (!do_solar_obstruction_check) {

            if (dsproc_get_att(qc_var, "solar_obstruction_azimuth_range") ||
                dsproc_get_att(qc_var, "solar_obstruction_elevation_range")) {

                do_solar_obstruction_check = 1;
            }
        }

        /* Check for a valid delta attribute */

        found = dsproc_get_data_att(var, "valid_delta", &att);

        if (found < 0) {
            return(0);
        }

        if (found) {

            /* Check if we need to allocate memory for the delta check lists */

            if (dc_nvars == 0) {

                dc_vars      = (CDSVar **)calloc(dataset->nvars, sizeof(CDSVar *));
                dc_qc_vars   = (CDSVar **)calloc(dataset->nvars, sizeof(CDSVar *));
                dc_var_names = (char **)calloc(dataset->nvars, sizeof(char *));

                if (!dc_vars || !dc_var_names) {

                    ERROR( DSPROC_LIB_NAME,
                        "Could not create list of variables that require delta checks in dataset: %s\n"
                        " -> memory allocation error\n",
                        dataset->name);

                    if (dc_vars)      free(dc_vars);
                    if (dc_var_names) free(dc_var_names);

                    dsproc_set_status(DSPROC_ENOMEM);
                    return(0);
                }
            }

            dc_vars[dc_nvars]          = var;
            dc_qc_vars[dc_nvars]       = qc_var;
            dc_var_names[2*dc_nvars]   = var->name;
            dc_var_names[2*dc_nvars+1] = qc_var->name;

            dc_nvars += 1;
        }
    }

    /************************************************************
    * Check if any delta checks were found
    *************************************************************/

    if (dc_nvars) {

        if (prior_sample_flag) {

            /* Get the previously stored values for all
             * variables that have a delta check */

            if (!dsfile) {

                if (!_dsproc_get_prev_dsfile_time_index(
                    ds, dataset, &dsfile, &index)) {

                    return(0);
                }
            }

            if (dsfile && index >= 0) {
                dc_dataset = _dsproc_fetch_dsfile_dataset(
                    dsfile, (size_t)index, 1,
                    2 * dc_nvars, (const char **)dc_var_names, NULL);
            }
        }

        /* Loop over all variables that need delta checks */

        for (vi = 0; vi < dc_nvars; ++vi) {

            var    = dc_vars[vi];
            qc_var = dc_qc_vars[vi];

            if (dc_dataset) {
                prev_var    = dsproc_get_var(dc_dataset, var->name);
                prev_qc_var = (prev_var) ? dsproc_get_qc_var(prev_var) : NULL;
            }
            else {
                prev_var    = (CDSVar *)NULL;
                prev_qc_var = (CDSVar *)NULL;
            }

            /* Revert to hard coding the bad_flags for the QC delta checks.
             * These should only be used by old DODs and processes, and these
             * may not have appropriate assessment values. */

            //bad_flags = dsproc_get_bad_qc_mask(qc_var);
            bad_flags = 0x1 | 0x2 | 0x4;

            if (!dsproc_qc_delta_checks(
                var,
                qc_var,
                prev_var,
                prev_qc_var,
                0x8,
                bad_flags)) {

                return(0);
            }
        }

        /* Free up the memory allocated for the delta checks */

        if (dc_vars)      free(dc_vars);
        if (dc_qc_vars)   free(dc_qc_vars);
        if (dc_var_names) free(dc_var_names);
        if (dc_dataset)   cds_delete_group(dc_dataset);
    }

    /************************************************************
    * Call dsproc_qc_solar_obstruction_check if necessary
    *************************************************************/

    if (do_solar_obstruction_check) {
        if (!dsproc_qc_solar_obstruction_checks(dataset)) {
            return(0);
        }
    }

    return(1);
}

/**
 *  Perform QC delta checks.
 *
 *  This function uses the following variable attribute to determine the
 *  delta limits:
 *
 *    - valid_delta
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  var         - pointer to the variable
 *  @param  qc_var      - pointer to the QC variable
 *  @param  prev_var    - pointer to the variable from the previous dataset
 *                        if delta checks need to be done on the first sample,
 *                        or NULL to skip the delta checks on the first sample.
 *  @param  prev_qc_var - pointer to the QC variable from the previous dataset
 *                        if delta checks need to be done on the first sample,
 *                        or NULL to skip the delta checks on the first sample.
 *  @param  delta_flag  - QC flag to use for failed delta checks.
 *  @param  bad_flags   - QC flags used to determine bad or missing values
 *                        that should not be used for delta checks.
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int dsproc_qc_delta_checks(
    CDSVar *var,
    CDSVar *qc_var,
    CDSVar *prev_var,
    CDSVar *prev_qc_var,
    int     delta_flag,
    int     bad_flags)
{
    size_t  sample_size;
    int     found;
    CDSAtt *att;
    int     ndeltas;
    void   *deltas_vp;
    int    *delta_flags;
    size_t *dim_lengths;
    void   *prev_sample_vp;
    int    *prev_qc_flags;
    int     free_prev_qc = 0;
    size_t  sample_start;
    int     retval;
    int     di;

    /* Make sure the QC variable has an integer data type */

    if (qc_var->type != CDS_INT) {

        ERROR( DSPROC_LIB_NAME,
            "Invalid data type for QC variable: %s\n",
            cds_get_object_path(qc_var));

        dsproc_set_status(DSPROC_EQCVARTYPE);
        return(0);
    }

    /* Make sure the sample sizes match */

    sample_size = dsproc_var_sample_size(var);
    if (!sample_size) {

        ERROR( DSPROC_LIB_NAME,
            "Found zero length dimension for variable: %s\n",
            cds_get_object_path(var));

        dsproc_set_status(DSPROC_ESAMPLESIZE);
        return(0);
    }

    if (dsproc_var_sample_size(qc_var) != sample_size) {

        ERROR( DSPROC_LIB_NAME,
            "QC variable dimensions do not match variable dimensions:\n"
            " - variable    %s has sample size: %d\n"
            " - qc variable %s has sample size: %d\n",
            cds_get_object_path(var), sample_size,
            cds_get_object_path(qc_var), dsproc_var_sample_size(qc_var));

        dsproc_set_status(DSPROC_EQCVARDIMS);
        return(0);
    }

    /* Check if we need to initialize memory for the QC flags */

    if (qc_var->sample_count < var->sample_count) {

        if (!dsproc_init_var_data(
            qc_var, qc_var->sample_count,
            (var->sample_count - qc_var->sample_count), 0)) {

            return(0);
        }
    }

    /* Check for a valid_delta attribute */

    found = dsproc_get_data_att(var, "valid_delta", &att);

    if (found  < 0) return(0);
    if (found == 0) return(1);

    ndeltas   = att->length;
    deltas_vp = att->value.vp;

    if (!ndeltas || !deltas_vp) {
        return(1);
    }

    /* Make sure we actually have data in the variable */

    if (var->sample_count == 0) {
        return(1);
    }

    /* Create the array of dimension lengths */

    dim_lengths = (size_t *)NULL;

    if (var->ndims) {

        dim_lengths = (size_t *)malloc(var->ndims * sizeof(size_t));

        if (!dim_lengths) {

            ERROR( DSPROC_LIB_NAME,
                "Could not perform standard QC delta checks\n"
                " -> memory allocation error\n");

            dsproc_set_status(DSPROC_ENOMEM);
            return(0);
        }

        dim_lengths[0] = var->sample_count;
        for (di = 1; di < var->ndims; di++) {
            dim_lengths[di] = var->dims[di]->length;
        }
    }

    /* Create the array of delta flags */

    delta_flags = (int *)malloc(ndeltas * sizeof(int));

    if (!delta_flags) {

        ERROR( DSPROC_LIB_NAME,
            "Could not perform standard QC delta checks\n"
            " -> memory allocation error\n");

        dsproc_set_status(DSPROC_ENOMEM);
        if (dim_lengths) free(dim_lengths);
        return(0);
    }

    for (di = 0; di < ndeltas; di++) {
        delta_flags[di] = delta_flag;
    }

    /* Check if a previous variable was specified */

    prev_sample_vp = (void *)NULL;
    prev_qc_flags  = (void *)NULL;
    free_prev_qc   = 0;

    if (prev_var &&
        dsproc_var_sample_size(prev_var) == sample_size) {

        sample_start = (prev_var->sample_count - 1) * sample_size;

        if (prev_qc_var &&
            prev_qc_var->type == CDS_INT &&
            prev_qc_var->sample_count >= prev_var->sample_count &&
            dsproc_var_sample_size(prev_qc_var) == sample_size) {

            prev_qc_flags = &(prev_qc_var->data.ip[sample_start]);
        }
        else {

            prev_qc_flags = (int *)calloc(sample_size, sizeof(int));

            if (prev_qc_flags) {

                ERROR( DSPROC_LIB_NAME,
                    "Could not perform standard QC delta checks\n"
                    " -> memory allocation error\n");

                dsproc_set_status(DSPROC_ENOMEM);
                free(delta_flags);
                if (dim_lengths) free(dim_lengths);
                return(0);
            }

            free_prev_qc = 1;
        }

        sample_start  *= cds_data_type_size(prev_var->type);
        prev_sample_vp = (void *)(prev_var->data.bp + sample_start);
    }

    /* Do the QC checks */

    retval = 1;

    if (!cds_qc_delta_checks(
        var->type,
        var->ndims,
        dim_lengths,
        var->data.vp,
        ndeltas,
        deltas_vp,
        delta_flags,
        prev_sample_vp,
        prev_qc_flags,
        bad_flags,
        qc_var->data.ip)) {

        ERROR( DSPROC_LIB_NAME,
            "Could not perform standard QC delta checks\n"
            " -> memory allocation error\n");

        dsproc_set_status(DSPROC_ENOMEM);
        retval = 0;
    }

    free(delta_flags);
    if (dim_lengths)  free(dim_lengths);
    if (free_prev_qc) free(prev_qc_flags);

    return(retval);
}

/**
 * Perform QC limit checks.
 *
 * This function will perform the standard missing value, valid min/max, warn
 * min/max, and fail min/max checks. It will be called automatically by the
 * dsproc_standard_qc_checks() function for b-level datastreams and datastreams
 * that have the DS_STANDARD_QC flag set (see dsproc_set_datastream_flags()).
 *
 * The bit flag to use for each check is specified using the standard bit
 * description attributes and can be defined under the QC variable or as global
 * attributes. When defined under the QC variable they must use the following
 * format:
 *
 *     bit_<#>_description = <bit description>
 *     bit_<#>_assessment = <state>
 *
 * When defined as global attributes they must be prefixed with qc_:
 *
 *     qc_bit_<#>_description = <bit description>
 *     qc_bit_<#>_assessment = <state>
 *
 * where <#> starts at 1 and the assessment <state> is either "Bad" or
 * "Indeterminate".  The missing value, valid min/max, and fail min/max checks
 * should have an assessment state of "Bad", and the warn min/max checks should
 * have an assessment state of "Indeterminate".
 *
 * The default flag arguments are used to maintain backward compatibility with
 * old DODs that do not use the standard bit descriptions described below. A
 * warning message will now be generated if an appropriate bit description can
 * not be found and the default value has to be used.
 *
 * <b>Missing value check:</b>
 *
 * NetCDF files always have a default _FillValue so this check will always be
 * performed if a missing value bit description is defined, even if one of the
 * standard missing value attributes isn't defined. The missing value bit
 * description must begin with one of the following strings:
 *
 *    - "Value is equal to missing_value"
 *    - "Value is equal to the missing_value"
 *    - "value = missing_value"
 *    - "value == missing_value"
 *
 *    - "Value is equal to missing value"
 *    - "Value is equal to the missing value"
 *    - "value = missing value"
 *    - "value == missing value"
 *
 *    - "Value is equal to _FillValue"
 *    - "Value is equal to the _FillValue"
 *    - "value = _FillValue"
 *    - "value == _FillValue"
 *
 * While the bit description attributes must be defined under the QC variable,
 * the missing value attributes must be defined under the <b>data variable</b>.
 * The missing_value attribute can be used to define a missing value that is
 * different than the _FillValue, and the _FillValue attribute can be used to
 * override the NetCDF libraries default value.
 *
 * The default_missing_flag will be used if a missing value bit description is
 * not found but a missing_value or _FillValue attribute was explicitly defined.
 *
 * <b>Valid min/max checks:</b>
 *
 * These checks will be performed if the valid_min and/or valid_max attributes
 * are defined for the <b>data variable</b>. The associated bit descriptions
 * are:
 *
 *   - valid_min:
 *       - "Value is less than valid_min"
 *       - "Value is less than the valid_min"
 *       - "value < valid_min"
 *
 *   - valid_max:
 *       - "Value is greater than valid_max"
 *       - "Value is greater than the valid_max"
 *       - "value > valid_max"
 * 
 * The default_min_flag and/or default_max_flag values will be used if the
 * associated bit description is not found.
 *
 * <b>Warn min/max checks:</b>
 * 
 * These checks will be performed if the warn_min and/or warn_max attributes
 * are defined for the <b>QC variable</b>. The associated bit descriptions are:
 *
 *   - warn_min:
 *       - "Value is less than warn_min"
 *       - "Value is less than the warn_min"
 *       - "value < warn_min"
 *
 *   - warn_max:
 *       - "Value is greater than warn_max"
 *       - "Value is greater than the warn_max"
 *       - "value > warn_max"
 *
 * <b>Fail min/max checks:</b>
 *
 * These checks will be performed if the fail_min and/or fail_max attributes
 * are defined for the <b>QC variable</b>. The associated bit descriptions are:
 *
 *   - fail_min:
 *       - "Value is less than fail_min"
 *       - "Value is less than the fail_min"
 *       - "value < fail_min"
 *
 *   - fail_max:
 *       - "Value is greater than fail_max"
 *       - "Value is greater than the fail_max"
 *       - "value > fail_max"
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  var                  - pointer to the variable
 *  @param  qc_var               - pointer to the QC variable
 *  @param  default_missing_flag - default missing value QC flag
 *  @param  default_min_flag     - default valid_min QC flag
 *  @param  default_max_flag     - default valid_max QC flag
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int dsproc_qc_limit_checks(
    CDSVar *var,
    CDSVar *qc_var,
    int     default_missing_flag,
    int     default_min_flag,
    int     default_max_flag)
{
    size_t        sample_size;
    size_t        nvalues;
    int           nmissings;
    void         *missings_vp   = (void *)NULL;
    int          *missing_flags = (int *)NULL;
    int           mi;
    CDSAtt       *att;
    void         *min_vp;
    void         *max_vp;
    int           found;
    int           missing_flag;
    int           min_flag;
    int           max_flag;

    int           bit_ndescs;
    const char  **bit_descs = (const char **)NULL;

    const char   *tests[] = { "warn", "fail", NULL };
    const char   *test_name;
    char          min_att[32];
    char          max_att[32];
    int           i;

    char          string[128];
    size_t        length;

    /* Make sure the QC variable has integer data type */

    if (qc_var->type != CDS_INT) {

        ERROR( DSPROC_LIB_NAME,
            "Could not perform QC limit checks for: %s\n"
            " -> invalid data type for QC variable: %s\n",
            cds_get_object_path(var),
            cds_get_object_path(qc_var));

        dsproc_set_status(DSPROC_EQCVARTYPE);
        return(0);
    }

    /* Make sure the sample sizes match */

    sample_size = dsproc_var_sample_size(var);
    if (!sample_size) {

        ERROR( DSPROC_LIB_NAME,
            "Could not perform QC limit checks for: %s\n"
            " -> found zero length dimension for variable\n",
            cds_get_object_path(var));

        dsproc_set_status(DSPROC_ESAMPLESIZE);
        return(0);
    }

    if (dsproc_var_sample_size(qc_var) != sample_size) {

        ERROR( DSPROC_LIB_NAME,
            "Could not perform QC limit checks for: %s\n"
            " -> QC variable dimensions do not match variable dimensions:\n"
            " -> variable sample size:    %d\n"
            " -> qc variable sample size: %d\n",
            cds_get_object_path(var),
            sample_size,
            dsproc_var_sample_size(qc_var));

        dsproc_set_status(DSPROC_EQCVARDIMS);
        return(0);
    }

    /* Make sure we actually have data in the variable */

    if (var->sample_count == 0) {
        return(1);
    }

    nvalues = var->sample_count * sample_size;

    /* Check if we need to initialize memory for the QC flags */

    if (qc_var->sample_count < var->sample_count) {

        if (!dsproc_init_var_data(
            qc_var, qc_var->sample_count,
            (var->sample_count - qc_var->sample_count), 0)) {

            return(0);
        }
    }

    /* Get the list of QC bit descriptions */

    bit_ndescs = dsproc_get_qc_bit_descriptions(qc_var, &bit_descs);
    if (bit_ndescs  < 0) return(0);

    /* Get the bit flag to use for the missing_value check */

    nmissings = 0;

    missing_flag = dsproc_get_missing_value_bit_flag(bit_ndescs, bit_descs);
    if (!missing_flag) {

        /* Use the default_missing_flag if a missing_value or _FillValue
         * attribute has been explicitly defined, otherwise we assume the
         * variable shouldn't have any missing values and the check will
         * be disabled. */

        found = dsproc_get_data_att(var, "missing_value", &att);
        if (found < 0) goto ERROR_EXIT;

        if (!found) {
            found = dsproc_get_data_att(var, "_FillValue", &att);
            if (found < 0) goto ERROR_EXIT;
        }

        if (found && default_missing_flag != 0) {

            WARNING( DSPROC_LIB_NAME,
                "Could not find missing_value bit description for: %s\n"
                " -> using default bit flag of: %u",
                cds_get_object_path(qc_var),
                default_missing_flag);

            missing_flag = default_missing_flag;
        }
    }

    /* Get the missing values used by the data variable */

    if (missing_flag) {

        missings_vp = (void *)NULL;
        nmissings   = dsproc_get_var_missing_values(var, &missings_vp);

        if (nmissings < 0) {
            goto ERROR_EXIT;
        }
        else if (nmissings == 0) {
            missing_flag  = 0;
            missing_flags = (int *)NULL;
        }
        else {
            missing_flags = (int *)malloc(nmissings * sizeof(int));

            if (!missing_flags) {

                ERROR( DSPROC_LIB_NAME,
                    "Could not perform QC limit checks for: %s\n"
                    " -> memory allocation error\n",
                    cds_get_object_path(var));

                dsproc_set_status(DSPROC_ENOMEM);
                goto ERROR_EXIT;
            }

            for (mi = 0; mi < nmissings; mi++) {
                missing_flags[mi] = missing_flag;
            }
        }
    }

    /* Get valid min limit and bit flag */

    found = dsproc_get_data_att(var, "valid_min", &att);
    if (found < 0) goto ERROR_EXIT;

    if (found == 0) {
        min_vp   = (void *)NULL;
        min_flag = 0;
    }
    else {
        min_vp   = att->value.vp;
        min_flag = dsproc_get_threshold_test_bit_flag(
            "valid", '<', bit_ndescs, bit_descs);

        if (!min_flag) {
            
            if (default_min_flag != 0) {

                WARNING( DSPROC_LIB_NAME,
                    "Could not find valid_min bit description for: %s\n"
                    " -> using default bit flag of: %u",
                    cds_get_object_path(qc_var),
                    default_min_flag);
            }

            min_flag = default_min_flag;
        }
    }

    /* Get valid max limit and bit flag */

    found = dsproc_get_data_att(var, "valid_max", &att);
    if (found < 0) goto ERROR_EXIT;

    if (found == 0) {
        max_vp   = (void *)NULL;
        max_flag = 0;
    }
    else {
        max_vp   = att->value.vp;
        max_flag = dsproc_get_threshold_test_bit_flag(
            "valid", '>', bit_ndescs, bit_descs);

        if (!max_flag) {

            if (default_max_flag != 0) {

                WARNING( DSPROC_LIB_NAME,
                    "Could not find valid_max bit description for: %s\n"
                    " -> using default bit flag of: %u",
                    cds_get_object_path(qc_var),
                    default_max_flag);
            }

            max_flag = default_max_flag;
        }
    }

    /* Print valid_min, valid_max, and missing value debug information */

    if (msngr_debug_level || msngr_provenance_level) {

        DEBUG_LV2( DSPROC_LIB_NAME,
            " - %s\n",
            var->name);

        if (missings_vp) {
            length = 128;
            cds_array_to_string(var->type, nmissings, missings_vp, &length, string);
            DEBUG_LV2( DSPROC_LIB_NAME,
                "    - bit %d (0x%o):\tmissing_value =\t%s\n",
                (int)log2((double)missing_flag) + 1, missing_flag, string);
        }

        if (min_vp) {
            length = 128;
            cds_array_to_string(var->type, 1, min_vp, &length, string);
            DEBUG_LV2( DSPROC_LIB_NAME,
                "    - bit %d (0x%o):\tvalid_min =\t%s\n",
                (int)log2((double)min_flag) + 1, min_flag, string);
        }

        if (max_vp) {
            length = 128;
            cds_array_to_string(var->type, 1, max_vp, &length, string);
            DEBUG_LV2( DSPROC_LIB_NAME,
                "    - bit %d (0x%o):\tvalid_max =\t%s\n",
                (int)log2((double)max_flag) + 1, max_flag, string);
        }
    }

    /* Perform valid min/max the QC checks */

    if (min_flag || max_flag || missing_flags) {

        cds_qc_limit_checks(
                var->type,
                nvalues,
                var->data.vp,
                nmissings,
                missings_vp,
                missing_flags,
                min_vp,
                min_flag,
                max_vp,
                max_flag,
                qc_var->data.ip);
    }

    /* Permform warn and fail QC checks */
 
    for (i = 0; tests[i]; ++i) {

        test_name = tests[i];
        sprintf(min_att, "%s_min", test_name);
        sprintf(max_att, "%s_max", test_name);

        /* Get min limit and bit flag */

        found = dsproc_get_qc_data_att(var, qc_var, min_att, &att);
        if (found < 0) goto ERROR_EXIT;

        if (found == 0) {
            min_vp   = (void *)NULL;
            min_flag = 0;
        }
        else {
            min_vp   = att->value.vp;
            min_flag = dsproc_get_threshold_test_bit_flag(
                test_name, '<', bit_ndescs, bit_descs);

            if (!min_flag) {

                ERROR( DSPROC_LIB_NAME,
                    "Could not find %s bit description for: %s\n",
                    min_att,
                    cds_get_object_path(qc_var));

                dsproc_set_status(DSPROC_ENOBITDESC);
                goto ERROR_EXIT;
            }
        }

        /* Get max limit and bit flag */

        found = dsproc_get_qc_data_att(var, qc_var, max_att, &att);
        if (found < 0) goto ERROR_EXIT;

        if (found == 0) {
            max_vp   = (void *)NULL;
            max_flag = 0;
        }
        else {
            max_vp   = att->value.vp;
            max_flag = dsproc_get_threshold_test_bit_flag(
                test_name, '>', bit_ndescs, bit_descs);

            if (!max_flag) {

                ERROR( DSPROC_LIB_NAME,
                    "Could not find %s bit description for: %s\n",
                    max_att,
                    cds_get_object_path(qc_var));

                dsproc_set_status(DSPROC_ENOBITDESC);
                goto ERROR_EXIT;
            }
        }

        /* Print debug information */

        if (msngr_debug_level || msngr_provenance_level) {

            if (min_vp) {
                length = 128;
                cds_array_to_string(var->type, 1, min_vp, &length, string);
                DEBUG_LV2( DSPROC_LIB_NAME,
                    "    - bit %d (0x%o):\t%s =\t%s\n",
                    (int)log2((double)min_flag) + 1, min_flag, min_att, string);
            }

            if (max_vp) {
                length = 128;
                cds_array_to_string(var->type, 1, max_vp, &length, string);
                DEBUG_LV2( DSPROC_LIB_NAME,
                    "    - bit %d (0x%o):\t%s =\t%s\n",
                    (int)log2((double)max_flag) + 1, max_flag, max_att, string);
            }
        }

        /* Perform the QC checks */

        if (min_flag || max_flag) {

            cds_qc_limit_checks(
                    var->type,
                    nvalues,
                    var->data.vp,
                    nmissings,
                    missings_vp,
                    missing_flags,
                    min_vp,
                    min_flag,
                    max_vp,
                    max_flag,
                    qc_var->data.ip);
        }
    }

    /* Cleanup and exit */

    if (missings_vp)   free(missings_vp);
    if (missing_flags) free(missing_flags);
    if (bit_descs)     free(bit_descs);

    return(1);
    
ERROR_EXIT:

    if (missings_vp)   free(missings_vp);
    if (missing_flags) free(missing_flags);
    if (bit_descs)     free(bit_descs);

    return(0);
}

/**
 *  Perform solar obstruction QC check.
 *
 *  This function will perform the standard solar obstruction check used to flag
 *  data that is shaded by some obstruction. It will be called automatically by
 *  the dsproc_standard_qc_checks() function for b-level datastreams and
 *  datastreams that have the DS_STANDARD_QC flag set
 *  (see dsproc_set_datastream_flags()).
 *
 *  The following QC variable attributes are used to specify the elevation
 *  and azimuth bounds of the solar positions that cause shading.
 *
 *    - solar_obstruction_azimuth_range 
 *    - solar_obstruction_elevation_range
 *
 *  The bit flag to use is specified using a standard bit description
 *  attribute with the value:
 * 
 *    - "Instrument shaded, solar position is within region defined by solar_obstruction_azimuth_range and solar_obstruction_elevation_range."
 * 
 *  The standard bit description attributes can be defined under the QC variable
 *  or as global attributes. When defined under the QC variable they must use
 *  the following format:
 *
 *     bit_<#>_description = <bit description>
 *     bit_<#>_assessment = <state>
 *
 *  When defined as global attributes they must be prefixed with qc_:
 *
 *     qc_bit_<#>_description = <bit description>
 *     qc_bit_<#>_assessment = <state>
 *
 *  where <#> starts at 1 and the assessment <state> is either "Bad" or
 *  "Indeterminate".
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  ntimes     - number of times
 *  @param  times      - pointer to array of times
 *  @param  azimuths   - pointer to the array of solar azimuths
 *  @param  elevations - pointer to the array of solar elevations
 *  @param  qc_var     - pointer to the QC variable
 *
 *  @return
 *    - 1 if successful (or not applicable)
 *    - 0 if an error occurred
 */
int dsproc_qc_solar_obstruction_check(
    size_t  ntimes,
    time_t *times,
    double *azimuths,
    double *elevations,
    CDSVar *qc_var)
{
    CDSGroup *dataset = (CDSGroup *)qc_var->parent;
    int       retval  = 0;
    CDSAtt   *att;
    size_t    length;
    int       found_az_bounds;
    int       found_el_bounds;
    double    azimuth_bounds[2];
    double    elevation_bounds[2];
    double    min_azi = 0;
    double    max_azi = 0;
    double    min_ele = 0;
    double    max_ele = 0;
    size_t    sample_size;
    double    azimuth, elevation;
    int      *qc_datap;
    int       start_shading;
    char      ts[32];
    size_t    ti, si;

    int          bit_ndescs;
    const char **bit_descs = (const char **)NULL;
    unsigned int solar_flag;

    /* Check for solar_obstruction_azimuth_range attribute */

    found_az_bounds = 0;
    if ((att = dsproc_get_att(qc_var,  "solar_obstruction_azimuth_range")) ||
        (att = dsproc_get_att(dataset, "solar_obstruction_azimuth_range"))) {

        found_az_bounds = 1;

        if (att->length != 2) {
            ERROR( DSPROC_LIB_NAME,
                "Could not perform solar obstruction QC checks for: %s\n"
                " -> solar_obstruction_azimuth_range has %d values but expected 2\n",
                qc_var->name, att->length);

            dsproc_set_status("Invalid solar_obstruction_azimuth_range length");
            goto ERROR_EXIT;
        }

        length = 2;
        cds_get_att_value(att, CDS_DOUBLE, &length, azimuth_bounds);

        min_azi = azimuth_bounds[0];
        max_azi = azimuth_bounds[1];

        /* Check if azimuth bounds are within the range 0-360, inclusive */

        if ((min_azi < 0.0) || (min_azi >= 360.0) || 
            (max_azi < 0.0) || (max_azi >= 360.0)) {

            ERROR( DSPROC_LIB_NAME,
                "Invalid solar_obstruction_azimuth_range [%f, %f] for: %s\n"
                " -> valid range is 0.0 to 360.0\n",
                min_azi, max_azi, qc_var->name);

            dsproc_set_status("Invalid solar_obstruction_azimuth_range values");
            goto ERROR_EXIT;
        }

        if (min_azi > max_azi) {
            /* adjust min to be less than max */
            min_azi -= 360;
        }
    }

    /* Check for solar_obstruction_elevation_range attribute */

    found_el_bounds = 0;
    if ((att = dsproc_get_att(qc_var,  "solar_obstruction_elevation_range")) ||
        (att = dsproc_get_att(dataset, "solar_obstruction_elevation_range"))) {

        found_el_bounds = 1;

        if (att->length != 2) {
            ERROR( DSPROC_LIB_NAME,
                "Could not perform solar obstruction QC checks for: %s\n"
                " -> solar_obstruction_elevation_range has %d values but expected 2\n",
                qc_var->name, att->length);

            dsproc_set_status("Invalid solar_obstruction_elevation_range length");
            goto ERROR_EXIT;
        }

        length = 2;
        cds_get_att_value(att, CDS_DOUBLE, &length, elevation_bounds);

        min_ele = elevation_bounds[0];
        max_ele = elevation_bounds[1];

        /* Check if elevation bounds are within the range -90 to 90, inclusive */
        if( (min_ele < -90.0) || (min_ele > 90.0) || 
            (max_ele < -90.0) || (max_ele > 90.0)   ) {

            ERROR( DSPROC_LIB_NAME,
                "Invalid solar_obstruction_elevation_range [%f, %f] for: %s\n"
                " -> valid range is 0.0 to 360.0\n",
                min_ele, max_ele, qc_var->name);

            dsproc_set_status("Invalid solar_obstruction_elevation_range values");
            goto ERROR_EXIT;
        }

        if (min_ele > max_ele) {

            ERROR( DSPROC_LIB_NAME,
                "Invalid solar_obstruction_elevation_range [%f, %f] for: %s\n"
                " -> lower limit is greater than upper limit\n",
                min_ele, max_ele, qc_var->name);

            dsproc_set_status("Invalid solar_obstruction_elevation_range values");
            goto ERROR_EXIT;
        }
    }

    if (!found_az_bounds) {

        if (!found_el_bounds) {
            /* Variable doesn't have solar obstruction check */
            return(1);
        }

        ERROR( DSPROC_LIB_NAME,
            "Missing solar_obstruction_azimuth_range attribute for: %s\n",
            qc_var->name);

        dsproc_set_status("Missing solar_obstruction_azimuth_range attribute");
        goto ERROR_EXIT;
    }
    else if (!found_el_bounds) {
        ERROR( DSPROC_LIB_NAME,
            "Missing solar_obstruction_elevation_range attribute for: %s\n",
            qc_var->name);

        dsproc_set_status("Missing solar_obstruction_elevation_range attribute");
        goto ERROR_EXIT;
    }

    /* Get the list of QC bit descriptions */

    bit_ndescs = dsproc_get_qc_bit_descriptions(qc_var, &bit_descs);
    if (bit_ndescs  < 0) goto ERROR_EXIT;
    if (bit_ndescs == 0) return(1);

    /* Get the QC flag to use for the solar obstruction check */

    solar_flag = dsproc_get_solar_obstruction_bit_flag(bit_ndescs, bit_descs);
    if (!solar_flag) {

        if (!dsproc_get_att(qc_var, "solar_obstruction_azimuth_range") &&
            !dsproc_get_att(qc_var, "solar_obstruction_elevation_range")) {
            
            /* These are global attributes so we can assume that this
             * variable doesn't have a solar obstruction check */

            free(bit_descs);
            return(1);
        }

        ERROR( DSPROC_LIB_NAME,
            "Could not find solar obstruction check bit description for: %s\n",
            qc_var->name);

        dsproc_set_status(DSPROC_ENOBITDESC);
        goto ERROR_EXIT;
    }

    /* Make sure the QC variable has integer data type */
    if (qc_var->type != CDS_INT) {

        ERROR( DSPROC_LIB_NAME,
            "Could not perform solar obstruction QC check\n"
            " -> invalid data type for QC variable: %s\n",
            cds_get_object_path(qc_var));

        dsproc_set_status(DSPROC_EQCVARTYPE);
        goto ERROR_EXIT;
    }

    /* Get qc_var sample size */
    sample_size = dsproc_var_sample_size(qc_var);
    if (!sample_size) {

        ERROR( DSPROC_LIB_NAME,
            "Could not perform solar obstruction QC check\n"
            " -> found zero length dimension for variable: %s\n",
            cds_get_object_path(qc_var));

        dsproc_set_status(DSPROC_ESAMPLESIZE);
        goto ERROR_EXIT;
    }

    /* Check if we need to initialize memory for the QC flags */
    if (qc_var->sample_count < ntimes) {

        if (!dsproc_init_var_data(qc_var, qc_var->sample_count,
            (ntimes - qc_var->sample_count), 0)) {

            goto ERROR_EXIT;
        }
    }

    if (msngr_debug_level || msngr_provenance_level) {

        DEBUG_LV2( DSPROC_LIB_NAME,
            " - %s\n",
            qc_var->name);

        DEBUG_LV2( DSPROC_LIB_NAME, 
            "    - bit %d (0x%o): solar obstruction qc check\n"
            "        - solar_obstruction_azimuth_range   = [%g, %g]\n"
            "        - solar_obstruction_elevation_range = [%g, %g]\n",
            (int)log2((double)solar_flag) + 1, solar_flag,
            min_azi, max_azi,
            min_ele, max_ele);
    }

    /* Do the QC check */

    start_shading = 0;
    qc_datap = qc_var->data.ip;

    for (ti = 0; ti < ntimes; ++ti) {

        azimuth   = azimuths[ti];
        elevation = elevations[ti];

        /* Make sure azimuth is in the appropriate range */
        if ((min_azi < 0) && (azimuth > max_azi)) {
            azimuth -= 360;
        }

        /* Check if position is within bounds */
        if ((min_azi <= azimuth)   && (azimuth <= max_azi)  &&  
            (min_ele <= elevation) && (elevation <= max_ele)) {

            if (!start_shading) {
                DEBUG_LV2( DSPROC_LIB_NAME,
                    "        - shading start = %s\n"
                    "            - azimuth   = %g\n"
                    "            - elevation = %g\n",
                    format_secs1970(times[ti], ts),
                    azimuth, elevation);
                start_shading = 1;
            }

            for (si = 0; si < sample_size; ++si) {
                *qc_datap++ |= solar_flag;
            }
        }
        else {
            if (start_shading) {
                DEBUG_LV2( DSPROC_LIB_NAME,
                    "        - shading end   = %s\n"
                    "            - azimuth   = %g\n"
                    "            - elevation = %g\n",
                    format_secs1970(times[ti], ts),
                    azimuth, elevation);
                start_shading = 0;
            }

            qc_datap += sample_size;
        }
    }

    retval = 1;

ERROR_EXIT:

    if (bit_descs) free(bit_descs);
    return(retval);
}

/**
 *  Perform solar obstruction QC check for all appropriate variables.
 *
 *  This function loops over all variables in the specified dataset and
 *  performs the solar obstruction check for the variables that have
 *  the appropriate metadata defining the check.
 *
 *  See dsproc_qc_solar_obstruction_check() for a decsription of the required
 *  metadata to define the qc check.
 * 
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  dataset - pointer to the dataset
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int dsproc_qc_solar_obstruction_checks(
    CDSGroup   *dataset)
{
    time_t *times      = (time_t *)NULL;
    double *azimuths   = (double *)NULL;
    double *elevations = (double *)NULL;
    int     retval     = 0;
    size_t  ntimes;
    double  lat;
    double  lon;

    CDSVar *var;
    CDSVar *qc_var;

    int     is_base_time;
    int     status;
    int     vi;

    DEBUG_LV1( DSPROC_LIB_NAME,
        "%s: Applying solar obstruction QC checks\n",
        dataset->name);

    /************************************************************
    * Get information needed by all checks
    *************************************************************/

    /* Get latitude and longitude */

    if (!dsproc_get_dataset_location(dataset, &lat, &lon, NULL)) {
        goto ERROR_EXIT;
    }

    DEBUG_LV1( DSPROC_LIB_NAME,
        " - latitude:  %g\n"
        " - longitude: %g\n",
        lat, lon);

    /* Get sample times */

    times = dsproc_get_sample_times(dataset, 0, &ntimes, NULL);
    if (!times) goto ERROR_EXIT;

    /* Get solar azimuths and elevations */

    status = dsproc_solar_positions(
        ntimes, times, lat, lon,
        NULL,       // ap_ra,
        NULL,       // ap_dec,
        &elevations,
        NULL,       // refraction,
        &azimuths,
        NULL);      // distance

    if (status <= 0) {
        goto ERROR_EXIT;
    }

    /************************************************************
    * Loop over all variables, applying the solar obstruction
    * checks to variables with the appropriate metadata defined.
    *************************************************************/

    for (vi = 0; vi < dataset->nvars; ++vi) {

        var = dataset->vars[vi];

        /* Skip the time variables */
        if (cds_is_time_var(var, &is_base_time)) {
            continue;
        }

        /* Check for a companion QC variable */
        if (!(qc_var = dsproc_get_qc_var(var))) {
            continue;
        }

        /* Check if this variable has been excluded from the QC checks */
        if (_dsproc_is_excluded_from_standard_qc_checks(var->name)) {
            continue;
        }

        /* Run solar obstruction QC check */
        if (!dsproc_qc_solar_obstruction_check(
            ntimes, times, azimuths, elevations, qc_var)) {

            goto ERROR_EXIT;
        }
    }

    retval = 1;

ERROR_EXIT:

    if (times)      free(times);
    if (azimuths)   free(azimuths);
    if (elevations) free(elevations);

    return(retval);
}

/**
 *  Perform QC time checks.
 *
 *  This function uses the following time variable attributes to determine
 *  the lower and upper delta time limits:
 *
 *     - delta_t_lower_limit
 *     - delta_t_upper_limit
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  time_var       - pointer to the time variable
 *  @param  qc_time_var    - pointer to the qc_time variable
 *  @param  prev_timeval   - the timeval of the previous sample in seconds
 *                           since 1970-1-1 00:00:00, or NULL to skip the
 *                           checks for the first sample time.
 *
 *  @param  lteq_zero_flag - QC flag to use for time deltas <= zero
 *  @param  min_delta_flag - QC flag to use for time deltas below the minimum
 *  @param  max_delta_flag - QC flag to use for time deltas above the maximum
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int dsproc_qc_time_checks(
    CDSVar    *time_var,
    CDSVar    *qc_time_var,
    timeval_t *prev_timeval,
    int        lteq_zero_flag,
    int        min_delta_flag,
    int        max_delta_flag)
{
    CDSAtt     *att;
    size_t      length;
    CDSData     min_delta;
    char        min_delta_buff[16];
    CDSData     max_delta;
    char        max_delta_buff[16];

    time_t      base_time;
    time_t      prev_tv_sec;
    CDSData     prev_offset;
    char        prev_offset_buff[16];

    /* Make sure the QC variable has integer data type */

    if (qc_time_var->type != CDS_INT) {

        ERROR( DSPROC_LIB_NAME,
            "Could not perform QC time checks\n"
            " -> invalid data type for QC time variable: %s\n",
            cds_get_object_path(qc_time_var));

        dsproc_set_status(DSPROC_EQCVARTYPE);
        return(0);
    }

    /* Check if we need to initialize memory for the QC flags */

    if (qc_time_var->sample_count < time_var->sample_count) {

        if (!dsproc_init_var_data(
            qc_time_var, qc_time_var->sample_count,
            (time_var->sample_count - qc_time_var->sample_count), 0)) {

            return(0);
        }
    }

    /* Get the delta_t_lower_limit attribute value */

    att = cds_get_att(qc_time_var, "delta_t_lower_limit");
    if (att) {
        length       = 1;
        min_delta.cp = min_delta_buff;
        cds_get_att_value(att, time_var->type, &length, min_delta.vp);
    }
    else {
        min_delta.vp = (void *)NULL;
    }

    /* Get the delta_t_upper_limit attribute value */

    att = cds_get_att(qc_time_var, "delta_t_upper_limit");
    if (att) {
        length       = 1;
        max_delta.cp = max_delta_buff;
        cds_get_att_value(att, time_var->type, &length, max_delta.vp);
    }
    else {
        max_delta.vp = (void *)NULL;
    }

    /* Make sure we have data in the time variable  */

    if (time_var->sample_count == 0) {
        return(1);
    }

    /* Check if a previous time was specified */

    prev_offset.vp = (void *)NULL;

    if (prev_timeval &&
        prev_timeval->tv_sec > 0) {

        base_time = cds_get_base_time(time_var);
        if (base_time < 0) {

            ERROR( DSPROC_LIB_NAME,
                "Could not perform QC time checks\n"
                " -> could not get base_time for variable: %s\n",
                cds_get_object_path(time_var));

            dsproc_set_status(DSPROC_EBASETIME);
            return(0);
        }

        prev_tv_sec    = prev_timeval->tv_sec - base_time;
        prev_offset.cp = prev_offset_buff;

        switch (time_var->type) {
            case CDS_DOUBLE: *prev_offset.dp = (double)prev_tv_sec + (double)prev_timeval->tv_usec * 1E-6; break;
            case CDS_FLOAT:  *prev_offset.fp = (float)prev_tv_sec  + (float)prev_timeval->tv_usec * 1E-6;  break;
            case CDS_INT:    *prev_offset.ip = (int)prev_tv_sec;           break;
            case CDS_SHORT:  *prev_offset.sp = (short)prev_tv_sec;         break;
            case CDS_BYTE:   *prev_offset.bp = (signed char)prev_tv_sec;   break;
            case CDS_CHAR:   *prev_offset.cp = (unsigned char)prev_tv_sec; break;
            default:
                break;
        }
    }

    /* Do the QC checks */

    cds_qc_time_offset_checks(
        time_var->type,
        time_var->sample_count,
        time_var->data.vp,
        prev_offset.vp,
        lteq_zero_flag,
        min_delta.vp,
        min_delta_flag,
        max_delta.vp,
        max_delta_flag,
        qc_time_var->data.ip);

    return(1);
}
