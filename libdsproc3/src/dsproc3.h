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
*    $Revision: 81662 $
*    $Author: ermold $
*    $Date: 2017-10-27 16:09:46 +0000 (Fri, 27 Oct 2017) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file dsproc3.h
 *  Header file for libdsproc3
 */

#ifndef _DSPROC3_H
#define _DSPROC3_H

#include "armutils.h"
#include "cds3.h"

/******************************************************************************/
/**
 *  @defgroup DSPROC_MAIN Process Main
 */
/*@{*/

/** Flag specifying that the data retrieval process should be run. */
#define DSP_RETRIEVER           0x001

/** Flag specifying if a retriever definition is required. */
#define DSP_RETRIEVER_REQUIRED  0x002

/** Flag specifying that the data transformation process should be run. */
#define DSP_TRANSFORM           0x004

/** Flag specifying that this is an ingest process. */
#define DSP_INGEST              0x100

/**
 *  Process Models
 */
typedef enum {

    /** Generic VAP process.
     *
     *  The retriever definition will be used if it exists in the database
     *  but it is not required for the process to run. This will also run the
     *  transform logic for any variables that are found in the retrieved
     *  data that have been tagged with a coordinate system name.
     */
    PM_GENERIC          = DSP_RETRIEVER | DSP_TRANSFORM,

    /** Retriever only VAP.
     *
     *  This VAP requires a retriever definition to be specified in the
     *  database, but will bypass the transformation logic.
     */
    PM_RETRIEVER_VAP    = DSP_RETRIEVER | DSP_RETRIEVER_REQUIRED,

    /** Transformation VAP.
     *
     *  This VAP requires a retriever definition to be specified in the
     *  database, and will run the transformation logic.
     */
    PM_TRANSFORM_VAP    = DSP_RETRIEVER | DSP_RETRIEVER_REQUIRED | DSP_TRANSFORM,

    /** Ingest Process.
     *
     *  This is an Ingest process that loops over all raw files in the
     *  input datastream directory.
     */
    PM_INGEST          = DSP_INGEST,

    /** Ingest/VAP Hybrid Process that bypasses the transform logic.
     *
     *  This is an Ingest process that uses the PM_RETRIEVER_VAP processing
     *  model, but is designed to run in real-time like an ingest without the
     *  need for the '-b begin_time' command line argument.  The standard
     *  VAP -b/-e command line options can still be used for reprocessing.
     */
    PM_RETRIEVER_INGEST = DSP_INGEST | DSP_RETRIEVER | DSP_RETRIEVER_REQUIRED,

    /** Ingest/VAP Hybrid Process that uses the transform logic.
     *
     *  This is an Ingest process that uses the PM_TRANSFORM_VAP processing
     *  model, but is designed to run in real-time like an ingest without the
     *  need for the '-b begin_time' command line argument.  The standard
     *  VAP -b/-e command line options can still be used for reprocessing.
     */
    PM_TRANSFORM_INGEST = DSP_INGEST | DSP_RETRIEVER | DSP_RETRIEVER_REQUIRED | DSP_TRANSFORM

} ProcModel;

void dsproc_freeopts(void);

int dsproc_getopt(
        const char  *option,
        const char **value);

int dsproc_setopt(
        const char   short_opt,
        const char  *long_opt,
        const char  *arg_name,
        const char  *opt_desc);

void dsproc_use_nc_extension(void);

int dsproc_main(
        int          argc,
        char       **argv,
        ProcModel    proc_model,
        const char  *proc_version,
        int          nproc_names,
        const char **proc_names);

void dsproc_set_init_process_hook(
    void *(*_init_process_hook)(void));

void dsproc_set_finish_process_hook(
    void (*finish_process_hook)(void *user_data));

void dsproc_set_process_data_hook(
    int (*process_data)(
        void     *user_data,
        time_t    begin_date,
        time_t    end_date,
        CDSGroup *input_data));

void dsproc_set_pre_retrieval_hook(
    int (*pre_retrieval_hook)(
        void     *user_data,
        time_t    begin_date,
        time_t    end_date));

void dsproc_set_post_retrieval_hook(
    int (*post_retrieval_hook)(
        void     *user_data,
        time_t    begin_date,
        time_t    end_date,
        CDSGroup *ret_data));

void dsproc_set_pre_transform_hook(
    int (*pre_transform_hook)(
        void     *user_data,
        time_t    begin_date,
        time_t    end_date,
        CDSGroup *ret_data));

void dsproc_set_post_transform_hook(
    int (*post_transform_hook)(
        void     *user_data,
        time_t    begin_date,
        time_t    end_date,
        CDSGroup *trans_data));

void dsproc_set_quicklook_hook(
    int (*quicklook_hook)(
        void     *user_data,
        time_t    begin_date,
        time_t    end_date));

void dsproc_set_process_file_hook(
    int (*process_file_hook)(
        void       *user_data,
        const char *input_dir,
        const char *file_name));

void dsproc_set_custom_qc_hook(
    int (*custom_qc_hook)(
        void       *user_data,
        int         ds_id,
        CDSGroup   *dataset));

// Station view
int  dsproc_station_view_hook(
        void     *user_data,
        time_t    begin_date,
        time_t    end_date,
        CDSGroup *trans_data);

/*@}*/

/******************************************************************************/
/**
 *  @defgroup DSPROC_MESSAGES Process Messages
 */
/*@{*/

/**
 *  Convenience macro for the dsproc_error() function.
 *
 *  Usage: DSPROC_ERROR(const char *status, const char *format, ...)
 *
 *  See printf for a complete description of the format string.
 */
#define DSPROC_ERROR(status, ...) \
dsproc_error(__func__, __FILE__, __LINE__, status, __VA_ARGS__)

/**
 *  Convenience macro for the dsproc_warning() function.
 *
 *  Usage: DSPROC_WARNING(const char *format, ...)
 *
 *  See printf for a complete description of the format string.
 */
#define DSPROC_WARNING(...) \
dsproc_warning(__func__, __FILE__, __LINE__, __VA_ARGS__)

/**
 *  Convenience macro for the dsproc_log() function.
 *
 *  Usage: DSPROC_LOG(const char *format, ...)
 *
 *  See printf for a complete description of the format string.
 */
#define DSPROC_LOG(...) \
dsproc_log(__func__, __FILE__, __LINE__, __VA_ARGS__)

