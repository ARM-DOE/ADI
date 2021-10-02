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

/** @file dsproc_var_tag.c
 *  Variable Tag Functions.
 */

#include "dsproc3.h"
#include "dsproc_private.h"

extern DSProc *_DSProc; /**< Internal DSProc structure */

/** @privatesection */

/*******************************************************************************
 *  Static Functions Visible Only To This Module
 */

/**
 *  Static: Create a VarTarget structure.
 *
 *  @param ds_id    - output datastream ID
 *  @param var_name - name of the variable in the output datastream
 *
 *  @return
 *    - new VarTarget structure
 *    - NULL if a memory allocation error occurred
 */
static VarTarget *_dsproc_create_var_target(
    int         ds_id,
    const char *var_name)
{
    VarTarget *target;

    target = (VarTarget *)calloc(1, sizeof(VarTarget));
    if (!target) {
        return((VarTarget *)NULL);
    }

    target->ds_id    = ds_id;
    target->var_name = strdup(var_name);

    if (!target->var_name) {
        free(target);
        return((VarTarget *)NULL);
    }

    return(target);
}


/**
 *  Static: Free all memory used by a variable tag.
 *
 *  @param var_tag - void pointer to the variable tag
 */
static void _dsproc_free_var_tag(void *var_tag)
{
    VarTag    *tag = (VarTag *)var_tag;
    VarTarget *target;
    int        ti;

    if (tag) {

        if (tag->in_var_name)    free((void *)tag->in_var_name);
        if (tag->valid_min)      free((void *)tag->valid_min);
        if (tag->valid_max)      free((void *)tag->valid_max);
        if (tag->valid_delta)    free((void *)tag->valid_delta);
        if (tag->coordsys_name)  free((void *)tag->coordsys_name);
        if (tag->ret_group_name) free((void *)tag->ret_group_name);

        for (ti = 0; ti < tag->ntargets; ti++) {

            target = tag->targets[ti];

            if (target->var_name) free((void *)target->var_name);
            free(target);
        }

        if (tag->targets) free(tag->targets);
        if (tag->dqrs)    _dsproc_free_var_dqrs(tag->dqrs);

        free(tag);
    }
}

/**
 *  Static: Get or create a variable tag.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param var - pointer to the variable
 *
 *  @return
 *    - pointer to the variable tag
 *    - NULL if an error occurred
 */
static VarTag *_dsproc_get_or_create_var_tag(CDSVar *var)
{
    VarTag *tag;

    tag = cds_get_user_data(var, "DSProcVarTag");
    if (!tag) {

        tag = (VarTag *)calloc(1, sizeof(VarTag));
        if (!tag) {
            goto MEMORY_ERROR;
        }

        /* Attach the VarTag structure to the variable as user data */

        if (!cds_set_user_data(
            var, "DSProcVarTag", tag, _dsproc_free_var_tag)) {

            goto MEMORY_ERROR;
        }
    }

    return(tag);

MEMORY_ERROR:

    ERROR( DSPROC_LIB_NAME,
        "Could not create variable tag for: %s\n"
        " -> memory allocation error\n",
        var->name);

    dsproc_set_status(DSPROC_ENOMEM);
    return((VarTag *)NULL);
}

/*******************************************************************************
 *  Private Functions Visible Only To This Library
 */

