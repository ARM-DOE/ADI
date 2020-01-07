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
*    $Revision: 60386 $
*    $Author: ermold $
*    $Date: 2015-02-19 20:43:41 +0000 (Thu, 19 Feb 2015) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file cds_var_data.c
 *  CDS Variable Data.
 */

#include "cds3.h"
#include "cds_private.h"

/*******************************************************************************
 *  Private Functions
 */
/** @privatesection */

const char *_MissingValueAttNames[] = {
    "missing_value",
    "missing-value",
    "missing_data",
    "missing-data",
    "missing_value1",
    "Missing_value",
    "_FillValue"
};

static int _NumMissingValueAttNames = sizeof(_MissingValueAttNames)/sizeof(const char *);

/**
 *  PRIVATE: Create a data index for multi-dimensional variable data.
 *
 *  This function will return a data index that can be used to access
 *  variable data using the traditional x[i][j][k] syntax. It is up
 *  to the calling process to cast the return value of this function
 *  into the proper type.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  var          - pointer to the variable
 *  @param  sample_start - start sample (0 based indexing)
 *
 *  @return
 *    - pointer to the data index
 *    - NULL if:
 *        - no data has been stored in the variable
 *        - sample_start is greater than var->sample_count
 *        - a memory allocation error occurred
 */
void *_cds_create_var_data_index(
    CDSVar *var,
    size_t  sample_start)
{
    size_t  type_size   = cds_data_type_size(var->type);
    size_t  sample_size = cds_var_sample_size(var);
    size_t *lengths;
    void   *data_index;
    void   *datap;
    int     di;

    /* Check if data has been stored in this variable */

    if (!var->sample_count || !var->data.vp) {
        return((void *)NULL);
    }

    /* Make sure sample start is greater than sample_count */

    if (sample_start > var->sample_count) {
        return((void *)NULL);
    }

    datap = var->data.bp + (sample_start * sample_size * type_size);

    /* Check if this variable has less than 2 dimensions */

    if (var->ndims < 2) {
        return(datap);
    }

    /* Create the dimension lengths array */

    lengths = (size_t *)malloc(var->ndims * sizeof(size_t));

    if (!lengths) {

        ERROR( CDS_LIB_NAME,
            "Could not create data index for: %s\n"
            " -> memory allocation error\n", cds_get_object_path(var));

        free(lengths);
        return((void *)NULL);
    }

    lengths[0] = var->sample_count - sample_start;

    for (di = 1; di < var->ndims; di++) {
        lengths[di] = var->dims[di]->length;
    }

    /* Create the variable data index */

    data_index = cds_create_data_index(
        datap, var->type, var->ndims, lengths);

    if (!data_index) {

        ERROR( CDS_LIB_NAME,
            "Could not create data index for: %s\n"
            " -> memory allocation error\n", cds_get_object_path(var));

        free(lengths);
        return((void *)NULL);
    }

    /* Add or replace the data index in the variable structure */

    _cds_delete_var_data_index(var);

    var->data_index         = data_index;
    var->data_index_ndims   = var->ndims;
    var->data_index_lengths = lengths;

    return(data_index);
}

/**
 *  PRIVATE: Delete the data index for multi-dimensional variable data.
 *
 *  @param  var - pointer to the variable
 */
void _cds_delete_var_data_index(CDSVar *var)
{
    if (var) {

        if (var->data_index &&
            var->data_index != var->data.vp) {

            cds_free_data_index(var->data_index,
                var->data_index_ndims, var->data_index_lengths);

            free(var->data_index_lengths);
        }

        var->data_index         = (void *)NULL;
        var->data_index_ndims   = 0;
        var->data_index_lengths = (size_t *)NULL;
    }
}

/**
 *  Get the first missing value defined for a CDS Variable.
 *
 *  @param  var   - pointer to the variable
 *  @param  value - output: the missing value, this must be large enough to
 *                  store the output value (see CDS_MAX_TYPE_SIZE).
 *
 *  @return
 *    - 1 if a missing or fill value was found
 *    - 0 if the variable does not have any missing or fill values defined.
 */
int _cds_get_first_missing_value(CDSVar *var, void *value)
{
    CDSGroup *parent;
    CDSAtt   *att;
    size_t    type_size;
    size_t    length;
    int       found;
    int       mi;

    found = 0;

    /* Check for a missing value attribute at the field level */

    for (mi = 0; mi < _NumMissingValueAttNames; ++mi) {
        att = cds_get_att(var, _MissingValueAttNames[mi]);
        if (att && att->length && att->value.vp) {
            found = 1;
            break;
        }
    }

    /* If a missing value attribute was not found, search again
     * at the global attribute level. */

    parent = (CDSGroup *)var->parent;

    while (!found && parent) {

        for (mi = 0; mi < _NumMissingValueAttNames; ++mi) {

            att = cds_get_att(parent, _MissingValueAttNames[mi]);
            if (att && att->length && att->value.vp) {
                found = 1;
                break;
            }
        }

        parent = (CDSGroup *)parent->parent;
    }

    /* If a missing value attribute was still not found,
     * check for a default fill value */

    if (!found) {

        if (var->default_fill) {
            type_size = cds_data_type_size(var->type);
            memcpy(value, var->default_fill, type_size);
            return(1);
        }
        else {
            return(0);
        }
    }

    length = 1;
    cds_get_att_value(att, var->type, &length, value);

    return(1);
}