/**
 *  Convenience macro for the dsproc_bad_file_warning() function.
 *
 *  Usage: DSPROC_BAD_FILE_WARNING(const char *file_name, const char *format, ...)
 *
 *  See printf for a complete description of the format string.
 */
#define DSPROC_BAD_FILE_WARNING(file_name, ...) \
    dsproc_bad_file_warning(__func__, __FILE__, __LINE__, file_name, __VA_ARGS__)

/**
 *  Convenience macro for the dsproc_bad_line_warning() function.
 *
 *  Usage: DSPROC_BAD_LINE_WARNING(const char *file_name, int line_num, const char *format, ...)
 *
 *  See printf for a complete description of the format string.
 */
#define DSPROC_BAD_LINE_WARNING(file_name, line_num, ...) \
    dsproc_bad_line_warning(__func__, __FILE__, __LINE__, file_name, line_num, __VA_ARGS__)

/**
 *  Convenience macro for the dsproc_bad_record_warning() function.
 *
 *  Usage: DSPROC_BAD_RECORD_WARNING(const char *file_name, int rec_num, const char *format, ...)
 *
 *  See printf for a complete description of the format string.
 */
#define DSPROC_BAD_RECORD_WARNING(file_name, rec_num, ...) \
    dsproc_bad_record_warning(__func__, __FILE__, __LINE__, file_name, rec_num, __VA_ARGS__)

/**
 *  Convenience macro for the dsproc_mentor_mail() function.
 *
 *  Usage: DSPROC_MENTOR_MAIL(const char *format, ...)
 *
 *  See printf for a complete description of the format string.
 */
#define DSPROC_MENTOR_MAIL(...) \
dsproc_mentor_mail(__func__, __FILE__, __LINE__, __VA_ARGS__)

/**
 *  Convenience macro for the dsproc_debug() function: level == 1.
 *
 *  Usage: DSPROC_DEBUG_LV1(const char *format, ...)
 *
 *  See printf for a complete description of the format string.
 */
#define DSPROC_DEBUG_LV1(...) \
if (msngr_debug_level || msngr_provenance_level) \
    dsproc_debug(__func__, __FILE__, __LINE__, 1, __VA_ARGS__)

/**
 *  Convenience macro for the dsproc_debug() function: level == 2.
 *
 *  Usage: DSPROC_DEBUG_LV2(const char *format, ...)
 *
 *  See printf for a complete description of the format string.
 */
#define DSPROC_DEBUG_LV2(...) \
if (msngr_debug_level || msngr_provenance_level) \
    dsproc_debug(__func__, __FILE__, __LINE__, 2, __VA_ARGS__)

/**
 *  Convenience macro for the dsproc_debug() function: level == 3.
 *
 *  Usage: DSPROC_DEBUG_LV3(const char *format, ...)
 *
 *  See printf for a complete description of the format string.
 */
#define DSPROC_DEBUG_LV3(...) \
if (msngr_debug_level || msngr_provenance_level) \
    dsproc_debug(__func__, __FILE__, __LINE__, 3, __VA_ARGS__)

/**
 *  Convenience macro for the dsproc_debug() function: level == 4.
 *
 *  Usage: DSPROC_DEBUG_LV4(const char *format, ...)
 *
 *  See printf for a complete description of the format string.
 */
#define DSPROC_DEBUG_LV4(...) \
if (msngr_debug_level || msngr_provenance_level) \
    dsproc_debug(__func__, __FILE__, __LINE__, 4, __VA_ARGS__)

/**
 *  Convenience macro for the dsproc_debug() function: level == 5.
 *
 *  Usage: DSPROC_DEBUG_LV5(const char *format, ...)
 *
 *  See printf for a complete description of the format string.
 */
#define DSPROC_DEBUG_LV5(...) \
if (msngr_debug_level || msngr_provenance_level) \
    dsproc_debug(__func__, __FILE__, __LINE__, 5, __VA_ARGS__)

int dsproc_get_debug_level(void);

/*@}*/

/******************************************************************************/
/**
 *  @defgroup DSPROC_INFO Process Information
 */
/*@{*/

const char *dsproc_get_site(void);
const char *dsproc_get_facility(void);
const char *dsproc_get_name(void);

/*@}*/

/******************************************************************************/
/**
 *  @defgroup DSPROC_CONTROL Process Control
 */
/*@{*/

/**
 *  Convenience macro for the dsproc_abort() function.
 *
 *  Usage: DSPROC_ABORT(const char *status, const char *format, ...)
 *
 *  See printf for a complete description of the format string.
 */
#define DSPROC_ABORT(status, ...) \
dsproc_abort(__func__, __FILE__, __LINE__, status, __VA_ARGS__)

/**
 *  Log File Intervals.
 */
typedef enum {

    LOG_MONTHLY = 0, /**< create monthly log files    */
    LOG_DAILY   = 1, /**< create daily log files      */
    LOG_RUN     = 2, /**< create one log file per run */

} LogInterval;

/**
 *  Output File Splitting Mode.
 */
typedef enum {

    SPLIT_ON_STORE  = 0, /**< Always create a new file when data is stored.   */

    SPLIT_ON_HOURS  = 1, /**< Split start is the hour of the day for the first
                              split [0-23], and split interval is in hours.   */

    SPLIT_ON_DAYS   = 2, /**< Split start is the day of the month for the first
                              split [1-31], and split interval is in days.    */

    SPLIT_ON_MONTHS = 3, /**< Split start is the month of the year for the first
                              split [1-12], and split interval is in months.  */

    SPLIT_NONE      = 4  /**< Always append output to the previous file
                              unless otherwise specified in the call to
                              dsproc_store_dataset. */

} SplitMode;

void    dsproc_set_datastream_split_mode(
            int       ds_id,
            SplitMode split_mode,
            double    split_start,
            double    split_interval);

void    dsproc_set_log_interval(LogInterval interval, int use_begin_time);
void    dsproc_set_processing_interval_offset(time_t offset);

void    dsproc_set_trans_qc_rollup_flag(int flag);
int     dsproc_set_trans_qc_rollup_bit_descriptions(
            const char *bad_desc,
            const char *ind_desc);

/*@}*/

/******************************************************************************/
/**
 *  @defgroup DSPROC_DATASTREAM DataStreams
 */
/*@{*/

/**
 *  DataStream Roles
 */
typedef enum {

    DSR_INPUT  =  1, /**< input datastream  */
    DSR_OUTPUT =  2  /**< output datastream */

} DSRole;

