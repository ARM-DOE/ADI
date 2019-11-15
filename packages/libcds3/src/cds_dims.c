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
*    $Revision: 9749 $
*    $Author: ermold $
*    $Date: 2011-12-01 20:22:48 +0000 (Thu, 01 Dec 2011) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file cds_dims.c
 *  CDS Dimensions.
 */

#include "cds3.h"
#include "cds_private.h"

/*******************************************************************************
 *  Private Functions
 */
/** @privatesection */

/**
 *  PRIVATE: Create a CDS Dimension.
 *
 *  Private function used to create a dimension.
 *
 *  @param  group        - pointer to the parent group
 *  @param  name         - dimension name
 *  @param  length       - dimension length
 *                         (ignored if is_unlimited == TRUE)
 *  @param  is_unlimited - specifies if this dimension is unlimited
 *                         (0 = FALSE, 1 = TRUE)
 *  @return
 *    - a new dimension
 *    - NULL if a memory allocation error occurred
 */
CDSDim *_cds_create_dim(
    CDSGroup   *group,
    const char *name,
    size_t      length,
    int         is_unlimited)
{
    CDSDim *dim;

    dim = (CDSDim *)calloc(1, sizeof(CDSDim));
    if (!dim) {
        return((CDSDim *)NULL);
    }

    if (!_cds_init_object_members(dim, CDS_DIM, group, name)) {
        free(dim);
        return((CDSDim *)NULL);
    }

    dim->length       = (is_unlimited) ? 0 : length;
    dim->is_unlimited = is_unlimited;

    return(dim);
}

/**
 *  PRIVATE: Destroy a CDS Dimension.
 *
 *  Private function used to destroy a dimension.
 *
 *  @param  dim - pointer to the dimension
 */
void _cds_destroy_dim(CDSDim *dim)
{
    if (dim) {
        _cds_free_object_members(dim);
        free(dim);
    }
}

/**
 *  PRIVATE: Check if a CDS Dimension is in use.
 *
 *  Private function used to check if data has been added to a variable
 *  using this dimension.
 *
 *  @param  group - pointer to the group the dimension belongs to
 *  @param  dim   - pointer to the dimension
 *
 *  @return
 *    - 1 if the dimension is being used
 *    - 0 if the dimesnion is not being used.
 */
int _cds_is_dim_used(CDSGroup *group, CDSDim *dim)
{
    CDSVar *var;
    int     vi, di, gi;

    /* Search the variables in this group */

    for (vi = 0; vi < group->nvars; vi++) {
        var = group->vars[vi];
        if (var->data.vp) {
            for (di = 0; di < var->ndims; di++) {
                if (var->dims[di] == dim) {
                    return(1);
                }
            }
        }
    }

    /* Recurse into sub-groups */

    for (gi = 0; gi < group->ngroups; gi++) {
        if (_cds_is_dim_used(group->groups[gi], dim)) {
            return(1);
        }
    }

    return(0);
}

/**
 *  PRIVATE: Delete dependant variables.
 *
 *  Private function used to delete all variables that use the
 *  specifed dimension.
 *
 *  @param  group - pointer to the group the dimension belongs to
 *  @param  dim   - pointer to the dimension
 */
void _cds_delete_dependant_vars(CDSGroup *group, CDSDim *dim)
{
    CDSVar *var;
    int     vi, di, gi;

    /* Search the variables in this group */

    for (vi = 0; vi < group->nvars; vi++) {
        var = group->vars[vi];
        for (di = 0; di < var->ndims; di++) {
            if (var->dims[di] == dim) {
                _cds_remove_object((void **)group->vars, &(group->nvars), var);
                _cds_destroy_var(var);
                vi--;
                break;
            }
        }
    }

    /* Recurse into sub-groups */

    for (gi = 0; gi < group->ngroups; gi++) {
        _cds_delete_dependant_vars(group->groups[gi], dim);
    }
}

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Change the length of a CDS Dimension.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  dim    - pointer to the dimension
 *  @param  length - new dimension length
 *
 *  @return
 *    - 1 if:
 *        - the dimension length was changed
 *        - the new length was equal to the old length
 *        - this is an unlimited dimension
 *    - 0 if:
 *        - the dimension is locked
 *        - data has already been added to a variable using this dimension
 */