/**
 *  Set cell boundary data values for a CDS coordinate variable.
 *
 *  This function can only be used to create the cell boundary data values
 *  for regular grids and requires:
 *
 *    - The boundary variable has the same dimensionality as the data variable
 *      plus the bounds dimension.
 *
 *    - The boundary variable and bound_offsets array have the same data
 *      type as the data variable.
 *
 *    - The length of the bound_offsets array has the same length of the
 *      boundary variable's bounds dimension.
 *
 *  @param  coord_var     - pointer to the coordinate variable
 *  @param  sample_start  - start sample (0 based indexing)
 *  @param  sample_count  - number of samples (0 for all available samples)
 *  @param  bound_offsets - array of cell boundary offsets from data values.
 *  @param  bounds_var    - pointer to the boundary variable
 *
 *  @return
 *    -  1 if successful
 *    -  0 if the data variable does not have any data defined
 *         for the specified sample_start.
 *    - -1 if an error occurred.
 */
int _cds_set_bounds_var_data(
    CDSVar *coord_var,
    size_t  sample_start,
    size_t  sample_count,
    void   *bound_offsets,
    CDSVar *bounds_var)
{
    const char *coord_units;
    const char *bounds_units;
    CDSDim     *bounds_dim;
    CDSData     data;
    CDSData     bounds;
    CDSData     offsets;
    size_t      sample_size;
    size_t      type_size;
    int         ndims;
    int         noffsets;
    int         nelems;
    int         di;

    /* Make sure the data and boundary variables
     * have the same data type */

    if (coord_var->type != bounds_var->type) {

        ERROR( CDS_LIB_NAME,
            "Invalid data type for boundary variable: %s\n"
            " -> a boundary variable must have the same data type\n"
            " -> as its associated coordinate variable\n",
            cds_get_object_path(bounds_var));

        return(-1);
    }

    /* Make sure the data and boundary variables
     * have compatible dimensionality */

    if (bounds_var->ndims != coord_var->ndims + 1) {

        ERROR( CDS_LIB_NAME,
            "Invalid dimensionality for boundary variable: %s\n"
            " -> a boundary variable must have the same dimensions as its\n"
            " -> associated coordinate variable plus the bounds dimension\n",
            cds_get_object_path(bounds_var));

        return(-1);
    }

    ndims = coord_var->ndims;

    for (di = 0; di < ndims; ++di) {

        if (coord_var->dims[di] != bounds_var->dims[di]) {

            ERROR( CDS_LIB_NAME,
                "Invalid dimensionality for boundary variable: %s\n"
                " -> a boundary variable must have the same dimensions as its\n"
                " -> associated coordinate variable plus the bounds dimension\n",
                cds_get_object_path(bounds_var));

            return(-1);
        }
    }

    /* Make sure the coordinate and boundary variables
     * have the same units. */

    if ((bounds_units = cds_get_var_units(bounds_var)) &&
        (coord_units  = cds_get_var_units(coord_var))) {

        if (cds_compare_units(coord_units, bounds_units) != 0) {

            ERROR( CDS_LIB_NAME,
                "Invalid units for boundary variable: %s\n"
                " -> a boundary variable must have the same units\n"
                " -> as its associated coordinate variable\n",
                cds_get_object_path(bounds_var));
        }
    }

    /* Check if the variable has any data for the requested sample_start */

    if (!coord_var->data.vp ||
        coord_var->sample_count <= sample_start) {

        return(0);
    }

    /* Adjust sample_count if necessary */

    if (!sample_count ||
        sample_count > coord_var->sample_count - sample_start) {
        sample_count = coord_var->sample_count - sample_start;
    }

    /* Allocate memory for the bounds variable */

    bounds.vp = cds_alloc_var_data(bounds_var, sample_start, sample_count);
    if (!bounds.vp) return(-1);

    /* Set cell boundary values */

    type_size   = cds_data_type_size(coord_var->type);
    sample_size = cds_var_sample_size(coord_var);
    nelems      = sample_count * sample_size;
    data.bp     = coord_var->data.bp + (sample_start * sample_size * type_size);

    bounds_dim  = bounds_var->dims[ndims];
    noffsets    = bounds_dim->length;
    offsets.vp  = bound_offsets;

    switch (coord_var->type) {

        case CDS_DOUBLE: CDS_SET_BOUNDS_DATA(nelems, data.dp, noffsets, offsets.dp, bounds.dp); break;
        case CDS_FLOAT:  CDS_SET_BOUNDS_DATA(nelems, data.fp, noffsets, offsets.fp, bounds.fp); break;
        case CDS_INT:    CDS_SET_BOUNDS_DATA(nelems, data.ip, noffsets, offsets.ip, bounds.ip); break;
        case CDS_SHORT:  CDS_SET_BOUNDS_DATA(nelems, data.sp, noffsets, offsets.sp, bounds.sp); break;
        case CDS_BYTE:   CDS_SET_BOUNDS_DATA(nelems, data.bp, noffsets, offsets.bp, bounds.bp); break;
        case CDS_CHAR:

            ERROR( CDS_LIB_NAME,
                "Invalid data type 'CDS_CHAR' for boundary variable: %s\n"
                " -> boundary variables can only be used for numeric data types\n",
                cds_get_object_path(bounds_var));

            return(-1);

        default:

            ERROR( CDS_LIB_NAME,
                "Unknown data type '%d' for boundary variable: %s\n",
                coord_var->type, cds_get_object_path(bounds_var));

            return(-1);
    }

    return(1);
}

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Allocate memory for a CDS variable's data array.
 *
 *  This function will allocate memory as necessary to ensure that the
 *  variable's data array is large enough to store another sample_count
 *  samples starting from sample_start. The calling process must cast the
 *  returned data pointer to the proper data type if it is going to be used
 *  directly. The cds_set_var_data() function can be used if the variable's
 *  data type is not known at compile time.
 *
 *  The data array returned by this function belongs to the CDS variable
 *  and will be freed when the variable is destroyed. The calling process
 *  must *not* attempt to free this memory.
 *
 *  The memory allocated by this function will *not* be initialized. It
 *  is the responsibility of the calling process to set the data values.
 *  If necessary, the cds_init_array() function can be used to initialize
 *  this memory. However, if the specified start sample is greater than the
 *  variable's current sample count, the hole between the two will be filled
 *  with the first missing value defined for the variable. The search order
 *  for missing value attributes is:
 *
 *       - missing_value
 *       - missing-value  depricated
 *       - missing_data   depricated
 *       - missing-data   depricated
 *       - missing_value1 depricated
 *       - Missing_value  depricated
 *       - _FillValue
 *       - variable's default missing value
 *
 *  <b>Note that the deprecated attributes should not be used by new code.</b>
 *
 *  If none of these attributes are found for the variable the parent group
 *  will be searched.
 *
 *  If the variable does not have any missing or fill values defined the
 *  default fill value for the variable's data type will be used and the
 *  default fill value for the variable will be set.
 *
 *  This function will also update the length of the variable's first
 *  dimension if it is unlimited and its length is less than sample_start
 *  plus sample_count.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  var          - pointer to the variable
 *  @param  sample_start - start sample (0 based indexing)
 *  @param  sample_count - number of new samples
 *
 *  @return
 *    - pointer to the specifed start sample in the variable data array
 *    - NULL if:
 *        - the specified sample count is zero
 *        - one of the variable's static dimensions has 0 length
 *        - the variable has no dimensions, and sample_start is not equal
 *          to 0 or sample_count is not equal to 1.
 *        - the first variable dimension is not unlimited, and
 *          sample_start + sample_count would exceed the dimension length.
 *        - a memory allocation error occurred
 */