int     dsproc_get_datastream_id(
            const char *site,
            const char *facility,
            const char *dsc_name,
            const char *dsc_level,
            DSRole      role);

int     dsproc_get_input_datastream_id(
            const char *dsc_name,
            const char *dsc_level);

int     dsproc_get_input_datastream_ids(int **ids);

int     dsproc_get_output_datastream_id(
            const char *dsc_name,
            const char *dsc_level);

int     dsproc_get_output_datastream_ids(int **ids);

const char *dsproc_datastream_name(int ds_id);
const char *dsproc_datastream_site(int ds_id);
const char *dsproc_datastream_facility(int ds_id);
const char *dsproc_datastream_class_name(int ds_id);
const char *dsproc_datastream_class_level(int ds_id);
const char *dsproc_datastream_path(int ds_id);

/*@}*/

/******************************************************************************/
/**
 *  @defgroup DSPROC_DATASTREAM_FILES DataStream Files
 */
/*@{*/

int     dsproc_add_datastream_file_patterns(
            int          ds_id,
            int          npatterns,
            const char **patterns,
            int          ignore_case);

int     dsproc_find_datastream_files(
            int       ds_id,
            time_t    begin_time,
            time_t    end_time,
            char   ***file_list);

void    dsproc_free_file_list(char **file_list);

int     dsproc_get_datastream_files(int ds_id, char ***file_list);

int     dsproc_set_datastream_path(int ds_id, const char *path);

void    dsproc_set_file_name_compare_function(
            int     ds_id,
            int    (*function)(const void *, const void *));

void    dsproc_set_file_name_time_function(
            int      ds_id,
            time_t (*function)(const char *));

void    dsproc_set_datastream_file_extension(
            int         ds_id,
            const char *extension);

/*@}*/

/******************************************************************************/
/**
 *  @defgroup DSPROC_DATASET Datasets
 */
/*@{*/

const char *dsproc_dataset_name(CDSGroup *dataset);

const char *dsproc_get_dataset_version(
                CDSGroup *dataset,
                int      *major,
                int      *minor,
                int      *micro);

CDSGroup   *dsproc_get_output_dataset(
                int ds_id,
                int obs_index);

CDSGroup   *dsproc_get_retrieved_dataset(
                int ds_id,
                int obs_index);

CDSGroup   *dsproc_get_transformed_dataset(
                const char *coordsys_name,
                int         ds_id,
                int         obs_index);

/*@}*/

/******************************************************************************/
/**
 *  @defgroup DSPROC_DATASET_DIMS Dataset Dimensions
 */
/*@{*/

CDSDim *dsproc_get_dim(
            CDSGroup   *dataset,
            const char *name);

size_t  dsproc_get_dim_length(
            CDSGroup   *dataset,
            const char *name);

int     dsproc_set_dim_length(
            CDSGroup    *dataset,
            const char  *name,
            size_t       length);

/*@}*/

/******************************************************************************/
/**
 *  @defgroup DSPROC_DATASET_ATTS Dataset Attributes
 */
/*@{*/

int     dsproc_change_att(
            void        *parent,
            int          overwrite,
            const char  *name,
            CDSDataType  type,
            size_t       length,
            void        *value);

CDSAtt *dsproc_get_att(
            void        *parent,
            const char  *name);

char   *dsproc_get_att_text(
            void       *parent,
            const char *name,
            size_t     *length,
            char       *value);

void   *dsproc_get_att_value(
            void        *parent,
            const char  *name,
            CDSDataType  type,
            size_t      *length,
            void        *value);

int     dsproc_set_att(
            void        *parent,
            int          overwrite,
            const char  *name,
            CDSDataType  type,
            size_t       length,
            void        *value);

int     dsproc_set_att_text(
            void        *parent,
            const char  *name,
            const char *format, ...);

int     dsproc_set_att_value(
            void        *parent,
            const char  *name,
            CDSDataType  type,
            size_t       length,
            void        *value);

int     dsproc_set_att_value_if_null(
            void        *parent,
            const char  *name,
            CDSDataType  type,
            size_t       length,
            void        *value);

int     dsproc_set_att_text_if_null(
            void        *parent,
            const char  *name,
            const char *format, ...);

int     dsproc_set_var_chunksizes(
            CDSVar *var,
            int    *time_chunksize);

void    dsproc_set_max_chunksize(
            size_t max_chunksize);

int     dsproc_set_chunksizes(
            CDSGroup *dataset,
            int       time_chunksize);

/*@}*/

/******************************************************************************/
/**
 *  @defgroup DSPROC_DATASET_VARS Dataset Variables
 */
/*@{*/

CDSVar *dsproc_clone_var(
            CDSVar       *src_var,
            CDSGroup     *dataset,
            const char   *var_name,
            CDSDataType   data_type,
            const char  **dim_names,
            int           copy_data);

CDSVar *dsproc_define_var(
            CDSGroup    *dataset,
            const char  *name,
            CDSDataType  type,
            int          ndims,
            const char **dim_names,
            const char  *long_name,
            const char  *standard_name,
            const char  *units,
            void        *valid_min,
            void        *valid_max,
            void        *missing_value,
            void        *fill_value);

int     dsproc_delete_var(
            CDSVar *var);

CDSVar *dsproc_get_bounds_var(CDSVar *coord_var);

CDSVar *dsproc_get_coord_var(
            CDSVar *var,
            int     dim_index);

int     dsproc_get_dataset_vars(
            CDSGroup     *dataset,
            const char  **var_names,
            int           required,
            CDSVar     ***vars,
            CDSVar     ***qc_vars,
            CDSVar     ***aqc_vars);

CDSVar *dsproc_get_metric_var(
            CDSVar     *var,
            const char *metric);

CDSVar *dsproc_get_output_var(
            int         ds_id,
            const char *var_name,
            int         obs_index);

CDSVar *dsproc_get_qc_var(
            CDSVar *var);

CDSVar *dsproc_get_retrieved_var(
            const char *var_name,
            int         obs_index);

CDSVar *dsproc_get_transformed_var(
            const char *var_name,
            int         obs_index);

CDSVar *dsproc_get_trans_coordsys_var(
            const char *coordsys_name,
            const char *var_name,
            int         obs_index);

CDSVar *dsproc_get_var(
            CDSGroup   *dataset,
            const char *name);

const char *dsproc_var_name(CDSVar *var);
size_t      dsproc_var_sample_count(CDSVar *var);
size_t      dsproc_var_sample_size(CDSVar *var);

