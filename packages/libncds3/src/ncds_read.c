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

/**
 *  @file ncds_read.c
 *  NetCDF Read Functions.
 */

#include "ncds3.h"
#include "ncds_private.h"

/*******************************************************************************
 *  Private Functions
 */
/** @privatesection */

/**
 *  PRIVATE: Read an attribute definition from a NetCDF group or variable.
 *
 *  For global attributes, specify NC_GLOBAL for the variable id.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  nc_grpid     - NetCDF group id
 *  @param  nc_varid     - NetCDF variable id
 *  @param  nc_attid     - NetCDF attribute id
 *  @param  cds_object   - pointer to the CDS variable or group
 *  @param  cds_att_name - attribute name to use in the CDS object,
 *                         or NULL to use the name of the NetCDF attribute.
 *
 *  @return
 *    - pointer to the CDS attribute defined in the CDS group or variable
 *    - NULL if:
 *        - a NetCDF or CDS error occurred
 *        - the NetCDF data type is not a supported CDS data type
 */
CDSAtt *_ncds_read_att(
    int         nc_grpid,
    int         nc_varid,
    int         nc_attid,
    void       *cds_object,
    const char *cds_att_name)
{
    CDSAtt     *att;
    int         status;
    char        att_name[NC_MAX_NAME + 1];
    nc_type     nctype;
    size_t      length;
    CDSDataType cds_type;

    /* Get attribute name */

    if (!ncds_inq_attname(nc_grpid, nc_varid, nc_attid, att_name)) {
        return((CDSAtt *)NULL);
    }

    /* Get attribute type and length */

    if (!ncds_inq_att(nc_grpid, nc_varid, att_name, &nctype, &length)) {
        return((CDSAtt *)NULL);
    }

    /* Get the CDS data type corresponding to the NetCDF data type */

    cds_type = ncds_cds_type(nctype);
    if (cds_type == CDS_NAT) {

        ERROR( NCDS_LIB_NAME,
            "Could not get attribute definition\n"
            " -> nc_grpid = %d, nc_varid = %d, att_name = '%s'\n"
            " -> unsupported netcdf data type (%d)\n",
            nc_grpid, nc_varid, att_name, nctype);

        return((CDSAtt *)NULL);
    }

    /* Define the attribute in the CDS object.
     * This will also allocate memory for the value...
     */

    if (!cds_att_name) {
        cds_att_name = att_name;
    }

    att = cds_define_att(cds_object, cds_att_name, cds_type, length, NULL);
    if (!att) {
        return((CDSAtt *)NULL);
    }

    /* Get the attribute value */

    switch (cds_type) {
        case CDS_BYTE:
            status = nc_get_att_schar(nc_grpid, nc_varid, att_name, att->value.bp);
            break;
        case CDS_CHAR:
            status = nc_get_att_text(nc_grpid, nc_varid, att_name, att->value.cp);
            break;
        case CDS_SHORT:
            status = nc_get_att_short(nc_grpid, nc_varid, att_name, att->value.sp);
            break;
        case CDS_INT:
            status = nc_get_att_int(nc_grpid, nc_varid, att_name, att->value.ip);
            break;
        case CDS_FLOAT:
            status = nc_get_att_float(nc_grpid, nc_varid, att_name, att->value.fp);
            break;
        case CDS_DOUBLE:
            status = nc_get_att_double(nc_grpid, nc_varid, att_name, att->value.dp);
            break;
        /* NetCDF4 extended data types */
        case CDS_INT64:
            status = nc_get_att_longlong(nc_grpid, nc_varid, att_name, att->value.i64p);
            break;
        case CDS_UBYTE:
            status = nc_get_att_uchar(nc_grpid, nc_varid, att_name, att->value.ubp);
            break;
        case CDS_USHORT:
            status = nc_get_att_ushort(nc_grpid, nc_varid, att_name, att->value.usp);
            break;
        case CDS_UINT:
            status = nc_get_att_uint(nc_grpid, nc_varid, att_name, att->value.uip);
            break;
        case CDS_UINT64:
            status = nc_get_att_ulonglong(nc_grpid, nc_varid, att_name, att->value.ui64p);
            break;
        case CDS_STRING:
            status = nc_get_att_string(nc_grpid, nc_varid, att_name, att->value.strp);
            break;

        default:
            status = NC_EBADTYPE;
            break;
    }

    if (status != NC_NOERR) {

        ERROR( NCDS_LIB_NAME,
            "Could not get attribute value\n"
            " -> nc_grpid = %d, nc_varid = %d, att_name = '%s'\n"
            " -> %s\n",
            nc_grpid, nc_varid, att_name, nc_strerror(status));

        cds_delete_att(att);
        return((CDSAtt *)NULL);
    }

    return(att);
}

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Read the contents of a NetCDF file into a new CDS group.
 *
 *  This function will read the entire contents of a NetCDF file into a new
 *  CDS group. The calling process is responsible for freeing the CDS group
 *  (see cds_delete_group()).
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  nc_file     - full path to the NetCDF file
 *  @param  recursive   - recurse into all subroups (1 = TRUE, 0 = FALSE)
 *  @param  header_only - only read in the NetCDF header (1 = TRUE, 0 = FALSE)
 *  @param  nc_format   - output: format of the NetCDF file:
 *
 *                          - NC_FORMAT_CLASSIC
 *                          - NC_FORMAT_64BIT
 *                          - NC_FORMAT_NETCDF4
 *                          - NC_FORMAT_NETCDF4_CLASSIC
 *
 *  @param  cds_group   - pointer to the CDS group to create the new
 *                        group in, or NULL to create a root group.
 *
 *  @return
 *    - pointer to the new CDS group
 *    - NULL if a NetCDF or CDS error occurred
 */