void *cds_alloc_var_data(
    CDSVar *var,
    size_t  sample_start,
    size_t  sample_count)
{
    size_t  type_size   = cds_data_type_size(var->type);
    size_t  sample_size = cds_var_sample_size(var);
    size_t  total_count;
    size_t  realloc_count;
    size_t  realloc_size;
    size_t  length;
    int     update_unlimdim;
    char    missing[CDS_MAX_TYPE_SIZE];
    void   *datap;

    if (!sample_count) {

        ERROR( CDS_LIB_NAME,
            "Could not allocate memory for variable data: %s\n"
            " -> specified sample count is zero\n",
            cds_get_object_path(var));

        return((void *)NULL);
    }

    /* Make sure sample_size is not equal to 0 */

    if (!sample_size) {

        ERROR( CDS_LIB_NAME,
            "Could not allocate memory for variable data: %s\n"
            " -> static dimension has 0 length\n",
            cds_get_object_path(var));

        return((void *)NULL);
    }

    /* Check sample_start and sample_count values */

    total_count     = sample_start + sample_count;
    update_unlimdim = 0;

    if (var->ndims == 0) {

        if (sample_start != 0) {

            ERROR( CDS_LIB_NAME,
                "Could not allocate memory for variable data: %s\n"
                " -> invalid start sample: %d\n",
                cds_get_object_path(var), sample_start);

            return((void *)NULL);
        }

        if (sample_count != 1) {

            ERROR( CDS_LIB_NAME,
                "Could not allocate memory for variable data: %s\n"
                " -> invalid sample count: %d\n",
                cds_get_object_path(var), sample_count);

            return((void *)NULL);
        }

        realloc_count = 1;
    }
    else if (var->dims[0]->is_unlimited) {

        if (total_count > var->alloc_count) {

            if (var->sample_count) {
                realloc_count = 2 * var->sample_count;
                while (realloc_count < total_count) {
                    realloc_count *= 2;
                }
            }
            else {
                realloc_count = total_count;
            }
        }
        else {
            realloc_count = 0;
        }

        update_unlimdim = 1;
    }
    else {

        if (total_count > var->dims[0]->length) {

            ERROR( CDS_LIB_NAME,
                "Could not allocate memory for variable data: %s\n"
                " -> start sample (%d) + sample count (%d) > dimension length (%d)\n",
                cds_get_object_path(var),
                sample_start, sample_count, var->dims[0]->length);

            return((void *)NULL);
        }

        realloc_count = var->dims[0]->length;
    }

    /* Allocate memory for the variable data */

    if (realloc_count > var->alloc_count) {

        realloc_size = realloc_count * sample_size * type_size;

        datap = realloc(var->data.vp, realloc_size);
        if (!datap) {

            ERROR( CDS_LIB_NAME,
                "Could not allocate memory for variable data: %s\n"
                " -> memory allocation error\n",
                cds_get_object_path(var));

            return((void *)NULL);
        }

        var->data.vp     = datap;
        var->alloc_count = realloc_count;
    }

    /* Check if the specified sample_start is greater than var->sample_count,
     * if it is we need to initialize the hole with missing or fill values. */

    if (sample_start > var->sample_count) {

        if (!_cds_get_first_missing_value(var, (void *)missing)) {

            cds_get_default_fill_value(var->type, (void *)missing);

            if (!cds_set_var_default_fill_value(var, (void *)missing)) {

                ERROR( CDS_LIB_NAME,
                    "Could not allocate memory for variable data: %s\n"
                    " -> memory allocation error\n",
                    cds_get_object_path(var));

                return((void *)NULL);
            }
        }

        length = (sample_start - var->sample_count) * sample_size;
        datap  = var->data.bp + (var->sample_count * sample_size * type_size);

        cds_init_array(var->type, length, (void *)missing, datap);
    }

    /* Update the variable's sample count */

    if (total_count > var->sample_count) {
        var->sample_count = total_count;
    }

    datap = var->data.bp + (sample_start * sample_size * type_size);

    /* Update the length of the unlimited dimension */

    if (update_unlimdim) {

        if (total_count > var->dims[0]->length) {
            var->dims[0]->length = total_count;
        }
    }

    return(datap);
}

