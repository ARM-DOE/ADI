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
*    $Revision: 7525 $
*    $Author: ermold $
*    $Date: 2011-07-20 00:38:56 +0000 (Wed, 20 Jul 2011) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file cds_groups.c
 *  CDS Groups.
 */

#include "cds3.h"
#include "cds_private.h"

/*******************************************************************************
 *  Private Functions
 */
/** @privatesection */

/**
 *  PRIVATE: Create a CDS Group.
 *
 *  Private function used to create a group.
 *
 *  @param  parent - pointer to the parent group, or
 *                   NULL to create a root group
 *  @param  name   - group name
 *
 *  @return
 *    - pointer to the new group
 *    - NULL if a memory allocation error occurred
 */
CDSGroup *_cds_create_group(CDSGroup *parent, const char *name)
{
    CDSGroup *group;

    group = (CDSGroup *)calloc(1, sizeof(CDSGroup));
    if (!group) {
        return((CDSGroup *)NULL);
    }

    if (!_cds_init_object_members(group, CDS_GROUP, parent, name)) {
        free(group);
        return((CDSGroup *)NULL);
    }

    return(group);
}

/**
 *  PRIVATE: Destroy a CDS Group.
 *
 *  Private function used to destroy a group.
 *
 *  @param  group - pointer to the group
 */
void _cds_destroy_group(CDSGroup *group)
{
    int id;

    if (group) {

        if (group->vargroups) {
            for (id = 0; id < group->nvargroups; id++) {
                _cds_destroy_vargroup(group->vargroups[id]);
            }
            free(group->vargroups);
        }

        if (group->groups) {
            for (id = 0; id < group->ngroups; id++) {
                _cds_destroy_group(group->groups[id]);
            }
            free(group->groups);
        }

        if (group->vars) {
            for (id = 0; id < group->nvars; id++) {
                _cds_destroy_var(group->vars[id]);
            }
            free(group->vars);
        }

        if (group->dims) {
            for (id = 0; id < group->ndims; id++) {
                _cds_destroy_dim(group->dims[id]);
            }
            free(group->dims);
        }

        if (group->atts) {
            for (id = 0; id < group->natts; id++) {
                _cds_destroy_att(group->atts[id]);
            }
            free(group->atts);
        }

        if (group->transform_params) {
            _cds_free_transform_params(group->transform_params);
        }

        _cds_free_object_members(group);

        free(group);
    }
}

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Define a CDS Group.
 *
 *  This function will first check if a group with the same name
 *  already exists in the parent group. If it does, the existing
 *  group will be returned.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  parent - pointer to the parent group, or
 *                   NULL to create the root group
 *  @param  name   - group name
 *
 *  @return
 *    - pointer to the group
 *    - NULL if:
 *        - the parent group is locked
 *        - a memory allocation error occurred
 */
CDSGroup *cds_define_group(CDSGroup *parent, const char *name)
{
    CDSGroup  *group;
    CDSGroup **groups;

    /* Check if we are creating the root group */

    if (!parent) {

        group = _cds_create_group(NULL, name);

        if (!group) {

            ERROR( CDS_LIB_NAME,
                "Could not define group: /%s\n"
                " -> memory allocation error\n", name);

            return((CDSGroup *)NULL);
        }

        return(group);
    }

    /* Check if a group with this name already exists */

    group = cds_get_group(parent, name);
    if (group) {
        return(group);
    }

    /* Check if the parent group is locked */

    if (parent->def_lock) {

        ERROR( CDS_LIB_NAME,
            "Could not define group: %s/%s\n"
            " -> the parent group definition lock is set to: %d\n",
            cds_get_object_path(parent), name, parent->def_lock);

        return((CDSGroup *)NULL);
    }

    /* Allocate space for a new group */

    groups = (CDSGroup **)realloc(
        parent->groups, (parent->ngroups + 2) * sizeof(CDSGroup *));

    if (!groups) {

        ERROR( CDS_LIB_NAME,
            "Could not define group: %s/%s\n"
            " -> memory allocation error\n",
            cds_get_object_path(parent), name);

        return((CDSGroup *)NULL);
    }

    parent->groups = groups;

    /* Create the group */

    parent->groups[parent->ngroups] = _cds_create_group(parent, name);

    if (!parent->groups[parent->ngroups]) {

        ERROR( CDS_LIB_NAME,
            "Could not define group: %s/%s\n"
            " -> memory allocation error\n",
            cds_get_object_path(parent), name);

        return((CDSGroup *)NULL);
    }

    parent->ngroups++;
    parent->groups[parent->ngroups] = (CDSGroup *)NULL;

    return(parent->groups[parent->ngroups - 1]);
}

