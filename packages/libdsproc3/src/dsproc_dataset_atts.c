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

/** @file dsproc_dataset_atts.c
 *  Dataset Attribute Functions.
 */

#include "dsproc3.h"
#include "dsproc_private.h"

extern DSProc *_DSProc; /**< Internal DSProc structure */

/** @privatesection */

/*******************************************************************************
 *  Static Functions and Data Visible Only To This Module
 */

static size_t _gMaxChunkSize = 4194304; // 4 * 2^20;

/**
 *  Get the chunk size to use for the time dimension.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  dataset - pointer to the CDSGroup
 *
 *  @return
 *    -  chunksize  if successful
 *    -  0          if no times where found in the dataset
 *    - -1          if a memory allocation error occurred
 */
int _dsproc_get_time_chunksize(CDSGroup *dataset)
{
    size_t 	ntimes;
    time_t *times;
    time_t  next_hour;
    int     count;
    int     chunksize;
    size_t  ti;

    /* Get array of sample times */

    times = cds_get_sample_times(dataset, 0, &ntimes, NULL);
    if (!times) {

        if (ntimes == 0) {
            return(0);
        }

        dsproc_set_status(DSPROC_ENOMEM);
        return(-1);
    }

    /* Determine maximum number of times in 1 hour of data */

    next_hour = times[0] + 3600;
    chunksize = 0;
    count     = 0;
    for (ti = 0; ti < ntimes; ++ti) {
        
        if (times[ti] >= next_hour) {
            if (chunksize < count) {
                chunksize = count;
            }
            count = 0;
            next_hour += 3600;
        }
        else {
            count++;
        }
    }

    if (chunksize < count) {
        chunksize = count;
    }

    free(times);

    return(chunksize);
}

/*******************************************************************************
 *  Private Functions Visible Only To This Library
 */

/** @publicsection */

/*******************************************************************************
 *  Internal Functions Visible To The Public
 */

/*******************************************************************************
 *  Public Functions
 */

/**
 *  Change an attribute for a dataset or variable.
 *
 *  This function will define the specified attribute if it does not exist.
 *  If the attribute does exist and the overwrite flag is set, the data type
 *  and value will be changed.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  parent    - pointer to the parent CDSGroup or CDSVar
 *  @param  overwrite - overwrite flag (1 = TRUE, 0 = FALSE)
 *  @param  name      - attribute name
 *  @param  type      - attribute data type
 *  @param  length    - attribute length
 *  @param  value     - pointer to the attribute value
 *
 *  @return
 *    - 1 if successful
 *    - 0 if:
 *        - the parent object is not a group or variable
 *        - the parent group or variable definition is locked
 *        - the attribute definition is locked
 *        - a memory allocation error occurred
 */
int dsproc_change_att(
    void        *parent,
    int          overwrite,
    const char  *name,
    CDSDataType  type,
    size_t       length,
    void        *value)
{
    CDSAtt *att = cds_change_att(
        parent, overwrite, name, type, length, value);

    if (!att) {
        dsproc_set_status(DSPROC_ECDSCHANGEATT);
        return(0);
    }

    return(1);
}

/**
 *  Get an attribute from a dataset or variable.
 *
 *  @param  parent - pointer to the parent CDSGroup or CDSVar
 *  @param  name   - name of the attribute
 *
 *  @return
 *    - pointer to the attribute
 *    - NULL if the attribute does not exist
 */
CDSAtt *dsproc_get_att(
    void        *parent,
    const char  *name)
{
    return(cds_get_att(parent, name));
}

/**
 *  Get a copy of an attribute value from a dataset or variable.
 *
 *  This function will get a copy of an attribute value converted to a
 *  text string. If the data type of the attribute is not CDS_CHAR the
 *  cds_array_to_string() function is used to create the output string.
 *
 *  Memory will be allocated for the returned string if the output string
 *  is NULL. In this case the calling process is responsible for freeing
 *  the allocated memory.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  parent - pointer to the parent CDSGroup or CDSVar
 *  @param  name   - name of the attribute
 *  @param  length - pointer to the length of the output string
 *                     - input:
 *                         - length of the output string
 *                         - ignored if the output string is NULL
 *                     - output:
 *                         - number of characters written to the output string
 *                         - 0 if the attribute value has zero length
 *                         - (size_t)-1 if a memory allocation error occurs
 *  @param  value  - pointer to the output string
 *                   or NULL to dynamically allocate the memory needed.
 *
 *  @return
 *    - pointer to the output string
 *    - NULL if:
 *        - the attribute does not exist or has zero length (length = 0)
 *        - a memory allocation error occurs (length == (size_t)-1)
 */
