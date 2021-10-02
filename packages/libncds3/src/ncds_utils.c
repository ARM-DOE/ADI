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

/** @file ncds_utils.c
 *  NCDS Utility Functions.
 */

#include <errno.h>
#include <time.h>

#include "ncds3.h"
#include "ncds_private.h"

/*******************************************************************************
 *  Private Functions
 */
/** @privatesection */

/**
 *  Get dimension information from a NetCDF file.
 *
 *  The output lists are dynamically allocated. It is the responsibility
 *  of the calling process to free the memory used by these lists when
 *  they are no longer needed. The output dim_names list is a list of
 *  dynamically allocated character strings and should be freed using
 *  the ncds_free_list() function.
 *
 *  Any of the output arguments can be NULL to indicate that the
 *  information should not retrieved or returned.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  nc_grpid    - NetCDF group id
 *  @param  ndims       - number of dimensions
 *  @param  dimids      - pointer to the list of dimension IDs
 *  @param  dim_names   - output: pointer to the list of dimension names
 *  @param  dim_lengths - output: pointer to the list of dimension lengths
 *  @param  is_unlimdim - output: pointer to the list of flags indicating if
 *                        the dimension is unlimited (1 == TRUE, 0 == FALSE).
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
static int _ncds_get_dim_info(
    int      nc_grpid,
    int      ndims,
    int     *dimids,
    char  ***dim_names,
    size_t **dim_lengths,
    int    **is_unlimdim)
{
    char  **d_names     = (char **)NULL;
    size_t *d_lengths   = (size_t *)NULL;
    int    *d_is_unlim  = (int *)NULL;
    char    dim_name[NC_MAX_NAME + 1];
    int     nunlimdims;
    int     unlimdimids[NC_MAX_DIMS];
    int     di, udi;

    dim_name[NC_MAX_NAME] = '\0';

    /* Get the dim_names if requested */

    if (dim_names) {

        d_names = (char **)malloc(ndims * sizeof(char *));
        if (!d_names) {
            goto MEMORY_ERROR;
        }

        for (di = 0; di < ndims; di++) {

            if (!ncds_inq_dimname(nc_grpid, dimids[di], dim_name)) {
                goto NETCDF_ERROR;
            }

            d_names[di] = strdup(dim_name);
            if (!d_names[di]) {
                goto MEMORY_ERROR;
            }
        }
    }

    /* Get the dim_lengths if requested */

    if (dim_lengths) {

        d_lengths = (size_t *)malloc(ndims * sizeof(size_t));
        if (!d_lengths) {
            goto MEMORY_ERROR;
        }

        for (di = 0; di < ndims; di++) {
            if (!ncds_inq_dimlen(nc_grpid, dimids[di], &d_lengths[di])) {
                goto NETCDF_ERROR;
            }
        }
    }

    /* Set the is_unlimdim flags if requested */

    if (is_unlimdim) {

        d_is_unlim = (int *)calloc(ndims, sizeof(int));
        if (!d_is_unlim) {
            goto MEMORY_ERROR;
        }

        if (!ncds_inq_unlimdims(nc_grpid, &nunlimdims, unlimdimids)) {
            goto NETCDF_ERROR;
        }

        for (di = 0; di < ndims; di++) {
            for (udi = 0; udi < nunlimdims; udi++) {
                if (dimids[di] == unlimdimids[udi]) {
                    d_is_unlim[di] = 1;
                    break;
                }
            }
        }
    }

    /* Set outputs and return number of dimensions found */

    if (dim_names)   *dim_names   = d_names;
    if (dim_lengths) *dim_lengths = d_lengths;
    if (is_unlimdim) *is_unlimdim = d_is_unlim;

    return(1);

MEMORY_ERROR:

    ERROR( NCDS_LIB_NAME,
        "Could not get netcdf dimension information\n"
        " -> grpid = %d\n"
        " -> memory allocation error\n",
        nc_grpid);

NETCDF_ERROR:

    if (d_names)    ncds_free_list(ndims, d_names);
    if (d_lengths)  free(d_lengths);
    if (d_is_unlim) free(d_is_unlim);

    return(0);
}

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Find an index in an array of time offsets.
 *
 *  @param  ntimes    - number of time offsets
 *  @param  base_time - base time
 *  @param  offsets   - time offsets from base time
 *  @param  ref_time  - reference time for search (in seconds since 1970)
 *  @param  mode      - search mode
 *
 *  Search Modes:
 *
 *    - NCDS_EQ   = find the index of the time that is equal to
 *                  the specified time.
 *
 *    - NCDS_LT   = find the index of the time that is less than
 *                  the specified time.
 *
 *    - NCDS_LTEQ = find the index of the time that is less than
 *                  or equal to the specified time.
 *
 *    - NCDS_GT   = find the index of the time that is greater than
 *                  the specified time.
 *
 *    - NCDS_GTEQ = find the index of the time that is greater than
 *                  or equal to the specified time.
 *
 *  @return
 *    - index of the requested time value
 *    - -1 if not found
 */
