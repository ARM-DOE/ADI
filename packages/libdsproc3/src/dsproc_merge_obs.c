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

/** @file dsproc_merge_obs.c
 *  Merge Observations Function.
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

/**
 *  Private: Compare two overlapping observations.
 *
 *  This function will generate an error message with information about the
 *  two overlapping observations if the FILTER_INPUT_OBS overlap filtering
 *  mode has *not* been set.
 *
 *  If the FILTER_INPUT_OBS overlap filtering mode has been set, this function
 *  will filter out the overlapping observation by keeping the one with the
 *  most recent creation time if the number of samples is 75% or more of the
 *  previous one, otherwise, it will keep the previous observation.
 *
 *  @param  g1  - pointer to the CDSGroup for obs1
 *  @param  nt1 - number of times in obs1
 *  @param  st1 - start time of obs1
 *  @param  et1 - end time of obs1
 *  @param  g2  - pointer to the CDSGroup for obs2
 *  @param  nt2 - number of times in obs2
 *  @param  st2 - start time of obs2
 *  @param  et2 - end time of obs2
 *
 *  @return
 *    -  1 one of the overlapping observations was removed.
 *    -  0 an error message was generated and the process should fail.
 */
int _dsproc_compare_overlapping_obs(
    CDSGroup  *g1,
    size_t     nt1,
    timeval_t  st1,
    timeval_t  et1,
    CDSGroup  *g2,
    size_t     nt2,
    timeval_t  st2,
    timeval_t  et2)
{
    int        filter_mode = dsproc_get_overlap_filtering_mode();
    CDSAtt    *att;
    char      *dod1, *dod2;
    char      *history1, *history2;
    time_t     created1, created2;
    char       ts11[32], ts12[32], ts21[32], ts22[32];
    int        keep_obs;
    int        retval = 1;

    /* Get DOD versions to include in warning or error message. */

    dod1 = dod2 = NULL;

    att = cds_get_att(g1, "dod_version");
    if (att) dod1 = att->value.cp;

    att = cds_get_att(g2, "dod_version");
    if (att) dod2 = att->value.cp;

    if (!dod1) dod1 = "Missing dod_version attribute.";
    if (!dod2) dod2 = "Missing dod_version attribute.";

    /* Get history attribute and creation times */

    created1 = created2 = 0;
    history1 = history2 = (char *)NULL;
    dsproc_get_dataset_creation_info(g1, &history1, &created1, NULL, NULL, NULL);
    dsproc_get_dataset_creation_info(g2, &history2, &created2, NULL, NULL, NULL);

    // need to use strdup here because we free the history strings later...
    if (!history1) history1 = strdup("Missing history attribute or invalid format.");
    if (!history2) history2 = strdup("Missing history attribute or invalid format.");

    /* Check if FILTER_INPUT_OBS overlapping filtering mode has been set.
     * If not, we can just bail out here. */

    if (!(filter_mode & FILTER_INPUT_OBS)) {

        DSPROC_ERROR( DSPROC_EINDSOVERLAP,
            "Overlapping input data found for: %s\n"
            "  - %s\n"
            "      - time range:  %s -> %s, ntimes = %lu\n"
            "      - DOD version: %s\n"
            "      - history:     %s\n"
            "  - %s\n"
            "      - time range:  %s -> %s, ntimes = %lu\n"
            "      - DOD version: %s\n"
            "      - history:     %s\n",
            g1->parent->name,
            g1->name,
            format_timeval(&st1, ts11), format_timeval(&et1, ts12), nt1,
            dod1,
            history1,
            g2->name,
            format_timeval(&st2, ts21), format_timeval(&et2, ts22), nt2,
            dod2,
            history2);

        retval = 0;
        goto CLEANUP_AND_EXIT;
    }

    /* The FILTER_INPUT_OBS overlapping filtering mode has been set
     * so we need to filter out one of the overlapping observations. */

    /* After talking to Krista we decided that the logic that will
     * be more correct most of the time is to keep the obs with the
     * most recent creation time if the number of samples is 75% or
     * more of the previous one... */

    if (created1 == 0 || created2 == 0) {

        if (created1 != 0) {
            /* The second obs is missing the history attribute or it
             * has an invalid format, so keep the first one. */ 
            keep_obs = 1;
        }
        else if (created2 != 0) {
            /* The first obs is missing the history attribute or it
             * has an invalid format, so keep the second one. */ 
            keep_obs = 2;
        }
        else {
            /* Both obs are missing the history attribute or they
             * have invalid formats, so keep the one with the most
             * samples. */ 
            keep_obs = (nt1 > nt2) ? 1 : 2;
        }
    }
    else if (created1 > created2 && nt1 >= 0.75 * nt2) {
        // Keep the first obs (see comment above)
        keep_obs = 1;
    }
    else {
        // created1 < created2 || nt1 < 0.75 * nt2
        keep_obs = 2;
    }

    if (keep_obs == 1) {
        DSPROC_WARNING(
            "Filtering overlapping input data for: %s\n"
            "  - Keeping:  %s\n"
            "      - time range:  %s -> %s, ntimes = %lu\n"
            "      - DOD version: %s\n"
            "      - history:     %s\n"
            "  - Skipping: %s\n"
            "      - time range:  %s -> %s, ntimes = %lu\n"
            "      - DOD version: %s\n"
            "      - history:     %s\n",
            g1->parent->name,
            g1->name,
            format_timeval(&st1, ts11), format_timeval(&et1, ts12), nt1,
            dod1,
            history1,
            g2->name,
            format_timeval(&st2, ts21), format_timeval(&et2, ts22), nt2,
            dod2,
            history2);

        cds_delete_group(g2);
    }
    else {
        DSPROC_WARNING(
            "Filtering overlapping input data for: %s\n"
            "  - Keeping:  %s\n"
            "      - time range:  %s -> %s, ntimes = %lu\n"
            "      - DOD version: %s\n"
            "      - history:     %s\n"
            "  - Skipping: %s\n"
            "      - time range:  %s -> %s, ntimes = %lu\n"
            "      - DOD version: %s\n"
            "      - history:     %s\n",
            g2->parent->name,
            g2->name,
            format_timeval(&st2, ts21), format_timeval(&et2, ts22), nt2,
            dod2,
            history2,
            g1->name,
            format_timeval(&st1, ts11), format_timeval(&et1, ts12), nt1,
            dod1,
            history1);

        cds_delete_group(g1);
    }

CLEANUP_AND_EXIT:

    if (history1) free(history1);
    if (history2) free(history2);

    return(retval);
}

