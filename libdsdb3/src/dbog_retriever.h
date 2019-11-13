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
*    $Revision: 12610 $
*    $Author: ermold $
*    $Date: 2012-02-08 06:22:32 +0000 (Wed, 08 Feb 2012) $
*
*******************************************************************************/

/** @file dbog_retriever.h
 *  Header file for Retriever object group functions.
 */

#ifndef _DBOG_RETRIEVER_H_
#define _DBOG_RETRIEVER_H_ 1

#define MAXSTRING 1024

#include "dbconn.h"

/*@{*/
/** @privatesection */

/*******************************************************************************
*  Get all coordinate system dimensions defined for a process.
*/

DBStatus retog_get_coord_dims(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    DBResult   **result);

#define RetCoordDim_SystemID(dbres,row)   DB_RESULT(dbres,row,0)
#define RetCoordDim_Order(dbres,row)      DB_RESULT(dbres,row,1)
#define RetCoordDim_Name(dbres,row)       DB_RESULT(dbres,row,2)
#define RetCoordDim_Interval(dbres,row)   DB_RESULT(dbres,row,3)
#define RetCoordDim_Units(dbres,row)      DB_RESULT(dbres,row,4)
#define RetCoordDim_SubGroupID(dbres,row) DB_RESULT(dbres,row,5)
#define RetCoordDim_ID(dbres,row)         DB_RESULT(dbres,row,6)
#define RetCoordDim_DataType(dbres,row)   DB_RESULT(dbres,row,7)
#define RetCoordDim_TransType(dbres,row)  DB_RESULT(dbres,row,8)
#define RetCoordDim_TransRange(dbres,row) DB_RESULT(dbres,row,9)
#define RetCoordDim_TransAlign(dbres,row) DB_RESULT(dbres,row,10)
#define RetCoordDim_Start(dbres,row)      DB_RESULT(dbres,row,11)
#define RetCoordDim_Length(dbres,row)     DB_RESULT(dbres,row,12)

/*******************************************************************************
*  Get all coordinate systems defined for a process.
*/

DBStatus retog_get_coord_systems(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    DBResult   **result);

#define RetCoordSystem_ID(dbres,row)   DB_RESULT(dbres,row,0)
#define RetCoordSystem_Name(dbres,row) DB_RESULT(dbres,row,1)

/*******************************************************************************
*  Get all coordinate variable names for all coordinate system dimensions
*  defined for a process.
*/

DBStatus retog_get_coord_var_names(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    DBResult   **result);

#define RetCoordDimVar_CoordDimID(dbres,row) DB_RESULT(dbres,row,0)
#define RetCoordDimVar_DsID(dbres,row)       DB_RESULT(dbres,row,1)
#define RetCoordDimVar_Priority(dbres,row)   DB_RESULT(dbres,row,2)
#define RetCoordDimVar_Name(dbres,row)       DB_RESULT(dbres,row,3)

/*******************************************************************************
*  Get all retriever datastreams defined for a process.
*/

DBStatus retog_get_datastreams(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    DBResult   **result);

#define RetDs_SubGroupID(dbres,row)       DB_RESULT(dbres,row,0)
#define RetDs_SubGroupPriority(dbres,row) DB_RESULT(dbres,row,1)
#define RetDs_DsID(dbres,row)             DB_RESULT(dbres,row,2)
#define RetDs_Name(dbres,row)             DB_RESULT(dbres,row,3)
#define RetDs_Level(dbres,row)            DB_RESULT(dbres,row,4)
#define RetDs_Site(dbres,row)             DB_RESULT(dbres,row,5)
#define RetDs_Fac(dbres,row)              DB_RESULT(dbres,row,6)
#define RetDs_SiteDep(dbres,row)          DB_RESULT(dbres,row,7)
#define RetDs_FacDep(dbres,row)           DB_RESULT(dbres,row,8)
#define RetDs_BegDateDep(dbres,row)       DB_RESULT(dbres,row,9)
#define RetDs_EndDateDep(dbres,row)       DB_RESULT(dbres,row,10)

/*******************************************************************************
*  Get all retriever groups and subgroups defined for a process.
*/

DBStatus retog_get_groups(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    DBResult   **result);

#define Ret_GroupID(dbres,row)       DB_RESULT(dbres,row,0)
#define Ret_SubGroupOrder(dbres,row) DB_RESULT(dbres,row,1)
#define Ret_SubGroupID(dbres,row)    DB_RESULT(dbres,row,2)
#define Ret_GroupName(dbres,row)     DB_RESULT(dbres,row,3)
#define Ret_SubGroupName(dbres,row)  DB_RESULT(dbres,row,4)

