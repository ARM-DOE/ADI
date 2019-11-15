/*******************************************************************************
*
*  COPYRIGHT (C) 2012 Battelle Memorial Institute.  All Rights Reserved.
*
********************************************************************************
*
*  Author:
*     name:  Brian Ermold
*     phone: (509) 375-5950
*     email: brian.ermold@pnnl.gov
*
********************************************************************************
*
*  REPOSITORY INFORMATION:
*    $Revision: 77126 $
*    $Author: ermold $
*    $Date: 2017-03-13 22:38:51 +0000 (Mon, 13 Mar 2017) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file dsproc_map_data.c
 *  Data Mapping Functions.
 */

#include "dsproc3.h"
#include "dsproc_private.h"

extern DSProc *_DSProc; /**< Internal DSProc structure */

/** @privatesection */

/*******************************************************************************
 *  Static Data and Functions Visible Only To This Module
 */

static int    _RollupTransQC = 0;
//static time_t _MapBeginTime  = 0;
//static time_t _MapEndTime    = 0;
static timeval_t _MapBeginTime  = { 0, 0 };
static timeval_t _MapEndTime    = { 0, 0 };

static int    _Disable_Dynamic_Metric_Vars = 0;
static int    _Enable_Legacy_Time_Vars     = 0;

static const char *_MetricNames[] = {
    "std",           // average
    "goodfraction",  // average
    "dist",          // subsample
    "dist_1",        // interpolate
    "dist_2",        // interpolate
    "nstat",         // caracena
    "deriv_lat",     // caracena
    "deriv_lon"      // caracena
};
static int _NumMetricNames = sizeof(_MetricNames)/sizeof(const char *);

/**
*  Data Mapping Node Types.
*/
typedef struct MapData    MapData;
typedef struct InMapData  InMapData;
typedef struct OutMapData OutMapData;

/**
*  Input Map Data Node.
*/
struct InMapData
{
    InMapData  *next;    /**< next node in list             */
    CDSGroup   *dataset; /**< pointer to the input dataset  */

    CDSVar     *time_var;
    CDSVar     *time_bounds_var;
    CDSVar     *qc_time_var;
    size_t      ntimes;
    timeval_t  *sample_times;
    timeval_t  *start_time;
    int         sample_start;
    int         sample_end;
    size_t      sample_count;
};

/**
*  Output Map Data Node.
*/
struct OutMapData
{
    OutMapData *next;    /**< next node in list             */
    CDSGroup   *dataset; /**< pointer to the input dataset  */

    CDSVar     *time_var;
    CDSVar     *time_bounds_var;
    CDSVar     *qc_time_var;
    size_t      ntimes;
    timeval_t  *sample_times;
    timeval_t  *start_time;
    int         sample_start;
};

/**
*  Map Data Node.
*/
struct MapData
{
    MapData    *next; /**< next node in list                          */
    InMapData  *in;   /**< pointer to the node for the input dataset  */
    OutMapData *out;  /**< pointer to the node for the output dataset */
};

/**
*  Linked lists of InMapData, OutMapData, and MapData structures.
*/
typedef struct MapList
{
    MapData    *map; /**< head node of the map data list */
    InMapData  *in;  /**< head node of the input map data list  */
    OutMapData *out; /**< head node of the output map data list */

} MapList;

/**
*  Free a linked list of InMapData structures.
*/
static void _dsproc_free_in_map_list(MapList *list)
{
    InMapData *in;
    InMapData *next;

    if (list) {

        for (in = list->in; in; in = next) {
            next = in->next;
            if (in->sample_times) free(in->sample_times);
            free(in);
        }

        list->in = (InMapData *)NULL;
    }
}

/**
*  Free a linked list of OutMapData structures.
*/
static void _dsproc_free_out_map_list(MapList *list)
{
    OutMapData *out;
    OutMapData *next;

    if (list) {

        for (out = list->out; out; out = next) {
            next = out->next;
            if (out->sample_times) free(out->sample_times);
            free(out);
        }

        list->out = (OutMapData *)NULL;
    }
}

/**
*  Free all memory used by a MapList.
*/
static void _dsproc_free_map_list(MapList *list)
{
    MapData *map;
    MapData *next;

    if (list) {

        _dsproc_free_in_map_list(list);
        _dsproc_free_out_map_list(list);

        for (map = list->map; map; map = next) {
            next = map->next;
            free(map);
        }

        free(list);
    }
}

