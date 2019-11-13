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
*    $Revision: 54207 $
*    $Author: ermold $
*    $Date: 2014-05-02 20:50:34 +0000 (Fri, 02 May 2014) $
*
*******************************************************************************/

/** @file dbog_dod.h
 *  Header file for DOD object group functions.
 */

#ifndef _DBOG_DOD_H_
#define _DBOG_DOD_H_ 1

#include "dbconn.h"

/*@{*/
/** @privatesection */

/*******************************************************************************
*  Highest DOD Version
*/

DBStatus dodog_get_highest_dod_version(
    DBConn      *dbconn,
    const char  *dsc_name,
    const char  *dsc_level,
    char       **result);

/*******************************************************************************
*  DOD Dimensions
*/

DBStatus dodog_get_dod_dims(
    DBConn      *dbconn,
    const char  *dsc_name,
    const char  *dsc_level,
    const char  *dod_version,
    DBResult   **result);

#define DodDimName(dbres,row)   DB_RESULT(dbres,row,0)
#define DodDimLength(dbres,row) DB_RESULT(dbres,row,1)
#define DodDimOrder(dbres,row)  DB_RESULT(dbres,row,2)

/*******************************************************************************
*  DOD Attributes
*/

DBStatus dodog_get_dod_atts(
    DBConn      *dbconn,
    const char  *dsc_name,
    const char  *dsc_level,
    const char  *dod_version,
    DBResult   **result);

#define DodAttName(dbres,row)  DB_RESULT(dbres,row,0)
#define DodAttType(dbres,row)  DB_RESULT(dbres,row,1)
#define DodAttValue(dbres,row) DB_RESULT(dbres,row,2)
#define DodAttOrder(dbres,row) DB_RESULT(dbres,row,3)

/*******************************************************************************
*  DOD Variables
*/

DBStatus dodog_get_dod_vars(
    DBConn      *dbconn,
    const char  *dsc_name,
    const char  *dsc_level,
    const char  *dod_version,
    DBResult   **result);

#define DodVarName(dbres,row)  DB_RESULT(dbres,row,0)
#define DodVarType(dbres,row)  DB_RESULT(dbres,row,1)
#define DodVarOrder(dbres,row) DB_RESULT(dbres,row,2)

/*******************************************************************************
*  DOD Variable Dimensions
*/

DBStatus dodog_get_dod_var_dims(
    DBConn      *dbconn,
    const char  *dsc_name,
    const char  *dsc_level,
    const char  *dod_version,
    const char  *var_name,
    DBResult   **result);

#define DodVarDimVarName(dbres,row)  DB_RESULT(dbres,row,0)
#define DodVarDimName(dbres,row)     DB_RESULT(dbres,row,1)
#define DodVarDimLength(dbres,row)   DB_RESULT(dbres,row,2)
#define DodVarDimVarOrder(dbres,row) DB_RESULT(dbres,row,3)
#define DodVarDimOrder(dbres,row)    DB_RESULT(dbres,row,4)

/*******************************************************************************
*  DOD Variable Attributes
*/

DBStatus dodog_get_dod_var_atts(
    DBConn      *dbconn,
    const char  *dsc_name,
    const char  *dsc_level,
    const char  *dod_version,
    const char  *var_name,
    DBResult   **result);

#define DodVarAttVarName(dbres,row)  DB_RESULT(dbres,row,0)
#define DodVarAttName(dbres,row)     DB_RESULT(dbres,row,1)
#define DodVarAttType(dbres,row)     DB_RESULT(dbres,row,2)
#define DodVarAttValue(dbres,row)    DB_RESULT(dbres,row,3)
#define DodVarAttVarOrder(dbres,row) DB_RESULT(dbres,row,4)
#define DodVarAttOrder(dbres,row)    DB_RESULT(dbres,row,5)

/*******************************************************************************
*  Datastream DODs
*/

DBStatus dodog_get_ds_dod_versions(
    DBConn      *dbconn,
    const char  *site,
    const char  *facility,
    const char  *dsc_name,
    const char  *dsc_level,
    DBResult   **result);

#define DsDodTime(dbres,row)    DB_RESULT(dbres,row,0)
#define DsDodVersion(dbres,row) DB_RESULT(dbres,row,1)

/*******************************************************************************
*  Datastream Attributes
*/

DBStatus dodog_get_ds_atts(
    DBConn      *dbconn,
    const char  *site,
    const char  *facility,
    const char  *dsc_name,
    const char  *dsc_level,
    DBResult   **result);