int cds_change_dim_length(CDSDim *dim, size_t length)
{
    CDSGroup *group = (CDSGroup *)dim->parent;

    /* Check if the new length equals the old length */

    if (dim->length == length) {
        return(1);
    }

    /* Check if this is an unlimited dimension. */

    if (dim->is_unlimited) {
        return(1);
    }

    /* Check if the dimension is locked */

    if (dim->def_lock) {

        ERROR( CDS_LIB_NAME,
            "Could not change dimension length for: %s\n"
            " -> the dimension definition lock is set to: %d\n",
            cds_get_object_path(dim), dim->def_lock);

        return(0);
    }

    /* The dimension size can not be changed if data has been added
     * to a variable using this dimension.
     */

    if (_cds_is_dim_used(group, dim)) {

        ERROR( CDS_LIB_NAME,
            "Could not change dimension length for: %s\n"
            " -> data has been added for a variable using this dimension\n",
            cds_get_object_path(dim));

        return(0);
    }

    /* Change the dimension length */

    dim->length = length;

    return(1);
}

/**
 *  Define a CDS Dimension.
 *
 *  This function will first check if a dimension with the same
 *  definition already exists in the specified group. If it does,
 *  the existing dimension will be returned.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  group        - pointer to the group
 *  @param  name         - dimension name
 *  @param  length       - dimension length
 *  @param  is_unlimited - is dimension unlimited (0 = FALSE, 1 = TRUE)
 *
 *  @return
 *    - pointer to the dimension
 *    - NULL if:
 *        - a static dimension with the same name but different length
 *          has already been defined for the specified group.
 *        - the group is locked
 *        - a memory allocation error occurred
 */
CDSDim *cds_define_dim(
    CDSGroup   *group,
    const char *name,
    size_t      length,
    int         is_unlimited)
{
    CDSDim  *dim;
    CDSDim **dims;

    /* Check if a dimension with this name already exists */

    dim = _cds_get_object((void **)group->dims, group->ndims, name);
    if (dim) {

        if ((is_unlimited == dim->is_unlimited) &&
            (is_unlimited || (length == dim->length))) {

            return(dim);
        }

        /* A dimension with this name but a different definition
         * has already been defined for this group */

        ERROR( CDS_LIB_NAME,
            "Could not define dimension: %s\n"
            " -> dimension exists\n", cds_get_object_path(dim));

        return((CDSDim *)NULL);
    }

    /* Check if the group is locked */

    if (group->def_lock) {

        ERROR( CDS_LIB_NAME,
            "Could not define dimension: %s/_dims_/%s\n"
            " -> the group definition lock is set to: %d\n",
            cds_get_object_path(group), name, group->def_lock);

        return((CDSDim *)NULL);
    }

    /* Allocate space for a new dimension */

    dims = (CDSDim **)realloc(
        group->dims, (group->ndims + 2) * sizeof(CDSDim *));

    if (!dims) {

        ERROR( CDS_LIB_NAME,
            "Could not define dimension: %s/_dims_/%s\n"
            " -> memory allocation error\n",
            cds_get_object_path(group), name);

        return((CDSDim *)NULL);
    }

    group->dims = dims;

    /* Create the dimension */

    group->dims[group->ndims] = _cds_create_dim(
        group, name, length, is_unlimited);

    if (!group->dims[group->ndims]) {

        ERROR( CDS_LIB_NAME,
            "Could not define dimension: %s/_dims_/%s\n"
            " -> memory allocation error\n",
            cds_get_object_path(group), name);

        return((CDSDim *)NULL);
    }

    group->ndims++;
    group->dims[group->ndims] = (CDSDim *)NULL;

    return(group->dims[group->ndims - 1]);
}

/**
 *  Delete a CDS Dimension.
 *
 *  This function will also delete all variables that use the
 *  specified dimension.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  dim - pointer to the dimension
 *
 *  @return
 *    - 1 if the dimension was deleted
 *    - 0 if:
 *        - the dimension is locked
 *        - the group is locked
 */
