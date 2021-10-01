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
*     email: brian.ermold@pnnl.gov
*
*******************************************************************************/

/** @file cds_converter.c
 *  CDS Conversions.
 */

#include "cds3.h"
#include "cds_private.h"

/*******************************************************************************
 *  Private Data and Functions
 */
/** @privatesection */

typedef struct CDSDataAtts {
    char *name;
    int   flags;
} CDSDataAtts;

static CDSDataAtts _DefaultDataAtts[] = {
    { "valid_min",      0 },
    { "valid_max",      0 },
    { "valid_range",    0 },
    { "bound_offsets",  CDS_DELTA_UNITS  },
    { "flag_masks",     CDS_IGNORE_UNITS },
    { "flag_values",    CDS_IGNORE_UNITS },
    { "valid_delta",    CDS_DELTA_UNITS  },
    { "missing_value",  CDS_IGNORE_UNITS },
    { "_FillValue",     CDS_IGNORE_UNITS },
    { "missing-value",  CDS_IGNORE_UNITS },
    { "missing_data",   CDS_IGNORE_UNITS },
    { "missing-data",   CDS_IGNORE_UNITS },
    { "missing_value1", CDS_IGNORE_UNITS },
    { "Missing_value",  CDS_IGNORE_UNITS },
};
static int _NumDefaultDataAtts = sizeof(_DefaultDataAtts)/sizeof(CDSDataAtts);

static CDSDataAtts *_UserDataAtts;
static int          _NumUserDataAtts;

/**
 *  Private: Cleanup CDS data converter mapping values.
 *
 *  @param  dc - pointer to the _CDSConverter
 */
static void _cds_cleanup_converter_map(_CDSConverter *dc)
{
    if (dc) {

        if (dc->in_map) {
            cds_free_array(dc->in_type, dc->map_length, dc->in_map);
        }

        if (dc->out_map) {
            cds_free_array(dc->out_type, dc->map_length, dc->out_map);
        }

        dc->map_length = 0;
        dc->in_map     = (void *)NULL;
        dc->out_map    = (void *)NULL;
    }
}

/**
 *  Private: Cleanup CDS data converter range values.
 *
 *  @param  dc - pointer to the _CDSConverter
 */
static void _cds_cleanup_converter_range(_CDSConverter *dc)
{
    if (dc) {

        if (dc->out_min) free(dc->out_min);
        if (dc->orv_min) free(dc->orv_min);
        if (dc->out_max) free(dc->out_max);
        if (dc->orv_max) free(dc->orv_max);

        dc->out_min = (void *)NULL;
        dc->orv_min = (void *)NULL;
        dc->out_max = (void *)NULL;
        dc->orv_max = (void *)NULL;
    }
}

/**
 *  Private: Free all memory used by a CDS data converter.
 *
 *  @param  dc - pointer to the _CDSConverter
 */
static void _cds_destroy_converter(_CDSConverter *dc)
{
    if (dc) {
        if (dc->in_units)  free(dc->in_units);
        if (dc->out_units) free(dc->out_units);
        if (dc->uc)        cds_free_unit_converter(dc->uc);

        _cds_cleanup_converter_map(dc);
        _cds_cleanup_converter_range(dc);

        free(dc);
    }
}

/**
 *  Check if a conversion is necessary.
 *
 *  @param  converter - the data converter
 *  @param  flags     - conversion flags
 *
 *  @return
 *    - 1 = TRUE
 *    - 0 = FALSE
 */
int _cds_has_conversion(CDSConverter converter, int flags)
{
    _CDSConverter *dc = (_CDSConverter *)converter;

    if ((dc->in_type != dc->out_type)           ||
        (dc->uc && !(flags & CDS_IGNORE_UNITS)) ||
        (dc->map_length && !dc->map_ident)      ||
        (dc->orv_min || dc->orv_max)) {

        return(1);
    }

    return(0);
}

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Add a data attribute.
 *
 *  A data attribute is a variable attribute that has the same data
 *  type as the variable and whose type and/or units need to be changed
 *  if the variable's type and/or units are changed.  By default the
 *  cds_change_var_types() and cds_change_var_units() functions will
 *  also convert the data type and/or units of the following attributes:
 *
 *    - valid_min
 *    - valid_max
 *    - valid_range
 *    - valid_delta
 *    - missing_value
 *    - _FillValue
 *
 *  This function can be used to add additional data attributes. The
 *  available conversion flags are:
 *
 *    - CDS_INGNORE_UNITS:   Do not apply the units conversion to the attribute
 *                           values when the variable units are changed.
 *
 *    - CDS_DELTA_UNITS:     Apply the units conversion by subtracting
 *                           the value converted to the new units from
 *                           twice the value converted to the new units.
 *
 *  The data attributes added by this function are stored internally
 *  in dynamically allocated memory.  This memory can be freed by
 *  calling the cds_free_data_atts() function.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  name  - name of the attribute
 *  @param  flags - conversion flags
 *
 *  @return
 *    - 1 if successful
 *    - 0 if a memory allocation error occurred
 */
