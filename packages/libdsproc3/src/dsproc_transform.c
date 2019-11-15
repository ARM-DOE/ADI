/*******************************************************************************
*
*  COPYRIGHT (C) 2010 Battelle Memorial Institute.  All Rights Reserved.
*
********************************************************************************
*
*  Author:
*     name:  Krista Gaustad
*     phone: (509) 375-5950
*     email: krista.gaustad@pnl.gov
*
*     name:  Brian Ermold
*     phone: (509) 375-5950
*     email: brian.ermold@pnnl.gov
*
********************************************************************************
*
*  REPOSITORY INFORMATION:
*    $Revision: 76935 $
*    $Author: ermold $
*    $Date: 2017-02-24 17:26:14 +0000 (Fri, 24 Feb 2017) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file dsproc_transform.c
 *  Tansformation Functions.
 */

#include "dsproc3.h"
#include "dsproc_private.h"
#include "trans.h"
#include <math.h>

extern DSProc *_DSProc; /**< Internal DSProc structure */

/** @privatesection */

/*******************************************************************************
 *  Static Functions and Data Visible Only To This Module
 */

static const char *gTransQCRollupBadDescDefault = "Transformation could not finish";
static const char *gTransQCRollupIndDescDefault = "Transformation resulted in an indeterminate outcome";

static const char *gTransQCRollupBadDesc = (const char *)NULL;
static const char *gTransQCRollupIndDesc = (const char *)NULL;

typedef struct TransAtts {
    const char *name;
    const char *value;
} TransAtts;

static TransAtts gTransQCAtts[] =
{
    { "units",             "unitless" },
    { "description",       "This field contains bit packed integer values, where each bit represents a QC test on the data. Non-zero bits indicate the QC condition given in the description for those bits; a value of 0 (no bits set) indicates the data has not failed any QC tests." },
    { "flag_method",       "bit" },
    { "bit_1_description", "QC_BAD:  Transformation could not finish, value set to missing_value." },
    { "bit_1_assessment",  "Bad" },
    { "bit_1_comment",     "An example that will trip this bit is if all values are bad or outside range." },
    { "bit_2_description", "QC_INDETERMINATE:  Some, or all, of the input values used to create this output value had a QC assessment of Indeterminate." },
    { "bit_2_assessment",  "Indeterminate" },
    { "bit_3_description", "QC_INTERPOLATE:  Indicates a non-standard interpolation using points other than the two that bracket the target index was applied." },
    { "bit_3_assessment",  "Indeterminate" },
    { "bit_3_comment",     "An example of why this may occur is if one or both of the nearest points was flagged as bad.  Applies only to interpolate transformation method." },
    { "bit_4_description", "QC_EXTRAPOLATE:  Indicates extrapolation is performed out from two points on the same side of the target index." },
    { "bit_4_assessment",  "Indeterminate" },
    { "bit_4_comment",     "This occurs because the input grid does not span the output grid, or because all the points within range and on one side of the target were flagged as bad.  Applies only to the interpolate transformation method." },
    { "bit_5_description", "QC_NOT_USING_CLOSEST:  Nearest good point is not the nearest actual point." },
    { "bit_5_assessment",  "Indeterminate" },
    { "bit_5_comment",     "Applies only to subsample transformation method." },
    { "bit_6_description", "QC_SOME_BAD_INPUTS:  Some, but not all, of the inputs in the averaging window were flagged as bad and excluded from the transform." },
    { "bit_6_assessment",  "Indeterminate" },
    { "bit_6_comment",     "Applies only to the bin average transformation method." },
    { "bit_7_description", "QC_ZERO_WEIGHT:  The weights for all the input points to be averaged for this output bin were set to zero." },
    { "bit_7_assessment",  "Indeterminate" },
    { "bit_7_comment",     "The output \"average\" value is set to zero, independent of the value of the input.  Applies only to bin average transformation method." },
    { "bit_8_description", "QC_OUTSIDE_RANGE:  No input samples exist in the transformation region, value set to missing_value." },
    { "bit_8_assessment",  "Bad" },
    { "bit_8_comment",     "Nearest good bracketing points are farther away than the \"range\" transform parameter if transformation is done using the interpolate or subsample method, or \"width\" if a bin average transform is applied.  Test can also fail if more than half an input bin is extrapolated beyond the first or last point of the input grid." },
    { "bit_9_description", "QC_ALL_BAD_INPUTS:  All the input values in the transformation region are bad, value set to missing_value." },
    { "bit_9_assessment",  "Bad" },
    { "bit_9_comment",     "The transformation could not be completed. Values in the output grid are set to missing_value and the QC_BAD bit is also set." },
    { "bit_10_description", "QC_BAD_STD:  Standard deviation over averaging interval is greater than limit set by transform parameter std_bad_max." },
    { "bit_10_assessment",  "Bad" },
    { "bit_10_comment",     "Applies only to the bin average transformation method." },
    { "bit_11_description", "QC_INDETERMINATE_STD:  Standard deviation over averaging interval is greater than limit set by transform parameter std_ind_max." },
    { "bit_11_assessment",  "Indeterminate" },
    { "bit_11_comment",     "Applies only to the bin average transformation method." },
    { "bit_12_description", "QC_BAD_GOODFRAC:  Fraction of good and indeterminate points over averaging interval are less than limit set by transform parameter goodfrac_bad_min." },
    { "bit_12_assessment",  "Bad" },
    { "bit_12_comment",     "Applies only to the bin average transformation method." },
    { "bit_13_description", "QC_INDETERMINATE_GOODFRAC:  Fraction of good and indeterminate points over averaging interval is less than limit set by transform parameter goodfrac_ind_min." },
    { "bit_13_assessment",  "Indeterminate" },
    { "bit_13_comment",     "Applies only to the bin average transformation method." },
    { NULL, NULL }
};

static TransAtts gConsTransQCAtts[] =
{
    { "units",             "unitless" },
    { "description",       "This field contains bit packed integer values, where each bit represents a QC test on the data. Non-zero bits indicate the QC condition given in the description for those bits; a value of 0 (no bits set) indicates the data has not failed any QC tests." },
    { "flag_method",       "bit" },
    { "bit_1_description", "Transformation could not finish (all values bad or outside range, etc.), value set to missing_value." },
    { "bit_1_assessment",  "Bad" },
    { "bit_2_description", "Transformation resulted in an indeterminate outcome." },
    { "bit_2_assessment",  "Indeterminate" },
    { NULL, NULL }
};

static TransAtts gCaracenaQCAtts[] =
{
    { "units",             "unitless" },
    { "description",       "This field contains bit packed integer values, where each bit represents a QC test on the data. Non-zero bits indicate the QC condition given in the description for those bits; a value of 0 (no bits set) indicates the data has not failed any QC tests." },
    { "flag_method",       "bit" },
    { NULL, NULL }
};

/**
 *  Get the output bits to use when consolidating transformation QC bits.
 *
 *  This function will use the bit description attributes to determine the
 *  correct bits for bad and indeterminate when consolidating transformation
 *  QC variables.
 *
 *  @param  prefix      attribute name prefix before the bit number
 *  @param  natts       number of attributes in the list
 *  @param  atts        list of attributes
 *  @param  bad_flag    output: QC flag to use for bad values
 *  @param  ind_flag    output: QC flag to use for indeterminate values
 *  @param  nfound      output: total number of qc assessment attributes found
 *  @param  max_bit_num output: highest QC bit number used
 *
 *  @retval  1  if the bad and ind flags could be determined
 *  @retval  0  if the required bit description attributes were not found.
 */
static int _dsproc_get_trans_qc_rollup_bits(
    const char   *prefix,
    int           natts,
    CDSAtt      **atts,
    unsigned int *bad_flag,
    unsigned int *ind_flag,
    int          *nfound,
    int          *max_bit_num)
{
    CDSAtt      *att;
    size_t       prefix_length;
    size_t       name_length;
    int          bit_num;
    int          ai;

    const char  *bad_desc;
    size_t       bad_desc_len;
    const char  *ind_desc;
    size_t       ind_desc_len;

    if (gTransQCRollupBadDesc) {
        bad_desc = gTransQCRollupBadDesc;
    }
    else {
        bad_desc = gTransQCRollupBadDescDefault;
    }

    if (gTransQCRollupIndDesc) {
        ind_desc = gTransQCRollupIndDesc;
    }
    else {
        ind_desc = gTransQCRollupIndDescDefault;
    }

    bad_desc_len = strlen(bad_desc);
    ind_desc_len = strlen(ind_desc);

    *bad_flag = 0;
    *ind_flag = 0;

    if (nfound)      *nfound      = 0;
    if (max_bit_num) *max_bit_num = 0;

    prefix_length = strlen(prefix);

    for (ai = 0; ai < natts; ++ai) {

        att = atts[ai];

        /* Check if this is a QC description attribute */

        if ((att->type != CDS_CHAR) ||
            (strncmp(att->name, prefix, prefix_length) != 0)) {

            continue;
        }

        name_length = strlen(att->name);
        if (name_length < prefix_length + 13 ||
            strcmp( (att->name + name_length - 11), "description") != 0) {

            continue;
        }

        bit_num = atoi(att->name + prefix_length);
        if (!bit_num) continue;

        /* Keep track of the number of description attributes found,
         * and the highest bit number used if requested */

        if (nfound) *nfound += 1;

        if (max_bit_num &&
            *max_bit_num < bit_num) {

            *max_bit_num = bit_num;
        }

        /* Check for the descriptions we are looking for */

        if (strncmp(att->value.cp,
            bad_desc, bad_desc_len) == 0) {

            *bad_flag |= 1 << (bit_num-1);
        }
        else if (strncmp(att->value.cp,
            ind_desc, ind_desc_len) == 0) {

            *ind_flag |= 1 << (bit_num-1);
        }
    }

    if (*bad_flag && *ind_flag) {
        return(1);
    }

    return(0);
}

/**
 *  Static: Cleanup previously transformed data.
 *
 *  This function will cleanup all data created by a previous transformation
 *  and prepare it to transform data for the next processing interval.
 */
static void _dsproc_cleanup_transformed_data(void)
{
    if (_DSProc->trans_data) {
        cds_set_definition_lock(_DSProc->trans_data, 0);
        cds_delete_group(_DSProc->trans_data);
        _DSProc->trans_data = (CDSGroup *)NULL;
    }
}

/**
 *  Static: Find the coordinate variable to map a transform dimension to.
 *
 * BDE: More work is needed to support obs based processing and mapped
 * coordinate variables... I'm not even sure how to handle this yet without
 * splitting observations from one datastream that overlap two observations
 * in another...
 *
 *  @param  ret_coorddim  - pointer to the RetCoordDim
 *  @param  ret_dsid      - output: datastream ID of the retrieved datastream
 *  @param  ret_coord_var - output: pointer to the variable in the retrieved data
 *
 *  @return
 *    -  1 if found
 *    -  0 if not found
 *    - -1 an error occurred
 */