CDSGroup *ncds_read_file(
    const char *nc_file,
    int         recursive,
    int         header_only,
    int        *nc_format,
    CDSGroup   *cds_group)
{
    CDSGroup   *new_group;
    const char *file_name;
    int         ncid;

    /* Open the NetCDF file */

    if (!ncds_open(nc_file, NC_NOWRITE, &ncid)) {
        return((CDSGroup *)NULL);
    }

    /* Get the NetCDF file format */

    if (nc_format) {
        if (!ncds_format(ncid, nc_format)) {
            ncds_close(ncid);
            return((CDSGroup *)NULL);
        }
    }

    /* Create a new CDS group */

    file_name = strrchr(nc_file, '/');
    if (!file_name) {
        file_name = nc_file;
    }

    new_group = cds_define_group(cds_group, file_name);
    if (!new_group) {
        ncds_close(ncid);
        return((CDSGroup *)NULL);
    }

    /* Read the NetCDF header */

    if (!ncds_read_group(ncid, recursive, new_group)) {
        ncds_close(ncid);
        cds_delete_group(new_group);
        return((CDSGroup *)NULL);
    }

    /* Read the NetCDF data */

    if (!header_only) {
        if (!ncds_read_group_data(ncid, 0, 0, recursive, new_group, 0)) {
            ncds_close(ncid);
            cds_delete_group(new_group);
            return((CDSGroup *)NULL);
        }
    }

    ncds_close(ncid);

    return(new_group);
}

/**
 *  Read a dimension definition from a NetCDF group into a CDS group.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  nc_grpid     - NetCDF group id
 *  @param  nc_dimid     - NetCDF dimension id
 *  @param  cds_group    - pointer to the CDS group
 *  @param  cds_dim_name - dimension name to use in the CDS group,
 *                         or NULL to use the name of the NetCDF dimension.
 *
 *  @return
 *    - pointer to the CDS dimension defined in the CDS group
 *    - NULL if a NetCDF or CDS error occurred
 */
CDSDim *ncds_read_dim(
    int         nc_grpid,
    int         nc_dimid,
    CDSGroup   *cds_group,
    const char *cds_dim_name)
{
    CDSDim *dim;
    int     nunlim_dimids;
    int     unlim_dimids[NC_MAX_DIMS];
    char    dim_name[NC_MAX_NAME + 1];
    size_t  dim_length;
    int     is_unlimited;
    int     ud_index;

    /* Get the ids of the unlimited dimensions */

    if (!ncds_inq_unlimdims(nc_grpid, &nunlim_dimids, unlim_dimids)) {
        return((CDSDim *)NULL);
    }

    /* Get the dimension name and length */

    if (!ncds_inq_dim(nc_grpid, nc_dimid, dim_name, &dim_length)) {
        return((CDSDim *)NULL);
    }

    /* Check if this is an unlimited dimension */

    is_unlimited = 0;

    for (ud_index = 0; ud_index < nunlim_dimids; ud_index++) {
        if (nc_dimid == unlim_dimids[ud_index]) {
            dim_length = 0;
            is_unlimited = 1;
            break;
        }
    }

    /* Define the dimension in the CDS group */

    if (!cds_dim_name) {
        cds_dim_name = dim_name;
    }

    dim = cds_define_dim(cds_group, cds_dim_name, dim_length, is_unlimited);

    return(dim);
}

/**
 *  Read all dimension definitions from a NetCDF group into a CDS group.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  nc_grpid  - NetCDF group id
 *  @param  cds_group - pointer to the CDS group
 *
 *  @return
 *    - number of dimensions read in from the NetCDF group
 *    - -1 if a NetCDF or CDS error occurred
 */