int cds_add_data_att(const char *name, int flags)
{
    CDSDataAtts *new_data_atts;
    char        *new_name;
    int          ai;

    /* Check if this is one of the default data attributes */

    for (ai = 0; ai < _NumDefaultDataAtts; ai++) {
        if (strcmp(name, _DefaultDataAtts[ai].name) == 0) {
            return(1);
        }
    }

    /* Check if this data attribute has already been defined */

    for (ai = 0; ai < _NumUserDataAtts; ai++) {
        if (strcmp(name, _UserDataAtts[ai].name) == 0) {
            _UserDataAtts[ai].flags |= flags;
            return(1);
        }
    }

    /* Allocate memory for one more data attribute */

    new_data_atts = (CDSDataAtts *)realloc(
        _UserDataAtts, (_NumUserDataAtts + 1) * sizeof(CDSDataAtts));

    if (!new_data_atts) {

        ERROR( CDS_LIB_NAME,
            "Could not add new data attribute: %s\n"
            " -> memory allocation error\n", name);

        return(0);
    }

    _UserDataAtts = new_data_atts;

    new_name = strdup(name);
    if (!new_name) {

        ERROR( CDS_LIB_NAME,
            "Could not add new data attribute: %s\n"
            " -> memory allocation error\n", name);

        return(0);
    }

    _UserDataAtts[_NumUserDataAtts].name  = new_name;
    _UserDataAtts[_NumUserDataAtts].flags = flags;
    _NumUserDataAtts++;

    return(1);
}

/**
 *  Free the internal memory used to stored the user defined data attributes.
 */
void cds_free_data_atts(void)
{
    int ai;

    if (_UserDataAtts) {

        for (ai = 0; ai < _NumUserDataAtts; ai++) {
            free(_UserDataAtts[ai].name);
        }

        free(_UserDataAtts);
    }

    _UserDataAtts = (CDSDataAtts *)NULL;
    _NumUserDataAtts = 0;
}

/**
 *  Check if an attribute is a data attribute.
 *
 *  See cds_add_data_att() for details.
 *
 *  @param att   - pointer to the attribute
 *  @param flags - output: data attribute flags
 *
 *  @return
 *    - 1 if this is a data attribute
 *    - 0 if this is not a data attribute
 */
int cds_is_data_att(CDSAtt *att, int *flags)
{
    CDSObject *parent = att->parent;
    CDSVar    *var;
    int        ai;

    if (parent->obj_type != CDS_VAR) {
        return(0);
    }

    var = (CDSVar *)att->parent;
    if (att->type != var->type) {
        return(0);
    }

    for (ai = 0; ai < _NumDefaultDataAtts; ai++) {
        if (strcmp(_DefaultDataAtts[ai].name, att->name) == 0) {
            if (flags) *flags = _DefaultDataAtts[ai].flags;
            return(1);
        }
    }

    for (ai = 0; ai < _NumUserDataAtts; ai++) {
        if (strcmp(_UserDataAtts[ai].name, att->name) == 0) {
            if (flags) *flags = _UserDataAtts[ai].flags;
            return(1);
        }
    }

    return(0);
}

/**
 *  Convert an array of data values.
 *
 *  This function will copy the data from the in_data array to the out_data
 *  array using the specified CDS converter.
 *
 *  Memory will be allocated for the output data array if the out_data argument
 *  is NULL. In this case the calling process is responsible for freeing the
 *  allocated memory.
 *
 *  The input and output data arrays can be identical. If this is the case and
 *  the size of the output data type is less than or equal to the size of the
 *  input data type the data conversion will be done in place. However, if the
 *  size of the output data type is greater than the size of the input data
 *  type, memory will be allocated for the output data array and the pointer
 *  to this new array will be returned. The calling process can check if memory
 *  was allocated by comparing the returned pointer with the in_data pointer.
 *  If they are no longer equal then memory was allocated and the calling
 *  process is responsible for freeing the memory.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  converter - the data converter
 *  @param  flags     - conversion flags (see cds_add_data_att())
 *  @param  length    - number of values in the data arrays
 *  @param  in_data   - pointer to the input data array
 *  @param  out_data  - pointer to the output data array
 *                      (memory will allocated if this argument is NULL)
 *
 *  @return
 *    - pointer to the output data array
 *    - NULL if a memory allocation error occurs
 *      (this can only happen if the specified out_data argument is NULL)
 */
