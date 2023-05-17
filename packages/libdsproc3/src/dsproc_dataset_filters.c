/*******************************************************************************
*
*  Copyright © 2014, Battelle Memorial Institute
*  All rights reserved.
*
********************************************************************************
*
*  Author:
*     name:  Brian Ermold
*     phone: (509) 375-2277
*     email: brian.ermold@pnl.gov
*
*******************************************************************************/

/** @file dsproc_dataset_filters.c
 *  Dataset Filtering Functions.
 */

#include <string.h>
#include <math.h>

#include "dsproc3.h"
#include "dsproc_private.h"

extern DSProc *_DSProc; /**< Internal DSProc structure */

/** @privatesection */

/*******************************************************************************
 *  Static Data and Functions Visible Only To This Module
 */

/** Flag used to disable the NaN filtering warnings. */
int gDisableNanFilterWarnings = 0;

/** Flag used to allow overlapping records to be filtered. */
int gFilterOverlaps = 0;

/*******************************************************************************
 *  Private Functions Visible Only To This Library
 */

/**
 *  Compare all samples in dataset1 with the samples in dataset2.
 *
 *  This function assumes that the time values have already been compared.
 *
 *  @param  dataset1 - pointer to dataset 1
 *  @param  start1   - start sample in dataset 1
 *  @param  dataset2 - pointer to dataset 2
 *  @param  start2   - start sample in dataset 2
 *  @param  count    - number of samples to compare
 *
 *  @return
 *      -  1 if all samples have identical data values
 *      -  0 if differences were found
 */
int _dsproc_compare_samples(
    CDSGroup *dataset1,
    size_t    start1,
    CDSGroup *dataset2,
    size_t    start2,
    size_t    count)
{
    CDSDim *time_dim1 = cds_get_dim(dataset1, "time");
    CDSDim *time_dim2 = cds_get_dim(dataset2, "time");
    CDSVar *var1;
    CDSVar *var2;
    size_t  var1_count;
    size_t  var2_count;
    void   *data1;
    void   *data2;
    size_t  sample_size;
    size_t  nbytes;
    int     vi;

    /* Make sure both datasets have a time dimension */

    if (!time_dim1 || !time_dim2) {
        if (time_dim1 || time_dim2) {
            return(0);
        }
        return(1);
    }

    /* Loop over all variables in dataset1 */

    for (vi = 0; vi < dataset1->nvars; vi++) {

        /* Check if this variable has the time dimension
         * and data for the samples to compare */

        var1 = dataset1->vars[vi];
        if ((var1->dims[0]      != time_dim1) ||
            (var1->sample_count <= start1)) {

            continue;
        }

        /* Skip the time variables */

        if ((strcmp(var1->name, "time") == 0) ||
            (strcmp(var1->name, "time_offset") == 0)) {

            continue;
        }

        /* Make sure dataset2 has this variable */

        var2 = cds_get_var(dataset2, var1->name);
        if (!var2) return(0);

        /* Make sure the variable in dataset2 has the time dimension
         * and data for the samples to compare */

        if ((var2->dims[0]      != time_dim2) ||
            (var2->sample_count <= start2)) {

            return(0);
        }

        /* Make sure the number of samples to compare from dataset1
         * is less than or equal to the number of samples that exist
         * in dataset2. */

        var1_count = var1->sample_count - start1;
        if (var1_count > count) var1_count = count;

        var2_count = var2->sample_count - start2;
        if (var2_count > count) var2_count = count;

        if (var1_count > var2_count) return(0);

        /* Make sure the data types match */

        if (var1->type != var2->type) return(0);

        /* Make sure the sample sizes match */

        sample_size = cds_var_sample_size(var1);
        if (sample_size != cds_var_sample_size(var2)) return(0);

        if (!sample_size) continue;

        /* Compare the data values */

        nbytes = sample_size * cds_data_type_size(var1->type);

        data1 = var1->data.bp + (start1 * nbytes);
        data2 = var2->data.bp + (start2 * nbytes);

        if (memcmp(data1, data2, var1_count * nbytes) != 0) return(0);

    } /* end loop over variables in dataset1 */

    return(1);
}

/**
 *  Remove samples from a dataset.
 *
 *  @param  ntimes  - input/output: number of times in the dataset
 *  @param  times   - input/output: array of times in the dataset
 *  @param  mask    - array of flags indicating the samples to remove
 *  @param  dataset - pointer to the dataset
 */
