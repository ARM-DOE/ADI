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
*    $Revision: 60294 $
*    $Author: ermold $
*    $Date: 2015-02-16 23:31:33 +0000 (Mon, 16 Feb 2015) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/**
 *  @file ncwrap_inquire.c
 *  Wrappers for NetCDF inquire functions.
 */

#include "ncds3.h"

/*******************************************************************************
 *  Private Functions
 */
/** @privatesection */

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Get information about an attribute in a NetCDF variable or group.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  grpid   - group id
 *  @param  varid   - variable id or NC_GLOBAL for global attributes
 *  @param  attname - attribute name
 *  @param  type    - output: attrubute data type
 *  @param  length  - output: length of the attribute value
 *
 *  @return
 *    -  1 if successful
 *    -  0 if the attribute was not found
 *    - -1 if an error occured
 */
int ncds_inq_att(
    int grpid, int varid, const char *attname,
    nc_type *type, size_t *length)
{
    int status = nc_inq_att(grpid, varid, attname, type, length);

    if (status == NC_ENOTATT) {
        return(0);
    }

    if (status != NC_NOERR) {

        ERROR( NCDS_LIB_NAME,
            "Could not get netcdf attribute information\n"
            " -> grpid = %d, varid = %d, attname = '%s'\n"
            " -> %s\n",
            grpid, varid, attname, nc_strerror(status));

        return(-1);
    }

    return(1);
}

/**
 *  Get the name of an attribute in a NetCDF variable or group.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  grpid   - group id
 *  @param  varid   - variable id or NC_GLOBAL for global attributes
 *  @param  attid   - attribute id
 *  @param  attname - output: attribute name
 *                    (this should be declared to have a length
 *                     of at least NC_MAX_NAME + 1 characters)
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occured
 */
int ncds_inq_attname(int grpid, int varid, int attid, char *attname)
{
    int status = nc_inq_attname(grpid, varid, attid, attname);

    if (status != NC_NOERR) {

        ERROR( NCDS_LIB_NAME,
            "Could not get netcdf attribute name\n"
            " -> grpid = %d, varid = %d, attid = %d\n"
            " -> %s\n",
            grpid, varid, attid, nc_strerror(status));

        return(0);
    }

    return(1);
}

/**
 *  Get information about a dimension in a NetCDF group.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  grpid   - group id
 *  @param  dimid   - dimension id
 *  @param  dimname - output: dimension name
 *                    (this should be declared to have a length
 *                     of at least NC_MAX_NAME + 1 characters)
 *  @param  length  - output: dimension length
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occured
 */
int ncds_inq_dim(int grpid, int dimid, char *dimname, size_t *length)
{
    int status = nc_inq_dim(grpid, dimid, dimname, length);

    if (status != NC_NOERR) {

        ERROR( NCDS_LIB_NAME,
            "Could not get netcdf dimension information\n"
            " -> grpid = %d, dimid = %d\n"
            " -> %s\n",
            grpid, dimid, nc_strerror(status));

        return(0);
    }

    return(1);
}

/**
 *  Get the id of a dimension in a NetCDF group.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  grpid   - group id
 *  @param  dimname - dimension name
 *  @param  dimid   - output: dimension id
 *
 *  @return
 *    -  1 if successful
 *    -  0 if the dimension was not found
 *    - -1 if an error occured
 */
int ncds_inq_dimid(int grpid, const char *dimname, int *dimid)
{
    int status = nc_inq_dimid(grpid, dimname, dimid);

    if (status == NC_EBADDIM) {
        return(0);
    }

    if (status != NC_NOERR) {

        ERROR( NCDS_LIB_NAME,
            "Could not get netcdf dimension id\n"
            " -> grpid = %d, dimname = '%s'\n"
            " -> %s\n",
            grpid, dimname, nc_strerror(status));

        return(-1);
    }

    return(1);
}

