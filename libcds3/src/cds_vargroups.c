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
*    $Revision: 6691 $
*    $Author: ermold $
*    $Date: 2011-05-16 19:55:00 +0000 (Mon, 16 May 2011) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file cds_vargroups.c
 *  CDS Variable Groups.
 */

#include "cds3.h"
#include "cds_private.h"

/*******************************************************************************
 *  Private Functions
 */
/** @privatesection */

/**
 *  PRIVATE: Create a CDS Variable Group.
 *
 *  Private function used to create a variable group.
 *
 *  @param  group - pointer to the CDS group
 *  @param  name  - variable group name
 *
 *  @return
 *    - pointer to the new variable group
 *    - NULL if a memory allocation error occurred
 */
CDSVarGroup *_cds_create_vargroup(CDSGroup *group, const char *name)
{
    CDSVarGroup *vargroup;

    vargroup = (CDSVarGroup *)calloc(1, sizeof(CDSVarGroup));
    if (!vargroup) {
        return((CDSVarGroup *)NULL);
    }

    if (!_cds_init_object_members(vargroup, CDS_VARGROUP, group, name)) {
        free(vargroup);
        return((CDSVarGroup *)NULL);
    }

    return(vargroup);
}

/**
 *  PRIVATE: Destroy a CDS Variable Group.
 *
 *  Private function used to destroy a variable group.
 *
 *  @param  vargroup - pointer to the variable group
 */
void _cds_destroy_vargroup(CDSVarGroup *vargroup)
{
    int i;

    if (vargroup) {

        if (vargroup->arrays) {
            for (i = 0; i < vargroup->narrays; i++) {
                _cds_destroy_vararray(vargroup->arrays[i]);
            }
            free(vargroup->arrays);
        }

        _cds_free_object_members(vargroup);

        free(vargroup);
    }
}

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Add variables to a CDS Variable Group.
 *
 *  This function will also define the variable array if it does
 *  not already exist.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  vargroup - pointer to the CDS variable group
 *  @param  name     - name of the variable array to add the variables to
 *  @param  nvars    - number of variable to add
 *  @param  vars     - array of variable pointers to add
 *
 *  @return
 *    - pointer to the variable array the variables were added to
 *    - NULL if a memory allocation error occurred
 */
CDSVarArray *cds_add_vargroup_vars(
    CDSVarGroup *vargroup,
    const char  *name,
    int          nvars,
    CDSVar     **vars)
{
    CDSVarArray *vararray;

    /* Create the variable array if it does not already exist */

    vararray = cds_define_vararray(vargroup, name);
    if (!vararray) {
        return((CDSVarArray *)NULL);
    }

    /* Add the variables to the array */

    if (!cds_add_vararray_vars(vararray, nvars, vars)) {
        return((CDSVarArray *)NULL);
    }

    return(vararray);
}

/**
 *  Define a CDS Variable Group.
 *
 *  This function will first check if a variable group with the same
 *  name already exists in the CDS group. If it does, the existing
 *  variable group will be returned.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  group - pointer to the CDS group
 *  @param  name  - variable group name
 *
 *  @return
 *    - pointer to the variable group
 *    - NULL if a memory allocation error occurred
 */
CDSVarGroup *cds_define_vargroup(CDSGroup *group, const char *name)
{
    CDSVarGroup  *vargroup;
    CDSVarGroup **vargroups;

    /* Check if a variable group with this name already exists */

    vargroup = cds_get_vargroup(group, name);
    if (vargroup) {
        return(vargroup);
    }

    /* Allocate space for a new variable group */

    vargroups = (CDSVarGroup **)realloc(
        group->vargroups, (group->nvargroups + 2) * sizeof(CDSVarGroup *));

    if (!vargroups) {

        ERROR( CDS_LIB_NAME,
            "Could not define variable group: %s/_vargroups_/%s\n"
            " -> memory allocation error\n",
            cds_get_object_path(group), name);

        return((CDSVarGroup *)NULL);
    }

    group->vargroups = vargroups;

    /* Create the variable group */

    group->vargroups[group->nvargroups] = _cds_create_vargroup(group, name);

    if (!group->vargroups[group->nvargroups]) {

        ERROR( CDS_LIB_NAME,
            "Could not define variable group: %s/_vargroups_/%s\n"
            " -> memory allocation error\n",
            cds_get_object_path(group), name);

        return((CDSVarGroup *)NULL);
    }

    group->nvargroups++;
    group->vargroups[group->nvargroups] = (CDSVarGroup *)NULL;

    return(group->vargroups[group->nvargroups - 1]);
}

/**
 *  Delete a CDS Variable Group.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  vargroup - pointer to the variable group
 *
 *  @return
 *    - 1 if the variable group was deleted
 *    - 0 if the variable group is locked
 */
int cds_delete_vargroup(CDSVarGroup *vargroup)
{
    CDSGroup *group = (CDSGroup *)vargroup->parent;

    /* Check if the vargroup is locked */

    if (vargroup->def_lock) {

        ERROR( CDS_LIB_NAME,
            "Could not delete variable group: %s\n"
            " -> the variable group definition lock is set to: %d\n",
            cds_get_object_path(vargroup), vargroup->def_lock);

        return(0);
    }

    /* Remove this vargroup from the parent */

    _cds_remove_object(
        (void **)group->vargroups, &(group->nvargroups), vargroup);

    /* Destroy the variable group */

    _cds_destroy_vargroup(vargroup);

    return(1);
}

/**
 *  Get a CDS Variable Group.
 *
 *  This function will search the specified CDS group for a
 *  vargroup with the specified name.
 *
 *  @param  group - pointer to the CDS group
 *  @param  name  - variable group name
 *
 *  @return
 *    - pointer to the variable group
 *    - NULL if not found
 */
CDSVarGroup *cds_get_vargroup(CDSGroup *group, const char *name)
{
    CDSVarGroup *vargroup;

    vargroup = _cds_get_object(
        (void **)group->vargroups, group->nvargroups, name);

    return(vargroup);
}
