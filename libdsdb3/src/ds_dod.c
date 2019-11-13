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
*    $Revision: 17019 $
*    $Author: ermold $
*    $Date: 2013-03-05 19:19:42 +0000 (Tue, 05 Mar 2013) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file ds_dod.c
 *  Datastream DOD Functions.
 */

#include "dsdb3.h"
#include "dbog_dod.h"

/**
 *  @defgroup DSDB_DSDODS Datastream DODs
 */
/*@{*/

/*******************************************************************************
 *  Private Functions
 */
/** @privatesection */

static int _dsdb_change_dsdod_att_value(
    CDSGroup *cds,
    char     *var_name,
    char     *att_name,
    char     *att_type,
    char     *att_value,
    int       locker)
{
    void        *parent;
    CDSAtt      *att;
    CDSDataType  type;
    size_t       length;
    size_t       nelems;
    void        *value;
    int          free_value;

    /* Get the CDS attribute */

    parent = (var_name) ? (void *)cds_get_var(cds, var_name) : (void *)cds;

    if (!parent) {
        return(1);
    }

    att = cds_get_att(parent, att_name);
    if (!att) {
        return(1);
    }

    /* Make sure the attribute types match */

    type = cds_data_type(att_type);

    if (type != att->type) {
        return(1);
    }

    /* Check if the attribute is locked */

    if (att->def_lock == locker) {
        cds_set_definition_lock(att, 0);
    }
    else if (att->def_lock) {
        return(1);
    }

    /* Create the attribute value */

    free_value = 0;

    if (!att_value) {
        return(1);
    }
    else if (type == CDS_CHAR) {
        value  = (void *)att_value;
        length = strlen(att_value) + 1;
    }
    else {

        value = cds_string_to_array(att_value, type, &nelems, NULL);

        if (value) {
            length     = (size_t)nelems;
            free_value = 1;
        }
        else if (nelems == 0) {
            return(1);
        }
        else {
            /* memory allocation error */
            return(0);
        }
    }

    /* change attribute value */

    if (!cds_change_att_value(att, type, length, value)) {
        if (free_value) free(value);
        return(0);
    }

    cds_set_definition_lock(att, locker);

    if (free_value) free(value);

    return(1);
}

/**
 *  Insert a time into an array of time values.
 *
 *  The time value will be inserted into the array such that the
 *  array maintains a sorted order starting with the earliest time.
 *  The time value will not be added to the array if it matches a
 *  time already defined in the array.
 *
 *  This function allocates memory for the times array so it must
 *  be freed when it is no longer needed.
 *
 *  @param  times    - memory address of the times array pointer
 *  @param  ntimes   - pointer to the number of time values
 *  @param  new_time - time value to add to the array
 *
 *  @return
 *    - pointer to the times array
 *    - NULL if a memory allocation error occurred
 */
static time_t *_dsdb_insert_time_array_value(
    time_t **times,
    int     *ntimes,
    time_t   new_time)
{
    time_t *new_times;
    int     insert_index;
    int     i;

    if (*ntimes <= 0) {
        *times       = NULL;
        *ntimes      = 0;
        insert_index = 0;
    }
    else if (new_time > (*times)[*ntimes - 1]) {
        insert_index = *ntimes;
    }
    else {
        for (i = 0; new_time > (*times)[i]; i++);

        if (new_time == (*times)[i]) {
            return(*times);
        }
        else {
            insert_index = i;
        }
    }

    new_times = (time_t *)realloc(*times, (*ntimes+1) * sizeof(time_t));
    if (!new_times) {
        return((time_t *)NULL);
    }

    *times = new_times;

    for (i = (*ntimes); i > insert_index; i--) {
        (*times)[i] = (*times)[i-1];
    }

    *ntimes += 1;
    (*times)[i] = new_time;

    return(*times);
}

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Free all memory used by a DSDOD structure.
 *
 *  @param  dsdod - pointer to the DSDOD structure
 */
void dsdb_free_dsdod(DSDOD *dsdod)
{
    int i;

    if (dsdod) {

        if (dsdod->cds_group) {
            cds_set_definition_lock(dsdod->cds_group, 0);
            cds_delete_group(dsdod->cds_group);
        }

        if (dsdod->site)       free((void *)dsdod->site);
        if (dsdod->facility)   free((void *)dsdod->facility);
        if (dsdod->name)       free((void *)dsdod->name);
        if (dsdod->level)      free((void *)dsdod->level);
        if (dsdod->version)    free((void *)dsdod->version);
        if (dsdod->att_times)  free((void *)dsdod->att_times);

        if (dsdod->ndod_times) {
            for (i = 0; i < dsdod->ndod_times; i++) {
                free(dsdod->dod_versions[i]);
            }
            free(dsdod->dod_times);
            free(dsdod->dod_versions);
        }

        free(dsdod);
    }
}