int ncds_read_dims(
    int       nc_grpid,
    CDSGroup *cds_group)
{
    int ndims;
    int dimids[NC_MAX_DIMS];
    int index;

    if (!ncds_inq_dimids(nc_grpid, &ndims, dimids, 0)) {
        return(-1);
    }

    for (index = 0; index < ndims; index++) {
        if (!ncds_read_dim(nc_grpid, dimids[index], cds_group, NULL)) {
            return(-1);
        }
    }

    return(ndims);
}

/**
 *  Read an attribute definition from a NetCDF group into a CDS group.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  nc_grpid     - NetCDF group id
 *  @param  nc_attid     - NetCDF attribute id
 *  @param  cds_group    - pointer to the CDS group
 *  @param  cds_att_name - attribute name to use in the CDS group,
 *                         or NULL to use the name of the NetCDF attribute.
 *
 *  @return
 *    - pointer to the CDS attribute defined in the CDS group
 *    - NULL if a NetCDF or CDS error occurred
 */
CDSAtt *ncds_read_att(
    int         nc_grpid,
    int         nc_attid,
    CDSGroup   *cds_group,
    const char *cds_att_name)
{
    return(_ncds_read_att(
        nc_grpid, NC_GLOBAL, nc_attid, (void *)cds_group, cds_att_name));
}

/**
 *  Read all attribute definitions from a NetCDF group into a CDS group.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  nc_grpid  - NetCDF group id
 *  @param  cds_group - pointer to the CDS group
 *
 *  @return
 *    - number of attributes read in from the NetCDF group
 *    - -1 if a NetCDF or CDS error occurred
 */
int ncds_read_atts(
    int       nc_grpid,
    CDSGroup *cds_group)
{
    int natts;
    int index;

    if (!ncds_inq_natts(nc_grpid, &natts)) {
        return(-1);
    }

    for (index = 0; index < natts; index++) {

        if (!_ncds_read_att(
            nc_grpid, NC_GLOBAL, index, (void *)cds_group, NULL)) {

            return(-1);
        }
    }

    return(natts);
}

/**
 *  Read a variable definition from a NetCDF group into a CDS group.
 *
 *  This function will also read in any dependant dimensions that
 *  have not already been defined and all the variable attributes.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  nc_grpid      - NetCDF group id
 *  @param  nc_varid      - NetCDF variable id
 *  @param  cds_group     - pointer to the CDS group
 *  @param  cds_var_name  - variable name to use in the CDS group,
 *                          or NULL to use the name of the NetCDF variable.
 *  @param  nmap_dims     - number of dimension names to map from the input
 *                          NetCDF file to the output CDS group.
 *  @param  nc_dim_names  - dimension names in the input NetCDF file.
 *  @param  cds_dim_names - dimension names in the output CDS group.
 *
 *  @return
 *    - pointer to the CDS variable defined in the CDS group
 *    - NULL if a NetCDF or CDS error occurred
 */
CDSVar *ncds_read_var_def(
    int          nc_grpid,
    int          nc_varid,
    CDSGroup    *cds_group,
    const char  *cds_var_name,
    int          nmap_dims,
    const char **nc_dim_names,
    const char **cds_dim_names)
{
    char         var_name[NC_MAX_NAME + 1];
    nc_type      nctype;
    int          ndims;
    int          dimids[NC_MAX_DIMS];
    int          natts;

    int          dim_index;
    int          dimid;
    const char  *dim_name;
    char         name_buffer[NC_MAX_NAME + 1];
    const char  *dim_names[NC_MAX_DIMS];
    CDSDim      *dim;

    CDSDataType  cds_type;
    CDSVar      *var;
    CDSAtt      *att;
    int          attid;
    int          has_fill_value_att;
    void        *fill_value;

    int          mdi;

    /* Get the variable name, type, dimids, and number of attributes */

    if (!ncds_inq_var(
        nc_grpid, nc_varid, var_name, &nctype, &ndims, dimids, &natts)) {

        return((CDSVar *)NULL);
    }

    /* Make sure the dims have been defined and create the dim names list */

    for (dim_index = 0; dim_index < ndims; dim_index++) {

        dimid = dimids[dim_index];

        /* Get the dimension name */

        if (!ncds_inq_dimname(nc_grpid, dimid, name_buffer)) {
            return((CDSVar *)NULL);
        }

        /* Check if we are mapping this dimension name to a
         * different dimension name in the output CDS group. */

        dim_name = name_buffer;

        for (mdi = 0; mdi < nmap_dims; mdi++) {
            if (strcmp(dim_name, nc_dim_names[mdi]) == 0) {
                dim_name = cds_dim_names[mdi];
                break;
            }
        }

        /* Make sure this dimension has been defined */

        dim = cds_get_dim(cds_group, dim_name);

        if (!dim) {
            dim = ncds_read_dim(nc_grpid, dimid, cds_group, dim_name);
            if (!dim) {
                return((CDSVar *)NULL);
            }
        }

        dim_names[dim_index] = dim->name;
    }

    /* Get the CDS data type corresponding to the NetCDF data type */

    cds_type = ncds_cds_type(nctype);
    if (cds_type == CDS_NAT) {

        ERROR( NCDS_LIB_NAME,
            "Could not get variable definition\n"
            " -> nc_grpid = %d, var_name = '%s'\n"
            " -> unsupported netcdf data type (%d)\n",
            nc_grpid, var_name, nctype);

        return((CDSVar *)NULL);
    }

    /* Define the variable in the CDS group */

    if (!cds_var_name) {
        cds_var_name = var_name;
    }

    var = cds_define_var(cds_group, cds_var_name, cds_type, ndims, dim_names);
    if (!var) {
        return((CDSVar *)NULL);
    }

    /* Define the variable attributes in the CDS variable */

    has_fill_value_att = 0;

    for (attid = 0; attid < natts; attid++) {

        att = _ncds_read_att(nc_grpid, nc_varid, attid, (void *)var, NULL);
        if (!att) {
            cds_delete_var(var);
            return((CDSVar *)NULL);
        }

        if (strcmp(att->name, "_FillValue") == 0) {
            has_fill_value_att = 1;
        }
    }

    /* Set the default fill value if the _FillValue attribute was not found */

    if (!has_fill_value_att) {

        fill_value = _ncds_default_fill_value(nctype);

        if (!cds_set_var_default_fill_value(var, fill_value)) {
            cds_delete_var(var);
            return((CDSVar *)NULL);
        }
    }

    return(var);
}