void _dsproc_delete_samples(
    size_t    *ntimes,
    timeval_t *times,
    int       *mask,
    CDSGroup  *dataset)
{
    CDSDim    *time_dim = cds_get_dim(dataset, "time");
    CDSVar    *var;
    size_t     nsamples;
    size_t     nbytes;
    void      *data1;
    void      *data2;
    timeval_t *time1;
    timeval_t *time2;
    int        vi;
    size_t     ti;

    /* Delete the flagged samples */

    for (vi = 0; vi < dataset->nvars; vi++) {

        /* Check if this variable has the time dimension
         * and data defined for it */

        var = dataset->vars[vi];
        if ((var->dims[0]      != time_dim) ||
            (var->sample_count == 0)) {

            continue;
        }

        /* Delete the flagged samples */

        nbytes = cds_var_sample_size(var) * cds_data_type_size(var->type);
        if (nbytes == 0) continue;

        data1    = var->data.bp;
        data2    = var->data.bp;
        nsamples = (var->sample_count < *ntimes) ? var->sample_count : *ntimes;

        for (ti = 0; ti < nsamples; ti++) {

            if (mask[ti]) {
                var->sample_count -= 1;
            }
            else {

                if (data1 != data2) {
                    memcpy(data1, data2, nbytes);
                }

                data1 += nbytes;
            }

            data2 += nbytes;
        }

    } /* end loop over variables in dataset1 */

    /* Delete the flagged times */

    time1 = times;
    time2 = times;

    nsamples = 0;

    for (ti = 0; ti < *ntimes; ti++) {

        if (!mask[ti]) {

            if (time1 != time2) {
                *time1 = *time2;
            }

            time1    += 1;
            nsamples += 1;
        }

        time2 += 1;
    }

    time_dim->length = *ntimes = nsamples;
}

/**
 *  Filter out duplicate samples from a dataset.
 *
 *  This function will filter out samples in a dataset that have identical
 *  times and data values. It will also verify that the remaining samples
 *  are in chronological order.
 *
 *  A warning mail message will be generated if any duplicate samples were
 *  found and removed.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  ntimes  - input/output: number of times in the dataset
 *  @param  times   - input/output: array of times in the dataset
 *  @param  dataset - input/output: pointer to the dataset
 *
 *  @return
 *      - 1 if successful
 *      - 0 if an error occurred
 */
