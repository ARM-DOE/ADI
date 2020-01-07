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
*    $Revision: 12565 $
*    $Author: ermold $
*    $Date: 2012-02-05 03:04:25 +0000 (Sun, 05 Feb 2012) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file ncds_private.h
 *  Private NCDS Functions.
 */

#ifndef _NCDS_PRIVATE_
#define _NCDS_PRIVATE_

/** @privatesection */

/*****  Data Type Functions  *****/

void       *_ncds_data_type_min(nc_type type);
void       *_ncds_data_type_max(nc_type type);
void       *_ncds_default_fill_value(nc_type type);

/*****  Read Functions  *****/

CDSAtt *_ncds_read_att(
            int         nc_grpid,
            int         nc_varid,
            int         nc_attid,
            void       *cds_object,
            const char *cds_att_name);

/*****  Write Functions  *****/

int     _ncds_write_att(
            CDSAtt *cds_att,
            int     nc_grpid,
            int     nc_varid);

/*****  DataStream Functions  *****/

NCDatastream *_ncds_create_datastream(
            const char *name,
            const char *path,
            const char *extension);

void    _ncds_destroy_datastream(
            NCDatastream *datastream);

#endif /* _NCDS_PRIVATE_ */
