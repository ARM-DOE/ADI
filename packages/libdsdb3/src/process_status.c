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
*    $Revision: 6726 $
*    $Author: ermold $
*    $Date: 2011-05-17 03:48:03 +0000 (Tue, 17 May 2011) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file process_status.c
 *  Process Status Functions.
 */

#include "dsdb3.h"
#include "dbog_dsdb.h"

/**
 *  @defgroup DSDB_PROCSTATUS Process Statuses
 */
/*@{*/

/*******************************************************************************
 *  Private Functions
 */
/** @privatesection */

static ProcStatus *_dsdb_create_process_status(
    DSDB       *dsdb,
    const char *name,
    const char *text,
    const char *is_successful,
    const char *last_started,
    const char *last_completed,
    const char *last_successful)
{
    ProcStatus *proc_status = (ProcStatus *)calloc(1, sizeof(ProcStatus));

    if (!proc_status) {
        return((ProcStatus *)NULL);
    }

    if (name) {

        proc_status->name = msngr_copy_string(name);

        if (!proc_status->name) {
            free(proc_status);
            return((ProcStatus *)NULL);
        }
    }
    else {
        proc_status->name = (char *)NULL;
    }

    if (text) {

        proc_status->text = msngr_copy_string(text);

        if (!proc_status->text) {
            dsdb_free_process_status(proc_status);
            return((ProcStatus *)NULL);
        }
    }
    else {
        proc_status->text = (char *)NULL;
    }

    if (is_successful) {
        dsdb_text_to_bool(dsdb, is_successful, &proc_status->is_successful);
    }
    else {
        proc_status->is_successful = 0;
    }

    if (last_started) {
        dsdb_text_to_time(dsdb, last_started, &proc_status->last_started);
    }
    else {
        proc_status->last_started = 0;
    }

    if (last_completed) {
        dsdb_text_to_time(dsdb, last_completed, &proc_status->last_completed);
    }
    else {
        proc_status->last_completed = 0;
    }

    if (last_successful) {
        dsdb_text_to_time(dsdb, last_successful, &proc_status->last_successful);
    }
    else {
        proc_status->last_successful = 0;
    }

    return(proc_status);
}

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Free all memory used by a ProcStatus structure.
 *
 *  @param  proc_status - pointer to the ProcStatus structure
 */
void dsdb_free_process_status(ProcStatus *proc_status)
{
    if (proc_status) {
        if (proc_status->name) free(proc_status->name);
        if (proc_status->text) free(proc_status->text);
        free(proc_status);
    }
}

/**
 *  Delete a process status from the database.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  dsdb      - pointer to the open database connection
 *  @param  site      - site name
 *  @param  facility  - facility name
 *  @param  proc_type - process type
 *  @param  proc_name - process name
 *
 *  @return
 *    -  1 if the process status was deleted
 *    -  0 if the process status was not found in the database
 *    - -1 an error occurred
 */
int dsdb_delete_process_status(
    DSDB       *dsdb,
    const char *site,
    const char *facility,
    const char *proc_type,
    const char *proc_name)
{
    DBStatus status;
    int      result;

    status = dsdbog_delete_family_process_status(
        dsdb->dbconn, site, facility, proc_type, proc_name, &result);

    if (status == DB_NO_ERROR) {
        return(result);
    }
    else if (status == DB_NULL_RESULT) {
        return(0);
    }

    return(-1);
}

/**
 *  Get the process status from the database.
 *
 *  The memory used by the output structure is dynamically allocated.
 *  It is the responsibility of the calling process to free this memory
 *  when it is no longer needed (see dsdb_free_process_status()).
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  Null results from the database are not reported as errors.
 *  It is the responsibility of the calling process to report
 *  these as errors if necessary.
 *
 *  @param  dsdb        - pointer to the open database connection
 *  @param  site        - site name
 *  @param  facility    - facility name
 *  @param  proc_type   - process type
 *  @param  proc_name   - process name
 *  @param  proc_status - output: pointer to the ProcStatus structure
 *
 *  @return
 *    -  1 if successful
 *    -  0 if the database returned a NULL result
 *    - -1 if an error occurred
 *
 *  @see  dsdb_free_process_status()
 */