/**
 *  Private: Create a variable tag for a retrieved variable.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  cds_var     - pointer to the CDSVar to attach the tag to
 *  @param  ret_group   - pointer to the RetDsGroup
 *  @param  ret_var     - pointer to the RetVariable
 *  @param  in_ds       - pointer to the input datastream in _DSProc
 *  @param  in_var_name - name of the variable in the input file
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int _dsproc_create_ret_var_tag(
    CDSVar       *cds_var,
    RetDsGroup   *ret_group,
    RetVariable  *ret_var,
    DataStream   *in_ds,
    const char   *in_var_name)
{
    VarTag       *tag;
    VarTarget    *target;
    RetVarOutput *out;
    int           ds_id;
    int           ti;

    /* Create the VarTag structure */

    tag = (VarTag *)calloc(1, sizeof(VarTag));
    if (!tag) {
        goto MEMORY_ERROR;
    }

    /* Set structure values */

    tag->in_ds = in_ds;

    if (STRDUP_FAILED( tag->in_var_name, in_var_name    ) ||
        STRDUP_FAILED( tag->valid_min,   ret_var->min   ) ||
        STRDUP_FAILED( tag->valid_max,   ret_var->max   ) ||
        STRDUP_FAILED( tag->valid_delta, ret_var->delta )) {

        goto MEMORY_ERROR;
    }

    if (ret_group && !(tag->ret_group_name = strdup(ret_group->name))) {
        goto MEMORY_ERROR;
    }

    if (ret_var->coord_system &&
        !(tag->coordsys_name = strdup(ret_var->coord_system->name))) {

        goto MEMORY_ERROR;
    }

    tag->required = ret_var->req_to_run;

    if (ret_var->noutputs) {

        tag->targets = (VarTarget **)
            calloc(ret_var->noutputs, sizeof(VarTarget *));

        if (!tag->targets) {
            goto MEMORY_ERROR;
        }

        for (ti = 0; ti < ret_var->noutputs; ti++) {

            out = ret_var->outputs[ti];

            ds_id = dsproc_get_datastream_id(
                NULL, NULL, out->dsc_name, out->dsc_level, DSR_OUTPUT);

            if (ds_id < 0) {

                WARNING( DSPROC_LIB_NAME,
                    "Could not set output target for variable:\n"
                    " -> input variable:  %s\n"
                    " -> output variable: %s.%s->%s\n"
                    " -> output datastream has not been defined\n",
                    cds_get_object_path(cds_var),
                    out->dsc_name, out->dsc_level, out->var_name);

                continue;
            }

            if (out->var_name) {
                target = _dsproc_create_var_target(ds_id, out->var_name);
            }
            else {
                target = _dsproc_create_var_target(ds_id, cds_var->name);
            }

            if (!target) {
                goto MEMORY_ERROR;
            }

            tag->targets[ti] = target;
            tag->ntargets++;
        }
    }

    /* Attach the VarTag structure to the variable as user data */

    if (!cds_set_user_data(
        cds_var, "DSProcVarTag", tag, _dsproc_free_var_tag)) {

        goto MEMORY_ERROR;
    }

    return(1);

MEMORY_ERROR:

    if (tag) {
        _dsproc_free_var_tag(tag);
    }

    ERROR( DSPROC_LIB_NAME,
        "Could not create variable tag for: %s\n"
        " -> memory allocation error\n",
        cds_var->name);

    dsproc_set_status(DSPROC_ENOMEM);
    return(0);
}

/** @publicsection */

/*******************************************************************************
 *  Internal Functions Visible To The Public
 */

/*******************************************************************************
 *  Public Functions
 */