int cds_delete_dim(CDSDim *dim)
{
    CDSGroup *group = (CDSGroup *)dim->parent;

    /* Check if the dimension is locked */

    if (dim->def_lock) {

        ERROR( CDS_LIB_NAME,
            "Could not delete dimension: %s\n"
            " -> the dimension definition lock is set to: %d\n",
            cds_get_object_path(dim), dim->def_lock);

        return(0);
    }

    /* Check if the group is locked */

    if (group->def_lock) {

        ERROR( CDS_LIB_NAME,
            "Could not delete dimension: %s\n"
            " -> the group definition lock is set to: %d\n",
            cds_get_object_path(dim), group->def_lock);

        return(0);
    }

    /* Delete variables using this dimension */

    _cds_delete_dependant_vars(group, dim);

    /* Remove this dimension from the group */

    _cds_remove_object((void **)group->dims, &(group->ndims), dim);

    /* Destroy the dimension */

    _cds_destroy_dim(dim);

    return(1);
}

/**
 *  Get a CDS Dimension.
 *
 *  This function will search the specified group then and all
 *  ancestor groups for a dimension with the specified name.
 *  The first dimension found will be returned.
 *
 *  @param  group - pointer to the group
 *  @param  name  - name of the dimension
 *
 *  @return
 *    - pointer to the dimension
 *    - NULL if not found
 */
CDSDim *cds_get_dim(CDSGroup *group, const char *name)
{
    CDSDim *dim;

    dim = _cds_get_object((void **)group->dims, group->ndims, name);

    if (!dim && group->parent) {
        dim = cds_get_dim((CDSGroup *)group->parent, name);
    }

    return(dim);
}

/**
 *  Get the coordinate variable for a CDS Dimension.
 *
 *  @param  dim - pointer to the dimension
 *
 *  @return
 *    - pointer to the coordinate variable
 *    - NULL if not found
 */
CDSVar *cds_get_dim_var(CDSDim *dim)
{
    return(cds_get_var((CDSGroup *)dim->parent, dim->name));
}

/**
 *  Rename a CDS Dimension.
 *
 *  This function will also rename the coordinate variable for the
 *  dimension if one exists.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  dim  - pointer to the dimension
 *  @param  name - pointer to the new dimension name
 *
 *  @return
 *    - 1 if the dimension was deleted
 *    - 0 if:
 *        - a dimension with the new name already exists
 *        - the dimension is locked
 *        - the group is locked
 *        - the coordinate variable could not be renamed (see cds_rename_var())
 *        - a memory allocation error occured
 */
int cds_rename_dim(CDSDim *dim, const char *name)
{
    CDSGroup *group = (CDSGroup *)dim->parent;
    CDSVar   *coord_var;
    char     *new_name;

    /* Check if a dimension with the new name already exists */

    if (cds_get_dim(group, name)) {

        ERROR( CDS_LIB_NAME,
            "Could not rename dimension: %s to %s\n"
            " -> dimension exists\n",
            cds_get_object_path(dim), name);

        return(0);
    }

    /* Check if the dimension is locked */

    if (dim->def_lock) {

        ERROR( CDS_LIB_NAME,
            "Could not rename dimension: %s to %s\n"
            " -> the dimension definition lock is set to: %d\n",
            cds_get_object_path(dim), name, dim->def_lock);

        return(0);
    }

    /* Check if the group is locked */

    if (group->def_lock) {

        ERROR( CDS_LIB_NAME,
            "Could not rename dimension: %s to %s\n"
            " -> the group definition lock is set to: %d\n",
            cds_get_object_path(dim), name, group->def_lock);

        return(0);
    }

    /* Create the new dimension name */

    new_name = strdup(name);
    if (!new_name) {

        ERROR( CDS_LIB_NAME,
            "Could not rename dimension: %s to %s\n"
            " -> memory allocation error\n",
            cds_get_object_path(dim), name);

        return(0);
    }

    /* Rename the coordinate variable if one exists */

    coord_var = cds_get_dim_var(dim);
    if (coord_var) {
        if (!cds_rename_var(coord_var, new_name)) {
            return(0);
        }
    }

    /* Rename the dimension */

    free(dim->name);
    dim->name = new_name;

    return(1);
}