/**
 *  Get the ids for all the dimensions in a NetCDF group.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  grpid           - group id
 *  @param  ndims           - output: number of dimensions
 *  @param  dimids          - output: dimension ids
 *                            (this should be declared to have a length
 *                             of at least NC_MAX_DIMS)
 *  @param  include_parents - if non-zero then all the dimensions in all
 *                            parent groups will also be retrieved.
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occured
 */
int ncds_inq_dimids(int grpid, int *ndims, int *dimids, int include_parents)
{
    int status = nc_inq_dimids(grpid, ndims, dimids, include_parents);

    if (status != NC_NOERR) {

        ERROR( NCDS_LIB_NAME,
            "Could not get netcdf dimension ids\n"
            " -> grpid = %d\n"
            " -> %s\n",
            grpid, nc_strerror(status));

        return(0);
    }

    return(1);
}

/**
 *  Get the length of a dimension in a NetCDF group.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  grpid  - group id
 *  @param  dimid  - dimension id
 *  @param  length - output: dimension length
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occured
 */
int ncds_inq_dimlen(int grpid, int dimid, size_t *length)
{
    int status = nc_inq_dimlen(grpid, dimid, length);

    if (status != NC_NOERR) {

        ERROR( NCDS_LIB_NAME,
            "Could not get netcdf dimension length\n"
            " -> grpid = %d, dimid = %d\n"
            " -> %s\n",
            grpid, dimid, nc_strerror(status));

        return(0);
    }

    return(1);
}

/**
 *  Get the name of a dimension in a NetCDF group.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  grpid   - group id
 *  @param  dimid   - dimension id
 *  @param  dimname - output: dimension name
 *                    (this should be declared to have a length
 *                     of at least NC_MAX_NAME + 1 characters)
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occured
 */
int ncds_inq_dimname(int grpid, int dimid, char *dimname)
{
    int status = nc_inq_dimname(grpid, dimid, dimname);

    if (status != NC_NOERR) {

        ERROR( NCDS_LIB_NAME,
            "Could not get netcdf dimension name\n"
            " -> grpid = %d, dimid = %d\n"
            " -> %s\n",
            grpid, dimid, nc_strerror(status));

        return(0);
    }

    return(1);
}

/**
 *  Get the ids for all the subgroups in a NetCDF group.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  grpid     - group id
 *  @param  nsubgrps  - output: number of subgroups
 *  @param  subgrpids - output: subgroup ids
 *
 *  Note: Specify NULL for subgrpids to get the number of subgroups. This
 *  can then be used to allocate memory for the subgroup ids array.
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occured
 */
int ncds_inq_grpids(int grpid, int *nsubgrps, int *subgrpids)
{
    int status = nc_inq_grps(grpid, nsubgrps, subgrpids);

    if (status != NC_NOERR) {

        ERROR( NCDS_LIB_NAME,
            "Could not get netcdf subgroup ids\n"
            " -> grpid = %d\n"
            " -> %s\n",
            grpid, nc_strerror(status));

        return(0);
    }

    return(1);
}

/**
 *  Get the id of a subgroup in a NetCDF group.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  grpid      - group id
 *  @param  subgrpname - subgroup name
 *  @param  subgrpid   - output: subgroup id
 *
 *  @return
 *    -  1 if successful
 *    -  0 if the subgroup was not found
 *    - -1 if an error occured
 */
int ncds_inq_grpid(int grpid, const char *subgrpname, int *subgrpid)
{
    int status = nc_inq_grp_ncid(grpid, subgrpname, subgrpid);

    if (status == NC_ENOGRP) {
        return(0);
    }

    if (status != NC_NOERR) {

        ERROR( NCDS_LIB_NAME,
            "Could not get netcdf subgroup id\n"
            " -> grpid = %d, subgrpname = '%s'\n"
            " -> %s\n",
            grpid, subgrpname, nc_strerror(status));

        return(-1);
    }

    return(1);
}

/**
 *  Get the name of a NetCDF group.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  grpid   - group id
 *  @param  grpname - output: group name
 *                    (this should be declared to have a length
 *                     of at least NC_MAX_NAME + 1 characters)
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occured
 */