/**
 *  Add an output target for a variable.
 *
 *  This function will add an output target for the variable.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param var      - pointer to the variable
 *  @param ds_id    - output datastream ID
 *  @param var_name - name of the variable in the output datastream
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int dsproc_add_var_output_target(
    CDSVar     *var,
    int         ds_id,
    const char *var_name)
{
    VarTag     *tag;
    VarTarget  *target;
    VarTarget **new_targets;

    if (ds_id < 0 ||
        ds_id >= _DSProc->ndatastreams ||
        _DSProc->datastreams[ds_id]->role != DSR_OUTPUT) {

        ERROR( DSPROC_LIB_NAME,
            "Could not add output target for variable:\n"
            " -> input variable:  %s\n"
            " -> output variable: %s\n"
            " -> invalid output datastream ID: %d\n",
            cds_get_object_path(var), var_name, ds_id);

        dsproc_set_status(DSPROC_EBADDSID);
        return(0);
    }

    if (msngr_debug_level || msngr_provenance_level) {

        DEBUG_LV1( DSPROC_LIB_NAME,
            "Adding output target for variable: %s\n"
            " -> %s->%s\n",
            cds_get_object_path(var),
            _DSProc->datastreams[ds_id]->name, var_name);
    }

    tag = _dsproc_get_or_create_var_tag(var);
    if (!tag) {
        return(0);
    }

    new_targets  = (VarTarget **)realloc(
        tag->targets, (tag->ntargets + 1) * sizeof(VarTarget *));

    if (!new_targets) {
        goto MEMORY_ERROR;
    }

    tag->targets = new_targets;

    target = _dsproc_create_var_target(ds_id, var_name);
    if (!target) {
        goto MEMORY_ERROR;
    }

    tag->targets[tag->ntargets] = target;
    tag->ntargets++;

    return(1);

MEMORY_ERROR:

    ERROR( DSPROC_LIB_NAME,
        "Could not add output target for variable: %s\n"
        " -> memory allocation error\n",
        var->name);

    dsproc_set_status(DSPROC_ENOMEM);
    return(0);
}

/**
 *  Copy a variable tag from one variable to another.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param src_var  - pointer to the source variable
 *  @param dest_var - pointer to the destination variable
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int dsproc_copy_var_tag(
    CDSVar *src_var,
    CDSVar *dest_var)
{
    VarTag *src_tag;
    VarTag *dest_tag;
    int     ti;

    /* Get the variable tag from the src_var */

    src_tag = cds_get_user_data(src_var, "DSProcVarTag");

    if (!src_tag) {
        return(1);
    }

    /* Create the variable tag for the dest_var */

    dest_tag = (VarTag *)calloc(1, sizeof(VarTag));
    if (!dest_tag) {
        goto MEMORY_ERROR;
    }

    /* Set structure values */

    dest_tag->in_ds = src_tag->in_ds;

    if (STRDUP_FAILED( dest_tag->in_var_name,    src_tag->in_var_name    ) ||
        STRDUP_FAILED( dest_tag->valid_min,      src_tag->valid_min      ) ||
        STRDUP_FAILED( dest_tag->valid_max,      src_tag->valid_max      ) ||
        STRDUP_FAILED( dest_tag->valid_delta,    src_tag->valid_delta    ) ||
        STRDUP_FAILED( dest_tag->ret_group_name, src_tag->ret_group_name ) ||
        STRDUP_FAILED( dest_tag->coordsys_name,  src_tag->coordsys_name  ) ) {

        goto MEMORY_ERROR;
    }

    dest_tag->required = src_tag->required;

    if (src_tag->ntargets) {

        dest_tag->targets = (VarTarget **)
            calloc(src_tag->ntargets, sizeof(VarTarget *));

        if (!dest_tag->targets) {
            goto MEMORY_ERROR;
        }

        for (ti = 0; ti < src_tag->ntargets; ti++) {

            dest_tag->targets[ti] = _dsproc_create_var_target(
                src_tag->targets[ti]->ds_id,
                src_tag->targets[ti]->var_name);

            if (!dest_tag->targets[ti]) {
                goto MEMORY_ERROR;
            }

            dest_tag->ntargets++;
        }
    }

    /* Attach the copy of the variable tag to the destination variable */

    if (!cds_set_user_data(
        dest_var, "DSProcVarTag", dest_tag, _dsproc_free_var_tag)) {

        goto MEMORY_ERROR;
    }

    return(1);

MEMORY_ERROR:

    if (dest_tag) {
        _dsproc_free_var_tag(dest_tag);
    }

    ERROR( DSPROC_LIB_NAME,
        "Could not copy variable tag for: %s\n"
        " -> memory allocation error\n",
        src_var->name);

    dsproc_set_status(DSPROC_ENOMEM);
    return(0);
}

/**
 *  Delete a variable tag.
 *
 *  @param var - pointer to the variable
 */
void dsproc_delete_var_tag(CDSVar *var)
{
    DEBUG_LV1( DSPROC_LIB_NAME,
        "Deleting variable tag for: %s\n",
        cds_get_object_path(var));

    cds_delete_user_data(var, "DSProcVarTag");
}