char *dsproc_get_att_text(
    void       *parent,
    const char *name,
    size_t     *length,
    char       *value)
{
    CDSAtt *att        = cds_get_att(parent, name);
    size_t  tmp_length = 0;

    if (!length) {
        length = &tmp_length;
    }

    if (!att) {
        *length = 0;
        return((char *)NULL);
    }

    value = cds_get_att_text(att, length, value);

    if (*length == (size_t)-1) {
        dsproc_set_status(DSPROC_ENOMEM);
    }

    return(value);
}

/**
 *  Get a copy of an attribute value from a dataset or variable.
 *
 *  This function will get a copy of an attribute value casted into
 *  the specified data type. The functions cds_string_to_array() and
 *  cds_array_to_string() are used to convert between text (CDS_CHAR)
 *  and numeric data types.
 *
 *  Memory will be allocated for the returned array if the output array
 *  is NULL. In this case the calling process is responsible for freeing
 *  the allocated memory.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  parent - pointer to the parent CDSGroup or CDSVar
 *  @param  name   - name of the attribute
 *  @param  type   - data type of the output array
 *  @param  length - pointer to the length of the output array
 *                     - input:
 *                         - length of the output array
 *                         - ignored if the output array is NULL
 *                     - output:
 *                         - number of values written to the output array
 *                         - 0 if the attribute value has zero length
 *                         - (size_t)-1 if a memory allocation error occurs
 *  @param  value  - pointer to the output array
 *                   or NULL to dynamically allocate the memory needed.
 *
 *  @return
 *    - pointer to the output array
 *    - NULL if:
 *        - the attribute value has zero length (length == 0)
 *        - a memory allocation error occurs (length == (size_t)-1)
 */
void *dsproc_get_att_value(
    void         *parent,
    const char   *name,
    CDSDataType   type,
    size_t       *length,
    void         *value)
{
    CDSAtt *att        = cds_get_att(parent, name);
    size_t  tmp_length = 0;

    if (!length) {
        length = &tmp_length;
    }

    if (!att) {
        *length = 0;
        return((void *)NULL);
    }

    value = cds_get_att_value(att, type, length, value);

    if (*length == (size_t)-1) {
        dsproc_set_status(DSPROC_ENOMEM);
    }

    return(value);
}

/**
 *  Set the value of an attribute in a dataset or variable.
 *
 *  This function will define the specified attribute if it does not exist.
 *  If the attribute does exist and the overwrite flag is set, the value will
 *  be set by casting the specified value into the data type of the attribute.
 *  The functions cds_string_to_array() and cds_array_to_string() are used to
 *  convert between text (CDS_CHAR) and numeric data types.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  parent    - pointer to the parent CDSGroup or CDSVar
 *  @param  overwrite - overwrite flag (1 = TRUE, 0 = FALSE)
 *  @param  name      - attribute name
 *  @param  type      - attribute data type
 *  @param  length    - attribute length
 *  @param  value     - pointer to the attribute value
 *
 *  @return
 *    - 1 if successful
 *    - 0 if:
 *        - the parent object is not a group or variable
 *        - the parent group or variable definition is locked
 *        - the attribute definition is locked
 *        - a memory allocation error occurred
 */
int dsproc_set_att(
    void        *parent,
    int          overwrite,
    const char  *name,
    CDSDataType  type,
    size_t       length,
    void        *value)
{
    CDSAtt *att = cds_set_att(
        parent, overwrite, name, type, length, value);

    if (!att) {
        dsproc_set_status(DSPROC_ECDSSETATT);
        return(0);
    }

    return(1);
}