int _dsproc_filter_duplicate_samples(
    size_t    *ntimes,
    timeval_t *times,
    CDSGroup  *dataset)
{
    Mail       *warning_mail   = msngr_get_mail(MSNGR_WARNING);
    int         force_mode     = dsproc_get_force_mode();
    char       *errmsg         = (char *)NULL;
    const char *status         = (char *)NULL;
    int        *filter_mask    = (int *)NULL;
    int         overlap_type   = 0;
    size_t      noverlaps      = 0;
    size_t      ndups          = 0;
    size_t      total_filtered = 0;
    timeval_t   time1, time2;
    char        ts1[32], ts2[32];
    size_t      mi, ti, tj, tii, tjj;

    DEBUG_LV1( DSPROC_LIB_NAME,
        "%s: Checking for overlapping samples in dataset\n",
        dataset->name);

    time1 = times[0];
    tii   = 0;

    for (tj = 1; tj < *ntimes; ++tj) {

        /* Check if time1 < time2 */

        time2 = times[tj];

        if (TV_LT(time1, time2)) {
            time1 = time2;
            tii   = tj;
            continue;
        }

        /* The times are not in chronological order,
         * so search for the start index of the overlap... */

        for (ti = 0; ti < tj; ++ti) {
            if (TV_GTEQ(times[ti], time2)) break;
        }

        ndups     = 0;
        noverlaps = 0;

        if (TV_EQ(times[ti], time2)) {

            /* A time equal to time2 was found,
             * so check for consecutive duplicate times */

            for (tii = ti+1, tjj = tj+1; tii < tj; ++tii, ++tjj) {
                if (TV_NEQ(times[tii], times[tjj])) break;
            }

            ndups = tjj - tj;
        }
        else if (gFilterOverlaps & FILTER_TIME_SHIFTS || force_mode) {

            /* Filter out overlapping records */

            for (tjj = tj+1; tjj < *ntimes; ++tjj) {
                if (TV_GT(times[tjj], time1)) break;
            }

            noverlaps    = tjj - tj;
            overlap_type = 1; // times do not match
        }
        else {

            /* If a time equal to time2 was not found, we have a section
             * of overlapping records that do not have matching times. */

            format_timeval(&time1, ts1);
            format_timeval(&time2, ts2);

            status = DSPROC_ETIMEORDER;
            errmsg = msngr_create_string(
                "%s: Invalid time order found in dataset\n"
                " -> '%s' < '%s': time of record %d < time of previous record\n",
                dataset->name, ts2, ts1, (int)tj);

            break;
        }

        /* Check if we found records with duplicate timestamps */

        if (ndups) {

            /* Check if these are duplicate or overlapping records */

            if (!_dsproc_compare_samples(dataset, tj, dataset, ti, ndups)) {

                if (gFilterOverlaps & FILTER_DUP_TIMES || force_mode) {
                    noverlaps    = ndups;
                    overlap_type = 2; // times match but data values do not
                    ndups        = 0;
                }
                else {

                    /* Set status and error message */

                    status = DSPROC_ETIMEOVERLAP;

                    if (ndups == 1) {

                        format_timeval(&times[tj], ts1);

                        errmsg = msngr_create_string(
                            "%s: Overlapping records found in dataset\n"
                            " -> '%s': time of record %d = time of record %d\n",
                            dataset->name, ts1, (int)tj, (int)ti);
                    }
                    else {

                        format_timeval(&times[tj],    ts1);
                        format_timeval(&times[tjj-1], ts2);

                        errmsg = msngr_create_string(
                            "%s: Overlapping records found in dataset\n"
                            " -> '%s' to '%s': records %d to %d overlap records %d to %d\n",
                            dataset->name,
                            ts1, ts2, (int)tj, (int)(tjj-1), (int)ti, (int)(tii-1));
                    }

                    break;
                }
            }
        }

        /* Check if this is the first set of records to be filtered */

        if (total_filtered == 0) {

            if (warning_mail) {
                mail_unset_flags(warning_mail, MAIL_ADD_NEWLINE);
            }

            if (gFilterOverlaps || force_mode) {

                WARNING( DSPROC_LIB_NAME,
                    "%s: Filtering overlapping records in dataset\n",
                    dataset->name);
            }
            else {

                WARNING( DSPROC_LIB_NAME,
                    "%s: Filtering duplicate records in dataset\n",
                    dataset->name);
            }

            filter_mask = (int *)calloc(*ntimes, sizeof(int));
            if (!filter_mask) {

                ERROR( DSPROC_LIB_NAME,
                    "Could not filter overlapping records from dataset: %s\n"
                    " -> memory allocation error\n",
                    dataset->name);

                dsproc_set_status(DSPROC_ENOMEM);
                return(0);
            }
        }

        /* Set the mask flags */

        for (mi = tj; mi < tjj; ++mi) {
            filter_mask[mi] = 1;
        }

        total_filtered += ndups + noverlaps;

        /* Print warning message */

        if (ndups) {

            format_timeval(&times[tj], ts1);

            if (ndups == 1) {

                WARNING( DSPROC_LIB_NAME,
                    " - '%s': record %d is identical to record %d\n",
                    ts1, (int)tj, (int)ti);
            }
            else {

                format_timeval(&times[tjj-1], ts2);

                WARNING( DSPROC_LIB_NAME,
                    " - '%s' to '%s': records %d to %d are identical to records %d to %d\n",
                    ts1, ts2, (int)tj, (int)(tjj-1), (int)ti, (int)(tii-1));
            }

        }
        else if (noverlaps) {

            format_timeval(&times[tj], ts1);

            if (noverlaps == 1) {

                if (overlap_type == 1) {
                    WARNING( DSPROC_LIB_NAME,
                        " - '%s': record %d overlaps previous records (invalid time order)\n",
                        ts1, (int)tj);
                }
                else {
                    WARNING( DSPROC_LIB_NAME,
                        " - '%s': record %d overlaps record %d (data values do not match)\n",
                        ts1, (int)tj, (int)ti);
                }
            }
            else {

                format_timeval(&times[tjj-1], ts2);

                if (overlap_type == 1) {
                    WARNING( DSPROC_LIB_NAME,
                        " - '%s' to '%s': records %d to %d overlap previous records (invalid time order)\n",
                        ts1, ts2, (int)tj, (int)(tjj-1));
                }
                else {
                    WARNING( DSPROC_LIB_NAME,
                        " - '%s' to '%s': records %d to %d overlap records %d to %d (data values do not match)\n",
                        ts1, ts2, (int)tj, (int)(tjj-1), (int)ti, (int)(tii-1));
                }
            }
        }

        tj = tjj - 1;

    } /* end loop over times */

    if (total_filtered) {

        if (warning_mail) {
            mail_set_flags(warning_mail, MAIL_ADD_NEWLINE);
        }

        if (errmsg) {
            WARNING( DSPROC_LIB_NAME,
                " - filtering aborted\n\n%s", errmsg);
        }
        else {

            _dsproc_delete_samples(ntimes, times, filter_mask, dataset);

            WARNING( DSPROC_LIB_NAME,
                " - total records filtered: %d\n", total_filtered);
        }

        free(filter_mask);
    }

    if (errmsg) {
        ERROR( DSPROC_LIB_NAME, "%s", errmsg);
        dsproc_set_status(status);
        free(errmsg);
        return(0);
    }

    return(1);
}