/**
 *  Read all variable definitions from a NetCDF group into a CDS group.
 *
 *  This function will also read in any dependant dimensions that
 *  have not already been defined and all the variable attributes.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  nc_grpid  - NetCDF group id
 *  @param  cds_group - pointer to the CDS group
 *
 *  @return
 *    - number of variables read in from the NetCDF group
 *    - -1 if a NetCDF or CDS error occurred
 */
int ncds_read_var_defs(
    int       nc_grpid,
    CDSGroup *cds_group)
{
    int nvars;
    int varids[NC_MAX_VARS];
    int index;

    if (!ncds_inq_varids(nc_grpid, &nvars, varids)) {
        return(-1);
    }

    for (index = 0; index < nvars; index++) {

        if (!ncds_read_var_def(
            nc_grpid, varids[index], cds_group, NULL, 0, NULL, NULL)) {

            return(-1);
        }
    }

    return(nvars);
}

/**
 *  Read a NetCDF group definition into a CDS group.
 *
 *  This function will read in all dimension, attribute, and variable
 *  definitions from a NetCDF group into a CDS group.  It will also recurse
 *  into subgroups if recursive is TRUE.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  nc_grpid  - NetCDF group id
 *  @param  recursive - recurse into subroups (1 = TRUE, 0 = FALSE)
 *  @param  cds_group - pointer to the CDS group
 *
 *  @return
 *    - 1 if successful
 *    - 0 if a NetCDF or CDS error occurred
 */
int ncds_read_group(
    int       nc_grpid,
    int       recursive,
    CDSGroup *cds_group)
{
    int       nids;
    int       ids[NC_MAX_VARS];
    int       index;
    char      subgroup_name[NC_MAX_NAME + 1];
    CDSGroup *subgroup;

    /* Read dimensions */

    if (!ncds_inq_dimids(nc_grpid, &nids, ids, 0)) {
        return(0);
    }

    for (index = 0; index < nids; index++) {
        if (!ncds_read_dim(nc_grpid, ids[index], cds_group, NULL)) {
            return(0);
        }
    }

    /* Read attributes */

    if (!ncds_inq_natts(nc_grpid, &nids)) {
        return(0);
    }

    for (index = 0; index < nids; index++) {

        if (!_ncds_read_att(
            nc_grpid, NC_GLOBAL, index, (void *)cds_group, NULL)) {

            return(0);
        }
    }

    /* Read variables */

    if (!ncds_inq_varids(nc_grpid, &nids, ids)) {
        return(0);
    }

    for (index = 0; index < nids; index++) {

        if (!ncds_read_var_def(
            nc_grpid, ids[index], cds_group, NULL, 0, NULL, NULL)) {

            return(0);
        }
    }

    /* Read subgroups */

    if (recursive) {

        if (!ncds_inq_grpids(nc_grpid, &nids, ids)) {
            return(0);
        }

        for (index = 0; index < nids; index++) {

            /* get the subgroup name */

            if (!ncds_inq_grpname(ids[index], subgroup_name)) {
                return(0);
            }

            /* create the CDS subgroup */

            subgroup = cds_define_group(cds_group, subgroup_name);
            if (!subgroup) {
                return(0);
            }

            /* recurse into the subgroup */

            if (!ncds_read_group(ids[index], recursive, subgroup)) {
                return(0);
            }
        }
    }

    return(1);
}

