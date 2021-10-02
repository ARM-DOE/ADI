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

/** @file dsproc_dataset_vars.c
 *  Dataset Variable Functions.
 */

#include "dsproc3.h"
#include "dsproc_private.h"

extern DSProc *_DSProc; /**< Internal DSProc structure */

/** @privatesection */

/*******************************************************************************
 *  Static Functions Visible Only To This Module
 */

/*******************************************************************************
 *  Private Functions Visible Only To This Library
 */

/**
 *  PRIVATE: Fix the order of dimensions and fields in a dataset.
 *
 *  @param  ds - pointer to the dataset
 */
void _dsproc_fix_field_order(CDSGroup *ds)
{
    CDSDim *dim;
    CDSVar *var;
    CDSAtt *att;
    int     di, ni, vi, tvi;

    const char *name;

    /* Make sure the time dimension is first */

    for (di = 0; di < ds->ndims; ++di) {

        dim = ds->dims[di];

        if (strcmp(dim->name, "time") == 0) {

            for (; di > 0; --di) {
                ds->dims[di] = ds->dims[di-1];
            }

            ds->dims[0] = dim;
            break;
        }
    }

    /* Make sure the bound dimension is last */

    for (di = 0; di < ds->ndims; ++di) {

        dim = ds->dims[di];

        if (strcmp(dim->name, "bound") == 0) {

            for (++di; di < ds->ndims; ++di) {
                ds->dims[di-1] = ds->dims[di];
            }

            ds->dims[ds->ndims-1] = dim;
            break;
        }
    }

    /* Move base_time and time_offset to the top of the vars list */

    tvi = 0;

    for (ni = 0; ni < 2; ++ni) {

        name = (ni == 0) ? "base_time" : "time_offset";

        for (vi = tvi; vi < ds->nvars; ++vi) {

            var = ds->vars[vi];

            if (strcmp(var->name, name) == 0) {

                for (; vi > tvi; --vi) {
                    ds->vars[vi] = ds->vars[vi-1];
                }
                
                ds->vars[tvi++] = var;
            }
        }
    }

    /* Make sure the coordinate variables are next */

    for (di = 0; di < ds->ndims; ++di) {

        dim = ds->dims[di];

        for (vi = tvi; vi < ds->nvars; ++vi) {

            var = ds->vars[vi];

            if (var->ndims   == 1   &&
                var->dims[0] == dim &&
                strcmp(var->name, dim->name) == 0) {

                /* Move the coordinate variable to the correct location */

                for (; vi > tvi; --vi) {
                    ds->vars[vi] = ds->vars[vi-1];
                }

                ds->vars[tvi++] = var;

                /* Check for the bounds attribute */

                att = cds_get_att(var, "bounds");
                if (att &&
                    att->type == CDS_CHAR &&
                    att->length > 0 &&
                    att->value.cp) {

                    for (vi = tvi; vi < ds->nvars; ++vi) {

                        var = ds->vars[vi];

                        if (strcmp(var->name, att->value.cp) == 0) {

                            /* Move the boundary variable */

                            for (; vi > tvi; --vi) {
                                ds->vars[vi] = ds->vars[vi-1];
                            }
                            ds->vars[tvi++] = var;
                        }
                    }
                }

                break;
            }

        } /* end loop over vars */

    } /* end loop over dims */
}

/** @publicsection */

/*******************************************************************************
 *  Internal Functions Visible To The Public
 */

/*******************************************************************************
 *  Public Functions
 */

/**
 *  Allocate memory for a variable's data array.
 *
 *  This function will allocate memory as necessary to ensure that the
 *  variable's data array is large enough to store another sample_count
 *  samples starting from sample_start.
 *
 *  The data type of the returned array will be the same as the variable’s
 *  data type. It is the responsibility of the calling process to cast the
 *  returned array into the proper data type. If the calling process does not
 *  know the data type of the variable, it can store the data in an array
 *  of a known type and then use the dsproc_set_var_data() function to cast
 *  this data into the variables data array. In this case the memory does not
 *  need to be preallocated and this function is not needed.
 *
 *  The data array returned by this function belongs to the variable
 *  and will be freed when the variable is destroyed. The calling process
 *  must *not* attempt to free this memory.
 *
 *  For multi-dimensional variables the data array is stored linearly
 *  in memory with the last dimension varying the fastest. See the
 *  dsproc_alloc_var_data_index() and/or dsproc_get_var_data_index()
 *  functions to get a "data index" into this array.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  var          - pointer to the variable
 *  @param  sample_start - start sample (0 based indexing)
 *  @param  sample_count - number of new samples
 *
 *  @return
 *    - pointer to the specifed start sample in the variable's data array
 *    - NULL if:
 *        - one of the variable's static dimensions has 0 length
 *        - the variable has no dimensions, and sample_start is not equal
 *          to 0 or sample_count is not equal to 1.
 *        - the first variable dimension is not unlimited, and
 *          sample_start + sample_count would exceed the dimension length.
 *        - a memory allocation error occurred
 */
void *dsproc_alloc_var_data(
    CDSVar *var,
    size_t  sample_start,
    size_t  sample_count)
{
    void *datap;

    datap = cds_alloc_var_data(var, sample_start, sample_count);

    if (!datap) {
        dsproc_set_status(DSPROC_ECDSALLOCVAR);
    }

    return(datap);
}

