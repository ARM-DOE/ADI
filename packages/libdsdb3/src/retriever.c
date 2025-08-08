/*******************************************************************************
*
*  COPYRIGHT (C) 2010 Battelle Memorial Institute.  All Rights Reserved.
*
********************************************************************************
*
*  Author:
*     name:  Krista Gaustd
*     phone: (509) 375-5950
*     email: krista.gaustad@pnl.gov
*
********************************************************************************
*
*  REPOSITORY INFORMATION:
*    $Revision: 54302 $
*    $Author: ermold $
*    $Date: 2014-05-13 02:30:31 +0000 (Tue, 13 May 2014) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file retriever.c
 *  Retriever Functions.
 */

#include <string.h>

#include "dsdb3.h"
#include "dbog_retriever.h"

/**
 *  @defgroup DSDB_RETRIEVER Retriever Functions
 */
/*@{*/

/*******************************************************************************
 *  Private Functions
 */
/** @privatesection */

/**
 *  Free all memory used by a RetDataStream structure.
 *
 *  @param  ds - pointer to the RetDataStream structure
 */
static void ret_free_datastream(RetDataStream *ds)
{
    if (ds) {

        if (ds->name)     free((void *)ds->name);
        if (ds->level)    free((void *)ds->level);
        if (ds->site)     free((void *)ds->site);
        if (ds->facility) free((void *)ds->facility);
        if (ds->dep_site) free((void *)ds->dep_site);
        if (ds->dep_fac)  free((void *)ds->dep_fac);

        free(ds);
    }
}

/**
 *  Free all memory used by a RetDsVarMap structure.
 *
 *  @param  varmap - pointer to the RetDsVarMap structure
 */
static void ret_free_varmap(RetDsVarMap *varmap)
{
    int i;

    if (varmap) {

        for (i = 0; i < varmap->nnames; i++) {
            free((void *)varmap->names[i]);
        }

        if (varmap->names) free((void *)varmap->names);

        free(varmap);
    }
}

/**
 *  Free all memory used by a RetVariable structure.
 *
 *  @param  var - pointer to the RetVariable structure
 */
static void ret_free_variable(RetVariable *var)
{
    RetVarOutput *output;
    int           i;

    if (var) {

        if (var->name)      free((void *)var->name);
        if (var->data_type) free((void *)var->data_type);
        if (var->units)     free((void *)var->units);

        if (var->min)       free((void *)var->min);
        if (var->max)       free((void *)var->max);
        if (var->delta)     free((void *)var->delta);

        /* Free variable dimension names */

        for (i = 0; i < var->ndim_names; i++) {
            free((void *)var->dim_names[i]);
        }

        if (var->dim_names) free((void *)var->dim_names);

        /* Free variable mapping structures */

        for (i = 0; i < var->nvarmaps; i++) {
            ret_free_varmap(var->varmaps[i]);
        }

        if (var->varmaps) free(var->varmaps);

        /* Free variable output structures */

        for (i = 0; i < var->noutputs; i++) {

            output = var->outputs[i];

            if (output->dsc_name)  free((void *)output->dsc_name);
            if (output->dsc_level) free((void *)output->dsc_level);
            if (output->var_name)  free((void *)output->var_name);

            free(output);
        }

        if (var->outputs) free(var->outputs);

        free(var);
    }
}

/**
 *  Free all memory used by a RetDsGroup structure.
 *
 *  @param  group - pointer to the RetDsGroup structure
 */
static void ret_free_group(RetDsGroup *group)
{
    int i;

    if (group) {

        if (group->name)      free((void *)group->name);
        if (group->subgroups) free(group->subgroups);

        /* Free variable structures */

        for (i = 0; i < group->nvars; i++) {
            ret_free_variable(group->vars[i]);
        }

        if (group->vars) free(group->vars);

        free(group);
    }
}

/**
 *  Free all memory used by a RetDsSubGroup structure.
 *
 *  @param  subgroup - pointer to the RetDsSubGroup structure
 */
static void ret_free_subgroup(RetDsSubGroup *subgroup)
{
    if (subgroup) {

        if (subgroup->name)        free((void *)subgroup->name);
        if (subgroup->datastreams) free(subgroup->datastreams);

        free(subgroup);
    }
}

/**
 *  Free all memory used by a RetTransParams structure.
 *
 *  @param  trans_params - pointer to the RetTransParams structure
 */
static void ret_free_trans_params(RetTransParams *trans_params)
{
    if (trans_params) {

        if (trans_params->coordsys) free((void *)trans_params->coordsys);
        if (trans_params->params)   free((void *)trans_params->params);

        free(trans_params);
    }
}

/**
 *  Delete entries from a varmaps list that reference a specified datastream.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  ds       - pointer to the RetDataStream structure.
 *  @param  nvarmaps - input/output: number of variable maps in the list
 *  @param  varmaps  - pointer to the list of RetDsVarMap structures.
 */
static void ret_delete_ds_from_varmap_list(
    RetDataStream *ds,
    int           *nvarmaps,
    RetDsVarMap  **varmaps)
{
    int nvm;
    int vmi;

    nvm = 0;

    for (vmi = 0; vmi < *nvarmaps; vmi++) {

        if (varmaps[vmi]->ds == ds) {
            ret_free_varmap(varmaps[vmi]);
            continue;
        }

        if (nvm != vmi) {
            varmaps[nvm] = varmaps[vmi];
        }

        nvm++;
    }

    *nvarmaps = nvm;
}

/**
 *  Delete a subgroup from a retriever structure.
 *
 *  This function will delete the specified subgroup and all references
 *  to it from a retriever structure.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  ret      - pointer to the Retriever structure
 *  @param  subgroup - pointer to the RetDsSubGroup to delete.
 */
static void ret_delete_subgroup(
    Retriever     *ret,
    RetDsSubGroup *subgroup)
{
    RetDsGroup *group;
    int         ngroups, nsubgroups;
    int         gi, sgi;

    /************************************************************
    * Remove subgroup references from groups.
    ************************************************************/

    ngroups = 0;

    for (gi = 0; gi < ret->ngroups; gi++) {

        group      = ret->groups[gi];
        nsubgroups = 0;

        for (sgi = 0; sgi < group->nsubgroups; sgi++) {

            if (group->subgroups[sgi] == subgroup) {
                continue;
            }

            if (nsubgroups != sgi) {
                group->subgroups[nsubgroups] = group->subgroups[sgi];
            }

            nsubgroups += 1;

        } /* end loop over subgroups */

        group->nsubgroups = nsubgroups;

        if (group->nsubgroups == 0) {
            ret_free_group(group);
            continue;
        }

        if (ngroups != gi) {
            ret->groups[ngroups] = ret->groups[gi];
        }

        ngroups += 1;

    } /* end loop over datastream groups */

    ret->ngroups = ngroups;

    /************************************************************
    * Delete the subgroup.
    ************************************************************/

    nsubgroups = 0;

    for (sgi = 0; sgi < ret->nsubgroups; sgi++) {

        if (ret->subgroups[sgi] == subgroup) {
            ret_free_subgroup(subgroup);
            continue;
        }

        if (nsubgroups != sgi) {
            ret->subgroups[nsubgroups] = ret->subgroups[sgi];
        }

        nsubgroups += 1;

    } /* end loop over subgroups */

    ret->nsubgroups = nsubgroups;
}

/**
 *  Delete a datastream from a retriever structure.
 *
 *  This function will delete the specified datastream and all references
 *  to it from a retriever structure.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  ret - pointer to the Retriever structure
 *  @param  ds  - pointer to the RetDataStream to delete.
 *
 *  @return
 *    - 1 if successful
 *    - 0 if one of the following errors occurred:
 *        - all input datastreams for a required variable were filtered
 *        - all input datastreams for a coordinate variable map were filtered
 *        - memory allocation error
 */