/**
 *  Private: Check for overlapping observations in the specified CDSGroup.
 *
 *  If overlapping observation are found, this function will call
 *  _dsproc_compare_overlapping_obs() to determine how to handle them.
 *
 *  @param  parent  - pointer to the parent CDSGroup
 *
 *  @return
 *    -  1 no overlapping observations or all overlaps were filtered out.
 *    -  0 found overlaping observations and they were *not* filtered out.
 */
int _dsproc_check_for_overlapping_obs(CDSGroup *parent)
{
    int        o1,  o2;  // obs index
    CDSGroup  *g1, *g2;
    size_t     nt1, nt2; // num times
    timeval_t  st1, st2; // start times
    timeval_t  et1, et2; // end times
    int        status;

    DEBUG_LV1( DSPROC_LIB_NAME,
        "Checking for overlapping observations in input data for: %s\n",
        cds_get_object_path(parent));

    if (parent->ngroups < 2) {
        return(parent->ngroups);
    }

    o1 = 0;
    o2 = 1;

    while (o2 < parent->ngroups) {

        g1 = parent->groups[o1];
        g2 = parent->groups[o2];

        nt1 = cds_get_time_range(g1, &st1, &et1);
        nt2 = cds_get_time_range(g2, &st2, &et2);

        if (TV_GT(st2, et1)) {
            /* The start time of obs2 is greater that the end time of obs1. */
            o1 += 1;
            o2 += 1;
        }
        else {
            /* Found overlap */

            status = _dsproc_compare_overlapping_obs(
                g1, nt1, st1, et1,
                g2, nt2, st2, et2);
            
            if (status == 0) {
                // Overlaps were found and *not* filtered out.
                return(0);
            }

            /* If we get here all overlaps were filtered out.
             * In this case the obs indexes are still correct
             * for the next pass through the loop. */
        }
    }

    return(1);
}

/**
 *  Private: Merge all the observations in the specified CDSGroup.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  parent  - pointer to the parent CDSGroup
 *
 *  @return
 *    -  number of observations after merge
 *    - -1 if an error occurred
 */