/**
 *  Allocate memory for a variable's data array.
 *
 *  This function is the same as dsproc_alloc_var_data() except that
 *  it returns a data index starting at the specified start sample
 *  (see dsproc_get_var_data_index()). For variables that have less than
 *  two dimensions this function is identical to dsproc_alloc_var_data().
 *  It is up to the calling process to cast the return value of this
 *  function into the proper data type.
 *
 *  The data index returned by this function belongs to the variable
 *  and will be freed when the variable is destroyed. The calling process
 *  must *not* attempt to free this memory.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  var          - pointer to the variable
 *  @param  sample_start - start sample (0 based indexing)
 *  @param  sample_count - number of new samples
 *
 *  @return
 *    - the data index into the variable's data array starting at the
 *      specified start sample
 *    - NULL if:
 *        - one of the variable's static dimensions has 0 length
 *        - the variable has no dimensions, and sample_start is not equal
 *          to 0 or sample_count is not equal to 1.
 *        - the first variable dimension is not unlimited, and
 *          sample_start + sample_count would exceed the dimension length.
 *        - a memory allocation error occurred
 */
void *dsproc_alloc_var_data_index(
    CDSVar *var,
    size_t  sample_start,
    size_t  sample_count)
{
    void *datap;

    datap = cds_alloc_var_data_index(var, sample_start, sample_count);

    if (!datap) {
        dsproc_set_status(DSPROC_ECDSALLOCVAR);
    }

    return(datap);
}

/**
 *  Create a clone of an existing variable.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  src_var    - pointer to the source variable to clone
 *  @param  dataset    - dataset to create the new variable in,
 *                       or NULL to create the variable in the same dataset
 *                       the source variable belongs to.
 *  @param  var_name   - name to use for the new variable,
 *                       or NULL to use the source variable name.
 *  @param  data_type  - data type to use for the new variable,
 *                       or CDS_NAT to use the same data type as the source
 *                       variable.
 *  @param  dim_names  - pointer to the list of corresponding dimension names
 *                       in the dataset the new variable will be created in,
 *                       or NULL if the dimension names are the same.
 *  @param  copy_data  - flag indicating if the data should be copied,
 *                       (1 == TRUE, 0 == FALSE)
 *
 *  @return
 *    - pointer to the new variable
 *    - NULL if:
 *        - the variable already exists in the dataset
 *        - a memory allocation error occurred
 */
CDSVar *dsproc_clone_var(
    CDSVar       *src_var,
    CDSGroup     *dataset,
    const char   *var_name,
    CDSDataType   data_type,
    const char  **dim_names,
    int           copy_data)
{
    CDSVar     *clone;
    CDSDim     *dim;
    const char *dims[NC_MAX_DIMS];
    int         status;
    int         di;

    /* Set NULL argument values */

    if (!dataset)   dataset   = (CDSGroup *)src_var->parent;
    if (!var_name)  var_name  = src_var->name;
    if (!data_type) data_type = src_var->type;
    if (!dim_names) {
        for (di = 0; di < src_var->ndims; di++) {
            dims[di] = src_var->dims[di]->name;
        }
        dim_names = dims;
    }

    /* Make sure this variable doesn't exist in the specified dataset */

    clone = cds_get_var(dataset, var_name);

    if (clone) {

        ERROR( DSPROC_LIB_NAME,
            "Could not clone variable:\n"
            " -> from: %s\n"
            " -> to:   %s\n"
            " -> destination variable already exists\n",
            cds_get_object_path(src_var),
            cds_get_object_path(clone));

        dsproc_set_status(DSPROC_ECLONEVAR);
        return((CDSVar *)NULL);
    }

    /* Define the dimensions used by this variable if necessary */

    for (di = 0; di < src_var->ndims; di++) {
        dim = cds_get_dim(dataset, dim_names[di]);
        if (!dim) {

            if (!cds_define_dim(dataset, dim_names[di],
                src_var->dims[di]->length, src_var->dims[di]->is_unlimited)) {

                dsproc_set_status(DSPROC_ECLONEVAR);
                return((CDSVar *)NULL);
            }
        }
    }

    /* Define the variable */

    clone = cds_define_var(
        dataset, var_name, data_type, src_var->ndims, dim_names);

    if (!clone) {
        dsproc_set_status(DSPROC_ENOMEM);
        return((CDSVar *)NULL);
    }

    /* Copy over the attributes and data */

    if (copy_data) {
        status = cds_copy_var(
            src_var, dataset, var_name, NULL, NULL, NULL, NULL,
            0, 0, src_var->sample_count, 0, NULL);
    }
    else {
        status = cds_copy_var(
            src_var, dataset, var_name, NULL, NULL, NULL, NULL,
            0, 0, 0, CDS_SKIP_DATA, NULL);
    }

    if (status < 0) {
        dsproc_set_status(DSPROC_ENOMEM);
        return((CDSVar *)NULL);
    }

    return(clone);
}