/**
 *  Get the DSDOD for a datastream.
 *
 *  This function will get the DSDOD for the specified datastream
 *  and data time. It will:
 *
 *    - create a new DSDOD structure
 *    - get the list of DOD versions used by the specified datastream
 *    - get the list of times when the attribute values change
 *    - get the DOD for the specified data time
 *    - load the site/facility specific attribute values
 *    - load the time varying attribute values for the specified data time
 *
 *  If the data time is not specified, the current time will be used.
 *  If the data time is less than the time of the earliest DOD version,
 *  the earliest DOD version will be used.
 *
 *  The memory used by the output structure is dynamically allocated.
 *  It is the responsibility of the calling process to free this memory
 *  when it is no longer needed (see dsdb_free_dsdod()).
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  Null results from the database are not reported as errors.
 *  It is the responsibility of the calling process to report
 *  these as errors if necessary.
 *
 *  @param  dsdb      - pointer to the open database connection
 *  @param  site      - site name
 *  @param  facility  - facility name
 *  @param  dsc_name  - datastream class name
 *  @param  dsc_level - datastream class level
 *
 *  @param  data_time - the time used to determine the DOD version and
 *                      set the time varying attribute values. If not
 *                      specified the current time will be used.
 *
 *  @param  dsdod     - output: pointer to the DSDOD structure
 *
 *  @return
 *    -  1 if successful
 *    -  0 if the database returned a NULL result
 *    - -1 if an error occurred
 *
 *  @see  dsdb_free_dsdod()
 */
int dsdb_get_dsdod(
    DSDB        *dsdb,
    const char  *site,
    const char  *facility,
    const char  *dsc_name,
    const char  *dsc_level,
    time_t       data_time,
    DSDOD      **dsdod)
{
    int status;
    int nversions;

    if (!data_time) {
        data_time = time(NULL);
    }

    /* Create a new DSDOD structure */

    *dsdod = dsdb_new_dsdod(site, facility, dsc_name, dsc_level);
    if (!*dsdod) {
        return(-1);
    }

    (*dsdod)->data_time = data_time;

    /* Get the list of DOD versions used by this datastream */

    nversions = dsdb_get_dsdod_versions(dsdb, *dsdod);
    if (nversions <= 0) {
        dsdb_free_dsdod(*dsdod);
        *dsdod = (DSDOD *)NULL;
        return(nversions);
    }

    /* Get the DOD and datastream attributes */

    status = dsdb_update_dsdod(dsdb, *dsdod, data_time);
    if (status <= 0) {
        dsdb_free_dsdod(*dsdod);
        *dsdod = (DSDOD *)NULL;
        return(status);
    }

    return(1);
}

/**
 *  Update a DSDOD for the time of the data being processed.
 *
 *  This function will use the time of the data being processed
 *  to update the DOD version and/or the time varying attribute
 *  values if they are different from what is currently loaded.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  dsdb      - pointer to the open database connection
 *  @param  dsdod     - pointer to the DSDOD structure
 *  @param  data_time - the time of the data being processed
 *
 *  @return
 *    -  1 if the DSDOD was updated or no updates were needed
 *    -  0 if the database returned a NULL result
 *    - -1 if an error occurred
 */
int dsdb_update_dsdod(
    DSDB   *dsdb,
    DSDOD  *dsdod,
    time_t  data_time)
{
    const char *dod_version;
    char       *dsdod_version;
    CDSGroup   *cds_group;
    int         natt_times;
    int         status;

    /* Check if the DOD version needs to be updated */

    dod_version = dsdb_check_for_dsdod_version_update(dsdod, data_time);
    if (dod_version) {

        /* Set DSDOD version */

        dsdod_version = msngr_copy_string(dod_version);

        if (!dsdod_version) {

            ERROR( DSDB_LIB_NAME,
                "Could not update DSDOD for: %s%s%s.%s\n"
                " -> memory allocation error\n",
                dsdod->site, dsdod->name, dsdod->facility, dsdod->level);

            return(-1);
        }

        /* Get the DOD */

        status = dsdb_get_dod(
            dsdb, dsdod->name, dsdod->level, dsdod_version, &cds_group);

        if (status <= 0) {
            free(dsdod_version);
            return(status);
        }

        /* Update the DSDOD */

        if (dsdod->version)   free(dsdod->version);
        if (dsdod->cds_group) {
            cds_set_definition_lock(dsdod->cds_group, 0);
            cds_delete_group(dsdod->cds_group);
        }

        dsdod->data_time = data_time;
        dsdod->version   = dsdod_version;
        dsdod->cds_group = cds_group;

        /* Get the site/facility specific attribute values */

        if (dsdb_get_dsdod_att_values(dsdb, dsdod) < 0) {
            return(-1);
        }

        /* Get the list of times when there are attribute value changes */

        natt_times = dsdb_get_dsdod_att_times(dsdb, dsdod);
        if (natt_times < 0) {
            return(-1);
        }

        /* Get the time varying attribute values */

        if (natt_times > 0) {
            if (dsdb_get_dsdod_time_att_values(dsdb, dsdod) < 0) {
                return(-1);
            }
        }

        return(1);
    }

    /* Check if the time varying attribute values need to be updated */

    if (dsdb_check_for_dsdod_time_atts_update(dsdod, data_time)) {

        dsdod->data_time = data_time;

        if (dsdb_get_dsdod_time_att_values(dsdb, dsdod) < 0) {
            return(-1);
        }

        return(1);
    }

    return(0);
}

