/*******************************************************************************
*
*  COPYRIGHT (C) 2010 Battelle Memorial Institute.  All Rights Reserved.
*
********************************************************************************
*
*  Author:
*     name:  Krista Gaustad
*     phone: (509) 375-5950
*     email: krista.gaustad@pnl.gov
*
********************************************************************************
*
*  REPOSITORY INFORMATION:
*    $Revision: 12566 $
*    $Author: ermold $
*    $Date: 2012-02-05 03:08:12 +0000 (Sun, 05 Feb 2012) $
*
*******************************************************************************/

/** @file dbog_retriever.c
 *  Retriever object group functions.
 */

#include "dbconn.h"
#include <strings.h>

/** @privatesection */

/**
 *  Get all coordinate system dimensions defined for a process.
 *
 *  The returned rows will be sorted by coordinate system ID and then by
 *  dimension order. The following macros can be used to access the column
 *  values for each row:
 *
 *    - RetCoordDim_SystemID(dbres,row)
 *    - RetCoordDim_Order(dbres,row)
 *    - RetCoordDim_Name(dbres,row)
 *    - RetCoordDim_Interval(dbres,row)
 *    - RetCoordDim_Units(dbres,row)
 *    - RetCoordDIM_SubGroupID(dbres,row)
 *    - RetCoordDim_ID(dbres,row)
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @return  Database Query Status
 *    - DB_NO_ERROR
 *    - DB_NULL_RESULT
 *    - DB_BAD_RESULT
 *    - DB_MEM_ERROR
 *    - DB_ERROR
 */
DBStatus retog_get_coord_dims(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    DBResult   **result)
{
    const char *command = "SELECT * FROM get_ret_coord_dims_with_ids($1,$2)";
    const char *params[2];

    params[0] = proc_type;
    params[1] = proc_name;

    return(dbconn_query(dbconn, command, 2, params, result));
}

/**
 *  Get all coordinate systems defined for a process.
 *
 *  The returned rows will be sorted by coordinate system ID. The following
 *  macros can be used to access the column values for each row:
 *
 *    - RetCoordSystem_ID(dbres,row)
 *    - RetCoordSystem_Name(dbres,row)
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @return  Database Query Status
 *    - DB_NO_ERROR
 *    - DB_NULL_RESULT
 *    - DB_BAD_RESULT
 *    - DB_MEM_ERROR
 *    - DB_ERROR
 */
DBStatus retog_get_coord_systems(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    DBResult   **result)
{
    const char *command = "SELECT * FROM get_ret_coord_systems_with_ids($1,$2)";
    const char *params[2];

    params[0] = proc_type;
    params[1] = proc_name;

    return(dbconn_query(dbconn, command, 2, params, result));
}

/**
 *  Get all datastream variable names for all coordinate system dimensions
 *  defined for a process.
 *
 *  The returned rows will be sorted by coordinate system dimension ID,
 *  datastream ID, and then by variable name priority. The following
 *  macros can be used to access the column values for each row:
 *
 *    - RetCoordDimVar_CoordDimID(dbres,row)
 *    - RetCoordDimVar_DsID(dbres,row)
 *    - RetCoordDimVar_Priority(dbres,row)
 *    - RetCoordDimVar_Name(dbres,row)
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @return  Database Query Status
 *    - DB_NO_ERROR
 *    - DB_NULL_RESULT
 *    - DB_BAD_RESULT
 *    - DB_MEM_ERROR
 *    - DB_ERROR
 */
DBStatus retog_get_coord_var_names(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    DBResult   **result)
{
    const char *command = "SELECT * FROM get_ret_coord_var_names_with_ids($1,$2)";
    const char *params[2];

    params[0] = proc_type;
    params[1] = proc_name;

    return(dbconn_query(dbconn, command, 2, params, result));
}

/**
 *  Get all retriever datastreams defined for a process.
 *
 *  The returned rows will be sorted by subgroup ID and then by subgroup
 *  priority. The following macros can be used to access the column values
 *  for each row:
 *
 *    - RetDs_DsID(dbres,row)
 *    - RetDs_SubGroupID(dbres,row)
 *    - RetDs_SubGroupPriority(dbres,row)
 *    - RetDs_Name(dbres,row)
 *    - RetDs_Level(dbres,row)
 *    - RetDs_Site(dbres,row)
 *    - RetDs_Fac(dbres,row)
 *    - RetDs_SiteDep(dbres,row)
 *    - RetDs_FacDep(dbres,row)
 *    - RetDs_BegDateDep(dbres,row)
 *    - RetDs_EndDateDep(dbres,row)
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @return  Database Query Status
 *    - DB_NO_ERROR
 *    - DB_NULL_RESULT
 *    - DB_BAD_RESULT
 *    - DB_MEM_ERROR
 *    - DB_ERROR
 */
