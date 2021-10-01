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
*    $Revision: 54278 $
*    $Author: ermold $
*    $Date: 2014-05-09 18:58:41 +0000 (Fri, 09 May 2014) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file cds_vars.c
 *  CDS Variables.
 */

#include "cds3.h"
#include "cds_private.h"

/*******************************************************************************
 *  Private Functions
 */
/** @privatesection */

/**
 *  PRIVATE: Create a CDS Variable.
 *
 *  Private function used to create a variable.
 *
 *  @param  group - pointer to the parent group
 *  @param  name  - variable name
 *  @param  type  - variable type
 *  @param  ndims - number of dimensions
 *  @param  dims  - array of dimension pointers
 *
 *  @return
 *    - pointer to the new variable
 *    - NULL if a memory allocation error occurred
 */
CDSVar *_cds_create_var(
    CDSGroup     *group,
    const char   *name,
    CDSDataType   type,
    int           ndims,
    CDSDim      **dims)
{
    CDSVar *var;

    var = (CDSVar *)calloc(1, sizeof(CDSVar));
    if (!var) {
        return((CDSVar *)NULL);
    }

    if (!_cds_init_object_members(var, CDS_VAR, group, name)) {
        free(var);
        return((CDSVar *)NULL);
    }

    var->type  = type;
    var->ndims = ndims;
    var->dims  = dims;

    return(var);
}

/**
 *  PRIVATE: Destroy a CDS Variable.
 *
 *  Private function used to destroy a variable.
 *
 *  @param  var - pointer to the variable
 */
void _cds_destroy_var(CDSVar *var)
{
    int ai;

    if (var) {

        cds_delete_var_data(var);

        if (var->atts) {
            for (ai = 0; ai < var->natts; ai++) {
                _cds_destroy_att(var->atts[ai]);
            }
            free(var->atts);
        }

        if (var->dims)           free(var->dims);
        if (var->default_fill)   free(var->default_fill);

        _cds_free_object_members(var);

        free(var);
    }
}

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Define a CDS Variable.
 *
 *  This function will first check if a variable with the same
 *  definition already exists in the specified group. If it does,
 *  the existing variable will be returned.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  group     - pointer to the group
 *  @param  name      - variable name
 *  @param  type      - variable data type
 *  @param  ndims     - number of dimensions
 *  @param  dim_names - array of pointers to the dimension names
 *
 *  @return
 *    - pointer to the variable
 *    - NULL if:
 *        - the group is locked
 *        - a dimension has not been defined
 *        - the unlimited dimension is not first
 *        - a memory allocation error occurred
 *        - a variable with the same name but different definition
 *          has already been defined
 */
CDSVar *cds_define_var(
    CDSGroup    *group,
    const char  *name,
    CDSDataType  type,
    int          ndims,
    const char **dim_names)
{
    CDSVar  *var;
    CDSVar **vars;
    CDSDim **dims;
    int      di;

    /* Check if a variable with this name already exists */

    var = cds_get_var(group, name);
    if (var) {

        if (type  == var->type &&
            ndims == var->ndims) {

            for (di = 0; di < ndims; di++) {
                if (strcmp(dim_names[di], var->dims[di]->name) != 0) {
                    break;
                }
            }

            if (di == ndims) {

                /* The existing variable has the same definition */

                return(var);
            }
        }

        ERROR( CDS_LIB_NAME,
            "Could not define variable: %s/_vars_/%s\n"
            " -> variable exists\n",
            cds_get_object_path(group), name);

        return((CDSVar *)NULL);
    }

    /* Check if the group is locked */

    if (group->def_lock) {

        ERROR( CDS_LIB_NAME,
            "Could not define variable: %s/_vars_/%s\n"
            " -> the group definition lock is set to: %d\n",
            cds_get_object_path(group), name, group->def_lock);

        return((CDSVar *)NULL);
    }

    /* Create the array of dimension pointers */

    dims = (CDSDim **)calloc(ndims+1, sizeof(CDSDim *));

    if (!dims) {

        ERROR( CDS_LIB_NAME,
            "Could not define variable: %s/_vars_/%s\n"
            " -> memory allocation error\n",
            cds_get_object_path(group), name);

        return((CDSVar *)NULL);
    }

    for (di = 0; di < ndims; di++) {

        dims[di] = cds_get_dim(group, dim_names[di]);

        if (!dims[di]) {

            ERROR( CDS_LIB_NAME,
                "Could not define variable: %s/_vars_/%s\n"
                " -> dimension not defined: %s\n",
                cds_get_object_path(group), name, dim_names[di]);

            free(dims);
            return((CDSVar *)NULL);
        }

        if (dims[di]->is_unlimited && di != 0) {

            ERROR( CDS_LIB_NAME,
                "Could not define variable: %s/_vars_/%s\n"
                " -> unlimited dimension must be first: %s\n",
                cds_get_object_path(group), name, dim_names[di]);

            free(dims);
            return((CDSVar *)NULL);
        }
    }

    /* Allocate space for a new variable */

    vars = (CDSVar **)realloc(
        group->vars, (group->nvars + 2) * sizeof(CDSVar *));

    if (!vars) {

        ERROR( CDS_LIB_NAME,
            "Could not define variable: %s/_vars_/%s\n"
            " -> memory allocation error\n",
            cds_get_object_path(group), name);

        return((CDSVar *)NULL);
    }

    group->vars = vars;

    /* Create the variable */

    group->vars[group->nvars] = _cds_create_var(
        group, name, type, ndims, dims);

    if (!group->vars[group->nvars]) {

        ERROR( CDS_LIB_NAME,
            "Could not define variable: %s/_vars_/%s\n"
            " -> memory allocation error\n",
            cds_get_object_path(group), name);

        return((CDSVar *)NULL);
    }

    group->nvars++;
    group->vars[group->nvars] = (CDSVar *)NULL;

    return(group->vars[group->nvars - 1]);
}

