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
*    $Revision: 48761 $
*    $Author: ermold $
*    $Date: 2013-10-28 20:35:44 +0000 (Mon, 28 Oct 2013) $
*
*******************************************************************************/

/** @file dbog_dsdb.h
 *  Header file for DSDB object group functions.
 */

#ifndef _DBOG_DSDB_H_
#define _DBOG_DSDB_H_ 1

#include "dbconn.h"

/*@{*/
/** @privatesection */

/*******************************************************************************
*  Facilities
*/

DBStatus dsdbog_get_facility_location(
    DBConn      *dbconn,
    const char  *site,
    const char  *facility,
    DBResult   **result);

#define FacLat(dbres)   dbres->data[0]
#define FacLon(dbres)   dbres->data[1]
#define FacAlt(dbres)   dbres->data[2]
#define FacLoc(dbres)   dbres->data[3]

DBStatus dsdbog_get_site_description(
    DBConn      *dbconn,
    const char  *site,
    DBResult   **result);

#define SiteName(dbres)   dbres->data[0]
#define SiteDesc(dbres)   dbres->data[1]

/*******************************************************************************
*  Process Procedures
*/

/*  Process Config Values  */


DBStatus dsdbog_define_process_config_key(
    DBConn     *dbconn,
    const char *key,
    int        *result);

DBStatus dsdbog_delete_process_config_key(
    DBConn     *dbconn,
    const char *key,
    int        *result);

DBStatus dsdbog_delete_process_config_value(
    DBConn     *dbconn,
    const char *proc_type,
    const char *proc_name,
    const char *site,
    const char *facility,
    const char *key,
    int        *result);

DBStatus dsdbog_get_process_config_values(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    const char  *site,
    const char  *facility,
    const char  *key,
    DBResult   **result);

#define ProcConfType(dbres,row)  DB_RESULT(dbres,row,0)
#define ProcConfName(dbres,row)  DB_RESULT(dbres,row,1)
#define ProcConfSite(dbres,row)  DB_RESULT(dbres,row,2)
#define ProcConfFac(dbres,row)   DB_RESULT(dbres,row,3)
#define ProcConfKey(dbres,row)   DB_RESULT(dbres,row,4)
#define ProcConfValue(dbres,row) DB_RESULT(dbres,row,5)

DBStatus dsdbog_update_process_config_value(
    DBConn     *dbconn,
    const char *proc_type,
    const char *proc_name,
    const char *site,
    const char *facility,
    const char *key,
    const char *value,
    int        *result);

/*  Process Families  */


DBStatus dsdbog_inquire_process_families(
    DBConn      *dbconn,
    const char  *category,
    const char  *proc_class,
    const char  *site,
    const char  *facility,
    DBResult   **result);

#define ProcFamCat(dbres,row)   DB_RESULT(dbres,row,0)
#define ProcFamClass(dbres,row) DB_RESULT(dbres,row,1)
#define ProcFamSite(dbres,row)  DB_RESULT(dbres,row,2)
#define ProcFamFac(dbres,row)   DB_RESULT(dbres,row,3)
#define ProcFamLoc(dbres,row)   DB_RESULT(dbres,row,4)
#define ProcFamLat(dbres,row)   DB_RESULT(dbres,row,5)
#define ProcFamLon(dbres,row)   DB_RESULT(dbres,row,6)
#define ProcFamAlt(dbres,row)   DB_RESULT(dbres,row,7)


/*  Family Processes  */


DBStatus dsdbog_get_family_process(
    DBConn      *dbconn,
    const char  *site,
    const char  *facility,
    const char  *proc_type,
    const char  *proc_name,
    DBResult   **result);

DBStatus dsdbog_inquire_family_processes(
    DBConn      *dbconn,
    const char  *category,
    const char  *proc_class,
    const char  *site,
    const char  *facility,
    const char  *proc_type,
    const char  *proc_name,
    DBResult   **result);

#define FamProcCat(dbres,row)   DB_RESULT(dbres,row,0)
#define FamProcClass(dbres,row) DB_RESULT(dbres,row,1)
#define FamProcSite(dbres,row)  DB_RESULT(dbres,row,2)
#define FamProcFac(dbres,row)   DB_RESULT(dbres,row,3)
#define FamProcType(dbres,row)  DB_RESULT(dbres,row,4)
#define FamProcName(dbres,row)  DB_RESULT(dbres,row,5)

DBStatus dsdbog_get_family_process_location(
    DBConn      *dbconn,
    const char  *site,
    const char  *facility,
    const char  *proc_type,
    const char  *proc_name,
    DBResult   **result);