/**
 *  Allocate memory for a CDS variable's data array.
 *
 *  This function behaves the same as cds_alloc_var_data() except that it
 *  returns a data index starting at the specified start sample, see
 *  cds_create_data_index() for details. For variables that have less than
 *  two dimensions this function is identical to cds_alloc_var_data().
 *
 *  The data index returned by this function belongs to the CDS variable
 *  and will be freed when the variable is destroyed. The calling process
 *  must *not* attempt to free this memory.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  var          - pointer to the variable
 *  @param  sample_start - start sample (0 based indexing)
 *  @param  sample_count - number of new samples
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
void *cds_alloc_var_data_index(
    CDSVar *var,
    size_t  sample_start,
    size_t  sample_count)
{
    void *datap = cds_alloc_var_data(var, sample_start, sample_count);

    if (!datap) {
        return((void *)NULL);
    }
    else if (var->ndims < 2) {
        return(datap);
    }

    return(_cds_create_var_data_index(var, sample_start));
}

/**
 *  Change the data type of a CDS Variable.
 *
 *  This function will change the data type of a CDS variable. All data and
 *  data attribute values (see cds_add_data_att()) will be converted to the
 *  new data type. This includes any associated boundary variables since
 *  they are considered to be part of a coordinate variable's metadata.
 *  Likewise, if var is a boundary variable the coordinate variable it is
 *  associated with will be converted as well.
 *
 *  All missing values defined for the variable will be mapped to the new data
 *  type using the cds_get_missing_values_map() function.
 *
 *  If the range of the new data type is less than the range of the variable's
 *  data type, all out-of-range values will be set to the min/max values of the
 *  new data type.
 *
 *  The variable's data index will also be destroyed if the size of the new
 *  data type is not equal to the size of the old data type. The calling
 *  process is responsible for recreating the data index if necessary
 *  (see cds_create_var_data_index()).
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  var  - pointer to the variable
 *  @param  type - new data type
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int cds_change_var_type(
    CDSVar      *var,
    CDSDataType  type)
{
    CDSConverter  converter;
    CDSVar       *coord_var;
    CDSVar       *bounds_var;

    coord_var = (CDSVar *)NULL;

    /* Check for an associated bounds variable */

    bounds_var = cds_get_bounds_var(var);
    if (bounds_var) {

        converter = cds_create_converter_var_to_array(
            bounds_var, type, NULL, 0, NULL);

        if (!converter) return(0);

        if (!cds_convert_var(converter, bounds_var)) {
            cds_destroy_converter(converter);
            return(0);
        }

        cds_destroy_converter(converter);
    }
    else {

        /* Check for an associated coordinate variable */

        coord_var = cds_get_bounds_coord_var(var);
    }

    /* Change the data type of the variable */

    converter = cds_create_converter_var_to_array(
        var, type, NULL, 0, NULL);

    if (!converter) return(0);

    if (!cds_convert_var(converter, var)) {
        cds_destroy_converter(converter);
        return(0);
    }

    cds_destroy_converter(converter);

    /* Convert the coordinate variable if one was found */

    if (coord_var) {

        converter = cds_create_converter_var_to_array(
            coord_var, type, NULL, 0, NULL);

        if (!converter) return(0);

        if (!cds_convert_var(converter, coord_var)) {
            cds_destroy_converter(converter);
            return(0);
        }

        cds_destroy_converter(converter);
    }

    return(1);
}

/**
 *  Change the units of a CDS Variable.
 *
 *  This function will change the data type and units of a CDS variable. All
 *  data and data attribute values (see cds_add_data_att()) will be converted
 *  to the new data type and units. This includes any associated boundary
 *  variables since they are considered to be part of a coordinate variable's
 *  metadata. Likewise, if var is a boundary variable the coordinate variable
 *  it is associated with will be converted as well.
 *
 *  All missing values defined for the variable will be mapped to the new data
 *  type using the cds_get_missing_values_map() function.
 *
 *  If the range of the new data type is less than the range of the variable's
 *  data type, all out-of-range values will be set to the min/max values of the
 *  new data type.
 *
 *  The variable's data index will also be destroyed if the size of the new
 *  data type is not equal to the size of the old data type. The calling
 *  process is responsible for recreating the data index if necessary
 *  (see cds_create_var_data_index()).
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  var   - pointer to the variable
 *  @param  type  - new data type
 *  @param  units - new units
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int cds_change_var_units(
    CDSVar      *var,
    CDSDataType  type,
    const char  *units)
{
    CDSConverter  converter;
    CDSVar       *coord_var;
    CDSVar       *bounds_var;

    coord_var = (CDSVar *)NULL;

    /* Check for an associated bounds variable */

    bounds_var = cds_get_bounds_var(var);
    if (bounds_var) {

        converter = cds_create_converter_var_to_array(
            bounds_var, type, units, 0, NULL);

        if (!converter) return(0);

        if (!cds_convert_var(converter, bounds_var)) {
            cds_destroy_converter(converter);
            return(0);
        }

        cds_destroy_converter(converter);
    }
    else {

        /* Check for an associated coordinate variable */

        coord_var = cds_get_bounds_coord_var(var);
    }

    converter = cds_create_converter_var_to_array(
        var, type, units, 0, NULL);

    if (!converter) return(0);

    if (!cds_convert_var(converter, var)) {
        cds_destroy_converter(converter);
        return(0);
    }

    cds_destroy_converter(converter);

    /* Convert the coordinate variable if one was found */

    if (coord_var) {

        converter = cds_create_converter_var_to_array(
            coord_var, type, units, 0, NULL);

        if (!converter) return(0);

        if (!cds_convert_var(converter, coord_var)) {
            cds_destroy_converter(converter);
            return(0);
        }

        cds_destroy_converter(converter);
    }

    return(1);
}