/**
 *  Read data from a NetCDF variable into a CDS variable.
 *
 *  This function will also do the necessary type, units, and missing
 *  value conversions,
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  nc_grpid         - NetCDF group id
 *  @param  nc_varid         - NetCDF variable id
 *  @param  nc_start         - NetCDF dimension start indexes
 *  @param  nc_count         - number of values to read along each dimension
 *  @param  cds_var          - pointer to the CDS variable
 *  @param  cds_sample_start - CDS variable start sample
 *
 *  @return
 *    - pointer to the specifed start sample in the variable's data array
 *    - NULL if a NetCDF or CDS error occurred
 */
void *ncds_read_var_data(
    int     nc_grpid,
    int     nc_varid,
    size_t *nc_start,
    size_t *nc_count,
    CDSVar *cds_var,
    size_t  cds_sample_start)
{
    nc_type           nc_var_type;
    CDSDataType       nc_cds_type;
    size_t            nc_type_size;
    size_t            cds_type_size;

    int               map_missing;
    int               nc_nmv;
    void             *nc_mv;
    int               cds_nmv;
    void             *cds_mv;
    void             *new_cds_mv;
    void             *mvp;
    int               mi;

    CDSUnitConverter  converter;
    char             *nc_units;
    const char       *cds_units;

    size_t            cds_sample_count;
    void             *cds_datap;
    void             *nc_datap;
    size_t            length;

    int               status;

    /* Get the netcdf variable data type */

    if (!ncds_inq_vartype(nc_grpid, nc_varid, &nc_var_type)) {
        return((void *)NULL);
    }

    nc_cds_type = ncds_cds_type(nc_var_type);
    if (nc_cds_type == CDS_NAT) {

        ERROR( NCDS_LIB_NAME,
            "Could not read variable data\n"
            " -> nc_grpid = %d, nc_varid = %d, cds_var = '%s'\n"
            " -> unsupported netcdf data type (%d)\n",
            nc_grpid, nc_varid, cds_var->name, nc_var_type);

        return((void *)NULL);
    }

    nc_type_size  = cds_data_type_size(nc_cds_type);
    cds_type_size = cds_data_type_size(cds_var->type);

    /* Check if we need to map the missing values in the NetCDF
     * variable data to the missing values in the CDS variable data */

    map_missing = 0;
    cds_mv      = (void *)NULL;
    cds_nmv     = 0;

    nc_nmv = ncds_get_missing_values(nc_grpid, nc_varid, &nc_mv);
    if (nc_nmv < 0) {
        return((void *)NULL);
    }
    else if (nc_nmv > 0) {

        /* Get the missing values to use in the CDS variable data */

        cds_nmv = cds_get_var_missing_values(cds_var, &cds_mv);
        if (cds_nmv < 0) {
            free(nc_mv);
            return((void *)NULL);
        }

        if (cds_nmv < nc_nmv) {

            new_cds_mv = malloc(nc_nmv * cds_type_size);

            if (!new_cds_mv) {

                ERROR( NCDS_LIB_NAME,
                    "Could not read variable data\n"
                    " -> nc_grpid = %d, nc_varid = %d, cds_var = '%s'\n"
                    " -> memory allocation error\n",
                    nc_grpid, nc_varid, cds_var->name);

//                if (cds_nmv) free(cds_mv);
                if (cds_nmv) cds_free_array(cds_var->type, cds_nmv, cds_mv);

                free(nc_mv);
                return((void *)NULL);
            }

            if (cds_nmv) {
                memcpy(new_cds_mv, cds_mv, cds_nmv * cds_type_size);
                free(cds_mv);
            }
            else {
                cds_get_default_fill_value(cds_var->type, new_cds_mv);
                cds_set_var_default_fill_value(cds_var, new_cds_mv);
                cds_nmv = 1;
            }

            cds_mv = new_cds_mv;

            for (mi = cds_nmv; mi < nc_nmv; ++mi) {
                mvp = (void *)((char *)cds_mv + mi * cds_type_size);
                memcpy(mvp, cds_mv, cds_type_size);
            }
        }

        map_missing = 0;

        if (nc_cds_type == cds_var->type) {
            if (cds_var->type == CDS_STRING) {
                for (mi = 0; mi < nc_nmv; ++mi) {
                    if (strcmp(((char **)nc_mv)[mi], ((char **)cds_mv)[mi]) != 0) {
                        map_missing = 1;
                        break;
                    }
                }
            }
            else {
                if (memcmp(nc_mv, cds_mv, nc_nmv * nc_type_size) != 0) {
                    map_missing = 1;
                }
            }
        }
    }

    /* Check if we need to do a units conversion */

    converter = (CDSUnitConverter)NULL;

    cds_units = cds_get_var_units(cds_var);
    if (cds_units) {

        status = ncds_get_var_units(nc_grpid, nc_varid, &nc_units);
        if (status < 0) {
//            if (nc_nmv)  free(nc_mv);
            if (nc_nmv) cds_free_array(nc_cds_type, nc_nmv, nc_mv);
//            if (cds_nmv) free(cds_mv);
            if (cds_nmv) cds_free_array(cds_var->type, cds_nmv, cds_mv);
            return((void *)NULL);
        }
        else if (status > 0) {
            status = cds_get_unit_converter(nc_units, cds_units, &converter);
            if (status < 0) {
                free(nc_units);
//                if (nc_nmv)  free(nc_mv);
                if (nc_nmv) cds_free_array(nc_cds_type, nc_nmv, nc_mv);
//                if (cds_nmv) free(cds_mv);
                if (cds_nmv) cds_free_array(cds_var->type, cds_nmv, cds_mv);
                return((void *)NULL);
            }
        }

        free(nc_units);
    }

    /* Allocate memory for the CDS variable data */

    cds_sample_count = nc_count[0];

    cds_datap = cds_alloc_var_data(cds_var, cds_sample_start, cds_sample_count);
    if (!cds_datap) {
        if (converter)   cds_free_unit_converter(converter);
//        if (nc_nmv)  free(nc_mv);
        if (nc_nmv) cds_free_array(nc_cds_type, nc_nmv, nc_mv);
//        if (cds_nmv) free(cds_mv);
        if (cds_nmv) cds_free_array(cds_var->type, cds_nmv, cds_mv);
        return((void *)NULL);
    }

    /* Check if we need a temporary buffer for a type conversion */

    nc_datap = cds_datap;

    if (nc_type_size != cds_type_size) {

        length   = cds_sample_count * cds_var_sample_size(cds_var);
        nc_datap = malloc(length * nc_type_size);

        if (!nc_datap) {

            ERROR( NCDS_LIB_NAME,
                "Could not read variable data\n"
                " -> nc_grpid = %d, nc_varid = %d, cds_var = '%s'\n"
                " -> memory allocation error\n",
                nc_grpid, nc_varid, cds_var->name);

            if (converter) cds_free_unit_converter(converter);
//            if (nc_nmv)    free(nc_mv);
            if (nc_nmv) cds_free_array(nc_cds_type, nc_nmv, nc_mv);
//            if (cds_nmv)   free(cds_mv);
            if (cds_nmv) cds_free_array(cds_var->type, cds_nmv, cds_mv);
            return((void *)NULL);
        }
    }

    /* Read the data from the NetCDF group */

    status = nc_get_vara(nc_grpid, nc_varid, nc_start, nc_count, nc_datap);

    if (status != NC_NOERR) {

        ERROR( NCDS_LIB_NAME,
            "Could not read variable data\n"
            " -> nc_grpid = %d, nc_varid = %d, cds_var = '%s'\n"
            " -> %s\n",
            nc_grpid, nc_varid, cds_var->name, nc_strerror(status));

        if (converter) cds_free_unit_converter(converter);
        if (nc_nmv)    free(nc_mv);
//        if (cds_nmv)   free(cds_mv);
        if (cds_nmv) cds_free_array(cds_var->type, cds_nmv, cds_mv);
        return((void *)NULL);
    }

    /* Check if we need to do any conversions */

    if (converter || map_missing || (nc_cds_type != cds_var->type)) {

        length = cds_sample_count * cds_var_sample_size(cds_var);

        if (converter) {
            cds_convert_units(converter,
                nc_cds_type, length, nc_datap, cds_var->type, cds_datap,
                nc_nmv, nc_mv, cds_mv,
                NULL, cds_mv, NULL, cds_mv);
        }
        else {
            cds_copy_array(
                nc_cds_type, length, nc_datap, cds_var->type, cds_datap,
                nc_nmv, nc_mv, cds_mv,
                NULL, cds_mv, NULL, cds_mv);
        }
    }

    /* Cleanup and return */

    if (converter) cds_free_unit_converter(converter);
//    if (nc_nmv)    free(nc_mv);
    if (nc_nmv) cds_free_array(nc_cds_type, nc_nmv, nc_mv);
//    if (cds_nmv)   free(cds_mv);
    if (cds_nmv) cds_free_array(cds_var->type, cds_nmv, cds_mv);

//    if (nc_datap != cds_datap) free(nc_datap);
    if (nc_datap != cds_datap) {
        length = cds_sample_count * cds_var_sample_size(cds_var);
        cds_free_array(nc_cds_type, length, nc_datap);
    }

    return(cds_datap);
}