static int _dsproc_get_mapped_ret_coord_var(
    RetCoordDim *ret_coorddim, int *ret_dsid, CDSVar **ret_coord_var)
{
    const char  *dim_name;
    RetDsVarMap *varmap;
    int          in_dsid;
    DataStream  *in_ds;
    RetDsCache  *cache;
    CDSGroup    *ret_obs_group;
    int          vmi;

    /* Loop over the entries in the variable map. */

/* BDE:
 * Krista said something about only looking at the first
 * entry in the map... but the retriever allows multiple entries
 * and so does the PCM so I am not doing anything here to pevent it.
 */

    dim_name       = ret_coorddim->name;
    *ret_dsid      = -1;
    *ret_coord_var = (CDSVar *)NULL;

    for (vmi = 0; vmi < ret_coorddim->nvarmaps; vmi++) {

        varmap = ret_coorddim->varmaps[vmi];

        /* Get the input datastream for this entry */

        in_dsid = dsproc_get_datastream_id(
            varmap->ds->site,
            varmap->ds->facility,
            varmap->ds->name,
            varmap->ds->level,
            DSR_INPUT);

        if (in_dsid < 0) {
            continue;
        }

        in_ds = _DSProc->datastreams[in_dsid];
        cache = in_ds->ret_cache;

        if (!cache ||
            !cache->ds_group ||
            !cache->ds_group->ngroups) {

            continue;
        }

/* BDE OBS UPDATE: We probably need to pass in an obs index here */

        /* Get the coordinate variable from the first observation */

        ret_obs_group  = cache->ds_group->groups[0];
        *ret_coord_var = cds_get_var(ret_obs_group, dim_name);

        if (*ret_coord_var) {

            if ((*ret_coord_var)->ndims != 1) {

                ERROR( DSPROC_LIB_NAME,
                    "Invalid coordinate variable specified in retriever: %s->%s\n"
                    " -> coordinate variables must have one and only one dimension\n",
                    ret_obs_group->name, dim_name);

                dsproc_set_status(DSPROC_ETRANSFORM);
                return(-1);
            }

            *ret_dsid = in_dsid;
            break;
        }

    } /* end loop over variable maps */

    if (*ret_coord_var) return(1);
    return(0);
}

/**
 *  Static: Copy a variable from a retriever group to a transform group.
 *
 *  This function requires that the trans_group dimensions used by the
 *  variable are already defined.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param ret_var         - pointer to the retrieved CDSVar
 *  @param nmap_dims       - number of dimension names that are being mapped
 *                           from the retrieved data to the transformed data.
 *  @param ret_dim_names   - names of the dimensions in the retrieved data.
 *  @param trans_dim_names - names of the dimensions in the transformed data.
 *  @param trans_group     - pointer to the transform CDSGroup
 *  @param trans_var_type  - desired data type of the trans_var,
 *                           or CDS_NAT for no data type conversion.
 *  @param copy_data       - flag indicating if the data should be copied
 *                           (1 == TRUE, 0 == FALSE)
 *
 *  @return
 *    - pointer to the trans_var
 *    - NULL if an error occurred
 */
static CDSVar *_dsproc_clone_ret_var(
    CDSVar      *ret_var,
    int          nmap_dims,
    const char **ret_dim_names,
    const char **trans_dim_names,
    CDSGroup    *trans_group,
    CDSDataType  trans_var_type,
    int          copy_data)
{
    const char *dim_names[NC_MAX_DIMS];
    CDSVar     *trans_var;
    int         di, mi;

    /* Map the dimension names used by the retrieved variable
     * to the dimension names used in the transformation group. */

    for (di = 0; di < ret_var->ndims; di++) {

        for (mi = 0; mi < nmap_dims; mi++) {

            if (strcmp(ret_var->dims[di]->name, ret_dim_names[mi]) == 0) {
                dim_names[di] = trans_dim_names[mi];
                break;
            }
        }

        if (mi == nmap_dims) {
            dim_names[di] = ret_var->dims[di]->name;
        }
    }

    /* Clone the variable */

    trans_var = dsproc_clone_var(ret_var, trans_group,
        ret_var->name, trans_var_type, dim_names, copy_data);

    if (!trans_var) {
        return((CDSVar *)NULL);
    }

    return(trans_var);
}

/**
 *  Static: Copy global attributes from a retriever group to a transform group.
 *
 *  This has less overhead than the cds_copy_atts function because we know
 *  it is a clean copy into a newly defined CDSGroup. It also provides a
 *  place to do some additional checking/filtering on the attributes if we
 *  ever need to.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param ret_group   - pointer to the retriever CDSGroup
 *  @param trans_group - pointer to the transform CDSGroup
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
static int _dsproc_copy_ret_atts_to_trans_group(
    CDSGroup *ret_group,
    CDSGroup *trans_group)
{
    CDSAtt *att;
    int     ai;

    for (ai = 0; ai < ret_group->natts; ai++) {

        att = ret_group->atts[ai];

        if (!cds_define_att(trans_group,
            att->name, att->type, att->length, att->value.vp)) {

            dsproc_set_status(DSPROC_ENOMEM);
            return(0);
        }
    }

    return(1);
}

/**
 *  Static: Copy a variable from a retriever group to a transform group.
 *
 *  This function requires that the trans_group dimensions used by the
 *  variable are already defined.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param ret_var         - pointer to the retrieved CDSVar
 *  @param nmap_dims       - number of dimension names that are being mapped
 *                           from the retrieved data to the transformed data.
 *  @param ret_dim_names   - names of the dimensions in the retrieved data.
 *  @param trans_dim_names - names of the dimensions in the transformed data.
 *  @param trans_group     - pointer to the transform CDSGroup
 *  @param trans_var_type  - desired data type of the trans_var,
 *                           or CDS_NAT for no data type conversion.
 *  @param trans_var_units - desired units of the trans_var,
 *                           or NULL for no units conversion.
 *  @param copy_data       - flag indicating if the data should be copied
 *                           (1 == TRUE, 0 == FALSE)
 *  @param copy_qc_var     - flag indicating if the companion qc variable
 *                           should be copied (1 == TRUE, 0 == FALSE)
 *
 *  @return
 *    - pointer to the trans_var
 *    - NULL if an error occurred
 */
static CDSVar *_dsproc_copy_ret_var_to_trans_group(
    CDSVar      *ret_var,
    int          nmap_dims,
    const char **ret_dim_names,
    const char **trans_dim_names,
    CDSGroup    *trans_group,
    CDSDataType  trans_var_type,
    const char  *trans_var_units,
    int          copy_data,
    int          copy_qc_var)
{
    CDSVar     *trans_var;
    CDSVar     *ret_bounds_var;
    CDSVar     *trans_bounds_var;
    CDSVar     *ret_qc_var;
    CDSVar     *trans_qc_var;
    char        qc_var_name[NC_MAX_NAME];

    /* Clone the variable */

    trans_var = _dsproc_clone_ret_var(ret_var,
        nmap_dims, ret_dim_names, trans_dim_names,
        trans_group, trans_var_type, copy_data);

    if (!trans_var) return((CDSVar *)NULL);

    /* Check if there is an associated boundary variable */

    ret_bounds_var = cds_get_bounds_var(ret_var);
    if (ret_bounds_var) {

        /* Clone the boundary variable */

        trans_bounds_var = _dsproc_clone_ret_var(ret_bounds_var,
            nmap_dims, ret_dim_names, trans_dim_names,
            trans_group, trans_var_type, copy_data);

        if (!trans_bounds_var) return((CDSVar *)NULL);
    }

    /* Convert units if necessary, this will also convert the units
     * of the associated boundary variable if one exists. */

    if (trans_var_units) {

        if (!cds_change_var_units(
            trans_var, trans_var->type, trans_var_units)) {

            ERROR( DSPROC_LIB_NAME,
                "Could not convert transformation variable units for: %s->%s\n",
                trans_group->name, trans_var->name);

            dsproc_set_status(DSPROC_ETRANSFORM);
            return((CDSVar *)NULL);
        }
    }

    /* Copy the companion QC variable */

    if (copy_qc_var) {

        snprintf(qc_var_name, NC_MAX_NAME, "qc_%s", ret_var->name);

        ret_qc_var = cds_get_var((CDSGroup *)ret_var->parent, qc_var_name);

        if (ret_qc_var) {

            trans_qc_var = _dsproc_clone_ret_var(ret_qc_var,
                nmap_dims, ret_dim_names, trans_dim_names,
                trans_group, trans_var_type, copy_data);

            if (!trans_qc_var) return((CDSVar *)NULL);
        }
    }

    return(trans_var);
}

/**
 *  Static: Copy a variable from a retriever group to a transform group.
 *
 *  This function requires that the trans_group dimensions used by the
 *  variable are already defined.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param ret_var         - pointer to the retrieved CDSVar
 *  @param trans_group     - pointer to the transform CDSGroup
 *  @param trans_var_name  - names of the variable tio create.
 *  @param trans_var_ndims - number of variable dimension names
 *  @param trans_var_dims  - names of the dimensions in the transformed data.
 *  @param trans_var_type  - desired data type of the trans_var,
 *                           or CDS_NAT for no data type conversion.
 *
 *  @return
 *    - pointer to the trans_var
 *    - NULL if an error occurred
 */
static CDSVar *_dsproc_create_trans_var(
    CDSVar      *ret_var,
    CDSGroup    *trans_group,
    const char  *trans_var_name,
    int          trans_var_ndims,
    const char **trans_var_dims,
    CDSDataType  trans_var_type)
{
    CDSVar     *trans_var;
    const char *dims[NC_MAX_DIMS];
    int         status;
    int         di;

    /* Set NULL argument values */

    if (!trans_var_name)  trans_var_name = ret_var->name;
    if (!trans_var_type)  trans_var_type = ret_var->type;
    if (!trans_var_dims) {
        for (di = 0; di < ret_var->ndims; di++) {
            dims[di] = ret_var->dims[di]->name;
        }
        trans_var_dims  = dims;
        trans_var_ndims = ret_var->ndims;
    }

    /* Make sure this variable doesn't already exist in the trans_group */

    trans_var = cds_get_var(trans_group, trans_var_name);

    if (trans_var) {

        ERROR( DSPROC_LIB_NAME,
            "Could not create transformation variable: %s\n"
            " -> variable already exists\n",
            cds_get_object_path(trans_var));

        dsproc_set_status(DSPROC_ECDSDEFVAR);
        return((CDSVar *)NULL);
    }

    /* Define the variable */

    trans_var = cds_define_var(trans_group,
        trans_var_name, trans_var_type, trans_var_ndims, trans_var_dims);

    if (!trans_var) {
        dsproc_set_status(DSPROC_ENOMEM);
        return((CDSVar *)NULL);
    }

    /* Copy over the ret_var attributes */

    status = cds_copy_var(
        ret_var, trans_group, trans_var_name, NULL, NULL, NULL, NULL,
        0, 0, 0, CDS_SKIP_DATA, NULL);

    if (status < 0) {
        dsproc_set_status(DSPROC_ENOMEM);
        return((CDSVar *)NULL);
    }

    return(trans_var);
}

/**
 *  Static: Create a boundary variable in a CDSGroup.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param trans_coord_var - pointer to the trans_coord_var
 *
 *  @return
 *    -  1 if successful
 *    -  0 if alignment and/or width are not defined, or the width is 0
 *    - -1 if an error occurred
 */