int ncds_find_time_index(
    size_t   ntimes,
    time_t   base_time,
    double  *offsets,
    double   ref_time,
    int      mode)
{
    double offset;
    int    bi, ei, hi;

    offset = ref_time - base_time;
    bi     = 0;
    ei     = ntimes-1;

    /* Check end points */

    if (mode & NCDS_EQ) {
        if (offset == offsets[bi]) { return(bi); }
        if (offset == offsets[ei]) { return(ei); }
    }

    if (mode & NCDS_LT) {
        if (offset < offsets[bi]) { return(-1); }
        if (offset > offsets[ei]) { return(ei); }
    }

    if (mode & NCDS_GT) {
        if (offset > offsets[ei]) { return(-1); }
        if (offset < offsets[bi]) { return(bi); }
    }

    /* Find specified time */

    while (1) {

        hi = bi + (ei - bi)/2;

        if (hi == bi) {

            if (mode & NCDS_LT) { return(bi); }
            if (mode & NCDS_GT) { return(ei); }

            return(-1);
        }

        if (offset < offsets[hi]) {
            ei = hi;
            continue;
        }

        if (offset > offsets[hi]) {
            bi = hi;
            continue;
        }

        /* offset == offsets[hi] */

        if (mode & NCDS_EQ) {
            for (; hi > bi; hi--) if (offset != offsets[hi-1]) break;
            return(hi);
        }

        if (mode & NCDS_LT) {
            for (hi--; hi > bi; hi--) if (offset != offsets[hi]) break;
            return(hi);
        }

        if (mode & NCDS_GT) {
            for (hi++; hi < ei; hi++) if (offset != offsets[hi]) break;
            return(hi);
        }
    }

    return(-1);
}

/**
 *  Convert seconds since 1970 to a timestamp.
 *
 *  This function will create a timestamp of the form:
 *
 *      'YYYYMMDD.hhmmss'.
 *
 *  The timestamp argument must be large enough to hold at
 *  least 16 characters (15 plus the null terminator).
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  secs1970  - seconds since 1970
 *  @param  timestamp - output: timestamp string in the form YYYYMMDD.hhmmss
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int ncds_format_timestamp(time_t secs1970, char *timestamp)
{
    struct tm tm_time;

    memset(&tm_time, 0, sizeof(struct tm));

    if (!gmtime_r(&secs1970, &tm_time)) {

        ERROR( NCDS_LIB_NAME,
            "Could not create timestamp for: %u\n"
            " -> %s\n", secs1970, strerror(errno));

        return(0);
    }

    sprintf(timestamp,
        "%04d%02d%02d.%02d%02d%02d",
        tm_time.tm_year + 1900,
        tm_time.tm_mon  + 1,
        tm_time.tm_mday,
        tm_time.tm_hour,
        tm_time.tm_min,
        tm_time.tm_sec);

    return(1);
}

/**
 *  Free a list of character strings.
 *
 *  @param  length - the number of entries in the list
 *  @param  list   - pointer to the list of character strings
 */
void ncds_free_list(int length, char **list)
{
    int i;

    if (list) {

        for (i = 0; i < length; i++) {
            if (list[i]) free(list[i]);
        }

        free(list);
    }
}

/**
 *  Get the value of an attribute.
 *
 *  This function will return the attribute value casted into the
 *  specified data type. The functions cds_string_to_array() and
 *  cds_array_to_string() are used to convert between text (NC_CHAR)
 *  and numeric data types.
 *
 *  The memory used by the output value is dynamically allocated.
 *  It is the responsibility of the calling process to free this
 *  memory when it is no longer needed.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  nc_grpid  - NetCDF group id
 *  @param  nc_varid  - NetCDF variable id (or NC_GLOBAL)
 *  @param  att_name  - name of the attribute
 *  @param  out_type  - data type of the output value
 *  @param  value     - output: pointer to the attribute value.
 *
 *  @return
 *    -  length of the attribute value
 *    -  0 if the attribute does not exist
 *    - (size_t)-1 if an error occurred
 */