/**
 *  Define a new variable in an existing dataset.
 *
 *  This function will define a new variable with all standard attributes.
 *  Any of the attribute values can be NULL to indicate that the attribute
 *  should not be created.
 *
 *  Description of Attributes:
 *
 *  <b>long_name:</b>
 *      This is a one line description of the variable and should be suitable
 *      to use as a plot title for the variable.
 *
 *  <b>standard_name:</b>
 *      This is defined in the CF Convention and describes the physical
 *      quantities being represented by the variable. Please refer to the
 *      "CF Standard Names" section of the CF Convention for the table of
 *      standard names.
 *
 *  <b>units:</b>
 *      This is the units string to use for the variable and must be
 *      recognized by the UDUNITS-2 libary.
 *
 *  <b>valid_min:</b>
 *      The smallest value that should be considered to be a valid data value.
 *      The specified value must be the same data type as the variable.
 *
 *  <b>valid_max:</b>
 *      The largest value that should be considered to be a valid data value.
 *      The specified value must be the same data type as the variable.
 *
 *  <b>missing_value:</b>
 *      This comes from an older NetCDF convention and has been used by ARM
 *      for almost 2 decades. The specified value must be the same data type
 *      as the variable.
 *
 *  <b>_FillValue:</b>
 *      Most newer conventions specify the use of _FillValue over missing_value.
 *      The value of this attribute is also recognized by the NetCDF library and
 *      will be used to initialize the data values on disk when the variable is
 *      created. Tools like ncdump will also display fill values as _ so they
 *      can be easily identified in a text dump. The libdsproc3 library allows
 *      you to use both missing_value and _FillValue and they do not need to be
 *      the same. The specified value must be the same data type as the variable.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  dataset       - pointer to the dataset
 *  @param  name          - name of the variable
 *  @param  type          - data type of the variable
 *  @param  ndims         - number of variable dimensions
 *  @param  dim_names     - array of pointers to the dimension names
 *  @param  long_name     - string to use for the long_name attribute
 *  @param  standard_name - string to use for the standard_name attribute
 *  @param  units         - string to use for the units attribute
 *  @param  valid_min     - void pointer to the valid_min
 *  @param  valid_max     - void pointer to the valid_max
 *  @param  missing_value - void pointer to the missing_value
 *  @param  fill_value    - void pointer to the _FillValue
 *
 *  @return
 *    - pointer to the new variable
 *    - NULL if an error occurred
 */
CDSVar *dsproc_define_var(
    CDSGroup    *dataset,
    const char  *name,
    CDSDataType  type,
    int          ndims,
    const char **dim_names,
    const char  *long_name,
    const char  *standard_name,
    const char  *units,
    void        *valid_min,
    void        *valid_max,
    void        *missing_value,
    void        *fill_value)
{
    CDSVar *var;

    var = cds_define_var(dataset, name, type, ndims, dim_names);
    if (!var) {
        dsproc_set_status(DSPROC_ECDSDEFVAR);
        return((CDSVar *)NULL);
    }

    if ((long_name     && !cds_define_att_text(var, "long_name",     "%s", long_name))     ||
        (standard_name && !cds_define_att_text(var, "standard_name", "%s", standard_name)) ||
        (units         && !cds_define_att_text(var, "units",         "%s", units))         ||
        (valid_min     && !cds_define_att(var, "valid_min",     type, 1, valid_min))       ||
        (valid_max     && !cds_define_att(var, "valid_max",     type, 1, valid_max))       ||
        (missing_value && !cds_define_att(var, "missing_value", type, 1, missing_value))   ||
        (fill_value    && !cds_define_att(var, "_FillValue",    type, 1, fill_value)) ) {

        ERROR( DSPROC_LIB_NAME,
            "Could not define variable: %s\n"
            " -> memory allocation error\n",
            cds_get_object_path(var));

        cds_delete_var(var);
        dsproc_set_status(DSPROC_ENOMEM);
        return((CDSVar *)NULL);
    }

    return(var);
}

/**
 *  Delete a variable from a dataset.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  var - pointer to the variable
 *
 *  @return
 *    - 1 if the variable was deleted (or the input var was NULL)
 *    - 0 if:
 *        - the variable is locked
 *        - the group is locked
 */
int dsproc_delete_var(CDSVar *var)
{
    if (!var) return(1);

    if (!cds_delete_var(var)) {
        dsproc_set_status(DSPROC_ECDSDELVAR);
        return(0);
    }

    return(1);
}

/**
 *  Get the boundary variable for a coordinate variable.
 *
 *  @param  coord_var - pointer to the coordinate variable
 *
 *  @return
 *    - pointer to the boundary variable
 *    - NULL if not found
 */
CDSVar *dsproc_get_bounds_var(CDSVar *coord_var)
{
    if (!coord_var) return((CDSVar *)NULL);

    return(cds_get_bounds_var(coord_var));
}

/**
 *  Get the coordinate variable for a variable's dimension.
 *
 *  @param  var       - pointer to the variable
 *  @param  dim_index - index of the dimension
 *
 *  @return
 *    - pointer to the coordinate variable
 *    - NULL if not found
 */
CDSVar *dsproc_get_coord_var(
    CDSVar *var,
    int     dim_index)
{
    if (!var) return((CDSVar *)NULL);

    return(cds_get_coord_var(var, dim_index));
}

/**
 *  Get variables and companion QC variables from a dataset.
 *
 *  If nvars is 0 or var_names is NULL, the output vars array will contain
 *  the pointers to the variables that are not companion QC variables. In
 *  this case the variables in the vars array will be in the same order they
 *  appear in the dataset. The following time and location variables will be
 *  excluded from this array:
 *
 *    - base_time
 *    - time_offset
 *    - time
 *    - time_bounds
 *    - lat
 *    - lon
 *    - alt
 *
 *  If nvars and var_names are specified, the output vars array will contain
 *  an entry for every variable in the var_names list, and will be in the
 *  specified order. Variables that are not found in the dataset will have
 *  a NULL value if the required flag is set to 0. If the required flag is
 *  set to 1 and a variable does not exist, an error will be generated.
 *
 *  If the qc_vars argument is not NULL it will contain the pointers to the
 *  companion qc_ variables. Likewise, if the aqc_vars argument is not NULL
 *  it will contain the pointers to the companion aqc_ variables. If a
 *  companion QC variable does not exist for a variable, the corresponding
 *  entry in the QC array will be NULL.
 *
 *  The memory used by the returned arrays belongs to a 'dsproc_user_...'
 *  CDSVarGroup defined in the dataset and must *NOT* be freed by the calling
 *  process.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  dataset   - pointer to the dataset
 *  @param  var_names - NULL terminated list of variable names
 *  @param  required  - specifies if all variables in the names list are required
 *  @param  vars      - output: pointer to the array of variables
 *  @param  qc_vars   - output: pointer to the array of companion qc_ variables
 *  @param  aqc_vars  - output: pointer to the array of companion aqc_ variables
 *
 *  @return
 *    - length of the output arrays
 *    - -1 if an error occurred
 */
