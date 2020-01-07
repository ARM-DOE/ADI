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
*    $Revision: 54207 $
*    $Author: ermold $
*    $Date: 2014-05-02 20:50:34 +0000 (Fri, 02 May 2014) $
*    $Version: dsdb-libdsdb3-3.0-devel $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file dsdb3.h
 *  Header file for libdsdb3
 */

#ifndef _DSDB3_H_
#define _DSDB3_H_ 1

#include <stdlib.h>
#include <time.h>

#include "dbconn.h"
#include "cds3.h"

/** DSDB library name. */
#define DSDB_LIB_NAME "libdsdb3"

const char *dsdb_lib_version(void);

/**
 *  @addtogroup DSDB_UTILITIES
 */
/*@{*/

/* Convenience Macro to duplicate a string or set the dest var to NULL */
#define DSDB_STRDUP(src,dest,s,err) \
    if (src) { \
        dest = strdup(src); \
        if (!dest) { \
            ERROR( DSDB_LIB_NAME, \
                "Could not duplicate string\n -> memory allocation error\n"); \
            free(s); \
            return(err); \
        } \
    } \
    else { \
        dest = (char *)NULL; \
    }

/*@}*/

/*******************************************************************************
*  DSDB Connection and Utility Functions
*/

/**
 *  @addtogroup DSDB_CONNECTION
 */
/*@{*/

/**
 *  DSDB Connection
 */
typedef struct
{
    DBConn *dbconn;         /**< pointer to the database connection */
    int     max_retries;    /**< number of times to retry a db connection  */
    int     retry_interval; /**< sleep interval between db connect retries */
    int     nreconnect;     /**< database disconnect/reconnect counter     */

} DSDB;

/*@}*/

/***** DSDB Connection Functions *****/

DSDB   *dsdb_create(const char *db_alias);
void    dsdb_destroy(DSDB *dsdb);

int     dsdb_connect(DSDB *dsdb);
void    dsdb_disconnect(DSDB *dsdb);
int     dsdb_is_connected(DSDB *dsdb);

void    dsdb_set_max_retries(DSDB *dsdb, int max_retries);
void    dsdb_set_retry_interval(DSDB *dsdb, int retry_interval);

/***** Utility Functions *****/

char   *dsdb_bool_to_text(
            DSDB *dsdb,
            int   bval,
            char *text);

int    *dsdb_text_to_bool(
            DSDB       *dsdb,
            const char *text,
            int        *bval);

char   *dsdb_time_to_text(
            DSDB   *dsdb,
            time_t  time,
            char   *text);

time_t *dsdb_text_to_time(
            DSDB       *dsdb,
            const char *text,
            time_t     *time);

char   *dsdb_timeval_to_text(
            DSDB      *dsdb,
            timeval_t *tval,
            char      *text);

timeval_t *dsdb_text_to_timeval(
            DSDB       *dsdb,
            const char *text,
            timeval_t  *tval);

/*******************************************************************************
*  Facility Functions
*/

/**
 *  @addtogroup DSDB_FACLOC
 */
/*@{*/

/**
 *  Facility Location.
 */
typedef struct
{
    char  *name; /**< location name  */
    float  lat;  /**< north latitude */
    float  lon;  /**< east longitude */
    float  alt;  /**< altitude MSL   */
} FacLoc;

/*@}*/

void    dsdb_free_facility_location(FacLoc *fac_loc);

int     dsdb_get_facility_location(
            DSDB        *dsdb,
            const char  *site,
            const char  *facility,
            FacLoc     **fac_loc);

int     dsdb_get_site_description(
            DSDB        *dsdb,
            const char  *site,
            char       **desc);

/*******************************************************************************
*  Process Functions
*/

/**
 *  @addtogroup DSDB_PROCCONF
 */
/*@{*/

/**
 *  Process Config.
 */
