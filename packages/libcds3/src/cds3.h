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
*    $Revision: 68010 $
*    $Author: ermold $
*    $Date: 2016-03-11 20:27:44 +0000 (Fri, 11 Mar 2016) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file cds3.h
 *  CDS Library Header File.
 */

#ifndef _CDS3_H
#define _CDS3_H

#include <stdlib.h>
#include <string.h>

#include "msngr.h"

/** CDS library name. */
#define CDS_LIB_NAME "libcds3"

/* structure typedefs */

typedef struct CDSObject    CDSObject;   /**< CDS Object         */
typedef struct CDSGroup     CDSGroup;    /**< CDS Group          */
typedef struct CDSDim       CDSDim;      /**< CDS Dimension      */
typedef struct CDSAtt       CDSAtt;      /**< CDS Attribute      */
typedef struct CDSVar       CDSVar;      /**< CDS Variable       */

typedef struct CDSVarArray  CDSVarArray; /**< CDS Variable Array */
typedef struct CDSVarGroup  CDSVarGroup; /**< CDS Variable Group */

/* copy and print flags */

#define CDS_SKIP_DIMS       0x00001 /**< skip dimensions                      */
#define CDS_SKIP_GROUP_ATTS 0x00002 /**< skip group attributes                */
#define CDS_SKIP_VAR_ATTS   0x00004 /**< skip variable attributes             */
#define CDS_SKIP_VARS       0x00008 /**< skip variables                       */

#define CDS_SKIP_DATA       0x00010 /**< skip variable data                   */
#define CDS_SKIP_SUBGROUPS  0x00020 /**< do not traverse into subgroups       */

#define CDS_PRINT_VARGROUPS 0x00100 /**< print variable groups                */

#define CDS_COPY_LOCKS      0x01000 /**< copy definition lock values          */
#define CDS_EXCLUSIVE       0x02000 /**< exclude objects that have not been
                                         defined in the destination parent    */

#define CDS_OVERWRITE_DIMS  0x10000 /**< overwrite existing dimension lengths */
#define CDS_OVERWRITE_ATTS  0x20000 /**< overwrite existing attribute values  */
#define CDS_OVERWRITE_DATA  0x40000 /**< overwrite existing variable data     */

/** overwrite existing object data */
#define CDS_OVERWRITE \
CDS_OVERWRITE_DIMS | CDS_OVERWRITE_ATTS | CDS_OVERWRITE_DATA

/* default _FillValues used by the NetCDF library (see netcdf.h) */

#define CDS_FILL_CHAR    ((char)0)
#define CDS_FILL_BYTE    ((signed char)-127)
#define CDS_FILL_SHORT   ((short)-32767)
#define CDS_FILL_INT     (-2147483647)
#define CDS_FILL_FLOAT   (9.9692099683868690e+36f) /* near 15 * 2^119 */
#define CDS_FILL_DOUBLE  (9.9692099683868690e+36)

/* data type ranges used by the NetCDF library (see netcdf.h) */

#define CDS_MAX_CHAR     255
#define CDS_MIN_CHAR     0
#define CDS_MAX_BYTE     127
#define CDS_MIN_BYTE     (-CDS_MAX_BYTE-1)
#define CDS_MAX_SHORT    32767
#define CDS_MIN_SHORT    (-CDS_MAX_SHORT - 1)
#define CDS_MAX_INT      2147483647
#define CDS_MIN_INT      (-CDS_MAX_INT - 1)
#define CDS_MAX_FLOAT    3.402823466e+38f
#define CDS_MIN_FLOAT    (-CDS_MAX_FLOAT)
#define CDS_MAX_DOUBLE   1.7976931348623157e+308
#define CDS_MIN_DOUBLE   (-CDS_MAX_DOUBLE)

/* Maximum size of a data type */

#define CDS_MAX_TYPE_SIZE  sizeof(double)

/******************************************************************************/
/**
 *  @defgroup CDSObject Objects
 */
/*@{*/

/**
 *  CDS Object Type.
 */
typedef enum {
    CDS_GROUP    = 1,          /**< CDS Group          */
    CDS_DIM      = 2,          /**< CDS Dimension      */
    CDS_ATT      = 3,          /**< CDS Attribute      */
    CDS_VAR      = 4,          /**< CDS Variable       */
    CDS_VARGROUP = 5,          /**< CDS Variable Group */
    CDS_VARARRAY = 6,          /**< CDS Variable Array */
} CDSObjectType;

