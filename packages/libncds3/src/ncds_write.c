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
*    $Revision: 50142 $
*    $Author: ermold $
*    $Date: 2013-11-21 00:06:12 +0000 (Thu, 21 Nov 2013) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/**
 *  @file ncds_write.c
 *  NetCDF Write Functions.
 */

#include <unistd.h>

#include "ncds3.h"
#include "ncds_private.h"

/*******************************************************************************
 *  Private Functions
 */
/** @privatesection */

/**
 *  PRIVATE: Write an attribute definition into a NetCDF group or variable.
 *
 *  For global attributes, specify NC_GLOBAL for the variable id.
 *
 *  The following special attributes are recognized by this function. These
 *  attributes are not written as attributes to the netCDF file but can be
 *  displayed as attributes by using the ncdump "-s" option.
 *
 *  _Shuffle	  - A field level attribute specifying if the shuffle filter
 *                  should be enabled, valid values are 'true' or 'false'.
 *                  The shuffle filter can assist with the compression of
 *                  integer data by changing the byte order in the data stream.
 *
 *  _DeflateLevel - A field level attribute specifying the compression level
 *                  between 0 and 9.
 *
 *  _ChunkSizes   - A field level attribute specifying a list of chunk sizes
 *                  for each dimension of the variable.
 *
 *  _Endianness   - A field level attribute specifying the endianness of the
 *                  data values, valid values are 'little', 'big', or 'native'.
 *
 *  _Fletcher32   - A field level attribute specifying if fletcher32 checksums
 *                  should be enabled, valid values are 'true' or 'false'.
 *
 *  _NoFill       - A field level attribute specifying if fill values should
 *                  be disabled, valid values are 'true' or 'false'.
 *
 *  http://www.unidata.ucar.edu/software/netcdf/workshops/2011/utilities/SpecialAttributes.html
 *  http://www.unidata.ucar.edu/software/netcdf/workshops/2012/advanced_utilities/SpecialAtts.html
 * 
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  cds_att  - pointer to the CDS attribute
 *  @param  nc_grpid - NetCDF group id
 *  @param  nc_varid - NetCDF variable id
 *
 *  @return
 *    - 1 if successful
 *    - 0 if a NetCDF error occurred
 */