/*

Need to add:

int         dsproc_var_is_unlimited(CDSVar *var);
CDSGroup   *dsproc_var_parent(CDSVar *var);
int         dsproc_var_shape(CDSVar *var, size_t **lengths);

*/

/*@}*/

/******************************************************************************/
/**
 *  @defgroup DSPROC_VAR_DATA Dataset Variable Data
 */
/*@{*/

void   *dsproc_alloc_var_data(
            CDSVar *var,
            size_t  sample_start,
            size_t  sample_count);

void   *dsproc_alloc_var_data_index(
            CDSVar *var,
            size_t  sample_start,
            size_t  sample_count);

void   *dsproc_get_var_data_index(
            CDSVar *var);

void   *dsproc_get_var_data(
            CDSVar       *var,
            CDSDataType   type,
            size_t        sample_start,
            size_t       *sample_count,
            void         *missing_value,
            void         *data);

int     dsproc_get_var_missing_values(
            CDSVar  *var,
            void   **values);

void   *dsproc_init_var_data(
            CDSVar *var,
            size_t  sample_start,
            size_t  sample_count,
            int     use_missing);

void   *dsproc_init_var_data_index(
            CDSVar *var,
            size_t  sample_start,
            size_t  sample_count,
            int     use_missing);

int     dsproc_set_bounds_data(
            CDSGroup *dataset,
            size_t    sample_start,
            size_t    sample_count);

int     dsproc_set_bounds_var_data(
            CDSVar *coord_var,
            size_t  sample_start,
            size_t  sample_count);

void   *dsproc_set_var_data(
            CDSVar       *var,
            CDSDataType   type,
            size_t        sample_start,
            size_t        sample_count,
            void         *missing_value,
            void         *data);

/*@}*/

/******************************************************************************/
/**
 *  @defgroup DSPROC_TIME_DATA Dataset Time Data
 */
/*@{*/

time_t  dsproc_get_base_time(void *cds_object);

size_t  dsproc_get_time_range(
            void      *cds_object,
            timeval_t *start_time,
            timeval_t *end_time);

CDSVar *dsproc_get_time_var(void *cds_object);

time_t *dsproc_get_sample_times(
            void   *cds_object,
            size_t  sample_start,
            size_t *sample_count,
            time_t *sample_times);

timeval_t *dsproc_get_sample_timevals(
            void      *cds_object,
            size_t     sample_start,
            size_t    *sample_count,
            timeval_t *sample_times);

int     dsproc_set_base_time(
            void       *cds_object,
            const char *long_name,
            time_t      base_time);

int     dsproc_set_sample_times(
            void     *cds_object,
            size_t    sample_start,
            size_t    sample_count,
            time_t   *sample_times);

int     dsproc_set_sample_timevals(
            void      *cds_object,
            size_t     sample_start,
            size_t     sample_count,
            timeval_t *sample_times);

/*@}*/

/******************************************************************************/
/**
 *  @defgroup DSPROC_QC_UTILS QC Utilities
 */
/*@{*/

int     dsproc_consolidate_var_qc(
            CDSVar       *in_qc_var,
            unsigned int  bad_mask,
            CDSVar       *out_qc_var,
            unsigned int  bad_flag,
            unsigned int  ind_flag,
            int           reset);

unsigned int dsproc_get_bad_qc_mask(CDSVar *qc_var);

/*@}*/

/******************************************************************************/
/**
 *  @defgroup DSPROC_MISC_UTILITIES Miscellaneous Utilities
 */
/*@{*/

int dsproc_create_timestamp(
        time_t  secs1970,
        char   *timestamp);

int dsproc_execvp(
        const char *file,
        char *const argv[],
        int         flags);

int dsproc_run_dq_inspector(
        int         dsid,
        time_t      begin_time,
        time_t      end_time,
        const char *args[],
        int         flags);

int dsproc_solar_position(
        time_t secs1970, 
        double latitude, 
        double longitude,
        double *ap_ra,
        double *ap_dec,
        double *altitude,
        double *refraction,
        double *azimuth,
        double *distance);

/*@}*/

/******************************************************************************/
/**
 *  @defgroup DSPROC_OUTLIER_FILTERS Outlier Filters
 */
/*@{*/

int dsproc_flag_outliers_iqd(
        CDSGroup     *dataset,
        const char   *var_name,
        double        window_width,
        int           min_npoints,
        unsigned int  skipped_flag,
        double        bad_threshold,
        unsigned int  bad_flag,
        double        ind_threshold,
        unsigned int  ind_flag,
        unsigned int  analyze);

int dsproc_flag_outliers_mean_dev(
        CDSGroup     *dataset,
        const char   *var_name,
        double        window_width,
        int           min_npoints,
        unsigned int  skipped_flag,
        double        bad_threshold,
        unsigned int  bad_flag,
        double        ind_threshold,
        unsigned int  ind_flag,
        unsigned int  analyze);

int dsproc_flag_outliers_mean_mad(
        CDSGroup     *dataset,
        const char   *var_name,
        double        window_width,
        int           min_npoints,
        unsigned int  skipped_flag,
        double        bad_threshold,
        unsigned int  bad_flag,
        double        ind_threshold,
        unsigned int  ind_flag,
        unsigned int  analyze);

int dsproc_flag_outliers_median_mad(
        CDSGroup     *dataset,
        const char   *var_name,
        double        window_width,
        int           min_npoints,
        unsigned int  skipped_flag,
        double        bad_threshold,
        unsigned int  bad_flag,
        double        ind_threshold,
        unsigned int  ind_flag,
        unsigned int  analyze);

int dsproc_flag_outliers_std(
        CDSGroup     *dataset,
        const char   *var_name,
        double        window_width,
        int           min_npoints,
        unsigned int  skipped_flag,
        double        bad_threshold,
        unsigned int  bad_flag,
        double        ind_threshold,
        unsigned int  ind_flag,
        unsigned int  analyze);

/*@}*/

/******************************************************************************/
/**
 *  @defgroup DSPROC_VARTAGS Variable Tags
 */
/*@{*/

/** Flag instructing the transform logic to ignore this variable. */
#define VAR_SKIP_TRANSFORM   0x1

/** Consolidate the transformation QC bits when mapped to the output dataset. */
#define VAR_ROLLUP_TRANS_QC  0x2

/**
 *  Output Variable Target.
 */
typedef struct {

    int         ds_id;      /**< output datastream ID */
    const char *var_name;   /**< output variable name */

} VarTarget;