static int _dsproc_create_trans_bounds_var(CDSVar *trans_coord_var)
{
    CDSGroup   *trans_group    = (CDSGroup *)trans_coord_var->parent;
    const char *coord_var_name = trans_coord_var->name;
    CDSDataType data_type      = trans_coord_var->type;
    char        bounds_var_name[NC_MAX_NAME];
    CDSVar     *trans_bounds_var;
    const char *dim_names[2];

    size_t      found_width;
    double      width;
    size_t      found_alignment;
    double      alignment;
    double      front_offset;
    double      back_offset;

    CDSData     front_edge;
    size_t      front_edge_length;
    CDSData     back_edge;
    size_t      back_edge_length;

    CDSAtt     *att;
    size_t      nsamples;
    CDSData     bounds_data;
    CDSData     coord_data;
    size_t      bi;

    /* Check if the bounds variable already exists */

    snprintf(bounds_var_name, NC_MAX_NAME, "%s_bounds", coord_var_name);

    if (cds_get_var(trans_group, bounds_var_name)) return(1);

    /* Check if a bound variable can be created for this coordinate variable */

    if (trans_coord_var->ndims != 1) {

        DEBUG_LV1( DSPROC_LIB_NAME,
            "%s->%s: Skipping creation of bounds variable\n"
            " -> coordinate variable has %d dimensions\n",
            trans_group->name, trans_coord_var->name, trans_coord_var->ndims);

        return(1);
    }

    if (data_type == CDS_CHAR) {

        DEBUG_LV1( DSPROC_LIB_NAME,
            "%s->%s: Skipping creation of bounds variable\n"
            " -> coordinate variable has character data type\n",
            trans_group->name, trans_coord_var->name);

        return(1);
    }

    /* Get the number of samples */

    nsamples = trans_coord_var->dims[0]->length;

    /* Check if front_edge and back_edge were specified */

    back_edge.vp = (void *)NULL;

    front_edge.vp = cds_get_transform_param(trans_coord_var,
        "front_edge", data_type, &front_edge_length, NULL);

    if (!front_edge.vp && front_edge_length != 0) {
        goto TRANSFORM_ERROR;
    }

    if (front_edge.vp) {

        back_edge.vp = cds_get_transform_param(trans_coord_var,
            "back_edge", data_type, &back_edge_length, NULL);

        if (!back_edge.vp && back_edge_length != 0) {
            goto TRANSFORM_ERROR;
        }
    }

    if (front_edge.vp && back_edge.vp) {

        if (front_edge_length != back_edge_length ||
            front_edge_length != nsamples) {

            ERROR( DSPROC_LIB_NAME,
                "Invalid transformation parameters for boundary variable: %s->%s\n"
                " - number of samples = %d\n"
                " - front_edge length = %d\n"
                " - back_edge length  = %d\n",
                trans_group->name, bounds_var_name,
                nsamples, front_edge_length, back_edge_length);

            dsproc_set_status(DSPROC_ETRANSFORM);
            return(-1);
        }

        DEBUG_LV1( DSPROC_LIB_NAME,
            "%s->%s\n"
            " - creating bounds variable using front_edge and back_edge trans params\n",
            trans_group->name, bounds_var_name);
    }
    else {
        /* Check if a width and alignment were specified */

        found_width = 1;
        cds_get_transform_param(trans_coord_var,
            "width", CDS_DOUBLE, &found_width, &width);

        found_alignment = 1;
        cds_get_transform_param(trans_coord_var,
            "alignment", CDS_DOUBLE, &found_alignment, &alignment);

        if (!found_width || !found_alignment || !width) return(0);

        DEBUG_LV1( DSPROC_LIB_NAME,
            "%s->%s\n"
            " - creating bounds variable\n"
            " - width     = %g\n"
            " - alignment = %g\n",
            trans_group->name, bounds_var_name, width, alignment);
    }

    /* Create the bound dimension if it does not exist. */

    if (!cds_get_dim(trans_group, "bound")) {
        if (!cds_define_dim(trans_group, "bound", 2, 0)) {
            goto TRANSFORM_ERROR;
        }
    }

    /* Create the boundary variable */

    dim_names[0] = trans_coord_var->dims[0]->name;
    dim_names[1] = "bound";

    trans_bounds_var = cds_define_var(trans_group,
        bounds_var_name, data_type, 2, dim_names);

    if (!trans_bounds_var) {
        goto TRANSFORM_ERROR;
    }

    /* Define the boundary variable attributes */

    att = cds_define_att_text(trans_bounds_var,
        "long_name", "%s cell bounds", coord_var_name);

    if (!att) goto TRANSFORM_ERROR;

    *att->value.cp = toupper(*att->value.cp);

    /* Allocate memory and set the data values in the boundary variable */

    if (!cds_alloc_var_data(trans_bounds_var, 0, nsamples)) {
        goto TRANSFORM_ERROR;
    }

    if (front_edge.vp && back_edge.vp) {

        bounds_data.vp = trans_bounds_var->data.vp;

        switch (data_type) {

            case CDS_BYTE:

                for (bi = 0; bi < nsamples; ++bi) {
                    *bounds_data.bp++ = front_edge.bp[bi];
                    *bounds_data.bp++ = back_edge.bp[bi];
                }
                break;

            case CDS_SHORT:

                for (bi = 0; bi < nsamples; ++bi) {
                    *bounds_data.sp++ = front_edge.sp[bi];
                    *bounds_data.sp++ = back_edge.sp[bi];
                }
                break;

            case CDS_INT:

                for (bi = 0; bi < nsamples; ++bi) {
                    *bounds_data.ip++ = front_edge.ip[bi];
                    *bounds_data.ip++ = back_edge.ip[bi];
                }
                break;

            case CDS_FLOAT:

                for (bi = 0; bi < nsamples; ++bi) {
                    *bounds_data.fp++ = front_edge.fp[bi];
                    *bounds_data.fp++ = back_edge.fp[bi];
                }
                break;

            case CDS_DOUBLE:

                for (bi = 0; bi < nsamples; ++bi) {
                    *bounds_data.dp++ = front_edge.dp[bi];
                    *bounds_data.dp++ = back_edge.dp[bi];
                }
                break;

            default:

                ERROR( DSPROC_LIB_NAME,
                    "Invalid data type '%d' for bounds variable: %s\n",
                    trans_bounds_var->type, cds_get_object_path(trans_bounds_var));

                dsproc_set_status(DSPROC_ETRANSFORM);
                return(-1);
        }
    }
    else {

        coord_data.vp  = trans_coord_var->data.vp;
        bounds_data.vp = trans_bounds_var->data.vp;

        front_offset = width * alignment;
        back_offset  = width * (1.0 - alignment);

        switch (data_type) {

            case CDS_BYTE:

                for (bi = 0; bi < nsamples; ++bi) {
                    *bounds_data.bp++ = *coord_data.bp - front_offset;
                    *bounds_data.bp++ = *coord_data.bp + back_offset;
                    ++coord_data.bp;
                }
                break;

            case CDS_SHORT:

                for (bi = 0; bi < nsamples; ++bi) {
                    *bounds_data.sp++ = *coord_data.sp - front_offset;
                    *bounds_data.sp++ = *coord_data.sp + back_offset;
                    ++coord_data.sp;
                }
                break;

            case CDS_INT:

                for (bi = 0; bi < nsamples; ++bi) {
                    *bounds_data.ip++ = *coord_data.ip - front_offset;
                    *bounds_data.ip++ = *coord_data.ip + back_offset;
                    ++coord_data.ip;
                }
                break;

            case CDS_FLOAT:

                for (bi = 0; bi < nsamples; ++bi) {
                    *bounds_data.fp++ = *coord_data.fp - front_offset;
                    *bounds_data.fp++ = *coord_data.fp + back_offset;
                    ++coord_data.fp;
                }
                break;

            case CDS_DOUBLE:

                for (bi = 0; bi < nsamples; ++bi) {
                    *bounds_data.dp++ = *coord_data.dp - front_offset;
                    *bounds_data.dp++ = *coord_data.dp + back_offset;
                    ++coord_data.dp;
                }
                break;

            default:

                ERROR( DSPROC_LIB_NAME,
                    "Invalid data type '%d' for bounds variable: %s\n",
                    trans_bounds_var->type, cds_get_object_path(trans_bounds_var));

                dsproc_set_status(DSPROC_ETRANSFORM);
                return(-1);
        }
    }

    /* Add the bounds attribute to the coordinate variable */

    att = cds_define_att_text(trans_coord_var,
        "bounds", "%s", bounds_var_name);

    if (!att) goto TRANSFORM_ERROR;

    if (front_edge.vp) free(front_edge.vp);
    if (back_edge.vp)  free(back_edge.vp);

    return(1);

TRANSFORM_ERROR:

    if (front_edge.vp) free(front_edge.vp);
    if (back_edge.vp)  free(back_edge.vp);

    ERROR( DSPROC_LIB_NAME,
        "Could not define transformation boundary variable: %s->%s\n",
        trans_group->name, bounds_var_name);

    dsproc_set_status(DSPROC_ETRANSFORM);
    return(-1);
}

/**
 *  Static: Create a dimension and coordinate variable in a CDSGroup.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  trans_group      - pointer to the transformation CDSGroup
 *  @param  dim_name         - dimension name
 *  @param  dim_length       - dimension length
 *  @param  dim_is_unlimited - flag specifying if this is an unlimited dimension
 *  @param  dim_type         - data type to use for the coordinate variable
 *  @param  dim_desc         - long_name to use for the coordinate variable
 *  @param  dim_units        - units of the coordinate variable data
 *  @param  dim_values       - coordinate variable data values,
 *                             or NULL to only allocate the memory.
 *
 *  @return
 *    - pointer to the coordinate variable
 *    - NULL if an error occurred
 */
static CDSVar *_dsproc_create_trans_coord_var(
    CDSGroup    *trans_group,
    const char  *dim_name,
    int          dim_length,
    int          dim_is_unlimited,
    CDSDataType  dim_type,
    const char  *dim_desc,
    const char  *dim_units,
    void        *dim_values)
{
    CDSVar *trans_coord_var;
    size_t  nbytes;

    /* Create the dimension */

    if (!cds_define_dim(trans_group,
        dim_name, dim_length, dim_is_unlimited)) {

        ERROR( DSPROC_LIB_NAME,
            "Could not define transformation dimension: %s->%s\n",
            trans_group->name, dim_name);

        dsproc_set_status(DSPROC_ETRANSFORM);
        return((CDSVar *)NULL);
    }

    /* Create the coordinate variable */

    trans_coord_var = cds_define_var(trans_group,
        dim_name, dim_type, 1, &dim_name);

    if (!trans_coord_var) {
        goto TRANSFORM_ERROR;
    }

    /* Allocate memory and set the data values in the coordinate variable */

    if (!cds_alloc_var_data(trans_coord_var, 0, dim_length)) {
        goto TRANSFORM_ERROR;
    }

    if (dim_values) {
        nbytes = dim_length * cds_data_type_size(dim_type);
        memcpy(trans_coord_var->data.vp, dim_values, nbytes);
    }

    /* Define the coordinate variable attributes */

    if (dim_desc) {
        if (!cds_define_att_text(trans_coord_var,
            "long_name", "%s", dim_desc)) {

            goto TRANSFORM_ERROR;
        }
    }
    else {
        if (!cds_define_att_text(trans_coord_var,
            "long_name", "Coordinate variable for dimension: %s", dim_name)) {

            goto TRANSFORM_ERROR;
        }
    }

    if (!cds_define_att_text(trans_coord_var,
        "units", "%s", dim_units)) {

        goto TRANSFORM_ERROR;
    }

    return(trans_coord_var);

TRANSFORM_ERROR:

    ERROR( DSPROC_LIB_NAME,
        "Could not define transformation coordinate variable: %s->%s\n",
        trans_group->name, dim_name);

    dsproc_set_status(DSPROC_ETRANSFORM);
    return((CDSVar *)NULL);
}

/**
 *  Static: Create a transformation QC variable in a CDSGroup.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  trans_group - pointer to the transformation CDSGroup
 *  @param  trans_var   - pointer to the variable in the transformation group
 *                        to create the QC variable for.
 *  @param  is_caracena - specifies if this is a Caracena transformation
 *
 *  @return
 *    - pointer to the QC variable
 *    - NULL if an error occurred
 */