int ncds_inq_grpname(int grpid, char *grpname)
{
    int status = nc_inq_grpname(grpid, grpname);

    if (status != NC_NOERR) {

        ERROR( NCDS_LIB_NAME,
            "Could not get netcdf group name\n"
            " -> grpid = %d\n"
            " -> %s\n",
            grpid, nc_strerror(status));

        return(0);
    }

    return(1);
}

/**
 *  Get the number of attributes in a NetCDF group.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  grpid - group id
 *  @param  natts - output: number of attributes
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occured
 */
int ncds_inq_natts(int grpid, int *natts)
{
    int status = nc_inq_natts(grpid, natts);

    if (status != NC_NOERR) {

        ERROR( NCDS_LIB_NAME,
            "Could not get number of netcdf attributes\n"
            " -> grpid = %d\n"
            " -> %s\n",
            grpid, nc_strerror(status));

        return(0);
    }

    return(1);
}

/**
 *  Get the number of dimensions visible from a NetCDF group.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  grpid - group id
 *  @param  ndims - output: number of dimensions
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occured
 */
int ncds_inq_ndims(int grpid, int *ndims)
{
    int status = nc_inq_ndims(grpid, ndims);

    if (status != NC_NOERR) {

        ERROR( NCDS_LIB_NAME,
            "Could not get number of netcdf dimensions\n"
            " -> grpid = %d\n"
            " -> %s\n",
            grpid, nc_strerror(status));

        return(0);
    }

    return(1);
}

/**
 *  Get information about a variable in a NetCDF group.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  grpid   - group id
 *  @param  varid   - variable id
 *  @param  varname - output: variable name
 *                    (this should be declared to have a length
 *                     of at least NC_MAX_NAME + 1 characters)
 *  @param  type    - output: variable data type
 *  @param  ndims   - output: number of variable dimensions
 *  @param  dimids  - output: variable dimension ids
 *                    (this should be declared to have a length
 *                     of at least NC_MAX_DIMS)
 *  @param  natts   - output: number of variable attributes
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occured
 */
int ncds_inq_var(
    int grpid, int varid,
    char *varname, nc_type *type, int *ndims, int *dimids, int *natts)
{
    int status = nc_inq_var(grpid, varid, varname, type, ndims, dimids, natts);

    if (status != NC_NOERR) {

        ERROR( NCDS_LIB_NAME,
            "Could not get netcdf variable information\n"
            " -> grpid = %d, varid = %d\n"
            " -> %s\n",
            grpid, varid, nc_strerror(status));

        return(0);
    }

    return(1);
}

/**
 *  Get the ids for all dimensions used by a variable in a NetCDF group.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  grpid   - group id
 *  @param  varid   - variable id
 *  @param  dimids  - output: variable dimension ids
 *                    (this should be declared to have a length
 *                     of at least NC_MAX_DIMS)
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occured
 */
int ncds_inq_vardimids(int grpid, int varid, int *dimids)
{
    int status = nc_inq_vardimid(grpid, varid, dimids);

    if (status != NC_NOERR) {

        ERROR( NCDS_LIB_NAME,
            "Could not get netcdf variable dimension ids\n"
            " -> grpid = %d, varid = %d\n"
            " -> %s\n",
            grpid, varid, nc_strerror(status));

        return(0);
    }

    return(1);
}

/**
 *  Get the id of a variable in a NetCDF group.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  grpid   - group id
 *  @param  varname - variable name
 *  @param  varid   - output: variable id
 *
 *  @return
 *    -  1 if successful
 *    -  0 if the variable was not found
 *    - -1 if an error occured
 */
int ncds_inq_varid(int grpid, const char *varname, int *varid)
{
    int status = nc_inq_varid(grpid, varname, varid);

    if (status == NC_ENOTVAR) {
        return(0);
    }

    if (status != NC_NOERR) {

        ERROR( NCDS_LIB_NAME,
            "Could not get netcdf variable id\n"
            " -> grpid = %d, varname = '%s'\n"
            " -> %s\n",
            grpid, varname, nc_strerror(status));

        return(-1);
    }

    return(1);
}

