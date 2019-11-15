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
*    $Revision: 70611 $
*    $Author: ermold $
*    $Date: 2016-05-19 19:30:56 +0000 (Thu, 19 May 2016) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/**
 *  @file ncds_get.c
 *  NetCDF Get Functions.
 */

#include "ncds3.h"
#include "ncds_private.h"

/*******************************************************************************
 *  Private Functions
 */
/** @privatesection */

/**
 *  PRIVATE: Get a variable and its data from a NetCDF file.
 *
 *  This function will read in a NetCDF variable and its data into the
 *  specified CDS group. If the variable already exists in the group it
 *  will not be read in again. Likewise, data that already exists in the
 *  variable will not be read in again or overwritten.
 *
 *  All coordinate variables, as specified by the variable’s dimensions,
 *  will also be read in if they do not already exist in the CDS group.
 *  These will be added to the top of the variable list in the order the
 *  dimensions were defined. Coordinate variable data that already exists
 *  will not be read in again or overwritten.
 *
 *  If sample_count is NULL or the value pointed to by sample_count is 0,
 *  the number of available samples will be read.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  nc_grpid         - NetCDF group id
 *  @param  nc_varid         - NetCDF variable id
 *  @param  nc_var_name      - NetCDF variable name
 *  @param  nc_sample_start  - NetCDF variable start sample
 *  @param  sample_count     - NULL or
 *                               - input:  number of samples to read
 *                               - output: number of samples actually read
 *
 *  @param  cds_group        - pointer to the CDS group to store the variable
 *  @param  cds_var_name     - pointer to the CDS variable name, or
 *                             NULL to use the NetCDF variable name.
 *  @param  cds_var_type     - data type to use for the CDS variable, or
 *                             0 (CDS_NAT) for no conversion.
 *  @param  cds_var_units    - units to use for the CDS variable, or
 *                             NULL for no conversion.
 *  @param  cds_sample_start - CDS variable start sample
 *
 *  @param  nmap_dims        - number of dimension names to map from the input
 *                             NetCDF file to the output CDS group.
 *  @param  nc_dim_names     - dimension names in the input NetCDF file.
 *  @param  cds_dim_names    - dimension names in the output CDS group.
 *  @param  cds_dim_types    - data types to use for the coordinate variables,
 *                             or 0 (CDS_NAT) for no conversion.
 *  @param  cds_dim_units    - units to use for the coordinate variables,
 *                             or NULL for no conversion.
 *
 *  @return
 *    - pointer to the variable defined in the CDS group
 *    - NULL if an error occurred
 */
static CDSVar *_ncds_get_var(
    int          nc_grpid,
    int          nc_varid,
    const char  *nc_var_name,
    size_t       nc_sample_start,
    size_t      *sample_count,
    CDSGroup    *cds_group,
    const char  *cds_var_name,
    CDSDataType  cds_var_type,
    const char  *cds_var_units,
    size_t       cds_sample_start,
    int          nmap_dims,
    const char **nc_dim_names,
    const char **cds_dim_names,
    CDSDataType *cds_dim_types,
    const char **cds_dim_units)
{
    CDSVar *var;
    void   *datap;
    int     status;

    if (!cds_var_name) {
        cds_var_name = nc_var_name;
    }

    /* Define the variable in the CDS group if it does not already exist */

    var = cds_get_var(cds_group, cds_var_name);
    if (!var) {

        var = ncds_read_var_def(
            nc_grpid, nc_varid,
            cds_group, cds_var_name,
            nmap_dims, nc_dim_names, cds_dim_names);

        if (!var) return((CDSVar *)NULL);
    }

    /* Read in the coordinate variable data */

    if (!ncds_get_coord_vars(
        nc_grpid, nc_sample_start, sample_count, var, cds_sample_start,
        nmap_dims, nc_dim_names, cds_dim_names, cds_dim_types, cds_dim_units)) {

        return((CDSVar *)NULL);
    }

    /* Set variable data type and units */

    if (cds_var_type || cds_var_units) {

        if (cds_var_units) {

            if (cds_var_type) {
                status = cds_change_var_units(var, cds_var_type, cds_var_units);
            }
            else {
                status = cds_change_var_units(var, var->type, cds_var_units);
            }
        }
        else {
            status = cds_change_var_type(var, cds_var_type);
        }

        if (!status) {
            return((CDSVar *)NULL);
        }
    }

    /* Read in data that has not already been read in */

    if (cds_sample_start >= var->sample_count) {

        datap = ncds_read_var_samples(
            nc_grpid, nc_varid, nc_sample_start, sample_count,
            var, cds_sample_start);

        if (!datap) {
            return((CDSVar *)NULL);
        }
    }

    return(var);
}