static CDSVar *_dsproc_create_trans_qc_var(
    CDSGroup *trans_group,
    CDSVar   *trans_var,
    int       is_caracena)
{
    const char *dim_names[NC_MAX_DIMS];
    char        trans_qc_var_name[NC_MAX_NAME];
    CDSVar     *trans_qc_var;
    CDSAtt     *long_name_att;
    char       *long_name;
    int         ai, di;

    /* Create the dimension names list */

    for (di = 0; di < trans_var->ndims; di++) {
        dim_names[di] = trans_var->dims[di]->name;
    }

    /* Create the QC variable */

    snprintf(trans_qc_var_name, NC_MAX_NAME, "qc_%s", trans_var->name);

    trans_qc_var = cds_define_var(trans_group,
        trans_qc_var_name, CDS_INT, trans_var->ndims, dim_names);

    if (!trans_qc_var) {
        goto TRANSFORM_ERROR;
    }

    /* Define the long_name attributes */

    long_name_att = cds_get_att(trans_var, "long_name");
    if (long_name_att && long_name_att->type == CDS_CHAR) {
        long_name = long_name_att->value.cp;
    }
    else {
        long_name = trans_var->name;
    }

    if (!cds_define_att_text(trans_qc_var,
        "long_name",
        "Quality check results on field: %s", long_name)) {

        goto TRANSFORM_ERROR;
    }

    /* Define the QC attributes */

    if (is_caracena) {
        for (ai = 0; gCaracenaQCAtts[ai].name; ++ai) {

            if (!cds_define_att_text(trans_qc_var,
                gCaracenaQCAtts[ai].name,
                gCaracenaQCAtts[ai].value)) {

                goto TRANSFORM_ERROR;
            }
        }
    }
    else {

        for (ai = 0; gTransQCAtts[ai].name; ++ai) {

            if (!cds_define_att_text(trans_qc_var,
                gTransQCAtts[ai].name,
                gTransQCAtts[ai].value)) {

                goto TRANSFORM_ERROR;
            }
        }
    }
    
    return(trans_qc_var);

TRANSFORM_ERROR:

    ERROR( DSPROC_LIB_NAME,
        "Could not create transformation QC variable: %s->%s\n",
        trans_group->name, trans_qc_var_name);

    dsproc_set_status(DSPROC_ETRANSFORM);
    return((CDSVar *)NULL);
}

/**
 *  Static: Add a dimension and coordinate variable to a coordinate system.
 *
 *  @param coordsys_name   - name of the coordinate system
 *  @param ret_var         - pointer to the retrieved CDSVar
 *  @param dim_name        - name of the coordinate system dimension to create.
 *  @param ret_coord_var   - pointer to the retrieved coordinate variable
 *  @param trans_coordsys  - pointer to the transformation CDSGroup in which to
 *                           create the dimension and coordinate variable.
 *  @param trans_coord_var - output: pointer to the trans_coord_var
 *
 *  @return
 *    -  1 if successful
 *    -  0 if the mapped coordinate variable was not found
 *    - -1 if an error occurred
 */
