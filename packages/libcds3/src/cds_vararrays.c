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

/** @file cds_vararrays.c
 *  CDS Variable Arrays.
 */

#include "cds3.h"
#include "cds_private.h"

/*******************************************************************************
 *  Private Functions
 */
/** @privatesection */

/**
 *  PRIVATE: Create a CDS Variable Array.
 *
 *  Private function used to create a variable array.
 *
 *  @param  vargroup - pointer to the variable group
 *  @param  name     - name of the variable array
 *
 *  @return
 *    - pointer to the new variable array
 *    - NULL if a memory allocation error occurred
 */
CDSVarArray *_cds_create_vararray(CDSVarGroup *vargroup, const char *name)
{
    CDSVarArray *vararray;

    vararray = (CDSVarArray *)calloc(1, sizeof(CDSVarArray));
    if (!vararray) {
        return((CDSVarArray *)NULL);
    }

    if (!_cds_init_object_members(vararray, CDS_VARARRAY, vargroup, name)) {
        free(vararray);
        return((CDSVarArray *)NULL);
    }

    return(vararray);
}

/**
 *  PRIVATE: Destroy a CDS Variable Array.
 *
 *  Private function used to destroy a variable array.
 *
 *  @param  vararray - pointer to the variable array
 */
void _cds_destroy_vararray(CDSVarArray *vararray)
{
    if (vararray) {
        if (vararray->vars) free(vararray->vars);
        _cds_free_object_members(vararray);
        free(vararray);
    }
}

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Add variables to a CDS Variable Array.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  vararray - pointer to the variable array
 *  @param  nvars    - number of variables to add
 *  @param  vars     - array of variable pointers to add
 *
 *  @return
 *    - 1 if successful
 *    - 0 if a memory allocation error occurred
 */
int cds_add_vararray_vars(
    CDSVarArray *vararray,
    int          nvars,
    CDSVar     **vars)
{
    CDSVar **new_vars;
    int      total_nvars;
    int      i, j;

    /* Allocate space for the variables being added */

    total_nvars = vararray->nvars + nvars;

    new_vars = (CDSVar **)realloc(
        vararray->vars, total_nvars * sizeof(CDSVar *));

    if (!new_vars) {

        ERROR( CDS_LIB_NAME,
            "Could not add variables to variable array: %s\n"
            " -> memory allocation error\n",
            cds_get_object_path(vararray));

        return(0);
    }

    vararray->vars = new_vars;

    for (i = 0, j = vararray->nvars; i < nvars; i++, j++) {
        vararray->vars[j] = vars[i];
    }

    vararray->nvars = total_nvars;

    return(1);
}

/**
 *  Create a CDS Variable Array.
 *
 *  The variable array returned by this function will contain an entry for
 *  every variable in the specified list of variable names. The entries for
 *  variables that are not found in the CDS group will be NULL.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  group         - pointer to the CDS group
 *  @param  vargroup_name - name of the variable group to add the array to
 *  @param  vararray_name - name of the variable array to create or replace
 *  @param  nvars         - number variable names
 *  @param  var_names     - array of variable names
 *
 *  @return
 *    - pointer to the variable array
 *    - NULL if an error occurred
 */
CDSVarArray *cds_create_vararray(
    CDSGroup     *group,
    const char   *vargroup_name,
    const char   *vararray_name,
    int           nvars,
    const char  **var_names)
{
    CDSVarGroup  *vargroup;
    CDSVarArray  *vararray;
    CDSVar      **vars;
    int           vi;

    /* Allocate memory for the variables array */

    vars = (CDSVar **)malloc(nvars * sizeof(CDSVar *));
    if (!vars) {

        ERROR( CDS_LIB_NAME,
            "Could not get create variable array: %s/_vargroups_/%s/%s\n"
            " -> memory allocation error\n",
            cds_get_object_path(group), vargroup_name, vararray_name);

        return((CDSVarArray *)NULL);
    }

    /* Add variable pointers to array */

    for (vi = 0; vi < nvars; vi++) {
        vars[vi] = cds_get_var(group, var_names[vi]);
    }

    /* Define the variable group if it doesn’t already exist */

    vargroup = cds_define_vargroup(group, vargroup_name);
    if (!vargroup) {
        free(vars);
        return((CDSVarArray *)NULL);
    }

    /* Define the variable array if it doesn’t already exist */

    vararray = cds_define_vararray(vargroup, vararray_name);
    if (!vararray) {
        free(vars);
        return((CDSVarArray *)NULL);
    }

    /* Add or replace the variables array in the vararray */

    if (vararray->vars) {
        free(vararray->vars);
    }

    vararray->nvars = nvars;
    vararray->vars  = vars;

    return(vararray);
}