/**
 *  Set the coordinate system for a variable.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param var           - pointer to the variable
 *  @param coordsys_name - name of the coordinate system
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int dsproc_set_var_coordsys_name(
    CDSVar     *var,
    const char *coordsys_name)
{
    VarTag *tag;

    DEBUG_LV1( DSPROC_LIB_NAME,
        "Setting coordinate system name for variable: %s\n"
        " -> %s\n",
        cds_get_object_path(var), coordsys_name);

    tag = _dsproc_get_or_create_var_tag(var);
    if (!tag) {
        return(0);
    }

    if (tag->coordsys_name) {
        free((void *)tag->coordsys_name);
    }

    if (coordsys_name) {
        tag->coordsys_name = strdup(coordsys_name);
        if (!tag->coordsys_name) {

            ERROR( DSPROC_LIB_NAME,
                "Could not set coordinate system name for variable: %s\n"
                " -> memory allocation error\n",
                var->name);

            dsproc_set_status(DSPROC_ENOMEM);
            return(0);
        }
    }
    else {
        tag->coordsys_name = (const char *)NULL;
    }

    return(1);
}

/**
 *  Set the control flags for a variable.
 *
 *  <b>Control Flags:</b>
 *
 *    - VAR_SKIP_TRANSFORM  = Instruct the transform logic to ignore this
 *                            variable.
 *
 *    - VAR_ROLLUP_TRANS_QC = Consolidate the transformation QC bits when
 *                            they are mapped to the output dataset.
 *
 *  @param  var   - pointer to the variable
 *  @param  flags - flags to set
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int dsproc_set_var_flags(CDSVar *var, int flags)
{
    VarTag *tag;

    if (msngr_debug_level || msngr_provenance_level) {

        DEBUG_LV1( DSPROC_LIB_NAME,
            "Setting control flags for variable: %s\n",
            cds_get_object_path(var));

        if (flags & VAR_SKIP_TRANSFORM) {
            DEBUG_LV1( DSPROC_LIB_NAME, " - VAR_SKIP_TRANSFORM\n");
        }

        if (flags & VAR_ROLLUP_TRANS_QC) {
            DEBUG_LV1( DSPROC_LIB_NAME, " - VAR_ROLLUP_TRANS_QC\n");
        }
    }

    tag = _dsproc_get_or_create_var_tag(var);
    if (!tag) {
        return(0);
    }

    tag->flags |= flags;

    return(1);
}

/**
 *  Set the output target for a variable.
 *
 *  This function will replace any previously specified output targets
 *  already set for the variable.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param var      - pointer to the variable
 *  @param ds_id    - output datastream ID
 *  @param var_name - name of the variable in the output datastream
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int dsproc_set_var_output_target(
    CDSVar     *var,
    int         ds_id,
    const char *var_name)
{
    VarTag     *tag;
    VarTarget **new_targets;

    if (ds_id < 0 ||
        ds_id >= _DSProc->ndatastreams ||
        _DSProc->datastreams[ds_id]->role != DSR_OUTPUT) {

        ERROR( DSPROC_LIB_NAME,
            "Could not set output target for variable:\n"
            " -> input variable:  %s\n"
            " -> output variable: %s\n"
            " -> invalid output datastream ID: %d\n",
            cds_get_object_path(var), var_name, ds_id);

        dsproc_set_status(DSPROC_EBADDSID);
        return(0);
    }

    if (msngr_debug_level || msngr_provenance_level) {

        DEBUG_LV1( DSPROC_LIB_NAME,
            "Setting output target for variable: %s\n"
            " -> %s->%s\n",
            cds_get_object_path(var),
            _DSProc->datastreams[ds_id]->name, var_name);
    }

    tag = _dsproc_get_or_create_var_tag(var);
    if (!tag) {
        return(0);
    }

    new_targets = (VarTarget **)realloc(tag->targets, sizeof(VarTarget *));

    if (!new_targets) {
        goto MEMORY_ERROR;
    }

    tag->targets    = new_targets;
    tag->targets[0] = _dsproc_create_var_target(ds_id, var_name);
    if (!tag->targets[0]) {
        tag->ntargets = 0;
        goto MEMORY_ERROR;
    }

    tag->ntargets = 1;

    return(1);

MEMORY_ERROR:

    ERROR( DSPROC_LIB_NAME,
        "Could not set output target for variable: %s\n"
        " -> memory allocation error\n",
        var->name);

    dsproc_set_status(DSPROC_ENOMEM);
    return(0);
}

/**
 *  Unset the control flags for a variable.
 *
 *  See dsproc_set_var_flags() for flags and descriptions.
 *
 *  @param  var   - pointer to the variable
 *  @param  flags - flags to set
 */