/**
 *  Check for a DSDOD version update.
 *
 *  This function will check if the DOD version being used by a DSDOD
 *  needs to be updated. The new DOD version will be determined for
 *  the specified data time using the dod_times and dod_versions
 *  listed in the DSDOD (see dsdb_get_dsdod_versions()).
 *
 *  @param  dsdod     - pointer to the DSDOD structure
 *  @param  data_time - the time of the data being processed
 *
 *  @return
 *    - the new DOD version if the DSDOD needs to be updated.
 *    - NULL if no update is needed.
 */
const char *dsdb_check_for_dsdod_version_update(
    DSDOD *dsdod,
    time_t data_time)
{
    const char *dod_version;
    int         i;

    if (dsdod->ndod_times == 0) {
        return((const char *)NULL);
    }

    for (i = 0; i < dsdod->ndod_times; i++) {
        if (data_time < dsdod->dod_times[i]) {
            break;
        }
    }

    if (i > 0) i--;

    dod_version = dsdod->dod_versions[i];

    if (!dsdod->version) {
        return(dod_version);
    }

    if (strcmp(dod_version, dsdod->version) != 0) {
        return(dod_version);
    }

    return((const char *)NULL);
}

/**
 *  Check for DSDOD time varying attribute value updates.
 *
 *  This function will check if the time varying attribute values
 *  for a DSDOD need to be updated. This is done by checking for
 *  an attribute change time in the DSDOD att_times array that falls
 *  between the data_time of the DSDOD and the specified data_time.
 *  (see dsdb_get_dsdod_att_times()).
 *
 *  @param  dsdod     - pointer to the DSDOD structure
 *  @param  data_time - the time of the data being processed
 *
 *  @return
 *    - 1 if one or more attribute values need to be updated.
 *    - 0 if no update is needed.
 */
int dsdb_check_for_dsdod_time_atts_update(
    DSDOD *dsdod,
    time_t data_time)
{
    time_t  t1;
    time_t  t2;
    time_t  tn;
    int     n;

    if (dsdod->natt_times == 0) {
        return(0);
    }

    if (dsdod->data_time < data_time) {
        t1 = dsdod->data_time;
        t2 = data_time;
    }
    else {
        t1 = data_time;
        t2 = dsdod->data_time;
    }

    for (n = 0; n < dsdod->natt_times; n++) {

        tn = dsdod->att_times[n];

        if (t1 < tn && tn <= t2) {
            break;
        }
    }

    if (n < dsdod->natt_times) {
        return(1);
    }

    return(0);
}

/**
 *  Create a new DSDOD structure.
 *
 *  The memory used by the returned structure is dynamically allocated.
 *  It is the responsibility of the calling process to free this memory
 *  when it is no longer needed (see dsdb_free_dsdod()).
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  site      - site name
 *  @param  facility  - facility name
 *  @param  dsc_name  - datastream class name
 *  @param  dsc_level - datastream class level
 *
 *  @return
 *    - pointer to the new DSDOD structure
 *    - NULL if a memory allocation error occurred
 *
 *  @see dsdb_free_dsdod()
 */
DSDOD *dsdb_new_dsdod(
    const char *site,
    const char *facility,
    const char *dsc_name,
    const char *dsc_level)
{
    DSDOD *dsdod;

    /* Create the DSDOD structure */

    dsdod = (DSDOD *)calloc(1, sizeof(DSDOD));
    if (!dsdod) {
        goto RETURN_MEM_ERROR;
    }

    /* Copy the input arguments into the DSDOD structure */

    dsdod->site = msngr_copy_string(site);
    if (!dsdod->site) {
        goto RETURN_MEM_ERROR;
    }

    dsdod->facility = msngr_copy_string(facility);
    if (!dsdod->facility) {
        goto RETURN_MEM_ERROR;
    }

    dsdod->name = msngr_copy_string(dsc_name);
    if (!dsdod->name) {
        goto RETURN_MEM_ERROR;
    }

    dsdod->level = msngr_copy_string(dsc_level);
    if (!dsdod->level) {
        goto RETURN_MEM_ERROR;
    }

    return(dsdod);

RETURN_MEM_ERROR:

    ERROR( DSDB_LIB_NAME,
        "Could not create new DSDOD structure for: %s%s%s.%s\n"
        " -> memory allocation error\n",
        site, dsc_name, facility, dsc_level);

    if (dsdod) {
        dsdb_free_dsdod(dsdod);
    }

    return((DSDOD *)NULL);
}