/**
 *  Get the ids for all the variables in a NetCDF group.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  grpid  - group id
 *  @param  nvars  - output: number of variables
 *  @param  varids - output: variable ids
 *                   (this should be declared to have a length
 *                    of at least NC_MAX_VARS)
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occured
 */
int ncds_inq_varids(int grpid, int *nvars, int *varids)
{
    int status = nc_inq_varids(grpid, nvars, varids);

    if (status != NC_NOERR) {

        ERROR( NCDS_LIB_NAME,
            "Could not get netcdf variable ids\n"
            " -> grpid = %d\n"
            " -> %s\n",
            grpid, nc_strerror(status));

        return(0);
    }

    return(1);
}

/**
 *  Get the name of a variable in a NetCDF group.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  grpid   - group id
 *  @param  varid   - variable id
 *  @param  varname - output: variable name
 *                    (this should be declared to have a length
 *                     of at least NC_MAX_NAME + 1 characters)
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occured
 */
int ncds_inq_varname(int grpid, int varid, char *varname)
{
    int status = nc_inq_varname(grpid, varid, varname);

    if (status != NC_NOERR) {

        ERROR( NCDS_LIB_NAME,
            "Could not get netcdf variable name\n"
            " -> grpid = %d, varid = %d\n"
            " -> %s\n",
            grpid, varid, nc_strerror(status));

        return(0);
    }

    return(1);
}

/**
 *  Get the number of attributes for a variable in a NetCDF group.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  grpid - group id
 *  @param  varid - variable id
 *  @param  natts - output: number of variable attributes
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occured
 */
int ncds_inq_varnatts(int grpid, int varid, int *natts)
{
    int status = nc_inq_varnatts(grpid, varid, natts);

    if (status != NC_NOERR) {

        ERROR( NCDS_LIB_NAME,
            "Could not get number of netcdf variable attributes\n"
            " -> grpid = %d, varid = %d\n"
            " -> %s\n",
            grpid, varid, nc_strerror(status));

        return(0);
    }

    return(1);
}

/**
 *  Get the number of dimensions for a variable in a NetCDF group.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  grpid - group id
 *  @param  varid - variable id
 *  @param  ndims - output: number of variable dimensions
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occured
 */
int ncds_inq_varndims(int grpid, int varid, int *ndims)
{
    int status = nc_inq_varndims(grpid, varid, ndims);

    if (status != NC_NOERR) {

        ERROR( NCDS_LIB_NAME,
            "Could not get number of netcdf variable dimensions\n"
            " -> grpid = %d, varid = %d\n"
            " -> %s\n",
            grpid, varid, nc_strerror(status));

        return(0);
    }

    return(1);
}

/**
 *  Get the type of a variable in a NetCDF group.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  grpid   - group id
 *  @param  varid   - variable id
 *  @param  vartype - output: variable type
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occured
 */
int ncds_inq_vartype(int grpid, int varid, nc_type *vartype)
{
    int status = nc_inq_vartype(grpid, varid, vartype);

    if (status != NC_NOERR) {

        ERROR( NCDS_LIB_NAME,
            "Could not get netcdf variable type\n"
            " -> grpid = %d, varid = %d\n"
            " -> %s\n",
            grpid, varid, nc_strerror(status));

        return(0);
    }

    return(1);
}

/**
 *  Get the ids for all unlimited dimensions in a NetCDF group.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  grpid       - group id
 *  @param  nunlimdims  - output: number of unlimited dimensions
 *  @param  unlimdimids - output: unlimited dimension ids
 *                        (this should be declared to have a length
 *                         of at least NC_MAX_DIMS)
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occured
 */
int ncds_inq_unlimdims(int grpid, int *nunlimdims, int *unlimdimids)
{
    int status = nc_inq_unlimdims(grpid, nunlimdims, unlimdimids);

    if (status != NC_NOERR) {

        ERROR( NCDS_LIB_NAME,
            "Could not get netcdf unlimited dimension ids\n"
            " -> grpid = %d\n"
            " -> %s\n",
            grpid, nc_strerror(status));

        return(0);
    }

    return(1);
}
