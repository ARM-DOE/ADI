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
*    $Revision: 15832 $
*    $Author: ermold $
*    $Date: 2012-11-09 01:10:42 +0000 (Fri, 09 Nov 2012) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/**
 *  @file ncwrap_dataset.c
 *  Wrappers for NetCDF dataset functions.
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
 *  Close a NetCDF file.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  ncid - NetCDF id of the root group
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occured
 */
int ncds_close(int ncid)
{
    int status = nc_close(ncid);

    if (status != NC_NOERR) {

        ERROR( NCDS_LIB_NAME,
            "Error closing netcdf file: ncid = %d\n"
            " -> %s\n",
            ncid, nc_strerror(status));

        return(0);
    }

    return(1);
}

/**
 *  Create a NetCDF file.
 *
 *  By default (cmode = 0) the NetCDF file will be in classic format
 *  and it will overwrite any existing file with the same name.
 *  Possible cmode flags include:
 *
 *    - NC_NOCLOBBER     = Do not overwrite an existing file
 *
 *    - NC_SHARE         = This flag should be used when one process may be
 *                         writing the dataset and one or more other processes
 *                         reading the dataset concurrently.
 *
 *    - NC_64BIT_OFFSET  = Create a 64-bit offset format file.
 *
 *    - NC_NETCDF4       = Create a HDF5/NetCDF-4 file.
 *
 *    - NC_CLASSIC_MODEL = Enforce the classic data model in this file.
 *
 *  See the NetCDF "C Interface Guide" for more detailed descriptions
 *  of the creation mode flags.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  file  - full path to the NetCDF file
 *  @param  cmode - creation mode of the file
 *  @param  ncid  - output: NetCDF id of the root group
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occured
 */
int ncds_create(const char *file, int cmode, int *ncid)
{
    int status = nc_create(file, cmode, ncid);

    if (status != NC_NOERR) {

        ERROR( NCDS_LIB_NAME,
            "Could not create netcdf file: %s\n"
            " -> %s\n",
            file, nc_strerror(status));

        return(0);
    }

    return(1);
}

/**
 *  End define mode for an open NetCDF file.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  ncid - NetCDF id of the root group
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occured
 */
int ncds_enddef(int ncid)
{
    int status = nc_enddef(ncid);

    if (status != NC_ENOTINDEFINE &&
        status != NC_NOERR) {

        ERROR( NCDS_LIB_NAME,
            "Could not end define mode for netcdf file: ncid = %d\n"
            " -> %s\n",
            ncid, nc_strerror(status));

        return(0);
    }

    return(1);
}

/**
 *  Get the format of a NetCDF file.
 *
 *  @param  ncid   - NetCDF id of the root group
 *  @param  format - output: format of the NetCDF file:
 *
 *                        - NC_FORMAT_CLASSIC
 *                        - NC_FORMAT_64BIT
 *                        - NC_FORMAT_NETCDF4
 *                        - NC_FORMAT_NETCDF4_CLASSIC
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occured
 */
int ncds_format(int ncid, int *format)
{
    int status;

    status = nc_inq_format(ncid, format);

    if (status != NC_NOERR) {

        ERROR( NCDS_LIB_NAME,
            "Could not get netcdf file format: ncid = %d\n"
            " -> %s\n",
            ncid, nc_strerror(status));

        return(0);
    }

    return(1);
}

/**
 *  Open a NetCDF file.
 *
 *  By default (omode = 0) the NetCDF file will be opened with
 *  read-only access. Possible omode flags include:
 *
 *    - NC_WRITE = Open the dataset with read-write access.
 *
 *    - NC_SHARE = This flag should be used when one process may be
 *                 writing the dataset and one or more other processes
 *                 reading the dataset concurrently.
 *
 *  See the NetCDF "C Interface Guide" for more detailed descriptions
 *  of the open mode flags.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  file  - full path to the NetCDF file
 *  @param  omode - open mode of the file
 *  @param  ncid  - output: NetCDF id of the root group
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occured
 */
int ncds_open(const char *file, int omode, int *ncid)
{
    int status = nc_open(file, omode, ncid);

    if (status != NC_NOERR) {

        ERROR( NCDS_LIB_NAME,
            "Could not open netcdf file: %s\n"
            " -> %s\n",
            file, nc_strerror(status));

        return(0);
    }

    return(1);
}

/**
 *  Put an open NetCDF file in define mode.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  ncid - NetCDF id of the root group
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occured
 */
int ncds_redef(int ncid)
{
    int status = nc_redef(ncid);

    if (status != NC_EINDEFINE &&
        status != NC_NOERR) {

        ERROR( NCDS_LIB_NAME,
            "Could not put netcdf file in define mode: ncid = %d\n"
            " -> %s\n",
            ncid, nc_strerror(status));

        return(0);
    }

    return(1);
}

/**
 *  Flush NetCDF data to disk, or make newly stored data available.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  ncid - NetCDF id of the root group
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occured
 */
int ncds_sync(int ncid)
{
    int status = nc_sync(ncid);

    if (status != NC_NOERR) {

        ERROR( NCDS_LIB_NAME,
            "Could not sync netcdf file: ncid = %d\n"
            " -> %s\n",
            ncid, nc_strerror(status));

        return(0);
    }

    return(1);
}