/**
 *  Filter out previously stored samples from a dataset.
 *
 *  This function will filter out samples in a dataset that have identical
 *  times and data values of previously stored data. It will also verify
 *  that the remaining samples do not overlap with any previously stored data.
 *
 *  This function assumes that the times in the specified dataset are all
 *  in chronological order and that no sample times are duplicated.
 *
 *  A warning mail message will be generated if any duplicate samples were
 *  found and removed.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  ds      - pointer to the DataStream structure
 *  @param  ntimes  - input/output: number of times in the dataset
 *  @param  times   - input/output: array of times in the dataset
 *  @param  dataset - input/output: pointer to the dataset
 *
 *  @return
 *      - 1 if all remaining samples do not overlap any stored data
 *      - 0 if an error occurred
 */
int _dsproc_filter_stored_samples(
    DataStream *ds,
    size_t     *ntimes,
    timeval_t  *times,
    CDSGroup   *dataset)
{
    Mail       *warning_mail   = msngr_get_mail(MSNGR_WARNING);
    int         force_mode     = dsproc_get_force_mode();
    char       *errmsg         = (char *)NULL;
    const char *status         = (char *)NULL;
    int        *filter_mask    = (int *)NULL;
    int         found_overlap  = 0;
    int         overlap_type   = 0;
    size_t      noverlaps      = 0;
    size_t      ndups          = 0;
    size_t      total_filtered = 0;
    int         ndsfiles;
    DSFile    **dsfiles;
    CDSGroup   *fetched;
    int         nobs;
    CDSGroup   *obs;
    timeval_t  *obs_times;
    size_t      obs_ntimes;
    timeval_t   obs_start;
    timeval_t   obs_end;
    timeval_t   ds_time;
    char        ts1[32], ts2[32];
    int         oi, si, ei;
    int         mi, ti, tj, tii, tjj;

    DEBUG_LV1( DSPROC_LIB_NAME,
        "%s: Checking For overlaps with previously stored data\n",
        dataset->name);

    /* Check for previously stored data within the time range
     * of the specified dataset. */

    ndsfiles = _dsproc_find_dsfiles(
        ds->dir, &(times[0]), &(times[*ntimes - 1]), &dsfiles);

    if (ndsfiles  < 0) return(0);
    if (ndsfiles == 0) return(1);

    fetched = cds_define_group(NULL, ds->name);
    if (!fetched) {

        ERROR( DSPROC_LIB_NAME,
            "Could not filter previously stored records from dataset: %s\n"
            " -> memory allocation error\n",
            dataset->name);

        dsproc_set_status(DSPROC_ENOMEM);
        free(dsfiles);
        return(0);
    }

    if (gFilterOverlaps & FILTER_DUP_TIMES || force_mode) {

        obs_ntimes = 1;
        if (!_dsproc_fetch_timevals(ds,
            ndsfiles, dsfiles, NULL, &(times[0]),
            &obs_ntimes, &obs_start)) {

            if (obs_ntimes != 0) return(0);
            obs_start = times[0];
        }

        obs_ntimes = 1;
        if (!_dsproc_fetch_timevals(ds,
            ndsfiles, dsfiles, &(times[*ntimes - 1]), NULL,
            &obs_ntimes, &obs_end)) {

            if (obs_ntimes != 0) return(0);
            obs_end = times[*ntimes - 1];
        }
    }
    else {
        obs_start = times[0];
        obs_end   = times[*ntimes - 1];
    }

    nobs = _dsproc_fetch_dataset(
        ndsfiles, dsfiles, &obs_start, &obs_end,
        0, NULL, 0, fetched);

    free(dsfiles);

    if (nobs <= 0) {

        cds_delete_group(fetched);

        if (nobs < 0) {
            return(0);
        }

        return(1);
    }

    /* Loop over retrieved observations */

    found_overlap = 0;

    for (oi = 0; oi < nobs; oi++) {

        obs = fetched->groups[oi];

        /* Get the times for this observation */

        obs_ntimes = 0;
        obs_times  = dsproc_get_sample_timevals(obs, 0, &obs_ntimes, NULL);

        if (!obs_times) {
            if (obs_ntimes != 0) {
                cds_delete_group(fetched);
                return(-1);
            }
            continue;
        }

        /* Find the time indexes in the specified dataset
         * that overlap this observation. */

        obs_start = obs_times[0];
        obs_end   = obs_times[obs_ntimes - 1];

        si = cds_find_timeval_index(*ntimes, times, obs_start, CDS_GTEQ);
        if (si < 0) {
            free(obs_times);
            continue;
        }

        ei = cds_find_timeval_index(*ntimes, times, obs_end, CDS_LTEQ);
        if (ei < 0) {
            free(obs_times);
            continue;
        }

        if (ei < si) {

            /* This observation fits between two records in the dataset.
             *
             * This may be ok if all the previous records were filtered
             * out, or all the remaining records will be filtered out.
             *
             * We will need to check for this again after filtering out
             * all the duplicate records. */

            free(obs_times);
            continue;
        }

        /* Loop over the dataset times */

        tj = 0;

        for (ti = si; ti <= ei; ++ti) {

            ds_time   = times[ti];
            noverlaps = 0;
            ndups     = 0;

            /* Skip obs times that are less than this dataset time */

            while ( TV_LT(obs_times[tj], ds_time) ) ++tj;

            /* We have overlapping records if the times are not equal */

            if ( TV_NEQ(obs_times[tj], ds_time) ) {

                /* The start ds time is >= the first obs time and
                 * the end   ds time is <= the last obs time,
                 *
                 * so if we get here we know that:
                 *
                 * obs_times[tj-1] < times[ti] < obs_times[tj]  */

                if (gFilterOverlaps & FILTER_TIME_SHIFTS || force_mode) {

                    /* Filter out dataset times until we find one equal to
                     * an obs_time, or greater than the last obs time. */

                    for (tii = ti+1, tjj = tj;
                         tii <= ei && tjj < (int)obs_ntimes;
                         ++tii) {

                        if    ( TV_EQ(times[tii], obs_times[tjj]) ) break;
                        while ( TV_GT(times[tii], obs_times[tjj]) ) ++tjj;
                    }

                    noverlaps    = tii - ti;
                    overlap_type = 1; // ds times do not line up with obs times
                }
                else {
                    found_overlap = 1;
                    break;
                }
            }
            else {

                /* Check for consecutive duplicate times */

                for (tii  = ti+1, tjj = tj+1;
                     tii <= ei && tjj < (int)obs_ntimes;
                     ++tii,       ++tjj) {

                    if ( TV_NEQ(times[tii], obs_times[tjj]) ) break;
                }

                ndups = tii - ti;

                /* Check if these are duplicate or overlapping records */

                if (!_dsproc_compare_samples(dataset, ti, obs, tj, ndups)) {
                    if (gFilterOverlaps & FILTER_DUP_TIMES || force_mode) {
                        noverlaps    = ndups;
                        ndups        = 0;
                        overlap_type = 2; // times match but data values do not
                    }
                    else {
                        found_overlap = 1;
                        break;
                    }
                }
            }

            /* Check if this is the first record being filtered */

            if (total_filtered == 0) {

                if (warning_mail) {
                    mail_unset_flags(warning_mail, MAIL_ADD_NEWLINE);
                }

                WARNING( DSPROC_LIB_NAME,
                    "%s: Filtering data previously stored in file: %s\n",
                    dataset->name, obs->name);

                filter_mask = (int *)calloc(*ntimes, sizeof(int));
                if (!filter_mask) {

                    ERROR( DSPROC_LIB_NAME,
                        "Could not filter previously stored records from dataset: %s\n"
                        " -> memory allocation error\n",
                        dataset->name);

                    dsproc_set_status(DSPROC_ENOMEM);
                    cds_delete_group(fetched);
                    free(obs_times);
                    return(0);
                }
            }

            /* Set the mask flags */

            for (mi = ti; mi < tii; ++mi) {
                filter_mask[mi] = 1;
            }

            total_filtered += ndups + noverlaps;

            /* Print warning message */

            if (ndups) {

                if (ndups == 1) {

                    format_timeval(&times[ti], ts1);

                    WARNING( DSPROC_LIB_NAME,
                        " - '%s': duplicate record %d\n",
                        ts1, (int)ti);
                }
                else {

                    format_timeval(&times[ti],    ts1);
                    format_timeval(&times[tii-1], ts2);

                    WARNING( DSPROC_LIB_NAME,
                        " - '%s' to '%s': duplicate records %d to %d\n",
                        ts1, ts2, (int)ti, (int)(tii-1));
                }
            }
            else if (noverlaps) {

                if (noverlaps == 1) {

                    format_timeval(&times[ti], ts1);

                    if (overlap_type == 1) {
                        WARNING( DSPROC_LIB_NAME,
                            " - '%s': overlapping record %d (times do not match)\n",
                            ts1, (int)ti);
                    }
                    else {
                        WARNING( DSPROC_LIB_NAME,
                            " - '%s': overlapping record %d (data values do not match)\n",
                            ts1, (int)ti);
                    }
                }
                else {

                    format_timeval(&times[ti],    ts1);
                    format_timeval(&times[tii-1], ts2);

                    if (overlap_type == 1) {
                        WARNING( DSPROC_LIB_NAME,
                            " - '%s' to '%s': overlapping records %d to %d (times do not match)\n",
                            ts1, ts2, (int)ti, (int)(tii-1));
                    }
                    else {
                        WARNING( DSPROC_LIB_NAME,
                            " - '%s' to '%s': overlapping records %d to %d (data values do not match)\n",
                            ts1, ts2, (int)ti, (int)(tii-1));
                    }
                }
            }

            if ((tii > ei) || (tjj == (int)obs_ntimes)) break;

            ti = tii - 1;
            tj = tjj;

        } /* end loop over dataset times */

        free(obs_times);

        if (found_overlap) break;

    } /* end loop over observations */

    /* Check if an overlap was found */

    if (found_overlap) {

        /* Set status and error message */

        status = DSPROC_ETIMEOVERLAP;

        if (ei == si) {

            format_timeval(&times[si], ts1);

            errmsg = msngr_create_string(
                "%s: Overlapping records found with previously stored data\n"
                " -> '%s': record %d overlaps data in: %s\n",
                dataset->name, ts1, si, obs->name);
        }
        else if (ei < si) {

            format_timeval(&times[ei], ts1);
            format_timeval(&times[si], ts2);

            errmsg = msngr_create_string(
                "%s: Overlapping records found with previously stored data\n"
                " -> '%s' to '%s': records %d to %d overlap data in: %s\n",
                dataset->name, ts1, ts2, ei, si, obs->name);
        }
        else {

            format_timeval(&times[si], ts1);
            format_timeval(&times[ei], ts2);

            errmsg = msngr_create_string(
                "%s: Overlapping records found with previously stored data\n"
                " -> '%s' to '%s': records %d to %d overlap data in: %s\n",
                dataset->name, ts1, ts2, si, ei, obs->name);
        }
    }

    /* Check if any duplicates need to be filtered */

    if (total_filtered) {

        if (warning_mail) {
            mail_set_flags(warning_mail, MAIL_ADD_NEWLINE);
        }

        if (errmsg) {
            WARNING( DSPROC_LIB_NAME,
                " - filtering aborted\n\n%s", errmsg);
        }
        else {

            _dsproc_delete_samples(ntimes, times, filter_mask, dataset);

            WARNING( DSPROC_LIB_NAME,
                " - total records filtered: %d\n", total_filtered);
        }

        free(filter_mask);
    }

    /* Generate error message if an overlap was found */

    if (errmsg) {
        ERROR( DSPROC_LIB_NAME, "%s", errmsg);
        dsproc_set_status(status);
        free(errmsg);
        cds_delete_group(fetched);
        return(0);
    }

    /* Now we need to loop over all retrieved observations again
     * to verify that there are no overlapping records */

    if (*ntimes == 0) {
        cds_delete_group(fetched);
        return(1);
    }

    for (oi = 0; oi < nobs; oi++) {

        obs = fetched->groups[oi];

        /* Get the start and end times of this observation */

        obs_ntimes = dsproc_get_time_range(obs, &obs_start, &obs_end);
        if (!obs_ntimes) continue;

        /* Find the time indexes in the specified dataset
         * that overlap this observation. */

        si = cds_find_timeval_index(*ntimes, times, obs_start, CDS_GTEQ);
        if (si < 0) continue;

        ei = cds_find_timeval_index(*ntimes, times, obs_end, CDS_LTEQ);
        if (ei < 0) continue;

        /* This observation still overlaps the specified dataset */

        if (ei == si) {

            format_timeval(&times[si], ts1);

            ERROR( DSPROC_LIB_NAME,
                "%s: Overlapping records found with previously stored data\n"
                " -> '%s': record %d overlaps data in: %s\n",
                dataset->name, ts1, si, obs->name);
        }
        else if (ei < si) {

            format_timeval(&times[ei], ts1);
            format_timeval(&times[si], ts2);

            ERROR( DSPROC_LIB_NAME,
                "%s: Overlapping records found with previously stored data\n"
                " -> '%s' to '%s': records %d to %d overlap data in: %s\n",
                dataset->name, ts1, ts2, ei, si, obs->name);
        }
        else {

            format_timeval(&times[si], ts1);
            format_timeval(&times[ei], ts2);

            ERROR( DSPROC_LIB_NAME,
                "%s: Overlapping records found with previously stored data\n"
                " -> '%s' to '%s': records %d to %d overlap data in: %s\n",
                dataset->name, ts1, ts2, si, ei, obs->name);
        }

        dsproc_set_status(DSPROC_ETIMEOVERLAP);
        cds_delete_group(fetched);
        return(0);

    } /* end second loop over observations */

    cds_delete_group(fetched);
    return(1);
}