/**
 *  CDS User Data.
 */
typedef struct CDSUserData {
    char  *key;                 /**< user defined key                */
    void  *value;               /**< user defined value              */
    void (*free_value)(void *); /**< function used to free the value */
} CDSUserData;

/**
 *  CDS Object Definition.
 */
#define _CDS_OBJECT_ \
    CDSObject     *parent;      /**< parent object           */ \
    CDSObjectType  obj_type;    /**< object type             */ \
    char          *obj_path;    /**< object path             */ \
    int            def_lock;    /**< definition lock         */ \
    CDSUserData  **user_data;   /**< user defined data       */ \
    char          *name;        /**< object name             */

/**
 *  CDS Object.
 */
struct CDSObject {
    _CDS_OBJECT_
};

const char *cds_get_object_path(void *cds_object);

void    cds_set_definition_lock(void *cds_object, int value);

void    cds_delete_user_data(void *cds_object, const char *key);
void   *cds_get_user_data(void *cds_object, const char *key);
int     cds_set_user_data(
            void       *cds_object,
            const char *key,
            void       *value,
            void      (*free_value)(void *));

/*@}*/

/******************************************************************************/
/**
 *  @defgroup CDS_DATA_TYPES Data Types
 */
/*@{*/

/**
 *  CDS Data Types.
 */
typedef enum {
    CDS_NAT     = 0,            /**< Not A Type                             */
    CDS_CHAR    = 1,            /**< ISO/ASCII character                    */
    CDS_BYTE    = 2,            /**< signed 1 byte integer                  */
    CDS_SHORT   = 3,            /**< signed 2 byte integer                  */
    CDS_INT     = 4,            /**< signed 4 byte integer                  */
    CDS_FLOAT   = 5,            /**< single precision floating point number */
    CDS_DOUBLE  = 6,            /**< double precision floating point number */
} CDSDataType;

/**
 *  CDS Data Union.
 */
typedef union {
    void        *vp;            /**< void:   void pointer                    */
    char        *cp;            /**< char:   ISO/ASCII character             */
    signed char *bp;            /**< byte:   signed 1 byte integer           */
    short       *sp;            /**< short:  signed 2 byte integer           */
    int         *ip;            /**< int:    signed 4 byte integer           */
    float       *fp;            /**< float:  single precision floating point */
    double      *dp;            /**< double: double precision floating point */
} CDSData;

CDSDataType cds_data_type(const char *name);
const char *cds_data_type_name(CDSDataType type);
size_t      cds_data_type_size(CDSDataType type);

void        cds_get_data_type_range(CDSDataType type, void *min, void *max);
void        cds_get_default_fill_value(CDSDataType type, void *value);

size_t      cds_max_type_size(void);

/*@}*/

/******************************************************************************/
/**
 *  @defgroup CDS_GROUPS Groups
 */
/*@{*/

/**
 *  CDS Group.
 */
struct CDSGroup {

    _CDS_OBJECT_

    int           ndims;        /**< number of dimensions             */
    CDSDim      **dims;         /**< array of dimension pointers      */

    int           natts;        /**< number of attributes             */
    CDSAtt      **atts;         /**< array of attribute pointers      */

    int           nvars;        /**< number of variables              */
    CDSVar      **vars;         /**< array of variable pointers       */

    int           ngroups;      /**< number of groups                 */
    CDSGroup    **groups;       /**< array of group pointers          */

    int           nvargroups;   /**< number of variable groups        */
    CDSVarGroup **vargroups;    /**< array of variable group pointers */

    void         *transform_params; /**< transformation parameters          */
};

CDSGroup   *cds_define_group(CDSGroup *parent, const char *name);
int         cds_delete_group(CDSGroup *group);
CDSGroup   *cds_get_group   (CDSGroup *parent, const char *name);
int         cds_rename_group(CDSGroup *group, const char *name);

/*@}*/

/******************************************************************************/
/**
 *  @defgroup CDS_DIMS Dimensions
 */
/*@{*/

/**
 *  CDS Dimension.
 */
struct CDSDim {

    _CDS_OBJECT_