typedef struct
{
    char *site;     /**< site name     */
    char *facility; /**< facility name */
    char *type;     /**< process type  */
    char *name;     /**< process name  */
    char *key;      /**< config key    */
    char *value;    /**< config value  */
} ProcConf;

/*@}*/

void    dsdb_free_process_config_values(ProcConf **conf_vals);

int     dsdb_get_process_config_values(
            DSDB         *dsdb,
            const char   *site,
            const char   *facility,
            const char   *proc_type,
            const char   *proc_name,
            const char   *key,
            ProcConf   ***proc_conf);

/**
 *  @addtogroup DSDB_FAMPROCS
 */
/*@{*/

/**
 *  Family Process.
 */
typedef struct
{
    char *category; /**< process category */
    char *proc_class; /**< process class    */
    char *type;     /**< process type     */
    char *name;     /**< process name     */
    char *site;     /**< site name        */
    char *facility; /**< facility name    */
} FamProc;

/*@}*/

void    dsdb_free_family_process(FamProc *fam_proc);
void    dsdb_free_family_processes(FamProc **fam_procs);

int     dsdb_get_family_process(
            DSDB        *dsdb,
            const char  *site,
            const char  *facility,
            const char  *proc_type,
            const char  *proc_name,
            FamProc    **fam_proc);

int     dsdb_inquire_family_processes(
            DSDB         *dsdb,
            const char   *category,
            const char   *proc_class,
            const char   *site,
            const char   *facility,
            const char   *proc_type,
            const char   *proc_name,
            FamProc    ***fam_procs);

/**
 *  @addtogroup DSDB_PROCLOCS
 */
/*@{*/

/**
 *  Process Location.
 */
typedef struct
{
    char  *name; /**< location name  */
    float  lat;  /**< north latitude */
    float  lon;  /**< east longitude */
    float  alt;  /**< altitude MSL   */
} ProcLoc;

/*@}*/

void    dsdb_free_process_location(ProcLoc *proc_loc);

int     dsdb_get_process_location(
            DSDB        *dsdb,
            const char  *site,
            const char  *facility,
            const char  *proc_type,
            const char  *proc_name,
            ProcLoc    **proc_loc);

/**
 *  @addtogroup DSDB_PROCSTATES
 */
/*@{*/

/**
 *  Process State.
 */
typedef struct
{
    char      *name;         /**< state name         */
    char      *text;         /**< state text         */
    int        is_enabled;   /**< is enabled flag    */
    time_t     last_updated; /**< last updated time  */
} ProcState;

/*@}*/

void    dsdb_free_process_state(ProcState *proc_state);

int     dsdb_get_process_state(
            DSDB        *dsdb,
            const char  *site,
            const char  *facility,
            const char  *proc_type,
            const char  *proc_name,
            ProcState  **proc_state);

int     dsdb_delete_process_state(
            DSDB       *dsdb,
            const char *site,
            const char *facility,
            const char *proc_type,
            const char *proc_name);

int     dsdb_is_process_enabled(
            DSDB       *dsdb,
            const char *site,
            const char *facility,
            const char *proc_type,
            const char *proc_name);

int     dsdb_update_process_state(
            DSDB       *dsdb,
            const char *site,
            const char *facility,
            const char *proc_type,
            const char *proc_name,
            const char *state,
            const char *desc,
            time_t      state_time);

/**
 *  @addtogroup DSDB_PROCSTATUS
 */
/*@{*/

/**
 *  Process Status.
 */
typedef struct
{
    char   *name;            /**< status name           */
    char   *text;            /**< status text           */
    int     is_successful;   /**< is successful flag    */
    time_t  last_started;    /**< last time started     */
    time_t  last_completed;  /**< last time completed   */
    time_t  last_successful; /**< last time successful  */
} ProcStatus;

/*@}*/

void    dsdb_free_process_status(ProcStatus *proc_status);

int     dsdb_get_process_status(
            DSDB        *dsdb,
            const char  *site,
            const char  *facility,
            const char  *proc_type,
            const char  *proc_name,
            ProcStatus **proc_status);