/** @publicsection */

/*******************************************************************************
 *  Internal Functions Visible To The Public
 */

/**
 *  Disable the warning messages from the NaN/Inf Filter.
 */
void dsproc_disable_nan_filter_warnings(void)
{
    gDisableNanFilterWarnings = 1;
}

/**
 *  Replace NaN and Inf values in a variable with missing values.
 *
 *  This function will only replace NaN and Inf values in variables that have
 *  a missing value defined.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  var - pointer to the variable
 *
 *  @return
 *    - number of NaN/Inf values replaced
 *    - -1 if a memory allocation error occurs
 */
int dsproc_filter_var_nans(CDSVar *var)
{
    int     nmissings;
    CDSData missings;
    size_t  sample_size;
    size_t  nvalues;
    int     nan_count;

    /* Only floats and doubles can have NaN/Inf values */

    if (var->type != CDS_FLOAT &&
        var->type != CDS_DOUBLE) {

        return(0);
    }

    /* Check if this variable has any missing values defined */

    missings.vp = (void *)NULL;
    nmissings   = dsproc_get_var_missing_values(var, &(missings.vp));

    if (nmissings <= 0) return(nmissings);

    /* Get the total number of values in the variables data array */

    sample_size = dsproc_var_sample_size(var);
    if (!sample_size) {
        free(missings.vp);
        return(0);
    }

    nvalues = var->sample_count * sample_size;

    /* Loop over all values, replacing NaNs and Infs with missing */

    nan_count  = 0;
    nvalues   += 1;

    if (var->type == CDS_FLOAT) {

        float *datap   = var->data.fp;
        float  missing = *(missings.fp);

        while (--nvalues) {

            if (!isfinite(*datap)) {
                *datap = missing;
                ++nan_count;
            }

            ++datap;
        }
    }
    else {

        double *datap   = var->data.dp;
        double  missing = *(missings.dp);

        while (--nvalues) {

            if (!isfinite(*datap)) {
                *datap = missing;
                ++nan_count;
            }

            ++datap;
        }
    }

    free(missings.vp);

    return(nan_count);
}