static int _dsproc_create_trans_coordsys_dimension(
    const char *coordsys_name,
    CDSVar     *ret_var,
    const char *dim_name,
    CDSVar     *ret_coord_var,
    CDSGroup   *trans_coordsys,
    CDSVar    **trans_coord_var)
{
    RetCoordSystem  *ret_coordsys;
    RetCoordDim     *ret_coorddim;
    CDSDim          *ret_dim;
    int              ret_dsid;

    size_t           param_length;
    char             type_buffer[16];
    char             unit_buffer[64];
    const char      *tpf_units;
    const char      *var_units;
    int              is_time_dim;
    size_t           end_index;
    int              dim_start_from_var;

    int              dim_length;
    int              dim_is_unlimited;
    CDSDataType      dim_type;
    const char      *dim_desc;
    const char      *dim_units;
    void            *dim_values;

    double           dim_start;
    double           dim_end;
    double           dim_interval;

    CDSGroup        *var_parent;
    int              status;
    int              di, rcsi;

    const char      *from_units;
    const char      *to_units;
    int              convert_values;
    CDSUnitConverter unit_converter;

    const char      *unitless = "unitless";
    const char      *seconds  = "seconds";

    dim_desc = (const char *)NULL;

    /**********************************************************************
    * Force time to be unlimited in the transformed data
    * and all other dimensions to be static.
    **********************************************************************/

    if (strcmp(dim_name, "time") == 0) {
        is_time_dim      = 1;
        dim_is_unlimited = 1;
    }
    else {
        is_time_dim      = 0;
        dim_is_unlimited = 0;
    }

    /**********************************************************************
    * Check for the coordinate system dimension in the retriever definition.
    **********************************************************************/

    ret_coorddim = (RetCoordDim *)NULL;

    if (coordsys_name) {

        ret_coordsys = _dsproc_get_ret_coordsys(coordsys_name);

        if (ret_coordsys) {
            for (rcsi = 0; rcsi < ret_coordsys->ndims; rcsi++) {
                if (strcmp(ret_coordsys->dims[rcsi]->name, dim_name) == 0) {
                    ret_coorddim = ret_coordsys->dims[rcsi];
                    break;
                }
            }
        }
    }

    /**********************************************************************
    * Determine the data type to use for the coordinate variable.
    **********************************************************************/

    dim_type = CDS_NAT;

    if (ret_coorddim && ret_coorddim->data_type) {

        /* Use the data type specified in the PCM */

        dim_type = cds_data_type(ret_coorddim->data_type);
    }
    else {

        param_length = 16;

        cds_get_transform_param_from_group(trans_coordsys, dim_name,
            "data_type", CDS_CHAR, &param_length, type_buffer);

        if (param_length > 0) {

            /* Use the data type specified in the params file */

            dim_type = cds_data_type(type_buffer);
        }
    }

    /**********************************************************************
    * Determine the units to use for the coordinate variable.
    **********************************************************************/

    dim_units = (const char *)NULL;
    tpf_units = (const char *)NULL;

    if (ret_coorddim && ret_coorddim->units) {

        /* Use the units specified in the PCM */

        dim_units = ret_coorddim->units;
    }
    else {

        param_length = 64;

        cds_get_transform_param_from_group(trans_coordsys, dim_name,
            "units", CDS_CHAR, &param_length, unit_buffer);

        if (param_length > 0) {

            /* Use the units specified in the params file */

            dim_units = unit_buffer;
            tpf_units = unit_buffer;
        }
    }

    /**********************************************************************
    * Check if we are mapping this dimension to a coordinate variable
    * defined in the retriever.
    **********************************************************************/

    if (ret_coorddim && ret_coorddim->nvarmaps) {

        /* Find the coordinate variable specified by the variable map */

        status = _dsproc_get_mapped_ret_coord_var(
            ret_coorddim, &ret_dsid, &ret_coord_var);

        if (status <= 0) {
            return(status);
        }

        var_parent = (CDSGroup *)ret_coord_var->parent;

        DEBUG_LV1( DSPROC_LIB_NAME,
            "%s->%s\n"
            " - creating mapped coordinate system dimension\n"
            " - using retrieved variable: %s->%s\n",
            trans_coordsys->name, dim_name,
            var_parent->name, ret_coord_var->name);

        /* Copy across the dimension definition */

        ret_dim = ret_coord_var->dims[0];

        if (!cds_define_dim(
            trans_coordsys,
            dim_name,
            ret_dim->length,
            dim_is_unlimited)) {

            dsproc_set_status(DSPROC_ENOMEM);
            return(-1);
        }

        /* If this is the time dimension we do not want to change its
         * data type or units, regardless of what is specified in the
         * PCM or transformation parameters file. */

        if (is_time_dim) {
            dim_type  = CDS_NAT;
            dim_units = (const char *)NULL;
        }

        /* Copy across the coordinate variable and companion
         * boundary and QC variables if they exist. */

        *trans_coord_var = _dsproc_copy_ret_var_to_trans_group(
            ret_coord_var, 1, (const char **)&(ret_dim->name), &dim_name,
            trans_coordsys, dim_type, dim_units, 1, 1);

        if (!_dsproc_set_trans_coord_var_params(
            *trans_coord_var, ret_dsid, ret_coorddim)) {

            return(-1);
        }

        if (_dsproc_create_trans_bounds_var(*trans_coord_var) < 0) {
            return(-1);
        }

        return(1);

    } /* end if mapped coordinate variable */

    /**********************************************************************
    * Check if the coordinate variable values were specified in the
    * transformation parameters file.
    **********************************************************************/

    dim_values = cds_get_transform_param_from_group(
        trans_coordsys, dim_name,
        "values", CDS_DOUBLE, &param_length, NULL);

    if (param_length == (size_t)-1) {

        ERROR( DSPROC_LIB_NAME,
            "Could not get coordinate variable values for: %s->%s\n"
            " -> memory allocation error\n",
            trans_coordsys->name, dim_name);

        dsproc_set_status(DSPROC_ENOMEM);
        return(-1);
    }

    if (dim_values) {

        DEBUG_LV1( DSPROC_LIB_NAME,
            "%s->%s\n"
            " - creating coordinate system dimension\n"
            " - using values specified in the transformation parameters\n",
            trans_coordsys->name, dim_name);

        dim_length = (int)param_length;

        /* Make sure we have the units specifed in the transformation
         * parameters file. */

        if (dim_units && !tpf_units) {

            param_length = 64;

            cds_get_transform_param_from_group(trans_coordsys, dim_name,
                "units", CDS_CHAR, &param_length, unit_buffer);

            if (param_length > 0) {
                tpf_units = unit_buffer;
            }
        }

        /* Check if we need to convert the data type and/or units of the
         * values specified in the transform params file to the data type
         * and/or units specified in the PCM.
         *
         * Unless this is the time dimension... then we need to make sure
         * the units are in seconds.
         */

        convert_values = 0;
        unit_converter = (CDSUnitConverter *)NULL;

        if (is_time_dim) {
            dim_type  = CDS_DOUBLE;
            dim_units = "seconds";
        }
        else if (dim_type == CDS_NAT) {
            dim_type = CDS_DOUBLE;
        }
        else if (dim_type != CDS_DOUBLE) {
            convert_values = 1;
        }

        if (!dim_units) {
            dim_units = unitless;
        }
        else if (tpf_units && (dim_units != tpf_units)) {

            DEBUG_LV1( DSPROC_LIB_NAME,
                " - checking for units conversion\n");

            status = cds_get_unit_converter(
                tpf_units, dim_units, &unit_converter);

            if (status < 0) {

                ERROR( DSPROC_LIB_NAME,
                    "Could not convert coordinate variable values for: %s->%s\n",
                    trans_coordsys->name, dim_name);

                dsproc_set_status(DSPROC_ETRANSFORM);
                free(dim_values);
                return(-1);
            }

            if (status > 0) {
                convert_values = 1;
            }
        }

        if (convert_values) {

            DEBUG_LV1( DSPROC_LIB_NAME,
                " - converting values to data type and/or units specified in PCM\n");

            /* If we are converting data type, it will be from double
             * to something with a smaller type size so we can do the
             * conversion in place... */

            cds_convert_units(unit_converter,
                CDS_DOUBLE, (size_t)dim_length, dim_values,
                dim_type, dim_values,
                0, NULL, NULL, NULL, NULL, NULL, NULL);
        }

        if (unit_converter) {
            cds_free_unit_converter(unit_converter);
        }

        /* If this is the time dimension, use the units string corresponding
         * to the start of the current processing interval. */

        if (is_time_dim) {
            dim_desc  = _dsproc_get_ret_data_time_desc();
            dim_units = _dsproc_get_ret_data_time_units();
        }

        /* Create the dimension and coordinate variable */

        *trans_coord_var = _dsproc_create_trans_coord_var(
            trans_coordsys,
            dim_name, dim_length, dim_is_unlimited,
            dim_type, dim_desc, dim_units, dim_values);

        free(dim_values);

        if (!_dsproc_set_trans_coord_var_params(
            *trans_coord_var, -1, ret_coorddim)) {

            return(-1);
        }

        if (_dsproc_create_trans_bounds_var(*trans_coord_var) < 0) {
            return(-1);
        }

        return(1);

    } /* end if dim values specified in transformation parameters file */

    /**********************************************************************
    * Check if an interval was specified in the retriever definition,
    * or the transformation parameters file. If not then we have an
    * implicit mapping to itself.
    **********************************************************************/

    param_length = 1;

    if (ret_coorddim && ret_coorddim->interval) {

        cds_string_to_array(
            ret_coorddim->interval, CDS_DOUBLE, &param_length, &dim_interval);
    }
    else {
        cds_get_transform_param_from_group(trans_coordsys, dim_name,
            "interval", CDS_DOUBLE, &param_length, &dim_interval);
    }

    if (param_length == 0 || dim_interval == 0) {

        if (!ret_coord_var) {

            ERROR( DSPROC_LIB_NAME,
                "Could not create coordinate system dimension: %s->%s\n"
                " -> interval not specified in retriever definition\n",
                trans_coordsys->name, dim_name);

            dsproc_set_status(DSPROC_ETRANSFORM);
            return(-1);
        }

        var_parent = (CDSGroup *)ret_coord_var->parent;

        DEBUG_LV1( DSPROC_LIB_NAME,
            "%s->%s\n"
            " - creating implicitly mapped coordinate system dimension\n"
            " - using retrieved variable: %s->%s\n",
            trans_coordsys->name, dim_name,
            var_parent->name, ret_coord_var->name);

        /* Copy across the dimension definition */

        ret_dim = ret_coord_var->dims[0];

        if (!cds_define_dim(
            trans_coordsys,
            dim_name,
            ret_dim->length,
            dim_is_unlimited)) {

            dsproc_set_status(DSPROC_ENOMEM);
            return(-1);
        }

        /* If this is the time dimension we do not want to change its
         * data type or units, regardless of what is specified in the
         * PCM or transformation parameters file. */

        if (is_time_dim) {
            dim_type  = CDS_NAT;
            dim_units = (const char *)NULL;
        }

        /* Copy across the coordinate variable and companion
         * QC variable if it exists. */

        *trans_coord_var = _dsproc_copy_ret_var_to_trans_group(
            ret_coord_var, 1, (const char **)&(ret_dim->name), &dim_name,
            trans_coordsys, dim_type, dim_units, 1, 1);

        ret_dsid = dsproc_get_source_ds_id(ret_var);

        if (!_dsproc_set_trans_coord_var_params(
            *trans_coord_var, ret_dsid, ret_coorddim)) {

            return(-1);
        }

        if (_dsproc_create_trans_bounds_var(*trans_coord_var) < 0) {
            return(-1);
        }

        return(1);

    } /* end implicit mapping to self */

    /**********************************************************************
    * If we get here we will be calculating the coordinate values from
    * information specified in the PCM, transformation parameters file,
    * and/or the coordinate variable in the retrieved data.
    **********************************************************************/

    var_units = (const char *)NULL;

    if (dim_type == CDS_NAT) {

        if (ret_coord_var) {

            /* Use the data type of the retrieved coordinate variable */

            dim_type = ret_coord_var->type;
        }
        else {
            dim_type = CDS_DOUBLE;
        }
    }

    if (!dim_units) {

        if (ret_coord_var) {

            /* Use the units of the retrieved coordinate variable */

            var_units = cds_get_var_units(ret_coord_var);

            if (!var_units) {
                var_units = unitless;
            }
        }
        else {
            var_units = unitless;
        }

        dim_units = var_units;
    }

    /**********************************************************************
    * Get the start value to use for the coordinate variable.
    **********************************************************************/

    dim_start_from_var = 0;
    param_length       = 1;

    if (ret_coorddim && ret_coorddim->start) {

        /* Use the start value specified in the PCM */

        cds_string_to_array(
            ret_coorddim->start, CDS_DOUBLE, &param_length, &dim_start);
    }
    else {

        /* Check the params file for a start value */

        cds_get_transform_param_from_group(trans_coordsys, dim_name,
            "start", CDS_DOUBLE, &param_length, &dim_start);
    }

    if (param_length == 0) {

        if (!ret_coord_var) {

            ERROR( DSPROC_LIB_NAME,
                "Could not create coordinate system dimension: %s->%s\n"
                " -> start value not specified in retriever definition\n",
                trans_coordsys->name, dim_name);

            dsproc_set_status(DSPROC_ETRANSFORM);
            return(-1);
        }

        param_length = 1;

        /* Use the start value in the retrieved data */

        cds_get_var_data(ret_coord_var,
            CDS_DOUBLE, 0, &param_length, NULL, &dim_start);

        dim_start_from_var = 1;
    }

    /**********************************************************************
    * Determine the length of the dimension.
    **********************************************************************/

    param_length = 1;

    if (ret_coorddim && ret_coorddim->length) {

        /* Use the length specified in the PCM */

        cds_string_to_array(
            ret_coorddim->length, CDS_INT, &param_length, &dim_length);
    }
    else {

        /* Check the params file for a dimension length */

        cds_get_transform_param_from_group(trans_coordsys, dim_name,
            "length", CDS_INT, &param_length, &dim_length);
    }

    if (param_length == 0) {

        if (!ret_coord_var) {

            ERROR( DSPROC_LIB_NAME,
                "Could not create coordinate system dimension: %s->%s\n"
                " -> length not specified in retriever definition\n",
                trans_coordsys->name, dim_name);

            dsproc_set_status(DSPROC_ETRANSFORM);
            return(-1);
        }

        /* We will need the end value in the retrieved data
         * to calculate the length. */

        end_index = ret_coord_var->sample_count - 1;

        cds_get_var_data(ret_coord_var,
            CDS_DOUBLE, end_index, &param_length, NULL, &dim_end);

        dim_length = CDS_FILL_INT;
    }

    /**********************************************************************
    * Do units conversions as necessary:
    *
    *  - convert start value if it was read in from the retrieved variable
    *
    *  - convert end value if it was read in from the retrieved variable
    *    to calculate the length.
    *
    *  - if this is the time dimension convert user specified time interval
    *    to seconds.
    **********************************************************************/

    from_units = (const char *)NULL;
    to_units   = (const char *)NULL;

    if (is_time_dim) {

        /* if (dim_units == var_units) then the units were not specified by
         * the user and we assume that all units are already in seconds so
         * a unit conversion is not necessary. */

        if (dim_units != var_units) {
            from_units = dim_units;
            to_units   = seconds;
        }
    }
    else {

        /* if (dim_units == var_units) then the units were not specified by
         * the user and we assume that all units are the same as the units
         * of the retrieved variable so a unit conversion is not necessary. */

        if (dim_units != var_units) {

            if (!var_units && ret_coord_var) {
                var_units = cds_get_var_units(ret_coord_var);
            }

            if (var_units) {
                from_units = var_units;
                to_units   = dim_units;
            }
        }
    }

    if (from_units && to_units) {

        status = cds_get_unit_converter(
            from_units, to_units, &unit_converter);

        if (status < 0) {

            ERROR( DSPROC_LIB_NAME,
                "Could not convert coordinate variable values for: %s->%s\n",
                trans_coordsys->name, dim_name);

            dsproc_set_status(DSPROC_ETRANSFORM);
            return(-1);
        }

        if (status > 0) {

            if (is_time_dim) {

                if (!dim_start_from_var) {
                    cds_convert_units(unit_converter,
                        CDS_DOUBLE, 1, &dim_start, CDS_DOUBLE, &dim_start,
                        0, NULL, NULL, NULL, NULL, NULL, NULL);
                }

                cds_convert_units(unit_converter,
                    CDS_DOUBLE, 1, &dim_interval, CDS_DOUBLE, &dim_interval,
                    0, NULL, NULL, NULL, NULL, NULL, NULL);
            }
            else {

                if (dim_start_from_var) {
                    cds_convert_units(unit_converter,
                        CDS_DOUBLE, 1, &dim_start, CDS_DOUBLE, &dim_start,
                        0, NULL, NULL, NULL, NULL, NULL, NULL);
                }

                if (dim_length == CDS_FILL_INT) {
                    cds_convert_units(unit_converter,
                        CDS_DOUBLE, 1, &dim_end, CDS_DOUBLE, &dim_end,
                        0, NULL, NULL, NULL, NULL, NULL, NULL);
                }
            }

            cds_free_unit_converter(unit_converter);
        }
    }

    /* Calculate the dimension length if necessary */

    if (dim_length == CDS_FILL_INT) {

        dim_length = (int)((dim_end - dim_start) / dim_interval) + 1;

        if (dim_length <= 0) {

            ERROR( DSPROC_LIB_NAME,
                "Could not calculate dimension length for: %s->%s\n"
                " -> dimension length is less than or equal to zero\n"
                " -> start value = %.15g\n"
                " -> end value   = %.15g\n"
                " -> interval    = %.15g\n",
                trans_coordsys->name, dim_name,
                dim_start, dim_end, dim_interval);

            dsproc_set_status(DSPROC_ETRANSFORM);
            return(-1);
        }
    }

    /* Create the dimension and coordinate variable (as type double) */

    DEBUG_LV1( DSPROC_LIB_NAME,
        "%s->%s\n"
        " - creating user defined coordinate system dimension\n"
        " - start value: %.15g\n"
        " - length:      %d\n"
        " - interval:    %.15g %s\n",
        trans_coordsys->name, dim_name,
        dim_start, dim_length, dim_interval, dim_units);

    /* If this is the time dimension we need to use the units string
     * corresponding to the start of the current processing interval. */

    if (is_time_dim) {
        dim_type  = CDS_DOUBLE;
        dim_desc  = _dsproc_get_ret_data_time_desc();
        dim_units = _dsproc_get_ret_data_time_units();
    }
    else {
        dim_desc = (const char *)NULL;
    }

    *trans_coord_var = _dsproc_create_trans_coord_var(
        trans_coordsys,
        dim_name, dim_length, dim_is_unlimited,
        CDS_DOUBLE, dim_desc, dim_units, NULL);

    if (!(*trans_coord_var)) {
        return(-1);
    }

    /* Create the coordinate variable values (as type double) */

    (*trans_coord_var)->data.dp[0] = dim_start;

    for (di = 1; di < dim_length; di++) {
        (*trans_coord_var)->data.dp[di] = (*trans_coord_var)->data.dp[di-1]
                                     + dim_interval;
    }

    /* Convert variable data type if necessary */

    if (dim_type != CDS_DOUBLE) {
        cds_change_var_type(*trans_coord_var, dim_type);
    }

    if (!_dsproc_set_trans_coord_var_params(
        *trans_coord_var, -1, ret_coorddim)) {

        return(-1);
    }

    if (_dsproc_create_trans_bounds_var(*trans_coord_var) < 0) {
        return(-1);
    }

    return(1);
}

/**
 *  Static: Transform a variable into a coordinate system.
 *
 *  This function will transform the retrieved variable into the coordinate
 *  system specified in the ret_var_tag structure. If the coordinate system
 *  does not exist it will be created. All coordinate dimensions will also
 *  be created as necessary.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param trans_data    - pointer to the parent trans_data CDSGroup
 *  @param ret_ds_group  - pointer to the retrieved datastream group
 *  @param ret_obs_group - pointer to the retrieved observation group
 *  @param ret_var       - pointer to the retrieved variable
 *  @param ret_var_tag   - pointer to the retrieved variable's tag
 *  @param trans_var     - output: pointer to the transformed variable
 *
 *  @return
 *    -  1 if successful
 *    -  0 if a mapped coordinate variable was not found for an optional variable
 *    - -1 if an error occurred
 */