size_t ncds_get_att_value(
    int           nc_grpid,
    int           nc_varid,
    const char   *att_name,
    nc_type       out_type,
    void        **value)
{
    nc_type     att_type;
    size_t      att_length;
    int         status;
    CDSDataType cds_in_type;
    CDSDataType cds_out_type;
    size_t      att_type_size;
    size_t      out_type_size;
    size_t      out_length;
    void       *tmp_value;
    char      **strpp;

    *value = (void *)NULL;

    /* Get the type and length of the attribute */

    status = ncds_inq_att(nc_grpid, nc_varid, att_name, &att_type, &att_length);
    if (status <= 0) {
        return(status);
    }

    if (!att_length) {
        return(0);
    }

    /* Convert types to CDS data types */

    cds_in_type = ncds_cds_type(att_type);

    if (cds_in_type == CDS_NAT) {

        ERROR( NCDS_LIB_NAME,
            "Could not get netcdf attribute value for: %s\n"
            " -> nc_grpid = %d, nc_varid = %d\n"
            " -> unsupported netcdf data type (%d)\n",
            att_name, nc_grpid, nc_varid, att_type);

        return(-1);
    }

    cds_out_type = ncds_cds_type(out_type);

    if (cds_out_type == CDS_NAT) {

        ERROR( NCDS_LIB_NAME,
            "Could not get netcdf attribute value for: %s\n"
            " -> nc_grpid = %d, nc_varid = %d\n"
            " -> unsupported netcdf data type (%d)\n",
            att_name, nc_grpid, nc_varid, out_type);

        return(-1);
    }

    /* Get attribute value casted to the requested data type */

    status = NC_NOERR;

    if (cds_out_type == CDS_CHAR) {

        if (cds_in_type == CDS_CHAR) {

            out_length = att_length + 1;
            *value     = calloc(out_length, sizeof(char));

            if (*value) {
                status = nc_get_att(nc_grpid, nc_varid, att_name, *value);
            }
            else {
                out_length = (size_t)-1;
            }
        }
        else {

            att_type_size = cds_data_type_size(cds_in_type);
            tmp_value     = malloc(att_length * att_type_size);

            if (!tmp_value) {
                out_length = (size_t)-1;
            }
            else {

                status = nc_get_att(nc_grpid, nc_varid, att_name, tmp_value);

                if (status == NC_NOERR) {
                    *value = (void *)cds_array_to_string(
                        cds_in_type, att_length, tmp_value, &out_length, NULL);
                }

                if (cds_in_type == CDS_STRING) {
                    nc_free_string(att_length, (char **)tmp_value);
                }
                else {
                    free(tmp_value);
                }
            }
        }
    }
    else if (cds_out_type == CDS_STRING) {

        if (cds_in_type == CDS_STRING) {

            out_length = att_length;
            *value     = calloc(out_length, sizeof(char *));

            if (*value) {
                status = nc_get_att(nc_grpid, nc_varid, att_name, *value);
            }
            else {
                out_length = (size_t)-1;
            }
        }
        else if (cds_in_type == CDS_CHAR) {

            out_length = 1;
            *value     = calloc(1, sizeof(char *));

            if (*value) {

                strpp  = (char **)*value;
                *strpp = calloc(att_length + 1, sizeof(char));

                if (strpp) {
                    status = nc_get_att(nc_grpid, nc_varid, att_name, *strpp);
                }
                else {
                    out_length = (size_t)-1;
                }
            }
            else {
                out_length = (size_t)-1;
            }
        }
        else {
            ERROR( NCDS_LIB_NAME,
                "Could not get netcdf attribute value for: %s\n"
                " -> nc_grpid = %d, nc_varid = %d\n"
                " -> attempt to convert between '%s' and '%s'\n",
                att_name, nc_grpid, nc_varid,
                cds_data_type_name(cds_in_type), cds_data_type_name(cds_out_type));

            *value = (void *)NULL;
            return(-1);
        }
    }
    else if (cds_in_type == CDS_CHAR) {

        out_length = att_length + 1;
        tmp_value  = calloc(out_length, sizeof(char));

        if (tmp_value) {

            status = nc_get_att(nc_grpid, nc_varid, att_name, tmp_value);

            if (status == NC_NOERR) {
                *value = cds_string_to_array(
                    tmp_value, cds_out_type, &out_length, NULL);
            }

            free(tmp_value);
        }
        else {
            out_length = (size_t)-1;
        }
    }
    else if (cds_in_type == CDS_STRING) {
        ERROR( NCDS_LIB_NAME,
            "Could not get netcdf attribute value for: %s\n"
            " -> nc_grpid = %d, nc_varid = %d\n"
            " -> attempt to convert between '%s' and '%s'\n",
            att_name, nc_grpid, nc_varid,
            cds_data_type_name(cds_in_type), cds_data_type_name(cds_out_type));

        *value = (void *)NULL;
        return(-1);
    }
    else {

        out_length    = att_length;
        out_type_size = cds_data_type_size(cds_out_type);
        *value        = malloc(out_length * out_type_size);

        if (*value) {

            switch (cds_out_type) {
                case CDS_BYTE:
                    status = nc_get_att_schar(nc_grpid, nc_varid, att_name, *value);
                    break;
                case CDS_CHAR:
                    status = nc_get_att_text(nc_grpid, nc_varid, att_name, *value);
                    break;
                case CDS_SHORT:
                    status = nc_get_att_short(nc_grpid, nc_varid, att_name, *value);
                    break;
                case CDS_INT:
                    status = nc_get_att_int(nc_grpid, nc_varid, att_name, *value);
                    break;
                case CDS_FLOAT:
                    status = nc_get_att_float(nc_grpid, nc_varid, att_name, *value);
                    break;
                case CDS_DOUBLE:
                    status = nc_get_att_double(nc_grpid, nc_varid, att_name, *value);
                    break;
                /* NetCDF4 extended data types */
                case CDS_INT64:
                    status = nc_get_att_longlong(nc_grpid, nc_varid, att_name, *value);
                    break;
                case CDS_UBYTE:
                    status = nc_get_att_uchar(nc_grpid, nc_varid, att_name, *value);
                    break;
                case CDS_USHORT:
                    status = nc_get_att_ushort(nc_grpid, nc_varid, att_name, *value);
                    break;
                case CDS_UINT:
                    status = nc_get_att_uint(nc_grpid, nc_varid, att_name, *value);
                    break;
                case CDS_UINT64:
                    status = nc_get_att_ulonglong(nc_grpid, nc_varid, att_name, *value);
                    break;
                default:
                    status = NC_EBADTYPE;
                    break;
            }
        }
        else {
            out_length = (size_t)-1;
        }
    }

    if (out_length == (size_t)-1 || status != NC_NOERR) {

        if (out_length == (size_t)-1) {

            ERROR( NCDS_LIB_NAME,
                "Could not get netcdf attribute value for: %s\n"
                " -> nc_grpid = %d, nc_varid = %d\n"
                " -> memory allocation error\n",
                att_name, nc_grpid, nc_varid);
        }
        else if (status != NC_NOERR) {

            ERROR( NCDS_LIB_NAME,
                "Could not get netcdf attribute value for: %s\n"
                " -> nc_grpid = %d, nc_varid = %d\n"
                " -> %s\n",
                att_name, nc_grpid, nc_varid, nc_strerror(status));
        }

        if (*value) free(*value);
        *value = (void *)NULL;

        return(-1);
    }

    return(out_length);
}

