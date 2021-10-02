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

/** @file dsproc_private.h
 *  Private DSPROC Functions.
 */

#ifndef _DSPROC_PRIVATE
#define _DSPROC_PRIVATE

/** @privatesection */

/** Macro to copy a string if the source string is not null. */
#define STRDUP_FAILED(s1,s2) ( s2 && !( s1 = strdup(s2) ) )


typedef struct OutputInterval OutputInterval;
typedef struct DataStream DataStream;

void _dsproc_trim_version(char *version);

/******************************************************************************/
/*
 *  @defgroup PRIVATE_DSPROC_MAIN Private: Process Main
 */
/*@{*/

/**
 *  DataSystem Process.
 */
typedef struct DSProc {

    /* Process Information */

    ProcModel    model;          /**< process model                          */
    const char  *version;        /**< process version                        */
    const char  *name;           /**< process name                           */
    const char  *type;           /**< process type                           */
    const char  *site;           /**< site name                              */
    const char  *facility;       /**< facility name                          */
    const char  *full_name;      /**< full process name including type       */

    const char   status[512];    /**< process status message                 */
    const char   disable[512];   /**< process disable message                */

    time_t       start_time;     /**< process start time                     */
    time_t       max_run_time;   /**< maximum runtime allowed                */
    time_t       data_interval;  /**< expected data interval                 */
    time_t       min_valid_time; /**< minimum valid data time                */

    int          ndatastreams;   /**< number of datastreams                  */
    DataStream **datastreams;    /**< datastreams                            */

    /* Processing Interval */

    time_t       cmd_line_begin;  /**< begin date specified on command line  */
    time_t       cmd_line_end;    /**< end date specified on command line    */

    time_t       period_begin;    /**< processing period begin time          */
    time_t       period_end;      /**< processing period end time            */
    time_t       period_end_max;  /**< maximum processing period end time    */

    time_t       proc_interval;   /**< processing interval                   */
    time_t       interval_begin;  /**< begin time of the processing interval */
    time_t       interval_end;    /**< end time of the processing interval   */
    time_t       interval_offset; /**< processing interval offset            */

    /* Processing Loop */

    time_t       loop_begin;     /**< begin time of the processing loop      */
    time_t       loop_end;       /**< end time of the processing loop        */
    int          use_obs_loop;   /**< loop over observations                 */

    /* Lockfile */

    char        *lockfile_path;  /**< path to the lockfile directory         */
    char        *lockfile_name;  /**< name of the lockfile                   */

    /* DSDB */

    const char  *db_alias;       /**< alias in the .db_connect file          */
    DSDB        *dsdb;           /**< database connection                    */
    ProcLoc     *location;       /**< process location                       */
    const char  *site_desc;      /**< site description                       */

    int          ndsc_inputs;    /**< number of input datastream classes     */
    DSClass    **dsc_inputs;     /**< input datastream classes               */

    int          ndsc_outputs;   /**< number of output datastream classes    */
    DSClass    **dsc_outputs;    /**< output datastream classes              */

    /* DQRDB */

    DQRDB       *dqrdb;          /**< DQR database connection                */

    /* Retriever Data */

    Retriever   *retriever;      /**< data retriever definition              */
    CDSGroup    *ret_data;       /**< root CDSGroup for retrieved data       */

    /* Transformation Data */

    CDSGroup    *trans_data;     /**< input data transformed to user specs   */
    CDSGroup    *trans_params;   /**< user defined transformation params     */

    /* Output Intervals */

    OutputInterval *output_intervals;

} DSProc;

const char *_dsproc_get_command_line(void);

void    _dsproc_ingest_parse_args(
            int          argc,
            char       **argv,
            int          nproc_names,
            const char **proc_names);

void    _dsproc_vap_parse_args(
            int          argc,
            char       **argv,
            int          nproc_names,
            const char **proc_names);

void    _dsproc_destroy(void);

void   *_dsproc_run_init_process_hook(void);
void    _dsproc_run_finish_process_hook(void);

int     _dsproc_run_process_data_hook(
            time_t    begin_date,
            time_t    end_date,
            CDSGroup *input_data);

int     _dsproc_run_pre_retrieval_hook(
            time_t   begin_date,
            time_t   end_date);

int     _dsproc_run_post_retrieval_hook(
            time_t    begin_date,
            time_t    end_date,
            CDSGroup *ret_data);

int     _dsproc_run_pre_transform_hook(
            time_t    begin_date,
            time_t    end_date,
            CDSGroup *ret_data);