static int ret_delete_datastream(
    Retriever     *ret,
    RetDataStream *ds)
{
    RetCoordSystem *coord_system;
    RetCoordDim    *coord_dim;
    RetDsGroup     *group;
    RetDsSubGroup  *subgroup;
    RetVariable    *var;
    int             retval;
    int             ngroups, nds, nvars;
    int             gi, sgi, dsi, vi, csi, di;

    retval = 1;

    /************************************************************
    * Remove variable maps that reference this datastream
    * from all variables.
    ************************************************************/

    ngroups = 0;

    for (gi = 0; gi < ret->ngroups; gi++) {

        group = ret->groups[gi];
        nvars = 0;

        for (vi = 0; vi < group->nvars; vi++) {

            var = group->vars[vi];

            if (var->nvarmaps == 0) {
                ret_free_variable(var);
                continue;
            }
            else {

                ret_delete_ds_from_varmap_list(
                    ds, &(var->nvarmaps), var->varmaps);

                if (var->nvarmaps == 0) {

                    if (var->req_to_run == 1) {

                        ERROR( DSDB_LIB_NAME,
                            "All retriever datastreams were filtered for required variable: %s:%s\n",
                            group->name, var->name);

                        retval = 0;
                    }

                    ret_free_variable(var);
                    continue;
                }
            }

            if (nvars != vi) {
                group->vars[nvars] = group->vars[vi];
            }

            nvars += 1;

        } /* end loop over variables */

        group->nvars = nvars;

        if (group->nvars == 0) {
            ret_free_group(group);
            continue;
        }

        if (ngroups != gi) {
            ret->groups[ngroups] = ret->groups[gi];
        }

        ngroups += 1;

    } /* end loop over datastream groups */

    ret->ngroups = ngroups;

    /************************************************************
    * Remove the datastream from all subgroups.
    ************************************************************/

    for (sgi = 0; sgi < ret->nsubgroups; sgi++) {

        subgroup = ret->subgroups[sgi];
        nds      = 0;

        for (dsi = 0; dsi < subgroup->ndatastreams; dsi++) {

            if (subgroup->datastreams[dsi] == ds) {
                continue;
            }

            if (nds != dsi) {
                subgroup->datastreams[nds] = subgroup->datastreams[dsi];
            }

            nds++;
        }

        subgroup->ndatastreams = nds;

        if (subgroup->ndatastreams == 0) {
            ret_delete_subgroup(ret, subgroup);
        }

    } /* end loop over subgroups */

    /************************************************************
    * Remove variable maps that reference this datastream
    * from all coordinate dimensions.
    ************************************************************/

    for (csi = 0; csi < ret->ncoord_systems; csi++) {

        coord_system = ret->coord_systems[csi];

        for (di = 0; di < coord_system->ndims; di++) {

            coord_dim = coord_system->dims[di];

            if (coord_dim->nvarmaps > 0) {

                ret_delete_ds_from_varmap_list(
                    ds, &(coord_dim->nvarmaps), coord_dim->varmaps);

                if (coord_dim->nvarmaps == 0) {
                    free(coord_dim->varmaps);
                    coord_dim->varmaps = (RetDsVarMap **)NULL;
                }
            }

        } /* end loop over coordinate dimensions */
    } /* end loop over coordinate systems */

    /************************************************************
    * Delete the datastream.
    ************************************************************/

    nds = 0;

    for (dsi = 0; dsi < ret->ndatastreams; dsi++) {

        if (ret->datastreams[dsi] == ds) {
            ret_free_datastream(ds);
            continue;
        }

        if (nds != dsi) {
            ret->datastreams[nds] = ret->datastreams[dsi];
        }

        nds += 1;

    } /* end loop over subgroups */

    ret->ndatastreams = nds;

    return(retval);
}

/**
 *  Load the list of all variable names used to create coordinate system
 *  dimensions.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  dsdb - pointer to the open database connection
 *  @param  ret  - pointer to the Retriever structure
 *
 *  @return
 *    - number of rows returned by the database query
 *    - -1 if an error occurred
 */
static int ret_load_coordinate_var_names(
    DSDB      *dsdb,
    Retriever *ret)
{
    DBStatus        status;
    DBResult       *dbres;
    int             nrows;
    int             row;
    RetCoordSystem *coord_system;
    RetCoordDim    *coord_dim;
    RetDsVarMap    *varmap;
    char           *varname;
    char           *prev_id[2];
    char           *char_id[2];
    int             datastream_id;
    int             coord_dim_id;
    int             nnames;
    int             i, j;

    /* Get the list of all coordinate variable names from the database */

    status = retog_get_coord_var_names(dsdb->dbconn,
        ret->proc_type, ret->proc_name, &dbres);

    if (status != DB_NO_ERROR) {
        if (status == DB_NULL_RESULT) {
            return(0);
        }
        else {
            return(-1);
        }
    }

    /* Loop through all rows in the database query result */

    coord_dim = (RetCoordDim *)NULL;
    varmap    = (RetDsVarMap *)NULL;

    for (row = 0; row < dbres->nrows; row++) {

        coord_dim_id  = atoi(RetCoordDimVar_CoordDimID(dbres,row));
        datastream_id = atoi(RetCoordDimVar_DsID(dbres,row));

        /* Find this coordinate dimension */

        if (!coord_dim ||
             coord_dim->_id != coord_dim_id) {

            coord_dim = (RetCoordDim *)NULL;

            for (i = 0; i < ret->ncoord_systems; i++) {

                coord_system = ret->coord_systems[i];

                for (j = 0; j < coord_system->ndims; j++) {

                    if (coord_system->dims[j]->_id == coord_dim_id) {
                        coord_dim = coord_system->dims[j];
                        break;
                    }
                }

                if (coord_dim) break;
            }

            if (!coord_dim) {

                ERROR( DSDB_LIB_NAME,
                    "Could not load retriever coordinate variable names from database.\n"
                    " -> corrupt ret_coord_dims reference in ret_var_dim_names table\n");

                dbres->free(dbres);
                return(-1);
            }

            varmap = (RetDsVarMap *)NULL;
        }

        /* Find the variable map for this datastream */

        if (!varmap ||
            varmap->ds->_id != datastream_id) {

            varmap = (RetDsVarMap *)NULL;

            for (i = 0; i < coord_dim->nvarmaps; i++) {
                if (coord_dim->varmaps[i]->ds->_id == datastream_id) {
                    varmap = coord_dim->varmaps[i];
                    break;
                }
            }

            if (!varmap) {

                ERROR( DSDB_LIB_NAME,
                    "Could not load retriever coordinate variable names from database.\n"
                    " -> corrupt ret_datastreams reference in ret_var_dim_names table\n");

                dbres->free(dbres);
                return(-1);
            }
        }

        /* Create the varmap names list if this is the first one in the list */

        if (varmap->nnames == 0) {

            /* Count the number of names in the list */

            nnames     = 1;
            prev_id[0] = RetCoordDimVar_CoordDimID(dbres,row);
            prev_id[1] = RetCoordDimVar_DsID(dbres,row);

            for (i = row + 1; i < dbres->nrows; i++) {

                char_id[0] = RetCoordDimVar_CoordDimID(dbres,i);
                char_id[1] = RetCoordDimVar_DsID(dbres,i);

                if ((strcmp(char_id[0], prev_id[0]) != 0)  ||
                    (strcmp(char_id[1], prev_id[1]) != 0) ) {

                    break;
                }

                nnames++;
            }

            varmap->names = (const char **)calloc(nnames + 1, sizeof(char *));
            if (!varmap->names) {
                goto MEMORY_ALLOCATION_ERROR;
            }
        }

        /* Add the variable name to the map */

        varname = strdup(RetCoordDimVar_Name(dbres,row));

        if (!varname) {
            goto MEMORY_ALLOCATION_ERROR;
        }

        varmap->names[varmap->nnames] = varname;
        varmap->nnames += 1;

    } /* end loop over all rows in the database query result */

    nrows = dbres->nrows;

    dbres->free(dbres);

    return(nrows);

MEMORY_ALLOCATION_ERROR:

    ERROR( DSDB_LIB_NAME,
        "Could not load retriever coordinate variable names from database.\n"
        " -> memory allocation error\n");

    dbres->free(dbres);
    return(-1);
}

/**
 *  Load the list of all coordinate system dimensions.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  dsdb - pointer to the open database connection
 *  @param  ret  - pointer to the Retriever structure
 *
 *  @return
 *    - number of rows returned by the database query
 *    - -1 if an error occurred
 */