/**
 *  Get the DOD for a datastream class and DOD version.
 *
 *  This function will set the definition lock value to 1 for all
 *  dimensions and attributes that do not have NULL values. It will
 *  also set the definition lock value to 1 for all variables
 *  (see cds_set_definition_lock()).
 *
 *  The memory used by the output structure is dynamically allocated.
 *  It is the responsibility of the calling process to free this memory
 *  when it is no longer needed (see cds_delete_group()).
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  Null results from the database are not reported as errors.
 *  It is the responsibility of the calling process to report
 *  these as errors if necessary.
 *
 *  @param  dsdb        - pointer to the open database connection
 *  @param  dsc_name    - datastream class name
 *  @param  dsc_level   - datastream class level
 *  @param  dod_version - DOD version
 *  @param  cds_group   - output: pointer to the CDSGroup containing the DOD
 *
 *  @return
 *    -  1 if successful
 *    -  0 if the database returned a NULL result
 *    - -1 if an error occurred
 *
 *  @see  cds_delete_group()
 */
int dsdb_get_dod(
    DSDB        *dsdb,
    const char  *dsc_name,
    const char  *dsc_level,
    const char  *dod_version,
    CDSGroup   **cds_group)
{
    DBStatus     status;
    char         cds_name[128];
    CDSDim      *dim;
    CDSAtt      *att;
    CDSVar      *var;
    DBResult    *dims;
    DBResult    *atts;
    DBResult    *vars;
    int          dims_row;
    int          atts_row;
    int          vars_row;
    int          unlimited;
    char        *dim_name;
    char        *att_name;
    char        *var_name;
    const char  *var_dims[8];
    int          nvar_dims;
    char        *strval;
    size_t       nelems;
    size_t       length;
    CDSDataType  type;
    void        *value;
    int          free_value;
    int          set_lock;
    int          dod_not_found;

    /* Create the CDS Group */

    sprintf(cds_name, "%s.%s-%s", dsc_name, dsc_level, dod_version);

    *cds_group = cds_define_group((CDSGroup *)NULL, cds_name);
    if (!*cds_group) {
        return(-1);
    }

    dod_not_found = 1;

    /* Get DOD Dimensions */

    status = dodog_get_dod_dims(dsdb->dbconn,
        dsc_name, dsc_level, dod_version, &dims);

    if (status != DB_NO_ERROR) {
        if (status != DB_NULL_RESULT) {
            cds_delete_group(*cds_group);
            *cds_group = (CDSGroup *)NULL;
            return(-1);
        }
    }
    else {

        dod_not_found = 0;

        for (dims_row = 0; dims_row < dims->nrows; dims_row++) {

            dim_name  = DodDimName(dims,dims_row);
            strval    = DodDimLength(dims,dims_row);
            unlimited = 0;
            set_lock  = 0;

            if (strval == NULL) {
                length = 0;
            }
            else {
                length = (size_t)atoi(strval);
                if (length == 0) {
                    unlimited = 1;
                }
                set_lock = 1;
            }

            dim = cds_define_dim(*cds_group, dim_name, length, unlimited);

            if (!dim) {
                dims->free(dims);
                cds_delete_group(*cds_group);
                *cds_group = (CDSGroup *)NULL;
                return(-1);
            }

            if (set_lock) cds_set_definition_lock(dim, 1);
        }

        dims->free(dims);
    }

    /* Get Global DOD Attributes */

    status = dodog_get_dod_atts(dsdb->dbconn,
        dsc_name, dsc_level, dod_version, &atts);

    if (status != DB_NO_ERROR) {
        if (status != DB_NULL_RESULT) {
            cds_delete_group(*cds_group);
            *cds_group = (CDSGroup *)NULL;
            return(-1);
        }
    }
    else {

        dod_not_found = 0;

        for (atts_row = 0; atts_row < atts->nrows; atts_row++) {

            att_name   = DodAttName(atts,atts_row);
            strval     = DodAttType(atts,atts_row);
            type       = cds_data_type(strval);
            strval     = DodAttValue(atts,atts_row);
            free_value = 0;
            set_lock   = 0;

            if (strval == (char *)NULL) {
                length = 0;
                value  = (void *)NULL;
            }
            else if (type == CDS_CHAR) {
                value    = (void *)strval;
                length   = strlen(strval) + 1;
                set_lock = 1;
            }
            else {
                value = cds_string_to_array(strval, type, &nelems, NULL);

                if (nelems == (size_t)-1) {

                    ERROR( DSDB_LIB_NAME,
                        "Could not convert string to array: '%s'\n"
                        " -> memory allocation error\n", strval);

                    atts->free(atts);
                    cds_delete_group(*cds_group);
                    *cds_group = (CDSGroup *)NULL;
                    return(-1);
                }

                length     = nelems;
                free_value = 1;
                set_lock   = 1;
            }

            att = cds_define_att(*cds_group, att_name, type, length, value);

            if (!att) {
                if (free_value) free(value);
                atts->free(atts);
                cds_delete_group(*cds_group);
                *cds_group = (CDSGroup *)NULL;
                return(-1);
            }

            if (set_lock)   cds_set_definition_lock(att, 1);
            if (free_value) free(value);
        }

        atts->free(atts);
    }

    /* Get DOD Variables */

    status = dodog_get_dod_vars(dsdb->dbconn,
        dsc_name, dsc_level, dod_version, &vars);

    if (status != DB_NO_ERROR) {
        if (status != DB_NULL_RESULT) {
            cds_delete_group(*cds_group);
            *cds_group = (CDSGroup *)NULL;
            return(-1);
        }
    }
    else {

        dod_not_found = 0;

        /* Get DOD Variable Dimensions */

        status = dodog_get_dod_var_dims(dsdb->dbconn,
            dsc_name, dsc_level, dod_version, "%", &dims);

        if (status != DB_NO_ERROR) {
            if (status != DB_NULL_RESULT) {
                vars->free(vars);
                cds_delete_group(*cds_group);
                *cds_group = (CDSGroup *)NULL;
                return(-1);
            }
        }

        dims_row = 0;

        /* Get DOD Variable Attributes */

        status = dodog_get_dod_var_atts(dsdb->dbconn,
            dsc_name, dsc_level, dod_version, "%", &atts);

        if (status != DB_NO_ERROR) {
            if (status != DB_NULL_RESULT) {
                if (dims) dims->free(dims);
                vars->free(vars);
                cds_delete_group(*cds_group);
                *cds_group = (CDSGroup *)NULL;
                return(-1);
            }
        }

        atts_row = 0;

        /* Define Variables */

        for (vars_row = 0; vars_row < vars->nrows; vars_row++) {

            var_name = DodVarName(vars,vars_row);
            strval   = DodVarType(vars,vars_row);
            type     = cds_data_type(strval);

            /* create list of variable dimension names */

            nvar_dims = 0;

            for (; dims_row < dims->nrows; dims_row++) {

                strval = DodVarDimVarName(dims,dims_row);

                if ((strcmp(var_name, strval) != 0)) {
                    break;
                }

                var_dims[nvar_dims++] = DodVarDimName(dims,dims_row);
            }

            /* define variable */

            var = cds_define_var(
                *cds_group, var_name, type, nvar_dims, var_dims);

            if (!var) {
                if (dims) dims->free(dims);
                if (atts) atts->free(atts);
                vars->free(vars);
                cds_delete_group(*cds_group);
                *cds_group = (CDSGroup *)NULL;
                return(-1);
            }

            /* define variable attributes */

            for (; atts_row < atts->nrows; atts_row++) {

                strval = DodVarAttVarName(atts,atts_row);

                if ((strcmp(var_name, strval) != 0)) {
                    break;
                }

                att_name   = DodVarAttName(atts,atts_row);
                strval     = DodVarAttType(atts,atts_row);
                type       = cds_data_type(strval);
                strval     = DodVarAttValue(atts,atts_row);
                free_value = 0;
                set_lock   = 0;

                if (strval == (char *)NULL) {
                    length = 0;
                    value  = (void *)NULL;
                }
                else if (type == CDS_CHAR) {
                    value    = (void *)strval;
                    length   = strlen(strval) + 1;
                    set_lock = 1;
                }
                else {
                    value = cds_string_to_array(strval, type, &nelems, NULL);

                    if (nelems == (size_t)-1) {

                        ERROR( DSDB_LIB_NAME,
                            "Could not convert string to array: '%s'\n"
                            " -> memory allocation error\n", strval);

                        if (dims) dims->free(dims);
                        if (atts) atts->free(atts);
                        vars->free(vars);
                        cds_delete_group(*cds_group);
                        *cds_group = (CDSGroup *)NULL;
                        return(-1);
                    }

                    length     = (size_t)nelems;
                    free_value = 1;
                    set_lock   = 1;
                }

                att = cds_define_att(var, att_name, type, length, value);

                if (!att) {
                    if (dims) dims->free(dims);
                    if (atts) atts->free(atts);
                    if (free_value) free(value);
                    vars->free(vars);
                    cds_delete_group(*cds_group);
                    *cds_group = (CDSGroup *)NULL;
                    return(-1);
                }

                if ((strcmp(var_name, "time")        == 0) ||
                    (strcmp(var_name, "base_time")   == 0) ||
                    (strcmp(var_name, "time_offset") == 0)) {

                    if ((strcmp(att_name, "long_name") == 0) ||
                        (strcmp(att_name, "units")     == 0) ||
                        (strcmp(att_name, "string")    == 0)) {

                        set_lock = 0;
                    }
                }

                if (set_lock)   cds_set_definition_lock(att, 1);
                if (free_value) free(value);
            }

            cds_set_definition_lock(var, 1);
        }

        if (dims) dims->free(dims);
        if (atts) atts->free(atts);
        vars->free(vars);
    }

    /* Check if the DOD was found */

    if (dod_not_found) {
        cds_delete_group(*cds_group);
        *cds_group = (CDSGroup *)NULL;
        return(0);
    }

    cds_set_definition_lock(*cds_group, 1);

    return(1);
}