static int _dsproc_transform_variable(
    CDSGroup  *trans_data,
    CDSGroup  *ret_ds_group,
    CDSGroup  *ret_obs_group,
    CDSVar    *ret_var,
    VarTag    *ret_var_tag,
    CDSVar   **trans_var)
{
    char        auto_coordsys_name[256];
    const char *coordsys_name;

    CDSGroup   *trans_coordsys;
    CDSGroup   *trans_ds_group;
    CDSGroup   *trans_obs_group;

    CDSDim     *ret_dim;
    CDSVar     *ret_coord_var;
    const char *ret_units;
    CDSVar     *ret_qc_var;
    char        ret_qc_var_name[NC_MAX_NAME];

    CDSDim     *trans_dim;
    CDSVar     *trans_coord_var;
    const char *trans_units;
    CDSVar     *trans_qc_var;

    size_t      param_length;
    char        param_name[256];
    char        param_value[256];
    int         do_transform;
    int         dim_index;
    int         status;

    int             is_caracena;
    TransDimGroup  *trans_dim_groups;
    int             trans_dim_ngroups;
    char          **group_dims;
    int             group_ndims;
    const char     *trans_dim_name;
    int             tdgi, tdi;

    const char     *trans_var_dims[NC_MAX_VAR_DIMS];
    int             trans_var_ndims;

    /**********************************************************************
    * Get the CDSGroup for the coordinate system,
    * or create it if it does not exist.
    **********************************************************************/

    /* Get the name of the coordinate system */

    if (ret_var_tag->coordsys_name) {
        coordsys_name = ret_var_tag->coordsys_name;
    }
    else {

        snprintf(auto_coordsys_name, 256,
            "auto_%s_%s",
            ret_var_tag->in_ds->dsc_name,
            ret_var_tag->in_ds->dsc_level);

        coordsys_name = auto_coordsys_name;
    }

    /* Get or create the CDSGroup for this coordinate system */

    trans_coordsys = cds_get_group(trans_data, coordsys_name);

    if (!trans_coordsys) {

        DEBUG_LV1( DSPROC_LIB_NAME,
            "Creating transformation coordinate system: %s\n",
            coordsys_name);

        /* Create the coordinate system group */

        trans_coordsys = cds_define_group(trans_data, coordsys_name);
        if (!trans_coordsys) {
            dsproc_set_status(DSPROC_ENOMEM);
            return(-1);
        }

        /* Copy transformation parameters set by the user using the
         * dsproc_set_coordsys_trans_param() function.
         *
         * This is done *before* loading the trasformation parameters
         * file because that will take precedence over what is coded as
         * the default behavior.
         */

        status = dsproc_load_user_transform_params(
            coordsys_name, trans_coordsys);

        if (status < 0) return(-1);

        /* Load the transformation parameters file for this
         * coordinate system if one exists */

        status = dsproc_load_transform_params(
            trans_coordsys, _DSProc->site, _DSProc->facility,
            coordsys_name, NULL);

        if (status < 0) {
            return(-1);
        }
    }

    /**********************************************************************
    * Get the CDSGroup for this datastream in the coordinate system group,
    * or create it if it does not exist.
    **********************************************************************/

    trans_ds_group = cds_get_group(trans_coordsys, ret_ds_group->name);

    if (!trans_ds_group) {

        DEBUG_LV1( DSPROC_LIB_NAME,
            "Creating transformation datastream group: %s->%s\n",
            trans_coordsys->name, ret_ds_group->name);

        /* Create the datastream group */

        trans_ds_group = cds_define_group(trans_coordsys, ret_ds_group->name);
        if (!trans_ds_group) {
            dsproc_set_status(DSPROC_ENOMEM);
            return(-1);
        }

        /* Copy across the global attributes */

/* BDE OBS UPDATE: Copy ret_ds_group atts to trans_ds_group_atts
   when the code is updated to handle multiple obervations

        if (!_dsproc_copy_ret_atts_to_trans_group(
            ret_ds_group, trans_ds_group)) {

            return(0);
        }
*/
        if (!_dsproc_copy_ret_atts_to_trans_group(
            ret_obs_group, trans_ds_group)) {

            return(-1);
        }
/* END BDE OBS UPDATE */

    }

/* BDE OBS UPDATE: For now we are only supporting the cases where
 * all observations in the retrieved data could be merged */

    trans_obs_group = trans_ds_group;

#if 0
    /**********************************************************************
    * Get the CDSGroup for this observation in the datastream group,
    * or create it if it does not exist.
    **********************************************************************/

    trans_obs_group = cds_get_group(trans_ds_group, ret_obs_group->name);

    if (!trans_obs_group) {

        DEBUG_LV1( DSPROC_LIB_NAME,
            "Creating transformation observation group: %s->%s->%s\n",
            trans_coordsys->name, trans_ds_group->name, ret_obs_group->name);

        /* Create the observation group */

        trans_obs_group = cds_define_group(trans_ds_group, ret_obs_group->name);
        if (!trans_obs_group) {
            dsproc_set_status(DSPROC_ENOMEM);
            return(-1);
        }

        /* Copy across the global attributes */

        if (!_dsproc_copy_ret_atts_to_trans_group(
            ret_obs_group, trans_obs_group)) {

            return(-1);
        }
    }
#endif
/* END BDE OBS UPDATE */

    /**********************************************************************
    * Create the coordinate system dimensions and coordinate variables
    * if they do not already exist, and check if a transformation is
    * needed for this variable.
    **********************************************************************/

    do_transform    = 0;
    is_caracena     = 0;
    trans_var_ndims = 0;

    /* Check if dimension groupings were specified for this variable.
     *
     * This was added to support the Caracena Method where a single
     * station dimension is transformed onto a 2D lat/lon grid.
     */

    trans_dim_ngroups = _dsproc_get_trans_dim_groups(
        trans_coordsys, ret_var->name, &trans_dim_groups);

    if (trans_dim_ngroups < 0) {
        return(-1);
    }

    /* Loop over the dimensions in the retrieved variable */

    for (dim_index = 0; dim_index < ret_var->ndims; dim_index++) {

        ret_dim = ret_var->dims[dim_index];

        /* Check if there is a trans_dim_group for this dimension. */

        group_dims  = (char **)NULL;
        group_ndims = 0;

        if (trans_dim_groups) {

            for (tdgi = 0; trans_dim_groups[tdgi].in_dim; ++tdgi) {

                if (strcmp(trans_dim_groups[tdgi].in_dim, ret_dim->name) == 0) {
                    group_dims  = trans_dim_groups[tdgi].out_dims;
                    group_ndims = trans_dim_groups[tdgi].out_ndims;
                    break;
                }
            }
        }

        if (group_ndims) {

            /* A trans_dim_group was specified for this dimension so treat
             * the output coordinate variable independently of the input. */

            ret_coord_var = (CDSVar *)NULL;
            do_transform  = 1;
        }
        else {

            group_dims  = &(ret_dim->name);
            group_ndims = 1;

            /* This is a "normal" transformation where we expect the
             * dimensionality of the input and output variables to be the
             * same. */

            /* Check if a coordinate variable exists in the retrieved data. */

            ret_coord_var = cds_get_coord_var(ret_var, dim_index);

            if (!ret_coord_var) {

                trans_dim = cds_get_dim(trans_coordsys, ret_dim->name);

                if (trans_dim) {

                    trans_coord_var = cds_get_var(
                        trans_coordsys, trans_dim->name);

                    /* We have a problem if a trans_coord_var exists,
                     * or the dimension lengths do not match. */

                    if (trans_coord_var) {

                        ERROR( DSPROC_LIB_NAME,
                            "Could not transform variable %s->%s into coordinate system: %s\n"
                            " -> coordinate variable not found for dimension: %s\n",
                            ret_ds_group->name, ret_var->name,
                            trans_coordsys->name, ret_dim->name);

                        dsproc_set_status(DSPROC_ETRANSFORM);
                        return(-1);
                    }
                    else if (trans_dim->length != ret_dim->length) {

                        ERROR( DSPROC_LIB_NAME,
                            "Could not transform variable %s->%s into coordinate system: %s\n"
                            " -> dimension lengths do not match for dimension: %d\n",
                            ret_ds_group->name, ret_var->name,
                            trans_coordsys->name, ret_dim->name);

                        dsproc_set_status(DSPROC_ETRANSFORM);
                        return(-1);
                    }
                }
                else {

                    /* Just copy the dimension over and hope for the best... */

                    if (!cds_define_dim(
                        trans_coordsys,
                        ret_dim->name,
                        ret_dim->length,
                        ret_dim->is_unlimited)) {

                        dsproc_set_status(DSPROC_ENOMEM);
                        return(-1);
                    }
                }

                trans_var_dims[trans_var_ndims++] = ret_dim->name;
                continue;

            } /* end if ret_coord_var does not exist */

        } /* end if (!group_ndims) */

        /* Create the coordinate system dimension(s)
         * if they do not already exist.  */

        for (tdi = 0; tdi < group_ndims; ++tdi) {

            trans_dim_name = group_dims[tdi];
            trans_dim      = cds_get_dim(trans_coordsys, trans_dim_name);

            trans_var_dims[trans_var_ndims++] = trans_dim_name;

            if (trans_dim) {
                trans_coord_var = cds_get_var(trans_coordsys, trans_dim->name);
            }
            else {
                trans_coord_var = (CDSVar *)NULL;
            }

            /* Create the trans_coord_var if it does not already exist. */

            if (!trans_coord_var) {

                if (trans_dim) {

                    /* If the dimension already exists this means a variable
                     * using this dimension has already been added to the
                     * coordinate system but it didn't have a coordinate
                     * variable. */

                    ERROR( DSPROC_LIB_NAME,
                        "Could not transform variable %s->%s into coordinate system: %s\n"
                        " -> a variable with dimension '%s' has already been added to the\n"
                        " -> coordinate system but it did not have an associated coordinate variable\n",
                        ret_ds_group->name, ret_var->name,
                        trans_coordsys->name, ret_dim->name);

                    dsproc_set_status(DSPROC_ETRANSFORM);
                    return(-1);
                }

                status = _dsproc_create_trans_coordsys_dimension(
                    coordsys_name,
                    ret_var,
                    trans_dim_name,
                    ret_coord_var,
                    trans_coordsys,
                    &trans_coord_var);

                if (status < 0) {
                    return(-1);
                }

                if (status == 0) {

/* BDE:
 * Need update in retriever logic to read in mapped coordinate
 * variables that would not be read in otherwise. */

                    if (ret_var_tag->required) {

                        ERROR( DSPROC_LIB_NAME,
                            "Could not transform variable %s->%s into coordinate system: %s\n"
                            " -> the mapped coordinate variable for %s was not found in the retrieved data\n",
                            ret_ds_group->name, ret_var->name,
                            trans_coordsys->name, ret_dim->name);

                        dsproc_set_status(DSPROC_ETRANSFORM);
                        return(-1);
                    }
                    else {

                        WARNING( DSPROC_LIB_NAME,
                            "Could not transform optional variable %s->%s into coordinate system: %s\n"
                            " -> the mapped coordinate variable for %s was not found in the retrieved data\n",
                            ret_ds_group->name, ret_var->name,
                            trans_coordsys->name, ret_dim->name);

                        _dsproc_free_trans_dim_groups(trans_dim_groups);
                        return(0);
                    }
                }

            } /* end if !trans_coord_var */

            if (ret_coord_var) {

                /* Make sure the ret_coord_var has the same units as the
                 * trans_coord_var.
                 *
                 * This is needed if variables from the same datastream using
                 * the same coordinate variable(s) are transformed into
                 * different coordinate systems that have different units for
                 * the coordinate variables.
                 */

                ret_units   = cds_get_var_units(ret_coord_var);
                trans_units = cds_get_var_units(trans_coord_var);

                status = cds_compare_units(ret_units, trans_units);

                if (status < 0) {

                    ERROR( DSPROC_LIB_NAME,
                        "Could not transform variable %s->%s into coordinate system: %s\n"
                        " -> coordinate variable units comparison failed for dimension: %s\n",
                        ret_ds_group->name, ret_var->name,
                        trans_coordsys->name, ret_dim->name);

                    dsproc_set_status(DSPROC_ETRANSFORM);
                    return(-1);
                }

                if (status > 0) {

                    WARNING( DSPROC_LIB_NAME,
                        "Converting ret_coord_var units to match trans_coord_var units\n"
                        " - ret_coord_var units:   %s->%s '%s'\n"
                        " - trans_coord_var units: %s->%s '%s'\n",
                        ret_ds_group->name, ret_coord_var->name, ret_units,
                        trans_coordsys->name, trans_coord_var->name, trans_units);

                    if (!cds_change_var_units(
                        ret_coord_var, trans_coord_var->type, trans_units)) {

                        ERROR( DSPROC_LIB_NAME,
                            "Could not transform variable %s->%s into coordinate system: %s\n"
                            " -> coordinate variable units conversion failed for dimension: %s\n",
                            ret_ds_group->name, ret_var->name,
                            trans_coordsys->name, ret_dim->name);

                        dsproc_set_status(DSPROC_ETRANSFORM);
                        return(-1);
                    }
                }
            }

            /* Check if a transformation was specified for this
             * dimension in the transformation parameters file. */

            param_length = 256;

            cds_get_transform_param_from_group(
                trans_coordsys, trans_coord_var->name,
                "transform", CDS_CHAR, &param_length, param_value);

            if (param_length > 0) {
                if (strcmp(param_value, "TRANS_CARACENA") == 0) {
                    is_caracena = 1;
                }
                do_transform = 1;
            }

            param_length = 256;
            snprintf(param_name, 256, "%s:transform", trans_coord_var->name);

            cds_get_transform_param_from_group(
                trans_coordsys, ret_var->name,
                param_name, CDS_CHAR, &param_length, param_value);

            if (param_length > 0) {
                if (strcmp(param_value, "TRANS_CARACENA") == 0) {
                    is_caracena = 1;
                }
                do_transform = 1;
            }

            /* If we have already determined that a transformation
             * will be needed there is no need to continue checking. */

            if (do_transform) {
                continue;
            }

            /* Compare the coordinate variable in the retrieved data to the
             * coordinate variable in the transformation coordinate system to
             * determine if a transformation is required for this dimension. */

            if (ret_coord_var) {

                ret_dim   = ret_coord_var->dims[0];
                trans_dim = trans_coord_var->dims[0];

                if (ret_dim->length != trans_dim->length) {
                    do_transform += 1;
                    continue;
                }

                status = cds_compare_arrays(
                    ret_dim->length,
                    ret_coord_var->type,
                    ret_coord_var->data.vp,
                    trans_coord_var->type,
                    trans_coord_var->data.vp,
                    NULL, NULL);

                if (status != 0) {
                    do_transform = 1;
                }
            }

        } /* end loop over dimensions in trans_dim_grouping */

    } /* end loop over variable dimensions */

    /* Check if a transformation was specified for this
     * variable in the transformation parameters file. */

    if (!do_transform) {

        param_length = 256;

        cds_get_transform_param_from_group(
            trans_coordsys, ret_var->name,
            "transform", CDS_CHAR, &param_length, param_value);

        if (param_length > 0) {
            do_transform = 1;
        }
    }

// This is probably not the correct approach because the user may want to
// pull through the primary datastream as is, and map the secondary datastreams
// to it. In this case the user may want the original QC to be preserved.
//
// If the user wants the transform to attempt to fill in missing values via
// interpolation or averaging they should explicitly specify this.
//
//    /* We *always* want to run the data through the transform logic if it was
//     * explicitly mapped to a transformation coordinate system.  This is
//     * because the transformation logic will attempt to fill in missing values
//     * and create the expected QC field in the output.
//     *
//     * I left the existing "do_transform" logic in the code in the off chance
//     * it might be useful later... */
//
//    if (ret_var->ndims &&
//        ret_var_tag->coordsys_name) {
//
//        do_transform = 1;
//    }

    if (do_transform) {

        DEBUG_LV1( DSPROC_LIB_NAME,
            "%s->%s\n"
            " - transforming variable data\n",
            ret_ds_group->name, ret_var->name);

        /* Create the variable in the transformation group. */

        if (is_caracena) {
            *trans_var = _dsproc_create_trans_var(
                ret_var, trans_obs_group,
                NULL,
                trans_var_ndims,
                trans_var_dims,
                CDS_NAT);
        }
        else {
            *trans_var = _dsproc_copy_ret_var_to_trans_group(
                ret_var, 0, NULL, NULL,
                trans_obs_group, CDS_NAT, NULL, 0, 0);
        }

        if (!(*trans_var)) {
            return(-1);
        }

        /* Create the QC variable in the transformation group. */

        trans_qc_var = _dsproc_create_trans_qc_var(
            trans_obs_group, *trans_var, is_caracena);

        if (!trans_qc_var) {
            return(-1);
        }

        /* Check for a retrieved QC variable. */

        snprintf(ret_qc_var_name, NC_MAX_NAME, "qc_%s", ret_var->name);
        ret_qc_var = cds_get_var(ret_obs_group, ret_qc_var_name);

        /* Call the transform. */

        status = cds_transform_driver(
            ret_var, ret_qc_var, *trans_var, trans_qc_var);

        if (status < 0) {

            ERROR( DSPROC_LIB_NAME,
                "Could not transform variable %s->%s into coordinate system: %s\n"
                " -> call to cds_transform_driver failed\n",
                ret_ds_group->name, ret_var->name, trans_coordsys->name);

            dsproc_set_status(DSPROC_ETRANSFORM);
            return(-1);
        }

    }
    else {

        /* No transformation needed so just copy the variable and
         * and it's QC variable to the transformation group. */

        DEBUG_LV1( DSPROC_LIB_NAME,
            "%s->%s\n"
            " - no transformation needed: copying variable data to transformation group\n",
            ret_ds_group->name, ret_var->name);

        *trans_var = _dsproc_copy_ret_var_to_trans_group(
            ret_var, 0, NULL, NULL,
            trans_obs_group, CDS_NAT, NULL, 1, 1);

        if (!*trans_var) {
            return(-1);
        }
    }

    _dsproc_free_trans_dim_groups(trans_dim_groups);

    return(1);
}