int dsproc_get_dataset_vars(
    CDSGroup     *dataset,
    const char  **var_names,
    int           required,
    CDSVar     ***vars,
    CDSVar     ***qc_vars,
    CDSVar     ***aqc_vars)
{
    CDSVarGroup  *vargroup;
    CDSVarArray  *vararray;
    CDSVar      **var_list;
    CDSVar     ***varsp;
    CDSVar       *var;
    const char   *vararray_name;
    const char   *qc_prefix;
    const char   *namep;
    char          name[256];
    int           user_list;
    int           loop_index;
    int           nvars;
    int           ni;
    int           vi;

    /* Initialize outputs */

    if (vars)     *vars     = (CDSVar **)NULL;
    if (qc_vars)  *qc_vars  = (CDSVar **)NULL;
    if (aqc_vars) *aqc_vars = (CDSVar **)NULL;

    /* Get the 'dsproc_user_arrays...' variable group  */

    if (var_names) {

        user_list = 1;

        /* Get the next unique variable group name */

        for (ni = 1; ; ni++) {
            sprintf(name, "dsproc_user_arrays_%d", ni);
            vargroup = cds_get_vargroup(dataset, name);
            if (!vargroup) {
                vargroup = cds_define_vargroup(dataset, name);
                if (!vargroup) {
                    dsproc_set_status(DSPROC_ENOMEM);
                    return(-1);
                }
                break;
            }
        }

        /* Get the number of variable names in the list */

        for (nvars = 0; var_names[nvars]; nvars++);
    }
    else {

        user_list = 0;

        /* Use the variable group reserved for the complete list */

        vargroup = cds_define_vargroup(dataset, "dsproc_user_arrays_0");
        if (!vargroup) {
            dsproc_set_status(DSPROC_ENOMEM);
            return(-1);
        }

        /* Create the var_names list */

        nvars     = 0;
        var_names = (const char **)malloc(dataset->nvars * sizeof(const char *));

        if (!var_names) {

            ERROR( DSPROC_LIB_NAME,
                "Could not get dataset variables for: %s\n"
                " -> memory allocation error\n",
                cds_get_object_path(dataset));

            cds_delete_vargroup(vargroup);
            dsproc_set_status(DSPROC_ENOMEM);
            return(-1);
        }

        for (vi = 0; vi < dataset->nvars; vi++) {

            var = dataset->vars[vi];

            /* Skip time and location variables */

            if ((strcmp(var->name, "base_time")   == 0) ||
                (strcmp(var->name, "time_offset") == 0) ||
                (strcmp(var->name, "time")        == 0) ||
                (strcmp(var->name, "time_bounds") == 0) ||
                (strcmp(var->name, "qc_time")     == 0) ||
                (strcmp(var->name, "lat")         == 0) ||
                (strcmp(var->name, "lon")         == 0) ||
                (strcmp(var->name, "alt")         == 0)) {

                continue;
            }

            /* Skip companion qc variables */

            namep = var->name;

            if (*namep == 'a') namep++;
            if (*namep == 'q') {
                if (*++namep == 'c' && *++namep == '_') {
                    if (cds_get_var(dataset, ++namep)) {
                        continue;
                    }
                }
            }

            var_names[nvars++] = var->name;
        }
    }

    /* Allocate memory for the variable list */

    var_list = (CDSVar **)malloc(nvars * sizeof(CDSVar *));
    if (!var_list) {

        ERROR( DSPROC_LIB_NAME,
            "Could not get dataset variables for: %s\n"
            " -> memory allocation error\n",
            cds_get_object_path(dataset));

        if (!user_list) free(var_names);

        cds_delete_vargroup(vargroup);
        dsproc_set_status(DSPROC_ENOMEM);
        return(-1);
    }

    /* Create the variable arrays */

    for (loop_index = 0; loop_index < 3; loop_index++) {

        /* Set loop variables */

        switch (loop_index) {
            case 0:
                varsp         = vars;
                qc_prefix     = (const char *)NULL;
                vararray_name = "vars";
                break;
            case 1:
                varsp         = qc_vars;
                qc_prefix     = "qc";
                vararray_name = "qc_vars";
                break;
            case 2:
                varsp         = aqc_vars;
                qc_prefix     = "aqc";
                vararray_name = "aqc_vars";
                break;
            default:
                varsp         = (CDSVar ***)NULL;
                qc_prefix     = (const char *)NULL;
                vararray_name = (const char *)NULL;
        }

        /* Check if this array was requested */

        if (!varsp) {
            continue;
        }

        /* Check if this array has already been created */

        vararray = cds_get_vararray(vargroup, vararray_name);
        if (vararray) {
            *varsp = vararray->vars;
            continue;
        }
        else {
            vararray = cds_define_vararray(vargroup, vararray_name);
            if (!vararray) {
                cds_delete_vargroup(vargroup);
                dsproc_set_status(DSPROC_ENOMEM);
                return(-1);
            }
        }

        /* Create variable list */

        if (qc_prefix) {
            for (vi = 0; vi < nvars; vi++) {
                snprintf(name, 256, "%s_%s", qc_prefix, var_names[vi]);
                var_list[vi] = cds_get_var(dataset, name);
            }
        }
        else {
            for (vi = 0; vi < nvars; vi++) {

                var_list[vi] = cds_get_var(dataset, var_names[vi]);

                if (required && user_list && !var_list[vi]) {

                    ERROR( DSPROC_LIB_NAME,
                        "Could not get dataset variables for: %s\n"
                        " -> required variable not found: %s\n",
                        cds_get_object_path(dataset), var_names[vi]);

                    cds_delete_vargroup(vargroup);
                    dsproc_set_status(DSPROC_EREQVAR);
                    return(-1);
                }
            }
        }

        /* Add variables to the vararray */

        if (!cds_add_vararray_vars(vararray, nvars, var_list)) {
            cds_delete_vargroup(vargroup);
            dsproc_set_status(DSPROC_ENOMEM);
            return(-1);
        }

        *varsp = vararray->vars;
    }

    /* Print the variable group entries if the debug level is greater than 1 */

    if (msngr_debug_level > 1) {
        cds_print_vargroup(stdout, "", vargroup, CDS_SKIP_VARS);
        fprintf(stdout, "\n");
    }

    /* Cleanup and return */

    if (!user_list) free(var_names);
    free(var_list);

    return(nvars);
}