void *cds_convert_array(
    CDSConverter  converter,
    int           flags,
    size_t        length,
    void         *in_data,
    void         *out_data)
{
    _CDSConverter *dc = (_CDSConverter *)converter;
    void          *datap;

    if (out_data && out_data == in_data) {

        if (dc->out_size > dc->in_size) {
            out_data = (void *)NULL;
        }
        else if (!_cds_has_conversion(converter,flags)) {
            return(out_data);
        }
    }

    if (dc->uc && !(flags & CDS_IGNORE_UNITS)) {

        /* Units Conversion */

        if (flags & CDS_DELTA_UNITS) {

            datap = cds_convert_unit_deltas(
                dc->uc,
                dc->in_type, length, in_data,
                dc->out_type, out_data,
                dc->map_length, dc->in_map, dc->out_map);
        }
        else {

            datap = cds_convert_units(
                dc->uc,
                dc->in_type, length, in_data,
                dc->out_type, out_data,
                dc->map_length, dc->in_map, dc->out_map,
                dc->out_min, dc->orv_min,
                dc->out_max, dc->orv_max);
        }
    }
    else {

        /* No Units Conversion */

        if (dc->map_ident) {
            datap = cds_copy_array(
                dc->in_type, length, in_data,
                dc->out_type, out_data,
                0, NULL, NULL,
                dc->out_min, dc->orv_min,
                dc->out_max, dc->orv_max);
        }
        else {
            datap = cds_copy_array(
                dc->in_type, length, in_data,
                dc->out_type, out_data,
                dc->map_length, dc->in_map, dc->out_map,
                dc->out_min, dc->orv_min,
                dc->out_max, dc->orv_max);
        }
    }

    return(datap);
}

/**
 *  Convert the data in a CDS Variable.
 *
 *  This function will update the data values in a CDS Variable using the
 *  specified converter. It will also perform the unit and/or type conversions
 *  for all data attributes (see cds_add_data_att()).
 *
 *  The variable's data index will also be destroyed if the size of the new
 *  data type is not equal to the size of the old data type. The calling
 *  process is responsible for recreating the data index if necessary
 *  (see cds_create_var_data_index()).
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  converter - the data converter
 *  @param  var       - pointer to the variable
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int cds_convert_var(
    CDSConverter  converter,
    CDSVar       *var)
{
    _CDSConverter *dc = (_CDSConverter *)converter;
    CDSAtt        *att;
    size_t         length;
    int            flags;
    void          *datap;
    int            ai;

    /* Check if a conversion is needed */

    if (!_cds_has_conversion(converter, 0)) {

        /* Check if the units attribute needs to be updated */

        if (dc->out_units &&
            (!dc->in_units || strcmp(dc->in_units, dc->out_units) != 0)) {

            /* Some variables (i.e. boundary variables) do not need to
             * have a units attribute so we do not want to add it if it
             * does not already exist. */

            if (cds_get_att(var, "units")) {

                length = strlen(dc->out_units) + 1;

                if (!cds_change_att(
                    var, 1, "units", CDS_CHAR, length, dc->out_units)) {

                    return(0);
                }
            }
        }

        return(1);
    }

    /* Check if we can update the variable's data type if necessary */

    if ((dc->in_type != dc->out_type) && var->def_lock) {

        ERROR( CDS_LIB_NAME,
            "Could not convert variable data type for: %s\n"
            " -> the variable definition lock is set to: %d\n",
            cds_get_object_path(var), var->def_lock);

        return(0);
    }

    /* Update the variable's units attribute */

    if (dc->out_units &&
        (!dc->in_units || strcmp(dc->in_units, dc->out_units) != 0)) {

        /* Some variables (i.e. boundary variables) do not need to
         * have a units attribute so we do not want to add it if it
         * does not already exist. */

        if (cds_get_att(var, "units")) {

            length = strlen(dc->out_units) + 1;

            if (!cds_change_att(
                var, 1, "units", CDS_CHAR, length, dc->out_units)) {

                return(0);
            }
        }
    }

    /* Convert data values */

    if (var->sample_count) {

        length = var->sample_count * cds_var_sample_size(var);
        datap  = cds_convert_array(
            converter, 0, length, var->data.vp, var->data.vp);

        if (!datap) {

            ERROR( CDS_LIB_NAME,
                "Could not convert variable data for: %s\n"
                " -> memory allocation error\n",
                cds_get_object_path(var));

            return(0);
        }

        if (datap != var->data.vp) {
            free(var->data.vp);
            var->data.vp = datap;
            var->alloc_count = var->sample_count;
        }

        if (dc->in_size != dc->out_size) {
            /* The old data index will no longer be valid */
            if (var->data_index) {
                _cds_delete_var_data_index(var);
            }
        }
    }

    /* Convert data attributes */

    for (ai = 0; ai < var->natts; ai++) {

        att = var->atts[ai];

        if (cds_is_data_att(att, &flags)) {

            datap = cds_convert_array(
                converter, flags, att->length, att->value.vp, att->value.vp);

            if (!datap) {

                ERROR( CDS_LIB_NAME,
                    "Could not convert variable data for: %s\n"
                    " -> memory allocation error\n",
                    cds_get_object_path(var));

                return(0);
            }

            if (datap != att->value.vp) {
                free(att->value.vp);
                att->value.vp = datap;
            }

            att->type = dc->out_type;
        }
    }

    /* Convert default fill value */

    if (var->default_fill) {

        datap = cds_convert_array(
            converter, CDS_IGNORE_UNITS,
            1, var->default_fill, var->default_fill);

        if (!datap) {

            ERROR( CDS_LIB_NAME,
                "Could not convert variable data for: %s\n"
                " -> memory allocation error\n",
                cds_get_object_path(var));

            return(0);
        }

        if (datap != var->default_fill) {
            free(var->default_fill);
            var->default_fill = datap;
        }
    }

    var->type = dc->out_type;

    return(1);
}