/**
 *  Define a CDS Variable Array.
 *
 *  This function will first check if a variable array with the same
 *  name already exists in the variable group. If it does, the existing
 *  variable array will be returned.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  vargroup - pointer to the CDS variable group
 *  @param  name     - name of the variable array
 *
 *  @return
 *    - pointer to the variable array
 *    - NULL if:
 *        - the parent variable group is locked
 *        - a memory allocation error occurred
 */
CDSVarArray *cds_define_vararray(CDSVarGroup *vargroup, const char *name)
{
    CDSVarArray  *vararray;
    CDSVarArray **vararrays;

    /* Check if a variable array with this name already exists */

    vararray = cds_get_vararray(vargroup, name);
    if (vararray) {
        return(vararray);
    }

    /* Check if the parent vargroup is locked */

    if (vargroup->def_lock) {

        ERROR( CDS_LIB_NAME,
            "Could not define variable array: %s/%s\n"
            " -> the variable group definition lock is set to: %d\n",
            cds_get_object_path(vargroup), name, vargroup->def_lock);

        return((CDSVarArray *)NULL);
    }

    /* Allocate space for a new variable array */

    vararrays = (CDSVarArray **)realloc(
        vargroup->arrays, (vargroup->narrays + 2) * sizeof(CDSVarArray *));

    if (!vararrays) {

        ERROR( CDS_LIB_NAME,
            "Could not define variable array: %s/%s\n"
            " -> memory allocation error\n",
            cds_get_object_path(vargroup), name);

        return((CDSVarArray *)NULL);
    }

    vargroup->arrays = vararrays;

    /* Create the variable array */

    vargroup->arrays[vargroup->narrays] = _cds_create_vararray(vargroup, name);

    if (!vargroup->arrays[vargroup->narrays]) {

        ERROR( CDS_LIB_NAME,
            "Could not define variable array: %s/%s\n"
            " -> memory allocation error\n",
            cds_get_object_path(vargroup), name);

        return((CDSVarArray *)NULL);
    }

    vargroup->narrays++;
    vargroup->arrays[vargroup->narrays] = (CDSVarArray *)NULL;

    return(vargroup->arrays[vargroup->narrays - 1]);
}

/**
 *  Delete a CDS Variable Array.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  vararray - pointer to the variable array
 *
 *  @return
 *    - 1 if the variable array was deleted
 *    - 0 if:
 *        - the variable array is locked
 *        - the parent variable group is locked
 */
int cds_delete_vararray(CDSVarArray *vararray)
{
    CDSVarGroup *vargroup = (CDSVarGroup *)vararray->parent;

    /* Check if the vararray is locked */

    if (vararray->def_lock) {

        ERROR( CDS_LIB_NAME,
            "Could not delete variable array: %s\n"
            " -> the variable array definition lock is set to: %d\n",
            cds_get_object_path(vararray), vararray->def_lock);

        return(0);
    }

    /* Check if the parent vargroup is locked */

    if (vargroup->def_lock) {

        ERROR( CDS_LIB_NAME,
            "Could not delete variable array: %s\n"
            " -> the variable group definition lock is set to: %d\n",
            cds_get_object_path(vararray), vargroup->def_lock);

        return(0);
    }

    /* Remove this vararray from the parent */

    _cds_remove_object(
        (void **)vargroup->arrays, &(vargroup->narrays), vararray);

    /* Destroy the variable array */

    _cds_destroy_vararray(vararray);

    return(1);
}

/**
 *  Get a CDS Variable Array.
 *
 *  This function will search the specified variable group for a
 *  vararray with the specified name.
 *
 *  @param  vargroup - pointer to the variable group
 *  @param  name     - variable array name
 *
 *  @return
 *    - pointer to the variable array
 *    - NULL if not found
 */
CDSVarArray *cds_get_vararray(CDSVarGroup *vargroup, const char *name)
{
    CDSVarArray *vararray;

    vararray = _cds_get_object(
        (void **)vargroup->arrays, vargroup->narrays, name);

    return(vararray);
}