/**
*  Get the InMapData structure for an input dataset.
*/
static InMapData *_dsproc_get_in_map_data(
    MapList  *list,
    CDSGroup *dataset)
{
    InMapData *in;
    timeval_t  ref_timeval;
    timeval_t  map_begin_time;
    timeval_t  map_end_time;

    /* Check if we already have an InMapData structure for this dataset */

    for (in = list->in; in; in = in->next) {
       if (in->dataset == dataset) {
           return(in);
       }
    }

    /* Add a new InMapData structure for this dataset */

    in = (InMapData *)calloc(1, sizeof(InMapData));
    if (!in) {

        ERROR( DSPROC_LIB_NAME,
            "Could not map input data to output data\n"
            " -> memory allocation error\n");

        dsproc_set_status(DSPROC_ENOMEM);
        return((InMapData *)NULL);
    }

    in->next    = list->in;
    list->in    = in;
    in->dataset = dataset;

    /* Get the sample times from the input dataset */

    in->time_var = cds_find_time_var(dataset);

    if (in->time_var) {

        in->sample_times = cds_get_sample_timevals(
            in->time_var, 0, &(in->ntimes), NULL);

        if (in->ntimes == (size_t)-1) {
            dsproc_set_status(DSPROC_EVARMAP);
            return((InMapData *)NULL);
        }

        /* Get the pointer to the time_bounds var if it exists */

        in->time_bounds_var = cds_get_bounds_var(in->time_var);

        /* Get the pointer to the qc_time var if it exists */

        in->qc_time_var = cds_get_var(
            (CDSGroup *)in->time_var->parent, "qc_time");

        /* Get start and end indexes for the current processing interval */

        in->sample_start = 0;
        in->sample_end   = in->ntimes - 1;

        if (_MapBeginTime.tv_sec) {
            map_begin_time = _MapBeginTime;
        }
        else {
            map_begin_time.tv_sec  = _DSProc->interval_begin;
            map_begin_time.tv_usec = 0;
        }

        if (_MapEndTime.tv_sec) {
            map_end_time = _MapEndTime;
        }
        else {
            map_end_time.tv_sec  = _DSProc->interval_end;
            map_end_time.tv_usec = 0;
        }

        ref_timeval = map_begin_time;

        in->sample_start = cds_find_timeval_index(
            in->ntimes, in->sample_times, ref_timeval, CDS_GTEQ);

        ref_timeval = map_end_time;

        in->sample_end = cds_find_timeval_index(
            in->ntimes, in->sample_times, ref_timeval, CDS_LT);

        if (in->sample_start < 0 ||
            in->sample_end   < 0 ||
            in->sample_start > in->sample_end) {

            in->sample_start = -1;
            in->start_time   = (timeval_t *)NULL;
            in->sample_count = 0;
        }
        else {
            in->start_time   = &in->sample_times[in->sample_start];
            in->sample_count = in->sample_end - in->sample_start + 1;
        }
    }

    return(in);
}

/**
*  Get the OutMapData structure for an output dataset.
*/
static OutMapData *_dsproc_get_out_map_data(
    MapList  *list,
    CDSGroup *dataset)
{
    OutMapData *out;

    /* Check if we already have an OutMapData structure for this dataset */

    for (out = list->out; out; out = out->next) {
       if (out->dataset == dataset) {
           return(out);
       }
    }

    /* Add a new OutMapData structure for this dataset */

    out = (OutMapData *)calloc(1, sizeof(OutMapData));
    if (!out) {

        ERROR( DSPROC_LIB_NAME,
            "Could not map input data to output data\n"
            " -> memory allocation error\n");

        dsproc_set_status(DSPROC_ENOMEM);
        return((OutMapData *)NULL);
    }

    out->next    = list->out;
    list->out    = out;
    out->dataset = dataset;

    /* Get the sample times from the output dataset */

    out->time_var = cds_find_time_var(dataset);

    if (out->time_var) {

        out->sample_times = cds_get_sample_timevals(
            out->time_var, 0, &(out->ntimes), NULL);

        if (out->ntimes == (size_t)-1) {
            dsproc_set_status(DSPROC_EVARMAP);
            return((OutMapData *)NULL);
        }

        out->time_bounds_var = cds_get_bounds_var(out->time_var);

        out->qc_time_var = cds_get_var(
            (CDSGroup *)out->time_var->parent, "qc_time");
    }

    return(out);
}