/**
 *  PRIVATE: Move a boundary variable to the correct position in the group.
 *
 *  @param  coord_var  - pointer to the coordinate variable
 *  @param  bounds_var - pointer to the boundary variable
 */
static void _ncds_move_bounds_var(CDSVar *coord_var, CDSVar *bounds_var)
{
    CDSGroup *group = (CDSGroup *)coord_var->parent;
    int       v0, vi;

    /* Find the index of the coordinate variable */

    for (v0 = 0; v0 < group->nvars; ++v0) {
        if (group->vars[v0] == coord_var) break;
    }

    if (v0 == group->nvars) return;

    /* Find the index of the boundary variable */

    v0 += 1;

    for (vi = group->nvars - 1; vi > v0; --vi) {
        if (group->vars[vi] == bounds_var)  break;
    }

    if (vi <= v0) return;

    /* Move the boundary variable to the target index */

    for (; vi > v0; vi--) {
        group->vars[vi] = group->vars[vi-1];
    }

    group->vars[v0] = bounds_var;
}

/**
 *  PRIVATE: Move a coordinate variable to the correct position in the group.
 *
 *  @param  coord_var  - pointer to the coordinate variable
 */
static void _ncds_move_coord_var(CDSVar *coord_var)
{
    CDSGroup *group = (CDSGroup *)coord_var->parent;
    CDSVar   *var;
    int       count;
    int       di, v0, vi;

    /* Find the index of the dimension */

    for (di = 0; di < group->ndims; ++di) {
        if (strcmp(group->dims[di]->name, coord_var->name) == 0) {
            break;
        }
    }

    if (di == group->ndims) return;

    /* Determine the index to move the coordinate variable to */

    if (di == 0) {
        v0 = 0;
    }
    else {

        count = 0;

        for (v0 = 0; v0 < group->nvars; ++v0) {

            var = group->vars[v0];
            if (var == coord_var) break;

            if (cds_get_dim(group, var->name)) {
                if (++count > di) break;
            }
            else if (!cds_get_bounds_coord_var(var)) {
                break;
            }
        }
    }

    if (v0 == group->nvars) return;
    if (group->vars[v0] == coord_var) return;

    /* Find the index of the coordinate variable */

    for (vi = group->nvars - 1; vi > v0; vi--) {
        if (group->vars[vi] == coord_var)  break;
    }

    if (vi == v0) return;

    /* Move the coordinate variable to the target index */

    for (; vi > v0; vi--) {
        group->vars[vi] = group->vars[vi-1];
    }

    group->vars[v0] = coord_var;
}