/**
 *  Delete a CDS Variable.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  var - pointer to the variable
 *
 *  @return
 *    - 1 if the variable was deleted
 *    - 0 if:
 *        - the variable is locked
 *        - the group is locked
 */
int cds_delete_var(CDSVar *var)
{
    CDSGroup *group = (CDSGroup *)var->parent;

    /* Check if the variable is locked */

    if (var->def_lock) {

        ERROR( CDS_LIB_NAME,
            "Could not delete variable: %s\n"
            " -> the variable definition lock is set to: %d\n",
            cds_get_object_path(var), var->def_lock);

        return(0);
    }

    /* Check if the group is locked */

    if (group->def_lock) {

        ERROR( CDS_LIB_NAME,
            "Could not delete variable: %s\n"
            " -> the group definition lock is set to: %d\n",
            cds_get_object_path(var), group->def_lock);

        return(0);
    }

    /* Remove this variable from the group */

    _cds_remove_object((void **)group->vars, &(group->nvars), var);

    /* Destroy the variable */

    _cds_destroy_var(var);

    return(1);
}

/**
 *  Get the coordinate variable associated with a boundary variable.
 *
 *  @param  bounds_var - pointer to the boundary variable
 *
 *  @return
 *    - pointer to the coordinate variable
 *    - NULL if not found
 */
CDSVar *cds_get_bounds_coord_var(CDSVar *bounds_var)
{
    CDSGroup *group = (CDSGroup *)bounds_var->parent;
    CDSVar   *var;
    CDSAtt   *att;
    int       vi;

    for (vi = 0; vi < group->nvars; ++vi) {
        var = group->vars[vi];
        att = cds_get_att(var, "bounds");
        if (att && att->type == CDS_CHAR && att->length > 0 && att->value.cp) {
            if (strcmp(bounds_var->name, att->value.cp) == 0) {
                return(var);
            }
        }
    }

    return((CDSVar *)NULL);
}

/**
 *  Get the boundary variable for a CDS coordinate variable.
 *
 *  @param  coord_var - pointer to the coordinate variable
 *
 *  @return
 *    - pointer to the boundary variable
 *    - NULL if not found
 */
CDSVar *cds_get_bounds_var(CDSVar *coord_var)
{
    CDSVar *bounds_var;
    CDSAtt *att;

    /* Check for the bounds attribute */

    att = cds_get_att(coord_var, "bounds");
    if (!att || att->type != CDS_CHAR || att->length == 0 || !att->value.cp) {
        return((CDSVar *)NULL);
    }

    /* Check for the bounds variable */

    bounds_var = cds_get_var((CDSGroup *)coord_var->parent, att->value.cp);
    if (!bounds_var) {
        return((CDSVar *)NULL);
    }

    return(bounds_var);
}

/**
 *  Get the coordinate variable for a CDS Variable's dimension.
 *
 *  @param  var       - pointer to the variable
 *  @param  dim_index - index of the dimension to get the coordinate
 *                      variable for
 *
 *  @return
 *    - pointer to the coordinate variable
 *    - NULL if not found
 */
