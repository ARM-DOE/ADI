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

/** @file process_state.c
 *  Process State Functions.
 */

#include "dsdb3.h"
#include "dbog_dsdb.h"

/**
 *  @defgroup DSDB_PROCSTATES Process States
 */
/*@{*/

/*******************************************************************************
 *  Private Functions
 */
/** @privatesection */

static ProcState *_dsdb_create_process_state(
    DSDB       *dsdb,
    const char *name,
    const char *text,
    const char *is_enabled,
    const char *last_updated)
{
    ProcState *proc_state = (ProcState *)calloc(1, sizeof(ProcState));

    if (!proc_state) {
        return((ProcState *)NULL);
    }

    if (name) {

        proc_state->name = msngr_copy_string(name);

        if (!proc_state->name) {
            free(proc_state);
            return((ProcState *)NULL);
        }
    }
    else {
        proc_state->name = (char *)NULL;
    }

    if (text) {

        proc_state->text = msngr_copy_string(text);

        if (!proc_state->text) {
            dsdb_free_process_state(proc_state);
            return((ProcState *)NULL);
        }
    }
    else {
        proc_state->text = (char *)NULL;
    }

    if (is_enabled) {
        dsdb_text_to_bool(dsdb, is_enabled, &proc_state->is_enabled);
    }
    else {
        proc_state->is_enabled = 0;
    }

    if (last_updated) {
         dsdb_text_to_time(dsdb, last_updated, &proc_state->last_updated);
    }
    else {
        proc_state->last_updated = 0;
    }

    return(proc_state);
}

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Free all memory used by a ProcState structure.
 *
 *  @param  proc_state - pointer to the ProcState structure
 */
void dsdb_free_process_state(ProcState *proc_state)
{
    if (proc_state) {
        if (proc_state->name) free(proc_state->name);
        if (proc_state->text) free(proc_state->text);
        free(proc_state);
    }
}

/**
 *  Delete a process state from the database.
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
 *    -  1 if the process state was deleted
 *    -  0 if the process sate was not found in the database
 *    - -1 if an error occurred
 */
int dsdb_delete_process_state(
    DSDB       *dsdb,
    const char *site,
    const char *facility,
    const char *proc_type,
    const char *proc_name)
{
    DBStatus status;
    int      result;

    status = dsdbog_delete_family_process_state(dsdb->dbconn,
        site, facility, proc_type, proc_name, &result);

    if (status == DB_NO_ERROR) {
        return(result);
    }
    else if (status == DB_NULL_RESULT) {
        return(0);
    }

    return(-1);
}

/**
 *  Get the process state from the database.
 *
 *  The memory used by the output structure is dynamically allocated.
 *  It is the responsibility of the calling process to free this memory
 *  when it is no longer needed (see dsdb_free_process_state()).
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  Null results from the database are not reported as errors.
 *  It is the responsibility of the calling process to report
 *  these as errors if necessary.
 *
 *  @param  dsdb       - pointer to the open database connection
 *  @param  site       - site name
 *  @param  facility   - facility name
 *  @param  proc_type  - process type
 *  @param  proc_name  - process name
 *  @param  proc_state - output: pointer to the ProcState structure
 *
 *  @return
 *    -  1 if successful
 *    -  0 if the database returned a NULL result
 *    - -1 if an error occurred
 *
 *  @see  dsdb_free_process_state()
 */
int dsdb_get_process_state(
    DSDB        *dsdb,
    const char  *site,
    const char  *facility,
    const char  *proc_type,
    const char  *proc_name,
    ProcState  **proc_state)
{
    DBStatus  status;
    DBResult *dbres;

    *proc_state = (ProcState *)NULL;

    status = dsdbog_get_family_process_state(
        dsdb->dbconn, site, facility, proc_type, proc_name, &dbres);

    if (status == DB_NO_ERROR) {

        *proc_state = _dsdb_create_process_state(
            dsdb,
            StateName(dbres,0),
            StateText(dbres,0),
            StateEnabled(dbres,0),
            StateTime(dbres,0));

        dbres->free(dbres);

        if (!*proc_state) {

            ERROR( DSDB_LIB_NAME,
                "Could not get process state for: %s%s %s %s\n"
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
 *  Check if a process is enabled.
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
 *    -  1 if the process is enabled
 *    -  0 if the process is disabled or not found
 *    - -1 if an error occurred
 */
int dsdb_is_process_enabled(
    DSDB       *dsdb,
    const char *site,
    const char *facility,
    const char *proc_type,
    const char *proc_name)
{
    DBStatus status;
    int      result;

    status = dsdbog_is_family_process_enabled(dsdb->dbconn,
        site, facility, proc_type, proc_name, &result);

    if (status == DB_NO_ERROR) {
        return(result);
    }
    else if (status == DB_NULL_RESULT) {
        return(0);
    }

    return(-1);
}

/**
 *  Update a process state in the database.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  dsdb       - pointer to the open database connection
 *  @param  site       - site name
 *  @param  facility   - facility name
 *  @param  proc_type  - process type
 *  @param  proc_name  - process name
 *  @param  state      - process state
 *  @param  desc       - description for the state update
 *  @param  state_time - time of the state update
 *                       (if 0 the current time will be used)
 *
 *  @return
 *    -  1 if the process state was updated
 *    -  0 the database returned a null result
 *    - -1 if an error occurred
 */
int dsdb_update_process_state(
    DSDB       *dsdb,
    const char *site,
    const char *facility,
    const char *proc_type,
    const char *proc_name,
    const char *state,
    const char *desc,
    time_t      state_time)
{
    DBStatus status;
    int      result;

    status = dsdbog_update_family_process_state(dsdb->dbconn,
        site, facility, proc_type, proc_name,
        state, desc, state_time, &result);

    if (status == DB_NO_ERROR) {
        return(1);
    }
    else if (status == DB_NULL_RESULT) {
        return(0);
    }

    return(-1);
}

/*@}*/