static int _ncds_replace_substring(
    const char *string,
    const char *old_substr,
    const char *new_substr,
    char      **new_string)
{
    size_t  str_len = strlen(string);
    size_t  old_len = strlen(old_substr);
    size_t  new_len = strlen(new_substr);
    size_t  out_len = str_len - old_len + new_len;

    size_t  len;
    char   *strp;
    char   *out_str;

    /* Check if the old substring exists in the specified string */

    strp = strstr(string, old_substr);
    if (!strp) return(0);

    /* Allocate memory for the new string */

    out_str = (char *)malloc((out_len + 1) * sizeof(char));
    if (!out_str) {

        ERROR( NCDS_LIB_NAME,
            "Could not replace substring '%s' in string: '%s'\n"
            " -> memory allocation error\n",
            old_substr, string);

        return(-1);
    }

    /* Create the new string */

    len = strp - string;
    if (len) strncpy(out_str, string, len);

    strcpy(out_str + len, new_substr);

    len += new_len;
    strcpy(out_str + len, strp + old_len);

    *new_string = out_str;
    return(1);
}

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Read in data for a coordinate variable.
 *
 *  If the coordinate variable does not already exist in the dimensions’s
 *  parent group will be added to the top of the variable list in the order
 *  the dimensions were defined. Coordinate variable data that already exists
 *  will not be read in again or overwritten.
 *
 *  If sample_count is NULL or the value pointed to by sample_count is 0,
 *  the number of available samples will be read in.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  nc_grpid         - NetCDF group id
 *  @param  nc_sample_start  - NetCDF variable start sample
 *  @param  sample_count     - NULL to read in all samples or
 *                               - input:  number of samples to read
 *                               - output: number of samples actually read
 *
 *  @param  cds_dim          - pointer to the CDS dimension
 *  @param  cds_sample_start - CDS variable start sample
 *
 *  @param  nmap_dims        - number of dimension names to map from the input
 *                             NetCDF file to the output CDS group.
 *  @param  nc_dim_names     - dimension names in the input NetCDF file.
 *  @param  cds_dim_names    - dimension names in the output CDS group.
 *  @param  cds_dim_types    - data types to use for the coordinate variables,
 *                             or 0 (CDS_NAT) for no conversion.
 *  @param  cds_dim_units    - units to use for the coordinate variables,
 *                             or NULL for no conversion.
 *
 *  @return
 *    -  1 if successful
 *    -  0 if a coordinate variable does not exist for this dimension
 *    - -1 if an error occurred
 */