/**
*  Initialize the data mapping for an input and output dataset.
*/
static MapData *_dsproc_init_data_map(
    MapList    *list,
    InMapData  *in,
    OutMapData *out)
{
    int      dynamic_dod = dsproc_get_dynamic_dods_mode();
    MapData *map;
    size_t   nbytes;
    int      status;
    int      copy_flags;
    CDSAtt  *att;

    const char *dim_names[] = { "time" };

    if (dynamic_dod) {
        copy_flags = 0;
    }
    else {
        copy_flags = CDS_EXCLUSIVE;
    }

    /* Check if the data mapping has already been initialized for this
     * input and output dataset */

    for (map = list->map; map; map = map->next) {

       if (map->in  == in &&
           map->out == out) {

           return(map);
       }
    }

    /* Add a new MapData structure for this input/output dataset combination */

    map = (MapData *)calloc(1, sizeof(MapData));
    if (!map) {

        ERROR( DSPROC_LIB_NAME,
            "Could not map input data to output data\n"
            " -> memory allocation error\n");

        dsproc_set_status(DSPROC_ENOMEM);
        return((MapData *)NULL);
    }

    map->next = list->map;
    list->map = map;
    map->in   = in;
    map->out  = out;

    /* Copy over the global attributes that haven't already
     * been set for this datastream */

    if (!cds_copy_atts(
        in->dataset, out->dataset, NULL, NULL, copy_flags)) {

        dsproc_set_status(DSPROC_ECDSCOPY);
        return((MapData *)NULL);
    }

    /* Check if the input dataset has a time variable */

    if (!in->time_var) {
        return(map);
    }

    /* Create the time and qc_time variables in the output dataset
     * if they do not exist and dynamic_dods is set. */

    if (dynamic_dod) {

        if (!out->time_var) {

            if (_Enable_Legacy_Time_Vars) {

                if (!cds_define_dim(out->dataset, "time", 0, 1)) {
                    dsproc_set_status(DSPROC_EVARMAP);
                    return((MapData *)NULL);
                }

                if (!cds_define_var(out->dataset,
                        "base_time", CDS_DOUBLE, 0, NULL) ||
                    !cds_define_var(out->dataset,
                        "time_offset", CDS_DOUBLE, 1, dim_names)) {

                    dsproc_set_status(DSPROC_EVARMAP);
                    return((MapData *)NULL);
                }
            }

            out->time_var = dsproc_clone_var(
                in->time_var, out->dataset, NULL, CDS_NAT, NULL, 0);

            if (!out->time_var) {
                return((MapData *)NULL);
            }

            if ((att = cds_get_att(out->time_var, "units"))) {
                cds_change_att_text(att, NULL);
            }
        }

        if (in->time_bounds_var &&
            !out->time_bounds_var) {

            out->time_bounds_var = dsproc_clone_var(
                in->time_bounds_var, out->dataset, NULL, CDS_NAT, NULL, 0);

            if (!out->time_bounds_var) {
                return((MapData *)NULL);
            }

            if ((att = cds_get_att(out->time_bounds_var, "units"))) {
                cds_delete_att(att);
            }
        }

        /* Note: we do not want to automatically propagate the qc_time
         * variable to the output dataset...  */

    }

    /* Check if the input dataset has any time values */

    if (in->sample_count == 0) {
        return(map);
    }

    /* Check if the output dataset has a time variable defined */

    if (!out->time_var) {
        return(map);
    }

    /* Check if we need to set the times in the output dataset */

    if (out->ntimes == 0) {
        out->sample_start = 0;
        out->start_time   = (timeval_t *)NULL;
    }
    else {
        out->sample_start = cds_find_timeval_index(
            out->ntimes, out->sample_times, in->start_time[0], CDS_GTEQ);

        if (out->sample_start == -1) {
            /* we are appending the input dataset to the output dataset */
            out->sample_start = out->ntimes;
            out->start_time   = (timeval_t *)NULL;
        }
        else {
            out->start_time = &out->sample_times[out->sample_start];
        }
    }

    if (!out->start_time) {

        if (!cds_set_sample_timevals(
            out->dataset, out->sample_start,
            in->sample_count, in->start_time)) {

            dsproc_set_status(DSPROC_EVARMAP);
            return((MapData *)NULL);
        }

        if (out->sample_times) free(out->sample_times);

        out->sample_times = cds_get_sample_timevals(
            out->time_var, 0, &(out->ntimes), NULL);

        if (out->ntimes == (size_t)-1) {
            dsproc_set_status(DSPROC_EVARMAP);
            return((MapData *)NULL);
        }

        out->start_time = &out->sample_times[out->sample_start];

        /* Check for the time_bounds variables */

        if (out->time_bounds_var) {

            if (in->time_bounds_var) {

                /* Copy the time bounds variable data */

                status = cds_copy_var(
                    in->time_bounds_var, out->dataset, out->time_bounds_var->name,
                    NULL, NULL, NULL, NULL,
                    in->sample_start, out->sample_start, in->sample_count,
                    copy_flags, NULL);

                if (status < 0) {
                    dsproc_set_status(DSPROC_EVARMAP);
                    return((MapData *)NULL);
                }
            }
        }

        /* Check for the qc_time variables */

        if (out->qc_time_var) {

            if (in->qc_time_var) {

                /* Copy the QC time variable data */

                status = cds_copy_var(
                    in->qc_time_var, out->dataset, out->qc_time_var->name,
                    NULL, NULL, NULL, NULL,
                    in->sample_start, out->sample_start, in->sample_count,
                    copy_flags, NULL);

                if (status < 0) {
                    dsproc_set_status(DSPROC_EVARMAP);
                    return((MapData *)NULL);
                }
            }
            else {

                /* Initialize the QC time variable data to zero */

                if (!dsproc_init_var_data(
                    out->qc_time_var, out->sample_start, in->sample_count, 0)) {

                    dsproc_set_status(DSPROC_EVARMAP);
                    return((MapData *)NULL);
                }
            }
        }

        return(map);
    }

    /* If we get here we need to compare the time values in the
     * input dataset with the time values in the output dataset. */

    nbytes = in->sample_count * sizeof(timeval_t);

    if ( ( in->sample_count > (out->ntimes - out->sample_start) ) ||
        memcmp(in->start_time, out->start_time, nbytes) != 0) {

        ERROR( DSPROC_LIB_NAME,
            "Could not map variables from input dataset to output dataset\n"
            " -> input dataset:  %s\n"
            " -> output dataset: %s\n"
            " -> conflicting time values in datasets\n",
            cds_get_object_path(in->dataset),
            cds_get_object_path(out->dataset));

        dsproc_set_status(DSPROC_EVARMAP);
        return((MapData *)NULL);
    }

    return(map);
}