/**
 *  Get the value of an attribute.
 *
 *  This function will return the attribute value converted to a
 *  text string if necessary. If the data type of the attribute is
 *  not NC_CHAR the cds_array_to_string() function is used to create
 *  the output string.
 *
 *  The memory used by the output string is dynamically allocated.
 *  It is the responsibility of the calling process to free this
 *  memory when it is no longer needed.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  nc_grpid  - NetCDF group id
 *  @param  nc_varid  - NetCDF variable id (or NC_GLOBAL)
 *  @param  att_name  - name of the attribute
 *  @param  value     - output: pointer to the attribute value
 *
 *  @return
 *    -  length of the string
 *    -  0 if the attribute does not exist
 *    - (size_t)-1 if an error occurred
 */
size_t ncds_get_att_text(
    int           nc_grpid,
    int           nc_varid,
    const char   *att_name,
    char        **value)
{
    return(ncds_get_att_value(
        nc_grpid, nc_varid, att_name, NC_CHAR, (void **)value));
}

/**
 *  Get the coordinate variable associated with a boundary variable.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  nc_grpid     - NetCDF group id
 *  @param  bounds_varid - NetCDF boundary variable id
 *  @param  coord_varid  - output: NetCDF coordinate variable id
 *
 *  @return
 *    -  1 if successful
 *    -  0 if not found
 *    - -1 an error occurred
 */
int ncds_get_bounds_coord_var(
    int  nc_grpid,
    int  bounds_varid,
    int *coord_varid)
{
    char   name[NC_MAX_NAME + 1];
    int    varids[NC_MAX_VARS];
    int    nvars;
    size_t length;
    char  *value;
    int    vi;

    if (!ncds_inq_varname(nc_grpid, bounds_varid, name) ||
        !ncds_inq_varids(nc_grpid, &nvars, varids)) {

        return(-1);
    }

    for (vi = 0; vi < nvars; ++vi) {

        length = ncds_get_att_text(nc_grpid, varids[vi], "bounds", &value);
        if (length == (size_t)-1) {
            return(-1);
        }

        if (length) {
            if (strcmp(value, name) == 0) {
                *coord_varid = varids[vi];
                free(value);
                return(1);
            }
            free(value);
        }
    }

    return(0);
}

/**
 *  Get the boundary variable associated with a coordinate variable.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  nc_grpid     - NetCDF group id
 *  @param  coord_varid  - NetCDF coordinate variable id
 *  @param  bounds_varid - output: NetCDF boundary variable id
 *
 *  @return
 *    -  1 if successful
 *    -  0 if not found
 *    - -1 an error occurred
 */
int ncds_get_bounds_var(
    int  nc_grpid,
    int  coord_varid,
    int *bounds_varid)
{
    size_t length;
    char  *value;
    int    status;

    length = ncds_get_att_text(nc_grpid, coord_varid, "bounds", &value);
    if (length == (size_t)-1) {
        return(-1);
    }

    if (length) {

        status = ncds_inq_varid(nc_grpid, value, bounds_varid);
        free(value);
        return(status);
    }

    return(0);
}

/**
 *  Get dimension information for a NetCDF group.
 *
 *  The output lists are dynamically allocated. It is the responsibility
 *  of the calling process to free the memory used by these lists when
 *  they are no longer needed. The output dim_names list is a list of
 *  dynamically allocated character strings and should be freed using
 *  the ncds_free_list() function.
 *
 *  Any of the output arguments can be NULL to indicate that the
 *  information should not retrieved or returned.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  nc_grpid        - NetCDF group id
 *  @param  include_parents - if non-zero then all the dimensions in all
 *                            parent groups will also be retrieved.
 *  @param  dimids          - output: pointer to the list of dimension IDs
 *  @param  dim_names       - output: pointer to the list of dimension names
 *  @param  dim_lengths     - output: pointer to the list of dimension lengths
 *  @param  is_unlimdim     - output: pointer to the list of flags indicating if
 *                            the dimension is unlimited (1 == TRUE, 0 == FALSE).
 *
 *  @return
 *    - number of dimensions
 *    - -1 if an error occurred
 */
int ncds_get_group_dim_info(
    int      nc_grpid,
    int      include_parents,
    int    **dimids,
    char  ***dim_names,
    size_t **dim_lengths,
    int    **is_unlimdim)
{
    int    *d_ids = (int *)NULL;
    int     ndims;

    /* Initialize outputs */

    if (dimids)      *dimids      = (int *)NULL;
    if (dim_names)   *dim_names   = (char **)NULL;
    if (dim_lengths) *dim_lengths = (size_t *)NULL;
    if (is_unlimdim) *is_unlimdim = (int *)NULL;

    /* Get the number of dimensions */

    if (!ncds_inq_ndims(nc_grpid, &ndims)) {
        return(-1);
    }

    if (ndims == 0) {
        return(0);
    }

    if (!dimids && !dim_names && !dim_lengths && !is_unlimdim) {
        return(ndims);
    }

    /* Get the dimids */

    d_ids = (int *)malloc(ndims * sizeof(int));

    if (!d_ids) {

        ERROR( NCDS_LIB_NAME,
            "Could not get netcdf dimension information\n"
            " -> grpid = %d\n"
            " -> memory allocation error\n",
            nc_grpid);

        return(-1);
    }

    if (!ncds_inq_dimids(nc_grpid, &ndims, d_ids, include_parents)) {
        free(d_ids);
        return(-1);
    }

    /* Get the dim names, lengths, and is_unlimdim flags */

    if (!_ncds_get_dim_info(
        nc_grpid, ndims, d_ids, dim_names, dim_lengths, is_unlimdim)) {

        free(d_ids);
        return(-1);
    }

    /* Set outputs and return number of dimensions found */

    if (dimids) {
        *dimids = d_ids;
    }
    else {
        free(d_ids);
    }

    return(ndims);
}