int     dsproc_add_var_output_target(
            CDSVar     *var,
            int         ds_id,
            const char *var_name);

int     dsproc_copy_var_tag(
            CDSVar *src_var,
            CDSVar *dest_var);

void    dsproc_delete_var_tag(
            CDSVar *var);

const char *dsproc_get_source_var_name(CDSVar *var);
const char *dsproc_get_source_ds_name(CDSVar *var);
int         dsproc_get_source_ds_id(CDSVar *var);

const char *dsproc_get_var_coordsys_name(CDSVar *var);

int     dsproc_get_var_output_targets(
            CDSVar      *var,
            VarTarget ***targets);

int     dsproc_set_var_coordsys_name(
            CDSVar     *var,
            const char *coordsys_name);

int     dsproc_set_var_flags(CDSVar *var, int flags);

int     dsproc_set_var_output_target(
            CDSVar     *var,
            int         ds_id,
            const char *var_name);

void    dsproc_unset_var_flags(CDSVar *var, int flags);

/*@}*/

/******************************************************************************/
/**
 *  @defgroup DSPROC_DQRS Variable DQRs
 */
/*@{*/

/**
 *  Variable DQR.
 *
 *  At the time of this writing the 'code => color => code_desc' values were:
 *
 *    - -1 => None        => Presumed not to exist
 *    -  0 => Black       => Missing
 *    -  1 => White       => Not inspected
 *    -  2 => Green       => Good
 *    -  3 => Yellow      => Suspect
 *    -  4 => Red         => Incorrect
 *    -  5 => Transparent => Does not affect quality
 */
typedef struct {
    const char  *id;          /**< DQR ID                           */
    const char  *desc;        /**< description                      */
    const char  *ds_name;     /**< datastream name                  */
    const char  *var_name;    /**< variable name                    */
    int          code;        /**< code number                      */
    const char  *color;       /**< code color                       */
    const char  *code_desc;   /**< code description                 */
    time_t       start_time;  /**< start time in seconds since 1970 */
    time_t       end_time;    /**< end time in seconds since 1970   */
    size_t       start_index; /**< start time index in dataset      */
    size_t       end_index;   /**< end time index in dataset        */
} VarDQR;

int dsproc_get_var_dqrs(CDSVar *var, VarDQR ***dqrs);

/*@}*/

/******************************************************************************/
/**
 *  @defgroup DSPROC_Print Print Functions
 */
/*@{*/

int dsproc_dump_dataset(
        CDSGroup   *dataset,
        const char *outdir,
        const char *prefix,
        time_t      file_time,
        const char *suffix,
        int         flags);

int dsproc_dump_output_datasets(
        const char *outdir,
        const char *suffix,
        int         flags);

int dsproc_dump_retrieved_datasets(
        const char *outdir,
        const char *suffix,
        int         flags);

int dsproc_dump_transformed_datasets(
        const char *outdir,
        const char *suffix,
        int         flags);

/*
Need functions:

void      dsproc_print_dataset_object(
            const char *file_name,
            void       *object);

*/

/*@}*/

/******************************************************************************/
/**
 *  @defgroup DSPROC_STATUS Process Status Definitions
 */
/*@{*/

/** Successful */
#define DSPROC_SUCCESS       "Successful"

/** Memory Allocation Error */
#define DSPROC_ENOMEM        "Memory Allocation Error"

/** Could Not Create Fork For New Process */
#define DSPROC_EFORK         "Could Not Create Fork For New Process"

/** No Input Data Found */
#define DSPROC_ENODATA       "No Input Data Found"

/** No Output Data Found */
#define DSPROC_ENOOUTDATA    "No Output Data Created"

/** Could Not Initialize Signal Handlers */
#define DSPROC_EINITSIGS     "Could Not Initialize Signal Handlers"

/** Maximum Run Time Limit Exceeded */
#define DSPROC_ERUNTIME      "Maximum Run Time Limit Exceeded"

/** Could Not Force Process To Continue */
#define DSPROC_EFORCE        "Could Not Force Process To Continue"

/** Could Not Determine Path To Datastream */
#define DSPROC_EDSPATH       "Could Not Determine Path To Datastream"

/** Could Not Determine Path To Logs Directory */
#define DSPROC_ELOGSPATH     "Could Not Determine Path To Logs Directory"

/** Could Not Open Log File */
#define DSPROC_EACCESS       "Could Not Access File or Directory"

/** Could Not Open Log File */
#define DSPROC_ELOGOPEN      "Could Not Open Log File"

/** Could Not Open Provenance Log */
#define DSPROC_EPROVOPEN     "Could Not Open Provenance Log"

/** Could Not Initialize Mail */
#define DSPROC_EMAILINIT     "Could Not Initialize Mail"

/** Database Error (see log file) */
#define DSPROC_EDBERROR      "Database Error (see log file)"

/** Database Connection Error */
#define DSPROC_EDBCONNECT    "Database Connection Error"

/** DQR Database Error (see log file) */
#define DSPROC_EDQRDBERROR   "DQR Database Error (see log file)"

/** DQR Database Connection Error */
#define DSPROC_EDQRDBCONNECT "DQR Database Connection Error"

/** Directory List Error */
#define DSPROC_EDIRLIST      "Could Not Get Directory Listing"

/** Directory List Error */
#define DSPROC_EREGEX        "Regular Expression Error"

/** Invalid Input Datastream Class */
#define DSPROC_EBADINDSC     "Invalid Input Datastream Class"

/** Could Not Find Input Datastream Class In Database */
#define DSPROC_ENOINDSC      "Could Not Find Input Datastream Class In Database"

/** Too Many Input Datastream Classes In Database */
#define DSPROC_ETOOMANYINDSC "Too Many Input Datastreams Defined In Database"

/** Invalid Output Datastream Class */
#define DSPROC_EBADOUTDSC    "Invalid Output Datastream Class"

/** Invalid Datastream ID */
#define DSPROC_EBADDSID      "Invalid Datastream ID"

/** Invalid Output Datastream Format */
#define DSPROC_EBADOUTFORMAT "Invalid Output Datastream Format"

/** Found Data Time Before Minimum Valid Time */
#define DSPROC_EMINTIME      "Found Data Time Before Minimum Valid Time"

/** Found Data Time In The Future */
#define DSPROC_EFUTURETIME   "Found Data Time In The Future"

/** Invalid Time Order */
#define DSPROC_ETIMEORDER    "Invalid Time Order"