/**
 *  Static: Map data from input datasets to output datasets
 *
 *  This function will map all input data to all output datasets if an output
 *  dataset is not specified. By default only the data within the current
 *  processing interval will be mapped to the output dataset. This can be
 *  changed using the dsproc_set_map_time_range() function.
 *
 *  Only one control flag has been implemented for this function so far:
 *
 *    - MAP_ROLLUP_TRANS_QC: This flag specifies that all bad and
 *      indeterminate bits in the transformation QC variables should be
 *      consolidated into a single bad or indeterminate bit in the output QC
 *      variables. The bit values used in the output QC variables will be
 *      determined by looking at the bit description attributes.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  in_parent    - pointer to the parent group of all input datasets,
 *                         this is typically the ret_data or trans_data parents.
 *  @param  out_dataset  - pointer to the output dataset,
 *                         or NULL to map all input data to all output datasets
 *  @param  maplist      - linked lists of MapData structures
 *  @param  flags        - reserved for control flags
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
static int _dsproc_map_datasets(
    CDSGroup *in_parent,
    CDSGroup *out_dataset,
    MapList  *maplist,
    int       flags)
{
    int         dynamic_dod = dsproc_get_dynamic_dods_mode();
    CDSVar     *in_var;
    VarTag     *in_var_tag;

    DataStream *out_ds;
    CDSGroup   *out_group;
    CDSVar     *out_var;

    VarTarget  *target;

    InMapData  *in;
    OutMapData *out;
    MapData    *map;

    size_t      in_sample_start;
    size_t      in_sample_count;
    size_t      out_sample_start;

    int         map_flags;
    int         gi, vi, ti;

    /* Loop over all variables in the input dataset */

    for (vi = 0; vi < in_parent->nvars; vi++) {

        /* Check for a variable tag containing output target information */

        in_var     = in_parent->vars[vi];
        in_var_tag = cds_get_user_data(in_var, "DSProcVarTag");

        if (!in_var_tag ||
            !in_var_tag->ntargets) {

            continue;
        }

        /* Get the map data for this input dataset */

        in = _dsproc_get_in_map_data(maplist, in_parent);
        if (!in) {
            return(0);
        }

        /* Loop over the output targets specified for this variable */

        for (ti = 0; ti < in_var_tag->ntargets; ti++) {

            map_flags = flags;

            /* Get the output datastream */

            target = in_var_tag->targets[ti];
            out_ds = _DSProc->datastreams[target->ds_id];

            if (!out_ds->out_cds) {
                continue;
            }

            if (out_ds->flags & DS_ROLLUP_TRANS_QC) {
                map_flags |= MAP_ROLLUP_TRANS_QC;
            }

            /* Find the output dataset
             *
             * This will need to be updated when we add obs based processing...
             */

            out_group = out_ds->out_cds;

            if (out_dataset &&
                out_dataset != out_group) {

                continue;
            }

            /* Find the output variable */

            out_var = cds_get_var(out_group, target->var_name);

            if (!out_var &&
                !dynamic_dod) {

                WARNING( DSPROC_LIB_NAME,
                    "Could not map input variable to output variable\n"
                    " -> input variable:  %s\n"
                    " -> output variable: %s/_vars_/%s\n"
                    " -> output variable does not exist in DOD\n",
                    cds_get_object_path(in_var),
                    cds_get_object_path(out_group),
                    target->var_name);

                continue;
            }

            /* Get the map data for this output dataset */

            out = _dsproc_get_out_map_data(maplist, out_group);
            if (!out) {
                return(0);
            }

            /* Get the map data for this input and output dataset */

            map = _dsproc_init_data_map(maplist, in, out);
            if (!map) {
                return(0);
            }

            /* Create the output variable if necessary. */

            if (!out_var) {

                out_var = dsproc_clone_var(
                    in_var, out_group, target->var_name, CDS_NAT, NULL, 0);

                if (!out_var) {
                    return(0);
                }
            }

            /* Map the input variable to the output variable */

            if (in_var->ndims > 0 &&
                strcmp(in_var->dims[0]->name, "time") == 0) {

                in_sample_start  = (size_t)in->sample_start;
                in_sample_count  = in->sample_count;
                out_sample_start = (size_t)out->sample_start;
            }
            else {
                in_sample_start  = 0;
                in_sample_count  = 0;
                out_sample_start = 0;
            }

            if (!dsproc_map_var(
                in_var,  in_sample_start,  in_sample_count,
                out_var, out_sample_start, map_flags)) {

                return(0);
            }

        } /* end loop over output targets for variable */

    } /* end loop over vars in input group */

    /* Recurse into all subgroups of the parent group */

    for (gi = 0; gi < in_parent->ngroups; gi++) {

        if (!_dsproc_map_datasets(
            in_parent->groups[gi], out_dataset, maplist, flags)) {

            return(0);
        }
    }

    return(1);
}

/*******************************************************************************
 *  Private Functions Visible Only To This Library
 */

/** @publicsection */

/*******************************************************************************
 *  Public Functions
 */