int _ncds_write_att(
    CDSAtt *cds_att,
    int     nc_grpid,
    int     nc_varid)
{
    CDSAtt    *shuffle_att;
    int        shuffle;
    CDSAtt    *deflate_att;
    int        deflate;
    int        deflate_level;
    size_t    *chunksizes;
    int       *ivals;
    nc_type    nctype;
    size_t     length;
    int        ndims;
    int        special;
    int        status;
    int        di;

    special = 0;
    status  = NC_NOERR;

    if (nc_varid == NC_GLOBAL) {
        if (strcmp(cds_att->name, "_Format") == 0) {
            /* This was handled in ncds_create_file() */
            special = 1;
        }
    }
    else if (strcmp(cds_att->name, "_Storage") == 0) {
        /* We can just ignore this attribute because we will only set this
         * property to NC_CHUNKED when specifying chunk sizes. */
        special = 1;
    }
    else if (strcmp(cds_att->name, "_Shuffle") == 0) {
        /* We will look for this if _DelateLevel was specified
         * because it is only usefull if compression is being done. */
        special = 1;
    }
    else if (strcmp(cds_att->name, "_ChunkSizes") == 0) {

        special = 1;
        length  = 0;
        ivals   = cds_get_att_value(cds_att, CDS_INT, &length, NULL);
        status  = nc_inq_varndims(nc_grpid, nc_varid, &ndims);

        if (status != NC_NOERR) goto NETCDF_ERROR;

        if (length != (size_t)ndims) {

            ERROR( NCDS_LIB_NAME,
                "Invalid length for _ChunkSizes attribute: %d\n"
                " -> a chunk size must be specified for each dimension of the variable\n",
                (int)length);

            return(0);
        }

        if (!(chunksizes = malloc(ndims * sizeof(size_t)))) {

            ERROR( NCDS_LIB_NAME,
                "Could not set variable chunk sizes\n"
                " -> memory allocation error\n");

            return(0);
        }

        for (di = 0; di < ndims; ++di) {
            chunksizes[di] = (size_t)ivals[di];
        }

        status = nc_def_var_chunking(nc_grpid, nc_varid, NC_CHUNKED, chunksizes);

        free(ivals);
        free(chunksizes);
    }
    else if (strcmp(cds_att->name, "_DeflateLevel") == 0) {

        special = 1;

        if (cds_att->value.vp && cds_att->length > 0) {

            shuffle_att = cds_get_att(cds_att->parent, "_Shuffle");
            deflate_att = cds_att;

            status = nc_inq_var_deflate(nc_grpid, nc_varid, &shuffle, NULL, NULL);
            if (status != NC_NOERR) goto NETCDF_ERROR;

            if (shuffle_att) {

                if (shuffle_att->type != CDS_CHAR) {

                    ERROR( NCDS_LIB_NAME,
                        "Invalid data type for _Shuffle attribute: %s\n"
                        " -> this must be a character attribute specifying 'true' or 'false'\n",
                        cds_data_type_name(shuffle_att->type));

                    return(0);
                }

                if (shuffle_att->value.vp && shuffle_att->length > 0) {

                    if (strcmp(shuffle_att->value.cp, "true") == 0) {
                        shuffle = 1;
                    }
                    else if (strcmp(shuffle_att->value.cp, "false") == 0) {
                        shuffle = 0;
                    }
                    else {
                        ERROR( NCDS_LIB_NAME,
                            "Invalid value for _Shuffle attribute: %s\n"
                            " -> expected values are 'true' or 'false'\n",
                            shuffle_att->value.cp);

                        return(0);
                    }
                }
            }

            length = 1;
            cds_get_att_value(deflate_att, CDS_INT, &length, &deflate_level);
            deflate = (deflate_level) ? 1 : 0;

            status = nc_def_var_deflate(nc_grpid, nc_varid, 
                shuffle, deflate, deflate_level);
        }
    }
    else if (strcmp(cds_att->name, "_Endianness") == 0) {

        special = 1;

        if (cds_att->type != CDS_CHAR) {

            ERROR( NCDS_LIB_NAME,
                "Invalid data type for _Endianness attribute: %s\n"
                " -> this must be a character attribute specifying 'little', 'big', or 'native'\n",
                cds_data_type_name(cds_att->type));

            return(0);
        }

        if (cds_att->value.vp && cds_att->length > 0) {

            if (strcmp(cds_att->value.cp, "little") == 0) {
                status = nc_def_var_endian(nc_grpid, nc_varid, NC_ENDIAN_LITTLE);
            }
            else if (strcmp(cds_att->value.cp, "big") == 0) {
                status = nc_def_var_endian(nc_grpid, nc_varid, NC_ENDIAN_BIG);
            }
            else if (strcmp(cds_att->value.cp, "native") == 0) {
                status = nc_def_var_endian(nc_grpid, nc_varid, NC_ENDIAN_NATIVE);
            }
            else {
                ERROR( NCDS_LIB_NAME,
                    "Invalid value for _Endianness attribute: '%s'\n"
                    " -> expected values are 'little', 'big', or 'native'\n",
                    cds_att->value.cp);

                return(0);
            }
        }
    }
    else if (strcmp(cds_att->name, "_Fletcher32") == 0) {

        special = 1;

        if (cds_att->type != CDS_CHAR) {

            ERROR( NCDS_LIB_NAME,
                "Invalid data type for _Fletcher32 attribute: %s\n"
                " -> this must be a character attribute specifying 'true' or 'false'\n",
                cds_data_type_name(cds_att->type));

            return(0);
        }

        if (cds_att->value.vp && cds_att->length > 0) {

            if (strcmp(cds_att->value.cp, "true") == 0) {
                status = nc_def_var_fletcher32(nc_grpid, nc_varid, NC_FLETCHER32);
            }
            else if (strcmp(cds_att->value.cp, "false") == 0) {
                status = nc_def_var_fletcher32(nc_grpid, nc_varid, NC_NOCHECKSUM);
            }
            else {
                ERROR( NCDS_LIB_NAME,
                    "Invalid value for _Fletcher32 attribute: '%s'\n"
                    " -> expected values are 'true' or 'false'\n",
                    cds_att->value.cp);

                return(0);
            }
        }
    }
    else if (strcmp(cds_att->name, "_NoFill") == 0) {

        special = 1;

        if (cds_att->type != CDS_CHAR) {

            ERROR( NCDS_LIB_NAME,
                "Invalid data type for _NoFill attribute: %s\n"
                " -> this must be a character attribute specifying 'true' or 'false'\n",
                cds_data_type_name(cds_att->type));

            return(0);
        }

        if (cds_att->value.vp && cds_att->length > 0) {

            if (strcmp(cds_att->value.cp, "true") == 0) {
                status = nc_def_var_fill(nc_grpid, nc_varid, 1, NULL);
            }
            else if (strcmp(cds_att->value.cp, "false") == 0) {
                status = nc_def_var_fill(nc_grpid, nc_varid, 0, NULL);
            }
            else {
                ERROR( NCDS_LIB_NAME,
                    "Invalid value for _NoFill attribute: '%s'\n"
                    " -> expected values are 'true' or 'false'\n",
                    cds_att->value.cp);

                return(0);
            }
        }
    }

    /* Define the attribute in the NetCDF file if it was not one of the
     * special attributes we already handled above. */

    if (!special) {

        nctype = ncds_nc_type(cds_att->type);

        switch (cds_att->type) {
            case CDS_BYTE:
                status = nc_put_att_schar(nc_grpid, nc_varid, cds_att->name,
                    nctype, cds_att->length, cds_att->value.bp);
                break;
            case CDS_CHAR:
                status = nc_put_att_text(nc_grpid, nc_varid, cds_att->name,
                    cds_att->length, cds_att->value.cp);
                break;
            case CDS_SHORT:
                status = nc_put_att_short(nc_grpid, nc_varid, cds_att->name,
                    nctype, cds_att->length, cds_att->value.sp);
                break;
            case CDS_INT:
                status = nc_put_att_int(nc_grpid, nc_varid, cds_att->name,
                    nctype, cds_att->length, cds_att->value.ip);
                break;
            case CDS_FLOAT:
                status = nc_put_att_float(nc_grpid, nc_varid, cds_att->name,
                    nctype, cds_att->length, cds_att->value.fp);
                break;
            case CDS_DOUBLE:
                status = nc_put_att_double(nc_grpid, nc_varid, cds_att->name,
                    nctype, cds_att->length, cds_att->value.dp);
                break;
            default:
                status = NC_EBADTYPE;
                break;
        }
    }
    
    if (status == NC_NOERR) {
        return(1);
    }

NETCDF_ERROR: 

    ERROR( NCDS_LIB_NAME,
        "Could not define attribute\n"
        " -> nc_grpid = %d, nc_varid = %d, att_name = '%s'\n"
        " -> %s\n",
        nc_grpid, nc_varid, cds_att->name, nc_strerror(status));

    return(0);
}

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Create a new NetCDF file.
 *
 *  This function will create a new NetCDF file using the specified CDS group.
 *
 *  By default (cmode = 0) the output file will be in the classic NetCDF
 *  format and will overwrite an existing file. Possible cmode flags include:
 *
 *      - NC_NOCLOBBER     = do not overwrite an existing file
 *
 *      - NC_NETCDF4       = create a HDF5/NetCDF-4 file
 *
 *      - NC_CLASSIC_MODEL = causes netCDF to enforce the classic data model
 *                           for netCDF-4/HDF5 files
 *
 *      - NC_64BIT_OFFSET  = create a 64-bit offset format file
 *
 *  If the specified cmode does not have a NC_NETCDF4 or NC_64BIT_OFFSET flag
 *  set, the global _Format attribute will be used if it exists. The valid
 *  values for the _Format attribute are: 'classic', '64-bit offset',
 *  'netCDF-4', or 'netCDF-4 classic model'.  The _Format attribute itself
 *  will not be stored in the netcdf file.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  cds_group   - pointer to the CDS group
 *  @param  nc_file     - full path to the NetCDF file
 *  @param  cmode       - creation mode flag
 *  @param  recursive   - recurse into all subroups (1 = TRUE, 0 = FALSE)
 *  @param  header_only - only write out the NetCDF header (1 = TRUE, 0 = FALSE)
 *
 *  @return
 *    - NetCDF id of the root group in the file
 *    - 0 if an error occurred
 */