int     _dsproc_run_post_transform_hook(
            time_t    begin_date,
            time_t    end_date,
            CDSGroup *trans_data);

int     _dsproc_run_quicklook_hook(
            time_t    begin_date,
            time_t    end_date);

int     _dsproc_run_process_file_hook(
            const char *input_dir,
            const char *file_name);

int     _dsproc_custom_qc_hook(
            int       ds_id,
            CDSGroup *dataset);

/*@}*/

/******************************************************************************/
/*
 *  @defgroup PRIVATE_DSPROC_VARTAG Private: Variable Tag
 */
/*@{*/

/**
 *  Variable tag.
 */
typedef struct {

    DataStream *in_ds;         /**< pointer to the input datastream          */
    const char *in_var_name;   /**< name of the variable in the input file   */

    const char *valid_min;     /**< valid minimum value                      */
    const char *valid_max;     /**< valid maximum value                      */
    const char *valid_delta;   /**< valid delta value                        */

    const char *coordsys_name; /**< the name of the target coordinate system */

    int         ntargets;      /**< the number of output variable targets    */
    VarTarget **targets;       /**< list of output variable targets          */

    int         ndqrs;         /**< the number of DQRs in the dqrs array     */
    VarDQR    **dqrs;          /**< list of DQRs for this variable           */

    /** the name of the datastream group as defined in the retriever. */
    const char *ret_group_name;
    int         required;      /**< flag indicating if this variable is required */

    int         flags;         /**< control flags */

} VarTag;

int _dsproc_create_ret_var_tag(
        CDSVar       *cds_var,
        RetDsGroup   *ret_group,
        RetVariable  *ret_var,
        DataStream   *in_ds,
        const char   *in_var_name);

/*@}*/

/******************************************************************************/
/**
 *  @defgroup PRIVATE_DSPROC_DQRDB Private: DQRDB
 */
/*@{*/

/**
 *  DataStream Variable DQRs typedef.
 */
typedef struct DSVarDQRs DSVarDQRs;

/**
 *  DataStream Variable DQRs.
 */
struct DSVarDQRs {

    DSVarDQRs  *next;     /**< pointer to the next variable in the list */
    const char *var_name; /**< variable name                            */
    int         ndqrs;    /**< number of DQRs in the list               */
    DQR       **dqrs;     /**< linked list of DQRs                      */
};

void _dsproc_free_dsvar_dqrs(DSVarDQRs *dsvar_dqrs);
void _dsproc_free_var_dqrs(VarDQR **var_dqrs);

/*@}*/

/******************************************************************************/
/**
 *  @defgroup PRIVATE_DSPROC_DATASTREAM_FILES Private: DataStream Files
 */
/*@{*/

typedef struct DSDir DSDir; /**< Datastream Directory Structure */

/**
 *  Datastream File Structure.
 */
typedef struct {

    DSDir       *dir;        /**< pointer to the parent datastream directory  */
    char        *name;       /**< name of the file                            */
    char        *full_path;  /**< the full path to the file                   */
    struct stat  stats;      /**< file stats                                  */
    int          touched;    /**< flag indicating if the file was used
                                  during the last processing loop             */

    int          mode;       /**< open mode of the file                       */
    int          ncid;       /**< ID of the open netcdf file                  */
    int          time_dimid; /**< ID of the time dimension in the file        */
    int          time_varid; /**< ID of the time variable in the file         */
    time_t       base_time;  /**< base time of the file                       */
    int          ntimes;     /**< number of times in the file                 */
    timeval_t   *timevals;   /**< array of time values                        */

    CDSGroup    *dod;        /**< CDSGroup containing the DOD for this file   */

} DSFile;

/**
 *  Datastream Directory Structure.
 */
struct DSDir {

    DataStream *ds;          /**< pointer to the parent datastrem      */
    char       *path;        /**< path to the directory                */
    struct stat stats;       /**< directory stats                      */

    REList     *patterns;    /**< list of file patterns to look for    */

    int         nfiles;      /**< number of files in the file list     */
    char      **files;       /**< list of files in the directory       */
    int         max_files;   /**< allocated length of the files list   */

    int         ndsfiles;    /**< number of cached dsfiles             */
    DSFile    **dsfiles;     /**< cached dsfiles                       */
    int         max_dsfiles; /**< allocated length of the dsfiles list */

    int         nopen;       /**< number of open files                 */
    int         max_open;    /**< maximum number of open files         */