/**
 *  Set flag used to disable dynamic metric vars.
 *
 *  By default the metric variables created by the transformation will be
 *  automatically mapped to the output datasets if the dynamic DOD mode is
 *  enabled.  This flag can be used to change that behavior.
 *
 *  @param  flag  1 == disable, 0 == enable
 */
void dsproc_disable_dynamic_metric_vars(int flag)
{
    _Disable_Dynamic_Metric_Vars = flag;
}

/**
 *  Set flag used to enable creation of legacy base_time/time_offset variables.
 *
 *  By default the legecy base_time/time_offset variables will not be
 *  automatically created if the dynamic DOD mode is used.  This flag
 *  can be used to enabled this feature.
 *
 *  @param  flag  0 == disable, 1 == enable
 */
void dsproc_enable_legacy_time_vars(int flag)
{
    _Enable_Legacy_Time_Vars = flag;
}

/**
 *  Map data from input datasets to output datasets
 *
 *  This function will map all input data to all output datasets if an output
 *  dataset is not specified. By default only the data within the current
 *  processing interval will be mapped to the output dataset. This can be
 *  changed using the dsproc_set_map_time_range() function.
 *
 *  Only one control flag has been implemented for this function so far:
 *
 *    - MAP_ROLLUP_TRANS_QC: This flag specifies that all bad and
 *      indeterminate bits in the transformation QC variables should be
 *      consolidated into a single bad or indeterminate bit in the output QC
 *      variables. The bit values used in the output QC variables will be
 *      determined by looking at the bit description attributes.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  in_parent   - pointer to the parent group of all input datasets,
 *                        this is typically the ret_data or trans_data parents.
 *  @param  out_dataset - pointer to the output dataset,
 *                        or NULL to map all input data to all output datasets
 *  @param  flags       - control flags
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int dsproc_map_datasets(
    CDSGroup *in_parent,
    CDSGroup *out_dataset,
    int       flags)
{
    MapList    *maplist;
    OutMapData *out;
    int         status;

    maplist = (MapList *)calloc(1, sizeof(MapList));

    if (!maplist) {

        if (out_dataset) {

            ERROR( DSPROC_LIB_NAME,
                "Could not map data from %s to %s\n"
                " -> memory allocation error\n",
                in_parent->name, out_dataset->name);
        }
        else {

            ERROR( DSPROC_LIB_NAME,
                "Could not map data from %s to all output datasets\n"
                " -> memory allocation error\n",
                in_parent->name);
        }

        dsproc_set_status(DSPROC_ENOMEM);
        return(0);
    }

    status = _dsproc_map_datasets(in_parent, out_dataset, maplist, flags);

    /* Fix the field order in the output datasets if
     * the dynamic DODs flag is set. */

    if (dsproc_get_dynamic_dods_mode()) {
        for (out = maplist->out; out; out = out->next) {
            _dsproc_fix_field_order(out->dataset);
        }
    }

    _dsproc_free_map_list(maplist);

    return(status);
}