/*******************************************************************************
 *  Private Functions Visible Only To This Library
 */

/**
 *  Private: Create a consolidated transformation QC variable in a CDSGroup.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  trans_qc_var     pointer to the transformation QC variable
 *  @param  out_group        pointer to the output dataset
 *  @param  out_qc_var_name  name of the variable to create in the output dataset
 *
 *  @return
 *    - pointer to the QC variable
 *    - NULL if an error occurred
 */
CDSVar *_dsproc_create_consolidated_trans_qc_var(
    CDSVar     *trans_qc_var,
    CDSGroup   *out_group,
    const char *out_qc_var_name)
{
    const char *dim_names[NC_MAX_DIMS];
    CDSVar     *out_qc_var;
    CDSAtt     *long_name_att;
    int         ai, di;

    /* Create the dimension names list */

    for (di = 0; di < trans_qc_var->ndims; di++) {
        dim_names[di] = trans_qc_var->dims[di]->name;
    }

    /* Create the QC variable */

    out_qc_var = cds_define_var(out_group,
        out_qc_var_name, CDS_INT, trans_qc_var->ndims, dim_names);

    if (!out_qc_var) {
        goto FATAL_ERROR;
    }

    /* Define the long_name attributes */

    long_name_att = cds_get_att(trans_qc_var, "long_name");
    if (long_name_att && long_name_att->type == CDS_CHAR) {

        if (!cds_define_att_text(out_qc_var,
            "long_name",
            "%s", long_name_att->value.cp)) {

            goto FATAL_ERROR;
        }
    }

    /* Define the QC attributes */

    for (ai = 0; gConsTransQCAtts[ai].name; ++ai) {

        if (!cds_define_att_text(out_qc_var,
            gConsTransQCAtts[ai].name,
            gConsTransQCAtts[ai].value)) {

            goto FATAL_ERROR;
        }
    }

    return(out_qc_var);

FATAL_ERROR:

    ERROR( DSPROC_LIB_NAME,
        "Could not create consolidated transformation QC variable: %s->%s\n",
        out_group->name, out_qc_var_name);

    dsproc_set_status(DSPROC_ETRANSQCVAR);
    return((CDSVar *)NULL);
}

/**
 *  Free memory used by the bit decsriptions set by the user.
 */
void _dsproc_free_trans_qc_rollup_bit_descriptions(void)
{
    if (gTransQCRollupBadDesc) {
        free((void *)gTransQCRollupBadDesc);
        gTransQCRollupBadDesc = (const char *)NULL;
    }

    if (gTransQCRollupIndDesc) {
        free((void *)gTransQCRollupIndDesc);
        gTransQCRollupIndDesc = (const char *)NULL;
    }
}

/** @publicsection */

/*******************************************************************************
 *  Internal Functions Visible To The Public
 */

/**
 *  Internal: Get information about a dimension in the target coordinate system.
 *
 *  The memory used by the information returned in the members of the
 *  TransDimInfo structure must not be freed or altered in any way by the
 *  calling process.
 *
 *  Any information that was not provided in the retriever definition will
 *  have values set to NULL or 0 depending on their datatype.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  ret_var   - pointer to the retrieved variable
 *  @param  dim_index - index of the dimension
 *  @param  dim_info  - pointer to the TransDimInfo structure to store
 *                      the information in
 *
 *  @return
 *    -  1 if information was found for this dimension
 *    -  0 if this will be an implicite mapping to either itself, or the first
 *         dimension of the same name added to the transformed dataset.
 *    - -1 if a fatal error occurs
 */
int dsproc_get_trans_dim_info(
    CDSVar       *ret_var,
    int           dim_index,
    TransDimInfo *dim_info)
{
    VarTag         *tag      = cds_get_user_data(ret_var, "DSProcVarTag");
    RetCoordSystem *coordsys = (RetCoordSystem *)NULL;
    RetCoordDim    *coorddim = (RetCoordDim *)NULL;
    CDSDim         *dim;
    int             status;
    int             cdi;

    /* Initialize all return values to NULL */

    memset(dim_info, 0, sizeof(TransDimInfo));

    /* Get the coordinate system definition */

    if (tag && tag->coordsys_name) {
        coordsys = _dsproc_get_ret_coordsys(tag->coordsys_name);
    }

    if (!coordsys) return(0);

    /* Check for this coordinate system dimension */

    dim = ret_var->dims[dim_index];

    for (cdi = 0; cdi < coordsys->ndims; ++cdi) {
        if (strcmp(coordsys->dims[cdi]->name, dim->name) == 0) {
            coorddim = coordsys->dims[cdi];
            break;
        }
    }

    if (!coorddim) return(0);

    /* Check if we are mapping this dimension to a retrieved variable */

    if (coorddim->nvarmaps) {

        status = _dsproc_get_mapped_ret_coord_var(
            coorddim, &(dim_info->ret_dsid), &(dim_info->ret_coord_var));

        if (status < 0) return(-1);
    }

    /* Set all other coordinate dimension information */

    dim_info->name        = coorddim->name;
    dim_info->data_type   = coorddim->data_type;
    dim_info->units       = coorddim->units;
    dim_info->trans_type  = coorddim->trans_type;

    if (coorddim->start)    dim_info->start    = atof(coorddim->start);
    if (coorddim->length)   dim_info->length   = atof(coorddim->length);
    if (coorddim->interval) dim_info->interval = atof(coorddim->interval);

    if (coorddim->trans_range)
        dim_info->trans_range = atof(coorddim->trans_range);

    if (coorddim->trans_align)
        dim_info->trans_align = atof(coorddim->trans_align);

    return(1);
}