    size_t  length;             /**< dimension length                        */
    int     is_unlimited;       /**< is unlimited flag (0 = FALSE, 1 = TRUE) */
};

int     cds_change_dim_length(CDSDim *dim, size_t length);

CDSDim *cds_define_dim(
            CDSGroup   *group,
            const char *name,
            size_t      length,
            int         is_unlimited);

int     cds_delete_dim(CDSDim *dim);

CDSDim *cds_get_dim(CDSGroup *group, const char *name);

CDSVar *cds_get_dim_var(CDSDim *dim);

int     cds_rename_dim(CDSDim *dim, const char *name);

/*@}*/

/******************************************************************************/
/**
 *  @defgroup CDS_ATTS Attributes
 */
/*@{*/

/**
 *  CDS Attribute.
 */
struct CDSAtt {

    _CDS_OBJECT_

    CDSDataType  type;          /**< attribute data type           */
    size_t       length;        /**< length of the attribute value */
    CDSData      value;         /**< attribute value               */
};

CDSAtt *cds_change_att(
            void        *parent,
            int          overwrite,
            const char  *name,
            CDSDataType  type,
            size_t       length,
            void        *value);

int     cds_change_att_value(
            CDSAtt      *att,
            CDSDataType  type,
            size_t       length,
            void        *value);

int     cds_change_att_text(
            CDSAtt     *att,
            const char *format, ...);

int     cds_change_att_va_list(
            CDSAtt     *att,
            const char *format,
            va_list     args);

int     cds_create_missing_value_att(
            CDSVar *var,
            int     flags);

CDSAtt *cds_define_att(
            void        *parent,
            const char  *name,
            CDSDataType  type,
            size_t       length,
            void        *value);

CDSAtt *cds_define_att_text(
            void       *parent,
            const char *name,
            const char *format, ...);

CDSAtt *cds_define_att_va_list(
            void       *parent,
            const char *name,
            const char *format,
            va_list     args);

int     cds_delete_att(CDSAtt *att);

CDSAtt *cds_get_att(void *parent, const char *name);

void   *cds_get_att_value(
            CDSAtt       *att,
            CDSDataType   type,
            size_t       *length,
            void         *value);

char   *cds_get_att_text(
            CDSAtt *att,
            size_t *length,
            char   *value);

int     cds_is_missing_value_att_name(const char *att_name);

int     cds_rename_att(CDSAtt *att, const char *name);

CDSAtt *cds_set_att(
            void        *parent,
            int          overwrite,
            const char  *name,
            CDSDataType  type,
            size_t       length,
            void        *value);

int     cds_set_att_value(
            CDSAtt      *att,
            CDSDataType  type,
            size_t       length,
            void        *value);

int     cds_set_att_text(
            CDSAtt     *att,
            const char *format, ...);

int     cds_set_att_va_list(
            CDSAtt     *att,
            const char *format,
            va_list     args);

/*@}*/

/******************************************************************************/
/**
 *  @defgroup CDS_VARS Variables
 */
/*@{*/

/**
 *  CDS Variable.
 */
struct CDSVar {

    _CDS_OBJECT_

    CDSDataType  type;           /**< data type                   */

    int          ndims;          /**< number of dimensions        */
    CDSDim     **dims;           /**< array of dimension pointers */

    int          natts;          /**< number of attributes        */
    CDSAtt     **atts;           /**< array of attribute pointers */

    /* data */

    size_t       sample_count;   /**< number of samples in the data array */
    size_t       alloc_count;    /**< number of samples allocated         */
    CDSData      data;           /**< array of data values                */

    /* data index */

    void        *data_index;         /**< data index array                   */
    int          data_index_ndims;   /**< number of dims in data index array */
    size_t      *data_index_lengths; /**< dimension lengths of data index    */

    /* default fill value */

    void        *default_fill;   /**< default fill value                 */
};

CDSVar *cds_define_var(
            CDSGroup    *group,
            const char  *name,
            CDSDataType  type,
            int          ndims,
            const char **dim_names);

int     cds_delete_var(CDSVar *var);

CDSVar *cds_get_bounds_coord_var(CDSVar *bounds_var);

CDSVar *cds_get_bounds_var(CDSVar *coord_var);

CDSVar *cds_get_coord_var(CDSVar *var, int dim_index);