/**
 *  Create a CDS data converter.
 *
 *  This function will create a data converter that can be used to convert
 *  data from one data type and/or units to another. Additional converter
 *  options can be set using the cds_set_converter_map() and
 *  cds_set_converter_range() functions.
 *
 *  If the range of the output data type is less than the range of the input
 *  data type, all out-of-range values will be set to the min/max values of
 *  the output data type. This default behavior can be changed using the
 *  cds_set_converter_range() function.
 *
 *  The memory used by the returned CDSConverter is dynamically allocated.
 *  It is the responsibility of the calling process to free this memory using
 *  the cds_destroy_converter() function.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  in_type   - data type of the input data
 *  @param  in_units  - units of the input data, or
 *                      NULL for no unit conversion
 *
 *  @param  out_type  - data type of the output data,
 *  @param  out_units - units of the output data, or
 *                      NULL for no unit conversion
 *
 *  @return
 *    - the data converter
 *    - NULL if attempting to convert between string and numbers,
 *           or attempting to convert units for string values,
 *           or a memory allocation error occurs
 *
 *  @see
 *    - cds_destroy_converter()
 *    - cds_set_converter_map()
 *    - cds_set_converter_range()
 */
CDSConverter cds_create_converter(
    CDSDataType  in_type,
    const char  *in_units,
    CDSDataType  out_type,
    const char  *out_units)
{
    _CDSConverter *dc;
    int            status;

    /* Check for CDS_STRING types */

    if (in_type == CDS_STRING) {
        if (out_type != CDS_STRING) {
            ERROR( CDS_LIB_NAME,
                "Attempt to convert between strings and numbers in cds_create_converter\n");
            return((CDSConverter)NULL);
        }
    }
    else if (out_type == CDS_STRING) {
        ERROR( CDS_LIB_NAME,
            "Attempt to convert between strings and numbers in cds_create_converter\n");
        return((CDSConverter)NULL);
    }

    /* Initialize the converter */

    dc = (_CDSConverter *)calloc(1, sizeof(_CDSConverter));

    if (!dc) {

        ERROR( CDS_LIB_NAME,
            "Could not create data converter\n"
            " -> memory allocation error\n");

        return((CDSConverter)NULL);
    }

    dc->in_type  = in_type;
    dc->in_size  = cds_data_type_size(in_type);
    dc->out_type = out_type;
    dc->out_size = cds_data_type_size(out_type);

    /* Check if we are doing a units convertion */

    if (in_units) {

        dc->in_units = strdup(in_units);
        if (!dc->in_units) {

            ERROR( CDS_LIB_NAME,
                "Could not create data converter\n"
                " -> memory allocation error\n");

            _cds_destroy_converter(dc);
            return((CDSConverter)NULL);
        }
    }

    if (out_units) {

        dc->out_units = strdup(out_units);
        if (!dc->out_units) {

            ERROR( CDS_LIB_NAME,
                "Could not create data converter\n"
                " -> memory allocation error\n");

            _cds_destroy_converter(dc);
            return((CDSConverter)NULL);
        }
    }

    if (in_units && out_units) {

        status = cds_get_unit_converter(in_units, out_units, &dc->uc);
        if (status < 0) {
            _cds_destroy_converter(dc);
            return((CDSConverter)NULL);
        }

        if (in_type == CDS_STRING && dc->uc) {
            ERROR( CDS_LIB_NAME,
                "Attempt to convert units for string values in cds_create_converter\n");
            _cds_destroy_converter(dc);
            return((CDSConverter)NULL);
        }
    }

    /* Set default range checking values */

    if (!cds_set_converter_range(dc,
        NULL, _cds_data_type_min(out_type),
        NULL, _cds_data_type_max(out_type))) {

        _cds_destroy_converter(dc);
        return((CDSConverter)NULL);
    }

    return((CDSConverter)dc);
}