int     dsdb_delete_process_status(
            DSDB       *dsdb,
            const char *site,
            const char *facility,
            const char *proc_type,
            const char *proc_name);

int     dsdb_update_process_started(
            DSDB       *dsdb,
            const char *site,
            const char *facility,
            const char *proc_type,
            const char *proc_name,
            time_t      started_time);

int     dsdb_update_process_completed(
            DSDB       *dsdb,
            const char *site,
            const char *facility,
            const char *proc_type,
            const char *proc_name,
            time_t      completed_time);

int     dsdb_update_process_status(
            DSDB       *dsdb,
            const char *site,
            const char *facility,
            const char *proc_type,
            const char *proc_name,
            const char *status,
            const char *desc,
            time_t      completed_time);

/**
 *  @addtogroup DSDB_DSCLASS
 */
/*@{*/

/**
 *  Process Input/Output Datastream Classes.
 */
typedef struct
{
    char *name;   /**< datastream class name  */
    char *level;  /**< datastream class level */
} DSClass;

/*@}*/

void    dsdb_free_ds_classes(DSClass **ds_classes);

int     dsdb_get_process_dsc_inputs(
            DSDB         *dsdb,
            const char   *proc_type,
            const char   *proc_name,
            DSClass    ***ds_classes);

int     dsdb_get_process_dsc_outputs(
            DSDB         *dsdb,
            const char   *proc_type,
            const char   *proc_name,
            DSClass    ***ds_classes);

/**
 *  @addtogroup DSDB_DSTIMES
 */
/*@{*/

/**
 *  Process Output Datastream Times.
 */
typedef struct
{
    timeval_t first; /**< first data time  */
    timeval_t last;  /**< last data time   */

} DSTimes;

/*@}*/

void    dsdb_free_ds_times(DSTimes *ds_times);

int     dsdb_get_process_output_ds_times(
            DSDB        *dsdb,
            const char  *site,
            const char  *facility,
            const char  *proc_type,
            const char  *proc_name,
            const char  *dsc_name,
            const char  *dsc_level,
            DSTimes    **ds_times);

int     dsdb_delete_process_output_ds_times(
            DSDB       *dsdb,
            const char *site,
            const char *facility,
            const char *proc_type,
            const char *proc_name,
            const char *dsc_name,
            const char *dsc_level);

int     dsdb_update_process_output_ds_times(
            DSDB       *dsdb,
            const char *site,
            const char *facility,
            const char *proc_type,
            const char *proc_name,
            const char *dsc_name,
            const char *dsc_level,
            const timeval_t *first_time,
            const timeval_t *last_time);

/**
 *  @addtogroup DSDB_DSCONF
 */
/*@{*/

/**
 *  Datastream Config.
 */
typedef struct
{
    char *site;     /**< site name               */
    char *facility; /**< facility name           */
    char *name;     /**< datastream class name   */
    char *level;    /**< datastream class level  */
    char *key;      /**< config key              */
    char *value;    /**< config value            */
} DSConf;

/*@}*/

void    dsdb_free_datastream_config_values(DSConf **dsconf_vals);

int     dsdb_get_datastream_config_values(
            DSDB         *dsdb,
            const char   *site,
            const char   *facility,
            const char   *ds_name,
            const char   *ds_level,
            const char   *key,
            DSConf     ***ds_conf);

/**
 *  @addtogroup DSDB_DSPROPS
 */
/*@{*/

/**
 *  Datastream Properties.
 */
typedef struct
{
    char   *dsc_name;  /**< datastream class name   */
    char   *dsc_level; /**< datastream class level  */
    char   *site;      /**< site name               */
    char   *facility;  /**< facility name           */
    char   *var_name;  /**< variable name           */
    char   *name;      /**< property name           */
    time_t  time;      /**< property time           */
    char   *value;     /**< property value          */

} DSProp;

/*@}*/

void    dsdb_free_ds_properties(DSProp **dsprops);