int ncds_create_file(
    CDSGroup   *cds_group,
    const char *nc_file,
    int         cmode,
    int         recursive,
    int         header_only)
{
    int     ncid;
    char   *nc_dir;
    CDSAtt *nc_format_att;
    char   *nc_format;
    char   *slash;
    char    errstr[MAX_LOG_ERROR];

    /* Make sure the path to the NetCDF file exists */

    nc_dir = strdup(nc_file);
    if (!nc_dir) {

        ERROR( NCDS_LIB_NAME,
            "Could not create netcdf file: %s\n"
            " -> memory allocation error\n",
            nc_file);

        return(0);
    }

    slash = strrchr(nc_dir, '/');

    if (slash) {

        *slash = '\0';
        if (!msngr_make_path(nc_dir, 00775, MAX_LOG_ERROR, errstr)) {
            ERROR( NCDS_LIB_NAME, errstr);
            free(nc_dir);
            return(0);
        }
    }

    free(nc_dir);

    /* Set the creation mode flags */

    if (!(cmode & NC_NETCDF4 || cmode & NC_64BIT_OFFSET)) {

        nc_format_att = cds_get_att(cds_group, "_Format");
        if (nc_format_att) {

            if (nc_format_att->type != CDS_CHAR) {

                ERROR( NCDS_LIB_NAME,
                    "Invalid data type for global _Format attribute: %s\n",
                    cds_data_type_name(nc_format_att->type));

                return(0);
            }

            nc_format = nc_format_att->value.cp;

            if (strcmp(nc_format, "netCDF-4 classic model") == 0) {
                cmode |= NC_NETCDF4 | NC_CLASSIC_MODEL;
            }
            else if (strcmp(nc_format, "netCDF-4") == 0) {
                cmode |= NC_NETCDF4;
            }
            else if (strcmp(nc_format, "64-bit offset") == 0) {
                cmode |= NC_64BIT_OFFSET;
            }
            else if (strcmp(nc_format, "classic") != 0) {

                ERROR( NCDS_LIB_NAME,
                    "Invalid value for global _Format attribute: '%s'\n",
                    nc_format);

                return(0);
            }
        }
    }

    /* Create the NetCDF file */

    if (!ncds_create(nc_file, cmode, &ncid)) {
        return(0);
    }

    /* Write the NetCDF header */

    if (!ncds_write_group(cds_group, ncid, recursive)) {
        ncds_close(ncid);
        unlink(nc_file);
        return(0);
    }

    if (!ncds_enddef(ncid)) {
        ncds_close(ncid);
        unlink(nc_file);
        return(0);
    }

    /* Write the NetCDF data */

    if (!header_only) {

        if (!ncds_write_group_data(cds_group, 0, ncid, 0, 0, recursive)) {
            ncds_close(ncid);
            unlink(nc_file);
            return(0);
        }
    }

    return(ncid);
}