CDSVar *cds_get_var(CDSGroup *group, const char *name);

int     cds_rename_var(CDSVar *var, const char *name);

int     cds_var_is_unlimited(CDSVar *var);
CDSDim *cds_var_has_dim(CDSVar *var, const char *name);
size_t  cds_var_sample_size(CDSVar *var);

/*@}*/

/******************************************************************************/
/**
 *  @defgroup CDS_VAR_DATA Variable Data
 */
/*@{*/

void   *cds_alloc_var_data(
            CDSVar *var,
            size_t  sample_start,
            size_t  sample_count);

void   *cds_alloc_var_data_index(
            CDSVar *var,
            size_t  sample_start,
            size_t  sample_count);

int     cds_change_var_type(
            CDSVar      *var,
            CDSDataType  type);

int     cds_change_var_units(
            CDSVar      *var,
            CDSDataType  type,
            const char  *units);

void   *cds_create_var_data_index(CDSVar *var);

void    cds_delete_var_data(CDSVar *var);

void   *cds_get_var_data(
            CDSVar       *var,
            CDSDataType   type,
            size_t        sample_start,
            size_t       *sample_count,
            void         *missing_value,
            void         *data);

void   *cds_get_var_datap(CDSVar *var, size_t sample_start);

int     cds_get_var_missing_values(CDSVar *var, void **values);

const char *cds_get_var_units(CDSVar *var);

void   *cds_init_var_data(
            CDSVar *var,
            size_t  sample_start,
            size_t  sample_count,
            int     use_missing);

void   *cds_init_var_data_index(
            CDSVar *var,
            size_t  sample_start,
            size_t  sample_count,
            int     use_missing);

void    cds_reset_sample_counts(
            CDSGroup *group,
            int       unlim_vars,
            int       static_vars);

int     cds_set_bounds_data(
            CDSGroup *group,
            size_t    sample_start,
            size_t    sample_count);

int     cds_set_bounds_var_data(
            CDSVar *coord_var,
            size_t  sample_start,
            size_t  sample_count);

int     cds_set_var_default_fill_value(CDSVar *var, void *fill_value);

void   *cds_set_var_data(
            CDSVar       *var,
            CDSDataType   type,
            size_t        sample_start,
            size_t        sample_count,
            void         *missing_value,
            void         *data);

/*@}*/

/******************************************************************************/
/**
 *  @defgroup CDS_TIME_DATA Time Data
 */
/*@{*/

#ifndef _TIMEVAL_T
#define _TIMEVAL_T
/**
 *  typedef for: struct timeval.
 *
 *  Structure Members:
 *
 *    - tv_sec  - seconds
 *    - tv_usec - microseconds
 */
typedef struct timeval timeval_t;
#endif  /* _TIMEVAL_T */

CDSVar *cds_find_time_var(void *object);

time_t  cds_get_base_time(
            void *object);

size_t  cds_get_time_range(
            void      *object,
            timeval_t *start_time,
            timeval_t *end_time);

time_t *cds_get_sample_times(
            void   *object,
            size_t  sample_start,
            size_t *sample_count,
            time_t *sample_times);

timeval_t *cds_get_sample_timevals(
            void      *object,
            size_t     sample_start,
            size_t    *sample_count,
            timeval_t *timevals);

int     cds_set_base_time(
            void       *object,
            const char *long_name,
            time_t      base_time);

int     cds_set_sample_times(
            void   *object,
            size_t  sample_start,
            size_t  sample_count,
            time_t *times);

int     cds_set_sample_timevals(
            void      *object,
            size_t     sample_start,
            size_t     sample_count,
            timeval_t *timevals);

/*@}*/

/******************************************************************************/
/**
 *  @defgroup CDS_TIME_UTILITIES Time Utilities
 */
/*@{*/

#ifndef _TIMEVAL_MACROS
#define _TIMEVAL_MACROS

/** Cast timeval to double. */
#define TV_DOUBLE(tv) \
    ( (double)(tv).tv_sec + (1E-6 * (double)(tv).tv_usec) )

/** Check if timeval 1 is equal to timeval 2. */
#define TV_EQ(tv1,tv2) \
    ( ( (tv1).tv_sec  == (tv2).tv_sec) && \
      ( (tv1).tv_usec == (tv2).tv_usec) )

