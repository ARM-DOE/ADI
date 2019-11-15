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
*    $Revision: 70612 $
*    $Author: ermold $
*    $Date: 2016-05-19 19:32:02 +0000 (Thu, 19 May 2016) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file ncds3.h
 *  NCDS Library Header File.
 */

#ifndef _NCDS3_H_
#define _NCDS3_H_

#include "netcdf.h"
#include "cds3.h"

/** NCDS library name. */
#define NCDS_LIB_NAME "libncds3"

/******************************************************************************/
/**
 *  @defgroup NCDS_DATA_TYPES Data Types
 */
/*@{*/

CDSDataType ncds_cds_type(nc_type nctype);
nc_type     ncds_nc_type(CDSDataType cds_type);

void        ncds_get_default_fill_value(nc_type type, void *value);

/*@}*/

/******************************************************************************/
/**
 *  @defgroup NCDS_READ NetCDF Read
 */
/*@{*/

CDSGroup *ncds_read_file(
            const char *nc_file,
            int         recursive,
            int         header_only,
            int        *nc_format,
            CDSGroup   *cds_group);

CDSDim *ncds_read_dim(
            int         nc_grpid,
            int         nc_dimid,
            CDSGroup   *cds_group,
            const char *cds_dim_name);

int     ncds_read_dims(
            int       nc_grpid,
            CDSGroup *cds_group);

CDSAtt *ncds_read_att(
            int         nc_grpid,
            int         nc_attid,
            CDSGroup   *cds_group,
            const char *cds_att_name);

int     ncds_read_atts(
            int       nc_grpid,
            CDSGroup *cds_group);

CDSVar *ncds_read_var_def(
            int          nc_grpid,
            int          nc_varid,
            CDSGroup    *cds_group,
            const char  *cds_var_name,
            int          nmap_dims,
            const char **nc_dim_names,
            const char **cds_dim_names);

int     ncds_read_var_defs(
            int       nc_grpid,
            CDSGroup *cds_group);

int     ncds_read_group(
            int       nc_grpid,
            int       recursive,
            CDSGroup *cds_group);

void   *ncds_read_var_data(
            int     nc_grpid,
            int     nc_varid,
            size_t *nc_start,
            size_t *nc_count,
            CDSVar *cds_var,
            size_t  cds_sample_start);

void   *ncds_read_var_samples(
            int     nc_grpid,
            int     nc_varid,
            size_t  nc_sample_start,
            size_t *sample_count,
            CDSVar *cds_var,
            size_t  cds_sample_start);

int     ncds_read_static_data(
            int       nc_grpid,
            CDSGroup *cds_group);

int     ncds_read_records(
            int       nc_grpid,
            size_t    nc_record_start,
            size_t    record_count,
            CDSGroup *cds_group,
            size_t    cds_record_start);

int     ncds_read_group_data(
            int       nc_grpid,
            size_t    nc_record_start,
            size_t    nc_record_count,
            int       recursive,
            CDSGroup *cds_group,
            size_t    cds_record_start);
/*@}*/

/******************************************************************************/
/**
 *  @defgroup NCDS_WRITE NetCDF Write
 */
/*@{*/

int     ncds_create_file(
            CDSGroup   *cds_group,
            const char *file,
            int         cmode,
            int         recursive,
            int         header_only);

int     ncds_write_dim(
            CDSDim *cds_dim,
            int     nc_grpid,
            int    *nc_dimid);

int     ncds_write_dims(
            CDSGroup *cds_group,
            int       nc_grpid);

int     ncds_write_att(
            CDSAtt *cds_att,
            int     nc_grpid);

int     ncds_write_atts(
            CDSGroup *cds_group,
            int       nc_grpid);

int     ncds_write_var(
            CDSVar *cds_var,
            int     nc_grpid,
            int    *nc_varid);

int     ncds_write_vars(
            CDSGroup *cds_group,
            int       nc_grpid);

int     ncds_write_group(
            CDSGroup *cds_group,
            int       nc_grpid,
            int       recursive);