#define FamProcLat(dbres)   dbres->data[0]
#define FamProcLon(dbres)   dbres->data[1]
#define FamProcAlt(dbres)   dbres->data[2]
#define FamProcLoc(dbres)   dbres->data[3]


/*  Family Process States  */


DBStatus dsdbog_delete_family_process_state(
    DBConn     *dbconn,
    const char *site,
    const char *facility,
    const char *proc_type,
    const char *proc_name,
    int        *result);

DBStatus dsdbog_get_family_process_state(
    DBConn      *dbconn,
    const char  *site,
    const char  *facility,
    const char  *proc_type,
    const char  *proc_name,
    DBResult   **result);

DBStatus dsdbog_inquire_family_process_states(
    DBConn      *dbconn,
    const char  *category,
    const char  *proc_class,
    const char  *site,
    const char  *facility,
    const char  *proc_type,
    const char  *proc_name,
    DBResult   **result);

#define StateCat(dbres,row)      DB_RESULT(dbres,row,0)
#define StateClass(dbres,row)    DB_RESULT(dbres,row,1)
#define StateSite(dbres,row)     DB_RESULT(dbres,row,2)
#define StateFac(dbres,row)      DB_RESULT(dbres,row,3)
#define StateProcType(dbres,row) DB_RESULT(dbres,row,4)
#define StateProcName(dbres,row) DB_RESULT(dbres,row,5)
#define StateName(dbres,row)     DB_RESULT(dbres,row,6)
#define StateEnabled(dbres,row)  DB_RESULT(dbres,row,7)
#define StateText(dbres,row)     DB_RESULT(dbres,row,8)
#define StateTime(dbres,row)     DB_RESULT(dbres,row,9)

DBStatus dsdbog_is_family_process_enabled(
    DBConn     *dbconn,
    const char *site,
    const char *facility,
    const char *proc_type,
    const char *proc_name,
    int        *result);

DBStatus dsdbog_update_family_process_state(
    DBConn     *dbconn,
    const char *site,
    const char *facility,
    const char *proc_type,
    const char *proc_name,
    const char *state,
    const char *description,
    time_t      state_time,
    int        *result);


/*  Family Process Statuses  */


DBStatus dsdbog_delete_family_process_status(
    DBConn     *dbconn,
    const char *site,
    const char *facility,
    const char *proc_type,
    const char *proc_name,
    int        *result);

DBStatus dsdbog_get_family_process_status(
    DBConn      *dbconn,
    const char  *site,
    const char  *facility,
    const char  *proc_type,
    const char  *proc_name,
    DBResult   **result);

DBStatus dsdbog_inquire_family_process_statuses(
    DBConn      *dbconn,
    const char  *category,
    const char  *proc_class,
    const char  *site,
    const char  *facility,
    const char  *proc_type,
    const char  *proc_name,
    DBResult   **result);

#define StatusCat(dbres,row)            DB_RESULT(dbres,row,0)
#define StatusClass(dbres,row)          DB_RESULT(dbres,row,1)
#define StatusSite(dbres,row)           DB_RESULT(dbres,row,2)
#define StatusFac(dbres,row)            DB_RESULT(dbres,row,3)
#define StatusProcType(dbres,row)       DB_RESULT(dbres,row,4)
#define StatusProcName(dbres,row)       DB_RESULT(dbres,row,5)
#define StatusName(dbres,row)           DB_RESULT(dbres,row,6)
#define StatusSuccessful(dbres,row)     DB_RESULT(dbres,row,7)
#define StatusText(dbres,row)           DB_RESULT(dbres,row,8)
#define StatusLastStarted(dbres,row)    DB_RESULT(dbres,row,9)
#define StatusLastCompleted(dbres,row)  DB_RESULT(dbres,row,10)
#define StatusLastSuccessful(dbres,row) DB_RESULT(dbres,row,11)

DBStatus dsdbog_update_family_process_started(
    DBConn     *dbconn,
    const char *site,
    const char *facility,
    const char *proc_type,
    const char *proc_name,
    time_t      started_time,
    int        *result);

DBStatus dsdbog_update_family_process_completed(
    DBConn     *dbconn,
    const char *site,
    const char *facility,
    const char *proc_type,
    const char *proc_name,
    time_t      completed_time,
    int        *result);

DBStatus dsdbog_update_family_process_status(
    DBConn     *dbconn,
    const char *site,
    const char *facility,
    const char *proc_type,
    const char *proc_name,
    const char *status,
    const char *description,
    time_t      completed_time,
    int        *result);