int _dsproc_merge_obs(CDSGroup *parent)
{
    int        o1,  o2;
    CDSGroup  *g1, *g2;
    CDSDim    *d1, *d2;
    CDSVar    *v1, *v2;
    int        di;
    int        vi;
    size_t     length;
    CDSVar    *time_var;
    timeval_t *sample_times;
    size_t     ntimes;
    int        is_base_time;

    o1 = 0;
    o2 = 1;

    if (parent->ngroups < 2) {
        return(parent->ngroups);
    }

    DEBUG_LV1( DSPROC_LIB_NAME,
        "Merging observations for %s\n",
        cds_get_object_path(parent));

    /* Check for overlapping observations. */

    if (!_dsproc_check_for_overlapping_obs(parent)) {
        return(-1);
    }

    /* Merge observations */

    while (o2 < parent->ngroups) {

        g1 = parent->groups[o1];
        g2 = parent->groups[o2];

        /* Make sure the number of dimensions and variables match */

        if (g1->ndims != g2->ndims) {

            WARNING( DSPROC_LIB_NAME,
                "Could not merge observations: %s and %s\n"
                " -> number of dimensions do not match: %d != %d\n",
                cds_get_object_path(g1), cds_get_object_path(g2),
                (int)g1->ndims, (int)g2->ndims);

            o1++; o2++;
            continue;
        }

        if (g1->nvars != g2->nvars) {

            WARNING( DSPROC_LIB_NAME,
                "Could not merge observations: %s and %s\n"
                " -> number of variables do not match: %d != %d\n",
                cds_get_object_path(g1), cds_get_object_path(g2),
                (int)g1->nvars, (int)g2->nvars);

            o1++; o2++;
            continue;
        }

        /* Make sure the dimensionality of the two observations is the same */

        for (di = 0; di < g1->ndims; di++) {

            d1 = g1->dims[di];
            d2 = cds_get_dim(g2, d1->name);

            if (!d2) {

                WARNING( DSPROC_LIB_NAME,
                    "Could not merge observations: %s and %s\n"
                    " -> dimension '%s' not found in the second observation\n",
                    cds_get_object_path(g1), cds_get_object_path(g2),
                    d1->name);

                break;
            }

            if (d1->is_unlimited != d2->is_unlimited) {

                WARNING( DSPROC_LIB_NAME,
                    "Could not merge observations: %s and %s\n"
                    " -> dimension '%s' is unlimited in one but not the other\n",
                    cds_get_object_path(g1), cds_get_object_path(g2),
                    d1->name);

                break;
            }

            if ((d1->is_unlimited == 0) &&
                (d1->length != d2->length)) {

                WARNING( DSPROC_LIB_NAME,
                    "Could not merge observations: %s and %s\n"
                    " -> dimension lengths for '%s' do not match: %d != %d\n",
                    cds_get_object_path(g1), cds_get_object_path(g2),
                    d1->name, (int)d1->length, (int)d2->length);

                break;
            }
        }

        if (di != g1->ndims) {
            o1++; o2++;
            continue;
        }

        /* Make sure the variables in the two observations have
         * the same dimensionality and static data */

        for (vi = 0; vi < g1->nvars; vi++) {

            v1 = g1->vars[vi];
            v2 = cds_get_var(g2, v1->name);

            /* Check dimensionality */

            if (v1->ndims != v2->ndims) {

                WARNING( DSPROC_LIB_NAME,
                    "Could not merge observations: %s and %s\n"
                    " -> number of dimensions for variable '%s' do not match: %d != %d\n",
                    cds_get_object_path(g1), cds_get_object_path(g2),
                    v1->name, (int)v1->ndims, (int)v2->ndims);

                break;
            }

            for (di = 0; di < v1->ndims; di++) {
                if (strcmp(v1->dims[di]->name, v2->dims[di]->name) != 0) {

                    WARNING( DSPROC_LIB_NAME,
                        "Could not merge observations: %s and %s\n"
                        " -> dimension names for variable '%s' do not match: %s != %s\n",
                        cds_get_object_path(g1), cds_get_object_path(g2),
                        v1->name, v1->dims[di]->name, v2->dims[di]->name);

                    break;
                }
            }

            if (di != v1->ndims) {
                break;
            }

            /* Check static data */

            if (v1->ndims > 0 &&
                v1->dims[0]->is_unlimited) {

                continue;
            }

            if (cds_is_time_var(v1, &is_base_time)) {
                continue;
            }

            if (v1->type != v2->type) {

                WARNING( DSPROC_LIB_NAME,
                    "Could not merge observations: %s and %s\n"
                    " -> data types for variable '%s' do not match: %s != %s\n",
                    cds_get_object_path(g1), cds_get_object_path(g2),
                    v1->name,
                    cds_data_type_name(v1->type), cds_data_type_name(v2->type));

                break;
            }

            if (v1->sample_count != v2->sample_count) {

                WARNING( DSPROC_LIB_NAME,
                    "Could not merge observations: %s and %s\n"
                    " -> sample counts for variable '%s' do not match: %d != %d\n",
                    cds_get_object_path(g1), cds_get_object_path(g2),
                    v1->name, v1->sample_count, v2->sample_count);

                break;
            }

            length = v1->sample_count
                   * cds_var_sample_size(v1)
                   * cds_data_type_size(v1->type);

            if (memcmp(v1->data.vp, v2->data.vp, length) != 0) {

                WARNING( DSPROC_LIB_NAME,
                    "Could not merge observations: %s and %s\n"
                    " -> static data for variable '%s' does not match\n",
                    cds_get_object_path(g1), cds_get_object_path(g2),
                    v1->name);

                break;
            }
        }

        if (vi != g1->nvars) {
            o1++; o2++;
            continue;
        }

        /* Merge time variable data */

        ntimes = 0;
        sample_times = cds_get_sample_timevals(g2, 0, &ntimes, NULL);

        if (ntimes == (size_t)-1) {

            ERROR( DSPROC_LIB_NAME,
                "Could not merge observations: %s and %s\n"
                " -> CDS Error getting sample times\n",
                cds_get_object_path(g1), cds_get_object_path(g2));

            dsproc_set_status(DSPROC_ECDSGETTIME);
            return(-1);
        }

        if (ntimes > 0) {

            time_var = cds_find_time_var(g1);

            if (!cds_set_sample_timevals(
                g1, time_var->sample_count, ntimes, sample_times)) {

                ERROR( DSPROC_LIB_NAME,
                    "Could not merge observations: %s and %s\n"
                    " -> CDS Error setting sample times\n",
                    cds_get_object_path(g1), cds_get_object_path(g2));

                dsproc_set_status(DSPROC_ECDSSETTIME);
                return(-1);
            }

            free(sample_times);
        }

        /* Merge variable data */

        for (vi = 0; vi < g1->nvars; vi++) {

            v1 = g1->vars[vi];

            if ((v1->ndims                 == 0) ||
                (v1->dims[0]->is_unlimited == 0) ||
                (cds_is_time_var(v1, &is_base_time))) {

                continue;
            }

            v2 = cds_get_var(g2, v1->name);

            if (!cds_set_var_data(v1,
                v2->type, v1->sample_count, v2->sample_count,
                NULL, v2->data.vp)) {

                ERROR( DSPROC_LIB_NAME,
                    "Could not merge observations: %s and %s\n"
                    " -> CDS Error setting data for variable: %s\n",
                    cds_get_object_path(g1), cds_get_object_path(g2),
                    v1->name);

                dsproc_set_status(DSPROC_ECDSSETDATA);
                return(-1);
            }
        }

        cds_delete_group(g2);
    }

    return(parent->ngroups);
}

/** @publicsection */

/*******************************************************************************
 *  Internal Functions Visible To The Public
 */

/**
 *  Merge observations in the _DSProc->ret_data group.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int dsproc_merge_retrieved_data()
{
    DataStream *ds;
    int         dsid;

    for (dsid = 0; dsid < _DSProc->ndatastreams; dsid++) {

        ds = _DSProc->datastreams[dsid];

        if (ds->flags & DS_PRESERVE_OBS ||
            ds->flags & DS_DISABLE_MERGE) {
            continue;
        }

        if (!ds->ret_cache ||
            !ds->ret_cache->ds_group) {

            continue;
        }

        if (_dsproc_merge_obs(ds->ret_cache->ds_group) < 0) {
            return(0);
        }
    }

    return(1);
}