/**
 *  Replace NaN and Inf values in a dataset with missing values.
 *
 *  This function will only replace NaN and Inf values in variables that have
 *  a missing value defined.
 *
 *  If the warn flag is set, a warning mail message will be generated if any
 *  NaN or Inf values are replaced with missing values.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  dataset - pointer to the dataset
 *  @param  warn    - flag specifying if warning messages should be generated
 *                    (0 == false, 1 == true)
 *
 *  @return
 *    - 1 if successful
 *    - 0 if a memory allocation error occurs
 */
int dsproc_filter_dataset_nans(CDSGroup *dataset, int warn)
{
    Mail   *warning_mail = msngr_get_mail(MSNGR_WARNING);
    int     total_nans   = 0;
    int     is_base_time;
    CDSVar *var;
    int     found_nans;
    int     vi;

    DEBUG_LV1( DSPROC_LIB_NAME,
        "%s: Checking for Nan/Inf values in dataset\n",
        dataset->name);

    /* Loop over all variables in the dataset */

    for (vi = 0; vi < dataset->nvars; ++vi) {

        var = dataset->vars[vi];

        /* Skip variables that are not float or doubles */

        if (var->type != CDS_FLOAT &&
            var->type != CDS_DOUBLE) {

            continue;
        }

        /* Skip the time variables */

        if (cds_is_time_var(var, &is_base_time)) {
            continue;
        }

        /* Filter NaN/Inf values */

        found_nans = dsproc_filter_var_nans(var);

        if (found_nans < 0) {
            return(0);
        }

        /* Generate Warning */

        if (warn && found_nans && !gDisableNanFilterWarnings) {

            if (!total_nans) {

                if (warning_mail) {
                    mail_unset_flags(warning_mail, MAIL_ADD_NEWLINE);
                }

                WARNING( DSPROC_LIB_NAME,
                    "%s: Replacing NaN/Inf values with missing values\n",
                    dataset->name);
            }

            WARNING( DSPROC_LIB_NAME,
                " - %s: replaced %d NaN/Inf values\n",
                var->name, found_nans);
        }

        total_nans += found_nans;
    }

    if (warn && total_nans && !gDisableNanFilterWarnings) {

        if (warning_mail) {
            mail_set_flags(warning_mail, MAIL_ADD_NEWLINE);
        }

        WARNING( DSPROC_LIB_NAME,
            " - total NaN/Inf values replaced: %d\n", total_nans);
    }

    return(1);
}