/**
 *  Create a CDS converter for copying data from an array to a variable.
 *
 *  This function will create a data converter that can be used to convert
 *  an array of data to the same data type, units, and missing values used
 *  by the specified variable.
 *
 *  If there are any missing values specified for the input data but the
 *  output variable does not have any missing or fill values defined, all
 *  missing values will be mapped to the default fill value for the variable's
 *  data type and the variable's default fill value will be set.
 *
 *  If the range of the variable's data type is less than the range of the
 *  input data type, all out-of-range values will be set to the min/max values
 *  of the variable's data type. This default behavior can be changed using the
 *  cds_set_converter_range() function.
 *
 *  The memory used by the returned CDSConverter is dynamically allocated.
 *  It is the responsibility of the calling process to free this memory using
 *  the cds_destroy_converter() function.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  in_type     - data type of the input data
 *  @param  in_units    - units of the input data,
 *                        or NULL for no unit conversion
 *  @param  in_nmissing - number of missing values used in the input data
 *  @param  in_missing  - array of missing values used in the input data
 *  @param  out_var     - pointer to the output variable
 *
 *  @return
 *    - the data converter
 *    - NULL if an error occurred
 */
CDSConverter cds_create_converter_array_to_var(
    CDSDataType  in_type,
    const char  *in_units,
    size_t       in_nmissing,
    void        *in_missing,
    CDSVar      *out_var)
{
    const char   *out_units;
    int           out_nmissing;
    void         *out_missing;
    CDSConverter  converter;
    void         *default_fill;

    out_missing = (void *)NULL;

    /* Create the converter */

    out_units = cds_get_var_units(out_var);
    converter = cds_create_converter(
        in_type, in_units, out_var->type, out_units);

    if (!converter) {

        ERROR( CDS_LIB_NAME,
            "Could not create array-to-var converter for: %s\n",
            cds_get_object_path(out_var));

        return((CDSConverter)NULL);
    }

    /* Set the missing values map if necessary */

    if (!in_nmissing) {
        return(converter);
    }

    out_nmissing = cds_get_var_missing_values(out_var, &out_missing);

    if (out_nmissing < 0) {

        ERROR( CDS_LIB_NAME,
            "Could not create array-to-var converter for: %s\n",
            cds_get_object_path(out_var));

        cds_destroy_converter(converter);
        return((CDSConverter)NULL);
    }

    if (out_nmissing == 0) {

        default_fill = _cds_default_fill_value(out_var->type);

        if (!cds_set_converter_map(converter,
            in_nmissing, in_missing, 1, default_fill)) {

            ERROR( CDS_LIB_NAME,
                "Could not create array-to-var converter for: %s\n",
                cds_get_object_path(out_var));

            cds_destroy_converter(converter);
            return((CDSConverter)NULL);
        }

        if (!cds_set_var_default_fill_value(out_var, default_fill)) {

            ERROR( CDS_LIB_NAME,
                "Could not create array-to-var converter for: %s\n",
                cds_get_object_path(out_var));

            cds_destroy_converter(converter);
            return((CDSConverter)NULL);
        }
    }
    else {

        if (!cds_set_converter_map(converter,
            in_nmissing, in_missing, (size_t)out_nmissing, out_missing)) {

            ERROR( CDS_LIB_NAME,
                "Could not create array-to-var converter for: %s\n",
                cds_get_object_path(out_var));

            cds_free_array(out_var->type, out_nmissing, out_missing);
            cds_destroy_converter(converter);
            return((CDSConverter)NULL);
        }

        cds_free_array(out_var->type, out_nmissing, out_missing);
    }

    return(converter);
}