/**
 *  Set the value of an attribute in a dataset or variable.
 *
 *  The cds_string_to_array() function will be used to set the attribute
 *  value if the data type of the attribute is not CDS_CHAR.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  parent - pointer to the parent CDSGroup or CDSVar
 *  @param  name   - name of the attribute
 *  @param  format - format string (see printf)
 *  @param  ...    - arguments for the format string
 *
 *  @return
 *    - 1 if successful
 *    - 0 if:
 *        - the attribute does not exist
 *        - the attribute definition is locked
 *        - a memory allocation error occurred
 */
int dsproc_set_att_text(
    void        *parent,
    const char  *name,
    const char *format, ...)
{
    va_list args;
    char   *string;
    size_t  length;
    int     retval;

    va_start(args, format);
    string = msngr_format_va_list(format, args);
    va_end(args);

    if (!string) {

        ERROR( DSPROC_LIB_NAME,
            "Could not set attribute value for: %s/_atts_/%s\n"
            " -> memory allocation error\n",
            cds_get_object_path(parent), name);

        dsproc_set_status(DSPROC_ENOMEM);
        return(0);
    }

    length = strlen(string) + 1;
    retval = dsproc_set_att_value(parent, name, CDS_CHAR, length, string);

    free(string);

    return(retval);
}

/**
 *  Set the value of an attribute in a dataset or variable.
 *
 *  This function will set the value of an attribute by casting the
 *  specified value into the data type of the attribute. The functions
 *  cds_string_to_array() and cds_array_to_string() are used to convert
 *  between text (CDS_CHAR) and numeric data types.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  parent - pointer to the parent CDSGroup or CDSVar
 *  @param  name   - name of the attribute
 *  @param  type   - data type of the specified value
 *  @param  length - length of the specified value
 *  @param  value  - pointer to the attribute value
 *
 *  @return
 *    - 1 if successful
 *    - 0 if:
 *        - the attribute does not exist
 *        - the attribute definition is locked
 *        - a memory allocation error occurred
 */
int dsproc_set_att_value(
    void        *parent,
    const char  *name,
    CDSDataType  type,
    size_t       length,
    void        *value)
{
    CDSAtt *att = cds_get_att(parent, name);
    int     status;

    if (att) {

        if (att->def_lock) {

            ERROR( DSPROC_LIB_NAME,
                "Could not set attribute value for: %s\n"
                " -> attribute value was defined in the DOD\n",
                cds_get_object_path(att));

            dsproc_set_status(DSPROC_ECDSSETATT);
            return(0);
        }

        status = cds_set_att_value(att, type, length, value);
        if (!status) {
            dsproc_set_status(DSPROC_ENOMEM);
            return(0);
        }
    }
    else {

        ERROR( DSPROC_LIB_NAME,
            "Could not set attribute value for: %s/_atts_/%s\n"
            " -> attribute does not exist\n",
            cds_get_object_path(parent), name);

        dsproc_set_status(DSPROC_ECDSSETATT);
        return(0);
    }

    return(1);
}

/**
 *  Set the value of an attribute if the current value is NULL.
 *
 *  This function will check if the value for the specified attribute is NULL.
 *
 *  If the attribute does not exist or the value is not NULL, nothing will be
 *  done and the function will return successfully.
 *
 *  If the value is NULL, it will be set by casting the specified value into
 *  the data type of the attribute. The functions cds_string_to_array() and
 *  cds_array_to_string() are used to convert between text (CDS_CHAR) and
 *  numeric data types.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  parent - pointer to the parent CDSGroup or CDSVar
 *  @param  name   - name of the attribute
 *  @param  type   - data type of the specified value
 *  @param  length - length of the specified value
 *  @param  value  - pointer to the attribute value
 *
 *  @return
 *    - 1 if successful
 *    - 0 if a memory allocation error occurred
 */
int dsproc_set_att_value_if_null(
    void        *parent,
    const char  *name,
    CDSDataType  type,
    size_t       length,
    void        *value)
{
    CDSAtt *att = cds_get_att(parent, name);
    int     status;

    if (!att) return(1);

    if ( att->length == 0 ||
        (att->type   == CDS_CHAR &&
         att->length == 1 &&
         att->value.cp[0] == '\0')) {

        att->def_lock = 0;

        status = cds_set_att_value(att, type, length, value);
        if (!status) {
            dsproc_set_status(DSPROC_ENOMEM);
            return(0);
        }
    }

    return(1);
}