/**
 *  Map an input variable to an output variable.
 *
 *  This function will also map all associated coordinate, qc, and metric
 *  variables associated with the input variable to the same dataset as
 *  the output variable. Only one control flag has been implemented for this
 *  function so far:
 *
 *    - MAP_ROLLUP_TRANS_QC: This flag specifies that all bad and
 *      indeterminate bits in the transformation QC variable should be
 *      consolidated into a single bad or indeterminate bit in the output QC
 *      variable. The bit values used in the output QC variable will be
 *      determined by looking at the bit description attributes.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  in_var           - pointer to the input variable
 *  @param  in_sample_start  - index of the start sample in the input variable
 *                             or (size_t)-1 to not copy the variable data.
 *  @param  sample_count     - number of sample to copy to the output variable,
 *                             or 0 to copy all samples
 *  @param  out_var          - pointer to the output variable
 *  @param  out_sample_start - index of the start sample in the output variable
 *  @param  flags            - control flags
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int dsproc_map_var(
    CDSVar *in_var,
    size_t  in_sample_start,
    size_t  sample_count,
    CDSVar *out_var,
    size_t  out_sample_start,
    int     flags)
{
    int          dynamic_dod = dsproc_get_dynamic_dods_mode();
    CDSGroup    *in_group;
    CDSDim      *in_dim;
    CDSVar      *in_coord_var;
    CDSVar      *in_bounds_var;
    char         in_qc_var_name[NC_MAX_NAME];
    CDSVar      *in_qc_var;
    char         in_metric_var_name[NC_MAX_NAME];
    CDSVar      *in_metric_var;

    CDSGroup    *out_group;
    CDSDim      *out_dim;
    CDSVar      *out_coord_var;
    CDSVar      *out_bounds_var;
    char         out_qc_var_name[NC_MAX_NAME];
    CDSVar      *out_qc_var;
    char         out_metric_var_name[NC_MAX_NAME];
    CDSVar      *out_metric_var;

    VarTag      *var_tag;
    CDSAtt      *src_att;

    size_t       new_length;
    int          copy_flags;
    int          consolidate_trans_qc;
    unsigned int bad_mask;
    unsigned int bad_flag;
    unsigned int ind_flag;
    int          status;
    int          di;
    int          mi;

    if (dynamic_dod) {
        copy_flags = 0;
    }
    else {
        copy_flags = CDS_EXCLUSIVE;
    }

    if (in_sample_start == (size_t)-1) {
        copy_flags |= CDS_SKIP_DATA;
    }

    in_group  = (CDSGroup *)in_var->parent;
    out_group = (CDSGroup *)out_var->parent;

    /* Make sure the dimensionality of the two variables is the same */

    if (in_var->ndims != out_var->ndims) {

        ERROR( DSPROC_LIB_NAME,
            "Could not map input variable to output variable\n"
            " -> number of dimensions do not match: %d != %d\n"
            " -> input variable:  %s\n"
            " -> output variable: %s\n",
            (int)in_var->ndims, (int)out_var->ndims,
            cds_get_object_path(in_var),
            cds_get_object_path(out_var));

        dsproc_set_status(DSPROC_EVARMAP);
        return(0);
    }

    /* Map the dimensions and coordinate variables to the output dataset */

    for (di = 0; di < in_var->ndims; di++) {

        in_dim  = in_var->dims[di];
        out_dim = out_var->dims[di];

        /* Check or set the dimension lengths of the output variable */

        if (di == 0) {

            if (!sample_count && in_sample_start != (size_t)-1) {
                sample_count = in_dim->length - in_sample_start;
            }

            if (!out_dim->is_unlimited) {

                new_length = out_sample_start + sample_count;

                if (out_dim->length < new_length) {

                    if (out_dim->def_lock) {

                        ERROR( DSPROC_LIB_NAME,
                            "Could not map input variable to output variable\n"
                            " -> could not change length of output dimension to: %d\n"
                            " -> dimension length was defined in the DOD as: %d\n"
                            " -> input variable dimension:  %s\n"
                            " -> output variable dimension: %s\n",
                            (int)new_length, (int)out_dim->length,
                            cds_get_object_path(in_dim),
                            cds_get_object_path(out_dim));

                        dsproc_set_status(DSPROC_EVARMAP);
                        return(0);
                    }

                    out_dim->length = new_length;
                }
            }
        }
        else {

            if (out_dim->length == 0) {
                out_dim->length = in_dim->length;
            }
            else if (out_dim->length != in_dim->length) {

                ERROR( DSPROC_LIB_NAME,
                    "Could not map input variable to output variable\n"
                    " -> dimension lengths do not match: %d != %d\n"
                    " -> input variable dimension:  %s\n"
                    " -> output variable dimension: %s\n",
                    (int)in_dim->length, (int)out_dim->length,
                    cds_get_object_path(in_dim),
                    cds_get_object_path(out_dim));

                dsproc_set_status(DSPROC_EVARMAP);
                return(0);
            }
        }

        /* Copy across coordinate variable data that has not been copied yet */

        if (strcmp(in_var->name,  in_dim->name)  == 0 &&
            strcmp(out_var->name, out_dim->name) == 0) {

            continue;
        }

        in_coord_var = cds_get_coord_var(in_var, di);

        if (!in_coord_var) {
            continue;
        }

        out_coord_var = cds_get_coord_var(out_var, di);

        if (dynamic_dod && !out_coord_var) {

            out_coord_var = dsproc_clone_var(
                in_coord_var, out_group, out_dim->name, CDS_NAT, NULL, 0);

            if (!out_coord_var) {
                return(0);
            }
        }

        if (!out_coord_var) {
            continue;
        }

        if (di == 0) {

            new_length = out_sample_start + sample_count;

            if (out_coord_var->sample_count < new_length) {

                if (!dsproc_map_var(
                    in_coord_var, in_sample_start, sample_count,
                    out_coord_var, out_sample_start, flags)) {

                    return(0);
                }
            }
        }
        else {

            if (out_coord_var->sample_count == 0 &&
                in_dim->length > 0) {

                if (!dsproc_map_var(
                    in_coord_var, 0, in_dim->length,
                    out_coord_var, 0, flags)) {

                    return(0);
                }
            }
        }
    } /* end loop over dimensions */

    DEBUG_LV1( DSPROC_LIB_NAME,
        "Mapping input variable to output variable\n"
        " - input:  %s->%s\n"
        " - output: %s->%s\n",
        in_group->name, in_var->name,
        out_group->name, out_var->name);

    /* Copy over the variable data and attributes */

    status = cds_copy_var(
        in_var, out_group, out_var->name,
        NULL, NULL, NULL, NULL,
        in_sample_start, out_sample_start, sample_count,
        copy_flags, NULL);

    if (status < 0) {
        dsproc_set_status(DSPROC_EVARMAP);
        return(0);
    }

    /* Copy over the variable tag */

    status = dsproc_copy_var_tag(in_var, out_var);
    if (status == 0) {
        return(0);
    }

    /* Set the source attribute */

    var_tag = cds_get_user_data(in_var, "DSProcVarTag");

    if (var_tag        &&
        var_tag->in_ds &&
        var_tag->in_var_name) {

        src_att = cds_get_att(out_var, "source");

        if (src_att) {

            if (src_att->type == CDS_CHAR &&
                !src_att->def_lock) {

                DEBUG_LV2( DSPROC_LIB_NAME,
                    " - source: '%s:%s'\n",
                    var_tag->in_ds->name, var_tag->in_var_name);

                if (!cds_change_att_text(src_att, "%s:%s",
                    var_tag->in_ds->name, var_tag->in_var_name)) {

                    dsproc_set_status(DSPROC_EVARMAP);
                    return(0);
                }
            }
        }
        else if (dynamic_dod) {

            DEBUG_LV2( DSPROC_LIB_NAME,
                " - source: '%s:%s'\n",
                var_tag->in_ds->name, var_tag->in_var_name);

            if (!cds_define_att_text(out_var, "source", "%s:%s",
                var_tag->in_ds->name, var_tag->in_var_name)) {

                dsproc_set_status(DSPROC_EVARMAP);
                return(0);
            }
        }
    }

    /* Copy over the bounds variable data and attributes */

    in_bounds_var  = cds_get_bounds_var(in_var);
    out_bounds_var = cds_get_bounds_var(out_var);

    if (dynamic_dod && in_bounds_var && !out_bounds_var) {

        out_bounds_var = dsproc_clone_var(
            in_bounds_var, out_group, in_bounds_var->name, CDS_NAT, NULL, 0);

        if (!out_bounds_var) {
            return(0);
        }
    }

    if (out_bounds_var) {

        if (in_bounds_var) {

            DEBUG_LV2( DSPROC_LIB_NAME,
                " - copying bounds variable data:\n"
                "     - input:  %s->%s\n"
                "     - output: %s->%s\n",
                in_group->name, in_bounds_var->name,
                out_group->name, out_bounds_var->name);

            status = cds_copy_var(
                in_bounds_var, out_group, out_bounds_var->name,
                NULL, NULL, NULL, NULL,
                in_sample_start, out_sample_start, sample_count,
                copy_flags, NULL);

            if (status < 0) {
                dsproc_set_status(DSPROC_EVARMAP);
                return(0);
            }
        }
    }

    /* Copy over the QC variable data and attributes */

    snprintf(in_qc_var_name, NC_MAX_NAME, "qc_%s", in_var->name);
    in_qc_var = cds_get_var(in_group, in_qc_var_name);

    snprintf(out_qc_var_name, NC_MAX_NAME, "qc_%s", out_var->name);
    out_qc_var = cds_get_var(out_group, out_qc_var_name);

    consolidate_trans_qc = 0;

    if (in_qc_var) {

        /* Check if we are consolidating the transformation QC bits */

        if ( _RollupTransQC                ||
            (flags && MAP_ROLLUP_TRANS_QC) ||
            (var_tag && (var_tag->flags & VAR_ROLLUP_TRANS_QC))) {

            if (dsproc_is_transform_qc_var(in_qc_var)) {

                consolidate_trans_qc = 1;

                if (out_qc_var) {

                    if (!dsproc_get_trans_qc_rollup_bits(
                        out_qc_var, &bad_flag, &ind_flag)) {

                        consolidate_trans_qc = 0;
                    }
                }
            }
        }

        if (dynamic_dod && !out_qc_var &&
            strcmp(out_qc_var_name, "qc_time") != 0) {

            if (consolidate_trans_qc) {

                out_qc_var = _dsproc_create_consolidated_trans_qc_var(
                    in_qc_var, out_group, out_qc_var_name);

                dsproc_get_trans_qc_rollup_bits(
                    out_qc_var, &bad_flag, &ind_flag);
            }
            else {

                /* Clone the input qc variable in the output dataset */

                out_qc_var = dsproc_clone_var(
                    in_qc_var, out_group, out_qc_var_name, CDS_NAT, NULL, 0);
            }

            if (!out_qc_var) {
                return(0);
            }
        }
    }

    if (out_qc_var) {

        if (in_qc_var) {

            if (consolidate_trans_qc) {

                DEBUG_LV2( DSPROC_LIB_NAME,
                    " - consolidating transformation QC variable data:\n"
                    "     - input:  %s->%s\n"
                    "     - output: %s->%s\n",
                    in_group->name, in_qc_var->name,
                    out_group->name, out_qc_var->name);

                bad_mask = dsproc_get_bad_qc_mask(in_qc_var);

                status = _dsproc_consolidate_var_qc(
                    in_qc_var, in_sample_start, sample_count, bad_mask,
                    out_qc_var, out_sample_start, bad_flag, ind_flag, 1);
            }
            else {

                DEBUG_LV2( DSPROC_LIB_NAME,
                    " - copying QC variable data:\n"
                    "     - input:  %s->%s\n"
                    "     - output: %s->%s\n",
                    in_group->name, in_qc_var->name,
                    out_group->name, out_qc_var->name);

                status = cds_copy_var(
                    in_qc_var, out_group, out_qc_var->name,
                    NULL, NULL, NULL, NULL,
                    in_sample_start, out_sample_start, sample_count,
                    copy_flags, NULL);
            }

            if (status < 0) {
                dsproc_set_status(DSPROC_EVARMAP);
                return(0);
            }
        }
        else if (sample_count > 0) {

            if (!dsproc_init_var_data(
                out_qc_var, out_sample_start, sample_count, 0)) {

                dsproc_set_status(DSPROC_EVARMAP);
                return(0);
            }
        }
    }

    /* Copy over the Metrics variable data and attributes */

    for (mi = 0; mi < _NumMetricNames; mi++) {

        snprintf(in_metric_var_name,
            NC_MAX_NAME, "%s_%s", in_var->name, _MetricNames[mi]);

        in_metric_var = cds_get_var(in_group, in_metric_var_name);

        snprintf(out_metric_var_name,
            NC_MAX_NAME, "%s_%s", out_var->name, _MetricNames[mi]);

        out_metric_var = cds_get_var(out_group, out_metric_var_name);

        if (dynamic_dod && !_Disable_Dynamic_Metric_Vars &&
            in_metric_var && !out_metric_var) {

            out_metric_var = dsproc_clone_var(
                in_metric_var, out_group, out_metric_var_name, CDS_NAT, NULL, 0);

            if (!out_metric_var) {
                return(0);
            }
        }

        if (out_metric_var) {

            if (in_metric_var) {

                DEBUG_LV2( DSPROC_LIB_NAME,
                    " - copying '%s' metric variable data:\n"
                    "     - input:  %s->%s\n"
                    "     - output: %s->%s\n",
                    _MetricNames[mi],
                    in_group->name, in_metric_var->name,
                    out_group->name, out_metric_var->name);

                status = cds_copy_var(
                    in_metric_var, out_group, out_metric_var->name,
                    NULL, NULL, NULL, NULL,
                    in_sample_start, out_sample_start, sample_count,
                    copy_flags, NULL);

                if (status < 0) {
                    dsproc_set_status(DSPROC_EVARMAP);
                    return(0);
                }
            }
            else if (sample_count > 0) {

                if (!dsproc_init_var_data(
                    out_metric_var, out_sample_start, sample_count, 0)) {

                    dsproc_set_status(DSPROC_EVARMAP);
                    return(0);
                }
            }
        }
    } /* end loop over metric variables */

    return(1);
}