static int ret_load_coordinate_dims(
    DSDB      *dsdb,
    Retriever *ret)
{
    DBStatus        status;
    DBResult       *dbres;
    int             nrows;
    int             row;
    RetCoordSystem *coord_system;
    RetCoordDim    *coord_dim;
    RetDsSubGroup  *subgroup;
    int             coord_system_id;
    int             coord_dim_id;
    int             subgroup_id;
    int             ncoord_dims;
    char           *prev_id;
    char           *char_id;
    char           *value;
    int             found_var_map;
    int             i;

    /* Get the list of all coordinate system dimensions from the database */

    status = retog_get_coord_dims(dsdb->dbconn,
        ret->proc_type, ret->proc_name, &dbres);

    if (status != DB_NO_ERROR) {
        if (status == DB_NULL_RESULT) {
            return(0);
        }
        else {
            return(-1);
        }
    }

    /* Loop through all rows in the database query result */

    found_var_map = 0;
    coord_system  = (RetCoordSystem *)NULL;

    for (row = 0; row < dbres->nrows; row++) {

        coord_system_id = atoi(RetCoordDim_SystemID(dbres,row));
        coord_dim_id    = atoi(RetCoordDim_ID(dbres,row));

        /* Find this coordinate system */

        if (!coord_system ||
            coord_system->_id != coord_system_id) {

            coord_system = (RetCoordSystem *)NULL;

            for (i = 0; i < ret->ncoord_systems; i++) {
                if (ret->coord_systems[i]->_id == coord_system_id) {
                    coord_system = ret->coord_systems[i];
                    break;
                }
            }

            if (!coord_system) {

                ERROR( DSDB_LIB_NAME,
                    "Could not load retriever coordinate system dimensions from database.\n"
                    " -> corrupt ret_coord_systems reference in ret_coord_dims table\n");

                dbres->free(dbres);
                return(-1);
            }

            /* Count the number of dimensions in this coordinate system */

            ncoord_dims = 1;
            prev_id     = RetCoordDim_SystemID(dbres,row);

            for (i = row + 1; i < dbres->nrows; i++) {

                char_id = RetCoordDim_SystemID(dbres, i);

                if (strcmp(char_id, prev_id) != 0) {
                    break;
                }

                ncoord_dims++;
            }

            /* Allocate memory for the list of dims in this coordinate system */

            coord_system->dims = (RetCoordDim **)calloc(
                ncoord_dims + 1, sizeof(RetCoordDim *));

            if (!coord_system->dims) {
                goto MEMORY_ALLOCATION_ERROR;
            }

        } /* end create new coordinate system */

        /* Create the new coordinate dimension structure */

        coord_dim = (RetCoordDim *)calloc(1, sizeof(RetCoordDim));
        if (!coord_dim) {
            goto MEMORY_ALLOCATION_ERROR;
        }

        /* Add the new dimension structure to the coordinate system */

        coord_system->dims[coord_system->ndims] = coord_dim;
        coord_system->ndims += 1;

        /* Set the coordinate dimension structure values */

        coord_dim->_id  = coord_dim_id;
        coord_dim->name = strdup(RetCoordDim_Name(dbres,row));

        if (!coord_dim->name) {
            goto MEMORY_ALLOCATION_ERROR;
        }

        /* Set data type if not NULL */

        value = RetCoordDim_DataType(dbres,row);
        if (value && value[0] != '\0') {
            coord_dim->data_type = strdup(value);
            if (!coord_dim->data_type) {
                goto MEMORY_ALLOCATION_ERROR;
            }
        }

        /* Set units if not NULL */

        value = RetCoordDim_Units(dbres,row);
        if (value && value[0] != '\0') {
            coord_dim->units = strdup(value);
            if (!coord_dim->units) {
                goto MEMORY_ALLOCATION_ERROR;
            }
        }

        /* Set start value if not NULL */

        value = RetCoordDim_Start(dbres,row);
        if (value && value[0] != '\0') {
            coord_dim->start = strdup(value);
            if (!coord_dim->start) {
                goto MEMORY_ALLOCATION_ERROR;
            }
        }

        /* Set interval if not NULL */

        value = RetCoordDim_Interval(dbres,row);
        if (value && value[0] != '\0') {
            coord_dim->interval = strdup(value);
            if (!coord_dim->interval) {
                goto MEMORY_ALLOCATION_ERROR;
            }
        }

        /* Set length if not NULL */

        value = RetCoordDim_Length(dbres,row);
        if (value && value[0] != '\0') {
            coord_dim->length = strdup(value);
            if (!coord_dim->length) {
                goto MEMORY_ALLOCATION_ERROR;
            }
        }

        /* Set transformation type if not NULL */

        value = RetCoordDim_TransType(dbres,row);
        if (value && value[0] != '\0') {
            coord_dim->trans_type = strdup(value);
            if (!coord_dim->trans_type) {
                goto MEMORY_ALLOCATION_ERROR;
            }
        }

        /* Set transformation range if not NULL */

        value = RetCoordDim_TransRange(dbres,row);
        if (value && value[0] != '\0') {
            coord_dim->trans_range = strdup(value);
            if (!coord_dim->trans_range) {
                goto MEMORY_ALLOCATION_ERROR;
            }
        }

        /* Set transformation alignment if not NULL */

        value = RetCoordDim_TransAlign(dbres,row);
        if (value && value[0] != '\0') {
            coord_dim->trans_align = strdup(value);
            if (!coord_dim->trans_align) {
                goto MEMORY_ALLOCATION_ERROR;
            }
        }

        /* Create the list of variable maps and set the datastream references
         * if the subgroup reference is not NULL. The variable name lists will
         * be created later by the call to ret_load_coordinate_var_names(). */

        subgroup = (RetDsSubGroup *)NULL;

        value = RetCoordDim_SubGroupID(dbres,row);
        if (value && value[0] != '\0') {
            subgroup_id = atoi(value);
            for (i = 0; i < ret->nsubgroups; i++) {
                if (ret->subgroups[i]->_id == subgroup_id) {
                    subgroup = ret->subgroups[i];
                    break;
                }
            }
        }

        if (subgroup) {

            coord_dim->varmaps = (RetDsVarMap **)
                calloc(subgroup->ndatastreams + 1, sizeof(RetDsVarMap *));

            if (!coord_dim->varmaps) {
                goto MEMORY_ALLOCATION_ERROR;
            }

            for (i = 0; i < subgroup->ndatastreams; i++) {

                coord_dim->varmaps[i] = (RetDsVarMap *)
                    calloc(1, sizeof(RetDsVarMap));

                if (!coord_dim->varmaps[i]) {
                    goto MEMORY_ALLOCATION_ERROR;
                }

                coord_dim->varmaps[i]->ds = subgroup->datastreams[i];

                coord_dim->nvarmaps += 1;
            }

            found_var_map = 1;
        }

    } /* end loop over all rows in the database query result */

    nrows = dbres->nrows;

    dbres->free(dbres);

    /* Load the coordinate variable names that map to the dimensions */

    if (found_var_map) {
        if (ret_load_coordinate_var_names(dsdb, ret) < 0) {
            return(-1);
        }
    }

    return(nrows);

MEMORY_ALLOCATION_ERROR:

    ERROR( DSDB_LIB_NAME,
        "Could not load retriever coordinate system dimensions from database.\n"
        " -> memory allocation error\n");

    dbres->free(dbres);
    return(-1);
}

/**
 *  Load the list of all coordinate systems.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  dsdb - pointer to the open database connection
 *  @param  ret  - pointer to the Retriever structure
 *
 *  @return
 *    - number of rows returned by the database query
 *    - -1 if an error occurred
 */
static int ret_load_coordinate_systems(
    DSDB      *dsdb,
    Retriever *ret)
{
    DBStatus        status;
    DBResult       *dbres;
    int             nrows;
    int             row;
    RetCoordSystem *coord_system;
    int             coord_system_id;

    /* Get the list of all coordinate systems from the database */

    status = retog_get_coord_systems(dsdb->dbconn,
        ret->proc_type, ret->proc_name, &dbres);

    if (status != DB_NO_ERROR) {
        if (status == DB_NULL_RESULT) {
            return(0);
        }
        else {
            return(-1);
        }
    }

    /* Allocate memory for the list of coordinate systems */

    ret->coord_systems = (RetCoordSystem **)
        calloc(dbres->nrows + 1, sizeof(RetCoordSystem *));

    if (!ret->coord_systems) {
        goto MEMORY_ALLOCATION_ERROR;
    }

    /* Loop through all rows in the database query result */

    for (row = 0; row < dbres->nrows; row++) {

        coord_system_id = atoi(RetCoordSystem_ID(dbres,row));

        /* Create the new coordinate system structure */

        coord_system = (RetCoordSystem *)calloc(1, sizeof(RetCoordSystem));
        if (!coord_system) {
            goto MEMORY_ALLOCATION_ERROR;
        }

        /* Add the new coordinate system structure to the list */

        ret->coord_systems[ret->ncoord_systems] = coord_system;
        ret->ncoord_systems += 1;

        /* Set the coordinate system structure values */

        coord_system->_id  = coord_system_id;
        coord_system->name = strdup(RetCoordSystem_Name(dbres,row));

        if (!coord_system->name) {
            goto MEMORY_ALLOCATION_ERROR;
        }

    } /* end loop over all rows in the database query result */

    nrows = dbres->nrows;

    dbres->free(dbres);

    /* Load the coordinate system dimensions */

    if (ret_load_coordinate_dims(dsdb, ret) < 0) {
        return(-1);
    }

    return(nrows);

MEMORY_ALLOCATION_ERROR:

    ERROR( DSDB_LIB_NAME,
        "Could not load retriever coordinate systems from database.\n"
        " -> memory allocation error\n");

    dbres->free(dbres);
    return(-1);
}

/**
 *  Load the list of all datastreams.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  dsdb - pointer to the open database connection
 *  @param  ret  - pointer to the Retriever structure
 *
 *  @return
 *    - number of rows returned by the database query
 *    - -1 if an error occurred
 */