CDSVar *cds_get_coord_var(CDSVar *var, int dim_index)
{
    CDSDim   *dim;
    CDSGroup *group;
    CDSVar   *coord_var;

    if (dim_index < 0 || dim_index >= var->ndims) {
        return((CDSVar *)NULL);
    }

    dim   = var->dims[dim_index];
    group = (CDSGroup *)var->parent;

    while (group) {

        coord_var = cds_get_var(group, dim->name);

        if (coord_var &&
            coord_var->ndims == 1 &&
            strcmp(coord_var->name, coord_var->dims[0]->name) == 0 ) {

            return(coord_var);
        }

        if (group == (CDSGroup *)dim->parent) {
            break;
        }

        group = (CDSGroup *)group->parent;
    }

    return((CDSVar *)NULL);
}

/**
 *  Get a CDS Variable.
 *
 *  This function will search the specified group for a variable
 *  with the specified name.
 *
 *  @param  group - pointer to the group
 *  @param  name  - name of the variable
 *
 *  @return
 *    - pointer to the variable
 *    - NULL if not found
 */
CDSVar *cds_get_var(CDSGroup *group, const char *name)
{
    CDSVar *var;

    var = _cds_get_object((void **)group->vars, group->nvars, name);

    return(var);
}

/**
 *  Rename a CDS Variable.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  var  - pointer to the variable
 *  @param  name - pointer to the new variable name
 *
 *  @return
 *    - 1 if the variable was renamed
 *    - 0 if:
 *        - a variable with the new name already exists
 *        - the variable is locked
 *        - the group is locked
 *        - a memory allocation error occured
 */
int cds_rename_var(CDSVar *var, const char *name)
{
    CDSGroup *group = (CDSGroup *)var->parent;
    char     *new_name;

    /* Check if a variable with the new name already exists */

    if (cds_get_var(group, name)) {

        ERROR( CDS_LIB_NAME,
            "Could not rename variable: %s to %s\n"
            " -> variable exists\n",
            cds_get_object_path(var), name);

        return(0);
    }

    /* Check if the variable is locked */

    if (var->def_lock) {

        ERROR( CDS_LIB_NAME,
            "Could not rename variable: %s to %s\n"
            " -> the variable definition lock is set to: %d\n",
            cds_get_object_path(var), name, var->def_lock);

        return(0);
    }

    /* Check if the group is locked */

    if (group->def_lock) {

        ERROR( CDS_LIB_NAME,
            "Could not rename variable: %s to %s\n"
            " -> the group definition lock is set to: %d\n",
            cds_get_object_path(var), name, group->def_lock);

        return(0);
    }

    /* Rename the variable */

    new_name = strdup(name);
    if (!new_name) {

        ERROR( CDS_LIB_NAME,
            "Could not rename variable: %s to %s\n"
            " -> memory allocation error\n",
            cds_get_object_path(var), name);

        return(0);
    }

    free(var->name);
    var->name = new_name;

    return(1);
}

/**
 *  Get the sample size of a CDS Variable.
 *
 *  The sample dimension is always the first dimension defined
 *  for a variable. If this dimension has unlimited length it
 *  is also refered to as the record dimension.
 *
 *  Variables with less than 2 dimensions will always have a
 *  sample_size of 1. The sample_size for variables with 2 or
 *  more dimensions is the product of all the dimension lengths
 *  starting with the 2nd dimension.
 *
 *  @param  var - pointer to the variable
 *
 *  @return  sample size of the variable
 */
size_t cds_var_sample_size(CDSVar *var)
{
    size_t sample_size = 1;
    int    di;

    if (var->ndims > 1) {
        for (di = 1; di < var->ndims; di++) {
            sample_size *= var->dims[di]->length;
        }
    }

    return(sample_size);
}

/**
 *  Check if a CDS Variable has the specified dimension.
 *
 *  @param  var   - pointer to the variable
 *  @param  name  - name of the dimension
 *
 *  @return
 *    - pointer to the dimension
 *    - NULL if not found
 */
CDSDim *cds_var_has_dim(CDSVar *var, const char *name)
{
    int di;

    for (di = 0; di < var->ndims; di++) {
        if (strcmp(var->dims[di]->name, name) == 0) {
            return(var->dims[di]);
        }
    }

    return((CDSDim *)NULL);
}

/**
 *  Check if a CDS Variable has an unlimited dimension.
 *
 *  @param  var - pointer to the variable
 *
 *  @return
 *    - 1 if the variable has an unlimited dimension
 *    - 0 if this variable does not have an unlimited dimension
 */
int cds_var_is_unlimited(CDSVar *var)
{
    if (var->ndims) {
        return(var->dims[0]->is_unlimited);
    }

    return(0);
}