/*******************************************************************************
*  Get transformation parameters for all coordinate systems
*  defined for a process.
*/

DBStatus retog_get_trans_params(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    DBResult   **result);

#define RetTransParams_Coordsys(dbres,row)  DB_RESULT(dbres,row,2)
#define RetTransParams_Params(dbres,row)   DB_RESULT(dbres,row,3)

/*******************************************************************************
*  Get all retriever variables defined for a process.
*/

DBStatus retog_get_variables(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    DBResult   **result);

#define RetVar_GroupID(dbres,row)       DB_RESULT(dbres,row,0)
#define RetVar_VarID(dbres,row)         DB_RESULT(dbres,row,1)
#define RetVar_Name(dbres,row)          DB_RESULT(dbres,row,2)
#define RetVar_CoordSystemID(dbres,row) DB_RESULT(dbres,row,3)
#define RetVar_Units(dbres,row)         DB_RESULT(dbres,row,4)
#define RetVar_DataType(dbres,row)      DB_RESULT(dbres,row,5)
#define RetVar_StartOffset(dbres,row)   DB_RESULT(dbres,row,6)
#define RetVar_EndOffset(dbres,row)     DB_RESULT(dbres,row,7)
#define RetVar_Max(dbres,row)           DB_RESULT(dbres,row,8)
#define RetVar_Min(dbres,row)           DB_RESULT(dbres,row,9)
#define RetVar_Delta(dbres,row)         DB_RESULT(dbres,row,10)
#define RetVar_ReqToRun(dbres,row)      DB_RESULT(dbres,row,11)
#define RetVar_QCFlag(dbres,row)        DB_RESULT(dbres,row,12)
#define RetVar_QCReqToRun(dbres,row)    DB_RESULT(dbres,row,13)

/*******************************************************************************
*  Get all dimension names names defined for all retriever variables.
*/

DBStatus retog_get_var_dims(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    DBResult   **result);

#define RetVarDim_VarID(dbres,row) DB_RESULT(dbres,row,0)
#define RetVarDim_Order(dbres,row) DB_RESULT(dbres,row,1)
#define RetVarDim_Name(dbres,row)  DB_RESULT(dbres,row,2)

/*******************************************************************************
*  Get all input variable names defined for all retriever variables.
*/

DBStatus retog_get_var_names(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    DBResult   **result);

#define RetVarName_VarID(dbres,row)    DB_RESULT(dbres,row,0)
#define RetVarName_DsID(dbres,row)     DB_RESULT(dbres,row,1)
#define RetVarName_Priority(dbres,row) DB_RESULT(dbres,row,2)
#define RetVarName_Name(dbres,row)     DB_RESULT(dbres,row,3)

/*******************************************************************************
*  Get all output datastreams and variables defined for all retriever variables.
*/

DBStatus retog_get_var_outputs(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    DBResult   **result);

#define RetVarOut_VarID(dbres,row)   DB_RESULT(dbres,row,0)
#define RetVarOut_DsName(dbres,row)  DB_RESULT(dbres,row,1)
#define RetVarOut_DsLevel(dbres,row) DB_RESULT(dbres,row,2)
#define RetVarOut_VarName(dbres,row) DB_RESULT(dbres,row,3)


/*******************************************************************************
*  Old database calls.
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
    DBResult   **result);

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
    DBResult   **result);

DBStatus retrieverog_get_coord_dims(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    const char  *ret_coord_system_name,
    DBResult   **result);

DBStatus retrieverog_get_vargroups(
    DBConn      *dbconn,
    const char  *ds_group_name,
    const char  *proc_type,
    const char  *proc_name,
    DBResult   **result);

DBStatus retrieverog_get_datastreams(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    const char  *ds_subgroup_name,
    DBResult   **result);

DBStatus retrieverog_select_ds_subgroup_name(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    int          subgroup_id,
    char        **result);

DBStatus retrieverog_get_ds_subgroups(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    DBResult   **result);

DBStatus retrieverog_get_ds_groups(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    DBResult   **result);

DBStatus retrieverog_get_ds_subgroups_by_group(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    const char  *group_name,
    DBResult   **result);

/* inquire_ds_groups returns, proc type, proc name, group name) */
#define DsGroupPtype(dbres,row)  DB_RESULT(dbres,row,0)
#define DsGroupPname(dbres,row)  DB_RESULT(dbres,row,1)
#define DsGroupGname(dbres,row)  DB_RESULT(dbres,row,2)