/**
 *  Get the list of DOD versions used by a datastream.
 *
 *  This function will populate the dod_times and dod_versions
 *  members of the specified DSDOD.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  Null results from the database are not reported as errors.
 *  It is the responsibility of the calling process to report
 *  these as errors if necessary.
 *
 *  @param  dsdb  - pointer to the open database connection
 *  @param  dsdod - pointer to the DSDOD structure
 *
 *  @return
 *    -  number of DOD versions
 *    -  0 if the database returned a NULL result
 *    - -1 if an error occurred
 */
int dsdb_get_dsdod_versions(
    DSDB  *dsdb,
    DSDOD *dsdod)
{
    DBStatus   status;
    int        row;
    char      *dod_version;
    time_t     dod_time;
    char      *time_string;

    DBResult *dbres        = (DBResult *)NULL;
    int       ndod_times   = 0;
    char    **dod_versions = (char **)NULL;
    time_t   *dod_times    = (time_t *)NULL;

    /* Get list of DOD times and versions */

    status = dodog_get_ds_dod_versions(dsdb->dbconn,
        dsdod->site, dsdod->facility, dsdod->name, dsdod->level, &dbres);

    if (status == DB_NO_ERROR) {

        dod_times = (time_t *)calloc((dbres->nrows + 1), sizeof(time_t));
        if (!dod_times) {
            goto RETURN_MEM_ERROR;
        }

        dod_versions = (char **)calloc((dbres->nrows + 1), sizeof(char *));
        if (!dod_versions) {
            goto RETURN_MEM_ERROR;
        }

        for (row = 0; row < dbres->nrows; row++) {

            dod_version = DsDodVersion(dbres,row);
            time_string = DsDodTime(dbres,row);

            dsdb_text_to_time(dsdb, time_string, &dod_time);

            dod_times[row]    = dod_time;
            dod_versions[row] = msngr_copy_string(dod_version);

            if (!dod_versions[row]) {
                goto RETURN_MEM_ERROR;
            }

            ndod_times++;
        }

        dbres->free(dbres);

        /* Update the time and version lists in the DSDOD */

        if (dsdod->ndod_times) {
            for (row = 0; row < dsdod->ndod_times; row++) {
                free(dsdod->dod_versions[row]);
            }
            free(dsdod->dod_times);
            free(dsdod->dod_versions);
        }

        dsdod->ndod_times   = ndod_times;
        dsdod->dod_times    = dod_times;
        dsdod->dod_versions = dod_versions;

        return(ndod_times);
    }
    else if (status == DB_NULL_RESULT) {
        return(0);
    }

    return(-1);

RETURN_MEM_ERROR:

    if (ndod_times) {
        for (row = 0; row < ndod_times; row++) {
            free(dod_versions[row]);
        }
    }

    if (dbres)        dbres->free(dbres);
    if (dod_times)    free(dod_times);
    if (dod_versions) free(dsdod->dod_versions);

    ERROR( DSDB_LIB_NAME,
        "Could not get DSDOD versions for: %s%s%s.%s\n"
        " -> memory allocation error\n",
        dsdod->site, dsdod->name, dsdod->facility, dsdod->level);

    return(-1);
}