/**
 *  Read data from a NetCDF variable into a CDS variable.
 *
 *  If sample_count is NULL or the value pointed to by sample_count
 *  is 0, the number of available samples will be read.
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
 *  @param  cds_var          - pointer to the CDS variable
 *  @param  cds_sample_start - CDS variable start sample
 *
 *  @return
 *    - pointer to the specifed start sample in the variable's data array
 *    - NULL if a NetCDF or CDS error occurred
 */
void *ncds_read_var_samples(
    int     nc_grpid,
    int     nc_varid,
    size_t  nc_sample_start,
    size_t *sample_count,
    CDSVar *cds_var,
    size_t  cds_sample_start)
{
    int     ndims;
    int     dimids[NC_MAX_DIMS];
    int     dim_index;
    int     dimid;
    CDSDim *dim;
    size_t  dim_length;
    size_t  start[NC_MAX_DIMS];
    size_t  count[NC_MAX_DIMS];
    void   *datap;

    /* Get the number of NetCDF variable dimensions */

    if (!ncds_inq_varndims(nc_grpid, nc_varid, &ndims)) {
        return((void *)NULL);
    }

    /* Make sure the NetCDF and CDS variables have the same number of dims */

    if (ndims != cds_var->ndims) {

        ERROR( NCDS_LIB_NAME,
            "Incompatible variable shapes\n"
            " -> nc_grpid = %d, nc_varid = %d, cds_var = '%s'\n"
            " -> number of CDS dims (%d) <> number of netcdf dims (%d)\n",
            nc_grpid, nc_varid, cds_var->name, cds_var->ndims, ndims);

        return((void *)NULL);
    }

    /* Check if this is a variable with no dimensions */

    if (ndims == 0) {

        start[0] = 0;
        count[0] = 1;

        if (sample_count) {
            *sample_count = 1;
        }

        datap = ncds_read_var_data(
            nc_grpid, nc_varid, start, count, cds_var, 0);

        return(datap);
    }

    /* Get the NetCDF variable dimension ids */

    if (!ncds_inq_vardimids(nc_grpid, nc_varid, dimids)) {
        return((void *)NULL);
    }

    /* Create start and count arrays */

    for (dim_index = 0; dim_index < ndims; dim_index++) {

        dimid = dimids[dim_index];
        dim   = cds_var->dims[dim_index];

        /* Get the NetCDF dimension length */

        if (!ncds_inq_dimlen(nc_grpid, dimid, &dim_length)) {
            return((void *)NULL);
        }

        if (dim_index == 0) {

            if (nc_sample_start >= dim_length) {

                ERROR( NCDS_LIB_NAME,
                    "Invalid netcdf variable start sample\n"
                    " -> nc_grpid = %d, nc_varid = %d, nc_dimid = %d\n"
                    " -> start sample (%d) >= dimension length (%d)\n",
                    nc_grpid, nc_varid, dimid,
                    nc_sample_start, dim_length);

                return((void *)NULL);
            }

            start[dim_index] = nc_sample_start;
            count[dim_index] = dim_length - nc_sample_start;

            if (dim->is_unlimited == 0) {

                if (cds_sample_start >= dim->length) {

                    ERROR( NCDS_LIB_NAME,
                        "Invalid CDS variable start sample\n"
                        " -> var_name = '%s', dim_name = '%s'\n"
                        " -> start sample (%d) >= dimension length (%d)\n",
                        cds_var->name, dim->name,
                        cds_sample_start, dim->length);

                    return((void *)NULL);
                }

                if (count[dim_index] > dim->length - cds_sample_start) {
                    count[dim_index] = dim->length - cds_sample_start;
                }
            }
        }
        else {

            if (dim->length > dim_length) {

                ERROR( NCDS_LIB_NAME,
                    "Incompatible variable shapes\n"
                    " -> nc_grpid = %d, nc_varid = %d, cds_var = '%s', dim_index = %d\n"
                    " -> length of CDS dim (%d) > length of netcdf dim (%d)\n",
                    nc_grpid, nc_varid, cds_var->name, dim_index,
                    dim->length, dim_length);

                return((void *)NULL);
            }

            start[dim_index] = 0;
            count[dim_index] = dim->length;
        }
    }

    /* At this point count[0] will be the maximum number of samples
     * that can be read. */

    if (sample_count) {

        if (*sample_count > 0 &&
            *sample_count < count[0]) {

            count[0] = *sample_count;
        }
        else {
            *sample_count = count[0];
        }
    }

    /* Read in the data */

    datap = ncds_read_var_data(
        nc_grpid, nc_varid, start, count, cds_var, cds_sample_start);

    return(datap);
}