/* inquire_ds_subgroups returns, proc type, proc name, group name, subgroup name) */
#define DsGroupSubGname(dbres,row)  DB_RESULT(dbres,row,3)
#define DsGroupSubGorder(dbres,row)  DB_RESULT(dbres,row,4)

/* get_datastreams returns, datastream name, datastream level, site, facility, */
/* datastream subgroup priority, site dependency, facility dependency, begin date */
/* dependency, end date dependency */
#define DstreamName(dbres,row)  DB_RESULT(dbres,row,2)
#define DstreamLevel(dbres,row)  DB_RESULT(dbres,row,3)
#define DstreamSite(dbres,row)  DB_RESULT(dbres,row,5)
#define DstreamFac(dbres,row)  DB_RESULT(dbres,row,6)
#define DstreamSubPriority(dbres,row)  DB_RESULT(dbres,row,7)
#define DstreamSiteDep(dbres,row)  DB_RESULT(dbres,row,8)
#define DstreamFacDep(dbres,row)  DB_RESULT(dbres,row,9)
#define DstreamBegDateDep(dbres,row)  DB_RESULT(dbres,row,10)
#define DstreamEndDateDep(dbres,row)  DB_RESULT(dbres,row,11)

/* get_vargroups returns: vargroup name, vargroup data type, var group units,  */
/* sample period, start offset, end offset, max, min, delta, ndims, dims,      */
/* number of variable names for the group, flag indicating whether field is    */
/* required for the vap to run, flag indicating whehter the qc field should be */
/* retrieved, and a flag indicating whether the qc fields is required for the  */
/* vap to run.                                                                 */
#define VGgroupName(dbres,row)  DB_RESULT(dbres,row,0)
#define VGvargroupName(dbres,row)  DB_RESULT(dbres,row,3)
#define VGCoordSystemName(dbres,row)  DB_RESULT(dbres,row,4)
#define VGgroupUnits(dbres,row)  DB_RESULT(dbres,row,5)
#define VGgroupDataType(dbres,row)  DB_RESULT(dbres,row,6)
#define VGgroupStartOffset(dbres,row)  DB_RESULT(dbres,row,7)
#define VGgroupEndOffset(dbres,row)  DB_RESULT(dbres,row,8)
#define VGgroupMax(dbres,row)  DB_RESULT(dbres,row,9)
#define VGgroupMin(dbres,row)  DB_RESULT(dbres,row,10)
#define VGgroupDelta(dbres,row)  DB_RESULT(dbres,row,11)
#define VGgroupReqRunFlg(dbres,row)  DB_RESULT(dbres,row,12)
#define VGgroupQCFlg(dbres,row)  DB_RESULT(dbres,row,13)
#define VGgroupQCReqRunFlag(dbres,row)  DB_RESULT(dbres,row,14)

/* get_varnames returns: process type, process name, datastream class     */
/* datastream level, datastream group name, datastream subgroup name,     */
/* site,facility,variable group name,variable name and variable priority  */
#define RetVname(dbres,row)    DB_RESULT(dbres,row,12)
#define RetVpriority(dbres,row)  DB_RESULT(dbres,row,13)

/* get_vardimnames returns: process type, process name, datastream class     */
/* datastream level, datastream subgroup name,     */
/* site,facility,coordinate system name,,dimension name and dimension priority  */
#define RetVarDname(dbres,row)    DB_RESULT(dbres,row,12)
#define RetVarDpriority(dbres,row)  DB_RESULT(dbres,row,13)

/* get_coord_dims returns: process type, process name, ret_coord_system_name */
/* dim_name level, ret_dim_order, subgroup name,dim_interval, and dim_unts     */
#define RetCDimDimName(dbres,row)  DB_RESULT(dbres,row,3)
#define RetCDimDimOrder(dbres,row)  DB_RESULT(dbres,row,4)
#define RetCDimSubgroupName(dbres,row)  DB_RESULT(dbres,row,5)
#define RetCDimInterval(dbres,row)  DB_RESULT(dbres,row,6)
#define RetCDimUnits(dbres,row)  DB_RESULT(dbres,row,7)
/*@}*/

#endif /* _DBOG_RETRIEVER_H_ */