/**
 *  Create a data index for multi-dimensional variable data.
 *
 *  This function will return a data index that can be used to access
 *  the data in a variable using the traditional x[i][j][k] syntax.
 *  It is up to the calling process to cast the return value of this
 *  function into the proper data type.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  var - pointer to the variable
 *
 *  @return
 *    - the data index into the variables data array
 *    - NULL if:
 *        - no data has been stored in the variable (var->sample_count == 0)
 *        - a memory allocation error occurred
 */
void *cds_create_var_data_index(CDSVar *var)
{
    void *data_index = _cds_create_var_data_index(var, 0);

    return(data_index);
}

/**
 *  Delete the data for a CDS variable.
 *
 *  @param  var - pointer to the variable
 */
void cds_delete_var_data(CDSVar *var)
{
    if (var) {

        _cds_delete_var_data_index(var);

        if (var->data.vp) free(var->data.vp);

        var->sample_count = 0;
        var->alloc_count  = 0;
        var->data.vp      = (void *)NULL;
    }
}

/**
 *  Get the data from a CDS variable.
 *
 *  This function will get the data from a variable casted into the specified
 *  data type. All missing values used in the data will be converted to a single
 *  missing value appropriate for the requested data type. The missing value
 *  used will be the first value returned by the cds_get_missing_values_map()
 *  function. If no missing values are defined for the variable data, the
 *  missing_value returned will be the default fill value for the requested
 *  data type.
 *
 *  If the range of the output data type is less than the range of the
 *  variable's data type, all out-of-range values will be set to the min/max
 *  values of the output data type.
 *
 *  Memory will be allocated for the returned data array if the output array
 *  is NULL. In this case the calling process is responsible for freeing
 *  the allocated memory. If an output data array is specified it must be
 *  large enough to hold (sample_count * cds_var_sample_size(var)) values
 *  of the specified data type.
 *
 *  For multi-dimensional variables, the values in the output data array will
 *  be stored linearly in memory with the last dimension varying the fastest.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
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
 *        - the variable has no data for sample_start (sample_count == 0)
 *        - a memory allocation error occurs (sample_count == (size_t)-1)
 */
void *cds_get_var_data(
    CDSVar       *var,
    CDSDataType   type,
    size_t        sample_start,
    size_t       *sample_count,
    void         *missing_value,
    void         *data)
{
    CDSConverter converter;
    size_t       type_size;
    size_t       sample_size;
    void        *var_data;
    size_t       nsamples;
    size_t       length;

    /* Check if the variable has any data for the requested sample_start */

    if (!var || !var->data.vp || var->sample_count <= sample_start) {
        if (sample_count) *sample_count = 0;
        return((void *)NULL);
    }

    /* Create the converter */

    converter = cds_create_converter_var_to_array(
        var, type, NULL, 0, missing_value);

    if (!converter) {
        return((void *)NULL);
    }

    /* Get pointer to the start sample in the variable data */

    type_size   = cds_data_type_size(var->type);
    sample_size = cds_var_sample_size(var);
    var_data    = var->data.bp + (sample_start * sample_size * type_size);

    /* Determine the number of samples to get */

    nsamples = var->sample_count - sample_start;

    if (data && sample_count && *sample_count > 0) {
        if (nsamples > *sample_count) {
            nsamples = *sample_count;
        }
    }

    if (sample_count) *sample_count = nsamples;

    /* Copy data from the variable to the output array */

    length = nsamples * sample_size;
    data   = cds_convert_array(converter, 0, length, var_data, data);

    cds_destroy_converter(converter);

    if (!data) {

        ERROR( CDS_LIB_NAME,
            "Could not get variable data for: %s\n"
            " -> memory allocation error\n",
            cds_get_object_path(var));

        if (sample_count) *sample_count = (size_t)-1;
        return((void *)NULL);
    }

    return(data);
}

/**
 *  Get a pointer to the data in a CDS variable.
 *
 *  @param  var          - pointer to the CDSVar
 *  @param  sample_start - start sample
 *
 *  @return
 *    -  pointer to the variable data
 *    -  NULL if the variable has no data for sample_start
 */
void *cds_get_var_datap(CDSVar *var, size_t sample_start)
{
    size_t sample_size;
    size_t type_size;

    if (!sample_start) {
        return(var->data.vp);
    }

    if (!var->data.vp || sample_start >= var->sample_count) {
        return((void *)NULL);
    }

    sample_size = cds_var_sample_size(var);
    type_size   = cds_data_type_size(var->type);

    return(var->data.bp + sample_start * sample_size * type_size);
}