int     dsdb_get_ds_properties(
            DSDB         *dsdb,
            const char   *dsc_name,
            const char   *dsc_level,
            const char   *site,
            const char   *facility,
            const char   *var_name,
            const char   *prop_name,
            DSProp     ***dsprops);

/*******************************************************************************
*  DOD Functions
*/

/**
 *  @addtogroup DSDB_DSDODS
 */
/*@{*/

/**
 *  Datastream DOD.
 */
typedef struct {

    CDSGroup *cds_group;    /**< CDS Group containing the current DSDOD       */

    char     *site;         /**< site name                                    */
    char     *facility;     /**< facility name                                */
    char     *name;         /**< datastream class name                        */
    char     *level;        /**< datastream class level                       */

    time_t    data_time;    /**< data time used to create the current DSDOD   */
    char     *version;      /**< version of the current DOD                   */

    int       ndod_times;   /**< number of times when the dod version changes */
    time_t   *dod_times;    /**< list of times when the dod version changes   */
    char    **dod_versions; /**< list of dod versions                         */

    int       natt_times;   /**< number of times when attribute values change */
    time_t   *att_times;    /**< list of times when attribute values change   */

} DSDOD;

/*@}*/

void    dsdb_free_dsdod(DSDOD *dsdod);

int     dsdb_get_dsdod(
            DSDB        *dsdb,
            const char  *site,
            const char  *facility,
            const char  *dsc_name,
            const char  *dsc_level,
            time_t       data_time,
            DSDOD      **dsdod);

int     dsdb_update_dsdod(
            DSDB   *dsdb,
            DSDOD  *dsdod,
            time_t  data_time);

const char *dsdb_check_for_dsdod_version_update(
            DSDOD *dsdod,
            time_t data_time);

int     dsdb_check_for_dsdod_time_atts_update(
            DSDOD *dsdod,
            time_t data_time);

DSDOD  *dsdb_new_dsdod(
            const char *site,
            const char *facility,
            const char *dsc_name,
            const char *dsc_level);

int     dsdb_get_dod(
            DSDB        *dsdb,
            const char  *dsc_name,
            const char  *dsc_level,
            const char  *dod_version,
            CDSGroup   **cds_group);

int     dsdb_get_dsdod_versions(
            DSDB  *dsdb,
            DSDOD *dsdod);

int     dsdb_get_dsdod_att_times(
            DSDB  *dsdb,
            DSDOD *dsdod);

int     dsdb_get_dsdod_att_values(
            DSDB  *dsdb,
            DSDOD *dsdod);

int     dsdb_get_dsdod_time_att_values(
            DSDB  *dsdb,
            DSDOD *dsdod);

int     dsdb_get_highest_dod_version(
            DSDB        *dsdb,
            const char  *dsc_name,
            const char  *dsc_level,
            char       **dod_version);

/*******************************************************************************
*  DQR Database Functions
*/

/**
 *  @addtogroup DQRDB DQR Database
 */
/*@{*/

/**
 *  DQR Database Connection.
 */
typedef struct
{
    DBConn *dbconn;         /**< pointer to the database connection        */
    int     max_retries;    /**< number of times to retry a db connection  */
    int     retry_interval; /**< sleep interval between db connect retries */
    int     nreconnect;     /**< database disconnect/reconnect counter     */

} DQRDB;

/**
 *  DQR Query Result.
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
typedef struct DQR
{
    char  *id;        /**< DQR ID                           */
    char  *desc;      /**< description                      */
    char  *ds_name;   /**< datastream name                  */
    char  *var_name;  /**< variable name                    */
    int    code;      /**< code number                      */
    char  *color;     /**< code color                       */
    char  *code_desc; /**< code description                 */
    time_t start;     /**< start time in seconds since 1970 */
    time_t end;       /**< end time in seconds since 1970   */

} DQR;

/*@}*/

DQRDB  *dqrdb_create(const char *db_alias);
void    dqrdb_destroy(DQRDB *dqrdb);

