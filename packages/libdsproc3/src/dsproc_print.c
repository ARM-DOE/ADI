/*******************************************************************************
*
*  COPYRIGHT (C) 2012 Battelle Memorial Institute.  All Rights Reserved.
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
*    $Revision: 16061 $
*    $Author: ermold $
*    $Date: 2012-11-30 08:13:13 +0000 (Fri, 30 Nov 2012) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file dsproc_print.c
 *  Print Functions.
 */

#include "dsproc3.h"
#include "dsproc_private.h"

extern DSProc *_DSProc; /**< Internal DSProc structure */

/** @privatesection */

/*******************************************************************************
 *  Static Functions Visible Only To This Module
 */

/*******************************************************************************
 *  Private Functions Visible Only To This Library
 */

/** @publicsection */

/*******************************************************************************
 *  Internal Functions Visible To The Public
 */

/*******************************************************************************
 *  Public Functions
 */

/**
 *  Dump the contents of a dataset to a text file.
 *
 *  This function will dump the contents of a dataset to a text file with the
 *  following name:
 *
 *      prefix.YYYYMMDD.hhmmss.suffix
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  dataset   - pointer to the dataset
 *  @param  outdir    - the output directory
 *  @param  prefix    - the prefix to use for the file name,
 *                      or NULL to use the name of the dataset.
 *  @param  file_time - the time to use to create the file timestamp,
 *                      or 0 to use the first sample time in the dataset.
 *  @param  suffix    - the suffix portion of the file name
 *  @param  flags     - reserved for control flags
 *
 *  @return
 *    - 1 if successful
 *    - 0 if and error occurred
 */
int dsproc_dump_dataset(
    CDSGroup   *dataset,
    const char *outdir,
    const char *prefix,
    time_t      file_time,
    const char *suffix,
    int         flags)
{
    char        full_path[PATH_MAX];
    size_t      size;
    char       *chrp;
    size_t      nbytes;
    CDSGroup   *cds;
    size_t      ntimes;
    timeval_t   start_time;
    timeval_t   end_time;
    struct tm   gmt;
    FILE       *fp;

    chrp  = full_path;
    size  = PATH_MAX;

    /* output directory */

    if (outdir) {
        if (!make_path(outdir, 0775)) {
            return(0);
        }
    }
    else {
        outdir = ".";
    }

    /* prefix */

    if (prefix) {
        nbytes = snprintf(chrp, size, "%s/%s.", outdir, prefix);
    }
    else {
        nbytes = snprintf(chrp, size, "%s/%s.", outdir, dataset->name);
    }

    chrp += nbytes;
    size -= nbytes;

    /* timestamp */

    if (!file_time) {

        for (cds = dataset;;) {

            ntimes = dsproc_get_time_range(cds, &start_time, &end_time);

            if (!ntimes && cds->ngroups) {
                cds = cds->groups[0];
                continue;
            }

            break;
        }

        file_time = (ntimes) ? start_time.tv_sec : 0;
    }

    memset(&gmt, 0, sizeof(struct tm));
    gmtime_r(&file_time, &gmt);

    nbytes = snprintf(chrp, size, "%d%02d%02d.%02d%02d%02d",
        gmt.tm_year + 1900, gmt.tm_mon + 1, gmt.tm_mday,
        gmt.tm_hour, gmt.tm_min, gmt.tm_sec);

    chrp += nbytes;
    size -= nbytes;

    /* suffix */

    if (!suffix) {
        suffix = dataset->name;
    }

    nbytes = snprintf(chrp, size, ".%s", suffix);

    /* create the output file */

    DEBUG_LV1( DSPROC_LIB_NAME,
        "Creating dataset dump file:\n"
        " - dataset: %s\n"
        " - file:    %s\n",
        cds_get_object_path(dataset), full_path);

    fp = fopen(full_path, "w");
    if (!fp) {

        ERROR( DSPROC_LIB_NAME,
            "Could not create dataset dump file:\n"
            " -> file: %s\n"
            " -> %s\n",
            full_path, strerror(errno));

        dsproc_set_status(DSPROC_EFILEOPEN);
        return(0);
    }

    cds_print(fp, dataset, 0);
    fclose(fp);

    return(1);
}