/**
 *  Get a companion metric variable for a variable.
 *
 *  Known metrics at the time of this writing (so there may be others):
 *
 *    - "frac": the fraction of available input values used
 *    - "std":  the standard deviation of the calculated value
 *
 *  @param  var    - pointer to the variable
 *  @param  metric - name of the metric
 *
 *  @return
 *    - pointer to the metric variable
 *    - NULL if not found
 */
CDSVar *dsproc_get_metric_var(CDSVar *var, const char *metric)
{
    char metric_var_name[NC_MAX_NAME];

    if (!var) return((CDSVar *)NULL);

    snprintf(metric_var_name, NC_MAX_NAME, "%s_%s", var->name, metric);

    return(cds_get_var((CDSGroup *)var->parent, metric_var_name));
}

/**
 *  Get a variable from an output dataset.
 *
 *  The obs_index should always be zero unless observation based processing is
 *  being used. This is because all input observations should have been merged
 *  into a single observation in the output datasets.
 *
 *  @param  ds_id     - output datastream ID
 *  @param  var_name  - variable name
 *  @param  obs_index - the index of the obervation to get the dataset for
 *
 *  @return
 *    - pointer to the output variable
 *    - NULL if it does not exist
 */
CDSVar *dsproc_get_output_var(
    int         ds_id,
    const char *var_name,
    int         obs_index)
{
    CDSGroup *dataset = dsproc_get_output_dataset(ds_id, obs_index);

    if (!dataset) {
        return((CDSVar *)NULL);
    }

    return(cds_get_var(dataset, var_name));
}

/**
 *  Get the companion QC variable for a variable.
 *
 *  @param  var - pointer to the variable
 *
 *  @return
 *    - pointer to the QC variable
 *    - NULL if not found
 */
CDSVar *dsproc_get_qc_var(CDSVar *var)
{
    char qc_var_name[NC_MAX_NAME];

    if (!var) return((CDSVar *)NULL);

    snprintf(qc_var_name, NC_MAX_NAME, "qc_%s", var->name);

    return(cds_get_var((CDSGroup *)var->parent, qc_var_name));
}

/**
 *  Get a primary variable from the retrieved data.
 *
 *  This function will find a variable in the retrieved data that was
 *  explicitly requested by the user in the retriever definition.
 *
 *  The obs_index is used to specify which observation to pull the variable
 *  from.  This value will typically be zero unless this function is called
 *  from a post_retrieval_hook() function, or the process is using observation
 *  based processing.  In either of these cases the retrieved data will
 *  contain one "observation" for every file the data was read from on disk.
 *
 *  It is also possible to have multiple observations in the retrieved data
 *  when a pre_transform_hook() is called if a dimensionality conflict
 *  prevented all observations from being merged.
 *
 *  @param var_name  - variable name
 *  @param obs_index - the index of the obervation to get the variable from
 *
 *  @return
 *    - pointer to the retrieved variable
 *    - NULL if not found
 */
CDSVar *dsproc_get_retrieved_var(
    const char *var_name,
    int         obs_index)
{
    CDSGroup *ret_data = _DSProc->ret_data;
    CDSGroup *ds_group;
    CDSVar   *ret_var;
    int       dsi;

    if (!ret_data || (obs_index < 0)) {
        return((CDSVar *)NULL);
    }

    for (dsi = 0; dsi < ret_data->ngroups; dsi++) {

        ds_group = ret_data->groups[dsi];

        if (obs_index >= ds_group->ngroups) {
            continue;
        }

        ret_var = cds_get_var(ds_group->groups[obs_index], var_name);

        if (ret_var &&
            cds_get_user_data(ret_var, "DSProcVarTag")) {

            return(ret_var);
        }
    }

    return((CDSVar *)NULL);
}

/**
 *  Get a primary variable from the transformed data.
 *
 *  This function will find a variable in the transformed data that was
 *  explicitly requested by the user in the retriever definition.
 *
 *  The obs_index is used to specify which observation to pull the variable
 *  from. This value will typically be zero unless the process is using
 *  observation based processing. If this is the case the transformed data will
 *  contain one "observation" for every file the data was read from on disk.
 *
 *  @param var_name  - variable name
 *  @param obs_index - the index of the obervation to get the variable from
 *
 *  @return
 *    - pointer to the transformed variable
 *    - NULL if not found
 */