/**
 *  Get the list of times when the attribute values change for a DSDOD.
 *
 *  The DOD and site/facility specific attributes must be loaded
 *  before this function can be called (see dsdb_new_dsdod(),
 *  dsdb_get_dod() and dsdb_get_dsdod_att_values()).
 *
 *  This function will only load the times for attributes found
 *  in the DOD that have not been locked or have a definition lock
 *  value equal to 3 (see dsdb_get_dsdod_time_att_values()).
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  dsdb  - pointer to the open database connection
 *  @param  dsdod - pointer to the DSDOD structure
 *
 *  @return
 *    -  number of attribute value change times
 *    -  0 if the database returned a NULL result
 *    - -1 if an error occurred
 */
int dsdb_get_dsdod_att_times(
    DSDB  *dsdb,
    DSDOD *dsdod)
{
    DBStatus  status;
    DBResult *dbres;
    CDSVar   *var;
    CDSAtt   *att;
    int       row;
    char     *var_name;
    char     *att_name;
    char     *time_string;
    time_t    secs1970;

    int       natt_times = 0;
    time_t   *att_times  = (time_t *)NULL;

    /* Get Global Attribute Times */

    status = dodog_get_ds_att_times(dsdb->dbconn,
        dsdod->site, dsdod->facility, dsdod->name, dsdod->level, "%", &dbres);

    if (status != DB_NO_ERROR) {
        if (status != DB_NULL_RESULT) {
            return(-1);
        }
    }
    else {
        for (row = 0; row < dbres->nrows; row++) {

            att_name = DsAttTimeName(dbres,row);

            att = cds_get_att(dsdod->cds_group, att_name);
            if (!att) {
                continue;
            }

            if (att->def_lock && att->def_lock != 3) {
                continue;
            }

            time_string = DsAttTimeTime(dbres,row);

            dsdb_text_to_time(dsdb, time_string, &secs1970);

            if (!_dsdb_insert_time_array_value(
                &att_times, &natt_times, secs1970)) {

                ERROR( DSDB_LIB_NAME,
                    "Could not get DSDOD attribute change times for: %s%s%s.%s\n"
                    " -> memory allocation error\n",
                    dsdod->site, dsdod->name, dsdod->facility, dsdod->level);

                if (att_times) free(att_times);

                dbres->free(dbres);
                return(-1);
            }
        }

        dbres->free(dbres);
    }

    /* Get Variable Attribute Times */

    status = dodog_get_ds_var_att_times(dsdb->dbconn,
        dsdod->site, dsdod->facility, dsdod->name, dsdod->level,
        "%", "%", &dbres);

    if (status != DB_NO_ERROR) {
        if (status != DB_NULL_RESULT) {
            return(-1);
        }
    }
    else {
        for (row = 0; row < dbres->nrows; row++) {

            var_name = DsVarAttTimeVar(dbres,row);
            att_name = DsVarAttTimeName(dbres,row);

            var = cds_get_var(dsdod->cds_group, var_name);
            if (!var) {
                continue;
            }

            att = cds_get_att(var, att_name);
            if (!att) {
                continue;
            }

            if (att->def_lock && att->def_lock != 3) {
                continue;
            }

            time_string = DsVarAttTimeTime(dbres,row);

            dsdb_text_to_time(dsdb, time_string, &secs1970);

            if (!_dsdb_insert_time_array_value(
                &att_times, &natt_times, secs1970)) {

                ERROR( DSDB_LIB_NAME,
                    "Could not get DSDOD attribute change times for: %s%s%s.%s\n"
                    " -> memory allocation error\n",
                    dsdod->site, dsdod->name, dsdod->facility, dsdod->level);

                if (att_times) free(att_times);

                dbres->free(dbres);
                return(-1);
            }
        }

        dbres->free(dbres);
    }

    /* Update the attribute times list in the DSDOD */

    if (dsdod->att_times) {
        free(dsdod->att_times);
    }

    dsdod->natt_times = natt_times;
    dsdod->att_times  = att_times;

    return(dsdod->natt_times);
}