static int ret_load_datastreams(
    DSDB      *dsdb,
    Retriever *ret)
{
    DBStatus       status;
    DBResult      *dbres;
    int            nrows;
    int            row;
    RetDsSubGroup *subgroup;
    RetDataStream *datastream;
    int            subgroup_id;
    int            datastream_id;
    int            ndatastreams;
    char          *prev_id;
    char          *char_id;
    char          *value;
    timeval_t      tv_time;
    int            i;

    /* Get the list of all datastreams from the database */

    status = retog_get_datastreams(dsdb->dbconn,
        ret->proc_type, ret->proc_name, &dbres);

    if (status != DB_NO_ERROR) {
        if (status == DB_NULL_RESULT) {
            return(0);
        }
        else {
            return(-1);
        }
    }

    /* Allocate memory for the list of datastreams */

    ret->datastreams = (RetDataStream **)
        calloc(dbres->nrows + 1, sizeof(RetDataStream *));

    if (!ret->datastreams) {
        goto MEMORY_ALLOCATION_ERROR;
    }

    /* Loop through all rows in the database query result */

    subgroup = (RetDsSubGroup *)NULL;

    for (row = 0; row < dbres->nrows; row++) {

        subgroup_id   = atoi(RetDs_SubGroupID(dbres,row));
        datastream_id = atoi(RetDs_DsID(dbres,row));

        /* Check if this is a new datastream */

        datastream = (RetDataStream *)NULL;

        for (i = 0; i < ret->ndatastreams; i++) {
            if (datastream_id == ret->datastreams[i]->_id) {
                datastream = ret->datastreams[i];
                break;
            }
        }

        if (!datastream) {

            /* Create the new datastream structure */

            datastream = (RetDataStream *)calloc(1, sizeof(RetDataStream));
            if (!datastream) {
                goto MEMORY_ALLOCATION_ERROR;
            }

            /* Add the new datastream structure to the list */

            ret->datastreams[ret->ndatastreams] = datastream;
            ret->ndatastreams += 1;

            /* Set the datastream structure values */

            datastream->_id   = datastream_id;
            datastream->name  = strdup(RetDs_Name(dbres,row));
            datastream->level = strdup(RetDs_Level(dbres,row));

            if (!datastream->name || !datastream->level) {
                goto MEMORY_ALLOCATION_ERROR;
            }

            /* Set site name if not NULL */

            value = RetDs_Site(dbres,row);
            if (value && value[0] != '\0') {
                datastream->site = strdup(value);
                if (!datastream->site) {
                    goto MEMORY_ALLOCATION_ERROR;
                }
            }

            /* Set facility name if not NULL */

            value = RetDs_Fac(dbres,row);
            if (value && value[0] != '\0') {
                datastream->facility = strdup(value);
                if (!datastream->facility) {
                    goto MEMORY_ALLOCATION_ERROR;
                }
            }

            /* Set site dependency if not NULL */

            value = RetDs_SiteDep(dbres,row);
            if (value && value[0] != '\0') {
                datastream->dep_site = strdup(value);
                if (!datastream->dep_site) {
                    goto MEMORY_ALLOCATION_ERROR;
                }
            }

            /* Set facility dependency if not NULL */

            value = RetDs_FacDep(dbres,row);
            if (value && value[0] != '\0') {
                datastream->dep_fac = strdup(value);
                if (!datastream->dep_fac) {
                    goto MEMORY_ALLOCATION_ERROR;
                }
            }

            /* Set begin date dependency if not NULL */

            value = RetDs_BegDateDep(dbres,row);
            if (value && value[0] != '\0') {
                dsdb_text_to_timeval(dsdb, value, &tv_time);
                datastream->dep_begin_date = tv_time.tv_sec;
            }

            /* Set end date dependency if not NULL */

            value = RetDs_EndDateDep(dbres,row);
            if (value && value[0] != '\0') {
                dsdb_text_to_timeval(dsdb, value, &tv_time);
                datastream->dep_end_date = tv_time.tv_sec;
            }

        } /* end create new datastream */

        /* Get the subgroup this datastream belongs to */

        if (!subgroup ||
            subgroup->_id != subgroup_id) {

            subgroup = (RetDsSubGroup *)NULL;

            for (i = 0; i < ret->nsubgroups; i++) {
                if (ret->subgroups[i]->_id == subgroup_id) {
                    subgroup = ret->subgroups[i];
                    break;
                }
            }

            if (!subgroup) {

                ERROR( DSDB_LIB_NAME,
                    "Could not load retriever datastreams from database.\n"
                    " -> corrupt ret_ds_subgroups reference in ret_datastreams table\n");

                dbres->free(dbres);
                return(-1);
            }

            /* Count the number of datastreams in this subgroup */

            ndatastreams = 1;
            prev_id      = RetDs_SubGroupID(dbres,row);

            for (i = row + 1; i < dbres->nrows; i++) {

                char_id = RetDs_SubGroupID(dbres, i);

                if (strcmp(char_id, prev_id) != 0) {
                    break;
                }

                ndatastreams++;
            }

            /* Allocate memory for the list of datastreams in this subgroup */

            subgroup->datastreams = (RetDataStream **)calloc(
                ndatastreams + 1, sizeof(RetDataStream *));

            if (!subgroup->datastreams) {
                goto MEMORY_ALLOCATION_ERROR;
            }
        }

        /* Add the datastream to the subgroup */

        subgroup->datastreams[subgroup->ndatastreams] = datastream;
        subgroup->ndatastreams += 1;

    } /* end loop over all rows in the database query result */

    nrows = dbres->nrows;

    dbres->free(dbres);

    return(nrows);

MEMORY_ALLOCATION_ERROR:

    ERROR( DSDB_LIB_NAME,
        "Could not load retriever datastreams from database.\n"
        " -> memory allocation error\n");

    dbres->free(dbres);
    return(-1);
}

/**
 *  Load the lists of all datastream groups and subgroups.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  dsdb - pointer to the open database connection
 *  @param  ret  - pointer to the Retriever structure
 *
 *  @return
 *    - number of rows returned by the database query
 *    - -1 if an error occurred
 */
static int ret_load_groups_and_subgroups(
    DSDB      *dsdb,
    Retriever *ret)
{
    DBStatus       status;
    DBResult      *dbres;
    int            nrows;
    int            row;
    RetDsGroup    *group;
    RetDsSubGroup *subgroup;
    int            group_id;
    int            subgroup_id;
    int            nsubgroups;
    char          *prev_id;
    char          *char_id;
    int            i;

    /* Get the list of all datastream groups and subgroups from the database */

    status = retog_get_groups(dsdb->dbconn,
        ret->proc_type, ret->proc_name, &dbres);

    if (status != DB_NO_ERROR) {
        if (status == DB_NULL_RESULT) {
            return(0);
        }
        else {
            return(-1);
        }
    }

    /* Allocate memory for the list of datastream groups */

    ret->groups = (RetDsGroup **)
        calloc(dbres->nrows + 1, sizeof(RetDsGroup *));

    if (!ret->groups) {
        goto MEMORY_ALLOCATION_ERROR;
    }

    /* Allocate memory for the list of datastream subgroups */

    ret->subgroups = (RetDsSubGroup **)
        calloc(dbres->nrows + 1, sizeof(RetDsSubGroup *));

    if (!ret->subgroups) {
        goto MEMORY_ALLOCATION_ERROR;
    }

    /* Loop through all rows in the database query result */

    group = (RetDsGroup *)NULL;

    for (row = 0; row < dbres->nrows; row++) {

        group_id    = atoi(Ret_GroupID(dbres,row));
        subgroup_id = atoi(Ret_SubGroupID(dbres,row));

        /* Check if this is a new datastream subgroup */

        subgroup = (RetDsSubGroup *)NULL;

        for (i = 0; i < ret->nsubgroups; i++) {
            if (subgroup_id == ret->subgroups[i]->_id) {
                subgroup = ret->subgroups[i];
                break;
            }
        }

        if (!subgroup) {

            /* Create the new datastream subgroup structure */

            subgroup = (RetDsSubGroup *)calloc(1, sizeof(RetDsSubGroup));
            if (!subgroup) {
                goto MEMORY_ALLOCATION_ERROR;
            }

            /* Add the new datastream subgroup to the list */

            ret->subgroups[ret->nsubgroups] = subgroup;
            ret->nsubgroups += 1;

            /* Set the datastream subgroup structure values */

            subgroup->_id  = atoi(Ret_SubGroupID(dbres,row));
            subgroup->name = strdup(Ret_SubGroupName(dbres,row));

            if (!subgroup->name) {
                goto MEMORY_ALLOCATION_ERROR;
            }

        } /* end create new datastream subgroup */

        /* Check if this is a new datastream group */

        if (!group ||
            group->_id != group_id) {

            /* Create the new datastream group structure */

            group = (RetDsGroup *)calloc(1, sizeof(RetDsGroup));
            if (!group) {
                goto MEMORY_ALLOCATION_ERROR;
            }

            /* Add the new datastream group to the list */

            ret->groups[ret->ngroups] = group;
            ret->ngroups += 1;

            /* Set the datastream group structure values */

            group->_id  = group_id;
            group->name = strdup(Ret_GroupName(dbres,row));

            if (!group->name) {
                goto MEMORY_ALLOCATION_ERROR;
            }

            /* Count the number of datastream subgroups in this group */

            nsubgroups = 1;
            prev_id    = Ret_GroupID(dbres,row);

            for (i = row + 1; i < dbres->nrows; i++) {

                char_id = Ret_GroupID(dbres, i);

                if (strcmp(char_id, prev_id) != 0) {
                    break;
                }

                nsubgroups++;
            }

            /* Allocate memory for the list of datastream subgroups */

            group->subgroups = (RetDsSubGroup **)calloc(
                nsubgroups + 1, sizeof(RetDsSubGroup *));

            if (!group->subgroups) {
                goto MEMORY_ALLOCATION_ERROR;
            }

        } /* end create new datastream group */

        /* Add the datastream subgroup to the group */

        group->subgroups[group->nsubgroups] = subgroup;
        group->nsubgroups += 1;

    } /* end loop over all rows in the database query result */

    nrows = dbres->nrows;

    dbres->free(dbres);

    return(nrows);

MEMORY_ALLOCATION_ERROR:

    ERROR( DSDB_LIB_NAME,
        "Could not load retriever groups and subgroups from database.\n"
        " -> memory allocation error\n");

    dbres->free(dbres);
    return(-1);
}

/**
 *  Load the list of all transformation parameters.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  dsdb - pointer to the open database connection
 *  @param  ret  - pointer to the Retriever structure
 *
 *  @return
 *    - number of rows returned by the database query
 *    - -1 if an error occurred
 */