    /** flag used to check for files with .v# extension and
     *  filter out lower versions. */
    int filter_versioned_files;

    /** function used by qsort to sort the file list */
    int  (*file_name_compare)(const void *, const void *);

    /** function used to get the time from the file name */
    time_t (*file_name_time)(const char *);

    /** List of compiled regex file name time patterns */
    RETimeList *file_name_time_patterns;
};

int     _dsproc_add_dsdir_patterns(
            DSDir       *dir,
            int          npatterns,
            const char **patterns,
            int          ignore_case);

char  **_dsproc_clone_file_list(int nfiles, char **file_list);

DSDir  *_dsproc_create_dsdir(const char *path);
void    _dsproc_free_dsdir(DSDir *dir);

int     _dsproc_find_dsdir_files(
            DSDir    *dir,
            time_t    begin_time,
            time_t    end_time,
            char   ***file_list);

int     _dsproc_find_dsfiles(
            DSDir     *dir,
            timeval_t *begin_timeval,
            timeval_t *end_timeval,
            DSFile  ***dsfile_list);

int     _dsproc_find_next_dsfile(
            DSDir     *dir,
            timeval_t *search_start,
            DSFile   **dsfile);

int     _dsproc_get_dsdir_files(DSDir *dir, char ***files);

time_t  _dsproc_get_file_name_time(
            DSDir      *dir,
            const char *file_name);

int     _dsproc_open_dsfile(DSFile *file, int mode);

int     _dsproc_set_input_file_list(const char *cmd_line_arg);
void    _dsproc_free_input_file_list(void);

/*@}*/

/******************************************************************************/
/*
 *  @defgroup PRIVATE_DSPROC_RETRIEVER Private: Retriever
 */
/*@{*/

/**
 *  Retriever DataStream File Structure.
 */
typedef struct {

    DSFile   *dsfile;         /**< pointer to the DSFile structure           */
    char     *version_string; /**< version string from input file            */
    timeval_t start_time;     /**< time of first record retrieved            */
    timeval_t end_time;       /**< time of last record retrieved             */

    size_t    sample_start; /**< start index of samples to retrieve           */
    size_t    sample_count; /**< number of samples to retrieve                */

    int       ndims;        /**< number of dimensions in this file            */
    int      *dimids;       /**< ids of the dimensions in the file            */
    char    **dim_names;    /**< names of the dimensions in the file          */
    size_t   *dim_lengths;  /**< lengths of the dimensions in the file        */
    int      *is_unlimdim;  /**< flags indicating if a dimension is unlimited */

    CDSGroup *obs_group;    /**< pointer to the CDS "observation" group       */
    int       var_count;    /**< the number of user requested variables found
                                 in this file. */
} RetDsFile;

/**
 *  Retriever DataStream Cache Structure.
 */
typedef struct {

    time_t      begin_offset;   /**< max begin time offset from processing interval */
    time_t      end_offset;     /**< max end time offset from processing interval   */
    time_t      dep_begin_date; /**< begin date dependency from retriever  */
    time_t      dep_end_date;   /**< end date dependency from retriever    */

    time_t      begin_time;     /**< adjusted begin time for the current
                                     processing interval                   */
    time_t      end_time;       /**< adjusted end time for the current
                                     processing interval                   */

    CDSGroup   *ds_group;       /**< pointer to the CDS datastream group   */

    int         nfiles;         /**< number of files in the list           */
    RetDsFile **files;          /**< list of files found within the
                                     current processing interval           */
} RetDsCache;

void            _dsproc_free_ret_ds_cache(RetDsCache *cache);
void            _dsproc_free_retriever();
RetCoordSystem *_dsproc_get_ret_coordsys(const char *name);
time_t          _dsproc_get_ret_data_base_time(void);
timeval_t       _dsproc_get_ret_data_end_time(void);
const char     *_dsproc_get_ret_data_time_desc(void);
const char     *_dsproc_get_ret_data_time_units(void);
int             _dsproc_get_ret_group_ds_id(CDSGroup *ret_ds_group);
int             _dsproc_init_retriever();

/*@}*/

/******************************************************************************/
/*
 *  @defgroup PRIVATE_DSPROC_DATASTREAM Private: DataStream
 */
/*@{*/

/**
 *  DataStream Structure.
 */
struct DataStream {

    const char  site[8];        /**< site name                                */
    const char  facility[8];    /**< facility name                            */
    const char  dsc_name[64];   /**< datastream class name                    */
    const char  dsc_level[8];   /**< datastream class level                   */
    DSRole      role;           /**< specifies input or output datastream     */

