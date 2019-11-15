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

/** @file ds_times.c
 *  Datastream Data Time Functions.
 */

#include "dsdb3.h"
#include "dbog_dsdb.h"

/**
 *  @defgroup DSDB_DSTIMES Datastream Times
 */
/*@{*/

/*******************************************************************************
 *  Private Functions
 */
/** @privatesection */

static DSTimes *_dsdb_create_ds_times(
    DSDB       *dsdb,
    const char *first,
    const char *last)
{
    DSTimes *ds_times = (DSTimes *)calloc(1, sizeof(DSTimes));

    if (!ds_times) {
        return((DSTimes *)NULL);
    }

    if (first) {
        dsdb_text_to_timeval(dsdb, first, &ds_times->first);
    }

    if (last) {
        dsdb_text_to_timeval(dsdb, last, &ds_times->last);
    }

    return(ds_times);
}

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Free all memory used by a DSTimes structure.
 *
 *  @param ds_times - pointer to the DSTimes structure
 */
void dsdb_free_ds_times(DSTimes *ds_times)
{
    if (ds_times) {
        free(ds_times);
    }
}

/**
 *  Delete process output datastream times from the database.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  dsdb      - pointer to the open database connection
 *  @param  site      - site name
 *  @param  facility  - facility name
 *  @param  proc_type - process type
 *  @param  proc_name - process name
 *  @param  dsc_name  - datastream class name
 *  @param  dsc_level - datastream class level
 *
 *  @return
 *    -  1 if process output datastream times were deleted
 *    -  0 if the process output datastream times were not found
 *    - -1 if an error occurred
 *
 *  @see  dsdb_get_status()
 */
int dsdb_delete_process_output_ds_times(
    DSDB       *dsdb,
    const char *site,
    const char *facility,
    const char *proc_type,
    const char *proc_name,
    const char *dsc_name,
    const char *dsc_level)
{
    DBStatus status;
    int      result;

    status = dsdbog_delete_process_output_datastream(dsdb->dbconn,
        proc_type, proc_name, dsc_name, dsc_level, site, facility, &result);

    if (status == DB_NO_ERROR) {
        return(result);
    }
    else if (status == DB_NULL_RESULT) {
        return(0);
    }

    return(-1);
}

/**
 *  Get process output datastream times from the database.
 *
 *  The memory used by the output structure is dynamically allocated.
 *  It is the responsibility of the calling process to free this memory
 *  when it is no longer needed (see dsdb_free_ds_times()).
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
 *  @param  proc_type - process type
 *  @param  proc_name - process name
 *  @param  dsc_name  - datastream class name
 *  @param  dsc_level - datastream class level
 *  @param  ds_times  - output: pointer to the DSTimes structure
 *
 *  @return
 *    -  1 if successful
 *    -  0 if the database returned a NULL result
 *    - -1 if an error occurred
 *
 *  @see  dsdb_free_ds_times()
 */
int dsdb_get_process_output_ds_times(
    DSDB        *dsdb,
    const char  *site,
    const char  *facility,
    const char  *proc_type,
    const char  *proc_name,
    const char  *dsc_name,
    const char  *dsc_level,
    DSTimes    **ds_times)
{
    DBStatus  status;
    DBResult *dbres;

    *ds_times = (DSTimes *)NULL;
    
    status = dsdbog_get_process_output_datastream(
        dsdb->dbconn, proc_type, proc_name,
        dsc_name, dsc_level, site, facility, &dbres);

    if (status == DB_NO_ERROR) {

        *ds_times = _dsdb_create_ds_times(
            dsdb,
            OutDsFirstTime(dbres,0),
            OutDsLastTime(dbres,0));

        dbres->free(dbres);

        if (!*ds_times) {

            ERROR( DSDB_LIB_NAME,
                "Could not get datastream times for: %s%s%s.%s\n"
                " -> memory allocation error\n",
                site, dsc_name, facility, dsc_level);

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
 *  Update process output datastream times.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  dsdb       - pointer to the open database connection
 *  @param  site       - site name
 *  @param  facility   - facility name
 *  @param  proc_type  - process type
 *  @param  proc_name  - process name
 *  @param  dsc_name   - datastream class name
 *  @param  dsc_level  - datastream class level
 *  @param  first_time - time of the first data sample
 *  @param  last_time  - time of the last data sample
 *
 *  @return
 *    -  1 if the datastream times were updated
 *    -  0 the database returned a null result
 *    - -1 if an error occurred
 */
int dsdb_update_process_output_ds_times(
    DSDB            *dsdb,
    const char      *site,
    const char      *facility,
    const char      *proc_type,
    const char      *proc_name,
    const char      *dsc_name,
    const char      *dsc_level,
    const timeval_t *first_time,
    const timeval_t *last_time)
{
    DBStatus  status;
    int       result;
    timeval_t tv_first;
    timeval_t tv_last;

    if (first_time && first_time->tv_sec) {
        tv_first = *first_time;
        if (last_time && last_time->tv_sec) {
            tv_last = *last_time;
        }
        else {
            tv_last = tv_first;
        }
    }
    else if (last_time && last_time->tv_sec) {
        tv_last  = *last_time;
        tv_first = tv_last;
    }
    else {
        return(1);
    }

    status = dsdbog_update_process_output_datastream(dsdb->dbconn,
        proc_type, proc_name, dsc_name, dsc_level, site, facility,
        &tv_first, &tv_last, &result);

    if (status == DB_NO_ERROR) {
        return(1);
    }
    else if (status == DB_NULL_RESULT) {
        return(0);
    }

    return(-1);
}

/*@}*/