CDSVar *dsproc_get_transformed_var(
    const char *var_name,
    int         obs_index)
{
    CDSGroup *trans_data = _DSProc->trans_data;
    CDSGroup *cs_group;
    CDSGroup *ds_group;
    CDSVar   *trans_var;
    int       csi, dsi;

    obs_index = 0;

    if (!trans_data || (obs_index < 0)) {
        return((CDSVar *)NULL);
    }

    for (csi = 0; csi < trans_data->ngroups; csi++) {

        cs_group = trans_data->groups[csi];

        for (dsi = 0; dsi < cs_group->ngroups; dsi++) {

            ds_group = cs_group->groups[dsi];

/* BDE OBS UPDATE */
            trans_var = cds_get_var(ds_group, var_name);
/*
            if (obs_index >= ds_group->ngroups) {
                continue;
            }

            trans_var = cds_get_var(ds_group->groups[obs_index], var_name);
*/
/* END BDE OBS UPDATE */

            if (trans_var &&
                cds_get_user_data(trans_var, "DSProcVarTag")) {

                return(trans_var);
            }
        }
    }

    return((CDSVar *)NULL);
}

/**
 *  Get a variable from a transformation coordinate system.
 *
 *  Unlike the dsproc_get_transformed_var() function, this function will find
 *  any variable in the specified transformation coordinate system.
 *
 *  The obs_index is used to specify which observation to pull the variable
 *  from. This value will typically be zero unless the process is using
 *  observation based processing. If this is the case the transformed data will
 *  contain one "observation" for every file the data was read from on disk.
 *
 *  @param coordsys_name - coordinate system name
 *  @param var_name      - variable name
 *  @param obs_index     - the index of the obervation to get the variable from
 *
 *  @return
 *    - pointer to the transformed variable
 *    - NULL if not found
 */
CDSVar *dsproc_get_trans_coordsys_var(
    const char *coordsys_name,
    const char *var_name,
    int         obs_index)
{
    CDSGroup *trans_data = _DSProc->trans_data;
    CDSGroup *cs_group;
    CDSGroup *ds_group;
    CDSVar   *trans_var;
    int       csi, dsi;

    obs_index = 0;

    if (!trans_data || (obs_index < 0)) {
        return((CDSVar *)NULL);
    }

    for (csi = 0; csi < trans_data->ngroups; csi++) {

        cs_group = trans_data->groups[csi];

        if (strcmp(cs_group->name, coordsys_name) != 0) {
            continue;
        }

        for (dsi = 0; dsi < cs_group->ngroups; dsi++) {

            ds_group = cs_group->groups[dsi];

/* BDE OBS UPDATE */
            trans_var = cds_get_var(ds_group, var_name);
/*
            if (obs_index >= ds_group->ngroups) {
                continue;
            }

            trans_var = cds_get_var(ds_group->groups[obs_index], var_name);
*/
/* END BDE OBS UPDATE */

            if (trans_var) {
                return(trans_var);
            }
        }

        trans_var = cds_get_var(cs_group, var_name);
        if (trans_var) {
            return(trans_var);
        }

        break;
    }

    return((CDSVar *)NULL);
}

/**
 *  Get a variable from a dataset.
 *
 *  @param  dataset - pointer to the dataset
 *  @param  name    - name of the variable
 *
 *  @return
 *    - pointer to the variable
 *    - NULL if the variable does not exist
 */
CDSVar *dsproc_get_var(
    CDSGroup   *dataset,
    const char *name)
{
    if (!dataset) return((CDSVar *)NULL);

    return(cds_get_var(dataset, name));
}

/**
 *  Get a data index for a multi-dimensional variable.
 *
 *  This function will return a data index that can be used to access
 *  the data in a variable using the traditional x[i][j][k] syntax.
 *  It is up to the calling process to cast the return value of this
 *  function into the proper data type.
 *
 *  Note: If the variable has less than 2 dimensions, the pointer to
 *  the variable’s data array will be returned.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  var - pointer to the variable
 *
 *  @return
 *    - the data index into the variable’s data array
 *    - NULL if:
 *        - the pointer to the variable was NULL
 *        - no data has been stored in the variable (var->sample_count == 0)
 *        - a memory allocation error occurs
 */
void *dsproc_get_var_data_index(CDSVar *var)
{
    void *datap;

    if (!var) return((void *)NULL);

    if (var->sample_count == 0) {
        return((void *)NULL);
    }

    datap = cds_create_var_data_index(var);

    if (!datap) {
        dsproc_set_status(DSPROC_ENOMEM);
    }

    return(datap);
}

/**
 *  Get a copy of the data from a dataset variable.
 *
 *  This function will get the data from a variable casted into the specified
 *  data type. All missing values used in the data will be converted to a single
 *  missing value appropriate for the requested data type. The missing value
 *  used will be the first value returned by cds_get_var_missing_values() if
 *  that value is within the range of the requested data type, otherwise, the
 *  default fill value for the requested data type will be used.
 *
 *  Memory will be allocated for the returned data array if the output array
 *  is NULL. In this case the calling process is responsible for freeing
 *  the allocated memory. If an output data array is specified it must be
 *  large enough to hold (sample_count * cds_var_sample_size(var)) elements.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  var           - pointer to the variable
 *  @param  type          - data type of the output missing_value and data array
 *  @param  sample_start  - start sample (0 based indexing)
 *  @param  sample_count  - pointer to the sample_count
 *                            - input:
 *                                - length of the output array
 *                                - ignored if the output array is NULL
 *                            - output:
 *                                - number of samples returned
 *                                - 0 if no data for sample_start
 *                                - (size_t)-1 if a memory allocation error occurs
 *  @param  missing_value - output: missing value
 *  @param  data          - pointer to the output data array
 *                          or NULL to dynamically allocate the memory needed.
 *
 *  @return
 *    - pointer to the output data array
 *    - NULL if:
 *        - the pointer to the variable was NULL
 *        - the variable has no data for sample_start (sample_count == 0)
 *        - a memory allocation error occurs (sample_count == (size_t)-1)
 */