/**
 *  Create a CDS converter for copying data from a variable to an array.
 *
 *  This function will create a data converter that can be used to convert
 *  the data in a CDS variable to the specified data type, units, and
 *  missing values.
 *
 *  If out_nmissing == 0 and out_missing == NULL, then the missing values
 *  defined for the variable will be mapped to the output data type using the
 *  cds_get_missing_values_map() function.
 *
 *  If out_nmissing == 0 and out_missing != NULL, then the first value in the
 *  array returned by cds_get_missing_values_map() will be used to map all
 *  of the variable's missing values to the output array, and *out_missing
 *  will be set to this value. If the variable does not have any missing values
 *  defined, the value of out_missing will be set to the default fill value
 *  for the output data type.
 *
 *  If the range of the output data type is less than the range of the
 *  variable's data type, all out-of-range values will be set to the min/max
 *  values of the output data type. This default behavior can be changed using
 *  the cds_set_converter_range() function.
 *
 *  The memory used by the returned CDSConverter is dynamically allocated.
 *  It is the responsibility of the calling process to free this memory using
 *  the cds_destroy_converter() function.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  in_var       - pointer to the input variable
 *  @param  out_type     - data type of the output data
 *  @param  out_units    - units of the output data,
 *                         or NULL for no unit conversion
 *  @param  out_nmissing - number of missing values to use in the output data
 *  @param  out_missing  - array of missing values to use in the output data
 *
 *  @return
 *    - the data converter
 *    - NULL if an error occurred
 */
CDSConverter cds_create_converter_var_to_array(
    CDSVar      *in_var,
    CDSDataType  out_type,
    const char  *out_units,
    size_t       out_nmissing,
    void        *out_missing)
{
    const char   *in_units;
    int           in_nmissing;
    void         *in_missing;
    CDSConverter  converter;

    /* Create the converter */

    in_units  = cds_get_var_units(in_var);
    converter = cds_create_converter(
        in_var->type, in_units, out_type, out_units);

    if (!converter) {

        ERROR( CDS_LIB_NAME,
            "Could not create var-to-array converter for: %s\n",
            cds_get_object_path(in_var));

        return((CDSConverter)NULL);
    }

    /* Set the missing values map if necessary */

    in_nmissing = cds_get_var_missing_values(in_var, &in_missing);
    if (in_nmissing < 0) {

        ERROR( CDS_LIB_NAME,
            "Could not create var-to-array converter for: %s\n",
            cds_get_object_path(in_var));

        return((CDSConverter)NULL);
    }

    if (in_nmissing) {

        if (!cds_set_converter_map(converter,
            (size_t)in_nmissing, in_missing, out_nmissing, out_missing)) {

            ERROR( CDS_LIB_NAME,
                "Could not create var-to-array converter for: %s\n",
                cds_get_object_path(in_var));

            cds_free_array(in_var->type, in_nmissing, in_missing);
            cds_destroy_converter(converter);
            return((CDSConverter)NULL);
        }

        cds_free_array(in_var->type, in_nmissing, in_missing);
    }
    else if (!out_nmissing && out_missing) {
        cds_get_default_fill_value(out_type, out_missing);
    }

    return(converter);
}

/**
 *  Create a CDS converter for copying data from one variable to another.
 *
 *  This function will get the data type, units and missing values from the
 *  input variable and then use cds_create_converter_array_to_var() to create
 *  the converter.
 *
 *  The memory used by the returned CDSConverter is dynamically allocated.
 *  It is the responsibility of the calling process to free this memory using
 *  the cds_destroy_converter() function.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  in_var  - pointer to the input variable
 *  @param  out_var - pointer to the output variable
 *
 *  @return
 *    - the data converter
 *    - NULL if an error occurred
 */
CDSConverter cds_create_converter_var_to_var(
    CDSVar *in_var,
    CDSVar *out_var)
{
    const char   *in_units;
    int           in_nmissing;
    void         *in_missing;
    const char   *out_units;
    int           out_nmissing;
    void         *out_missing;
    CDSConverter  converter;

    in_missing  = (void *)NULL;
    out_missing = (void *)NULL;

    /* Create the converter */

    in_units  = cds_get_var_units(in_var);
    out_units = cds_get_var_units(out_var);
    converter = cds_create_converter(
        in_var->type, in_units, out_var->type, out_units);

    if (!converter) {
        goto RETURN_ERROR;
    }

    /* Set the missing values map if necessary */

    in_nmissing = cds_get_var_missing_values(in_var, &in_missing);

    if (in_nmissing < 0) {
        goto RETURN_ERROR;
    }

    if (in_nmissing == 0) {
        return(converter);
    }

    out_nmissing = cds_get_var_missing_values(out_var, &out_missing);

    if (out_nmissing < 0) {
        goto RETURN_ERROR;
    }

    if (out_nmissing == 0) {

        out_missing = cds_get_missing_values_map(
            in_var->type, in_nmissing, in_missing, out_var->type, NULL);

        if (!out_missing) {
            goto RETURN_ERROR;
        }

        out_nmissing = in_nmissing;
    }

    if (!cds_set_converter_map(converter,
        (size_t)in_nmissing, in_missing,
        (size_t)out_nmissing, out_missing)) {

        goto RETURN_ERROR;
    }

    free(in_missing);
    free(out_missing);

//    cds_free_array(in_var->type, in_nmissing, in_missing);
//    cds_free_array(out_var->type, out_nmissing, out_missing);

    return(converter);

RETURN_ERROR:

    if (converter)   cds_destroy_converter(converter);
    if (in_missing)  free(in_missing);
    if (out_missing) free(out_missing);
//    if (in_missing)  cds_free_array(in_var->type, in_nmissing, in_missing);
//    if (out_missing) cds_free_array(out_var->type, out_nmissing, out_missing);

    ERROR( CDS_LIB_NAME,
        "Could not create var-to-var converter\n"
        " -> from: %s\n"
        " -> to:   %s\n",
        cds_get_object_path(in_var),
        cds_get_object_path(out_var));

    return((CDSConverter)NULL);
}