int     dqrdb_connect(DQRDB *dqrdb);
void    dqrdb_disconnect(DQRDB *dqrdb);
int     dqrdb_is_connected(DQRDB *dqrdb);

void    dqrdb_set_max_retries(DQRDB *dqrdb, int max_retries);
void    dqrdb_set_retry_interval(DQRDB *dqrdb, int retry_interval);

void    dqrdb_free_dqrs(DQR **dqrs);
int     dqrdb_get_dqrs(
            DQRDB      *dqrdb,
            const char *site,
            const char *facility,
            const char *dsc_name,
            const char *dsc_level,
            const char *var_name,
            time_t      start,
            time_t      end,
            DQR      ***dqrs);

void    dqrdb_print_dqrs(FILE *fp, DQRDB *dqrdb, DQR **dqrs);

/*******************************************************************************
*  Retriever Functions
*/

/**
 *  @addtogroup DSDB_RETRIEVER
 */
/*@{*/

/**
 *  Retriever Datastream.
 */
typedef struct {

    const char     *name;           /**< Datastream class name.              */
    const char     *level;          /**< Datastream class level.             */
    const char     *site;           /**< Site name
                                         or NULL to use process site.        */
    const char     *facility;       /**< Facility name
                                         or NULL to use process facility.    */
    const char     *dep_site;       /**< Dependeny of datastream by site
                                         being processed.                    */
    const char     *dep_fac;        /**< Dependeny of datastream by facility
                                         being processed.                    */
    time_t          dep_begin_date; /**< Dependency of datastream by begin
                                         date in seconds since 1970          */
    time_t          dep_end_date;   /**< Dependency of datastream by end
                                         date in seconds since 1970          */

    int             _id;            /**< Internal database row id.           */

} RetDataStream;

/**
 *  Retriever Coordinate System Dimension Variable Names Map.
 *
 *  The variable names are listed by variable priority as specified
 *  in the database.
 */
typedef struct {

    RetDataStream *ds;     /**< Pointer to the input datastream. */

    int            nnames; /**< Number of variable names.        */
    const char   **names;  /**< List of variable names in the
                                order of variable priority.      */
} RetDsVarMap;

/**
 *  Retriever Coordinate System Dimension.
 *
 *  The varmaps are listed by datastream subgroup priority as specified
 *  in the database.
 */
typedef struct {

    const char   *name;        /**< Coordinate dimension name.             */
    const char   *data_type;   /**< User defined data type.                */
    const char   *units;       /**< User defined  units.                   */

    const char   *start;       /**< dimension start value                  */
    const char   *length;      /**< dimension length                       */
    const char   *interval;    /**< dimension interval                     */

    const char   *trans_type;  /**< dimension interval                     */
    const char   *trans_range; /**< dimension interval                     */
    const char   *trans_align; /**< dimension interval                     */

    int           nvarmaps;    /**< Number of variable maps.               */
    RetDsVarMap **varmaps;     /**< List of variable maps in the
                                    order of datastream subgroup priority. */

    int         _id;           /**< Internal database row id.              */

} RetCoordDim;

/**
 *  Retriever Coordinate System.
 *
 *  The dimensions are listed by dimension order as specified
 *  in the database.
 */
typedef struct {

    const char      *name;  /**< Coordinate system name.   */

    int              ndims; /**< Number of dimensions.     */
    RetCoordDim    **dims;  /**< List of dimensions.       */

    int              _id;   /**< Internal database row id. */

} RetCoordSystem;

/**
 *  Retriever Variable Output Target.
 */
typedef struct {

    const char *dsc_name;   /**< Datastream class name.   */
    const char *dsc_level;  /**< Datastream class level.  */
    const char *var_name;   /**< Variable name.           */

} RetVarOutput;

/**
 *  Retriever Variable.
 *
 *  The dimension names are listed by dimension order as specified
 *  in the database.
 */