void dsproc_unset_var_flags(CDSVar *var, int flags)
{
    VarTag *tag;

    if (msngr_debug_level || msngr_provenance_level) {

        DEBUG_LV1( DSPROC_LIB_NAME,
            "Unsetting control flags for variable: %s\n",
            cds_get_object_path(var));

        if (flags & VAR_SKIP_TRANSFORM) {
            DEBUG_LV1( DSPROC_LIB_NAME, " - VAR_SKIP_TRANSFORM\n");
        }
    }

    tag = cds_get_user_data(var, "DSProcVarTag");
    if (tag) {

        tag->flags &= (0xffff ^ flags);
    }
}

/**
 *  Get the name of the transformation coordinate system.
 *
 *  The memory used by the returned name belongs to the internal variable
 *  tag and must not be freed or altered by the calling process.
 *
 *  @param  var - pointer to the variable
 *
 *  @return
 *    - name of the coordinate system
 *    - NULL if a coordinate system was not specified for the variable
 */
const char *dsproc_get_var_coordsys_name(CDSVar *var)
{
    VarTag *tag = cds_get_user_data(var, "DSProcVarTag");

    if (tag) {
        return(tag->coordsys_name);
    }

    return((const char *)NULL);
}

/**
 *  Get the output targets defined for the specified variable.
 *
 *  The memory used by the output array belongs to the internal variable
 *  tag and must not be freed or altered by the calling process.
 *
 *  @param  var     - pointer to the variable
 *  @param  targets - output: pointer to the array of pointers
 *                            to the VarTarget structures.
 *
 *  @return
 *    - number of variable targets
 *    - 0 if no variable targets have been defined
 */
int dsproc_get_var_output_targets(CDSVar *var, VarTarget ***targets)
{
    VarTag *tag = cds_get_user_data(var, "DSProcVarTag");

    if (tag) {
        *targets = tag->targets;
        return(tag->ntargets);
    }

    *targets = (VarTarget **)NULL;
    return(0);
}

/**
 *  Get the name of the source variable read in from the input file.
 *
 *  The memory used by the returned variable name belongs to the internal
 *  variable tag and must not be freed or altered by the calling process.
 *
 *  @param  var - pointer to the variable
 *
 *  @return
 *    - pointer to the name of the source variable
 *    - NULL if the variable was not explicitly requested by the user
 *      in the retriever definition
 */
const char *dsproc_get_source_var_name(CDSVar *var)
{
    VarTag *tag = cds_get_user_data(var, "DSProcVarTag");

    if (tag &&
        tag->in_var_name) {

        return(tag->in_var_name);
    }

    return((const char *)NULL);
}

/**
 *  Get the name of the input datastream the variable was retrieved from.
 *
 *  The memory used by the returned datastream name belongs to the internal
 *  datastream structure and must not be freed or altered by the calling
 *  process.
 *
 *  @param  var - pointer to the variable
 *
 *  @return
 *    - pointer to the name of the input datastream
 *    - NULL if the variable was not explicitly requested by the user
 *      in the retriever definition
 */
const char *dsproc_get_source_ds_name(CDSVar *var)
{
    VarTag *tag = cds_get_user_data(var, "DSProcVarTag");

    if (tag &&
        tag->in_ds) {

        return(tag->in_ds->name);
    }

    return((const char *)NULL);
}

/**
 *  Get the ID of the input datastream the variable was retrieved from.
 *
 *  @param  var - pointer to the variable
 *
 *  @return
 *    - input datastream ID
 *    - -1 if the variable was not explicitly requested by the user
 *      in the retriever definition
 */
int dsproc_get_source_ds_id(CDSVar *var)
{
    VarTag *tag = cds_get_user_data(var, "DSProcVarTag");
    int     ds_id;

    if (tag &&
        tag->in_ds) {

        for (ds_id = 0; ds_id < _DSProc->ndatastreams; ++ds_id) {
            if (strcmp(_DSProc->datastreams[ds_id]->name, tag->in_ds->name) == 0) {
                return(ds_id);
            }
        }
    }

    return(-1);
}
