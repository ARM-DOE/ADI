/*******************************************************************************
*
*  COPYRIGHT (C) 2011 Battelle Memorial Institute.  All Rights Reserved.
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
*    $Revision: 16828 $
*    $Author: ermold $
*    $Date: 2013-02-11 22:13:21 +0000 (Mon, 11 Feb 2013) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
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
 *  Private: Merge all the observations in the specified CDSGroup.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  parent  - pointer to the parent CDSGroup
 *
 *  @return
 *    -  number of observations after merge
 *    -  0 if there were less then 2 observations found in the group
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