/**
 *  Set the time range to use in subsequent calls to dsproc_map_datasets().
 *
 *  @param  begin_time  - Only map data whose time is greater than
 *                        or equal to this begin_time.
 *  @param  end_time    - Only map data whose time is less than this end_time.
 *
 */
void dsproc_set_map_time_range(time_t begin_time, time_t end_time)
{
    char ts1[32], ts2[32];

    DEBUG_LV1( DSPROC_LIB_NAME,
       "Setting data mapping time range:\n"
       " - begin: %s\n"
       " - end:   %s\n",
       format_secs1970(begin_time, ts1),
       format_secs1970(end_time, ts2));

    _MapBeginTime.tv_sec  = begin_time;
    _MapBeginTime.tv_usec = 0;
    _MapEndTime.tv_sec    = end_time;
    _MapEndTime.tv_usec   = 0;
}

/**
 *  Set the time range to use in subsequent calls to dsproc_map_datasets().
 *
 *  @param  begin_time  - Only map data whose time is greater than
 *                        or equal to this begin_time.
 *  @param  end_time    - Only map data whose time is less than this end_time.
 *
 */
void dsproc_set_map_timeval_range(timeval_t *begin_time, timeval_t *end_time)
{
    char ts1[32], ts2[32];

    DEBUG_LV1( DSPROC_LIB_NAME,
       "Setting data mapping time range:\n"
       " - begin: %s\n"
       " - end:   %s\n",
       format_timeval(begin_time, ts1),
       format_timeval(end_time, ts2));

    _MapBeginTime = *begin_time;
    _MapEndTime   = *end_time;
}