/**
 *  Get the missing values for a CDS Variable.
 *
 *  This function will return an array containing all possible missing values
 *  used by the specified variable, and will be the same data type as the
 *  variable.  These are determined by searching for the following attributes:
 *
 *       - missing_value
 *       - missing-value  depricated
 *       - missing_data   depricated
 *       - missing-data   depricated
 *       - missing_value1 depricated
 *       - Missing_value  depricated
 *       - _FillValue
 *
 *  <b>Note that the deprecated attributes should not be used by new code.</b>
 *
 *  If none of these attributes are found for the variable, the parent group
 *  will be searched.
 *
 *  If the _FillValue attribute does not exist but a default fill value has
 *  been defined, it will be used instead.
 *
 *  The memory used by the output array of missing values is dynamically
 *  allocated. It is the responsibility of the calling process to free
 *  this memory when it is no longer needed.
 *
 *  This function has also been updated to search for the following
 *  non-standard missing value attributes:
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
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
int cds_get_var_missing_values(CDSVar *var, void **values)
{
    CDSGroup  *parent;
    CDSAtt    *mv_att;
    size_t     nmv;
    size_t     type_size;
    size_t     nvalues;
    void      *mvp;
    int        mi;

    nvalues   = 0;
    *values   = (void *)NULL;
    type_size = cds_data_type_size(var->type);

    /* Loop over all known variations of the missing value
     * attribute names at the field level. */

    for (mi = 0; mi < _NumMissingValueAttNames; ++mi) {

        mv_att = cds_get_att(var, _MissingValueAttNames[mi]);
        if (!mv_att || !mv_att->length || !mv_att->value.vp) continue;

        nmv = mv_att->length;

        *values = realloc(*values, (nvalues + nmv) * type_size);
        if (!*values) goto MEMORY_ERROR;

        mvp = (char *)(*values) + (nvalues * type_size);

        cds_get_att_value(mv_att, var->type, &nmv, mvp);
        if (nmv == (size_t)-1) return(-1);

        nvalues += nmv;
    }

    /* If a missing value attribute was not found, search again
     * at the global attribute level. */

    parent = (CDSGroup *)var->parent;

    while (!nvalues && parent) {

        for (mi = 0; mi < _NumMissingValueAttNames; ++mi) {

            mv_att = cds_get_att(parent, _MissingValueAttNames[mi]);
            if (!mv_att || !mv_att->length || !mv_att->value.vp) continue;

            nmv = mv_att->length;

            *values = realloc(*values, (nvalues + nmv) * type_size);
            if (!*values) goto MEMORY_ERROR;

            mvp = (char *)(*values) + (nvalues * type_size);

            cds_get_att_value(mv_att, var->type, &nmv, mvp);
            if (nmv == (size_t)-1) return(-1);

            nvalues += nmv;
        }

        parent = (CDSGroup *)parent->parent;
    }

    /* Check for the var->default_fill value if the
     * _FillValue attribute was not found. */

    if (var->default_fill && !cds_get_att(var, "_FillValue")) {

        *values = realloc(*values, (nvalues + 1) * type_size);
        if (!*values) goto MEMORY_ERROR;

        mvp = (char *)(*values) + (nvalues * type_size);

        memcpy(mvp, var->default_fill, type_size);

        nvalues += 1;
    }

    /* Return the number of missing values found. */

    return(nvalues);

MEMORY_ERROR:

    ERROR( CDS_LIB_NAME,
        "Could not get mising values for variable: %s\n"
        " -> memory allocation error\n",
        cds_get_object_path(var));

    return(-1);
}

/**
 *  Get the units of a CDS Variable.
 *
 *  The memory used by the returned string is internal to the variable's
 *  units attribute and must not be freed by the calling process.
 *
 *  @param  var - pointer to the variable
 *
 *  @return
 *    -  pointer to the variable's units attribute value
 *    -  NULL if the attribute does not exist or is not a text attribute
 */
const char *cds_get_var_units(CDSVar *var)
{
    CDSAtt *units_att;

    units_att = cds_get_att(var, "units");

    if (!units_att) {

        /* Check if this is a bounds variable */

        var = cds_get_bounds_coord_var(var);
        if (var) {
            units_att = cds_get_att(var, "units");
        }

        if (!units_att) {
            return((const char *)NULL);
        }
    }

    if (units_att->type != CDS_CHAR) {
        return((const char *)NULL);
    }

    return((const char *)units_att->value.cp);
}

/**
 *  Initialize the data values for a CDS variable.
 *
 *  This function will make sure enough memory is allocated for the specified
 *  samples and initializing the data values to either the variable's missing
 *  value (use_missing == 1), or 0 (use_missing == 0).
 *
 *  The search order for missing values is:
 *
 *       - missing_value
 *       - missing-value  depricated
 *       - missing_data   depricated
 *       - missing-data   depricated
 *       - missing_value1 depricated
 *       - Missing_value  depricated
 *       - _FillValue
 *       - variable's default missing value
 *
 *  <b>Note that the deprecated attributes should not be used by new code.</b>
 *
 *  If none of these attributes are found for the variable, the parent group
 *  will be searched.
 *
 *  If the variable does not have any missing or fill values defined the
 *  default fill value for the variable's data type will be used and the
 *  default fill value for the variable will be set.
 *
 *  If the specified start sample is greater than the variable's current sample
 *  count, the hole between the two will be filled with the first missing or
 *  fill value defined for the variable.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
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
void *cds_init_var_data(
    CDSVar *var,
    size_t  sample_start,
    size_t  sample_count,
    int     use_missing)
{
    size_t  type_size   = cds_data_type_size(var->type);
    size_t  sample_size = cds_var_sample_size(var);
    void   *var_data;
    size_t  length;
    char    missing[CDS_MAX_TYPE_SIZE];

    if (!sample_count) {

        ERROR( CDS_LIB_NAME,
            "Could not initialize variable data for: %s\n"
            " -> specified sample count is zero\n",
            cds_get_object_path(var));

        return((void *)NULL);
    }

    /* Allocate memory for the data samples and get the pointer to the
     * start sample in the variable's data array */

    var_data = cds_alloc_var_data(var, sample_start, sample_count);
    if (!var_data) {
        return((void *)NULL);
    }

    /* Initialize the data values */

    if (use_missing) {

        if (!_cds_get_first_missing_value(var, (void *)missing)) {

            cds_get_default_fill_value(var->type, (void *)missing);

            if (!cds_set_var_default_fill_value(var, (void *)missing)) {

                ERROR( CDS_LIB_NAME,
                    "Could not initialize variable data: %s\n"
                    " -> memory allocation error\n",
                    cds_get_object_path(var));

                return((void *)NULL);
            }
        }

        length = sample_count * sample_size;

        cds_init_array(var->type, length, (void *)missing, var_data);
    }
    else {
        length = sample_count * sample_size * type_size;
        memset(var_data, 0, length);
    }

    return(var_data);
}