static int ret_load_trans_params(
    DSDB      *dsdb,
    Retriever *ret)
{
    DBStatus        status;
    DBResult       *dbres;
    int             nrows;
    int             row;
    RetTransParams *trans_params;

    /* Get the list of all transformation params from the database */

    status = retog_get_trans_params(dsdb->dbconn,
        ret->proc_type, ret->proc_name, &dbres);

    if (status != DB_NO_ERROR) {
        if (status == DB_NULL_RESULT) {
            return(0);
        }
        else {
            return(-1);
        }
    }

    /* Allocate memory for the list of coordinate systems */

    ret->trans_params = (RetTransParams **)
        calloc(dbres->nrows + 1, sizeof(RetTransParams *));

    if (!ret->trans_params) {
        goto MEMORY_ALLOCATION_ERROR;
    }

    /* Loop through all rows in the database query result */

    for (row = 0; row < dbres->nrows; row++) {

        /* Create the new RetTransParams structure */

        trans_params = (RetTransParams *)calloc(1, sizeof(RetTransParams));
        if (!trans_params) {
            goto MEMORY_ALLOCATION_ERROR;
        }

        /* Add the new RetTransParams structure to the list */

        ret->trans_params[ret->ntrans_params] = trans_params;
        ret->ntrans_params += 1;

        /* Set the RetTransParams structure values */

        trans_params->coordsys = strdup(RetTransParams_Coordsys(dbres,row));
        trans_params->params   = strdup(RetTransParams_Params(dbres,row));

        if (!trans_params->coordsys ||
            !trans_params->params) {

            goto MEMORY_ALLOCATION_ERROR;
        }

    } /* end loop over all rows in the database query result */

    nrows = dbres->nrows;

    dbres->free(dbres);

    return(nrows);

MEMORY_ALLOCATION_ERROR:

    ERROR( DSDB_LIB_NAME,
        "Could not load retriever transformation params from database.\n"
        " -> memory allocation error\n");

    dbres->free(dbres);
    return(-1);
}

/**
 *  Load the lists of all variable dimension names.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  dsdb - pointer to the open database connection
 *  @param  ret  - pointer to the Retriever structure
 *
 *  @return
 *    - number of rows returned by the database query
 *    - -1 if an error occurred
 */
static int ret_load_var_dims(
    DSDB      *dsdb,
    Retriever *ret)
{
    DBStatus       status;
    DBResult      *dbres;
    int            nrows;
    int            row;
    RetDsGroup    *group;
    RetVariable   *var;
    int            var_id;
    int            ndim_names;
    char          *dim_name;
    char          *prev_id;
    char          *char_id;
    int            i, j;

    /* Get the list of all variable dimensions from the database */

    status = retog_get_var_dims(dsdb->dbconn,
        ret->proc_type, ret->proc_name, &dbres);

    if (status != DB_NO_ERROR) {
        if (status == DB_NULL_RESULT) {
            return(0);
        }
        else {
            return(-1);
        }
    }

    /* Loop through all rows in the database query result */

    var = (RetVariable *)NULL;

    for (row = 0; row < dbres->nrows; row++) {

        var_id = atoi(RetVarDim_VarID(dbres,row));

        /* Find this variable */

        if (!var ||
             var->_id != var_id) {

            var = (RetVariable *)NULL;

            for (i = 0; i < ret->ngroups; i++) {

                group = ret->groups[i];

                for (j = 0; j < group->nvars; j++) {

                    if (group->vars[j]->_id == var_id) {
                        var = group->vars[j];
                        break;
                    }
                }

                if (var) break;
            }

            if (!var) {

                ERROR( DSDB_LIB_NAME,
                    "Could not load retriever variable dimension names from database.\n"
                    " -> corrupt ret_var_groups reference in ret_var_dims table\n");

                dbres->free(dbres);
                return(-1);
            }

            /* Count the number of dimension names for this variable */

            ndim_names = 1;
            prev_id    = RetVarDim_VarID(dbres,row);

            for (i = row + 1; i < dbres->nrows; i++) {

                char_id = RetVarDim_VarID(dbres,i);

                if (strcmp(char_id, prev_id) != 0) {
                    break;
                }

                ndim_names++;
            }

            /* Allocate memory for the list of dimension names */

            var->dim_names = (const char **)
                calloc(ndim_names + 1, sizeof(const char *));

            if (!var->dim_names) {
                goto MEMORY_ALLOCATION_ERROR;
            }

        } /* end find variable */

        /* Add the dimension name to the list */

        dim_name = strdup(RetVarDim_Name(dbres,row));

        if (!dim_name) {
            goto MEMORY_ALLOCATION_ERROR;
        }

        var->dim_names[var->ndim_names] = dim_name;
        var->ndim_names += 1;

    } /* end loop over all rows in the database query result */

    nrows = dbres->nrows;

    dbres->free(dbres);

    return(nrows);

MEMORY_ALLOCATION_ERROR:

    ERROR( DSDB_LIB_NAME,
        "Could not load retriever variable dimension names from database.\n"
        " -> memory allocation error\n");

    dbres->free(dbres);
    return(-1);
}

/**
 *  Load the lists of all input datastream variable names.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  dsdb - pointer to the open database connection
 *  @param  ret  - pointer to the Retriever structure
 *
 *  @return
 *    - number of rows returned by the database query
 *    - -1 if an error occurred
 */
static int ret_load_var_names(
    DSDB      *dsdb,
    Retriever *ret)
{
    DBStatus        status;
    DBResult       *dbres;
    int             nrows;
    int             row;
    RetDsGroup     *group;
    RetVariable    *var;
    RetDsVarMap    *varmap;
    char           *varname;
    char           *prev_id[2];
    char           *char_id[2];
    int             var_id;
    int             datastream_id;
    int             nnames;
    int             i, j;

    /* Get the list of all input datastream variable names */

    status = retog_get_var_names(dsdb->dbconn,
        ret->proc_type, ret->proc_name, &dbres);

    if (status != DB_NO_ERROR) {
        if (status == DB_NULL_RESULT) {
            return(0);
        }
        else {
            return(-1);
        }
    }

    /* Loop through all rows in the database query result */

    var    = (RetVariable *)NULL;
    varmap = (RetDsVarMap *)NULL;

    for (row = 0; row < dbres->nrows; row++) {

        var_id        = atoi(RetVarName_VarID(dbres,row));
        datastream_id = atoi(RetVarName_DsID(dbres,row));

        /* Find this variable */

        if (!var ||
             var->_id != var_id) {

            var = (RetVariable *)NULL;

            for (i = 0; i < ret->ngroups; i++) {

                group = ret->groups[i];

                for (j = 0; j < group->nvars; j++) {

                    if (group->vars[j]->_id == var_id) {
                        var = group->vars[j];
                        break;
                    }
                }

                if (var) break;
            }

            if (!var) {

                ERROR( DSDB_LIB_NAME,
                    "Could not load retriever datastream variable names from database.\n"
                    " -> corrupt ret_var_groups reference in ret_var_names table\n");

                dbres->free(dbres);
                return(-1);
            }

            varmap = (RetDsVarMap *)NULL;
        }

        /* Find the variable map for this datastream */

        if (!varmap ||
            varmap->ds->_id != datastream_id) {

            varmap = (RetDsVarMap *)NULL;

            for (i = 0; i < var->nvarmaps; i++) {
                if (var->varmaps[i]->ds->_id == datastream_id) {
                    varmap = var->varmaps[i];
                    break;
                }
            }

            if (!varmap) {

                ERROR( DSDB_LIB_NAME,
                    "Could not load retriever datastream variable names from database.\n"
                    " -> corrupt ret_datastreams reference in ret_var_names table\n");

                dbres->free(dbres);
                return(-1);
            }
        }

        /* Create the varmap names list if this is the first one in the list */

        if (varmap->nnames == 0) {

            /* Count the number of names in the list */

            nnames     = 1;
            prev_id[0] = RetVarName_VarID(dbres,row);
            prev_id[1] = RetVarName_DsID(dbres,row);

            for (i = row + 1; i < dbres->nrows; i++) {

                char_id[0] = RetVarName_VarID(dbres,i);
                char_id[1] = RetVarName_DsID(dbres,i);

                if ((strcmp(char_id[0], prev_id[0]) != 0)  ||
                    (strcmp(char_id[1], prev_id[1]) != 0) ) {

                    break;
                }

                nnames++;
            }

            varmap->names = (const char **)calloc(nnames + 1, sizeof(char *));
            if (!varmap->names) {
                goto MEMORY_ALLOCATION_ERROR;
            }
        }

        /* Add the variable name to the map */

        varname = strdup(RetVarName_Name(dbres,row));

        if (!varname) {
            goto MEMORY_ALLOCATION_ERROR;
        }

        varmap->names[varmap->nnames] = varname;
        varmap->nnames += 1;

    } /* end loop over all rows in the database query result */

    nrows = dbres->nrows;

    dbres->free(dbres);

    return(nrows);

MEMORY_ALLOCATION_ERROR:

    ERROR( DSDB_LIB_NAME,
        "Could not load retriever datastream variable names from database.\n"
        " -> memory allocation error\n");

    dbres->free(dbres);
    return(-1);
}

/**
 *  Load the lists of all output datastreams and variable names.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  dsdb - pointer to the open database connection
 *  @param  ret  - pointer to the Retriever structure
 *
 *  @return
 *    - number of rows returned by the database query
 *    - -1 if an error occurred
 */