/**
 *  Get the site/facility specific attributes for a DSDOD.
 *
 *  The DOD must be loaded before this function can be called
 *  (see dsdb_new_dsdod() and dsdb_get_dod()).
 *
 *  This function will only update attributes found in the DOD that
 *  have not been locked or have definition lock values of 2. All
 *  attributes updated by this function will have their definition
 *  lock value set to 2 (see cds_set_definition_lock()).
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  Null results from the database are not reported as errors.
 *  It is the responsibility of the calling process to report
 *  these as errors if necessary.
 *
 *  @param  dsdb  - pointer to the open database connection
 *  @param  dsdod - pointer to the DSDOD structure
 *
 *  @return
 *    -  number of attribute values updated
 *    -  0 if the database returned a NULL result
 *    - -1 if an error occurred
 */
int dsdb_get_dsdod_att_values(
    DSDB   *dsdb,
    DSDOD  *dsdod)
{
    DBStatus  status;
    DBResult *dbres;
    int       row;
    char     *var_name;
    char     *att_name;
    char     *att_type;
    char     *att_value;

    int att_count = 0;

    /* Get Global Attributes */

    status = dodog_get_ds_atts(dsdb->dbconn,
        dsdod->site, dsdod->facility, dsdod->name, dsdod->level, &dbres);

    if (status != DB_NO_ERROR) {
        if (status != DB_NULL_RESULT) {
            return(-1);
        }
    }
    else {

        for (row = 0; row < dbres->nrows; row++) {

            att_name  = DsAttName(dbres,row);
            att_type  = DsAttType(dbres,row);
            att_value = DsAttValue(dbres,row);

            if (!_dsdb_change_dsdod_att_value(dsdod->cds_group,
                NULL, att_name, att_type, att_value, 2)) {

                ERROR( DSDB_LIB_NAME,
                    "Could not get DSDOD attribute values for: %s%s%s.%s\n"
                    " -> memory allocation error\n",
                    dsdod->site, dsdod->name, dsdod->facility, dsdod->level);

                dbres->free(dbres);
                return(-1);
            }

            att_count++;
        }

        dbres->free(dbres);
    }

    /* Get Variable Attributes */

    status = dodog_get_ds_var_atts(dsdb->dbconn,
        dsdod->site, dsdod->facility, dsdod->name, dsdod->level, "%", &dbres);

    if (status != DB_NO_ERROR) {
        if (status != DB_NULL_RESULT) {
            return(-1);
        }
    }
    else {

        for (row = 0; row < dbres->nrows; row++) {

            var_name  = DsVarAttVar(dbres,row);
            att_name  = DsVarAttName(dbres,row);
            att_type  = DsVarAttType(dbres,row);
            att_value = DsVarAttValue(dbres,row);

            if (!_dsdb_change_dsdod_att_value(dsdod->cds_group,
                var_name, att_name, att_type, att_value, 2)) {

                ERROR( DSDB_LIB_NAME,
                    "Could not get DSDOD attribute values for: %s%s%s.%s\n"
                    " -> memory allocation error\n",
                    dsdod->site, dsdod->name, dsdod->facility, dsdod->level);

                dbres->free(dbres);
                return(-1);
            }

            att_count++;
        }

        dbres->free(dbres);
    }

    return(att_count);
}