typedef struct {

    const char     *name;          /**< User defined variable name.           */
    const char     *data_type;     /**< User defined data type.               */
    const char     *units;         /**< User defined units.                   */
    time_t          start_offset;  /**< Time in seconds to offset data
                                        collection from begin date.           */
    time_t          end_offset;    /**< Time in secondss to offset data
                                        collection from end date.             */
    const char     *min;           /**< User defined valid_min.               */
    const char     *max;           /**< User defined valid_max.               */
    const char     *delta;         /**< User defined valid_delta.             */

    int             req_to_run;    /**< Flag indicating if the variable is
                                        required to run the process.          */
    int             retrieve_qc;   /**< Flag indicating if the companion
                                        qc variable should be retrieved.      */
    int             qc_req_to_run; /**< Flag indicating if the qc variable is
                                        required to run the process.          */

    RetCoordSystem *coord_system;  /**< The coordinate system this variable
                                        should be mapped to.                  */

    int             ndim_names;    /**< The number of dimension names.        */
    const char    **dim_names;     /**< List of dimension names.              */

    int             nvarmaps;      /**< Number of variable maps.              */
    RetDsVarMap   **varmaps;       /**< List of variable maps in the order
                                        of datastream subgroup priority.      */

    int             noutputs;      /**< The number of output targets.         */
    RetVarOutput  **outputs;       /**< List of output targets.               */

    int             _id;           /**< Internal database row id.             */

} RetVariable;

/**
 *  Retriever Datastream Subgroup.
 *
 *  The datastreams are listed by subgroup priority as specified
 *  in the database.
 */
typedef struct {

    const char     *name;         /**< Datastream subgroup name.  */

    int             ndatastreams; /**< Number of datastreams.     */
    RetDataStream **datastreams;  /**< List of datastreams in the
                                       order of subgroup priority */

    int             _id;          /**< Internal database row id.  */

} RetDsSubGroup;

/**
 *  Retriever Datastream Group.
 *
 *  The subgroups are listed by subgroup order as specified
 *  in the database.
 */
typedef struct {

    const char     *name;       /**< Datastream group name.          */

    int             nsubgroups; /**< Number of datastream subgroups. */
    RetDsSubGroup **subgroups;  /**< List of datastream subgroups.   */

    int             nvars;      /**< Number of variables.            */
    RetVariable   **vars;       /**< List of variables.              */

    int             _id;        /**< Internal database row id.       */

} RetDsGroup;

/**
 *  Retriever Transformation Parameters.
 */
typedef struct {
    const char *coordsys;  /**< Coodinate system name.     */
    const char *params;    /**< transformation parameters. */
} RetTransParams;

/**
 *  Retriever
 */
typedef struct {

    const char      *proc_type;       /**< Process type.                   */
    const char      *proc_name;       /**< Process name.                   */

    int              ngroups;         /**< Number of datastream groups.    */
    RetDsGroup     **groups;          /**< List of datastream groups.      */

    int              nsubgroups;      /**< Number of datastream subgroups. */
    RetDsSubGroup  **subgroups;       /**< List of datastream subgroups.   */

    int              ndatastreams;    /**< Number of datastreams.          */
    RetDataStream  **datastreams;     /**< List of datastreams.            */

    int              ncoord_systems;  /**< Number of coordinate systems.   */
    RetCoordSystem **coord_systems;   /**< List of coordinate systems.     */

    int              ntrans_params;   /**< Number of transformation params.*/
    RetTransParams **trans_params;    /**< List of transformation params.  */

} Retriever;

void dsdb_free_retriever(Retriever *retriever);

int  dsdb_get_retriever(
        DSDB        *dsdb,
        const char  *proc_type,
        const char  *proc_name,
        Retriever  **retriever);

void dsdb_print_retriever(FILE *fp, Retriever *ret);

int  dsdb_set_retriever_location(
        Retriever  *ret,
        const char *site,
        const char *facility);

/*@}*/

#endif /* _DSDB3_H_ */