void *dsproc_get_var_data(
    CDSVar       *var,
    CDSDataType   type,
    size_t        sample_start,
    size_t       *sample_count,
    void         *missing_value,
    void         *data)
{
    size_t tmp_sample_count = 0;

    if (!var) return((void *)NULL);

    if (!sample_count) {
        sample_count = &tmp_sample_count;
    }

    data = cds_get_var_data(
        var, type, sample_start, sample_count, missing_value, data);

    if (*sample_count == (size_t)-1) {
        dsproc_set_status(DSPROC_ENOMEM);
    }

    return(data);
}

/**
 *  Get the missing values for a CDS Variable.
 *
 *  This function will return an array containing all values specified by
 *  the missing_value and _FillValue attributes (in that order), and will
 *  be the same data type as the variable. If the _FillValue attribute does
 *  not exist but a default fill value has been defined, it will be used
 *  instead.
 *
 *  The memory used by the output array of missing values is dynamically
 *  allocated. It is the responsibility of the calling process to free
 *  this memory when it is no longer needed.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  var    - pointer to the variable
 *  @param  values - output: pointer to the array of missing values, the
 *                   data type of this array will be the same as the variable
 *
 *  @return
 *    -  number of missing values
 *    -  0 if there are no missing or fill values
 *    - -1 if a memory allocation error occurs
 */
int dsproc_get_var_missing_values(CDSVar *var, void **values)
{
    int status;

    if (!var) return(0);

    status = cds_get_var_missing_values(var, values);

    if (status < 0) {
        dsproc_set_status(DSPROC_ENOMEM);
    }

    return(status);
}

/**
 *  Initialize the data values for a dataset variable.
 *
 *  This function will make sure enough memory is allocated for the specified
 *  samples and initializing the data values to either the variable's missing
 *  value (use_missing == 1), or 0 (use_missing == 0).
 *
 *  The search order for missing values is:
 *
 *    - missing_value attribute
 *    - _FillValue attribute
 *    - variable's default missing value
 *
 *  If the variable does not have any missing or fill values defined the
 *  default fill value for the variable's data type will be used and the
 *  default fill value for the variable will be set.
 *
 *  If the specified start sample is greater than the variable's current sample
 *  count, the hole between the two will be filled with the first missing or
 *  fill value defined for the variable.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  var           - pointer to the variable
 *  @param  sample_start  - start sample of the data to initialize
 *                          (0 based indexing)
 *  @param  sample_count  - number of samples to initialize
 *  @param  use_missing   - flag indicating if the variables missing value
 *                          should be used (1 == TRUE, 0 == fill with zeros)
 *
 *  @return
 *    - pointer to the specifed start sample in the variable's data array
 *    - NULL if:
 *        - the specified sample count is zero
 *        - one of the variable's static dimensions has 0 length
 *        - the variable has no dimensions, and sample_start is not equal
 *          to 0 or sample_count is not equal to 1.
 *        - the first variable dimension is not unlimited, and
 *          sample_start + sample_count would exceed the dimension length.
 *        - a memory allocation error occurred
 */
void *dsproc_init_var_data(
    CDSVar *var,
    size_t  sample_start,
    size_t  sample_count,
    int     use_missing)
{
    void *datap;

    datap = cds_init_var_data(var, sample_start, sample_count, use_missing);

    if (!datap) {
        dsproc_set_status(DSPROC_ECDSALLOCVAR);
    }

    return(datap);
}

/**
 *  Initialize the data values for a dataset variable.
 *
 *  This function behaves the same as dsproc_init_var_data() except that it
 *  returns a data index starting at the specified start sample, see
 *  dsproc_get_var_data_index() for details. For variables that have less than
 *  two dimensions this function is identical to dsproc_init_var_data().
 *
 *  The data index returned by this function belongs to the CDS variable
 *  and will be freed when the variable is destroyed. The calling process
 *  must *not* attempt to free this memory.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  var           - pointer to the variable
 *  @param  sample_start  - start sample of the data to initialize
 *                          (0 based indexing)
 *  @param  sample_count  - number of samples to initialize
 *  @param  use_missing   - flag indicating if the variables missing value
 *                          should be used (1 == TRUE, 0 == fill with zeros)
 *
 *  @return
 *    - the data index into the variables data array starting at the
 *      specified start sample.
 *    - NULL if:
 *        - the specified sample count is zero
 *        - one of the variable's static dimensions has 0 length
 *        - the variable has no dimensions, and sample_start is not equal
 *          to 0 or sample_count is not equal to 1.
 *        - the first variable dimension is not unlimited, and
 *          sample_start + sample_count would exceed the dimension length.
 *        - a memory allocation error occurred
 */
void *dsproc_init_var_data_index(
    CDSVar *var,
    size_t  sample_start,
    size_t  sample_count,
    int     use_missing)
{
    void *datap;

    datap = cds_init_var_data_index(
        var, sample_start, sample_count, use_missing);

    if (!datap) {
        dsproc_set_status(DSPROC_ECDSALLOCVAR);
    }

    return(datap);
}

/**
 *  Set cell boundary data for all coordinate variables in a dataset.
 *
 *  This function will call dsproc_set_bounds_var_data() for all variables
 *  in the specified dataset that have a bounds attribute defined.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  dataset      - pointer to the CDS variable
 *  @param  sample_start - start sample along the unlimited dimension
 *                         (0 based indexing)
 *  @param  sample_count - number of samples along the unlimited dimension
 *                         (0 for all available samples)
 *
 *  @return
 *    -  1 if successful
 *    -  0 if an error occurred.
 */