/**
 *  Get the missing values for a variable in an open NetCDF file.
 *
 *  This function will return an array containing all values specified by
 *  the missing_value and _FillValue attributes, and will be the same data
 *  type as the variable.
 *
 *  The memory used by the returned array is dynamically allocated.
 *  It is the responsibility of the calling process to free this
 *  memory when it is no longer needed.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  nc_grpid  - NetCDF group id
 *  @param  nc_varid  - NetCDF variable id
 *  @param  values    - output: pointer to the array of missing values, the
 *                      data type of this array will be the same as the variable
 *
 *  @return
 *    -   number of missing values
 *    -   0 if there are no missing or fill values
 *    -  -1 if an error occurs
 */
int ncds_get_missing_values(
    int    nc_grpid,
    int    nc_varid,
    void **values)
{
    nc_type      nc_vartype;
    CDSDataType  cds_type;
    int          natts;
    int          attid;
    char         att_name[NC_MAX_NAME + 1];
    void        *att_value;
    size_t       att_length;
    void        *mvp;
    int          free_att_value;
    size_t       type_size;
    int          nvalues;
    int          found_fill;
    const char  *strp;

    *values     = (void *)NULL;
    nvalues     = 0;
    found_fill  = 0;

    /* Get the netcdf variable data type */

    if (!ncds_inq_vartype(nc_grpid, nc_varid, &nc_vartype)) {
        return(-1);
    }

    cds_type = ncds_cds_type(nc_vartype);
    if (cds_type == CDS_NAT) {

        ERROR( NCDS_LIB_NAME,
            "Could not get missing values for netcdf variable\n"
            " -> nc_grpid = %d, nc_varid = %d\n"
            " -> unsupported netcdf data type (%d)\n",
            nc_grpid, nc_varid, nc_vartype);

        return(-1);
    }

    type_size = cds_data_type_size(cds_type);

    /* Search for all variations of the missing value attribute
     * at the field level. */

    if (!ncds_inq_varnatts(nc_grpid, nc_varid, &natts)) {
        return(-1);
    }

    for (attid = 0; attid < natts; attid++) {

        if (!ncds_inq_attname(nc_grpid, nc_varid, attid, att_name)) {
            return(-1);
        }

        if (!cds_is_missing_value_att_name(att_name)) {
            continue;
        }

        if (strcmp(att_name, "_FillValue") == 0) {
            found_fill = 1;
        }

        att_length = ncds_get_att_value(nc_grpid, nc_varid,
            att_name, nc_vartype, &att_value);

        if (att_length == (size_t)-1) return(-1);
        if (att_length == 0) continue;

        *values = realloc(*values, (nvalues + att_length) * type_size);
        if (!*values) goto MEMORY_ERROR;

        mvp = (char *)(*values) + (nvalues * type_size);
        memcpy(mvp, att_value, (att_length * type_size));

        free(att_value);

        nvalues += att_length;
    }

    /* If a missing value attribute was not found, search again
     * at the global attribute level. */

    if (!nvalues) {

        if (!ncds_inq_natts(nc_grpid, &natts)) {
            return(-1);
        }

        for (attid = 0; attid < natts; attid++) {

            if (!ncds_inq_attname(nc_grpid, NC_GLOBAL, attid, att_name)) {
                return(-1);
            }

            if (!cds_is_missing_value_att_name(att_name)) {
                continue;
            }

            att_length = ncds_get_att_value(nc_grpid, NC_GLOBAL,
                att_name, nc_vartype, &att_value);

            if (att_length == (size_t)-1) return(-1);
            if (att_length == 0) continue;

            *values = realloc(*values, (nvalues + att_length) * type_size);
            if (!*values) goto MEMORY_ERROR;

            mvp = (char *)(*values) + (nvalues * type_size);
            memcpy(mvp, att_value, (att_length * type_size));

            free(att_value);

            nvalues += att_length;
        }
    }

    /* Get the default fill value if the _FillValues
     * attribute was not explicitly defined. */

    if (!found_fill) {

        att_length = ncds_get_att_value(nc_grpid, nc_varid,
            "_FillValue", nc_vartype, &att_value);

        if (att_length == (size_t)-1) {
            return(-1);
        }

        if (att_length == 0) {
            att_length     = 1;
            free_att_value = 0;
            if (nc_vartype == NC_STRING) {
                strp = strdup(_ncds_default_fill_value(nc_vartype));
                if (!strp) goto MEMORY_ERROR;
                att_value = &strp;
            }
            else {
                att_value = _ncds_default_fill_value(nc_vartype);
            }
        }
        else {
            free_att_value = 1;
        }

        /* Append fill values to the end of the missing values array */

        *values = realloc(*values, (nvalues + att_length) * type_size);
        if (!*values) goto MEMORY_ERROR;

        mvp = (char *)(*values) + (nvalues * type_size);
        memcpy(mvp, att_value, (att_length * type_size));

        if (free_att_value) free(att_value);

        nvalues += att_length;
    }

    return(nvalues);

MEMORY_ERROR:

    if (*values) free(*values);

    *values = (void *)NULL;

    ERROR( NCDS_LIB_NAME,
        "Could not get missing values for netcdf variable\n"
        " -> nc_grpid = %d, nc_varid = %d\n"
        " -> memory allocation error\n",
        nc_grpid, nc_varid);

    return(-1);

}