int ncds_get_coord_var(
    int          nc_grpid,
    size_t       nc_sample_start,
    size_t      *sample_count,
    CDSDim      *cds_dim,
    size_t       cds_sample_start,
    int          nmap_dims,
    const char **nc_dim_names,
    const char **cds_dim_names,
    CDSDataType *cds_dim_types,
    const char **cds_dim_units)
{
    CDSGroup    *group = (CDSGroup *)cds_dim->parent;
    CDSVar      *coord_var;
    int          coord_varid;
    const char  *nc_dim_name;
    CDSDataType  cds_dim_type;
    const char  *cds_dim_unit;
    int          status;
    void        *datap;
    int          mdi;

    CDSAtt      *att;
    CDSVar      *bounds_var;
    int          bounds_varid;
    char        *nc_var_name;
    char        *cds_var_name;
    size_t       length;

    /* Check for a mapped dimension name. */

    nc_dim_name  = cds_dim->name;
    cds_dim_type = 0;
    cds_dim_unit = (const char *)NULL;

    for (mdi = 0; mdi < nmap_dims; mdi++) {

        if (strcmp(cds_dim->name, cds_dim_names[mdi]) == 0) {

            if (nc_dim_names)  nc_dim_name  = nc_dim_names[mdi];
            if (cds_dim_types) cds_dim_type = cds_dim_types[mdi];
            if (cds_dim_units) cds_dim_unit = cds_dim_units[mdi];

            break;
        }
    }

    /* Check if there is a coordinate variable for this dimension */

    status = ncds_inq_varid(nc_grpid, nc_dim_name, &coord_varid);

    if (status == 0) {

        /* Check for time_offset if this is the time dimension */

        if (strcmp(nc_dim_name, "time") == 0) {
            status = ncds_inq_varid(nc_grpid, "time_offset", &coord_varid);
        }
    }

    if (status <= 0) return(status);

    /* Define the coordinate variable in the CDS group
     * if it does not already exist */

    coord_var = cds_get_var(group, cds_dim->name);
    if (!coord_var) {

        coord_var = ncds_read_var_def(
            nc_grpid, coord_varid, group, cds_dim->name,
            nmap_dims, nc_dim_names, cds_dim_names);

        if (!coord_var) return(-1);

        /* Make sure this is a true coordinate variable */

        if (coord_var->ndims != 1 ||
            strcmp(coord_var->dims[0]->name, cds_dim->name) != 0) {

            cds_delete_var(coord_var);
            return(0);
        }

    } /* end if (!coord_var) */

    /* Move the variable to the correct position in the group if we
     * are reading it in for the first time. */

    if (coord_var->sample_count == 0) {
        _ncds_move_coord_var(coord_var);
    }

    /* Check if the NetCDF coordinate variable has a bounds attribute */

    bounds_var = (CDSVar *)NULL;

    length = ncds_get_att_text(nc_grpid, coord_varid, "bounds", &nc_var_name);
    if (length == (size_t)-1) return(-1);
    if (length > 0) {

        /* Check if the NetCDF bounds variable exists */

        status = ncds_inq_varid(nc_grpid, nc_var_name, &bounds_varid);
        if (status < 0) return(-1);
        if (status > 0) {

            /* Get the name of the cds boundary variable. */

            cds_var_name = (char *)nc_var_name;

            if ((strcmp(nc_dim_name, cds_dim->name) != 0) &&
                (strstr(nc_var_name, nc_dim_name)) ) {

                status = _ncds_replace_substring(
                    nc_var_name, nc_dim_name, cds_dim->name, &cds_var_name);

                if (status < 0) return(-1);
            }

            /* Check if the cds boundary variable exists. */

            bounds_var = cds_get_var(group, cds_var_name);
            if (!bounds_var) {

                /* Define the boundary variable in the CDS group
                 * if it does not already exist */

                bounds_var = ncds_read_var_def(
                    nc_grpid, bounds_varid, group, cds_var_name,
                    nmap_dims, nc_dim_names, cds_dim_names);

                if (!bounds_var) return(-1);
            }

            if (cds_var_name != nc_var_name) {

                att = cds_get_att(coord_var, "bounds");
                if (att && att->type == CDS_CHAR) {

                    status = cds_change_att_text(att, "%s", cds_var_name);
                    if (status == 0) return(-1);
                }

                free(cds_var_name);
            }

            /* Move the variable to the correct position in the group */

            _ncds_move_bounds_var(coord_var, bounds_var);
        }

        free(nc_var_name);

    } /* end if has boundary variable */

    /* Set coordinate variable data type and units, this will also
     * set the data type and units for the boundary variable. */

    if (coord_var->sample_count == 0) {

        status = 1;

        if (cds_dim_unit) {

            if (cds_dim_type) {
                status = cds_change_var_units(
                    coord_var, cds_dim_type, cds_dim_unit);
            }
            else {
                status = cds_change_var_units(
                    coord_var, coord_var->type, cds_dim_unit);
            }
        }
        else if (cds_dim_type) {
            status = cds_change_var_type(coord_var, cds_dim_type);
        }

        if (status == 0) {
            cds_delete_var(coord_var);
            return(-1);
        }
    }

    /* Read in coordinate variable data that does not already exist */

    if (cds_sample_start >= coord_var->sample_count) {

        datap = ncds_read_var_samples(
            nc_grpid, coord_varid, nc_sample_start, sample_count,
            coord_var, cds_sample_start);

        if (!datap) return(-1);
    }

    /* Read in boundary variable data that does not already exist */

    if (bounds_var) {

        if (cds_sample_start >= bounds_var->sample_count) {

            datap = ncds_read_var_samples(
                nc_grpid, bounds_varid, nc_sample_start, sample_count,
                bounds_var, cds_sample_start);

            if (!datap) return(-1);
        }
    }

    return(1);
}

