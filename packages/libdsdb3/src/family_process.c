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

/** @file family_process.c
 *  Family Process Functions.
 */

#include "dsdb3.h"
#include "dbog_dsdb.h"

/**
 *  @defgroup DSDB_FAMPROCS Family Processes
 */
/*@{*/

/*******************************************************************************
 *  Private Functions
 */
/** @privatesection */

static FamProc *_dsdb_create_family_process(
    const char *category,
    const char *proc_class,
    const char *site,
    const char *facility,
    const char *type,
    const char *name)
{
    FamProc *fam_proc = (FamProc *)calloc(1, sizeof(FamProc));

    if (!fam_proc) {
        return((FamProc *)NULL);
    }

    if (category) {
        fam_proc->category = msngr_copy_string(category);
        if (!fam_proc->category) {
            free(fam_proc);
            return((FamProc *)NULL);
        }
    }
    else {
        fam_proc->category = (char *)NULL;
    }

    if (proc_class) {
        fam_proc->proc_class = msngr_copy_string(proc_class);
        if (!fam_proc->proc_class) {
            dsdb_free_family_process(fam_proc);
            return((FamProc *)NULL);
        }
    }
    else {
        fam_proc->proc_class = (char *)NULL;
    }

    if (site) {
        fam_proc->site = msngr_copy_string(site);
        if (!fam_proc->site) {
            dsdb_free_family_process(fam_proc);
            return((FamProc *)NULL);
        }
    }
    else {
        fam_proc->site = (char *)NULL;
    }

    if (facility) {
        fam_proc->facility = msngr_copy_string(facility);
        if (!fam_proc->facility) {
            dsdb_free_family_process(fam_proc);
            return((FamProc *)NULL);
        }
    }
    else {
        fam_proc->facility = (char *)NULL;
    }

    if (type) {
        fam_proc->type = msngr_copy_string(type);
        if (!fam_proc->type) {
            dsdb_free_family_process(fam_proc);
            return((FamProc *)NULL);
        }
    }
    else {
        fam_proc->type = (char *)NULL;
    }

    if (name) {
        fam_proc->name = msngr_copy_string(name);
        if (!fam_proc->name) {
            dsdb_free_family_process(fam_proc);
            return((FamProc *)NULL);
        }
    }
    else {
        fam_proc->name = (char *)NULL;
    }

    return(fam_proc);
}

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Free all memory used by a FamProc structure.
 *
 *  @param  fam_proc - pointer to the FamProc structure
 */
void dsdb_free_family_process(FamProc *fam_proc)
{
    if (fam_proc) {
        if (fam_proc->category) free(fam_proc->category);
        if (fam_proc->proc_class) free(fam_proc->proc_class);
        if (fam_proc->site)     free(fam_proc->site);
        if (fam_proc->facility) free(fam_proc->facility);
        if (fam_proc->type)     free(fam_proc->type);
        if (fam_proc->name)     free(fam_proc->name);
        free(fam_proc);
    }
}

/**
 *  Get a family process from the database.
 *
 *  The memory used by the output structure is dynamically allocated.
 *  It is the responsibility of the calling process to free this memory
 *  when it is no longer needed (see dsdb_free_family_process()).
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
 *  @param  fam_proc  - output: pointer to the FamProc structure
 *
 *  @return
 *    -  1 if successful
 *    -  0 if the database returned a NULL result
 *    - -1 if an error occurred
 *
 *  @see  dsdb_free_family_process()
 */
int dsdb_get_family_process(
    DSDB        *dsdb,
    const char  *site,
    const char  *facility,
    const char  *proc_type,
    const char  *proc_name,
    FamProc    **fam_proc)
{
    DBStatus  status;
    DBResult *dbres;

    *fam_proc = (FamProc *)NULL;

    status = dsdbog_get_family_process(
        dsdb->dbconn, site, facility, proc_type, proc_name, &dbres);

    if (status == DB_NO_ERROR) {

        *fam_proc = _dsdb_create_family_process(
            FamProcCat(dbres,0),
            FamProcClass(dbres,0),
            FamProcSite(dbres,0),
            FamProcFac(dbres,0),
            FamProcType(dbres,0),
            FamProcName(dbres,0));

        dbres->free(dbres);

        if (!*fam_proc) {

            ERROR( DSDB_LIB_NAME,
                "Could not get family process for: %s%s %s %s\n"
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
 *  Free all memory used by an array of FamProc structures.
 *
 *  @param  fam_procs - pointer to the array of FamProc structures
 */
void dsdb_free_family_processes(FamProc **fam_procs)
{
    int row;

    if (fam_procs) {
        for (row = 0; fam_procs[row]; row++) {
            dsdb_free_family_process(fam_procs[row]);
        }
        free(fam_procs);
    }
}

/**
 *  Get a list of family processes from the database.
 *
 *  SQL reqular expressions can be used for all query arguments.
 *
 *  The memory used by the output array is dynamically allocated.
 *  It is the responsibility of the calling process to free this memory
 *  when it is no longer needed (see dsdb_free_family_processes()).
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  Null results from the database are not reported as errors.
 *  It is the responsibility of the calling process to report
 *  these as errors if necessary.
 *
 *  @param  dsdb      - pointer to the open database connection
 *  @param  category  - process category
 *  @param  proc_class - process class
 *  @param  site      - site name
 *  @param  facility  - facility name
 *  @param  proc_type - process type
 *  @param  proc_name - process name
 *  @param  fam_procs - output: pointer to the NULL terminated array
 *                              of FamProc structure pointers
 *
 *  @return
 *    -  1 if successful
 *    -  0 if the database returned a NULL result
 *    - -1 if an error occurred
 *
 *  @see  dsdb_free_family_processes()
 */
int dsdb_inquire_family_processes(
    DSDB         *dsdb,
    const char   *category,
    const char   *proc_class,
    const char   *site,
    const char   *facility,
    const char   *proc_type,
    const char   *proc_name,
    FamProc    ***fam_procs)
{
    DBStatus  status;
    DBResult *dbres;
    int       row;

    *fam_procs = (FamProc **)NULL;

    status = dsdbog_inquire_family_processes(dsdb->dbconn,
        category, proc_class, site, facility, proc_type, proc_name, &dbres);

    if (status == DB_NO_ERROR) {

        *fam_procs = (FamProc **)calloc(dbres->nrows + 1, sizeof(FamProc *));
        if (!*fam_procs) {

            ERROR( DSDB_LIB_NAME,
                "Could not get list of family processes\n"
                " -> memory allocation error\n");

            dbres->free(dbres);
            return(-1);
        }

        for (row = 0; row < dbres->nrows; row++) {

            (*fam_procs)[row] = _dsdb_create_family_process(
                FamProcCat(dbres,row),
                FamProcClass(dbres,row),
                FamProcSite(dbres,row),
                FamProcFac(dbres,row),
                FamProcType(dbres,row),
                FamProcName(dbres,row));

            if (!(*fam_procs)[row]) {

                ERROR( DSDB_LIB_NAME,
                    "Could not get list of family processes\n"
                    " -> memory allocation error\n");

                dsdb_free_family_processes(*fam_procs);
                *fam_procs = (FamProc **)NULL;

                dbres->free(dbres);
                return(-1);
            }
        }

        (*fam_procs)[dbres->nrows] = (FamProc *)NULL;

        dbres->free(dbres);
        return(1);
    }
    else if (status == DB_NULL_RESULT) {
        return(0);
    }

    return(-1);
}

/*@}*/