/**
 *  Initialize the data values for a CDS variable.
 *
 *  This function behaves the same as cds_init_var_data() except that it
 *  returns a data index starting at the specified start sample, see
 *  cds_create_data_index() for details. For variables that have less than
 *  two dimensions this function is identical to cds_init_var_data().
 *
 *  The data index returned by this function belongs to the CDS variable
 *  and will be freed when the variable is destroyed. The calling process
 *  must *not* attempt to free this memory.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
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
void *cds_init_var_data_index(
    CDSVar *var,
    size_t  sample_start,
    size_t  sample_count,
    int     use_missing)
{
    void *datap = cds_init_var_data(
        var, sample_start, sample_count, use_missing);

    if (!datap) {
        return((void *)NULL);
    }
    else if (var->ndims < 2) {
        return(datap);
    }

    return(_cds_create_var_data_index(var, sample_start));
}

/**
 *  Check if an attribute name is one of the known variations of "missing_value".
 *
 *  The list of know variations of the missing value attribute names are:
 *
 *       - missing_value
 *       - missing-value  depricated
 *       - missing_data   depricated
 *       - missing-data   depricated
 *       - missing_value1 depricated
 *       - Missing_value  depricated
 *       - _FillValue
 *
 *  <b>Note that the deprecated attributes should not be used by new code.</b>
 *
 *  @param  att_name - attribute name
 *
 *  @return
 *    - 1  if this is a missing value attribute
 *    - 0  if this is *not* a missing value attribute
 */
int cds_is_missing_value_att_name(const char *att_name)
{
    int mi;

    for (mi = 0; mi < _NumMissingValueAttNames; ++mi) {
        if (strcmp(att_name, _MissingValueAttNames[mi]) == 0) {
            return(1);
        }
    }

    return(0);
}

/**
 *  Deprecated: use cds_set_var_data() instead.
 *
 *  @param  var          - pointer to the variable
 *  @param  sample_start - start sample of the new data (0 based indexing)
 *  @param  sample_count - number of new samples
 *  @param  type         - data type of the input data array
 *  @param  data         - pointer to the input data array
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
void *cds_put_var_data(
    CDSVar      *var,
    size_t       sample_start,
    size_t       sample_count,
    CDSDataType  type,
    void        *data)
{
    return(cds_set_var_data(
        var, type, (int)sample_start, (int)sample_count, NULL, data));
}

/**
 *  Reset the sample counts for the variables in a CDSGroup.
 *
 *  This function will reset the sample counts for the variables in a CDSGroup
 *  to 0 without freeing the memory already allocated. This allows for the
 *  memory already allocated for the variable data to be reused without the
 *  need to free and reallocate it.
 *
 *  When reseting the sample counts for all variables that have an unlimited
 *  dimension, the length of the unlimited dimension will also be reset to 0.
 *
 *  The functions cds_alloc_var_data(), cds_alloc_var_data_index(), and/or
 *  cds_set_var_data() must still be used when adding data to the variable.
 *  These ensure there is enough memory allocated for the data being added,
 *  and set the unlimited dimension length properly.
 *
 *  @param  group       - pointer to the group
 *  @param  unlim_vars  - reset the sample_count for all variables that
 *                        have an unlimited dimension
 *  @param  static_vars - reset the sample counts for all static variables
 *                        that do not have an unlimited dimension
 */
void cds_reset_sample_counts(
    CDSGroup *group,
    int       unlim_vars,
    int       static_vars)
{
    CDSVar *var;
    int     vi;

    for (vi = 0; vi < group->nvars; vi++) {

        var = group->vars[vi];

        if (var->ndims && var->dims[0]->is_unlimited) {
            if (unlim_vars) {
                var->dims[0]->length = 0;
                var->sample_count    = 0;
            }
        }
        else if (static_vars) {
            var->sample_count = 0;
        }
    }
}

/**
 *  Set cell boundary data for all coordinate variables in a CDS group.
 *
 *  This function will call cds_set_bounds_var_data() for all variables
 *  in the specified CDS group that have a bounds attribute defined.
 *
 *  @param  group        - pointer to the CDS variable
 *  @param  sample_start - start sample along the unlimited dimension
 *                         (0 based indexing)
 *  @param  sample_count - number of samples along the unlimited dimension
 *                         (0 for all available samples)
 *
 *  @return
 *    -  1 if successful
 *    -  0 if an error occurred.
 */
int cds_set_bounds_data(
    CDSGroup *group,
    size_t    sample_start,
    size_t    sample_count)
{
    CDSVar *var;
    size_t  start;
    size_t  count;
    int     status;
    int     vi;

    for (vi = 0; vi < group->nvars; ++vi) {

        var = group->vars[vi];

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

        status = cds_set_bounds_var_data(var, start, count);
        if (status < 0) return(0);
    }

    return(1);
}