/**
 *  Internal: Get the output bits to use when consolidating transformation QC bits.
 *
 *  This function will use the bit description attributes to determine the
 *  correct bits for bad and indeterminate when consolidating transformation
 *  QC variables. It will first check for field level bit description
 *  attributes, and then for the global attributes if they are not found.
 *
 *  @param  qc_var     pointer to the QC variable
 *  @param  bad_flag   output: QC flag to use for bad values
 *  @param  ind_flag   output: QC flag to use for indeterminate values
 *
 *  @retval  1  if the bad and ind flags could be determined
 *  @retval  0  if the required bit description attributes were not found.
 */
int dsproc_get_trans_qc_rollup_bits(
    CDSVar       *qc_var,
    unsigned int *bad_flag,
    unsigned int *ind_flag)
{
    CDSGroup    *dataset;
    int          nfound;
    int          status;

    status = _dsproc_get_trans_qc_rollup_bits(
        "bit_", qc_var->natts, qc_var->atts,
        bad_flag, ind_flag,
        &nfound,  NULL);

    if (!nfound) {

        dataset = (CDSGroup *)qc_var->parent;

        status = _dsproc_get_trans_qc_rollup_bits(
            "qc_bit_", dataset->natts, dataset->atts,
            bad_flag, ind_flag,
            &nfound,  NULL);
    }

    return(status);
}

/**
 *  Internal: Check if a QC variable is from a transformation process.
 *
 *  @param  qc_var  pointer to the QC variable
 *
 *  @retval  1  if this QC variable from a transformation process
 *  @retval  0  if this QC variable is not from a transformation process
 */
int dsproc_is_transform_qc_var(CDSVar *qc_var)
{
    CDSAtt     *att;
    const char *name;
    const char *value;
    size_t      length;
    int         ai;

    for (ai = 3; gTransQCAtts[ai].name; ++ai) {

        name = gTransQCAtts[ai].name;
        if (strstr(name, "description") != 0) continue;

        /* Bits 10 and higher were added later so we only check
        *  the first 9 to maintain backward compatibility. */

        if (strcmp(name, "bit_10_description") == 0) break;

        att = cds_get_att(qc_var, name);
        if (!att || att->type != CDS_CHAR) return(0);

        value  = gTransQCAtts[ai].value;
        length = strchr(value, ':') - value + 1;

        if (strncmp(att->value.cp, value, length) != 0) return(0);
    }

    return(1);
}

/**
 *  Deprecated: Find a transformation datastream group.
 *
 *  This function will find the first datastream group in the specified
 *  coordinate system that has variable that came from the datastream
 *  group specified in the retriever.
 *
 *  @param coordsys_name  - name of the coordinate system
 *  @param ret_group_name - name of the datastream group as specified
 *                          in the retriever definition.
 *
 *  @return
 *    -  pointer to the transformation datastream CDSGroup.
 *    -  NULL if not found
 */
CDSGroup *dsproc_get_trans_ds_by_group_name(
    const char *coordsys_name,
    const char *ret_group_name)
{
    CDSGroup *coordsys;
    CDSGroup *ds_group;
    CDSVar   *var;
    VarTag   *var_tag;
    int       gi, vi;

    coordsys = cds_get_group(_DSProc->trans_data, coordsys_name);

    if (!coordsys) {
        return((CDSGroup *)NULL);
    }

    for (gi = 0; gi < coordsys->ngroups; gi++) {

        ds_group = coordsys->groups[gi];

        for (vi = 0; vi < ds_group->nvars; vi++) {

            var     = ds_group->vars[vi];
            var_tag = cds_get_user_data(var, "DSProcVarTag");

            if (!var_tag ||
                !var_tag->ret_group_name) {

                continue;
            }

            if (strcmp(var_tag->ret_group_name, ret_group_name) == 0) {
                return(ds_group);
            }
        }
    }

    return((CDSGroup *)NULL);
}

/**
 *  Run the data transformation logic.
 *
 *  This function will transform the retrieved variable data to the
 *  coordinate systems specified in the retriever definition.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param trans_data - output: pointer to the trans_data CDSGroup
 *
 *  @return
 *    -  1 if successful or there was no retrieved data to transform
 *    -  0 if the transformation logic could not be run for the current
 *         processing interval.
 *    - -1 if an error occurred
 */
int dsproc_transform_data(
    CDSGroup  **trans_data)
{
    CDSGroup   *ret_data;
    CDSGroup   *ret_ds_group;
    CDSGroup   *ret_obs_group;
    CDSVar     *ret_var;
    VarTag     *ret_var_tag;
    CDSVar     *trans_var;
    int         in_dsid;
    DataStream *in_ds;
    int         status;
    int         csi, dsi, vari;

    CDSGroup   *trans_coordsys;
    CDSGroup   *trans_ds_group;

    *trans_data = (CDSGroup *)NULL;

    /* Get the retriever information */

    ret_data = _DSProc->ret_data;
    if (!ret_data) {
        return(1);
    }

    DEBUG_LV1( DSPROC_LIB_NAME,
        "---------------------------------------\n"
        "Transforming retrieved data to user defined coordinate systems\n"
        "---------------------------------------\n");

    /* Clean up any previously transformed data */

    _dsproc_cleanup_transformed_data();

    /* Define the parent CDSGroup used to store the transformed data */

    _DSProc->trans_data = cds_define_group(NULL, "transformed_data");
    if (!_DSProc->trans_data) {
        dsproc_set_status(DSPROC_ENOMEM);
        return(-1);
    }

    *trans_data = _DSProc->trans_data;

    /* Loop over each datastream group in the retrieved data  */

    for (dsi = 0; dsi < ret_data->ngroups; dsi++) {

        ret_ds_group = ret_data->groups[dsi];

        /* Check if we should skip this datastream */

        in_dsid = _dsproc_get_ret_group_ds_id(ret_ds_group);
        if (in_dsid >= 0) {
            in_ds = _DSProc->datastreams[in_dsid];
            if (in_ds->flags & DS_SKIP_TRANSFORM) {
                continue;
            }
        }

/* BDE OBS UPDATE: For now we are only supporting the cases where
 * all observations in the retrieved data could be merged */

if (ret_ds_group->ngroups > 1) {

    WARNING( DSPROC_LIB_NAME,
        "Found multiple observations in the retrieved data for: %s\n"
        " -> currently the transform logic only handles observations that can be merged\n"
        " -> an update is being worked on to support this in the future\n"
        " -> skipping current processing interval and continuing\n",
        ret_ds_group->name);

    return(0);
}

if (ret_ds_group->ngroups < 1) {
    continue;
}
ret_obs_group = ret_ds_group->groups[0];

        /* Loop over each observation for each datastream */
/*
        for (obsi = 0; obsi < ret_ds_group->ngroups; obsi++) {

            ret_obs_group = ret_ds_group->groups[obsi];
*/
/* END BDE OBS UPDATE */

            if (!_dsproc_set_ret_obs_params(in_dsid, ret_obs_group)) {
                return(-1);
            }

            /* Loop over each variable in each observation */

            for (vari = 0; vari < ret_obs_group->nvars; vari++) {

                ret_var     = ret_obs_group->vars[vari];
                ret_var_tag = cds_get_user_data(ret_var, "DSProcVarTag");

                /* Skip coordinate, boundary, and QC variables that were auto
                 * retrieved, these will be pulled through with the primary
                 * variables. */

                if (!ret_var_tag) {
                    continue;
                }

                /* Skip variables that have the VAR_SKIP_TRANSFORM flag set. */

                if (ret_var_tag->flags & VAR_SKIP_TRANSFORM) {
                    continue;
                }

                /* Skip variables defined by the user in the hook functions
                 * that do not have a coordinate system name defined. */

                if (!ret_var_tag->in_ds &&
                    !ret_var_tag->coordsys_name) {
                    continue;
                }

                /* Skip companion QC variables that were explicitly requested */

                if ((strncmp(ret_var->name, "qc_", 3) == 0) &&
                    cds_get_var(ret_obs_group, &(ret_var->name[3]))) {

                    continue;
                }

                /* Skip coordinate variables that were explicitly requested */

                if ((ret_var->ndims == 1) &&
                    (strcmp(ret_var->name, ret_var->dims[0]->name) == 0)) {

                    continue;
                }

                /* Skip boundary variables that were explicitly requested */

                if (cds_get_bounds_coord_var(ret_var)) {
                    continue;
                }

                /* Transform variable into the coordinate system
                 * specified in the retriever definition. */

                status = _dsproc_transform_variable(
                    *trans_data,
                    ret_ds_group,
                    ret_obs_group,
                    ret_var,
                    ret_var_tag,
                    &trans_var);

                if (status < 0) {
                    return(-1);
                }

                if (status == 0) {
                    continue;
                }

                status = dsproc_copy_var_tag(ret_var, trans_var);
                if (status == 0) {
                    return(-1);
                }

            } /* end var loop */

/* BDE OBS UPDATE */
/*        } end obs loop */
/* BDE END OBS UPDATE */

    } /* end ds loop */

    /* Remove empty coordinate systems. This can happen if all variables
     * in a coordinate system are optional and were not found. */

    for (csi = 0; csi < (*trans_data)->ngroups; csi++) {

        trans_coordsys = (*trans_data)->groups[csi];

        for (dsi = 0; dsi < trans_coordsys->ngroups; dsi++) {

            trans_ds_group = trans_coordsys->groups[dsi];

            if (trans_ds_group->nvars == 0) {
                cds_delete_group(trans_ds_group);
                dsi--;
            }
        }

        if (trans_coordsys->ngroups == 0) {
            cds_delete_group(trans_coordsys);
            csi--;
        }
    }

    return(1);
}

/*******************************************************************************
 *  Public Functions
 */

/**
 *  Set the decsriptions of the bits to use when consolidating transformation QC bits.
 *
 *  This function will set the bit descriptions to use to determine the
 *  correct bits for bad and indeterminate when consolidating transformation
 *  QC variables.
 *
 *  @param  bad_desc  description of bad bit
 *  @param  ind_desc  description of indeterminate bit
 *
 *  @retval  1  if successful
 *  @retval  0  if a memory allocation error occurred
 */
int dsproc_set_trans_qc_rollup_bit_descriptions(
    const char *bad_desc,
    const char *ind_desc)
{
    if (bad_desc) {
        if (!(gTransQCRollupBadDesc = strdup(bad_desc))) {
            ERROR( DSPROC_LIB_NAME,
                "Could not set descripions for transformation QC rollup bits\n"
                " -> memory allocation error\n");

            dsproc_set_status(DSPROC_ENOMEM);
            return(0);
        }
    }

    if (ind_desc) {
        if (!(gTransQCRollupIndDesc = strdup(ind_desc))) {
            ERROR( DSPROC_LIB_NAME,
                "Could not set descripions for transformation QC rollup bits\n"
                " -> memory allocation error\n");

            dsproc_set_status(DSPROC_ENOMEM);
            return(0);
        }
    }

    return(1);
}