/**
 *  Filter overlapping data records.
 *
 *  This function can be used to configure the filtering logic to remove
 *  data records from a dataset that overlap with records in either the
 *  current dataset or previously stored data.  It can also be used to
 *  remove overlapping observations in the input data for processes that
 *  use the VAP or Hybrid Ingest processing models.
 *
 *  The available modes are:
 *
 *    - FILTER_DUP_RECS:    This is the default setting and can be used to
 *                          reset the filtering mode back to only filtering
 *                          records in the output datasets with duplicate
 *                          times and data values.
 *
 *    - FILTER_TIME_SHIFTS: Filter overlapping records in the output datasets
 *                          that are not in chronological order. This filters
 *                          data records with times that fall in-between two
 *                          records in either the current dataset or previously
 *                          stored data.
 *
 *    - FILTER_DUP_TIMES:   Filter overlapping records in the output datasets
 *                          that have the same times but different data values
 *                          as records in either the current dataset or
 *                          previously stored data.
 *
 *    - FILTER_INPUT_OBS:   Filter overlapping observations in the input
 *                          datasets. This mode is only relevant for VAPS and
 *                          Hybrid Ingests. When filtering overlapping
 *                          observations, the one with the most recent creation
 *                          time will be used if the number of samples is 75%
 *                          or more of the previous one, otherwise, the previous
 *                          observation will be used.
 *
 *    - FILTER_OVERLAPS:    Same as FILTER_TIME_SHIFTS | FILTER_DUP_TIMES | FILTER_INPUT_OBS.
 *
 *  @param  mode  - filtering mode
 */
void dsproc_set_overlap_filtering_mode(int mode)
{
    gFilterOverlaps = mode;
}

/**
 *  Get current overlap filtering mode.
 *
 *  @return
 *    - mode  current overlap filtering mode
 */
int dsproc_get_overlap_filtering_mode(void)
{
    return(gFilterOverlaps);
}

/*******************************************************************************
 *  Public Functions
 */