/**
 *  Dump all output datasets to text files.
 *
 *  This function will dump all output datasets to a text file with the
 *  following names:
 *
 *      datastream.YYYYMMDD.hhmmss.suffix
 *
 *  where:
 *    - datastream = the output datastream name
 *    - YYYYMMDD   = the date of the first sample in the dataset
 *    - hhmmss     = the time of the first sample in the dataset
 *    - suffix     = the user specified suffix in the function call
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  outdir - the output directory
 *  @param  suffix - the suffix portion of the file name
 *  @param  flags  - reserved for control flags
 *
 *  @return
 *    - 1 if successful
 *    - 0 if and error occurred
 */
int dsproc_dump_output_datasets(
    const char *outdir,
    const char *suffix,
    int         flags)
{
    DataStream *ds;
    int         ds_id;
    int         status;

    for (ds_id = 0; ds_id < _DSProc->ndatastreams; ds_id++) {

        ds = _DSProc->datastreams[ds_id];

        if (ds->role == DSR_OUTPUT && ds->out_cds) {

            status = dsproc_dump_dataset(
                ds->out_cds, outdir, ds->name, 0, suffix, flags);

            if (status == 0) {
                return(0);
            }
        }
    }

    return(1);
}

/**
 *  Dump all retrieved datasets to a text file.
 *
 *  This function will dump all retrieved data to a text file with the
 *  following name:
 *
 *      {site}{process}{facility}.YYYYMMDD.hhmmss.suffix
 *
 *  where:
 *    - site      = the site name as specified on the command line
 *    - process   = the name of the process as defined in the database
 *    - facility  = the facility name as specified on the command line
 *    - YYYYMMDD  = the start date of the current processing interval
 *    - hhmmss    = the start time of the current processing interval
 *    - suffix    = the user specified suffix in the function call
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  outdir - the output directory
 *  @param  suffix - the suffix portion of the file name
 *  @param  flags  - reserved for control flags
 *
 *  @return
 *    - 1 if successful
 *    - 0 if and error occurred
 */
int dsproc_dump_retrieved_datasets(
    const char *outdir,
    const char *suffix,
    int         flags)
{
    CDSGroup *ret_data  = _DSProc->ret_data;
    time_t    file_time = _DSProc->interval_begin;
    char      prefix[256];
    int       status;

    snprintf(prefix, 256, "%s%s%s",
        _DSProc->site, _DSProc->name, _DSProc->facility);

    status = dsproc_dump_dataset(
        ret_data, outdir, prefix, file_time, suffix, flags);

    return(status);
}

/**
 *  Dump all transformed datasets to a text file.
 *
 *  This function will dump all transformed data to a text file with the
 *  following name:
 *
 *      {site}{process}{facility}.YYYYMMDD.hhmmss.suffix
 *
 *  where:
 *    - site      = the site name as specified on the command line
 *    - process   = the name of the process as defined in the database
 *    - facility  = the facility name as specified on the command line
 *    - YYYYMMDD  = the start date of the current processing interval
 *    - hhmmss    = the start time of the current processing interval
 *    - suffix    = the user specified suffix in the function call
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  outdir - the output directory
 *  @param  suffix - the suffix portion of the file name
 *  @param  flags  - reserved for control flags
 *
 *  @return
 *    - 1 if successful
 *    - 0 if and error occurred
 */
int dsproc_dump_transformed_datasets(
    const char *outdir,
    const char *suffix,
    int         flags)
{
    CDSGroup *trans_data = _DSProc->trans_data;
    time_t    file_time  = _DSProc->interval_begin;
    char      prefix[256];
    int       status;

    snprintf(prefix, 256, "%s%s%s",
        _DSProc->site, _DSProc->name, _DSProc->facility);

    status = dsproc_dump_dataset(
        trans_data, outdir, prefix, file_time, suffix, flags);

    return(status);
}