int     ncds_write_var_data(
            CDSVar *cds_var,
            size_t  cds_sample_start,
            int     nc_grpid,
            int     nc_varid,
            size_t *nc_start,
            size_t *nc_count);

int     ncds_write_var_samples(
            CDSVar *cds_var,
            size_t  cds_sample_start,
            int     nc_grpid,
            int     nc_varid,
            size_t  nc_sample_start,
            size_t *sample_count);

int     ncds_write_static_data(
            CDSGroup *cds_group,
            int       nc_grpid);

int     ncds_write_records(
            CDSGroup *cds_group,
            size_t    cds_record_start,
            int       nc_grpid,
            size_t    nc_record_start,
            size_t    record_count);

int     ncds_write_group_data(
            CDSGroup *cds_group,
            size_t    cds_record_start,
            int       nc_grpid,
            size_t    nc_record_start,
            size_t    record_count,
            int       recursive);

/*@}*/

/******************************************************************************/
/**
 *  @defgroup NCDS_GET NetCDF Get
 */
/*@{*/

int     ncds_get_coord_var(
            int          nc_grpid,
            size_t       nc_sample_start,
            size_t      *sample_count,
            CDSDim      *cds_dim,
            size_t       cds_sample_start,
            int          nmap_dims,
            const char **nc_dim_names,
            const char **cds_dim_names,
            CDSDataType *cds_dim_types,
            const char **cds_dim_units);

int     ncds_get_coord_vars(
            int          nc_grpid,
            size_t       nc_sample_start,
            size_t      *sample_count,
            CDSVar      *cds_var,
            size_t       cds_sample_start,
            int          nmap_dims,
            const char **nc_dim_names,
            const char **cds_dim_names,
            CDSDataType *cds_dim_types,
            const char **cds_dim_units);

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
            const char **cds_dim_units);

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
            const char **cds_dim_units);

/*@}*/

/******************************************************************************/
/**
 *  @defgroup NCDS_FIND_FILES NetCDF Find Files
 */
/*@{*/

void    ncds_free_file_list(char **file_list);

int     ncds_find_files(
            const char *path,
            const char *prefix,
            const char *extension,
            time_t      start_time,
            time_t      end_time,
            char     ***file_list);
/*@}*/

/******************************************************************************/
/**
 *  @defgroup NCDS_UTILS Utility Functions
 */
/*@{*/

#define NCDS_EQ   CDS_EQ    /**< equal to flag                   */
#define NCDS_LT   CDS_LT    /**< less than flag                  */
#define NCDS_GT   CDS_GT    /**< greater than                    */
#define NCDS_LTEQ CDS_LTEQ  /**< less than or equal to flags     */
#define NCDS_GTEQ CDS_GTEQ  /**< greater than or equal to  flags */

int     ncds_find_time_index(
            size_t   ntimes,
            time_t   base_time,
            double  *offsets,
            double   ref_time,
            int      mode);

int     ncds_format_timestamp(time_t secs1970, char *timestamp);

void    ncds_free_list(int length, char **list);

size_t  ncds_get_att_value(
            int           nc_grpid,
            int           nc_varid,
            const char   *att_name,
            nc_type       out_type,
            void        **value);

size_t  ncds_get_att_text(
            int           nc_grpid,
            int           nc_varid,
            const char   *att_name,
            char        **value);

int     ncds_get_bounds_coord_var(
            int  nc_grpid,
            int  bounds_varid,
            int *coord_varid);

int     ncds_get_bounds_var(
            int  nc_grpid,
            int  coord_varid,
            int *bounds_varid);

int     ncds_get_group_dim_info(
            int      nc_grpid,
            int      include_parents,
            int    **dimids,
            char  ***dim_names,
            size_t **dim_lengths,
            int    **is_unlimdim);

int     ncds_get_missing_values(
            int    nc_grpid,
            int    nc_varid,
            void **values);

int     ncds_get_time_info(
            int     nc_grpid,
            int    *time_dimid,
            int    *time_varid,
            size_t *num_times,
            time_t *base_time,
            double *start_offset,
            double *end_offset);