/** Check if timeval 1 is not equal to timeval 2. */
#define TV_NEQ(tv1,tv2) \
    ( ( (tv1).tv_sec  != (tv2).tv_sec) || \
      ( (tv1).tv_usec != (tv2).tv_usec) )

/** Check if timeval 1 is greater than timeval 2. */
#define TV_GT(tv1,tv2) \
    ( ( (tv1).tv_sec == (tv2).tv_sec)  ? \
      ( (tv1).tv_usec > (tv2).tv_usec) : \
      ( (tv1).tv_sec  > (tv2).tv_sec) )

/** Check if timeval 1 is greater than or equal to timeval 2. */
#define TV_GTEQ(tv1,tv2) \
    ( ( (tv1).tv_sec == (tv2).tv_sec)  ? \
      ( (tv1).tv_usec >= (tv2).tv_usec) : \
      ( (tv1).tv_sec  > (tv2).tv_sec) )

/** Check if timeval 1 is less than timeval 2. */
#define TV_LT(tv1,tv2) \
    ( ( (tv1).tv_sec == (tv2).tv_sec)  ? \
      ( (tv1).tv_usec < (tv2).tv_usec) : \
      ( (tv1).tv_sec  < (tv2).tv_sec) )

/** Check if timeval 1 is less than or equal to timeval 2. */
#define TV_LTEQ(tv1,tv2) \
    ( ( (tv1).tv_sec == (tv2).tv_sec)  ? \
      ( (tv1).tv_usec <= (tv2).tv_usec) : \
      ( (tv1).tv_sec  < (tv2).tv_sec) )

#endif  /* _TIMEVAL_MACROS */

#define CDS_EQ    0x1              /**< equal to flag                   */
#define CDS_LT    0x2              /**< less than flag                  */
#define CDS_GT    0x4              /**< greater than                    */
#define CDS_LTEQ  CDS_LT | CDS_EQ  /**< less than or equal to flags     */
#define CDS_GTEQ  CDS_GT | CDS_EQ  /**< greater than or equal to  flags */

int     cds_find_time_index(
            size_t   ntimes,
            time_t  *times,
            time_t   ref_time,
            int      mode);

int     cds_find_timeval_index(
            size_t      ntimevals,
            timeval_t  *timevals,
            timeval_t   ref_timeval,
            int         mode);

time_t  cds_get_midnight(
            time_t data_time);

int     cds_is_time_var(
            CDSVar *var,
            int    *is_base_time);

int     cds_base_time_to_units_string(
            time_t  base_time,
            char   *units_string);

int     cds_units_string_to_base_time(
            char   *units_string,
            time_t *base_time);

/*@}*/

/******************************************************************************/
/**
 *  @defgroup CDS_VAR_GROUPS Variable Groups
 */
/*@{*/

/**
 *  CDS Variable Group.
 */
struct CDSVarGroup {

    _CDS_OBJECT_

    int           narrays;      /**< number of variable arrays in the group */
    CDSVarArray **arrays;       /**< array of variable array pointers       */
};

CDSVarArray *cds_add_vargroup_vars(
                CDSVarGroup *vargroup,
                const char  *name,
                int          nvars,
                CDSVar     **vars);

CDSVarGroup *cds_define_vargroup(
                CDSGroup   *group,
                const char *name);

int          cds_delete_vargroup(
                CDSVarGroup *vargroup);

CDSVarGroup *cds_get_vargroup(
                CDSGroup   *group,
                const char *name);

/*@}*/

/******************************************************************************/
/**
 *  @defgroup CDS_VAR_ARRAYS Variable Arrays
 */
/*@{*/

/**
 *  CDS Variable Array.
 */
struct CDSVarArray {

    _CDS_OBJECT_

    int       nvars;            /**< number of variables in the array */
    CDSVar  **vars;             /**< array of variable pointers       */
};

int          cds_add_vararray_vars(
                CDSVarArray *vararray,
                int          nvars,
                CDSVar     **vars);

CDSVarArray *cds_create_vararray(
                CDSGroup     *group,
                const char   *vargroup_name,
                const char   *vararray_name,
                int           nvars,
                const char  **var_names);

CDSVarArray *cds_define_vararray(
                CDSVarGroup *vargroup,
                const char  *name);

