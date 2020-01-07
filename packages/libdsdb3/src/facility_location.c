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

/** @file facility_location.c
 *  Facility Location Functions.
 */

#include "dsdb3.h"
#include "dbog_dsdb.h"

/**
 *  @defgroup DSDB_FACLOC Facility Locations
 */
/*@{*/

/*******************************************************************************
 *  Private Functions
 */
/** @privatesection */

static FacLoc *_dsdb_create_facility_location(
    const char *name,
    const char *lat,
    const char *lon,
    const char *alt)
{
    FacLoc *fac_loc = (FacLoc *)calloc(1, sizeof(FacLoc));

    if (!fac_loc) {
        return((FacLoc *)NULL);
    }

    if (name) {

        fac_loc->name = msngr_copy_string(name);

        if (!fac_loc->name) {
            free(fac_loc);
            return((FacLoc *)NULL);
        }
    }
    else {
        fac_loc->name = (char *)NULL;
    }

    fac_loc->lat = (lat) ? (float)atof(lat) : 0;
    fac_loc->lon = (lon) ? (float)atof(lon) : 0;
    fac_loc->alt = (alt) ? (float)atof(alt) : 0;

    return(fac_loc);
}

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Free all memory used by a FacLoc structure.
 *
 *  @param  fac_loc - pointer to the FacLoc structure
 */
void dsdb_free_facility_location(FacLoc *fac_loc)
{
    if (fac_loc) {
        if (fac_loc->name) free(fac_loc->name);
        free(fac_loc);
    }
}

/**
 *  Get a facility location from the database.
 *
 *  The memory used by the output structure is dynamically allocated.
 *  It is the responsibility of the calling process to free this memory
 *  when it is no longer needed (see dsdb_free_facility_location()).
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  Null results from the database are not reported as errors.
 *  It is the responsibility of the calling process to report
 *  these as errors if necessary.
 *
 *  @param  dsdb     - pointer to the open database connection
 *  @param  site     - site name
 *  @param  facility - facility name
 *  @param  fac_loc  - output: pointer to the FacLoc structure
 *
 *  @return
 *    -  1 if successful
 *    -  0 if the database returned a NULL result
 *    - -1 if an error occurred
 *
 *  @see  dsdb_free_facility_location()
 */
int dsdb_get_facility_location(
    DSDB        *dsdb,
    const char  *site,
    const char  *facility,
    FacLoc     **fac_loc)
{
    DBStatus  status;
    DBResult *dbres;

    *fac_loc = (FacLoc *)NULL;

    status = dsdbog_get_facility_location(
        dsdb->dbconn, site, facility, &dbres);

    if (status == DB_NO_ERROR) {

        *fac_loc = _dsdb_create_facility_location(
            FacLoc(dbres),
            FacLat(dbres),
            FacLon(dbres),
            FacAlt(dbres));

        dbres->free(dbres);

        if (!*fac_loc) {

            ERROR( DSDB_LIB_NAME,
                "Could not get facility location for: %s%s\n"
                " -> memory allocation error\n",
                site, facility);

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