/**
 *  Write a dimension definition from a CDS group into a NetCDF group.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  cds_dim  - pointer to the CDS dimension
 *  @param  nc_grpid - NetCDF group id
 *  @param  nc_dimid - output: the NetCDF dimension id if not NULL
 *
 *  @return
 *    - 1 if successful
 *    - 0 if a NetCDF error occurred
 */
int ncds_write_dim(
    CDSDim *cds_dim,
    int     nc_grpid,
    int    *nc_dimid)
{
    int    status;
    size_t length;
    int    dimid;

    if (nc_dimid) {
        *nc_dimid = -1;
    }

    length = (cds_dim->is_unlimited) ? 0 : cds_dim->length;

    status = nc_def_dim(nc_grpid, cds_dim->name, length, &dimid);
    if (status != NC_NOERR) {

        ERROR( NCDS_LIB_NAME,
            "Could not define dimension\n"
            " -> nc_grpid = %d, dim_name = '%s'\n"
            " -> %s\n",
            nc_grpid, cds_dim->name, nc_strerror(status));

        return(0);
    }

    if (nc_dimid) {
        *nc_dimid = dimid;
    }

    return(1);
}

/**
 *  Write all dimension definitions from a CDS group into a NetCDF group.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  cds_group - pointer to the CDS group
 *  @param  nc_grpid  - NetCDF group id
 *
 *  @return
 *    - 1 if successful
 *    - 0 if a NetCDF error occurred
 */
int ncds_write_dims(
    CDSGroup *cds_group,
    int       nc_grpid)
{
    int index;
    int dimid;

    for (index = 0; index < cds_group->ndims; index++) {
        if (!ncds_write_dim(cds_group->dims[index], nc_grpid, &dimid)) {
            return(0);
        }
    }

    return(1);
}

/**
 *  Write an attribute definition from a CDS group into a NetCDF group.
 *
 *  The following special attributes are recognized by this function. These
 *  attributes are not written as attributes to the netCDF file but can be
 *  displayed as attributes by using the ncdump "-s" option.
 *
 *  _Shuffle	  - A field level attribute specifying if the shuffle filter
 *                  should be enabled, valid values are 'true' or 'false'.
 *                  The shuffle filter can assist with the compression of
 *                  integer data by changing the byte order in the data stream.
 *
 *  _DeflateLevel - A field level attribute specifying the compression level
 *                  between 0 and 9.
 *
 *  _ChunkSizes   - A field level attribute specifying a list of chunk sizes
 *                  for each dimension of the variable.
 *
 *  _Endianness   - A field level attribute specifying the endianness of the
 *                  data values, valid values are 'little', 'big', or 'native'.
 *
 *  _Fletcher32   - A field level attribute specifying if fletcher32 checksums
 *                  should be enabled, valid values are 'true' or 'false'.
 *
 *  _NoFill       - A field level attribute specifying if fill values should
 *                  be disabled, valid values are 'true' or 'false'.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  cds_att  - pointer to the CDS attribute
 *  @param  nc_grpid - NetCDF file id
 *
 *  @return
 *    - 1 if successful
 *    - 0 if a NetCDF error occurred
 */
int ncds_write_att(
    CDSAtt *cds_att,
    int     nc_grpid)
{
    return(_ncds_write_att(cds_att, nc_grpid, NC_GLOBAL));
}

/**
 *  Write all attribute definitions from a CDS group into a NetCDF group.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  cds_group - pointer to the CDS group
 *  @param  nc_grpid  - NetCDF group id
 *
 *  @return
 *    - 1 if successful
 *    - 0 if a NetCDF error occurred
 */
int ncds_write_atts(
    CDSGroup *cds_group,
    int       nc_grpid)
{
    int index;

    for (index = 0; index < cds_group->natts; index++) {
        if (!ncds_write_att(cds_group->atts[index], nc_grpid)) {
            return(0);
        }
    }

    return(1);
}

/**
 *  Write a variable definition from a CDS group into a NetCDF group.
 *
 *  This function will define any dimensions used by the variable
 *  that have not already been defined. It will also define all
 *  the variable attributes.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  cds_var  - pointer to the CDS variable
 *  @param  nc_grpid - NetCDF group id
 *  @param  nc_varid - output: the NetCDF variable id if not NULL
 *
 *  @return
 *    - 1 if successful
 *    - 0 if a NetCDF error occurred
 */