int          cds_delete_vararray(
                CDSVarArray *vararray);

CDSVarArray *cds_get_vararray(
                CDSVarGroup *vargroup,
                const char  *name);

/*@}*/

/******************************************************************************/
/**
 *  @defgroup CDS_PRINT Print Functions
 */
/*@{*/

int     cds_print(
            FILE     *fp,
            CDSGroup *group,
            int       flags);

int     cds_print_att(
            FILE       *fp,
            const char *indent,
            int         min_width,
            CDSAtt     *att);

int     cds_print_atts(
            FILE       *fp,
            const char *indent,
            void       *parent);

int     cds_print_dim(
            FILE       *fp,
            const char *indent,
            int         min_width,
            CDSDim     *dim);

int     cds_print_dims(
            FILE       *fp,
            const char *indent,
            CDSGroup   *group);

int     cds_print_var(
            FILE       *fp,
            const char *indent,
            CDSVar     *var,
            int         flags);

int     cds_print_vars(
            FILE       *fp,
            const char *indent,
            CDSGroup   *group,
            int         flags);

int     cds_print_var_data(
            FILE       *fp,
            const char *label,
            const char *indent,
            CDSVar     *var);

int     cds_print_data(
            FILE       *fp,
            const char *indent,
            CDSGroup   *group);

int     cds_print_group(
            FILE       *fp,
            const char *indent,
            CDSGroup   *group,
            int         flags);

int     cds_print_groups(
            FILE       *fp,
            const char *indent,
            CDSGroup   *group,
            int         flags);

int     cds_print_vararray(
            FILE        *fp,
            const char  *indent,
            CDSVarArray *vararray,
            int          flags);

int     cds_print_vargroup(
            FILE        *fp,
            const char  *indent,
            CDSVarGroup *vargroup,
            int          flags);

int     cds_print_vargroups(
            FILE       *fp,
            const char *indent,
            CDSGroup   *group,
            int         flags);

/*@}*/

/******************************************************************************/
/**
 *  @defgroup CDS_TRANSFORM_PARAMS Transformation Parameters
 */
/*@{*/

int     cds_copy_transform_params(
            CDSGroup    *src_group,
            CDSGroup    *dest_group);

int     cds_set_transform_param(
            CDSGroup    *group,
            const char  *obj_name,
            const char  *param_name,
            CDSDataType  type,
            size_t       length,
            void        *value);

void   *cds_get_transform_param(
            void        *object,
            const char  *param_name,
            CDSDataType  type,
            size_t      *length,
            void        *value);

void   *cds_get_transform_param_from_group(
            CDSGroup    *group,
            const char  *obj_name,
            const char  *param_name,
            CDSDataType  type,
            size_t      *length,
            void        *value);

int     cds_load_transform_params_file(
            CDSGroup   *group,
            const char *path,
            const char *file);

int     cds_parse_transform_params(
            CDSGroup   *group,
            char       *string,
            const char *path);

int     cds_print_group_transform_params(
            FILE       *fp,
            const char *indent,
            CDSGroup   *group,
            const char *obj_name);

int     cds_print_transform_params(
            FILE       *fp,
            const char *indent,
            CDSGroup   *group,
            const char *obj_name);

/*@}*/

/******************************************************************************/
/**
 *  @defgroup CDS_CORE_CONVERTER Core Converter Functions
 */
/*@{*/

#define CDS_IGNORE_UNITS  0x01  /**< do not apply units conversion */
#define CDS_DELTA_UNITS   0x02  /**< convert units using deltas    */

/**
 *  CDS data converter.
 */
typedef void * CDSConverter;

int     cds_add_data_att(const char *name, int flags);
void    cds_free_data_atts(void);
int     cds_is_data_att(CDSAtt *att, int *flags);

void   *cds_convert_array(
            CDSConverter  converter,
            int           flags,
            size_t        length,
            void         *in_data,
            void         *out_data);

int     cds_convert_var(
            CDSConverter  converter,
            CDSVar       *var);

CDSConverter cds_create_converter(
            CDSDataType  in_type,
            const char  *in_units,
            CDSDataType  out_type,
            const char  *out_units);

CDSConverter cds_create_converter_array_to_var(
            CDSDataType  in_type,
            const char  *in_units,
            size_t       in_nmissing,
            void        *in_missing,
            CDSVar      *out_var);