static int ret_load_var_outputs(
    DSDB      *dsdb,
    Retriever *ret)
{
    DBStatus       status;
    DBResult      *dbres;
    int            nrows;
    int            row;
    RetDsGroup    *group;
    RetVariable   *var;
    int            noutputs;
    RetVarOutput  *output;
    int            var_id;
    char          *prev_id;
    char          *char_id;
    int            i, j;

    /* Get the list of all variable output targets from the database */

    status = retog_get_var_outputs(dsdb->dbconn,
        ret->proc_type, ret->proc_name, &dbres);

    if (status != DB_NO_ERROR) {
        if (status == DB_NULL_RESULT) {
            return(0);
        }
        else {
            return(-1);
        }
    }

    /* Loop through all rows in the database query result */

    var = (RetVariable *)NULL;

    for (row = 0; row < dbres->nrows; row++) {

        var_id = atoi(RetVarOut_VarID(dbres,row));

        /* Find this variable */

        if (!var ||
             var->_id != var_id) {

            var = (RetVariable *)NULL;

            for (i = 0; i < ret->ngroups; i++) {

                group = ret->groups[i];

                for (j = 0; j < group->nvars; j++) {

                    if (group->vars[j]->_id == var_id) {
                        var = group->vars[j];
                        break;
                    }
                }

                if (var) break;
            }

            if (!var) {

                ERROR( DSDB_LIB_NAME,
                    "Could not load retriever variable output targets from database.\n"
                    " -> corrupt ret_var_groups reference in ret_var_outputs table\n");

                dbres->free(dbres);
                return(-1);
            }

            /* Count the number of output targets for this variable */

            noutputs = 1;
            prev_id  = RetVarOut_VarID(dbres,row);

            for (i = row + 1; i < dbres->nrows; i++) {

                char_id = RetVarOut_VarID(dbres,i);

                if (strcmp(char_id, prev_id) != 0) {
                    break;
                }

                noutputs++;
            }

            /* Allocate memory for the list of output targets */

            var->outputs = (RetVarOutput **)
                calloc(noutputs + 1, sizeof(RetVarOutput *));

            if (!var->outputs) {
                goto MEMORY_ALLOCATION_ERROR;
            }

        } /* end find variable */

        /* Create the new variable output structure */

        output = (RetVarOutput *)calloc(1, sizeof(RetVarOutput));
        if (!output) {
            goto MEMORY_ALLOCATION_ERROR;
        }

        /* Add the new variable output structure to the variable */

        var->outputs[var->noutputs] = output;
        var->noutputs += 1;

        /* Set the variable output structure values */

        output->dsc_name  = strdup(RetVarOut_DsName(dbres,row));
        output->dsc_level = strdup(RetVarOut_DsLevel(dbres,row));
        output->var_name  = strdup(RetVarOut_VarName(dbres,row));

        if (!output->dsc_name  ||
            !output->dsc_level ||
            !output->var_name) {

            goto MEMORY_ALLOCATION_ERROR;
        }

    } /* end loop over all rows in the database query result */

    nrows = dbres->nrows;

    dbres->free(dbres);

    return(nrows);

MEMORY_ALLOCATION_ERROR:

    ERROR( DSDB_LIB_NAME,
        "Could not load retriever variable output targets from database.\n"
        " -> memory allocation error\n");

    dbres->free(dbres);
    return(-1);
}

/**
 *  Load the lists of all variables.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  dsdb - pointer to the open database connection
 *  @param  ret  - pointer to the Retriever structure
 *
 *  @return
 *    - number of rows returned by the database query
 *    - -1 if an error occurred
 */
static int ret_load_variables(
    DSDB      *dsdb,
    Retriever *ret)
{
    DBStatus       status;
    DBResult      *dbres;
    int            nrows;
    int            row;
    RetDsGroup    *group;
    RetDsSubGroup *subgroup;
    RetVariable   *var;
    RetDsVarMap   *varmap;
    int            coord_system_id;
    int            group_id;
    int            var_id;
    int            nvars;
    int            nvarmaps;
    char          *prev_id;
    char          *char_id;
    char          *value;
    int            i, j;

    /* Get the list of all variable groups from the database */

    status = retog_get_variables(dsdb->dbconn,
        ret->proc_type, ret->proc_name, &dbres);

    if (status != DB_NO_ERROR) {
        if (status == DB_NULL_RESULT) {
            return(0);
        }
        else {
            return(-1);
        }
    }

    /* Loop through all rows in the database query result */

    group = (RetDsGroup *)NULL;

    for (row = 0; row < dbres->nrows; row++) {

        var_id   = atoi(RetVar_VarID(dbres,row));
        group_id = atoi(RetVar_GroupID(dbres,row));

        /* Find the datastream group this variable belongs to */

        if (!group ||
            group->_id != group_id) {

            group = (RetDsGroup *)NULL;

            for (i = 0; i < ret->ngroups; i++) {
                if (ret->groups[i]->_id == group_id) {
                    group = ret->groups[i];
                    break;
                }
            }

            if (!group) {

                ERROR( DSDB_LIB_NAME,
                    "Could not load retriever variables from database.\n"
                    " -> corrupt ret_ds_groups reference in ret_var_groups table\n");

                dbres->free(dbres);
                return(-1);
            }

            /* Count the number of variables in this group */

            nvars   = 1;
            prev_id = RetVar_GroupID(dbres,row);

            for (i = row + 1; i < dbres->nrows; i++) {

                char_id = RetVar_GroupID(dbres,i);

                if (strcmp(char_id, prev_id) != 0) {
                    break;
                }

                nvars++;
            }

            /* Allocate memory for the list of variables */

            group->vars = (RetVariable **)calloc(
                nvars + 1, sizeof(RetVariable *));

            if (!group->vars) {
                goto MEMORY_ALLOCATION_ERROR;
            }

        } /* end find datastream group */

        /* Create the new variable structure */

        var = (RetVariable *)calloc(1, sizeof(RetVariable));
        if (!var) {
            goto MEMORY_ALLOCATION_ERROR;
        }

        /* Add the new variable structure to the datastream group */

        group->vars[group->nvars] = var;
        group->nvars += 1;

        /* Set the variable structure values */

        var->_id  = var_id;
        var->name = strdup(RetVar_Name(dbres,row));

        if (!var->name) {
            goto MEMORY_ALLOCATION_ERROR;
        }

        /* Set data type if not NULL */

        value = RetVar_DataType(dbres,row);
        if (value && value[0] != '\0') {
            var->data_type = strdup(value);
            if (!var->data_type) {
                goto MEMORY_ALLOCATION_ERROR;
            }
        }

        /* Set units if not NULL */

        value = RetVar_Units(dbres,row);
        if (value && value[0] != '\0') {
            var->units = strdup(value);
            if (!var->units) {
                goto MEMORY_ALLOCATION_ERROR;
            }
        }

        /* Set start offset if not NULL */

        value = RetVar_StartOffset(dbres,row);
        if (value && value[0] != '\0') {
            var->start_offset = (time_t)atoi(value);
        }

        /* Set end offset if not NULL */

        value = RetVar_EndOffset(dbres,row);
        if (value && value[0] != '\0') {
            var->end_offset = (time_t)atoi(value);
        }

        /* Set valid min if not NULL */

        value = RetVar_Min(dbres,row);
        if (value && value[0] != '\0') {
            var->min = strdup(value);
            if (!var->min) {
                goto MEMORY_ALLOCATION_ERROR;
            }
        }

        /* Set valid max if not NULL */

        value = RetVar_Max(dbres,row);
        if (value && value[0] != '\0') {
            var->max = strdup(value);
            if (!var->max) {
                goto MEMORY_ALLOCATION_ERROR;
            }
        }

        /* Set valid delta if not NULL */

        value = RetVar_Delta(dbres,row);
        if (value && value[0] != '\0') {
            var->delta = strdup(value);
            if (!var->delta) {
                goto MEMORY_ALLOCATION_ERROR;
            }
        }

        /* Set flags that are not NULL */

        value = RetVar_ReqToRun(dbres,row);
        if (value && value[0] != '\0') {
            var->req_to_run = atoi(value);
        }

        value = RetVar_QCFlag(dbres,row);
        if (value && value[0] != '\0') {
            var->retrieve_qc = atoi(value);
        }

        value = RetVar_QCReqToRun(dbres,row);
        if (value && value[0] != '\0') {
            var->qc_req_to_run = atoi(value);
        }

        /* Set pointer to coordinate system if not NULL */

        value = RetVar_CoordSystemID(dbres,row);
        if (value && value[0] != '\0') {

            coord_system_id = atoi(value);

            for (i = 0; i < ret->ncoord_systems; i++) {

                if (ret->coord_systems[i]->_id == coord_system_id) {
                    var->coord_system = ret->coord_systems[i];
                    break;
                }
            }
        }

        /* Create the list of variable maps and set the datastream references.
         * The variable name lists will be created later by the call to
         * ret_load_variable_names(). */

        /* Allocate memory for the list of variable maps */

        nvarmaps = 0;

        for (i = 0; i < group->nsubgroups; i++) {
            nvarmaps += group->subgroups[i]->ndatastreams;
        }

        var->varmaps = (RetDsVarMap **)
            calloc(nvarmaps + 1, sizeof(RetDsVarMap *));

        if (!var->varmaps) {
            goto MEMORY_ALLOCATION_ERROR;
        }

        /* Create the variable map structures */

        for (i = 0; i < group->nsubgroups; i++) {

            subgroup = group->subgroups[i];

            for (j = 0; j < subgroup->ndatastreams; j++) {

                varmap = (RetDsVarMap *)calloc(1, sizeof(RetDsVarMap));
                if (!varmap) {
                    goto MEMORY_ALLOCATION_ERROR;
                }

                varmap->ds = subgroup->datastreams[j];

                var->varmaps[var->nvarmaps] = varmap;
                var->nvarmaps += 1;
            }
        }

    } /* end loop over all rows in the database query result */

    nrows = dbres->nrows;

    dbres->free(dbres);

    /* Load the variable dimension names */

    if (ret_load_var_dims(dsdb, ret) < 0) {
        return(-1);
    }

    /* Load the input datastream variable names */

    if (ret_load_var_names(dsdb, ret) < 0) {
        return(-1);
    }

    /* Load the output datastreams and variable names */

    if (ret_load_var_outputs(dsdb, ret) < 0) {
        return(-1);
    }

    return(nrows);

MEMORY_ALLOCATION_ERROR:

    ERROR( DSDB_LIB_NAME,
        "Could not load retriever variables from database.\n"
        " -> memory allocation error\n");

    dbres->free(dbres);
    return(-1);
}