/** Found Overlapping Data Times */
#define DSPROC_ETIMEOVERLAP  "Found Overlapping Data Times"

/** Invalid Base Time */
#define DSPROC_EBASETIME     "Could Not Get Base Time For Time Variable"

/** Invalid Global Attribute Value */
#define DSPROC_EGLOBALATT     "Invalid Global Attribute Value"

/** Invalid Data Type For Time Variable */
#define DSPROC_ETIMEVARTYPE  "Invalid Data Type For Time Variable"

/** Invalid Data Type For QC Variable */
#define DSPROC_EQCVARTYPE    "Invalid Data Type For QC Variable"

/** Invalid QC Variable Sample Size */
#define DSPROC_EQCSAMPLESIZE "Invalid QC Variable Sample Size"

/** Invalid QC Variable Dimensions */
#define DSPROC_EQCVARDIMS    "Invalid QC Variable Dimensions"

/** Missing QC Bit Description */
#define DSPROC_ENOBITDESC    "Missing QC Bit Description"

/** Invalid Data Type For Variable */
#define DSPROC_EVARTYPE      "Invalid Data Type For Variable"

/** Invalid Variable Sample Size */
#define DSPROC_ESAMPLESIZE   "Invalid Variable Sample Size"

/** Data Attribute Has Invalid Data Type */
#define DSPROC_EDATAATTTYPE  "Data Attribute Has Invalid Data Type"

/** Could Not Copy File */
#define DSPROC_EFILECOPY     "Could Not Copy File"

/** Could Not Move File */
#define DSPROC_EFILEMOVE     "Could Not Move File"

/** Could Not Open File */
#define DSPROC_EFILEOPEN     "Could Not Open File"

/** Could Not Read From File */
#define DSPROC_EFILEREAD     "Could Not Read From File"

/** Could Not Write To File */
#define DSPROC_EFILEWRITE    "Could Not Write To File"

/** Could Not Get File Stats */
#define DSPROC_EFILESTATS    "Could Not Get File Stats"

/** Could Not Delete File */
#define DSPROC_EUNLINK       "Could Not Delete File"

/** Source File Does Not Exist */
#define DSPROC_ENOSRCFILE    "Source File Does Not Exist"

/** Could Not Determine File Time */
#define DSPROC_ENOFILETIME   "Could Not Determine File Time"

/** Could Not Create Destination Directory */
#define DSPROC_EDESTDIRMAKE  "Could Not Create Destination Directory"

/** Time Calculation Error */
#define DSPROC_ETIMECALC     "Time Calculation Error"

/** Could Not Get File MD5 */
#define DSPROC_EFILEMD5      "Could Not Get File MD5"

/** Source And Destination File MD5s Do Not Match */
#define DSPROC_EMD5CHECK     "Source And Destination File MD5s Do Not Match"

/** Could Not Allocate Memory For Dataset Variable */
#define DSPROC_ECDSALLOCVAR  "Could Not Allocate Memory For Dataset Variable"

/** Could Not Copy Dataset Variable */
#define DSPROC_ECDSCOPYVAR   "Could Not Copy Dataset Variable"

/** Could Not Copy Dataset Variable */
#define DSPROC_ECLONEVAR     "Could Not Clone Dataset Variable"

/** Could Not Define Dataset Variable */
#define DSPROC_ECDSDEFVAR    "Could Not Define Dataset Variable"

/** Could Not Delete Dataset Variable */
#define DSPROC_ECDSDELVAR    "Could Not Delete Dataset Variable"

/** Could Not Copy Dataset Metadata */
#define DSPROC_ECDSCOPY      "Could Not Copy Dataset Metadata"

/** Could Not Change Attribute Value In Dataset */
#define DSPROC_ECDSCHANGEATT "Could Not Change Attribute Value In Dataset"

/** Could Not Set Attribute Value In Dataset */
#define DSPROC_ECDSSETATT    "Could Not Set Attribute Value In Dataset"

/** Could Not Set Dimension Length In Dataset */
#define DSPROC_ECDSSETDIM    "Could Not Set Dimension Length In Dataset"

/** Could Not Set Variable Data In Dataset */
#define DSPROC_ECDSSETDATA   "Could Not Set Variable Data In Dataset"

/** Could Not Set Time Values In Dataset */
#define DSPROC_ECDSSETTIME   "Could Not Set Time Values In Dataset"

/** Could Not Get Time Values From Dataset */
#define DSPROC_ECDSGETTIME   "Could Not Get Time Values From Dataset"

/** Could Not Merge Datasets */
#define DSPROC_EMERGE        "Could Not Merge Datasets"

/** Invalid Cell Boundary Variable or Definition */
#define DSPROC_EBOUNDSVAR    "Invalid Cell Boundary Variable or Definition"

/** DOD Not Defined In Database */
#define DSPROC_ENODOD        "DOD Not Defined In Database"

/** Could Not Find Data Retriever Information */
#define DSPROC_ENORETRIEVER  "Could Not Find Retriever Definition In Database"

/** Invalid Retriever Definition */
#define DSPROC_EBADRETRIEVER "Invalid Retriever Definition"

/** Required Variable Missing From Dataset */
#define DSPROC_EREQVAR       "Required Variable Missing From Dataset"

/** Required Attribute Variable Missing From Variable or Dataset */
#define DSPROC_EREQATT       "Required Attribute Variable Missing From Variable or Dataset"

/** Could Not Retrieve Input Data */
#define DSPROC_ERETRIEVER    "Could Not Retrieve Input Data"

/** Could Not Create NetCDF File */
#define DSPROC_ENCCREATE     "Could Not Create NetCDF File"

/** Could Not Open NetCDF File */
#define DSPROC_ENCOPEN       "Could Not Open NetCDF File"

/** Could Not Close NetCDF File */
#define DSPROC_ENCCLOSE      "Could Not Close NetCDF File"

/** Could Not Close NetCDF File */
#define DSPROC_ENCSYNC       "Could Not Sync NetCDF File"

/** NetCDF File Read Error */
#define DSPROC_ENCREAD       "Could Not Read From NetCDF File"

/** NetCDF File Write Error */
#define DSPROC_ENCWRITE      "Could Not Write To NetCDF File"

/** Could Not Find Data Transform Information */
#define DSPROC_ENOTRANSFORM  "Could Not Find Data Transform Information"

/** Could Not Find Data Transform Information */
#define DSPROC_ETRANSFORM    "Could Not Transform Input Data"