/**
 *  Get the time varying attribute values for a DSDOD.
 *
 *  This function will load the time varying attribute values for
 *  a DSDOD using the data time specified in the DSDOD structure.
 *
 *  The DOD and site/facility specific attributes must be loaded
 *  before this function can be called (see dsdb_new_dsdod(),
 *  dsdb_get_dod() and dsdb_get_dsdod_att_values()).
 *
 *  This function will only update attributes found in the DOD that
 *  have not been locked or have definition lock values of 3. All
 *  attributes updated by this function will have their definition
 *  lock value set to 3 (see cds_set_definition_lock()).
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  dsdb  - pointer to the open database connection
 *  @param  dsdod - pointer to the DSDOD structure
 *
 *  @return
 *    -  number of attribute values updated
 *    -  0 if the database returned a NULL result
 *    - -1 if an error occurred
 */
int dsdb_get_dsdod_time_att_values(
    DSDB   *dsdb,
    DSDOD  *dsdod)
{
    DBStatus  status;
    DBResult *dbres;
    int       row;
    char     *var_name;
    char     *att_name;
    char     *att_type;
    char     *att_value;

    int att_count = 0;

    /* Get Datastream Global Time Attributes */

    status = dodog_get_ds_time_atts(dsdb->dbconn,
        dsdod->site, dsdod->facility, dsdod->name, dsdod->level,
        dsdod->data_time, &dbres);

    if (status != DB_NO_ERROR) {
        if (status != DB_NULL_RESULT) {
            return(-1);
        }
    }
    else {

        for (row = 0; row < dbres->nrows; row++) {

            att_name  = DsTimeAttName(dbres,row);
            att_type  = DsTimeAttType(dbres,row);
            att_value = DsTimeAttValue(dbres,row);

            if (!_dsdb_change_dsdod_att_value(dsdod->cds_group,
                NULL, att_name, att_type, att_value, 3)) {

                ERROR( DSDB_LIB_NAME,
                    "Could not get DSDOD attribute values for: %s%s%s.%s\n"
                    " -> memory allocation error\n",
                    dsdod->site, dsdod->name, dsdod->facility, dsdod->level);

                dbres->free(dbres);
                return(-1);
            }

            att_count++;
        }

        dbres->free(dbres);
    }

    /* Get Datastream Variable Attributes */

    status = dodog_get_ds_var_time_atts(dsdb->dbconn,
        dsdod->site, dsdod->facility, dsdod->name, dsdod->level, "%",
        dsdod->data_time, &dbres);

    if (status != DB_NO_ERROR) {
        if (status != DB_NULL_RESULT) {
            return(-1);
        }
    }
    else {

        for (row = 0; row < dbres->nrows; row++) {

            var_name  = DsVarTimeAttVar(dbres,row);
            att_name  = DsVarTimeAttName(dbres,row);
            att_type  = DsVarTimeAttType(dbres,row);
            att_value = DsVarTimeAttValue(dbres,row);

            if (!_dsdb_change_dsdod_att_value(dsdod->cds_group,
                var_name, att_name, att_type, att_value, 3)) {

                ERROR( DSDB_LIB_NAME,
                    "Could not get DSDOD attribute values for: %s%s%s.%s\n"
                    " -> memory allocation error\n",
                    dsdod->site, dsdod->name, dsdod->facility, dsdod->level);

                dbres->free(dbres);
                return(-1);
            }

            att_count++;
        }

        dbres->free(dbres);
    }

    return(att_count);
}

/**
 *  Get the highest DOD version for a datastream class.
 *
 *  The memory used by the output string is dynamically allocated.
 *  It is the responsibility of the calling process to free this memory
 *  when it is no longer needed.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  Null results from the database are not reported as errors.
 *  It is the responsibility of the calling process to report
 *  these as errors if necessary.
 *
 *  @param  dsdb        - pointer to the open database connection
 *  @param  dsc_name    - datastream class name
 *  @param  dsc_level   - datastream class level
 *  @param  dod_version - output: the highest dod version
 *
 *  @return
 *    -  1 if successful
 *    -  0 if the database returned a NULL result
 *    - -1 if an error occurred
 */
int dsdb_get_highest_dod_version(
    DSDB        *dsdb,
    const char  *dsc_name,
    const char  *dsc_level,
    char       **dod_version)
{
    DBStatus status;

    status = dodog_get_highest_dod_version(
        dsdb->dbconn, dsc_name, dsc_level, dod_version);

    if (status == DB_NO_ERROR) {
        return(1);
    }
    else if (status == DB_NULL_RESULT) {
        return(0);
    }

    return(-1);
}

/*@}*/