int ncds_write_var(
    CDSVar *cds_var,
    int     nc_grpid,
    int    *nc_varid)
{
    int      status;
    int      dim_index;
    CDSDim  *dim;
    int      dimids[NC_MAX_DIMS];
    nc_type  nctype;
    int      varid;
    int      attid;

    if (nc_varid) {
        *nc_varid = -1;
    }

    /* Create the dimids array */

    for (dim_index = 0; dim_index < cds_var->ndims; dim_index++) {

        dim = cds_var->dims[dim_index];

        /* Make sure the dimension is defined in the NetCDF file */

        status = ncds_inq_dimid(nc_grpid, dim->name, &dimids[dim_index]);
        if (status == 0) {
            if (!ncds_write_dim(dim, nc_grpid, &dimids[dim_index])) {
                return(0);
            }
        }
        else if (status < 0) {
            return(0);
        }
    }

    /* Define the variable in the NetCDF group */

    nctype = ncds_nc_type(cds_var->type);

    status = nc_def_var(
        nc_grpid, cds_var->name, nctype, cds_var->ndims, dimids, &varid);

    if (status != NC_NOERR) {

        ERROR( NCDS_LIB_NAME,
            "Could not define variable\n"
            " -> nc_grpid = %d, var_name = '%s'\n"
            " -> %s\n",
            nc_grpid, cds_var->name, nc_strerror(status));

        return(0);
    }

    if (nc_varid) {
        *nc_varid = varid;
    }

    /* Define the variable attributes */

    for (attid = 0; attid < cds_var->natts; attid++) {
        if (!_ncds_write_att(cds_var->atts[attid], nc_grpid, varid)) {
            return(0);
        }
    }

    return(1);
}

/**
 *  Write all variable definitions from a CDS group into a NetCDF group.
 *
 *  This function will define any dimensions used by the variables
 *  that have not already been defined. It will also define all
 *  the variable attributes.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  cds_group - pointer to the CDS group
 *  @param  nc_grpid  - NetCDF group id
 *
 *  @return
 *    - 1 if successful
 *    - 0 if a NetCDF error occurred
 */
int ncds_write_vars(
    CDSGroup *cds_group,
    int       nc_grpid)
{
    int index;
    int varid;

    for (index = 0; index < cds_group->nvars; index++) {
        if (!ncds_write_var(cds_group->vars[index], nc_grpid, &varid)) {
            return(0);
        }
    }

    return(1);
}

/**
 *  Write a CDS group definition into a NetCDF group.
 *
 *  This function will write out all dimension, attribute, and variable
 *  definitions from a CDS group into a NetCDF group.  It will also recurse
 *  into subgroups if recursive is TRUE.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  cds_group - pointer to the CDS group
 *  @param  nc_grpid  - NetCDF group id
 *  @param  recursive - recurse into subroups (1 = TRUE, 0 = FALSE)
 *
 *  @return
 *    - 1 if successful
 *    - 0 if a NetCDF error occurred
 */
int ncds_write_group(
    CDSGroup *cds_group,
    int       nc_grpid,
    int       recursive)
{
    int       status;
    int       index;
    int       id;
    int       subgrpid;
    CDSGroup *subgroup;

    /* Write dimensions */

    for (index = 0; index < cds_group->ndims; index++) {
        if (!ncds_write_dim(cds_group->dims[index], nc_grpid, &id)) {
            return(0);
        }
    }

    /* Write attributes */

    for (index = 0; index < cds_group->natts; index++) {
        if (!ncds_write_att(cds_group->atts[index], nc_grpid)) {
            return(0);
        }
    }

    /* Write variables */

    for (index = 0; index < cds_group->nvars; index++) {
        if (!ncds_write_var(cds_group->vars[index], nc_grpid, &id)) {
            return(0);
        }
    }

    /* Write subgroups */

    if (recursive) {

        for (index = 0; index < cds_group->ngroups; index++) {

            subgroup = cds_group->groups[index];

            status = nc_def_grp(nc_grpid, subgroup->name, &subgrpid);
            if (status != NC_NOERR) {

                ERROR( NCDS_LIB_NAME,
                    "Could not define group\n"
                    " -> nc_grpid = %d, group_name = '%s'\n"
                    " -> %s\n",
                    nc_grpid, subgroup->name, nc_strerror(status));

                return(0);
            }

            if (!ncds_write_group(subgroup, subgrpid, recursive)) {
                return(0);
            }
        }
    }

    return(1);
}

/**
 *  Write data from a CDS variable into a NetCDF variable.
 *
 *  This function will also do the necessary type, units, and missing
 *  value conversions,
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  cds_var          - pointer to the CDS variable
 *  @param  cds_sample_start - CDS variable start sample
 *  @param  nc_grpid         - NetCDF group id
 *  @param  nc_varid         - NetCDF variable id
 *  @param  nc_start         - NetCDF dimension start indexes
 *  @param  nc_count         - number of values to write along each dimension
 *
 *  @return
 *    - 1 if successful
 *    - 0 if a NetCDF error occurred
 */