/**
 *  Free all memory used by a CDS data converter.
 *
 *  @param  converter - the data converter
 */
void cds_destroy_converter(CDSConverter converter)
{
    _cds_destroy_converter((_CDSConverter *)converter);
}

/**
 *  Set the mapping values for a CDS data converter.
 *
 *  The mapping values can be used to map missing and fill values from the
 *  input data to the output data without performing the unit conversion or
 *  range checking. All values in the input data mapping array will be replaced
 *  with their corresponding value in the output data mapping array.
 *
 *  If in_map_length == 0 or in_map == NULL, this function will cleanup all
 *  previous mapping values and return successfully.
 *
 *  If out_map_length == 0 and out_map == NULL, then the output map will be
 *  created using cds_get_missing_values_map().
 *
 *  If out_map_length == 0 and out_map != NULL, then the first value in the
 *  array returned by cds_get_missing_values_map() will be used to map all
 *  in_map values to the output array, and *out_map will be set to this value.
 *
 *  If out_map_length < in_map_length, then the output map will be padded
 *  using the first value in the array.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  converter      - the data converter
 *
 *  @param  in_map_length  - number of values in the input data mapping array
 *  @param  in_map         - pointer to the input data mapping array,
 *                           this must be the same data type as the input data
 *
 *  @param  out_map_length - number of values in the output data mapping array
 *  @param  out_map        - pointer to the output data mapping array,
 *                           this must be the same data type as the output data
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int cds_set_converter_map(
    CDSConverter converter,
    size_t       in_map_length,
    void        *in_map,
    size_t       out_map_length,
    void        *out_map)
{
    _CDSConverter *dc         = (_CDSConverter *)converter;
    size_t         in_nbytes  = dc->in_size * in_map_length;
    size_t         out_nbytes = dc->out_size * out_map_length;
    size_t         mi;
    char          *mp;
    int            status;

    /* Cleanup previous values */

    _cds_cleanup_converter_map(dc);

    /* Set input data mapping values in the converter */

    if (!in_map_length || !in_map) {
        return(1);
    }

    if (dc->in_type == CDS_STRING) {
        dc->in_map = cds_copy_string_array(in_map_length, in_map, NULL);
    }
    else {
        dc->in_map = cds_memdup(in_nbytes, in_map);
    }

    if (!dc->in_map) {

        ERROR( CDS_LIB_NAME,
            "Could not set data converter mapping values\n"
            " -> memory allocation error\n");

        return(0);
    }

    dc->map_length = in_map_length;

    /* Set output data mapping values in the converter */

    if (!out_map_length) {

        /* Map input values to output data type */

        dc->out_map = cds_get_missing_values_map(
            dc->in_type, in_map_length, in_map, dc->out_type, NULL);

        if (!dc->out_map) {
            _cds_cleanup_converter_map(dc);
            return(0);
        }

        if (out_map) {

            /* Map all input values to a single output value */

            if (dc->out_type == CDS_STRING) {

                char **strpp = dc->out_map;
                char  *strp  = strpp[0];

                for (mi = 1; mi < in_map_length; mi++) {
                    if (strpp[mi]) free(strpp[mi]);
                    if (strp) {
                        strpp[mi] = strdup(strp);
                        if (strpp[mi]) goto MEMORY_ERROR;
                    }
                    else {
                        strpp[mi] = (char *)NULL;
                    }
                }

                strpp = out_map;

                if (strp) {
                    *strpp = strdup(strp);
                    if (!*strpp) goto MEMORY_ERROR;
                }
                else {
                    *strpp = (char *)NULL;
                }
            }
            else {
                mp = (char *)dc->out_map + dc->out_size;
                for (mi = 1; mi < in_map_length; mi++) {
                    memcpy(mp, dc->out_map, dc->out_size);
                    mp += dc->out_size;
                }
                memcpy(out_map, dc->out_map, dc->out_size);
            }
        }
    }
    else if (out_map_length < in_map_length) {

        /* Pad output data mapping array with first value in the array */

        dc->out_map = calloc(in_map_length, dc->out_size);
        if (!dc->out_map) goto MEMORY_ERROR;

        if (dc->out_type == CDS_STRING) {

            char **strpp1 = (char **)out_map;
            char **strpp2 = (char **)dc->out_map;

            if (!cds_copy_string_array(out_map_length, strpp1, strpp2)) {
                goto MEMORY_ERROR;
            }

            char *strp = strpp1[0];

            for (mi = out_map_length; mi < in_map_length; mi++) {
                strpp2[mi] = strdup(strp);
                if (!strpp2[mi]) goto MEMORY_ERROR;
            }
        }
        else {

            memcpy(dc->out_map, out_map, out_nbytes);

            mp = (char *)dc->out_map + out_nbytes;

            for (mi = out_map_length; mi < in_map_length; mi++) {
                memcpy(mp, out_map, dc->out_size);
                mp += dc->out_size;
            }
        }
    }
    else {

        /* Use specified output data mapping array */

        if (dc->out_type == CDS_STRING) {
            dc->out_map = cds_copy_string_array(out_map_length, (char **)out_map, NULL);
        }
        else {
            dc->out_map = cds_memdup(out_nbytes, out_map);
        }

        if (!dc->out_map) goto MEMORY_ERROR;
    }

    /* Check if the input and output map values are equal */

    status = cds_compare_arrays(
        dc->map_length,
        dc->in_type, dc->in_map,
        dc->out_type, dc->out_map, 0, NULL);

    if (status == 0) {
        dc->map_ident = 1;
    }
    else {
        dc->map_ident = 0;
    }

    return(1);