/**
 *  Get time dimension and coordinate variable information for a NetCDF group.
 *
 *  Any of the output arguments can be NULL to indicate that the
 *  information should not retrieved or returned.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  nc_grpid     - NetCDF group id
 *  @param  time_dimid   - output: time dimension id
 *  @param  time_varid   - output: time variable id
 *  @param  num_times    - output: number of times
 *  @param  base_time    - output: base time
 *  @param  start_offset - output: start time offset from base time
 *  @param  end_offset   - output: end time offset from base time
 *
 *  @return
 *    -  1 if successful
 *    -  0 if the time dimension doesn't exist
 *    - -1 if an error occurred
 */
int ncds_get_time_info(
    int     nc_grpid,
    int    *time_dimid,
    int    *time_varid,
    size_t *num_times,
    time_t *base_time,
    double *start_offset,
    double *end_offset)
{
    int       status;
    int       dimid;
    int       varid;
    size_t    ntimes;
    char     *time_varname;
    time_t    secs1970;
    char     *time_units;
    int       use_to_var;
    int       bt_varid;
    double    bt_double;
    size_t    index;

    /* Initialize outputs */

    if (time_dimid)   *time_dimid   = 0;
    if (time_varid)   *time_varid   = 0;
    if (num_times)    *num_times    = 0;
    if (base_time)    *base_time    = 0;
    if (start_offset) *start_offset = 0;
    if (end_offset)   *end_offset   = 0;

    /* Get the id of the time dimension */

    status = ncds_inq_dimid(nc_grpid, "time", &dimid);

    if (status <= 0) {
        return(status);
    }

    if (time_dimid) *time_dimid = dimid;

    /* Get the length of the time dimension */

    if (!ncds_inq_dimlen(nc_grpid, dimid, &ntimes)) {
        return(-1);
    }

    if (num_times) *num_times = ntimes;

    /* Get the id of the time dimension's coordinate variable */

    secs1970     = -1;
    use_to_var   = 0;
    time_varname = "time";
    time_units   = (char *)NULL;
    status       = ncds_inq_varid(nc_grpid, time_varname, &varid);

    if (status  < 0) return(-1);
    if (status == 1) {

        /* Verify that the time variable has valid units */

        secs1970 = ncds_get_var_time_units(nc_grpid, varid, &time_units);

        if (secs1970  < -1) return(-1);
        if (secs1970 == -1) use_to_var = 1;
    }

    if (status == 0 ||
        use_to_var) {

        /* Look for the time_offset variable instead */

        time_varname = "time_offset";
        status       = ncds_inq_varid(nc_grpid, time_varname, &varid);

        if (status < 0) {
            if (time_units) free(time_units);
            return(-1);
        }

        if (status == 0) {

            if (use_to_var) {

                if (time_units) {

                    ERROR( NCDS_LIB_NAME,
                        "Invalid netcdf time variable units format: '%s'\n"
                        " -> nc_grpid = %d, nc_varid = %d\n",
                        time_units, nc_grpid, varid);
                }
                else {

                    ERROR( NCDS_LIB_NAME,
                        "Units attribute for time variable does not exist\n"
                        " -> nc_grpid = %d, nc_varid = %d\n",
                        nc_grpid, varid);
                }
            }
            else {

                ERROR( NCDS_LIB_NAME,
                    "Coordinate variable for time dimension does not exist\n"
                    " -> nc_grpid = %d\n", nc_grpid);
            }

            if (time_units) free(time_units);
            return(-1);
        }

        /* Check if the time_offset variable has valid units */

        if (time_units) {
            free(time_units);
            time_units = (char *)NULL;
        }

        secs1970 = ncds_get_var_time_units(nc_grpid, varid, &time_units);

        if (secs1970 < -1) {
            if (time_units) free(time_units);
            return(-1);
        }
    }

    if (time_varid) *time_varid = varid;

    /* Get the base_time if it was requested */

    if (base_time) {

        if (secs1970 > -1) {
            *base_time = secs1970;
        }
        else {

            /* Check for a base_time variable */

            status = ncds_inq_varid(nc_grpid, "base_time", &bt_varid);

            if (status  < 0) return(-1);
            if (status == 0) {

                if (time_units) {

                    ERROR( NCDS_LIB_NAME,
                        "Invalid netcdf %s variable units format: '%s'\n"
                        " -> nc_grpid = %d, nc_varid = %d\n",
                        time_varname, time_units, nc_grpid, varid);

                    free(time_units);
                }
                else {

                    ERROR( NCDS_LIB_NAME,
                        "Units attribute for %s variable does not exist\n"
                        " -> nc_grpid = %d, nc_varid = %d\n",
                        time_varname, nc_grpid, varid);
                }

                return(-1);
            }

            /* Get the base time from the base_time variable */

            index  = 0;
            status = nc_get_var1_double(nc_grpid, bt_varid, &index, &bt_double);

            if (status != NC_NOERR) {

                ERROR( NCDS_LIB_NAME,
                    "Could not read base_time variable data\n"
                    " -> nc_grpid = %d, nc_varid = %d\n",
                    " -> %s\n",
                    nc_grpid, varid, nc_strerror(status));

                if (time_units) free(time_units);
                return(-1);
            }

            *base_time = (time_t)bt_double;
        }

    } /* end get base_time */

    if (time_units) free(time_units);

    /* Get start_offset if it was requested */

    if (ntimes && start_offset) {

        index  = 0;
        status = nc_get_var1_double(nc_grpid, varid, &index, start_offset);

        if (status != NC_NOERR) {

            ERROR( NCDS_LIB_NAME,
                "Could not read %s variable data\n"
                " -> nc_grpid = %d, nc_varid = %d\n",
                " -> %s\n",
                time_varname, nc_grpid, varid, nc_strerror(status));

            return(-1);
        }

    } /* end get start_offset */

    /* Get end_offset if it was requested */

    if (ntimes && end_offset) {

        index  = ntimes - 1;
        status = nc_get_var1_double(nc_grpid, varid, &index, end_offset);

        if (status != NC_NOERR) {

            ERROR( NCDS_LIB_NAME,
                "Could not read %s variable data\n"
                " -> nc_grpid = %d, nc_varid = %d\n",
                " -> %s\n",
                time_varname, nc_grpid, varid, nc_strerror(status));

            return(-1);
        }

    } /* end get end_offset */

    return(1);
}