int ncds_write_var_data(
    CDSVar *cds_var,
    size_t  cds_sample_start,
    int     nc_grpid,
    int     nc_varid,
    size_t *nc_start,
    size_t *nc_count)
{
    nc_type           nc_var_type;
    CDSDataType       nc_cds_type;
    size_t            nc_type_size;
    size_t            cds_type_size;

    int               map_missing;
    int               cds_nmv;
    void             *cds_mv;
    int               nc_nmv;
    void             *nc_mv;
    void             *new_nc_mv;
    void             *mvp;
    int               mi;

    CDSUnitConverter  converter;
    char             *nc_units;
    const char       *cds_units;

    size_t            cds_sample_count;
    size_t            cds_sample_size;
    void             *cds_datap;
    void             *nc_datap;
    size_t            length;

    int               status;

    /* Get the netcdf variable data type */

    if (!ncds_inq_vartype(nc_grpid, nc_varid, &nc_var_type)) {
        return(0);
    }

    nc_cds_type = ncds_cds_type(nc_var_type);
    if (nc_cds_type == CDS_NAT) {

        ERROR( NCDS_LIB_NAME,
            "Could not write variable data\n"
            " -> nc_grpid = %d, nc_varid = %d, cds_var = '%s'\n"
            " -> unsupported netcdf data type (%d)\n",
            nc_grpid, nc_varid, cds_var->name, nc_var_type);

        return(0);
    }

    nc_type_size  = cds_data_type_size(nc_cds_type);
    cds_type_size = cds_data_type_size(cds_var->type);

    /* Check if we need to map the missing values in the CDS variable
     * data to the missing values in the NetCDF variable data */

    map_missing = 0;
    nc_mv       = (void *)NULL;
    nc_nmv      = 0;

    cds_nmv = cds_get_var_missing_values(cds_var, &cds_mv);
    if (cds_nmv < 0) {
        return(0);
    }
    else if (cds_nmv > 0) {

        nc_nmv = ncds_get_missing_values(nc_grpid, nc_varid, &nc_mv);
        if (nc_nmv < 0) {
            if (cds_nmv) free(cds_mv);
            return(0);
        }

        /* Get the missing values to use in the NetCDF variable data */

        if (nc_nmv < cds_nmv) {

            new_nc_mv = realloc(nc_mv, cds_nmv * nc_type_size);

            if (!new_nc_mv) {

                ERROR( NCDS_LIB_NAME,
                    "Could not write variable data\n"
                    " -> nc_grpid = %d, nc_varid = %d, cds_var = '%s'\n"
                    " -> memory allocation error\n",
                    nc_grpid, nc_varid, cds_var->name);

                if (cds_nmv) free(cds_mv);
                if (nc_nmv)  free(nc_mv);
                return(0);
            }

            nc_mv = new_nc_mv;

            if (!nc_nmv) {
                ncds_get_default_fill_value(nc_var_type, nc_mv);
                nc_nmv = 1;
            }

            for (mi = nc_nmv; mi < cds_nmv; ++mi) {
                mvp = (void *)((char *)nc_mv + mi * nc_type_size);
                memcpy(mvp, nc_mv, nc_type_size);
            }
        }

        map_missing = 1;

        if (nc_cds_type == cds_var->type) {
            if (memcmp(cds_mv, nc_mv, cds_nmv * cds_type_size) == 0) {
                map_missing = 0;
            }
        }
    }

    /* Check if we need to do a units conversion */

    converter = (CDSUnitConverter)NULL;

    cds_units = cds_get_var_units(cds_var);
    if (cds_units) {

        status = ncds_get_var_units(nc_grpid, nc_varid, &nc_units);
        if (status < 0) {
            if (cds_nmv) free(cds_mv);
            if (nc_nmv)  free(nc_mv);
            return(0);
        }
        else if (status > 0) {
            status = cds_get_unit_converter(cds_units, nc_units, &converter);
            if (status < 0) {
                free(nc_units);
                if (cds_nmv) free(cds_mv);
                if (nc_nmv)  free(nc_mv);
                return(0);
            }
        }

        free(nc_units);
    }

    /* Get pointer to the start of the variable data to write */

    cds_sample_count = nc_count[0];
    cds_sample_size  = cds_var_sample_size(cds_var);
    cds_datap        = cds_var->data.bp
                     + (cds_sample_start * cds_sample_size * cds_type_size);

    /* Check if we need to do any conversions */

    nc_datap = cds_datap;

    if (converter || map_missing || (nc_cds_type != cds_var->type)) {

        length = cds_sample_count * cds_sample_size;

        nc_datap = malloc(length * nc_type_size);

        if (!nc_datap) {

            ERROR( NCDS_LIB_NAME,
                "Could not write variable data\n"
                " -> nc_grpid = %d, nc_varid = %d, cds_var = '%s'\n"
                " -> memory allocation error\n",
                nc_grpid, nc_varid, cds_var->name);

            if (converter) cds_free_unit_converter(converter);
            if (cds_nmv)   free(cds_mv);
            if (nc_nmv)    free(nc_mv);
            return(0);
        }

        if (converter) {
            cds_convert_units(converter,
                cds_var->type, length, cds_datap, nc_cds_type, nc_datap,
                cds_nmv, cds_mv, nc_mv,
                NULL, nc_mv, NULL, nc_mv);
        }
        else {
            cds_copy_array(
                cds_var->type, length, cds_datap, nc_cds_type, nc_datap,
                cds_nmv, cds_mv, nc_mv,
                NULL, nc_mv, NULL, nc_mv);
        }
    }

    /* Write the data to the NetCDF file */

    status = nc_put_vara(nc_grpid, nc_varid, nc_start, nc_count, nc_datap);

    /* Cleanup and return */

    if (converter) cds_free_unit_converter(converter);
    if (cds_nmv)   free(cds_mv);
    if (nc_nmv)    free(nc_mv);

    if (nc_datap != cds_datap) free(nc_datap);

    if (status != NC_NOERR) {

        ERROR( NCDS_LIB_NAME,
            "Could not write variable data\n"
            " -> nc_grpid = %d, nc_varid = %d, cds_var = '%s'\n"
            " -> %s\n",
            nc_grpid, nc_varid, cds_var->name, nc_strerror(status));

        return(0);
    }

    return(1);
}