/**
 *  Read static data from a NetCDF group into a CDS group.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  This function will read in the NetCDF data for each variable
 *  defined in the CDS group that do not have an unlimited dimension.
 *  Variables that are defined in the CDS group but not in the NetCDF
 *  group will be ignored.
 *
 *  @param  nc_grpid  - NetCDF group id
 *  @param  cds_group - pointer to the CDS group
 *
 *  @return
 *    - 1 if successful
 *    - 0 if a NetCDF or CDS error occurred
 */
int ncds_read_static_data(
    int       nc_grpid,
    CDSGroup *cds_group)
{
    CDSVar *var;
    int     index;
    int     status;
    int     varid;

    for (index = 0; index < cds_group->nvars; index++) {

        var = cds_group->vars[index];

        if (var->ndims == 0 ||
            var->dims[0]->is_unlimited == 0) {

            status = ncds_inq_varid(nc_grpid, var->name, &varid);

            if (status == 1) {
                if (!ncds_read_var_samples(nc_grpid, varid, 0, NULL, var, 0)) {
                    return(0);
                }
            }
            else if (status < 0) {
                return(0);
            }
        }
    }

    return(1);
}

/**
 *  Read data records from a NetCDF group into a CDS group.
 *
 *  This function will read in the NetCDF data for each variable
 *  defined in the CDS group that have an unlimited dimension.
 *  Variables that are defined in the CDS group but not in the
 *  NetCDF group will be ignored.
 *
 *  If record_count is 0, the number of available records will be read.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  nc_grpid         - NetCDF group id
 *  @param  nc_record_start  - NetCDF start record
 *  @param  record_count     - number of records to read
 *  @param  cds_group        - pointer to the CDS group
 *  @param  cds_record_start - CDS start record
 *
 *  @return
 *    - 1 if successful
 *    - 0 if a NetCDF or CDS error occurred
 */