CDSConverter cds_create_converter_var_to_array(
            CDSVar      *in_var,
            CDSDataType  out_type,
            const char  *out_units,
            size_t       out_nmissing,
            void        *out_missing);

CDSConverter cds_create_converter_var_to_var(
            CDSVar *in_var,
            CDSVar *out_var);

void    cds_destroy_converter(
            CDSConverter converter);

int     cds_set_converter_map(
            CDSConverter converter,
            size_t       in_map_length,
            void        *in_map,
            size_t       out_map_length,
            void        *out_map);

int     cds_set_converter_range(
            CDSConverter  converter,
            void         *out_min,
            void         *orv_min,
            void         *out_max,
            void         *orv_max);

/*@}*/

/******************************************************************************/
/**
 *  @defgroup CDS_CORE_COPY Core Copy Functions
 */
/*@{*/

int cds_copy_att(
        CDSAtt      *src_att,
        void        *dest_parent,
        const char  *dest_name,
        int          flags,
        CDSAtt     **dest_att);

int cds_copy_atts(
        void        *src_parent,
        void        *dest_parent,
        const char **src_names,
        const char **dest_names,
        int          flags);

int cds_copy_dim(
        CDSDim     *src_dim,
        CDSGroup   *dest_group,
        const char *dest_name,
        int         flags,
        CDSDim    **dest_dim);

int cds_copy_dims(
        CDSGroup    *src_group,
        CDSGroup    *dest_group,
        const char **src_names,
        const char **dest_names,
        int          flags);

int cds_copy_var(
        CDSVar      *src_var,
        CDSGroup    *dest_group,
        const char  *dest_name,
        const char **src_dim_names,
        const char **dest_dim_names,
        const char **src_att_names,
        const char **dest_att_names,
        size_t       src_start,
        size_t       dest_start,
        size_t       sample_count,
        int          flags,
        CDSVar     **dest_var);

int cds_copy_vars(
        CDSGroup    *src_group,
        CDSGroup    *dest_group,
        const char **src_dim_names,
        const char **dest_dim_names,
        const char **src_var_names,
        const char **dest_var_names,
        size_t       src_start,
        size_t       dest_start,
        size_t       sample_count,
        int          flags);

int cds_copy_group(
        CDSGroup    *src_group,
        CDSGroup    *dest_parent,
        const char  *dest_name,
        const char **src_dim_names,
        const char **dest_dim_names,
        const char **src_att_names,
        const char **dest_att_names,
        const char **src_var_names,
        const char **dest_var_names,
        const char **src_subgroup_names,
        const char **dest_subgroup_names,
        size_t       src_start,
        size_t       dest_start,
        size_t       sample_count,
        int          flags,
        CDSGroup   **dest_group);

int cds_copy_subgroups(
        CDSGroup    *src_group,
        CDSGroup    *dest_group,
        const char **src_dim_names,
        const char **dest_dim_names,
        const char **src_att_names,
        const char **dest_att_names,
        const char **src_var_names,
        const char **dest_var_names,
        const char **src_subgroup_names,
        const char **dest_subgroup_names,
        size_t       src_start,
        size_t       dest_start,
        size_t       sample_count,
        int          flags);

/*@}*/

/******************************************************************************/
/**
 *  @defgroup CDS_CORE_UNITS Core Units Functions
 */
/*@{*/

/**
 *  generic unit converter type.
 */
typedef void * CDSUnitConverter;

int     cds_compare_units(
            const char *from_units,
            const char *to_units);

void    *cds_convert_units(
            CDSUnitConverter  converter,
            CDSDataType       in_type,
            size_t            length,
            void             *in_data,
            CDSDataType       out_type,
            void             *out_data,
            size_t            nmap,
            void             *in_map,
            void             *out_map,
            void             *out_min,
            void             *orv_min,
            void             *out_max,
            void             *orv_max);

void    *cds_convert_unit_deltas(
            CDSUnitConverter  converter,
            CDSDataType       in_type,
            size_t            length,
            void             *in_data,
            CDSDataType       out_type,
            void             *out_data,
            size_t            nmap,
            void             *in_map,
            void             *out_map);

void    cds_free_unit_converter(CDSUnitConverter converter);

void    cds_free_unit_system(void);