/**
 *  Print Retriever Datastream name.
 *
 *  @param  fp - pointer to the output stream to print to
 *  @param  ds - pointer to the Retriever Data Stream structure
 */
static void ret_print_retriever_ds_name(FILE *fp, RetDataStream *ds)
{
    const char *value;

    value = (ds->site) ? ds->site : "sss";
    fprintf(fp, "%s%s", value, ds->name);

    value = (ds->facility) ? ds->facility : "F#";
    fprintf(fp, "%s.%s", value, ds->level);
}

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Free all memory used by a Retriever structure.
 *
 *  @param  ret - pointer to the Retriever structure
 */
void dsdb_free_retriever(Retriever *ret)
{
    RetCoordSystem *coord_system;
    RetCoordDim    *dim;
    int             i, j, k;

    if (!ret) {
        return;
    }

    if (ret->proc_type) free((void *)ret->proc_type);
    if (ret->proc_name) free((void *)ret->proc_name);

    /************************************************************
    * Free all datastream group structures
    ************************************************************/

    for (i = 0; i < ret->ngroups; i++) {
        ret_free_group(ret->groups[i]);
    }

    if (ret->groups) free(ret->groups);

    /************************************************************
    * Free all datastream subgroup structures
    ************************************************************/

    for (i = 0; i < ret->nsubgroups; i++) {
        ret_free_subgroup(ret->subgroups[i]);
    }

    if (ret->subgroups) free(ret->subgroups);

    /************************************************************
    * Free all datastream structures
    ************************************************************/

    for (i = 0; i < ret->ndatastreams; i++) {
        ret_free_datastream(ret->datastreams[i]);
    }

    if (ret->datastreams) free(ret->datastreams);

    /************************************************************
    * Free all coordinate system structures
    ************************************************************/

    for (i = 0; i < ret->ncoord_systems; i++) {

        coord_system = ret->coord_systems[i];

        if (coord_system->name) free((void *)coord_system->name);

        /* Free coordinate dimension structures */

        for (j = 0; j < coord_system->ndims; j++) {

            dim = coord_system->dims[j];

            if (dim->name)        free((void *)dim->name);
            if (dim->data_type)   free((void *)dim->data_type);
            if (dim->units)       free((void *)dim->units);

            if (dim->start)       free((void *)dim->start);
            if (dim->length)      free((void *)dim->length);
            if (dim->interval)    free((void *)dim->interval);

            if (dim->trans_type)  free((void *)dim->trans_type);
            if (dim->trans_range) free((void *)dim->trans_range);
            if (dim->trans_align) free((void *)dim->trans_align);

            /* Free coordinate variable mapping structures */

            for (k = 0; k < dim->nvarmaps; k++) {
                ret_free_varmap(dim->varmaps[k]);
            }

            if (dim->varmaps) free(dim->varmaps);

            free(dim);
        }

        if (coord_system->dims) free(coord_system->dims);

        free(coord_system);
    }

    if (ret->coord_systems) free(ret->coord_systems);

    /************************************************************
    * Free all tramsformation parameter structures
    ************************************************************/

    for (i = 0; i < ret->ntrans_params; i++) {
        ret_free_trans_params(ret->trans_params[i]);
    }

    if (ret->trans_params) free(ret->trans_params);

    free(ret);
}

/**
 *  Get the Retriever information from the database
 *
 *  This function gets the retriever information from the database for the
 *  specified process and populates the retriever data structures.
 *
 *  The memory used by the output structure is dynamically allocated.
 *  It is the responsibility of the calling process to free this memory
 *  when it is no longer needed (see dsdb_free_retriever()).
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  dsdb      - pointer to the open database connection
 *  @param  proc_type - type of process, usually VAP
 *  @param  proc_name - name of VAP
 *  @param  retriever - output: pointer to the Retriever structure
 *
 *  @return
 *    -  1 if successful
 *    -  0 if no retriever information was found in the database
 *    - -1 if an error occurred
 *
 *  @see dsdb_free_retriever()
 */
int dsdb_get_retriever(
    DSDB        *dsdb,
    const char  *proc_type,
    const char  *proc_name,
    Retriever  **retriever)
{
    Retriever *ret;
    int        status;
    int        found_ret_info;

    *retriever = (Retriever *)NULL;
    found_ret_info = 0;

    /* Initialize a new Retriever structure */

    ret = (Retriever *)calloc(1, sizeof(Retriever));
    if (!ret) {

        ERROR( DSDB_LIB_NAME,
            "Could not create new Retriever structure for: %s %s\n"
            " -> memory allocation error\n",
            proc_name, proc_type);

        return(-1);
    }

    /* Copy the input arguments into the Retriever structure */

    ret->proc_type = strdup(proc_type);
    ret->proc_name = strdup(proc_name);

    if (!ret->proc_type || !ret->proc_name) {

        ERROR( DSDB_LIB_NAME,
            "Could not create new Retriever structure for: %s %s\n"
            " -> memory allocation error\n",
            proc_name, proc_type);

        dsdb_free_retriever(ret);
        return(-1);
    }

    /* Load all datastream groups and subgroups */

    status = ret_load_groups_and_subgroups(dsdb, ret);

    if (status < 0) {
        dsdb_free_retriever(ret);
        return(status);
    }
    found_ret_info += status;

    /* Load all datastreams */

    status = ret_load_datastreams(dsdb, ret);

    if (status < 0) {
        dsdb_free_retriever(ret);
        return(status);
    }
    found_ret_info += status;

    /* Load all coordinate systems */

    status = ret_load_coordinate_systems(dsdb, ret);
    if (status < 0) {
        dsdb_free_retriever(ret);
        return(status);
    }
    found_ret_info += status;

    /* Load all variables */

    status = ret_load_variables(dsdb, ret);
    if (status < 0) {
        dsdb_free_retriever(ret);
        return(status);
    }
    found_ret_info += status;

    /* Load all transformation parameters */

    status = ret_load_trans_params(dsdb, ret);
    if (status < 0) {
        dsdb_free_retriever(ret);
        return(status);
    }
    found_ret_info += status;

    *retriever = ret;

    return(found_ret_info);
}

/**
 *  Print Retriever structure.
 *
 *  @param  fp  - pointer to the output stream to print to
 *  @param  ret - pointer to the Retriever structure
 */