/**
 *  Write data from a CDS variable into a NetCDF variable.
 *
 *  If sample_count is NULL or the value pointed to by sample_count is 0,
 *  the number of available samples will be written.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  cds_var          - pointer to the CDS variable
 *  @param  cds_sample_start - CDS variable start sample
 *  @param  nc_grpid         - NetCDF group id
 *  @param  nc_varid         - NetCDF variable id
 *  @param  nc_sample_start  - NetCDF variable start sample
 *  @param  sample_count     - NULL or
 *                               - input:  number of samples to write
 *                               - output: number of samples actually written
 *
 *  @return
 *    - 1 if successful
 *    - 0 if a NetCDF or CDS error occurred
 */
int ncds_write_var_samples(
    CDSVar *cds_var,
    size_t  cds_sample_start,
    int     nc_grpid,
    int     nc_varid,
    size_t  nc_sample_start,
    size_t *sample_count)
{
    int     ndims;
    int     dimids[NC_MAX_DIMS];
    int     dim_index;
    int     dimid;
    CDSDim *dim;
    size_t  dim_length;
    size_t  start[NC_MAX_DIMS];
    size_t  count[NC_MAX_DIMS];
    int     retval;

    int     nunlim_dimids;
    int     unlim_dimids[NC_MAX_DIMS];
    int     ud_index;
    int     is_unlimited;

    /* Get the number of NetCDF variable dimensions */

    if (!ncds_inq_varndims(nc_grpid, nc_varid, &ndims)) {
        return(0);
    }

    /* Make sure the NetCDF and CDS variables have the same number of dims */

    if (ndims != cds_var->ndims) {

        ERROR( NCDS_LIB_NAME,
            "Incompatible variable shapes\n"
            " -> nc_grpid = %d, nc_varid = %d, cds_var = '%s'\n"
            " -> number of CDS dims (%d) <> number of netcdf dims (%d)\n",
            nc_grpid, nc_varid, cds_var->name, cds_var->ndims, ndims);

        return(0);
    }

    /* Check if this is a variable with no dimensions */

    if (ndims == 0) {

        start[0] = 0;
        count[0] = 1;

        if (sample_count) {
            *sample_count = 1;
        }

        retval = ncds_write_var_data(
            cds_var, 0, nc_grpid, nc_varid, start, count);

        return(retval);
    }

    /* Get the NetCDF variable dimension ids */

    if (!ncds_inq_vardimids(nc_grpid, nc_varid, dimids)) {
        return(0);
    }

    /* Get the ids of the unlimited dimensions */

    if (!ncds_inq_unlimdims(nc_grpid, &nunlim_dimids, unlim_dimids)) {
        return(0);
    }

    /* Create start and count arrays */

    for (dim_index = 0; dim_index < ndims; dim_index++) {

        dimid = dimids[dim_index];
        dim   = cds_var->dims[dim_index];

        /* Get the NetCDF dimension length */

        if (!ncds_inq_dimlen(nc_grpid, dimid, &dim_length)) {
            return(0);
        }

        /* Check if this is an unlimited dimension */

        is_unlimited = 0;

        for (ud_index = 0; ud_index < nunlim_dimids; ud_index++) {
            if (dimid == unlim_dimids[ud_index]) {
                is_unlimited = 1;
                break;
            }
        }

        if (dim_index == 0) {

            if (cds_sample_start >= cds_var->sample_count) {

                ERROR( NCDS_LIB_NAME,
                    "Invalid CDS variable start sample\n"
                    " -> var_name = '%s', dim_name = '%s'\n"
                    " -> start sample (%d) >= sample count (%d)\n",
                    cds_var->name, dim->name,
                    cds_sample_start, cds_var->sample_count);

                return(0);
            }

            start[dim_index] = nc_sample_start;
            count[dim_index] = cds_var->sample_count - cds_sample_start;

            if (is_unlimited == 0) {

                if (nc_sample_start >= dim_length) {

                    ERROR( NCDS_LIB_NAME,
                        "Invalid netcdf variable start sample\n"
                        " -> nc_grpid = %d, nc_varid = %d, nc_dimid = %d\n"
                        " -> start sample (%d) >= dimension length (%d)\n",
                        nc_grpid, nc_varid, dimid,
                        nc_sample_start, dim_length);

                    return(0);
                }

                if (count[dim_index] > dim_length - nc_sample_start) {
                    count[dim_index] = dim_length - nc_sample_start;
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

                return(0);
            }

            start[dim_index] = 0;
            count[dim_index] = dim->length;
        }
    }

    /* At this point count[0] will be the maximum number of samples
     * that can be written. */

    if (sample_count) {

        if (*sample_count > 0 &&
            *sample_count < count[0]) {

            count[0] = *sample_count;
        }
        else {
            *sample_count = count[0];
        }
    }

    /* Write the data */

    retval = ncds_write_var_data(
        cds_var, cds_sample_start, nc_grpid, nc_varid, start, count);

    return(retval);
}

/**
 *  Write static data from a CDS group into a NetCDF group.
 *
 *  This function will write out the data for each variable defined
 *  in the CDS group that do not have an unlimited dimension.
 *  Variables that are defined in the CDS group but not in the
 *  NetCDF group will be ignored.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  cds_group - pointer to the CDS group
 *  @param  nc_grpid  - NetCDF group id
 *
 *  @return
 *    - 1 if successful
 *    - 0 if a NetCDF or CDS error occurred
 */
int ncds_write_static_data(
    CDSGroup *cds_group,
    int       nc_grpid)
{
    CDSVar *var;
    int     index;
    int     status;
    int     varid;

    for (index = 0; index < cds_group->nvars; index++) {

        var = cds_group->vars[index];

        if (var->sample_count &&
           (var->ndims == 0 || var->dims[0]->is_unlimited == 0)) {

            status = ncds_inq_varid(nc_grpid, var->name, &varid);

            if (status == 1) {
                if (!ncds_write_var_samples(var, 0, nc_grpid, varid, 0, NULL)) {
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
 *  Write data records from a CDS group into a NetCDF group.
 *
 *  This function will write out the data for each variable defined
 *  in the CDS group that have an unlimited dimension. Variables
 *  that are defined in the CDS group but not in the NetCDF group
 *  will be ignored.
 *
 *  If record_count is 0, the number of available records will be written.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  cds_group        - pointer to the CDS group
 *  @param  cds_record_start - CDS start record
 *  @param  nc_grpid         - NetCDF group id
 *  @param  nc_record_start  - NetCDF start record
 *  @param  record_count     - number of records to write
 *
 *  @return
 *    - 1 if successful
 *    - 0 if a NetCDF or CDS error occurred
 */
int ncds_write_records(
    CDSGroup *cds_group,
    size_t    cds_record_start,
    int       nc_grpid,
    size_t    nc_record_start,
    size_t    record_count)
{
    CDSVar *var;
    int     index;
    int     status;
    int     varid;
    size_t  count;

    for (index = 0; index < cds_group->nvars; index++) {

        var = cds_group->vars[index];

        if (var->sample_count &&
            var->ndims        &&
            var->dims[0]->is_unlimited) {

            status = ncds_inq_varid(nc_grpid, var->name, &varid);

            if (status == 1) {

                count = record_count;

                if (!ncds_write_var_samples(
                    var, cds_record_start,
                    nc_grpid, varid, nc_record_start, &count)) {

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
 *  Write data from a CDS group into a NetCDF group.
 *
 *  This function will write out the data for each variable defined
 *  in the CDS group. Variables and groups that are defined in the
 *  CDS group but not in the NetCDF group will be ignored.
 *
 *  If record_count is 0, the number of available records will be written.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  cds_group        - pointer to the CDS group
 *  @param  cds_record_start - CDS start record
 *  @param  nc_grpid         - NetCDF group id
 *  @param  nc_record_start  - NetCDF start record
 *  @param  record_count     - number of records to read
 *  @param  recursive        - recurse into subroups (1 = TRUE, 0 = FALSE)
 *
 *  @return
 *    - 1 if successful
 *    - 0 if a NetCDF or CDS error occurred
 */
int ncds_write_group_data(
    CDSGroup *cds_group,
    size_t    cds_record_start,
    int       nc_grpid,
    size_t    nc_record_start,
    size_t    record_count,
    int       recursive)
{
    int       index;
    int       status;
    CDSGroup *subgroup;
    int       subgrpid;

    if (!ncds_write_static_data(cds_group, nc_grpid)) {
        return(0);
    }

    if (!ncds_write_records(
        cds_group, cds_record_start, nc_grpid, nc_record_start, record_count)) {

        return(0);
    }

    if (recursive) {

        for (index = 0; index < cds_group->ngroups; index++) {

            subgroup = cds_group->groups[index];

            status = ncds_inq_grpid(nc_grpid, subgroup->name, &subgrpid);

            if (status == 1) {
                if (!ncds_write_group_data(
                    subgroup, cds_record_start, subgrpid, nc_record_start,
                    record_count, recursive)) {

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