DBStatus retog_get_datastreams(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    DBResult   **result)
{
    const char *command = "SELECT * FROM get_ret_datastreams_with_ids($1,$2)";
    const char *params[2];

    params[0] = proc_type;
    params[1] = proc_name;

    return(dbconn_query(dbconn, command, 2, params, result));
}

/**
 *  Get all retriever groups and subgroups defined for a process.
 *
 *  The returned rows will be sorted by datastream group ID and then by
 *  subgroup order. The following macros can be used to access the column
 *  values for each row:
 *
 *    - Ret_GroupID(dbres,row)
 *    - Ret_GroupName(dbres,row)
 *    - Ret_SubGroupID(dbres,row)
 *    - Ret_SubGroupName(dbres,row)
 *    - Ret_SubGroupOrder(dbres,row)
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @return  Database Query Status
 *    - DB_NO_ERROR
 *    - DB_NULL_RESULT
 *    - DB_BAD_RESULT
 *    - DB_MEM_ERROR
 *    - DB_ERROR
 */
DBStatus retog_get_groups(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    DBResult   **result)
{
    const char *command = "SELECT * FROM get_ret_subgroups_with_ids($1,$2)";
    const char *params[2];

    params[0] = proc_type;
    params[1] = proc_name;

    return(dbconn_query(dbconn, command, 2, params, result));
}

/**
 *  Get transformation parameters for all coordinate systems
 *  defined for a process.
 *
 *  The returned rows will be sorted by coordinate system name.
 *  The following macros can be used to access the column values
 *  for each row:
 *
 *    - RetTransParams_Coorsys(dbres,row)
 *    - RetTransParams_Params(dbres,row)
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @return  Database Query Status
 *    - DB_NO_ERROR
 *    - DB_NULL_RESULT
 *    - DB_BAD_RESULT
 *    - DB_MEM_ERROR
 *    - DB_ERROR
 */
DBStatus retog_get_trans_params(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    DBResult   **result)
{
    const char *command = "SELECT * FROM get_ret_transform_params($1,$2)";
    const char *params[2];

    params[0] = proc_type;
    params[1] = proc_name;

    return(dbconn_query(dbconn, command, 2, params, result));
}

/**
 *  Get all retriever variables defined for a process.
 *
 *  The returned rows will be sorted by datastream group ID and then by
 *  variable ID. The following macros can be used to access the column
 *  values for each row:
 *
 *    - RetVar_VarID(dbres,row)
 *    - RetVar_Name(dbres,row)
 *    - RetVar_GroupID(dbres,row)
 *    - RetVar_CoordSystemID(dbres,row)
 *    - RetVar_Units(dbres,row)
 *    - RetVar_DataType(dbres,row)
 *    - RetVar_StartOffset(dbres,row)
 *    - RetVar_EndOffset(dbres,row)
 *    - RetVar_Max(dbres,row)
 *    - RetVar_Min(dbres,row)
 *    - RetVar_Delta(dbres,row)
 *    - RetVar_ReqToRun(dbres,row)
 *    - RetVar_QCFlag(dbres,row)
 *    - RetVar_QCReqToRun(dbres,row)
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @return  Database Query Status
 *    - DB_NO_ERROR
 *    - DB_NULL_RESULT
 *    - DB_BAD_RESULT
 *    - DB_MEM_ERROR
 *    - DB_ERROR
 */
DBStatus retog_get_variables(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    DBResult   **result)
{
    const char *command = "SELECT * FROM get_ret_variables_with_ids($1,$2)";
    const char *params[2];

    params[0] = proc_type;
    params[1] = proc_name;

    return(dbconn_query(dbconn, command, 2, params, result));
}