void dsdb_print_retriever(FILE *fp, Retriever *ret)
{
    RetDsGroup     *group;
    RetDsSubGroup  *subgroup;
    RetDataStream  *ds;
    RetVariable    *var;
    RetVarOutput   *output;
    RetCoordSystem *coord_system;
    RetCoordDim    *dim;
    RetDsVarMap    *varmap;
    RetTransParams *trans_params;
    const char     *value;
    char            time_string[32];
    int             i, j, k, n;

    if (!fp || !ret) {
        return;
    }

    /************************************************************
    * Print datastream groups
    ************************************************************/

    fprintf(fp,
    "------------------------------------------------------------\n"
    "Retriever Datastream Groups:\n"
    "------------------------------------------------------------\n");

    if (ret->ngroups == 0) {
        fprintf(fp, "\nNo groups defined\n");
    }

    for (i = 0; i < ret->ngroups; i++) {

        group = ret->groups[i];

        fprintf(fp, "\nGroup: %s\n", group->name);

        if (group->nsubgroups <= 0) {
            fprintf(fp, "\nWARNING: No subgroups found in database.\n");
            continue;
        }

        if (group->nsubgroups > 1) {

            fprintf(fp,
                "\nWARNING: Multiple subgroups are not currently supported. Only\n"
                "the first subgroup in the following list will be processed:\n");

            for (j = 0; j < group->nsubgroups; j++) {
                fprintf(fp, "  - %s\n", group->subgroups[j]->name);
            }
        }

        subgroup = group->subgroups[0];

        for (j = 0; j < subgroup->ndatastreams; j++) {

            ds = subgroup->datastreams[j];

            /* print datastream name */

            fprintf(fp, "\n    ");

            ret_print_retriever_ds_name(fp, ds);

            fprintf(fp, "\n");

            /* print datastream properties */

            value = (ds->dep_site) ? ds->dep_site : "NULL";
            fprintf(fp, "      - dep_site:       %s\n", value);

            value = (ds->dep_fac) ? ds->dep_fac : "NULL";
            fprintf(fp, "      - dep_fac:        %s\n", value);

            if (ds->dep_begin_date) {
                msngr_format_time(ds->dep_begin_date, time_string);
            }
            else {
                strcpy(time_string, "NULL");
            }
            fprintf(fp, "      - dep_begin_date: %s\n", time_string);

            if (ds->dep_end_date) {
                msngr_format_time(ds->dep_end_date, time_string);
            }
            else {
                strcpy(time_string, "NULL");
            }
            fprintf(fp, "      - dep_end_date:   %s\n", time_string);

        } /* end loop over datastreams */

    } /* end loop over groups */

    /************************************************************
    * Print variables
    ************************************************************/

    fprintf(fp,
    "\n------------------------------------------------------------\n"
    "Retriever Variables:\n"
    "------------------------------------------------------------\n");

    if (ret->ngroups == 0) {
        fprintf(fp, "\nNo variables defined\n");
    }

    for (i = 0; i < ret->ngroups; i++) {

        group = ret->groups[i];

        fprintf(fp, "\nGroup: %s\n", group->name);

        subgroup = group->subgroups[0];

        for (j = 0; j < group->nvars; j++) {

            var = group->vars[j];

            /* print variable name */

            fprintf(fp, "\n    %s(", var->name);

            if (var->ndim_names) {
                fprintf(fp, "%s", var->dim_names[0]);
                for (n = 1; n < var->ndim_names; n++) {
                    fprintf(fp, ", %s", var->dim_names[n]);
                }
            }

            fprintf(fp, ")\n");

            /* print input search order */

            if (var->nvarmaps == 0) {
                fprintf(fp, "      - input source:       NULL");
            }
            else if (
                var->nvarmaps == 1 &&
                var->varmaps[0]->nnames == 1) {

                varmap = var->varmaps[0];

                fprintf(fp, "      - input source:       ");
                ret_print_retriever_ds_name(fp, varmap->ds);
                fprintf(fp, ":%s\n", varmap->names[0]);
            }
            else {

                fprintf(fp, "      - input search order:\n");
                for (k = 0; k < var->nvarmaps; k++) {

                    varmap = var->varmaps[k];

                    for (n = 0; n < varmap->nnames; n++) {
                        fprintf(fp, "          - ");
                        ret_print_retriever_ds_name(fp, varmap->ds);
                        fprintf(fp, ":%s\n", varmap->names[n]);
                    }
                }
            }

            /* print variable properties */

            value = (var->data_type) ? var->data_type : "NULL";
            fprintf(fp, "      - data_type:          %s\n", value);

            value = (var->units) ? var->units : "NULL";
            fprintf(fp, "      - units:              %s\n", value);

            value = (var->min) ? var->min : "NULL";
            fprintf(fp, "      - valid_min:          %s\n", value);

            value = (var->max) ? var->max : "NULL";
            fprintf(fp, "      - valid_max:          %s\n", value);

            value = (var->delta) ? var->delta : "NULL";
            fprintf(fp, "      - valid_delta:        %s\n", value);
            fprintf(fp, "      - start_offset:       %d\n", (int)var->start_offset);
            fprintf(fp, "      - end_offset:         %d\n", (int)var->end_offset);
            fprintf(fp, "      - required_to_run:    %d\n", var->req_to_run);
            fprintf(fp, "      - retrieve_qc:        %d\n", var->retrieve_qc);
            fprintf(fp, "      - qc_required_to_run: %d\n", var->qc_req_to_run);

            value = (var->coord_system) ? var->coord_system->name : "NULL";
            fprintf(fp, "      - coordinate_system:  %s\n", value);

            /* print output targets */

            if (var->noutputs == 0) {
                fprintf(fp, "      - output target:      NULL\n");
            }
            else if (var->noutputs == 1) {

                output = var->outputs[0];

                fprintf(fp, "      - output target:      %s.%s:%s\n",
                    output->dsc_name, output->dsc_level, output->var_name);
            }
            else {

                fprintf(fp, "      - output targets:\n");
                for (k = 0; k < var->noutputs; k++) {

                    output = var->outputs[k];
                    fprintf(fp, "          - %s.%s:%s\n",
                        output->dsc_name, output->dsc_level, output->var_name);
                }
            }

        } /* end loop over variables */

    } /* end loop over groups */

    /************************************************************
    * Print coordinate systems
    ************************************************************/

    fprintf(fp,
    "\n------------------------------------------------------------\n"
    "Retriever Coordinate Systems:\n"
    "------------------------------------------------------------\n");

    if (ret->ncoord_systems == 0) {
        fprintf(fp, "\nNo coordinate systems defined\n");
    }

    for (i = 0; i < ret->ncoord_systems; i++) {

        coord_system = ret->coord_systems[i];

        fprintf(fp, "\nCoordinate System: %s\n", coord_system->name);

        for (j = 0; j < coord_system->ndims; j++) {

            dim = coord_system->dims[j];

            /* print dimension name */

            fprintf(fp, "\n    %s\n", dim->name);

            /* print dimension properties */

            value = (dim->data_type) ? dim->data_type : "NULL";
            fprintf(fp, "      - data_type:     %s\n", value);

            value = (dim->units) ? dim->units : "NULL";
            fprintf(fp, "      - units:         %s\n", value);

            value = (dim->start) ? dim->start : "NULL";
            fprintf(fp, "      - start value:   %s\n", value);

            value = (dim->interval) ? dim->interval : "NULL";
            fprintf(fp, "      - interval:      %s\n", value);

            value = (dim->length) ? dim->length : "NULL";
            fprintf(fp, "      - length:        %s\n", value);

            fprintf(fp, "      - transformation parameters:\n");

            value = (dim->trans_type) ? dim->trans_type : "NULL";
            fprintf(fp, "          - type:      %s\n", value);

            value = (dim->trans_range) ? dim->trans_range : "NULL";
            fprintf(fp, "          - range:     %s\n", value);

            value = (dim->trans_align) ? dim->trans_align : "NULL";
            fprintf(fp, "          - alignment: %s\n", value);

            /* print coordinate variable maps */

            if (dim->nvarmaps == 0) {
                fprintf(fp, "      - variable map:  NULL\n");
            }
            else if (
                dim->nvarmaps == 1 &&
                dim->varmaps[0]->nnames == 1) {

                varmap = dim->varmaps[0];

                fprintf(fp, "      - variable map:  ");

                ret_print_retriever_ds_name(fp, varmap->ds);
                fprintf(fp, ":%s\n", varmap->names[0]);
            }
            else {

                fprintf(fp, "      - variable map search order:\n");
                for (k = 0; k < dim->nvarmaps; k++) {

                    varmap = dim->varmaps[k];

                    for (n = 0; n < varmap->nnames; n++) {
                        fprintf(fp, "          - ");
                        ret_print_retriever_ds_name(fp, varmap->ds);
                        fprintf(fp, ":%s\n", varmap->names[n]);
                    }
                }
            }
        }

    } /* end loop over coord_systems */

    /************************************************************
    * Print transformation parameters
    ************************************************************/

    fprintf(fp,
    "\n------------------------------------------------------------\n"
    "Retriever Extended Transformation Parameters:\n"
    "------------------------------------------------------------\n");

    if (ret->ntrans_params == 0) {
        fprintf(fp, "\nNo extended transformation parameters defined\n");
    }

    for (i = 0; i < ret->ntrans_params; i++) {

        trans_params = ret->trans_params[i];

        fprintf(fp, "\nCoordinate System: %s\n", trans_params->coordsys);
        fprintf(fp, "\n%s\n", trans_params->params);
    }
}

/**
 *  Set the location for a Retriever structure.
 *
 *  This function will set the site and facility values in all datastream
 *  structures, and filter out the datastreams that have site and/or facility
 *  depedencies that do not match the specified site and/or facility.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  ret       - pointer to the Retriever structure
 *  @param  site      - site name
 *  @param  facility  - facility name
 *
 *  @return
 *    - 1 if successful
 *    - 0 if one of the following errors occurred:
 *        - site and/or facility argument(s) were NULL
 *        - all input datastreams for a required variable were filtered
 *        - all input datastreams for a coordinate variable map were filtered
 *        - memory allocation error
 */
int dsdb_set_retriever_location(
    Retriever  *ret,
    const char *site,
    const char *facility)
{
    RetDataStream  *ds;
    int             retval;
    int             i;

    if (!site || !facility) {

        ERROR( DSDB_LIB_NAME,
            "Both site and facility are required to set retriever location.\n",
            site, facility);

        return(0);
    }

    retval = 1;

    for (i = 0; i < ret->ndatastreams; i++) {

        ds = ret->datastreams[i];

        if ((ds->dep_site && strcmp(site, ds->dep_site) != 0) ||
            (ds->dep_fac  && strcmp(facility, ds->dep_fac) != 0)) {

            if (ret_delete_datastream(ret, ds) == 0) {
                retval = 0;
            }

            i--;

            continue;
        }

        if (!ds->site)     ds->site     = strdup(site);
        if (!ds->facility) ds->facility = strdup(facility);

        if (!ds->site || !ds->facility) {

            ERROR( DSDB_LIB_NAME,
                "Could not set location for retriever definition.\n"
                " -> memory allocation error\n");

            return(-1);
        }
    }

    if (retval == 0) {

        ERROR( DSDB_LIB_NAME,
            "Retriever definition is not valid for: %s%s\n",
            site, facility);
    }

    return(retval);
}

/*@}*/