/*  Process Input Datastream Classes  */


DBStatus dsdbog_get_process_input_ds_classes(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    DBResult   **result);

DBStatus dsdbog_inquire_process_input_ds_classes(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    const char  *dsc_name,
    const char  *dsc_level,
    DBResult   **result);

#define InDscProcType(dbres,row)  DB_RESULT(dbres,row,0)
#define InDscProcName(dbres,row)  DB_RESULT(dbres,row,1)
#define InDscName(dbres,row)      DB_RESULT(dbres,row,2)
#define InDscLevel(dbres,row)     DB_RESULT(dbres,row,3)


/*  Process Output Datastream Classes  */


DBStatus dsdbog_get_process_output_ds_classes(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    DBResult   **result);

DBStatus dsdbog_inquire_process_output_ds_classes(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    const char  *dsc_name,
    const char  *dsc_level,
    DBResult   **result);

#define OutDscProcType(dbres,row)  DB_RESULT(dbres,row,0)
#define OutDscProcName(dbres,row)  DB_RESULT(dbres,row,1)
#define OutDscName(dbres,row)      DB_RESULT(dbres,row,2)
#define OutDscLevel(dbres,row)     DB_RESULT(dbres,row,3)


/*  Process Output Datastreams  */


DBStatus dsdbog_delete_process_output_datastream(
    DBConn     *dbconn,
    const char *proc_type,
    const char *proc_name,
    const char *dsc_name,
    const char *dsc_level,
    const char *site,
    const char *facility,
    int        *result);

DBStatus dsdbog_get_process_output_datastream(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    const char  *dsc_name,
    const char  *dsc_level,
    const char  *site,
    const char  *facility,
    DBResult   **result);

DBStatus dsdbog_inquire_process_output_datastreams(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    const char  *dsc_name,
    const char  *dsc_level,
    const char  *site,
    const char  *facility,
    DBResult   **result);

#define OutDsProcType(dbres,row)   DB_RESULT(dbres,row,0)
#define OutDsProcName(dbres,row)   DB_RESULT(dbres,row,1)
#define OutDsClassName(dbres,row)  DB_RESULT(dbres,row,2)
#define OutDsClassLevel(dbres,row) DB_RESULT(dbres,row,3)
#define OutDsSite(dbres,row)       DB_RESULT(dbres,row,4)
#define OutDsFac(dbres,row)        DB_RESULT(dbres,row,5)
#define OutDsFirstTime(dbres,row)  DB_RESULT(dbres,row,6)
#define OutDsLastTime(dbres,row)   DB_RESULT(dbres,row,7)

DBStatus dsdbog_update_process_output_datastream(
    DBConn          *dbconn,
    const char      *proc_type,
    const char      *proc_name,
    const char      *dsc_name,
    const char      *dsc_level,
    const char      *site,
    const char      *facility,
    const timeval_t *first_time,
    const timeval_t *last_time,
    int             *result);

/*  Datastream Config Values  */


DBStatus dsdbog_define_datastream_config_key(
    DBConn     *dbconn,
    const char *key,
    int        *result);

DBStatus dsdbog_delete_datastream_config_key(
    DBConn     *dbconn,
    const char *key,
    int        *result);

DBStatus dsdbog_delete_datastream_config_value(
    DBConn     *dbconn,
    const char *dsc_type,
    const char *dsc_name,
    const char *site,
    const char *facility,
    const char *key,
    int        *result);

DBStatus dsdbog_get_datastream_config_values(
    DBConn      *dbconn,
    const char  *dsc_type,
    const char  *dsc_name,
    const char  *site,
    const char  *facility,
    const char  *key,
    DBResult   **result);

#define DSConfName(dbres,row)  DB_RESULT(dbres,row,0)
#define DSConfLevel(dbres,row) DB_RESULT(dbres,row,1)
#define DSConfSite(dbres,row)  DB_RESULT(dbres,row,2)
#define DSConfFac(dbres,row)   DB_RESULT(dbres,row,3)
#define DSConfKey(dbres,row)   DB_RESULT(dbres,row,4)
#define DSConfValue(dbres,row) DB_RESULT(dbres,row,5)

DBStatus dsdbog_update_datastream_config_value(
    DBConn     *dbconn,
    const char *dsc_type,
    const char *dsc_name,
    const char *site,
    const char *facility,
    const char *key,
    const char *value,
    int        *result);

/*@}*/

#endif /* _DBOG_DSDB_H_ */