int     ncds_get_time_offsets(
            int      nc_grpid,
            size_t   start,
            size_t   count,
            time_t  *base_time,
            double **offsets);

int     ncds_get_timevals(
            int         nc_grpid,
            size_t      start,
            size_t      count,
            timeval_t **timevals);

int     ncds_get_var_dim_info(
            int      nc_grpid,
            int      nc_varid,
            int    **dimids,
            char  ***dim_names,
            size_t **dim_lengths,
            int    **is_unlimdim);

time_t  ncds_get_var_time_units(
            int    nc_grpid,
            int    nc_varid,
            char **units);

int     ncds_get_var_units(
            int    nc_grpid,
            int    nc_varid,
            char **units);
/*@}*/

/******************************************************************************/
/**
 *  @defgroup NCWRAP_DATASET Dataset Function Wrappers
 */
/*@{*/

int     ncds_close(int ncid);
int     ncds_create(const char *file, int cmode, int *ncid);
int     ncds_enddef(int ncid);
int     ncds_format(int ncid, int *format);
int     ncds_open(const char *file, int omode, int *ncid);
int     ncds_redef(int ncid);
int     ncds_sync(int ncid);

/*@}*/

/******************************************************************************/
/**
 *  @defgroup NCWRAP_INQUIRE Inquire Function Wrappers
 */
/*@{*/

int     ncds_inq_att(
            int grpid, int varid, const char *attname,
            nc_type *type, size_t *length);

int     ncds_inq_attname(int grpid, int varid, int attid, char *attname);

int     ncds_inq_dim(int grpid, int dimid, char *dimname, size_t *length);
int     ncds_inq_dimid(int grpid, const char *dimname, int *dimid);
int     ncds_inq_dimids(int grpid, int *ndims, int *dimids, int include_parents);
int     ncds_inq_dimlen(int grpid, int dimid, size_t *length);
int     ncds_inq_dimname(int grpid, int dimid, char *dimname);

int     ncds_inq_grpid(int grpid, const char *subgrpname, int *subgrpid);
int     ncds_inq_grpids(int grpid, int *nsubgrps, int *subgrpids);
int     ncds_inq_grpname(int grpid, char *grpname);

int     ncds_inq_natts(int grpid, int *natts);
int     ncds_inq_ndims(int grpid, int *ndims);

int     ncds_inq_var(
            int grpid, int varid,
            char *varname, nc_type *type, int *ndims, int *dimids, int *natts);

int     ncds_inq_vardimids(int grpid, int varid, int *dimids);
int     ncds_inq_varid(int grpid, const char *varname, int *varid);
int     ncds_inq_varids(int grpid, int *nvars, int *varids);
int     ncds_inq_varname(int grpid, int varid, char *varname);
int     ncds_inq_varnatts(int grpid, int varid, int *natts);
int     ncds_inq_varndims(int grpid, int varid, int *ndims);
int     ncds_inq_vartype(int grpid, int varid, nc_type *vartype);

int     ncds_inq_unlimdims(int grpid, int *nunlimdims, int *unlimdimids);

/*@}*/

/******************************************************************************/
/**
 *  @defgroup NCDS_VERSION Library Version
 */
/*@{*/

const char *ncds_lib_version(void);

/*@}*/

/**
 * NetCDF Data Stream.
 */

typedef struct NCDatastream {

    char  *name;        /**< datastream name                       */
    char  *path;        /**< full path to the datastream directory */
    char  *extension;   /**< datastream file extension             */

    int    split_nhours; /**< number of split hours */
    int   *split_hours;  /**< split hours           */

    int    split_ndays;  /**< number of split days  */
    int   *split_days;   /**< split days            */

} NCDatastream;

#define NCDS_FORCE_SPLIT  0x1

int     ncds_add_datastream(
            const char *name,
            const char *path,
            const char *extension);

void    ncds_delete_datastream(
            int ds_index);

int     ncds_set_split_hours(
            int  ds_index,
            int  nhours,
            int *hours);

int     ncds_set_split_days(
            int  ds_index,
            int  ndays,
            int *days);

#endif /* _NCDS3_H_ */