/** Could Not Create Consolidated Transformation QC Variable */
#define DSPROC_ETRANSQCVAR   "Could Not Create Consolidated Transformation QC Variable"

/** Could Not Load Transform Parameters File */
#define DSPROC_ETRANSPARAMLOAD "Could Not Load Transform Parameters File"

/** Could Not Map Input Variable To Output Variable */
#define DSPROC_EVARMAP       "Could Not Map Input Variable To Output Variable"

/** Could Not Parse Input CSV File */
#define DSPROC_ECSVPARSER    "Could Not Parse CSV File"

/** Could Not Read CSV Ingest Configuration File */
#define DSPROC_ECSVCONF      "Could Not Read CSV Ingest Configuration File"

/** Could Not Map Input CSV Data To Output Dataset */
#define DSPROC_ECSV2CDS      "Could Not Map Input CSV Data To Output Dataset"

/*@}*/

/******************************************************************************/
/**
 *  @defgroup DSPROC_FILE_UTILS File Utilities
 */
/*@{*/

int     dsproc_copy_file(const char *src_file, const char *dest_file);
int     dsproc_move_file(const char *src_file, const char *dest_file);
FILE   *dsproc_open_file(const char *file);

/*@}*/

/******************************************************************************/
/**
 *  @defgroup DSPROC_INGEST_RENAME_RAW Ingest: Rename Raw
 */
/*@{*/

int     dsproc_rename(
            int         ds_id,
            const char *file_path,
            const char *file_name,
            time_t      begin_time,
            time_t      end_time);

int     dsproc_rename_tv(
            int              ds_id,
            const char      *file_path,
            const char      *file_name,
            const timeval_t *begin_time,
            const timeval_t *end_time);

int     dsproc_rename_bad(
            int         ds_id,
            const char *file_path,
            const char *file_name,
            time_t      file_time);

int     dsproc_set_preserve_dots_from_name(int ds_id, const char *file_name);

void    dsproc_set_rename_preserve_dots(int ds_id, int preserve_dots);

/*@}*/

/******************************************************************************/
/**
 *  @defgroup DSPROC_CSV_UTILITIES Ingest: CSV Utility Functions
 */
/*@{*/

int         dsproc_count_csv_delims(const char *strp, char delim);
char       *dsproc_find_csv_delim(const char *strp, char delim);
char       *dsproc_skip_csv_whitespace(const char *strp, char delim);
int         dsproc_split_csv_string(char *strp, char delim, int length, char **list);

/*@}*/

/******************************************************************************/
/**
 *  @defgroup DSPROC_CSV_FILE_PARSING Ingest: CSV Parser
 */
/*@{*/

/**
 *  CSV Parsing Structure.
 */
typedef struct
{
    char        *file_path;       /**< path to the directory the file is in    */
    char        *file_name;       /**< name of the file                        */
    struct stat  file_stats;      /**< file stats                              */
    char        *file_data;       /**< in memory copy of the parsed data file  */
    int          nlines;          /**< number of lines in the file             */
    char       **lines;           /**< array of line pointers                  */
    int          linenum;         /**< current line number                     */
    char        *linep;           /**< pointer to the current line in memory   */

    char       **headers;         /**< pointers to the header fields           */
    char      ***values;          /**< pointers to the field values            */
    int          nfields;         /**< number of fields per record             */
    int          nrecs;           /**< number of records                       */

    char        *header_data;     /**< parsed copy of header line              */
    int         *free_header;     /**< only used when adding headers manually  */
    char       **rec_buff;        /**< buffer used to parse record lines       */

    int          nbytes_alloced;  /**< allocated length of the file_data array */
    int          nlines_alloced;  /**< allocated length of the lines array     */
    int          nfields_alloced; /**< number of fields allocated              */
    int          nrecs_alloced;   /**< number of records allocated             */

    char         delim;           /**< CSV column delimiter                    */
    int          nlines_guess;    /**< estimated number of lines in a file     */
    int          nfields_guess;   /**< only used when adding headers manually  */

    RETimeList  *ft_patterns;     /**< compiled list of file time patterns     */
    RETimeRes   *ft_result;       /**< file time used internally               */

    int          ntc;             /**< number of time columns                  */
    char       **tc_names;        /**< list of time column names               */
    RETimeList **tc_patterns;     /**< compiled list of time string patterns   */
    int         *tc_index;        /**< indexes of time columns                 */

    timeval_t   *tvs;             /**< array of record times                   */

    time_t       time_offset;     /**< offset to apply to record times         */
    struct tm    base_tm;         /**< base time to use for record times       */

    int          tro_threshold;   /**< threshold used to detect time rollovers */
    time_t       tro_offset;      /**< offset used to track time rollovers     */

} CSVParser;

void        dsproc_free_csv_parser(CSVParser *csv);

char      **dsproc_get_csv_column_headers(CSVParser *csv, int *nfields);
char      **dsproc_get_csv_field_strvals(CSVParser *csv, const char *name);
timeval_t  *dsproc_get_csv_timevals(CSVParser *csv, int *nrecs);

time_t      dsproc_get_csv_file_name_time(
                CSVParser  *csv,
                const char *name,
                RETimeRes  *result);

char       *dsproc_get_next_csv_line(CSVParser *csv);

CSVParser  *dsproc_init_csv_parser(CSVParser *csv);
int         dsproc_load_csv_file(CSVParser *csv, const char *path, const char *name);

int         dsproc_parse_csv_header(CSVParser *csv, const char *linep);
int         dsproc_parse_csv_record(CSVParser *csv, char *linep, int flags);

int         dsproc_print_csv(FILE *fp, CSVParser *csv);
int         dsproc_print_csv_header(FILE *fp, CSVParser *csv);
int         dsproc_print_csv_record(FILE *fp, CSVParser *csv);

int         dsproc_set_csv_column_name(
                CSVParser  *csv,
                int         index,
                const char *name);

void        dsproc_set_csv_delimiter(
                CSVParser *csv,
                char       delim);

int         dsproc_set_csv_base_time(
                CSVParser   *csv,
                time_t       base_time);

int         dsproc_set_csv_file_time_patterns(
                CSVParser   *csv,
                int          npatterns,
                const char **patterns);

void        dsproc_set_csv_time_offset(
                CSVParser *csv,
                time_t     time_offset);

int         dsproc_set_csv_time_patterns(
                CSVParser   *csv,
                const char  *name,
                int          npatterns,
                const char **patterns);

/*@}*/