MEMORY_ERROR:

    ERROR( CDS_LIB_NAME,
        "Could not set data converter mapping values\n"
        " -> memory allocation error\n");
    _cds_cleanup_converter_map(dc);
    return(0);
}

/**
 *  Set the valid data range for a CDS data converter.
 *
 *  The range variables can be used to replace all values outside a specified
 *  range with a less-than-min or a greater-than-max value. If the range of the
 *  output data type is less than the range of the input data type and an
 *  out-of-range value is specified but the corresponding min/max value is not,
 *  the min/max value of the output data type will be used.
 *
 *  @param  converter - the data converter
 *
 *  @param  out_min   - pointer to the minimum value, or NULL to use the
 *                      minimum value of the output data type if the 
 *                      minimum value of the input data type is less than
 *                      the minimum value of the output data type.
 * 
 *  @param  orv_min   - pointer to the value to use for values less than min,
 *                      or NULL to disable the min value check.
 *
 *  @param  out_max   - pointer to the maximum value, or NULL to use the
 *                      maximum value of the output data type if the
 *                      maximum value of the input data type is greater than
 *                      the maximum value of the output data type.
 *
 *  @param  orv_max   - pointer to the value to use for values greater than max,
 *                      or NULL to disable the max value check.
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int cds_set_converter_range(
    CDSConverter  converter,
    void         *out_min,
    void         *orv_min,
    void         *out_max,
    void         *orv_max)
{
    _CDSConverter *dc = (_CDSConverter *)converter;

    /* Cleanup previous values */

    _cds_cleanup_converter_range(dc);

    /* Set new values */

    if (orv_min) {

        if (!out_min) {
            if (_cds_data_type_mincmp(dc->in_type, dc->out_type) < 0) {
                out_min = _cds_data_type_min(dc->out_type);
            }
        }

        if (out_min) {
            if (!(dc->out_min = cds_memdup(dc->out_size, out_min)) ||
                !(dc->orv_min = cds_memdup(dc->out_size, orv_min))) {

                ERROR( CDS_LIB_NAME,
                    "Could not set data converter range\n"
                    " -> memory allocation error\n");

                _cds_cleanup_converter_range(dc);

                return(0);
            }
        }
    }

    if (orv_max) {

        if (!out_max) {
            if (_cds_data_type_maxcmp(dc->in_type, dc->out_type) > 0) {
                out_max = _cds_data_type_max(dc->out_type);
            }
        }

        if (out_max) {
            if (!(dc->out_max = cds_memdup(dc->out_size, out_max)) ||
                !(dc->orv_max = cds_memdup(dc->out_size, orv_max))) {

                ERROR( CDS_LIB_NAME,
                    "Could not set data converter range\n"
                    " -> memory allocation error\n");

                _cds_cleanup_converter_range(dc);

                return(0);
            }
        }
    }

    return(1);
}