int ncds_read_records(
    int       nc_grpid,
    size_t    nc_record_start,
    size_t    record_count,
    CDSGroup *cds_group,
    size_t    cds_record_start)
{
    CDSVar *var;
    int     index;
    int     status;
    int     varid;
    size_t  count;

    for (index = 0; index < cds_group->nvars; index++) {

        var = cds_group->vars[index];

        if (var->ndims && var->dims[0]->is_unlimited) {

            status = ncds_inq_varid(nc_grpid, var->name, &varid);

            if (status == 1) {

                count = record_count;

                if (!ncds_read_var_samples(
                    nc_grpid, varid, nc_record_start, &count,
                    var, cds_record_start)) {

                    return(0);
                }
            }
            else if (status < 0) {
                return(0);
            }
        }
    }

    return(1);
}

/**
 *  Read data from a NetCDF group into a CDS group.
 *
 *  This function will read in the NetCDF data for each variable
 *  defined in the CDS group. Variables and groups that are defined
 *  in the CDS group but not in the NetCDF group will be ignored.
 *
 *  If record_count is 0, the number of available records will be read.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  nc_grpid         - NetCDF group id
 *  @param  nc_record_start  - NetCDF start record
 *  @param  record_count     - number of records to read
 *  @param  recursive        - recurse into subroups (1 = TRUE, 0 = FALSE)
 *  @param  cds_group        - pointer to the CDS group
 *  @param  cds_record_start - CDS start record
 *
 *  @return
 *    - 1 if successful
 *    - 0 if a NetCDF or CDS error occurred
 */
int ncds_read_group_data(
    int       nc_grpid,
    size_t    nc_record_start,
    size_t    record_count,
    int       recursive,
    CDSGroup *cds_group,
    size_t    cds_record_start)
{
    int       index;
    int       status;
    CDSGroup *subgroup;
    int       subgrpid;

    if (!ncds_read_static_data(nc_grpid, cds_group)) {
        return(0);
    }

    if (!ncds_read_records(
        nc_grpid, nc_record_start, record_count, cds_group, cds_record_start)) {

        return(0);
    }

    if (recursive) {

        for (index = 0; index < cds_group->ngroups; index++) {

            subgroup = cds_group->groups[index];

            status = ncds_inq_grpid(nc_grpid, subgroup->name, &subgrpid);

            if (status == 1) {

                if (!ncds_read_group_data(
                    subgrpid, nc_record_start, record_count, recursive,
                    subgroup, cds_record_start)) {

                    return(0);
                }
            }
            else if (status < 0) {

                ERROR( NCDS_LIB_NAME,
                    "Could not get group id\n"
                    " -> nc_grpid = %d, group_name = '%s'\n"
                    " -> %s\n",
                    nc_grpid, subgroup->name, nc_strerror(status));

                return(0);
            }
        }
    }

    return(1);
}