int dsproc_set_bounds_data(
    CDSGroup *dataset,
    size_t    sample_start,
    size_t    sample_count)
{
    CDSVar *var;
    size_t  start;
    size_t  count;
    int     status;
    int     vi;

    for (vi = 0; vi < dataset->nvars; ++vi) {

        var = dataset->vars[vi];

        if (!cds_get_att(var, "bounds") ||
            var->ndims        == 0      ||
            var->sample_count == 0) {

            continue;
        }

        if (var->dims[0]->is_unlimited) {
           start = sample_start;
           count = sample_count;
        }
        else {
           start = 0;
           count = var->sample_count;
        }

        status = dsproc_set_bounds_var_data(var, start, count);
        if (status < 0) return(0);
    }

    return(1);
}

/**
 *  Set cell boundary data values for a dataset variable.
 *
 *  This function can only be used to create the cell boundary data values
 *  for regular grids and requires:
 *
 *    - The variable has a bounds attribute defined that specifies the
 *      name of the boundary variable.
 *
 *    - The boundary variable has a bound_offsets attribute defined that
 *      specifies the cell boundary offsets from the data values.
 *
 *    - The number of boundary variable dimensions is one greater than
 *      than the number of variable dimensions.
 *
 *    - The boundary variable and bound_offsets attribute have the same
 *      data type as the variable.
 *
 *    - The bound_offsets attribute has the same length as the last
 *      dimension of the boundary variable.
 *
 *  @param  coord_var     - pointer to the coordinate variable
 *  @param  sample_start  - start sample (0 based indexing)
 *  @param  sample_count  - number of samples (0 for all available samples)
 *
 *  @return
 *    -  1 if successful
 *    -  0 if the bounds and/or bound_offsets attributes are not defined, or
 *       the variable does not have any data for the specified sample_start.
 *    - -1 if an error occurred.
 */
int dsproc_set_bounds_var_data(
    CDSVar *coord_var,
    size_t  sample_start,
    size_t  sample_count)
{
    int status = cds_set_bounds_var_data(coord_var, sample_start, sample_count);

    if (status < 0) {
        dsproc_set_status(DSPROC_EBOUNDSVAR);
    }

    return(status);
}

/**
 *  Set the data values for a dataset variable.
 *
 *  This function will set the data values of a variable by casting the values
 *  in the input data array into the data type of the variable. All missing
 *  values in the input data array will be converted to the first missing value
 *  used by the variable as returned by cds_get_var_missing_values(). If the
 *  variable does not have a missing_value or _FillValue attribute defined, the
 *  default fill value for the variable's data type will be used.
 *
 *  For multi-dimensional variables, the specified data array must be stored
 *  linearly in memory with the last dimension varying the fastest.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  var           - pointer to the variable
 *  @param  type          - data type of the input missing_value and data array
 *  @param  sample_start  - start sample of the new data (0 based indexing)
 *  @param  sample_count  - number of new samples
 *  @param  missing_value - pointer to the missing value used in the data array,
 *                          or NULL if the data does not contain any missing values
 *  @param  data          - pointer to the input data array
 *
 *  @return
 *    - pointer to the specifed start sample in the variable's data array
 *    - NULL if:
 *        - one of the variable's static dimensions has 0 length
 *        - the variable has no dimensions, and sample_start is not equal
 *          to 0 or sample_count is not equal to 1.
 *        - the first variable dimension is not unlimited, and
 *          sample_start + sample_count would exceed the dimension length.
 *        - a memory allocation error occurred
 */
void *dsproc_set_var_data(
    CDSVar       *var,
    CDSDataType   type,
    size_t        sample_start,
    size_t        sample_count,
    void         *missing_value,
    void         *data)
{
    data = cds_set_var_data(
        var, type, sample_start, sample_count, missing_value, data);

    if (!data) {
        dsproc_set_status(DSPROC_ECDSSETDATA);
    }

    return(data);
}

/**
 *  Check if a variable has an unlimited dimension.
 *
 *  @param  var - pointer to the variable
 *
 *  @return
 *    - 1 if the variable has an unlimited dimension
 *    - 0 if this variable does not have an unlimited dimension
 */
int dsproc_var_is_unlimited(CDSVar *var)
{
    if (var->ndims) {
        return(var->dims[0]->is_unlimited);
    }

    return(0);
}

/**
 *  Returns the variable name.
 *
 *  The returned name belongs to the variable structure and must not be freed
 *  or altered by the calling process.
 *
 *  @param  var - pointer to the variable
 *
 *  @return
 *    - pointer to the variable name
 *    - NULL if the specified variable is NULL
 */
const char *dsproc_var_name(CDSVar *var)
{
    if (!var) {
        return((const char *)NULL);
    }

    return(var->name);
}

/**
 *  Returns the number of samples in a variable's data array.
 *
 *  The sample_count for a variable is the number of samples stored
 *  along the variable's first dimension.
 *
 *  @param  var - pointer to the variable
 *
 *  @return  number of samples in the variable's data array
 */
size_t dsproc_var_sample_count(CDSVar *var)
{
    if (!var) {
        return(0);
    }

    return(var->sample_count);
}

/**
 *  Returns the sample size of a variable.
 *
 *  Variables with less than 2 dimensions will always have a sample_size of 1.
 *  The sample_size for variables with 2 or more dimensions is the product of
 *  all the dimension lengths starting with the 2nd dimension.
 *
 *  @param  var - pointer to the variable
 *
 *  @return  sample size of the variable
 */
size_t dsproc_var_sample_size(CDSVar *var)
{
    if (!var) {
        return(0);
    }

    return(cds_var_sample_size(var));
}