/**
 *  Set the global transformation QC rollup flag.
 *
 *  This function should typically be called from the users init_process
 *  function, but must be called before the post-transform hook returns.
 *
 *  Setting this flag to true specifies that all bad and indeterminate bits
 *  in transformation QC variables should be consolidated into a single bad
 *  or indeterminate bit when they are mapped to the output datasets. This
 *  bit consolidation will only be done if the input and output QC variables
 *  have the appropriate bit descriptions:
 *
 *    - The input transformation QC variables will be determined by checking
 *      the tag names in the bit description attributes. These must be in
 *      same order as the transformation would define them.
 *
 *    - The output QC variables must contain two bit descriptions for the bad
 *      and indeterminate bits to use, and these bit descriptions must begin
 *      with the following text:
 *
 *        - "Transformation could not finish"
 *        - "Transformation resulted in an indeterminate outcome"
 *
 *  An alternative to calling this function is to set the "ROLLUP TRANS QC"
 *  flags for the output datastreams and/or retrieved variables. See
 *  dsproc_set_datastream_flags() and dsproc_set_var_flags(). These options
 *  should not typically be needed, however, because the internal mapping
 *  logic will determine when it is appropriate to do the bit consolidation.
 *
 *  @param  flag - transformation QC rollup flag (1 = TRUE, 0 = FALSE)
 */
void dsproc_set_trans_qc_rollup_flag(int flag)
{
    DEBUG_LV1( DSPROC_LIB_NAME,
       "Setting transformation QC rollup flag to: %d\n",
        flag);

    _RollupTransQC = flag;
}