    const char *name;           /**< fully qualified datastream name          */
    DSFormat    format;         /**< datastream data format                   */
    const char  extension[64];  /**< datastream file extension                */
    int         flags;          /**< control flags                            */

    DSDir      *dir;            /**< datastream directory                     */

    /* datastream DOD */

    DSDOD      *dsdod;          /**< datastream dod retrieved from database   */
    CDSGroup   *metadata;       /**< datastream metadata set at runtime       */

    /* datastream properties */

    DSProp    **dsprops;        /**< NULL terminated array of DSProp pointers */
    int         ndsprops;       /**< number of datastream properties          */

    /* datastream DQRs */

    DSVarDQRs  *dsvar_dqrs;     /**< linked list of datastream variable DQRs  */

    /* retriever cache */

    RetDsCache *ret_cache;      /**< cached retriever datastream information  */

    /* fetched dataset */

    CDSGroup   *fetched_cds;    /**< pointer to the fetched dataset           */
    timeval_t   fetch_begin;    /**< begin time of previous fetch request     */
    timeval_t   fetch_end;      /**< end time of previous fetch request       */
    size_t      fetch_nvars;    /**< number of vars in previous fetch request */

    /* output dataset */

    CDSGroup   *out_cds;        /**< pointer to the output dataset            */

    /* output file splitting mode */

    SplitMode   split_mode;      /**< splitting mode (see SplitMode)           */
    double      split_start;     /**< the start of the split interval          */
    double      split_interval;  /**< split interval                           */
    int         split_tz_offset; /**< time zone offset                         */

    /* additional rename raw options */

    int         preserve_dots;  /**< portion of original name to preserve     */

    /* previously processed data times */

    timeval_t   ppdt_begin;     /**< previously processed data begin time     */
    timeval_t   ppdt_end;       /**< previously processed data end time       */

    /* current processing stats */

    int         total_files;    /**< total number of files processed          */
    double      total_bytes;    /**< total number of bytes processed          */
    int         total_records;  /**< total number of records processed        */
    timeval_t   begin_time;     /**< time of the first record processed       */
    timeval_t   end_time;       /**< time of the last record processed        */
};

void _dsproc_free_datastream_fetched_cds(DataStream *ds);
void _dsproc_free_datastream_out_cds(DataStream *ds);
void _dsproc_free_datastream_metadata(DataStream *ds);
void _dsproc_free_datastream(DataStream *ds);

int  _dsproc_init_input_datastreams(void);
int  _dsproc_init_output_datastreams(void);

const char *_dsproc_dsrole_to_name(DSRole role);

const char *_dsproc_dsformat_to_name(DSFormat format);
DSFormat    _dsproc_name_to_dsformat(const char *name);

/* Output intervals set from process definition in PCM */

struct OutputInterval {
    char      *dsc_name;        /**< datastream class name         */
    char      *dsc_level;       /**< datastream class level        */
    SplitMode  split_mode;      /**< the file splitting mode       */
    double     split_start;     /**< start of the split interval   */
    double     split_interval;  /**< split interval                */
    int        split_tz_offset; /**< time zone offset (in hours)   */
    OutputInterval *next;       /**< next structure in linked list */
};

int _dsproc_add_output_interval(
        const char *dsc_name,
        const char *dsc_level,
        SplitMode   split_mode,
        double      split_start,
        double      split_interval,
        int         split_tz_offset);

void _dsproc_free_output_intervals(void);

OutputInterval *_dsproc_get_output_interval(
            const char *dsc_name,
            const char *dsc_level);

int _dsproc_parse_output_interval_string(const char *string);

/*@}*/

/******************************************************************************/
/*
 *  @defgroup PRIVATE_DSPROC_DATASTREAM_DOD Private: Datastream DOD
 */
/*@{*/

int  _dsproc_get_dsdod(DataStream *ds, time_t data_time);

/*@}*/

/******************************************************************************/
/*
 *  @defgroup PRIVATE_DATASET_FETCH Private: Dataset Fetch
 */
/*@{*/

CDSGroup *_dsproc_fetch_dsfile_dataset(
            DSFile      *dsfile,
            size_t       start,
            size_t       count,
            size_t       nvars,
            const char **var_names,
            CDSGroup    *parent);

CDSGroup *_dsproc_fetch_dsfile_dod(
            DSFile *file);