/**
 *  Delete a CDS Group.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  group - pointer to the group
 *
 *  @return
 *    - 1 if the group was deleted
 *    - 0 if:
 *        - the group is locked
 *        - the parent group is locked
 */
int cds_delete_group(CDSGroup *group)
{
    CDSGroup *parent = (CDSGroup *)group->parent;

    /* Check if the group is locked */

    if (group->def_lock) {

        ERROR( CDS_LIB_NAME,
            "Could not delete group: %s\n"
            " -> the group definition lock is set to: %d\n",
            cds_get_object_path(group), group->def_lock);

        return(0);
    }

    /* Check if the parent group is locked */

    if (parent && parent->def_lock) {

        ERROR( CDS_LIB_NAME,
            "Could not delete group: %s\n"
            " -> the parent group definition lock is set to: %d\n",
            cds_get_object_path(group), parent->def_lock);

        return(0);
    }

    /* Remove this group from the parent */

    if (parent) {
        _cds_remove_object((void **)parent->groups, &(parent->ngroups), group);
    }

    /* Destroy the group */

    _cds_destroy_group(group);

    return(1);
}

/**
 *  Get a CDS Group.
 *
 *  This function will search the specified parent group for a
 *  group with the specified name.
 *
 *  @param  parent - pointer to the parent group
 *  @param  name   - name of the child group
 *
 *  @return
 *    - pointer to the child group
 *    - NULL if not found
 */
CDSGroup *cds_get_group(CDSGroup *parent, const char *name)
{
    CDSGroup *group;

    group = _cds_get_object((void **)parent->groups, parent->ngroups, name);

    return(group);
}

/**
 *  Rename a CDS Group.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  group - pointer to the group
 *  @param  name  - pointer to the new group name
 *
 *  @return
 *    - 1 if the group was renamed
 *    - 0 if:
 *        - a group with the new name already exists
 *        - the group is locked
 *        - the parent group is locked
 *        - a memory allocation error occured
 */
int cds_rename_group(CDSGroup *group, const char *name)
{
    CDSGroup *parent = (CDSGroup *)group->parent;
    char     *new_name;

    /* Check if a group with the new name already exists */

    if (parent && cds_get_group(parent, name)) {

        ERROR( CDS_LIB_NAME,
            "Could not rename group: %s to %s\n"
            " -> group exists\n",
            cds_get_object_path(group), name);

        return(0);
    }

    /* Check if the group is locked */

    if (group->def_lock) {

        ERROR( CDS_LIB_NAME,
            "Could not rename group: %s to %s\n"
            " -> the group definition lock is set to: %d\n",
            cds_get_object_path(group), name, group->def_lock);

        return(0);
    }

    /* Check if the parent group is locked */

    if (parent && parent->def_lock) {

        ERROR( CDS_LIB_NAME,
            "Could not rename group: %s to %s\n"
            " -> the parent group definition lock is set to: %d\n",
            cds_get_object_path(group), name, parent->def_lock);

        return(0);
    }

    /* Rename the group */

    new_name = strdup(name);
    if (!new_name) {

        ERROR( CDS_LIB_NAME,
            "Could not rename group: %s to %s\n"
            " -> memory allocation error\n",
            cds_get_object_path(group), name);

        return(0);
    }

    free(group->name);
    group->name = new_name;

    return(1);
}