/**
 *  Get all dimension names defined for all retriever variables.
 *
 *  The returned rows will be sorted by variable ID and then by dimension
 *  order. The following macros can be used to access the column values
 *  for each row:
 *
 *    - RetVarDim_VarID(dbres,row)
 *    - RetVarDim_Order(dbres,row)
 *    - RetVarDim_Name(dbres,row)
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @return  Database Query Status
 *    - DB_NO_ERROR
 *    - DB_NULL_RESULT
 *    - DB_BAD_RESULT
 *    - DB_MEM_ERROR
 *    - DB_ERROR
 */
DBStatus retog_get_var_dims(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    DBResult   **result)
{
    const char *command = "SELECT * FROM get_ret_var_dims_with_ids($1,$2)";
    const char *params[2];

    params[0] = proc_type;
    params[1] = proc_name;

    return(dbconn_query(dbconn, command, 2, params, result));
}

/**
 *  Get all input variable names defined for all retriever variables.
 *
 *  The returned rows will be sorted by variable ID, datastream ID, and then
 *  by variable name priority. The following macros can be used to access the
 *  column values for each row:
 *
 *    - RetVarName_VarID(dbres,row)
 *    - RetVarName_DsID(dbres,row)
 *    - RetVarName_Priority(dbres,row)
 *    - RetVarName_Name(dbres,row)
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @return  Database Query Status
 *    - DB_NO_ERROR
 *    - DB_NULL_RESULT
 *    - DB_BAD_RESULT
 *    - DB_MEM_ERROR
 *    - DB_ERROR
 */
DBStatus retog_get_var_names(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    DBResult   **result)
{
    const char *command = "SELECT * FROM get_ret_var_names_with_ids($1,$2)";
    const char *params[2];

    params[0] = proc_type;
    params[1] = proc_name;

    return(dbconn_query(dbconn, command, 2, params, result));
}

/**
 *  Get all output datastreams and variables defined for all retriever variables.
 *
 *  The returned rows will be sorted by variable ID. The following macros can
 *  be used to access the column values for each row:
 *
 *    - RetVarOut_VarID(dbres,row)
 *    - RetVarOut_DsName(dbres,row)
 *    - RetVarOut_DsLevel(dbres,row)
 *    - RetVarOut_VarName(dbres,row)
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @return  Database Query Status
 *    - DB_NO_ERROR
 *    - DB_NULL_RESULT
 *    - DB_BAD_RESULT
 *    - DB_MEM_ERROR
 *    - DB_ERROR
 */
DBStatus retog_get_var_outputs(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    DBResult   **result)
{
    const char *command = "SELECT * FROM get_ret_var_outputs_with_ids($1,$2)";
    const char *params[2];

    params[0] = proc_type;
    params[1] = proc_name;

    return(dbconn_query(dbconn, command, 2, params, result));
}

/*******************************************************************************
********************************************************************************
*  Old Database Calls.
********************************************************************************
*******************************************************************************/


/******************************************************************************
*  Retriever DS VarNames
*/

DBStatus retrieverog_get_varnames(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    const char  *datastream_name,
    const char  *datastream_level,
    const char  *subgroup_name,
    const char  *site,
    const char  *facility,
    const char  *site_dependency,
    const char  *facility_dependency,
    time_t      begin_date_dependency,
    const char  *group_name,
    const char  *vargroup_name,
    DBResult   **result)
{
    const char *command = "SELECT * FROM get_ret_var_names($1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12)";
    const char *params[12];
    char        time_string[32];

    params[0] = proc_type;
    params[1] = proc_name;
    params[2] = datastream_name;
    params[3] = datastream_level;
    params[4] = subgroup_name;
    params[5] = site;
    params[6] = facility;
    params[7] = site_dependency;
    params[8] = facility_dependency;
    params[10] = group_name;
    params[11] = vargroup_name;


    if (begin_date_dependency) {
        params[9] = dbconn_time_to_text(dbconn, begin_date_dependency, time_string);
        if (!params[9]) {
            *result = 0;
            return(DB_ERROR);
        }
    }
    else {
        params[9] = (const char *)NULL;
    }

    return(dbconn_query(dbconn, command, 12, params, result));
}