int     _dsproc_fetch_dataset(
            int          ndsfiles,
            DSFile     **dsfiles,
            timeval_t   *begin_timeval,
            timeval_t   *end_timeval,
            size_t       nvars,
            const char **var_names,
            int          merge_obs,
            CDSGroup    *parent);

timeval_t *_dsproc_fetch_timevals(
            DataStream  *ds,
            int          ndsfiles,
            DSFile     **dsfiles,
            timeval_t   *begin_timeval,
            timeval_t   *end_timeval,
            size_t      *ntimevals,
            timeval_t   *timevals);

/*@}*/

/******************************************************************************/
/*
 *  @defgroup PRIVATE_DATASET_FILTERS Private: Dataset Filters
 */
/*@{*/

int _dsproc_filter_duplicate_samples(
        size_t    *ntimes,
        timeval_t *times,
        CDSGroup  *dataset);

int _dsproc_filter_stored_samples(
        DataStream *ds,
        size_t     *ntimes,
        timeval_t  *times,
        CDSGroup   *dataset);

/*@}*/

/******************************************************************************/
/*
 *  @defgroup PRIVATE_DATASET_COMPARE Private: Dataset Compare
 */
/*@{*/

void _dsproc_free_exclude_atts(void);
int  _dsproc_set_standard_exclude_atts(void);

/*@}*/

/******************************************************************************/
/*
 *  @defgroup PRIVATE_DATASET_STORE Private: Dataset Store
 */
/*@{*/

int _dsproc_update_stored_metadata(CDSGroup *dataset, int ncid);

/*@}*/

/******************************************************************************/
/*
 *  @defgroup PRIVATE_DATASET_QC Private: Dataset QC
 */
/*@{*/

void _dsproc_free_excluded_qc_vars(void);

/*@}*/

/******************************************************************************/
/*
 *  @defgroup PRIVATE_QC_UTILS Private: QC Utils
 */
/*@{*/

int _dsproc_consolidate_var_qc(
    CDSVar       *in_qc_var,
    size_t        in_sample_start,
    size_t        sample_count,
    unsigned int  bad_mask,
    CDSVar       *out_qc_var,
    size_t        out_sample_start,
    unsigned int  bad_flag,
    unsigned int  ind_flag,
    int           reset);

/*@}*/

/******************************************************************************/
/*
 *  @defgroup PRIVATE_DATASET_VARS Private: Dataset Vars
 */
/*@{*/

void _dsproc_fix_field_order(CDSGroup *ds);

/*@}*/

/******************************************************************************/
/*
 *  @defgroup PRIVATE_DSPROC_DSDB Private: DSDB
 */
/*@{*/

int _dsproc_get_output_datastream_times(DataStream *ds);
int _dsproc_store_output_datastream_times(void);

/*@}*/

/******************************************************************************/
/*
 *  @defgroup PRIVATE_DSPROC_UTILS Private: DSPROC Utils
 */
/*@{*/

int _dsproc_merge_obs(CDSGroup *parent);

/*@}*/

/******************************************************************************/
/*
 *  @defgroup PRIVATE_DSPROC_TRANSFORM Private: DSPROC Transformation
 */
/*@{*/

CDSVar *_dsproc_create_consolidated_trans_qc_var(
            CDSVar     *trans_qc_var,
            CDSGroup   *out_group,
            const char *out_qc_var_name);

void    _dsproc_free_trans_qc_rollup_bit_descriptions(void);

int     _dsproc_set_trans_params_from_bounds_var(CDSDim *dim);
int     _dsproc_set_trans_params_from_dsprops(int dsid, CDSDim *dim);
int     _dsproc_set_ret_dim_trans_params(int dsid, CDSDim *dim);
int     _dsproc_set_ret_obs_params(int dsid, CDSGroup *obs);

int     _dsproc_set_trans_coord_var_params(
            CDSVar      *trans_coord_var,
            int          ret_dsid,
            RetCoordDim *ret_coord_dim);

/*@}*/

/******************************************************************************/
/*
 *  @defgroup PRIVATE_DSPROC_TRANS_DIM_GROUPS Private: Trans Dim Groups
 */
/*@{*/

typedef struct {

    char  *in_dim;
    int    out_ndims;
    char **out_dims;

} TransDimGroup;

void _dsproc_free_trans_dim_groups(TransDimGroup *trans_dim_groups);

int  _dsproc_get_trans_dim_groups(
        CDSGroup       *ds_group,
        const char     *var_name,
        TransDimGroup **trans_dim_groups);

/*@}*/

#endif /* _DSPROC_PRIVATE */