/**
 *  Read in coordinate data for a variable.
 *
 *  This function will read in the coordinate variables as specified by a
 *  variable’s dimensions. All coordinate variables that do not already exist
 *  in the variable’s parent group will be added to the top of the variable
 *  list in the order the dimensions were defined. Coordinate variable data
 *  that already exists will not be read in again or overwritten.
 *
 *  The sample start and count values are only used when loading the coordinate
 *  variable data for the first variable dimension. If sample_count is NULL or
 *  the value pointed to by sample_count is 0, the number of available samples
 *  will be read.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  nc_grpid         - NetCDF group id
 *  @param  nc_sample_start  - NetCDF variable start sample
 *  @param  sample_count     - NULL or
 *                               - input:  number of samples to read
 *                               - output: number of samples actually read
 *
 *  @param  cds_var          - pointer to the CDS variable
 *  @param  cds_sample_start - CDS variable start sample
 *
 *  @param  nmap_dims        - number of dimension names to map from the input
 *                             NetCDF file to the output CDS group.
 *  @param  nc_dim_names     - dimension names in the input NetCDF file.
 *  @param  cds_dim_names    - dimension names in the output CDS group.
 *  @param  cds_dim_types    - data types to use for the coordinate variables,
 *                             or 0 (CDS_NAT) for no conversion.
 *  @param  cds_dim_units    - units to use for the coordinate variables,
 *                             or NULL for no conversion.
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int ncds_get_coord_vars(
    int          nc_grpid,
    size_t       nc_sample_start,
    size_t      *sample_count,
    CDSVar      *cds_var,
    size_t       cds_sample_start,
    int          nmap_dims,
    const char **nc_dim_names,
    const char **cds_dim_names,
    CDSDataType *cds_dim_types,
    const char **cds_dim_units)
{
    CDSDim *cds_dim;
    int     status;
    int     di;

    for (di = 0; di < cds_var->ndims; di++) {

        cds_dim = cds_var->dims[di];

        if (di == 0) {

            status = ncds_get_coord_var(
                nc_grpid, nc_sample_start, sample_count,
                cds_dim, cds_sample_start,
                nmap_dims, nc_dim_names, cds_dim_names,
                cds_dim_types, cds_dim_units);
        }
        else {

            status = ncds_get_coord_var(
                nc_grpid, 0, NULL,
                cds_dim, 0,
                nmap_dims, nc_dim_names, cds_dim_names,
                cds_dim_types, cds_dim_units);
        }

        if (status < 0) return(0);
    }

    return(1);
}

/**
 *  Get a variable and its data from a NetCDF file.
 *
 *  This function will read in a NetCDF variable and its data into the
 *  specified CDS group. If the variable already exists in the group it
 *  will not be read in again. Likewise, data that already exists in the
 *  variable will not be read in again or overwritten.
 *
 *  All coordinate variables, as specified by the variable’s dimensions,
 *  will also be read in if they do not already exist in the CDS group.
 *  These will be added to the top of the variable list in the order the
 *  dimensions were defined. Coordinate variable data that already exists
 *  will not be read in again or overwritten.
 *
 *  If sample_count is NULL or the value pointed to by sample_count is 0,
 *  the number of available samples will be read.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  nc_grpid         - NetCDF group id
 *  @param  nc_var_name      - NetCDF variable name
 *  @param  nc_sample_start  - NetCDF variable start sample
 *  @param  sample_count     - NULL or
 *                               - input:  number of samples to read
 *                               - output: number of samples actually read
 *
 *  @param  cds_group        - pointer to the CDS group to store the variable
 *  @param  cds_var_name     - pointer to the CDS variable name, or
 *                             NULL to use the NetCDF variable name.
 *  @param  cds_var_type     - data type to use for the CDS variable, or
 *                             0 (CDS_NAT) for no conversion.
 *  @param  cds_var_units    - units to use for the CDS variable, or
 *                             NULL for no conversion.
 *  @param  cds_sample_start - CDS variable start sample
 *
 *  @param  nmap_dims        - number of dimension names to map from the input
 *                             NetCDF file to the output CDS group.
 *  @param  nc_dim_names     - dimension names in the input NetCDF file.
 *  @param  cds_dim_names    - dimension names in the output CDS group.
 *  @param  cds_dim_types    - data types to use for the coordinate variables,
 *                             or 0 (CDS_NAT) for no conversion.
 *  @param  cds_dim_units    - units to use for the coordinate variables,
 *                             or NULL for no conversion.
 *
 *  @return
 *    - pointer to the variable defined in the CDS group
 *    - (CDSVar *)NULL if the variable was not found
 *    - (CDSVar *)-1 if an error occurred
 */