#define DsAttName(dbres,row)  DB_RESULT(dbres,row,0)
#define DsAttType(dbres,row)  DB_RESULT(dbres,row,1)
#define DsAttValue(dbres,row) DB_RESULT(dbres,row,2)

/*******************************************************************************
*  Time Varying Datastream Attributes
*/

DBStatus dodog_get_ds_att_times(
    DBConn      *dbconn,
    const char  *site,
    const char  *facility,
    const char  *dsc_name,
    const char  *dsc_level,
    const char  *att_name,
    DBResult   **result);

#define DsAttTimeName(dbres,row)  DB_RESULT(dbres,row,0)
#define DsAttTimeTime(dbres,row)  DB_RESULT(dbres,row,1)


DBStatus dodog_get_ds_time_atts(
    DBConn     *dbconn,
    const char *site,
    const char *facility,
    const char *dsc_name,
    const char *dsc_level,
    time_t      att_time,
    DBResult  **result);

#define DsTimeAttName(dbres,row)  DB_RESULT(dbres,row,0)
#define DsTimeAttTime(dbres,row)  DB_RESULT(dbres,row,1)
#define DsTimeAttType(dbres,row)  DB_RESULT(dbres,row,2)
#define DsTimeAttValue(dbres,row) DB_RESULT(dbres,row,3)

/*******************************************************************************
*  Datastream Variable Attributes
*/

DBStatus dodog_get_ds_var_atts(
    DBConn      *dbconn,
    const char  *site,
    const char  *facility,
    const char  *dsc_name,
    const char  *dsc_level,
    const char  *var_name,
    DBResult   **result);

#define DsVarAttVar(dbres,row)   DB_RESULT(dbres,row,0)
#define DsVarAttName(dbres,row)  DB_RESULT(dbres,row,1)
#define DsVarAttType(dbres,row)  DB_RESULT(dbres,row,2)
#define DsVarAttValue(dbres,row) DB_RESULT(dbres,row,3)

/*******************************************************************************
*  Time Varying Datastream Variable Attributes
*/

DBStatus dodog_get_ds_var_att_times(
    DBConn     *dbconn,
    const char *site,
    const char *facility,
    const char *dsc_name,
    const char *dsc_level,
    const char *var_name,
    const char *att_name,
    DBResult  **result);

#define DsVarAttTimeVar(dbres,row)   DB_RESULT(dbres,row,0)
#define DsVarAttTimeName(dbres,row)  DB_RESULT(dbres,row,1)
#define DsVarAttTimeTime(dbres,row)  DB_RESULT(dbres,row,2)


DBStatus dodog_get_ds_var_time_atts(
    DBConn     *dbconn,
    const char *site,
    const char *facility,
    const char *dsc_name,
    const char *dsc_level,
    const char *var_name,
    time_t      att_time,
    DBResult  **result);

#define DsVarTimeAttVar(dbres,row)   DB_RESULT(dbres,row,0)
#define DsVarTimeAttName(dbres,row)  DB_RESULT(dbres,row,1)
#define DsVarTimeAttTime(dbres,row)  DB_RESULT(dbres,row,2)
#define DsVarTimeAttType(dbres,row)  DB_RESULT(dbres,row,3)
#define DsVarTimeAttValue(dbres,row) DB_RESULT(dbres,row,4)

/*******************************************************************************
*  Datastream Properties
*/

DBStatus dodog_get_ds_properties(
    DBConn     *dbconn,
    const char *dsc_name,
    const char *dsc_level,
    const char *site,
    const char *facility,
    const char *var_name,
    const char *prop_name,
    DBResult  **result);

#define DsPropDscName(dbres,row)   DB_RESULT(dbres,row,0)
#define DsPropDscLevel(dbres,row)  DB_RESULT(dbres,row,1)
#define DsPropSite(dbres,row)      DB_RESULT(dbres,row,2)
#define DsPropFac(dbres,row)       DB_RESULT(dbres,row,3)
#define DsPropVar(dbres,row)       DB_RESULT(dbres,row,4)
#define DsPropName(dbres,row)      DB_RESULT(dbres,row,5)
#define DsPropTime(dbres,row)      DB_RESULT(dbres,row,6)
#define DsPropValue(dbres,row)     DB_RESULT(dbres,row,7)

/*@}*/

#endif /* _DBOG_DOD_H_ */