/**
 *  Get the base time and time offsets for a NetCDF group.
 *
 *  The output array is dynamically allocated. It is the responsibility
 *  of the calling process to free this memory when it is no longer needed.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  nc_grpid  - NetCDF group id
 *  @param  start     - start record index
 *  @param  count     - number of times to read in (0 for all)
 *  @param  base_time - output: base time
 *  @param  offsets   - output: pointer to the array of time offsets
 *
 *  @return
 *    - number of times in output array
 *    - -1 if an error occurred
 */
int ncds_get_time_offsets(
    int      nc_grpid,
    size_t   start,
    size_t   count,
    time_t  *base_time,
    double **offsets)
{
    int    status;
    int    time_dimid;
    int    time_varid;
    size_t ntimes;

    /* Initialize output */

    *base_time = 0;
    *offsets   = (double *)NULL;

    /* Get time dimension and coordinate variable info */

    status = ncds_get_time_info(
        nc_grpid, &time_dimid, &time_varid, &ntimes, base_time, NULL, NULL);

    if (status <= 0) {
        return(status);
    }

    if (ntimes == 0) return(0);

    /* Read in the time offsets */

    if (count == 0) {
        count = ntimes - start;
    }

    *offsets = (double *)malloc(count * sizeof(double));

    if (!(*offsets)) {

        ERROR( NCDS_LIB_NAME,
            "Could not get netcdf time offsets\n"
            " -> nc_grpid = %d\n"
            " -> memory allocation error\n", nc_grpid);

        *base_time = 0;

        return(-1);
    }

    status = nc_get_vara_double(
        nc_grpid, time_varid, &start, &count, (double *)(*offsets));

    if (status != NC_NOERR) {

        ERROR( NCDS_LIB_NAME,
            "Could not get netcdf time offsets\n"
            " -> nc_grpid = %d\n"
            " -> %s\n",
            nc_grpid, nc_strerror(status));

        free(*offsets);

        *base_time = 0;
        *offsets   = (double *)NULL;

        return(-1);
    }

    return((int)count);
}

/**
 *  Get the timevals for a NetCDF group.
 *
 *  The output array is dynamically allocated. It is the responsibility
 *  of the calling process to free this memory when it is no longer needed.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  nc_grpid  - NetCDF group id
 *  @param  start     - start record index
 *  @param  count     - number of times to read in (0 for all)
 *  @param  timevals  - output: pointer to the array of timevals
 *
 *  @return
 *    - number of times in output array
 *    - -1 if an error occurred
 */
int ncds_get_timevals(
    int         nc_grpid,
    size_t      start,
    size_t      count,
    timeval_t **timevals)
{
    int     ntimes;
    time_t  base_time;
    double *offsets;

    /* Initialize output */

    *timevals = (timeval_t *)NULL;

    /* Read in the time offsets */

    ntimes = ncds_get_time_offsets(
        nc_grpid, start, count, &base_time, &offsets);

    if (ntimes <= 0) {
        return(ntimes);
    }

    *timevals = cds_offsets_to_timevals(
        CDS_DOUBLE, ntimes, base_time, offsets, NULL);

    free(offsets);

    if (!(*timevals)) {

        ERROR( NCDS_LIB_NAME,
            "Could not get netcdf time values\n"
            " -> nc_grpid = %d\n"
            " -> memory allocation error\n", nc_grpid);

        return(-1);
    }

    return(ntimes);
}

/**
 *  Get dimension information for a NetCDF variable.
 *
 *  The output lists are dynamically allocated. It is the responsibility
 *  of the calling process to free the memory used by these lists when
 *  they are no longer needed. The output dim_names list is a list of
 *  dynamically allocated character strings and should be freed using
 *  the ncds_free_list() function.
 *
 *  Any of the output arguments can be NULL to indicate that the
 *  information should not retrieved or returned.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  nc_grpid    - NetCDF group id
 *  @param  nc_varid    - NetCDF variable id
 *  @param  dimids      - output: pointer to the list of dimension IDs
 *  @param  dim_names   - output: pointer to the list of dimension names
 *  @param  dim_lengths - output: pointer to the list of dimension lengths
 *  @param  is_unlimdim - output: pointer to the list of flags indicating if
 *                        the dimension is unlimited (1 == TRUE, 0 == FALSE).
 *
 *  @return
 *    - number of variable dimensions
 *    - -1 if an error occurred
 */