CDSVar *ncds_get_var(
    int          nc_grpid,
    const char  *nc_var_name,
    size_t       nc_sample_start,
    size_t      *sample_count,
    CDSGroup    *cds_group,
    const char  *cds_var_name,
    CDSDataType  cds_var_type,
    const char  *cds_var_units,
    size_t       cds_sample_start,
    int          nmap_dims,
    const char **nc_dim_names,
    const char **cds_dim_names,
    CDSDataType *cds_dim_types,
    const char **cds_dim_units)
{
    CDSVar *var;
    int     status;
    int     nc_varid;

    status = ncds_inq_varid(nc_grpid, nc_var_name, &nc_varid);

    if (status < 0) {
        return((CDSVar *)-1);
    }

    if (status == 0) {
        return((CDSVar *)NULL);
    }

    var = _ncds_get_var(
        nc_grpid, nc_varid, nc_var_name, nc_sample_start, sample_count,
        cds_group, cds_var_name, cds_var_type, cds_var_units, cds_sample_start,
        nmap_dims, nc_dim_names, cds_dim_names, cds_dim_types, cds_dim_units);

    if (!var) {
        return((CDSVar *)-1);
    }

    return(var);
}

/**
 *  Get a variable and its data from a NetCDF file.
 *
 *  This function will read in a NetCDF variable and its data into the
 *  specified CDS group. If the variable already exists in the group it
 *  will not be read in again. Likewise, data that already exists in the
 *  variable will not be read in again or overwritten.
 *
 *  All coordinate variables, as specified by the variable’s dimensions,
 *  will also be read in if they do not already exist in the CDS group.
 *  These will be added to the top of the variable list in the order the
 *  dimensions were defined. Coordinate variable data that already exists
 *  will not be read in again or overwritten.
 *
 *  If sample_count is NULL or the value pointed to by sample_count is 0,
 *  the number of available samples will be read.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  nc_grpid         - NetCDF group id
 *  @param  nc_varid         - NetCDF variable id
 *  @param  nc_sample_start  - NetCDF variable start sample
 *  @param  sample_count     - NULL or
 *                               - input:  number of samples to read
 *                               - output: number of samples actually read
 *
 *  @param  cds_group        - pointer to the CDS group to store the variable
 *  @param  cds_var_name     - pointer to the CDS variable name, or
 *                             NULL to use the NetCDF variable name.
 *  @param  cds_var_type     - data type to use for the CDS variable, or
 *                             0 (CDS_NAT) for no conversion.
 *  @param  cds_var_units    - units to use for the CDS variable, or
 *                             NULL for no conversion.
 *  @param  cds_sample_start - CDS variable start sample
 *
 *  @param  nmap_dims        - number of dimension names to map from the input
 *                             NetCDF file to the output CDS group.
 *  @param  nc_dim_names     - dimension names in the input NetCDF file.
 *  @param  cds_dim_names    - dimension names in the output CDS group.
 *  @param  cds_dim_types    - data types to use for the coordinate variables,
 *                             or 0 (CDS_NAT) for no conversion.
 *  @param  cds_dim_units    - units to use for the coordinate variables,
 *                             or NULL for no conversion.
 *
 *  @return
 *    - pointer to the variable defined in the CDS group
 *    - NULL if an error occurred
 */
CDSVar *ncds_get_var_by_id(
    int          nc_grpid,
    int          nc_varid,
    size_t       nc_sample_start,
    size_t      *sample_count,
    CDSGroup    *cds_group,
    const char  *cds_var_name,
    CDSDataType  cds_var_type,
    const char  *cds_var_units,
    size_t       cds_sample_start,
    int          nmap_dims,
    const char **nc_dim_names,
    const char **cds_dim_names,
    CDSDataType *cds_dim_types,
    const char **cds_dim_units)
{
    CDSVar *var;
    char    nc_var_name[NC_MAX_NAME + 1];

    if (!ncds_inq_varname(nc_grpid, nc_varid, nc_var_name)) {
        return((CDSVar *)NULL);
    }

    var = _ncds_get_var(
        nc_grpid, nc_varid, nc_var_name, nc_sample_start, sample_count,
        cds_group, cds_var_name, cds_var_type, cds_var_units, cds_sample_start,
        nmap_dims, nc_dim_names, cds_dim_names, cds_dim_types, cds_dim_units);

    return(var);
}