/**
 *  Set the value of an attribute if the current value is NULL.
 *
 *  This function will check if the value for the specified attribute is NULL.
 *
 *  If the attribute does not exist or the value is not NULL, nothing will be
 *  done and the function will return successfully.
 *
 *  If the value is NULL, it will be set to the specified value. The
 *  cds_string_to_array() function will be used to set the attribute
 *  value if the data type of the attribute is not CDS_CHAR.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  parent - pointer to the parent CDSGroup or CDSVar
 *  @param  name   - name of the attribute
 *  @param  format - format string (see printf)
 *  @param  ...    - arguments for the format string
 *
 *  @return
 *    - 1 if successful
 *    - 0 if a memory allocation error occurred
 */
int dsproc_set_att_text_if_null(
    void        *parent,
    const char  *name,
    const char *format, ...)
{
    CDSAtt *att = cds_get_att(parent, name);
    int     status;
    va_list args;
    char   *string;
    size_t  length;

    if (!att) return(1);

    if ( att->length == 0 ||
        (att->type   == CDS_CHAR &&
         att->length == 1 &&
         att->value.cp[0] == '\0')) {

        att->def_lock = 0;

        va_start(args, format);
        string = msngr_format_va_list(format, args);
        va_end(args);

        if (!string) {

            ERROR( DSPROC_LIB_NAME,
                "Could not set attribute value for: %s/_atts_/%s\n"
                " -> memory allocation error\n",
                cds_get_object_path(parent), name);

            dsproc_set_status(DSPROC_ENOMEM);
            return(0);
        }

        length = strlen(string) + 1;
        status = cds_set_att_value(att, CDS_CHAR, length, string);

        free(string);

        if (!status) {
            dsproc_set_status(DSPROC_ENOMEM);
            return(0);
        }
    }

    return(1);
}

/**
 *  Set the values of all _ChunkSizes attributes that have not been defined.
 *
 *  This function will call dsproc_set_var_chunksizes() for every variable
 *  in a dataset that has one or more dimensions.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  dataset        - pointer to the dataset
 *  @param  time_chunksize - chunk size to use for the time dimension,
 *                           or 0 to compute it.
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int dsproc_set_chunksizes(CDSGroup *dataset, int time_chunksize)
{
    CDSAtt *att;
    CDSVar *var;
    int     nc4_format;
    int     vi;

    /* Check if this is a netcdf4 data model */

    nc4_format = 0;

    att = cds_get_att(dataset, "_Format");
    if (att && att->type == CDS_CHAR) {
        if (strstr(att->value.cp, "netCDF-4")) {
            nc4_format = 1;
        }
    }

    if (!nc4_format) {
        return(1);
    }

    /* Loop over all variables and set _ChunkSizes attributes */

    for (vi = 0; vi < dataset->nvars; ++vi) {

        var = dataset->vars[vi];
        if (var->ndims > 0) {
            if (dsproc_set_var_chunksizes(var, &time_chunksize) < 0) {
                return(0);
            }
        }
    }

    return(1);
}

/**
 *  Set the maximum size of a chunk to use when setting _ChunkSizes.
 *
 *  @param  max_chunksize - maximum size of an uncompressed chunk in bytes
 */
void dsproc_set_max_chunksize(size_t max_chunksize)
{
    _gMaxChunkSize = max_chunksize;
}

/**
 *  Set the _ChunkSizes attribute value for a variable.
 *
 *  This function will use the lengths the variable's dimensions to set the
 *  value for the _ChunkSizes attribute if it has not already been defined.
 *
 *  The time_chunksize argument can be used to specify the size that should
 *  be used for the time dimension. If the specified value is 0, the chunk
 *  size for the time dimension will be computed using the maximum number of
 *  samples per hour as determined from the parent dataset.
 *
 *  If the size of the uncompressed chunk exceeds the maximum allowed size,
 *  the chunk size for the first dimension will be cut in half until the size
 *  of the chunk is within limits. This will continue on to the secondary
 *  dimesnions if necessary. By default the maximum allowed size of an
 *  uncompressed chunk is 4 MiB, this value can be changed using the
 *  cds_set_max_chunksize() function.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  var            - pointer to the CDSVar
 *  @param  time_chunksize - input/output: chunk size to use for the time
 *                           dimension. This will be computed and returned
 *                           if the value is 0.
 *
 *  @return
 *    -  1 if successful
 *    -  0 if this is a dimensionless variable,
 *         the _ChunkSizes attribute value has already been set,
 *         or no time values were found in the parent dataset.
 *    - -1 if an error occurred
 */