int dsdb_get_process_status(
    DSDB        *dsdb,
    const char  *site,
    const char  *facility,
    const char  *proc_type,
    const char  *proc_name,
    ProcStatus **proc_status)
{
    DBStatus  status;
    DBResult *dbres;

    *proc_status = (ProcStatus *)NULL;
    
    status = dsdbog_get_family_process_status(
        dsdb->dbconn, site, facility, proc_type, proc_name, &dbres);

    if (status == DB_NO_ERROR) {

        *proc_status = _dsdb_create_process_status(
            dsdb,
            StatusName(dbres,0),
            StatusText(dbres,0),
            StatusSuccessful(dbres,0),
            StatusLastStarted(dbres,0),
            StatusLastCompleted(dbres,0),
            StatusLastSuccessful(dbres,0));

        dbres->free(dbres);

        if (!*proc_status) {

            ERROR( DSDB_LIB_NAME,
                "Could not get process status for: %s%s %s %s\n"
                " -> memory allocation error\n",
                site, facility, proc_name, proc_type);

            return(-1);
        }

        return(1);
    }
    else if (status == DB_NULL_RESULT) {
        return(0);
    }

    return(-1);
}

/**
 *  Update the last started time for a process.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  dsdb         - pointer to the open database connection
 *  @param  site         - site name
 *  @param  facility     - facility name
 *  @param  proc_type    - process type
 *  @param  proc_name    - process name
 *  @param  started_time - time the process was started
 *                         (if 0 the current time will be used)
 *
 *  @return
 *    -  1 if the last started time was updated
 *    -  0 the database returned a null result
 *    - -1 if an error occurred
 */
int dsdb_update_process_started(
    DSDB       *dsdb,
    const char *site,
    const char *facility,
    const char *proc_type,
    const char *proc_name,
    time_t      started_time)
{
    DBStatus status;
    int      result;

    status = dsdbog_update_family_process_started(dsdb->dbconn,
        site, facility, proc_type, proc_name, started_time, &result);

    if (status == DB_NO_ERROR) {
        return(1);
    }
    else if (status == DB_NULL_RESULT) {
        return(0);
    }

    return(-1);
}

/**
 *  Update the last completed time for a process.
 *
 *  This function should only be used to update the last completed time
 *  for a process without also updating the status of the process
 *  (see dsdb_update_process_status()).
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  dsdb           - pointer to the open database connection
 *  @param  site           - site name
 *  @param  facility       - facility name
 *  @param  proc_type      - process type
 *  @param  proc_name      - process name
 *  @param  completed_time - time the process was completed
 *                           (if 0 the current time will be used)
 *
 *  @return
 *    -  1 if the last completed time was updated
 *    -  0 the database returned a null result
 *    - -1 if an error occurred
 */
int dsdb_update_process_completed(
    DSDB       *dsdb,
    const char *site,
    const char *facility,
    const char *proc_type,
    const char *proc_name,
    time_t      completed_time)
{
    DBStatus status;
    int      result;

    status = dsdbog_update_family_process_completed(dsdb->dbconn,
        site, facility, proc_type, proc_name, completed_time, &result);

    if (status == DB_NO_ERROR) {
        return(1);
    }
    else if (status == DB_NULL_RESULT) {
        return(0);
    }

    return(-1);
}

/**
 *  Update a process status in the database.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  dsdb           - pointer to the open database connection
 *  @param  site           - site name
 *  @param  facility       - facility name
 *  @param  proc_type      - process type
 *  @param  proc_name      - process name
 *  @param  proc_status    - process status
 *  @param  desc           - description of the status update
 *  @param  completed_time - time the process was completed
 *                           (if 0 the current time will be used)
 *
 *  @return
 *    -  1 if the process status was updated
 *    -  0 the database returned a null result
 *    - -1 if an error occurred
 */
int dsdb_update_process_status(
    DSDB       *dsdb,
    const char *site,
    const char *facility,
    const char *proc_type,
    const char *proc_name,
    const char *proc_status,
    const char *desc,
    time_t      completed_time)
{
    DBStatus status;
    int      result;

    status = dsdbog_update_family_process_status(dsdb->dbconn,
        site, facility, proc_type, proc_name,
        proc_status, desc, completed_time, &result);

    if (status == DB_NO_ERROR) {
        return(1);
    }
    else if (status == DB_NULL_RESULT) {
        return(0);
    }

    return(-1);
}

/*@}*/
