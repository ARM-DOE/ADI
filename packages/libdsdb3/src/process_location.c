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
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file process_location.c
 *  Process Location Functions.
 */

#include "dsdb3.h"
#include "dbog_dsdb.h"

/**
 *  @defgroup DSDB_PROCLOCS Process Locations
 */
/*@{*/

/*******************************************************************************
 *  Private Functions
 */
/** @privatesection */

static ProcLoc *_dsdb_create_process_location(
    const char *name,
    const char *lat,
    const char *lon,
    const char *alt)
{
    ProcLoc *proc_loc = (ProcLoc *)calloc(1, sizeof(ProcLoc));

    if (!proc_loc) {
        return((ProcLoc *)NULL);
    }

    if (name) {

        proc_loc->name = msngr_copy_string(name);

        if (!proc_loc->name) {
            free(proc_loc);
            return((ProcLoc *)NULL);
        }
    }
    else {
        proc_loc->name = (char *)NULL;
    }

    proc_loc->lat = (lat) ? (float)atof(lat) : 0;
    proc_loc->lon = (lon) ? (float)atof(lon) : 0;
    proc_loc->alt = (alt) ? (float)atof(alt) : 0;

    return(proc_loc);
}

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Free all memory used by a ProcLoc structure.
 *
 *  @param  proc_loc - pointer to the ProcLoc structure
 */
void dsdb_free_process_location(ProcLoc *proc_loc)
{
    if (proc_loc) {
        if (proc_loc->name) free(proc_loc->name);
        free(proc_loc);
    }
}

/**
 *  Get a process location from the database.
 *
 *  The memory used by the output structure is dynamically allocated.
 *  It is the responsibility of the calling process to free this memory
 *  when it is no longer needed (see dsdb_free_process_location()).
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
 *  @param  proc_loc  - output: pointer to the ProcLoc structure
 *
 *  @return
 *    -  1 if successful
 *    -  0 if the database returned a NULL result
 *    - -1 if an error occurred
 *
 *  @see  dsdb_free_process_location()
 */
int dsdb_get_process_location(
    DSDB        *dsdb,
    const char  *site,
    const char  *facility,
    const char  *proc_type,
    const char  *proc_name,
    ProcLoc    **proc_loc)
{
    DBStatus  status;
    DBResult *dbres;

    *proc_loc = (ProcLoc *)NULL;
    
    status = dsdbog_get_family_process_location(
        dsdb->dbconn, site, facility, proc_type, proc_name, &dbres);

    if (status == DB_NO_ERROR) {

        *proc_loc = _dsdb_create_process_location(
            FamProcLoc(dbres),
            FamProcLat(dbres),
            FamProcLon(dbres),
            FamProcAlt(dbres));

        dbres->free(dbres);

        if (!*proc_loc) {

            ERROR( DSDB_LIB_NAME,
                "Could not get process location for: %s%s %s %s\n"
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
 *  Get a site description from the database.
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
 *  @param  dsdb  - pointer to the open database connection
 *  @param  site  - site name
 *  @param  desc  - output: site description
 *
 *  @return
 *    -  1 if successful
 *    -  0 if the database returned a NULL result
 *    - -1 if an error occurred
 */
int dsdb_get_site_description(
    DSDB        *dsdb,
    const char  *site,
    char       **desc)
{
    DBStatus  status;
    DBResult *dbres;

    *desc = (char *)NULL;

    status = dsdbog_get_site_description(dsdb->dbconn, site, &dbres);

    if (status == DB_NO_ERROR) {

        *desc = strdup(SiteDesc(dbres));

        dbres->free(dbres);

        if (!*desc) {

            ERROR( DSDB_LIB_NAME,
                "Could not get site description for: %s\n"
                " -> memory allocation error\n",
                site);

            return(-1);
        }

        return(1);
    }
    else if (status == DB_NULL_RESULT) {
        return(0);
    }

    return(-1);
}

/*@}*/