int     cds_get_unit_converter(
            const char       *from_units,
            const char       *to_units,
            CDSUnitConverter *converter);

int     cds_init_unit_system(const char *xml_db_path);

int     cds_map_symbol_to_unit(const char *symbol, const char *unit_name);

time_t  cds_validate_time_units(char *time_units);

/*@}*/

/******************************************************************************/
/**
 *  @defgroup CDS_CORE_UTILITIES Core Utility Functions
 */
/*@{*/

int     cds_compare_arrays(
            size_t       length,
            CDSDataType  array1_type,
            void        *array1,
            CDSDataType  array2_type,
            void        *array2,
            void        *threshold,
            size_t      *diff_index);

void    *cds_copy_array(
            CDSDataType  in_type,
            size_t       length,
            void        *in_data,
            CDSDataType  out_type,
            void        *out_data,
            size_t       nmap,
            void        *in_map,
            void        *out_map,
            void        *out_min,
            void        *orv_min,
            void        *out_max,
            void        *orv_max);

void   *cds_create_data_index(
            void        *data,
            CDSDataType  type,
            int          ndims,
            size_t      *lengths);

void    cds_free_data_index(
            void   *index,
            int     ndims,
            size_t *lengths);

void   *cds_get_missing_values_map(
            CDSDataType  in_type,
            int          nmissing,
            void        *in_missing,
            CDSDataType  out_type,
            void        *out_missing);

void   *cds_init_array(
            CDSDataType  type,
            size_t       length,
            void        *fill_value,
            void        *array);

void   *cds_memdup(size_t nbytes, void *memp);

time_t *cds_offsets_to_times(
            CDSDataType  type,
            size_t       ntimes,
            time_t       base_time,
            void        *offsets,
            time_t      *times);

timeval_t *cds_offsets_to_timevals(
            CDSDataType  type,
            size_t       ntimes,
            time_t       base_time,
            void        *offsets,
            timeval_t   *timevals);

int     *cds_qc_delta_checks(
            CDSDataType  data_type,
            size_t       ndims,
            size_t      *dim_lengths,
            void        *data_vp,
            size_t       ndeltas,
            void        *deltas_vp,
            int         *delta_flags,
            void        *prev_sample_vp,
            int         *prev_qc_flags,
            int          bad_flags,
            int         *qc_flags);

int    *cds_qc_limit_checks(
            CDSDataType  data_type,
            size_t       nvalues,
            void        *data_vp,
            size_t       nmissings,
            void        *missings_vp,
            int         *missing_flags,
            void        *min_vp,
            int          min_flag,
            void        *max_vp,
            int          max_flag,
            int         *qc_flags);

int    *cds_qc_time_offset_checks(
            CDSDataType  data_type,
            size_t       noffsets,
            void        *offsets_vp,
            void        *prev_offset_vp,
            int          lteq_zero_flag,
            void        *min_delta_vp,
            int          min_delta_flag,
            void        *max_delta_vp,
            int          max_delta_flag,
            int         *qc_flags);

size_t  cds_print_array(
            FILE        *fp,
            CDSDataType  type,
            size_t       length,
            void        *array,
            const char  *indent,
            size_t       maxline,
            size_t       linepos,
            int          flags);

char *cds_sprint_array(
            CDSDataType  type,
            size_t       array_length,
            void        *array,
            size_t      *string_length,
            char        *string,
            const char  *indent,
            size_t       maxline,
            size_t       linepos,
            int          flags);

void   *cds_string_to_array(
            const char  *string,
            CDSDataType  type,
            size_t      *length,
            void        *array);

void   *cds_string_to_array_use_fill(
            const char  *string,
            CDSDataType  type,
            size_t      *length,
            void        *array);

char   *cds_array_to_string(
            CDSDataType  type,
            size_t       array_length,
            void        *array,
            size_t      *string_length,
            char        *string);

/*@}*/

/******************************************************************************/
/**
 *  @defgroup CDS_VERSION Library Version
 */
/*@{*/

const char *cds_lib_version(void);

/*@}*/

/******************************************************************************
* DEPRECATED
*/

void *cds_put_var_data(
        CDSVar      *var,
        size_t       sample_start,
        size_t       sample_count,
        CDSDataType  type,
        void        *data);

#endif /* _CDS3_H */