int ncds_get_var_dim_info(
    int      nc_grpid,
    int      nc_varid,
    int    **dimids,
    char  ***dim_names,
    size_t **dim_lengths,
    int    **is_unlimdim)
{
    int    *d_ids = (int *)NULL;
    int     ndims;

    /* Initialize outputs */

    if (dimids)      *dimids      = (int *)NULL;
    if (dim_names)   *dim_names   = (char **)NULL;
    if (dim_lengths) *dim_lengths = (size_t *)NULL;
    if (is_unlimdim) *is_unlimdim = (int *)NULL;

    /* Get the number of dimensions */

    if (!ncds_inq_varndims(nc_grpid, nc_varid, &ndims)) {
        return(-1);
    }

    if (ndims == 0) {
        return(0);
    }

    if (!dimids && !dim_names && !dim_lengths && !is_unlimdim) {
        return(ndims);
    }

    /* Get the dimids */

    d_ids = (int *)malloc(ndims * sizeof(int));

    if (!d_ids) {

        ERROR( NCDS_LIB_NAME,
            "Could not get netcdf variable dimension information\n"
            " -> grpid = %d, varid = %d\n"
            " -> memory allocation error\n",
            nc_grpid, nc_varid);

        return(-1);
    }

    if (!ncds_inq_vardimids(nc_grpid, nc_varid, d_ids)) {
        free(d_ids);
        return(-1);
    }

    /* Get the dim names, lengths, and is_unlimdim flags */

    if (!_ncds_get_dim_info(
        nc_grpid, ndims, d_ids, dim_names, dim_lengths, is_unlimdim)) {

        free(d_ids);
        return(-1);
    }

    /* Set outputs and return number of dimensions found */

    if (dimids) {
        *dimids = d_ids;
    }
    else {
        free(d_ids);
    }

    return(ndims);
}

/**
 *  Get the value of a time variable's units attribute.
 *
 *  This function will get the units attribute of the specified variable
 *  and use cds_validate_time_units() to verify that the units string has
 *  the correct format (and fix it if possible).
 *
 *  The output units string is dynamically allocated. It is the responsibility
 *  of the calling process to free this memory when it is no longer needed.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  nc_grpid  - NetCDF group id
 *  @param  nc_varid  - NetCDF variable id
 *  @param  units     - output: pointer to the units string, or NULL if this
 *                      string should not be returned.
 *
 *  @return
 *    -  time in seconds since 1970
 *    - -1 if:
 *           - the units attribute does not exist or is not a character string.
 *             (units == NULL)
 *           - the time units string is not valid and could not be fixed.
 *             (units == invalid units string)
 *    - -2 if an error occurred
 */
time_t ncds_get_var_time_units(
    int    nc_grpid,
    int    nc_varid,
    char **units)
{
    int     status;
    time_t  base_time;
    char   *unitsp;

    if (units) *units = (char *)NULL;

    status = ncds_get_var_units(nc_grpid, nc_varid, &unitsp);

    if (status <= 0) {
        return(status - 1);
    }

    base_time = cds_validate_time_units(unitsp);

    if (base_time < 0) {
        free(unitsp);
        return(base_time);
    }

    if (units) {
        *units = unitsp;
    }
    else {
        free(unitsp);
    }

    return(base_time);
}

/**
 *  Get the value of a variable's units attribute.
 *
 *  The output units string is dynamically allocated. It is the responsibility
 *  of the calling process to free this memory when it is no longer needed.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  nc_grpid  - NetCDF group id
 *  @param  nc_varid  - NetCDF variable id
 *  @param  units     - output: pointer to the units string
 *
 *  @return
 *    -  1 if successful
 *    -  0 if the units attribute does not exist or is not a text attribute.
 *    - -1 if an error occurred
 */
int ncds_get_var_units(
    int    nc_grpid,
    int    nc_varid,
    char **units)
{
    int      status;
    nc_type  att_type;
    size_t   att_length;

    *units = (char *)NULL;

    /* Get the units attribute type and length */

    status = ncds_inq_att(nc_grpid, nc_varid, "units", &att_type, &att_length);

    if (status == 0) {

        /* Check if this is a bounds variable */

        status = ncds_get_bounds_coord_var(nc_grpid, nc_varid, &nc_varid);

        if (status > 0) {
            status = ncds_inq_att(
                nc_grpid, nc_varid, "units", &att_type, &att_length);
        }
    }

    if (status <= 0) {
        return(status);
    }

    if (att_type != NC_CHAR) {
        return(0);
    }

    /* Get the units string */

    *units = (char *)calloc((att_length + 1), sizeof(char));

    if (!(*units)) {

        ERROR( NCDS_LIB_NAME,
            "Could not get netcdf variable units\n"
            " -> grpid = %d, varid = %d\n"
            " -> memory allocation error\n",
            nc_grpid, nc_varid);

        return(-1);
    }

    status = nc_get_att_text(nc_grpid, nc_varid, "units", *units);

    if (status != NC_NOERR) {

        ERROR( NCDS_LIB_NAME,
            "Could not get netcdf variable units\n"
            " -> grpid = %d, varid = %d\n"
            " -> %s\n",
            nc_grpid, nc_varid, nc_strerror(status));

        free(*units);
        *units = (char *)NULL;
        return(-1);
    }

    return(1);
}
