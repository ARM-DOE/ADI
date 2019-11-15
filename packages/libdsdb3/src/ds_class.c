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

/** @file ds_class.c
 *  Datastream Class Functions.
 */

#include "dsdb3.h"
#include "dbog_dsdb.h"

/**
 *  @defgroup DSDB_DSCLASS Datastream Classes
 */
/*@{*/

/*******************************************************************************
 *  Private Functions
 */
/** @privatesection */

static void _dsdb_destroy_ds_class(DSClass *ds_class)
{
    if (ds_class) {
        if (ds_class->name)  free((void *)ds_class->name);
        if (ds_class->level) free((void *)ds_class->level);
        free(ds_class);
    }
}

static DSClass *_dsdb_create_ds_class(
    const char *name,
    const char *level)
{
    DSClass *ds_class = (DSClass *)calloc(1, sizeof(DSClass));

    if (!ds_class) {
        return((DSClass *)NULL);
    }

    if (name) {

        ds_class->name = msngr_copy_string(name);

        if (!ds_class->name) {
            free(ds_class);
            return((DSClass *)NULL);
        }
    }
    else {
        ds_class->name = (char *)NULL;
    }

    if (level) {

        ds_class->level = msngr_copy_string(level);

        if (!ds_class->level) {
            _dsdb_destroy_ds_class(ds_class);
            return((DSClass *)NULL);
        }
    }
    else {
        ds_class->level = (char *)NULL;
    }

    return(ds_class);
}

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Free all memory used by an array of DSClass structures.
 *
 *  @param  ds_classes - pointer to the array of DSClass structures
 */
void dsdb_free_ds_classes(DSClass **ds_classes)
{
    int row;

    if (ds_classes) {
        for (row = 0; ds_classes[row]; row++) {
            _dsdb_destroy_ds_class(ds_classes[row]);
        }
        free(ds_classes);
    }
}

/**
 *  Get the input datastream classes for a process.
 *
 *  The memory used by the output array is dynamically allocated.
 *  It is the responsibility of the calling process to free this memory
 *  when it is no longer needed (see dsdb_free_ds_classes()).
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  Null results from the database are not reported as errors.
 *  It is the responsibility of the calling process to report
 *  these as errors if necessary.
 *
 *  @param  dsdb       - pointer to the open database connection
 *  @param  proc_type  - process type
 *  @param  proc_name  - process name
 *  @param  ds_classes - output: pointer to the NULL terminated array
 *                               of DSClass structure pointers
 *
 *  @return
 *    -  number of input datastream classes
 *    -  0 if the database returned a NULL result
 *    - -1 if an error occurred
 *
 *  @see  dsdb_free_ds_classes()
 */
int dsdb_get_process_dsc_inputs(
    DSDB         *dsdb,
    const char   *proc_type,
    const char   *proc_name,
    DSClass    ***ds_classes)
{
    DBStatus  status;
    DBResult *dbres;
    int       nds_classes;
    int       row;

    nds_classes = 0;
    *ds_classes = (DSClass **)NULL;

    status = dsdbog_get_process_input_ds_classes(
        dsdb->dbconn, proc_type, proc_name, &dbres);

    if (status == DB_NO_ERROR) {

        *ds_classes = (DSClass **)calloc(dbres->nrows + 1, sizeof(DSClass *));
        if (!*ds_classes) {

            ERROR( DSDB_LIB_NAME,
                "Could not get list of input datastream classes for: %s %s\n"
                " -> memory allocation error\n",
                proc_name, proc_type);

            dbres->free(dbres);
            return(-1);
        }

        for (row = 0; row < dbres->nrows; row++) {

            (*ds_classes)[row] = _dsdb_create_ds_class(
                InDscName(dbres,row),
                InDscLevel(dbres,row));

            if (!(*ds_classes)[row]) {

                ERROR( DSDB_LIB_NAME,
                    "Could not get list of input datastream classes for: %s %s\n"
                    " -> memory allocation error\n",
                    proc_name, proc_type);

                dsdb_free_ds_classes(*ds_classes);
                *ds_classes = (DSClass **)NULL;

                dbres->free(dbres);
                return(-1);
            }

            nds_classes++;
        }

        (*ds_classes)[dbres->nrows] = (DSClass *)NULL;

        dbres->free(dbres);
        return(nds_classes);
    }
    else if (status == DB_NULL_RESULT) {
        return(0);
    }

    return(-1);
}

/**
 *  Get the output datastream classes for a process.
 *
 *  The memory used by the output array is dynamically allocated.
 *  It is the responsibility of the calling process to free this memory
 *  when it is no longer needed (see dsdb_free_ds_classes()).
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  Null results from the database are not reported as errors.
 *  It is the responsibility of the calling process to report
 *  these as errors if necessary.
 *
 *  @param  dsdb       - pointer to the open database connection
 *  @param  proc_type  - process type
 *  @param  proc_name  - process name
 *  @param  ds_classes - output: pointer to the NULL terminated array
 *                               of DSClass structure pointers
 *
 *  @return
 *    -  number of output datastream classes
 *    -  0 if the database returned a NULL result
 *    - -1 if an error occurred
 *
 *  @see  dsdb_free_ds_classes()
 */
int dsdb_get_process_dsc_outputs(
    DSDB         *dsdb,
    const char   *proc_type,
    const char   *proc_name,
    DSClass    ***ds_classes)
{
    DBStatus  status;
    DBResult *dbres;
    int       nds_classes;
    int       row;

    nds_classes = 0;
    *ds_classes = (DSClass **)NULL;

    status = dsdbog_get_process_output_ds_classes(
        dsdb->dbconn, proc_type, proc_name, &dbres);

    if (status == DB_NO_ERROR) {

        *ds_classes = (DSClass **)calloc(dbres->nrows + 1, sizeof(DSClass *));
        if (!*ds_classes) {

            ERROR( DSDB_LIB_NAME,
                "Could not get list of output datastream classes for: %s %s\n"
                " -> memory allocation error\n",
                proc_name, proc_type);

            dbres->free(dbres);
            return(-1);
        }

        for (row = 0; row < dbres->nrows; row++) {

            (*ds_classes)[row] = _dsdb_create_ds_class(
                OutDscName(dbres,row),
                OutDscLevel(dbres,row));

            if (!(*ds_classes)[row]) {

                ERROR( DSDB_LIB_NAME,
                    "Could not get list of output datastream classes for: %s %s\n"
                    " -> memory allocation error\n",
                    proc_name, proc_type);

                dsdb_free_ds_classes(*ds_classes);
                *ds_classes = (DSClass **)NULL;

                dbres->free(dbres);
                return(-1);
            }

            nds_classes++;
        }

        (*ds_classes)[dbres->nrows] = (DSClass *)NULL;

        dbres->free(dbres);
        return(nds_classes);
    }
    else if (status == DB_NULL_RESULT) {
        return(0);
    }

    return(-1);
}

/*@}*/