int dsproc_set_var_chunksizes(
    CDSVar *var,
    int    *time_chunksize)
{
    CDSGroup *dataset;
    CDSAtt   *att;
    CDSDim   *dim;
    size_t    ndims, di;
    size_t    nbytes;
    int       chunksizes[NC_MAX_DIMS];
    size_t    length = 256;
    char      string[length];
    int       def_lock;

    ndims = var->ndims;

    /* Check for _ChunkSizes attribute */

    att = cds_get_att(var, "_ChunkSizes");

    if (!att) {

        // Check if this variable has an unlimited dimension

        for (di = 0; di < ndims; ++di) {
            dim = var->dims[di];
            if (dim->is_unlimited) {
                break;
            }
        }

        if (di == ndims) {
            // no unlimited dimensions found
            return(0);
        }

        // Create the _ChunkSizes attribute

        def_lock = var->def_lock;
        var->def_lock = 0;
        att = cds_define_att(var, "_ChunkSizes", CDS_CHAR, 0, NULL);
        var->def_lock = def_lock;

        if (!att) {
            dsproc_set_status("Could not define _ChunkSizes attribute");
            return(-1);
        }
    }
    else if (att->length != 0) {
        // _ChunkSizes attribute value has already been set
        return(0);
    }
    else if (att->type != CDS_INT) {

        ERROR( DSPROC_LIB_NAME,
            "Invalid data type for: %s\n"
            " -> data type must be 'int' but the defined type is '%s'\n",
            cds_get_object_path(att),
            cds_data_type_name(att->type));

        dsproc_set_status("Invalid data type for _ChunkSizes attribute");
        return(-1);
    }

    /* Make sure this variable has at least one dimension */

    if (ndims == 0) {

        ERROR( DSPROC_LIB_NAME,
            "Invalid _ChunkSizes attribute found for dimensionless variable: %s\n",
            cds_get_object_path(var));

        dsproc_set_status(
            "Invalid _ChunkSizes attribute found for dimensionless variable");
        return(-1);
    }

    /* Get chunk sizes for each dimension */
    
    nbytes = cds_data_type_size(var->type);

    for (di = 0; di < ndims; ++di) {

        dim = var->dims[di];

        if (strcmp(dim->name, "time") == 0) {

            if (!*time_chunksize) {

                dataset = (CDSGroup *)var->parent;
                *time_chunksize = _dsproc_get_time_chunksize(dataset);
 
                if (*time_chunksize <= 0) {
                    
                    if (*time_chunksize == 0) {
                        return(0);
                    }
                    
                    return(-1);
                }
            }

            chunksizes[di] = *time_chunksize;
        }
        else {
            chunksizes[di] = dim->length;
        }

        nbytes *= chunksizes[di];
    }

    /* Make sure the uncompressed chunk size is less than _gMaxChunkSize */

    di = 0;

    while (nbytes > _gMaxChunkSize) {

        nbytes /= chunksizes[di];
        chunksizes[di] = (int)((chunksizes[di] + 1) / 2);
        nbytes *= chunksizes[di];

        if (chunksizes[di] == 1) {
            di += 1;
            if (di == ndims) {
                break;
            }
        }
    }

    DEBUG_LV1( DSPROC_LIB_NAME,
        "Setting _ChunkSizes for %s =\t[ %s ]\n",
        var->name,
        cds_array_to_string(CDS_INT, ndims, chunksizes, &length, string));

    /* Set _ChunkSizes attribute value */

    if (!cds_set_att_value(att, CDS_INT, ndims, chunksizes)) {
        dsproc_set_status(DSPROC_ECDSSETATT);
        return(-1);
    }

    return(1);
}