/******************************************************************************/
/**
 *  @defgroup DSPROC_CSV2CDS Ingest: CSV to CDS Mapping Functions
 */
/*@{*/

/** Mapping flag to specify that existing data should be overwritten */
#define CSV_OVERWRITE  0x1

/**
 *  Structure used to map a string to a data value.
 */
typedef struct {

    const char *strval; /**< string value in CSV file       */
    double      dblval; /**< value to use in output dataset */

} CSVStrMap;

/**
 *  Structure used to map CSVParser data to a CDSGroup.
 */
typedef struct {

    const char   *csv_name;     /**< column name in the CSV file        */
    const char   *csv_units;    /**< units string                       */
    const char  **csv_missings; /**< list if missing values in CSV data */
    const char   *cds_name;     /**< variable name in the CDS structure */

    /** list of string to double mapping structures */
    CSVStrMap    *str_map;

    /** function used to translate a string to a double */
    double      (*str_to_dbl)(const char *strval, int *status);

    /** advanced function for mapping CSV data to CDS variable data */
    int         (*set_data)(
        const char  *csv_strval,
        const char **csv_missings,
        CDSVar      *cds_var,
        size_t       cds_sample_size,
        CDSData      cds_missing,
        CDSData      cds_datap);

} CSV2CDSMap;

int     dsproc_map_csv_to_cds(
            CSVParser  *csv,
            int         csv_start,
            int         csv_count,
            CSV2CDSMap *map,
            CDSGroup   *cds,
            int         cds_start,
            int         flags);

int     dsproc_map_csv_to_cds_by_index(
            CSVParser  *csv,
            int        *csv_indexes,
            int         csv_count,
            CSV2CDSMap *map,
            CDSGroup   *cds,
            int         cds_start,
            int         flags);

int     dsproc_add_csv_str_to_dbl_function(
            CSV2CDSMap *map,
            const char *csv_name,
            double    (*str_to_dbl)(const char *strval, int *status));

/*@}*/

/******************************************************************************/
/**
 *  @defgroup DSPROC_CSV_INGEST_CONFIG Ingest: CSV Ingest Config
 */
/*@{*/

/** Flag used by csv_read_conf_file() to check for config files under
 *  the root directory defined by the CONF_DATA environment variable. */

#define CSV_CHECK_DATA_CONF  0x01

/**
 *  CSV Time Column Names and Patterns.
 */
typedef struct
{
    const char  *name;      /**< name of the date/time column            */
    int          npatterns; /**< number of possible time string patterns */
    const char **patterns;  /**< list of possile time string patterns    */

} CSVTimeCol;

/**
 *  CSV Field Map Structure.
 */
typedef struct
{
    const char  *out_name;  /**< name of the variable in the output dataset        */
    const char  *col_name;  /**< name of the column in the input CSV file          */
    const char  *units;     /**< units used in the CSV file                        */
    int          nmissings; /**< number of missing values used in the CSV file     */
    const char **missings;  /**< list of missing values used in the CSV file       */
    char        *missbuf;   /**< buffer used to parse the string of missing values */

} CSVFieldMap;

/**
 *  CSV Configuration Structure.
 */
typedef struct
{
    /* Set by csv_init_conf function */

    const char   *proc;           /**< the process name                      */
    const char   *site;           /**< the site name                         */
    const char   *fac;            /**< the facility name                     */
    const char   *name;           /**< the conf file base name               */
    const char   *level;          /**< the conf file level                   */

    /* Set by csv_read_conf_file */

    const char   *file_path;      /**< path to the configuration file        */
    const char   *file_name;      /**< name of the configuration file        */

    /* Used to find configuration files */

    int           search_npaths;  /**< number of conf file search paths      */
    const char  **search_paths;   /**< list of conf file search paths        */
    DirList      *dirlist;        /**< list of time varying conf files       */

    /* Read from conf file */

    int           fn_npatterns;   /**< number of csv file name patterns      */
    const char  **fn_patterns;    /**< list of csv file name patterns        */

    int           ft_npatterns;   /**< number of csv file time patterns      */
    const char  **ft_patterns;    /**< list of csv file time patterns        */

    char          delim;          /**< column delimiter                      */

    const char   *header_line;    /**< string containing the header line     */
    const char   *header_tag;     /**< string identifier for the header line */
    int           header_linenum; /**< line number of the first header line  */
    int           header_nlines;  /**< number of header lines                */

    int           exp_ncols;      /**< expected number of columns            */

    int           time_ncols;     /**< number of time columns                */
    CSVTimeCol   *time_cols;      /**< list of time columns                  */

    int           field_nmaps;    /**< number of entries in the field map     */
    CSVFieldMap  *field_maps;     /**< field map                              */

    const char   *split_interval; /**< split interval for output files        */

} CSVConf;

int         dsproc_add_csv_field_map(
                CSVConf     *conf,
                const char  *out_name,
                const char  *col_name,
                int          nargs,
                const char **args);

int         dsproc_add_csv_file_name_patterns(
                CSVConf     *conf,
                int          npatterns,
                const char **patterns);

int         dsproc_add_csv_file_time_patterns(
                CSVConf     *conf,
                int          npatterns,
                const char **patterns);

int         dsproc_add_csv_time_column_patterns(
                CSVConf     *conf,
                const char  *name,
                int          npatterns,
                const char **patterns);

int         dsproc_append_csv_header_line(
                CSVConf    *conf,
                const char *string);

void        dsproc_clear_csv_field_maps(CSVConf *conf);
void        dsproc_clear_csv_file_name_patterns(CSVConf *conf);
void        dsproc_clear_csv_file_time_patterns(CSVConf *conf);
void        dsproc_clear_csv_time_column_patterns(CSVConf *conf);

int         dsproc_configure_csv_parser(CSVConf *conf, CSVParser *csv);

CSV2CDSMap *dsproc_create_csv_to_cds_map(
                CSVConf   *conf,
                CSVParser *csv,
                CDSGroup  *cds,
                int        flags);

void        dsproc_free_csv_conf(CSVConf *conf);

void        dsproc_free_csv_to_cds_map(
                CSV2CDSMap *map);

CSVConf    *dsproc_init_csv_conf(
                const char *name,
                const char *level);

int         dsproc_load_csv_conf(
                CSVConf *conf,
                time_t   data_time,
                int      flags);

int         dsproc_print_csv_conf(
                FILE *fp, CSVConf *conf);

/*@}*/

#include "dsproc3_internal.h"

#endif /* _DSPROC3_H */