/**
 *  Set cell boundary data values for a CDS coordinate variable.
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
int cds_set_bounds_var_data(
    CDSVar *coord_var,
    size_t  sample_start,
    size_t  sample_count)
{
    CDSVar *bounds_var;
    CDSDim *bounds_dim;
    CDSAtt *att;

    /* Check for the bounds variable */

    bounds_var = cds_get_bounds_var(coord_var);
    if (!bounds_var) return(0);

    /* Check for the bound_offsets attribute */

    att = cds_get_att(bounds_var, "bound_offsets");
    if (!att) return(0);

    if (att->type != coord_var->type) {

        ERROR( CDS_LIB_NAME,
            "Invalid data type for bound_offsets attribute: %s\n"
            " -> the bound_offsets attribute must have the same\n"
            " -> data type as its associated variable\n",
            cds_get_object_path(att));

        return(-1);
    }

    bounds_dim = bounds_var->dims[coord_var->ndims];

    if (att->length != bounds_dim->length) {

        ERROR( CDS_LIB_NAME,
            "Invalid length for bound_offsets attribute: %s\n"
            " -> the bound_offsets attribute must have the same\n"
            " -> length as the bounds dimension\n",
            cds_get_object_path(att));

        return(-1);
    }

    return(_cds_set_bounds_var_data(
        coord_var, sample_start, sample_count, att->value.vp, bounds_var));
}

/**
 *  Set the default _FillValue for a CDS Variable.
 *
 *  This function will set the default _FillValue that should be used
 *  if a _FillValue attribute is not defined. The value pointed to by
 *  the fill_value argument must be the same data type as the variable.
 *
 *  @param  var        - pointer to the variable
 *  @param  fill_value - pointer to the fill value, or NULL to use the
 *                       default fill value for the variable's data type.
 *
 *  @return
 *    - 1 if successful
 *    - 0 if a memory allocation error occurred.
 */
int cds_set_var_default_fill_value(CDSVar *var, void *fill_value)
{
    size_t  type_size = cds_data_type_size(var->type);
    void   *new_fill;

    if (!fill_value) {
        fill_value = _cds_default_fill_value(var->type);
    }

    new_fill = cds_memdup(type_size, fill_value);

    if (!new_fill) {

        ERROR( CDS_LIB_NAME,
            "Could not set default fill value for variable: %s\n"
            " -> memory allocation error\n",
            cds_get_object_path(var));

        return(0);
    }

    if (var->default_fill) free(var->default_fill);

    var->default_fill = new_fill;

    return(1);
}

/**
 *  Set data values for a CDS variable.
 *
 *  This function will set the data values of a variable by casting the values
 *  in the input data array into the data type of the variable. All missing
 *  values in the input data array will be converted to the first missing or
 *  fill value defined for the variable. The search order for missing values is:
 *
 *       - missing_value
 *       - missing-value  depricated
 *       - missing_data   depricated
 *       - missing-data   depricated
 *       - missing_value1 depricated
 *       - Missing_value  depricated
 *       - _FillValue
 *       - variable's default missing value
 *
 *  <b>Note that the deprecated attributes should not be used by new code.</b>
 *
 *  If none of these attributes are found for the variable, the parent group
 *  will be searched.
 *
 *  If the variable does not have any missing or fill values defined the
 *  default fill value for the variable's data type will be used and the
 *  default fill value for the variable will be set.
 *
 *  If the specified start sample is greater than the variable's current sample
 *  count, the hole between the two will be filled with the first missing or
 *  fill value defined for the variable.
 *
 *  If the range of the variable's data type is less than the range of the
 *  input data type, all out-of-range values will be set to the min/max values
 *  of the variable's data type.
 *
 *  For multi-dimensional variables, the values in the input data array must
 *  be stored linearly in memory with the last dimension varying the fastest.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  var           - pointer to the variable
 *  @param  type          - data type of the input data
 *  @param  sample_start  - start sample of the new data (0 based indexing)
 *  @param  sample_count  - number of new samples
 *  @param  missing_value - pointer to the missing value used in the data array,
 *                          or NULL if the data does not contain any missing values
 *  @param  data          - pointer to the input data array
 *
 *  @return
 *    - pointer to the specifed start sample in the variable's data array
 *
 *    - NULL if:
 *        - the specified sample count is zero, or the data pointer is NULL
 *        - one of the variable's static dimensions has 0 length
 *        - the variable has no dimensions, and sample_start is not equal
 *          to 0 or sample_count is not equal to 1.
 *        - the first variable dimension is not unlimited, and
 *          sample_start + sample_count would exceed the dimension length.
 *        - a memory allocation error occurred
 */
void *cds_set_var_data(
    CDSVar       *var,
    CDSDataType   type,
    size_t        sample_start,
    size_t        sample_count,
    void         *missing_value,
    void         *data)
{
    CDSConverter converter;
    void        *var_data;
    size_t       length;

    if (!sample_count) {

        ERROR( CDS_LIB_NAME,
            "Could not set variable data for: %s\n"
            " -> specified sample count is zero\n",
            cds_get_object_path(var));

        return((void *)NULL);
    }

    if (!data) {

        ERROR( CDS_LIB_NAME,
            "Could not set variable data for: %s\n"
            " -> specified data pointer is NULL\n",
            cds_get_object_path(var));

        return((void *)NULL);
    }

    /* Create the converter */

    if (missing_value) {
        converter = cds_create_converter_array_to_var(
            type, NULL, 1, missing_value, var);
    }
    else {
        converter = cds_create_converter_array_to_var(
            type, NULL, 0, NULL, var);
    }

    if (!converter) {
        return((void *)NULL);
    }

    /* Allocate memory for the new data samples and get the pointer to the
     * start sample in the variable's data array */

    var_data = cds_alloc_var_data(var, sample_start, sample_count);
    if (!var_data) {
        cds_destroy_converter(converter);
        return((void *)NULL);
    }

    /* Copy data from the input data array into the variable's data array */

    length = sample_count * cds_var_sample_size(var);
    cds_convert_array(converter, 0, length, data, var_data);

    cds_destroy_converter(converter);

    return(var_data);
}