/******************************************************************************
*  Retriever DS VarDimNames
*/
DBStatus retrieverog_get_vardimnames(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    const char  *datastream_name,
    const char  *datastream_level,
    const char  *subgroup_name,
    const char  *site,
    const char  *facility,
    const char  *site_dependency,
    const char  *facility_dependency,
    time_t      begin_date_dependency,
    const char  *coord_system_name,
    const char  *dimname,
    DBResult   **result)
{
    const char *command = "SELECT * FROM get_ret_var_dim_names($1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12)";
    const char *params[12];
    char        time_string[32];

    params[0] = proc_type;
    params[1] = proc_name;
    params[2] = datastream_name;
    params[3] = datastream_level;
    params[4] = subgroup_name;
    params[5] = site;
    params[6] = facility;
    params[7] = site_dependency;
    params[8] = facility_dependency;
    params[10] = coord_system_name;
    params[11] = dimname;

    if (begin_date_dependency) {
        params[9] = dbconn_time_to_text(dbconn, begin_date_dependency, time_string);
        if (!params[9]) {
            *result = 0;
            return(DB_ERROR);
        }
    }
    else {
        params[9] = (const char *)NULL;
    }


    return(dbconn_query(dbconn, command, 12, params, result));
}

/******************************************************************************
*  Retriever DS CoordDims
*/
DBStatus retrieverog_get_coord_dims(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    const char  *ret_coord_system_name,
    DBResult   **result)
{
    const char *command = "SELECT * FROM get_ret_coord_dims($1,$2,$3)";
    const char *params[3];

    params[0] = proc_type;
    params[1] = proc_name;
    params[2] = ret_coord_system_name;

    return(dbconn_query(dbconn, command, 3, params, result));
}

/******************************************************************************
*  Retriever DS VarGroups
*/

DBStatus retrieverog_get_vargroups(
    DBConn      *dbconn,
    const char  *ds_group_name,
    const char  *proc_type,
    const char  *proc_name,
    DBResult   **result)
{
    const char *command = "SELECT * FROM get_ret_var_groups($1,$2,$3)";
    const char *params[3];

    params[0] = ds_group_name;
    params[1] = proc_type;
    params[2] = proc_name;

    return(dbconn_query(dbconn, command, 3, params, result));
}

/******************************************************************************
*  Retriever DS Datastreams
*/

DBStatus retrieverog_get_datastreams(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    const char  *ds_subgroup_name,
    DBResult   **result)
{
    const char *command = "SELECT * FROM get_ret_datastreams($1,$2,$3)";
    const char *params[3];

    params[0] = proc_type;
    params[1] = proc_name;
    params[2] = ds_subgroup_name;

    return(dbconn_query(dbconn, command, 3, params, result));
}

/******************************************************************************
*  Retriever DS SubGroups by Group
*/

DBStatus retrieverog_get_ds_subgroups_by_group(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    const char  *group_name,
    DBResult   **result)
{
    const char *command = "SELECT * FROM get_ret_ds_subgroups_by_group($1,$2,$3)";
    const char *params[3];

    params[0] = proc_type;
    params[1] = proc_name;
    params[2] = group_name;

    return(dbconn_query(dbconn, command, 3, params, result));
}

/******************************************************************************
*  Retriever DS SubGroup Name
*/

DBStatus retrieverog_select_ds_subgroup_name(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    int          subgroup_id,
    char   **result)
{
    const char *command = "SELECT * FROM select_ret_ds_subgroup_name($1,$2,$3)";
    const char *params[3];
    char       pstring[1024];

    params[0] = proc_type;
    params[1] = proc_name;

    if (subgroup_id) {
        sprintf(pstring,"%d",subgroup_id);
        if (!params[2]) {
            *result = 0;
            return(DB_ERROR);
        }
        params[2]=(const char *)pstring;
    }
    else {
        params[2] = (const char *)NULL;
    }

    return(dbconn_query_text(dbconn, command, 3, params, result));
}

/******************************************************************************
*  Retriever DS SubGroups
*/

DBStatus retrieverog_get_ds_subgroups(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    DBResult   **result)
{
    const char *command = "SELECT * FROM get_ret_ds_subgroups($1,$2)";
    const char *params[2];

    params[0] = proc_type;
    params[1] = proc_name;

    return(dbconn_query(dbconn, command, 2, params, result));
}

/******************************************************************************
*  Retriever DS Groups
*/

DBStatus retrieverog_get_ds_groups(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    DBResult   **result)
{
    const char *command = "SELECT * FROM get_ret_ds_groups($1,$2)";
    const char *params[2];

    params[0] = proc_type;
    params[1] = proc_name;
    return(dbconn_query(dbconn, command, 2, params, result));
}

