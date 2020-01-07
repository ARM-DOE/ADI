/*******************************************************************************
*
*  COPYRIGHT (C) 2014 Battelle Memorial Institute.  All Rights Reserved.
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
*    $Revision: 57300 $
*    $Author: ermold $
*    $Date: 2014-10-06 20:03:42 +0000 (Mon, 06 Oct 2014) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file dsproc_qc_utils.c
 *  QC Utilities.
 */

#include "dsproc3.h"
#include "dsproc_private.h"

extern DSProc *_DSProc; /**< Internal DSProc structure */

/** @privatesection */

/*******************************************************************************
 *  Static Data and Functions Visible Only To This Module
 */

/**
 *  Get QC bit descriptions.
 *
 *  Search a list of CDSAtts and return the QC bit descriptions.
 *
 *  The index of the attribute description will correspond the the power of two
 *  of the QC bit flag (i.e. index 0 = 2^0 = 1, index 1 = 2^1 = 2, etc...).
 *
 *  @param  prefix  attribute name prefix before the bit number
 *  @param  natts   number of attributes in the list
 *  @param  atts    list of attributes
 *  @param  descs   list to store the pointers to the bit descriptions,
 *                  this must me large enough to stores all descriptions.
 *
 *  @retval  nfound  number of bit descriptions found
 */
static int _dsproc_get_qc_bit_descriptions(
    const char   *prefix,
    int           natts,
    CDSAtt      **atts,
    const char  **descs)
{
    CDSAtt *att;
    size_t  prefix_length;
    size_t  name_length;
    int     bit_num;
    int     nfound;
    int     ai;

    prefix_length = strlen(prefix);
    nfound = 0;

    for (ai = 0; ai < natts; ++ai) {

        att = atts[ai];

        /* Check if this is a QC attribute */

        if ((att->type != CDS_CHAR) ||
            (strncmp(att->name, prefix, prefix_length) != 0)) {

            continue;
        }

        /* Check if this is a QC description attribute */

        name_length = strlen(att->name);
        if (name_length < prefix_length + 13 ||
            name_length > prefix_length + 14 ||
            strcmp( (att->name + name_length - 11), "description") != 0) {

            continue;
        }

        /* Get the bit number */

        bit_num = atoi(att->name + prefix_length);

        /* Add the QC description to the list */

        if (bit_num > 0) {

            descs[bit_num-1] = att->value.cp;

            if (nfound < bit_num) {
                nfound = bit_num;
            }
        }
    }

    return(nfound);
}

/**
 *  Get the QC mask for a specific QC assessment.
 *
 *  @param  assessment  - the QC assessment to get the mask for
 *                        (i.e. "Bad" or "Indeterminate")
 *  @param  prefix      - attribute name prefix before the bit number
 *  @param  natts       - number of attributes in the list
 *  @param  atts        - list of attributes
 *  @param  nfound      - output: total number of qc assessment attributes found
 *  @param  max_bit_num - output: highest QC bit number used
 *
 *  @retval  qc_mask  The QC mask for the specified QC assessment.
 */
static unsigned int _dsproc_get_qc_assessment_mask(
    const char  *assessment,
    const char  *prefix,
    int          natts,
    CDSAtt     **atts,
    int         *nfound,
    int         *max_bit_num)
{
    CDSAtt      *att;
    size_t       prefix_length;
    size_t       name_length;
    unsigned int qc_mask;
    int          bit_num;
    int          ai;

    if (nfound)      *nfound      = 0;
    if (max_bit_num) *max_bit_num = 0;

    qc_mask       = 0;
    prefix_length = strlen(prefix);

    for (ai = 0; ai < natts; ++ai) {

        att = atts[ai];

        /* Check if this is a QC assessment attribute */

        if ((att->type != CDS_CHAR) ||
            (strncmp(att->name, prefix, prefix_length) != 0)) {

            continue;
        }

        name_length = strlen(att->name);
        if (name_length < prefix_length + 12 ||
            strcmp( (att->name + name_length - 10), "assessment") != 0) {

            continue;
        }

        bit_num = atoi(att->name + prefix_length);
        if (!bit_num) continue;

        /* Keep track of the number of assessment attributes found,
         * and the highest bit number used if requested */

        if (nfound) *nfound += 1;

        if (max_bit_num &&
            *max_bit_num < bit_num) {

            *max_bit_num = bit_num;
        }

        /* Update the qc_mask if this is the assessment we are looking for */

        if (strcmp(att->value.cp, assessment) != 0) {
            continue;
        }

        qc_mask |= 1 << (bit_num-1);
    }

    return(qc_mask);
}

/*******************************************************************************
 *  Private Functions Visible Only To This Library
 */

/**
 *  Consolidate all QC bits for a variable into bad or indeterminate.
 *
 *  This function will consolidate all the bad and indeterminate QC bits in
 *  the input QC variable into a single bad or indeterminate bit in the output
 *  QC variable. By default (reset == 0) the bit values of the output variable
 *  will be updated without affecting any of the bits that may have already been
 *  set. The output QC values can be reset to zero before setting the bad or
 *  indeterminate bit by specifying 1 for the reset parameter.
 *
 *  The bad_mask can be determined using the bit assessment attributes by
 *  passing in 0 for it's value (see dsproc_get_bad_qc_mask()).
 *
 *  The bad and indeteminate flag values are *not* the bit numbers. They are
 *  the actual numeric values of the bits. For example:
 *
 *    - bit_1 = pow(2,0) = (1 << 0) = 0x01 =   1
 *    - bit_2 = pow(2,1) = (1 << 1) = 0x02 =   2
 *    - bit_3 = pow(2,2) = (1 << 2) = 0x04 =   4
 *    - bit_4 = pow(2,3) = (1 << 3) = 0x08 =   8
 *    - bit_5 = pow(2,4) = (1 << 4) = 0x10 =  16
 *    - bit_6 = pow(2,5) = (1 << 5) = 0x20 =  32
 *    - bit_7 = pow(2,6) = (1 << 6) = 0x40 =  64
 *    - bit_8 = pow(2,7) = (1 << 7) = 0x80 = 128
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  in_qc_var        - pointer to the input QC variable
 *  @param  in_sample_start  - index of the start sample in the input variable.
 *  @param  sample_count     - number of samples or 0 to for all samples
 *  @param  bad_mask         - mask used to determine bad QC values in the input
 *                             QC, or 0 to indicate that the bit assessment
 *                             attributes should be used to determine it.
 *  @param  out_qc_var       - pointer to the output QC variable
 *  @param  out_sample_start - index of the start sample in the output variable
 *  @param  bad_flag         - QC flag to use for bad values
 *  @param  ind_flag         - QC flag to use for indeterminate values
 *  @param  reset            - specifies if the output QC values should be reset
 *                             to zero (0 = false, 1 = true)
 *
 *  @retval  1 if successful
 *  @retval  0 if a fatal error occurred
 */
int _dsproc_consolidate_var_qc(
    CDSVar       *in_qc_var,
    size_t        in_sample_start,
    size_t        sample_count,
    unsigned int  bad_mask,
    CDSVar       *out_qc_var,
    size_t        out_sample_start,
    unsigned int  bad_flag,
    unsigned int  ind_flag,
    int           reset)
{
    size_t  sample_size;
    int     count;
    int    *inp;
    int    *outp;

    size_t  out_nsamples;
    size_t  init_count;

    if (!sample_count) {
        sample_count = in_qc_var->sample_count - in_sample_start;
    }

    if (!bad_mask) bad_mask = dsproc_get_bad_qc_mask(in_qc_var);

    /* Make sure the QC variables have interger data types */

    if (in_qc_var->type != CDS_INT) {

        ERROR( DSPROC_LIB_NAME,
            "Could not consolidate QC bits\n"
            " -> invalid data type for input QC variable: %s\n",
            cds_get_object_path(in_qc_var));

        dsproc_set_status(DSPROC_EQCVARTYPE);
        return(0);
    }

    if (out_qc_var->type != CDS_INT) {

        ERROR( DSPROC_LIB_NAME,
            "Could not consolidate QC bits\n"
            " -> invalid data type for output QC variable: %s\n",
            cds_get_object_path(out_qc_var));

        dsproc_set_status(DSPROC_EQCVARTYPE);
        return(0);
    }

    /* Allocate memory for the output variable if necessary */

    sample_size = dsproc_var_sample_size(in_qc_var);

    if (!sample_count) {
        sample_count = in_qc_var->sample_count;
    }

    if (out_qc_var != in_qc_var) {

        /* Make sure the output QC variable has the correct shape */

        if (dsproc_var_sample_size(out_qc_var) != sample_size) {

            ERROR( DSPROC_LIB_NAME,
                "Could not consolidate QC bits\n"
                " -> QC variables do not have the same shape\n"
                " -> input:  %s\n"
                " -> output: %s\n",
                cds_get_object_path(in_qc_var),
                cds_get_object_path(out_qc_var));

            dsproc_set_status(DSPROC_EQCVARDIMS);
            return(0);
        }

        /* Allocate memory for the output variable's data values
         * if necessary. */

        out_nsamples = out_sample_start + sample_count;

        if (out_qc_var->sample_count < out_nsamples) {

            init_count = out_nsamples - out_qc_var->sample_count;

            if (init_count > 0) {
                if (!dsproc_init_var_data(out_qc_var,
                    out_qc_var->sample_count, init_count, 0)) {
                    return(0);
                }
            }
        }
    }

    /* Map all QC bits to bad or indeterminate */

    count = (int)(sample_count * sample_size) + 1;
    inp   = in_qc_var->data.ip + in_sample_start;
    outp  = out_qc_var->data.ip + out_sample_start;

    if (reset) {
        for ( ; --count; ++inp, ++outp) {
            if (*inp) *outp = (*inp & bad_mask) ? bad_flag : ind_flag;
            else      *outp = 0;
        }
    }
    else {
        for ( ; --count; ++inp, ++outp) {
            if (*inp) *outp |= (*inp & bad_mask) ? bad_flag : ind_flag;
        }
    }

    return(1);
}

/** @publicsection */

/*******************************************************************************
 *  Internal Functions Visible To The Public
 */

/**
 *  Search for a bit description in a list of bit descriptions.
 *
 *  Find the first occurance of a string in the descs list in the bit_descs
 *  list. The descs string must match at the beginning of the bit_desc
 *  string.
 *
 *  Note: Use dsproc_get_qc_bit_descriptions() to get the list of bit
 *  descriptions for a QC variable.
 * 
 *  @param  ndescs     - number of dscriptions to look for
 *  @param  descs      - list of dscriptions to look for
 *  @param  bit_ndescs - number of dscriptions to search in
 *  @param  bit_descs  - list of dscriptions to search in
 *
 *  @return
 *    - index of the mathcing string from the bit_descs list
 *    - -1 if no match was found
 */
int dsproc_find_bit_description(
    int          ndescs,
    const char **descs,
    int          bit_ndescs,
    const char **bit_descs)
{
    int i1, i2;
    size_t len[ndescs];

    for (i1 = 0; i1 < ndescs; ++i1) {
        len[i1] = strlen(descs[i1]);
    }

    for (i2 = 0; i2 < bit_ndescs; ++i2) {
        for (i1 = 0; i1 < ndescs; ++i1) {
            if (strncmp(descs[i1], bit_descs[i2], len[i1]) == 0) {
                return(i2);
            }
        }
    }
    
    return(-1);
}

/**
 *  Get a data attribute.
 * 
 *  Get a variable attribute that must have the same data type as the
 *  variable.  If the attribute does not have the same data type as the
 *  variable an error will be genereated.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  var      - pointer to the variable
 *  @param  att_name - the attribute name
 *  @param  attp     - output: pointer to the attribute
 *
 *  @return
 *    -  1 if found
 *    -  0 if not found
 *    - -1 if an error occurred
 */
int dsproc_get_data_att(
    CDSVar      *var,
    const char  *att_name,
    CDSAtt     **attp)
{
    CDSAtt *att;

    *attp = (CDSAtt *)NULL;

    att = cds_get_att(var, att_name);

    if (att && att->length && att->value.vp) {

        if (att->type != var->type) {

            ERROR( DSPROC_LIB_NAME,
                "Attribute type does not match variable type:\n"
                " - variable  %s has type: %s\n"
                " - attribute %s has type: %s\n",
                cds_get_object_path(var),
                cds_data_type_name(var->type),
                cds_get_object_path(att),
                cds_data_type_name(att->type));

            dsproc_set_status(DSPROC_EDATAATTTYPE);
            return(-1);
        }

        *attp = att;

        return(1);
    }

    return(0);
}

/**
 *  Get a data attribute from a QC variable.
 *
 *  Get a QC variable attribute that must have the same data type as the
 *  data variable.  If the attribute does not have the same data type as
 *  the data variable an error will be genereated.
 * 
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  var      - pointer to the data variable
 *  @param  qc_var   - pointer to the QC variable
 *  @param  att_name - the attribute name
 *  @param  attp     - output: pointer to the attribute
 *
 *  @return
 *    -  1 if found
 *    -  0 if not found
 *    - -1 if an error occurred
 */
int dsproc_get_qc_data_att(
    CDSVar      *var,
    CDSVar      *qc_var,
    const char  *att_name,
    CDSAtt     **attp)
{
    CDSAtt *att;

    *attp = (CDSAtt *)NULL;

    att = cds_get_att(qc_var, att_name);

    if (att && att->length && att->value.vp) {

        if (att->type != var->type) {

            ERROR( DSPROC_LIB_NAME,
                "Attribute type does not match data variable type:\n"
                " - variable  %s has type: %s\n"
                " - attribute %s has type: %s\n",
                cds_get_object_path(var),
                cds_data_type_name(var->type),
                cds_get_object_path(att),
                cds_data_type_name(att->type));

            dsproc_set_status(DSPROC_EDATAATTTYPE);
            return(-1);
        }

        *attp = att;

        return(1);
    }

    return(0);
}

/**
 *  Get the QC mask for a specific QC assessment.
 *
 *  This function will use the bit assessment attributes to create a mask
 *  with all bits set for the specified assessment value.  It will first
 *  check for field level bit assessment attributes, and then for the global
 *  attributes if they are not found.
 *
 *  @param  qc_var      - pointer to the QC variable
 *  @param  assessment  - the QC assessment to get the mask for
 *                        (i.e. "Bad" or "Indeterminate")
 *  @param  nfound      - output: total number of qc assessment attributes found
 *  @param  max_bit_num - output: highest QC bit number used
 *
 *  @retval  qc_mask  The QC mask for the specified QC assessment.
 */
unsigned int dsproc_get_qc_assessment_mask(
    CDSVar     *qc_var,
    const char *assessment,
    int        *nfound,
    int        *max_bit_num)
{
    unsigned int qc_mask;
    CDSGroup    *dataset;
    int          _nfound;

    if (!nfound) nfound = &_nfound;

    qc_mask = _dsproc_get_qc_assessment_mask(
        assessment,
        "bit_",
        qc_var->natts,
        qc_var->atts,
        nfound,
        max_bit_num);

    if (!nfound) {

        dataset = (CDSGroup *)qc_var->parent;

        qc_mask = _dsproc_get_qc_assessment_mask(
            assessment,
            "qc_bit_",
            dataset->natts,
            dataset->atts,
            nfound,
            max_bit_num);
    }

    return(qc_mask);
}

/**
 *  Get QC bit descriptions.
 *
 *  Search QC variable attributes and return the list of all QC bit
 *  descriptions. If no bit_#_description variable attributes are found,
 *  the qc_bit_#_description global attributes will be used.
 *
 *  The index of the attribute description will correspond to the power of two
 *  of the QC bit value (i.e. index 0 = 2^0 = 1, index 1 = 2^1 = 2, etc...).
 *
 *  The memory used by the returned list is dynamically allocated and must be
 *  freed by the calling process, however, the memory used by the individual
 *  description strings belongs to the CDS structures and must not be altered
 *  or freed by the calling process.
 *
 *  @param  qc_var     pointer to the QC CDSVar
 *  @param  bit_descs  output: list of bit description
 *
 *  @retval  nfound  number of bit descriptions found
 *  @retval  -1      if a memory allocation error occurred
 */
int dsproc_get_qc_bit_descriptions(
    CDSVar        *qc_var,
    const char  ***bit_descs)
{
    CDSGroup *dataset;
    int       nfound;
    int       natts;

    *bit_descs = (const char **)NULL;

    /* Alocated memory of list of descriptions */

    dataset = (CDSGroup *)qc_var->parent;

    natts = (qc_var->natts > dataset->natts) ? qc_var->natts : dataset->natts;
    if (natts == 0) return(0);

    *bit_descs = (const char **)calloc(natts, sizeof(const char *));
    
    if (!bit_descs) {
        DSPROC_ERROR( DSPROC_ENOMEM, 
            "Could not allocate memory for list of bit descriptions.");
        
        return(-1);
    }

    /* Check variable attributes */

    nfound = _dsproc_get_qc_bit_descriptions(
        "bit_", qc_var->natts, qc_var->atts, *bit_descs);

    if (!nfound) {

        /* Check global attributes */

        nfound = _dsproc_get_qc_bit_descriptions(
            "qc_bit_", dataset->natts, dataset->atts, *bit_descs);
    }

    return(nfound);
}

/**
 *  Get bit flag for the missing_value check.
 *
 *  This function will search for a bit description that begins with
 *  one of the following strings:
 * 
 *    - "Value is equal to missing_value"
 *    - "Value is equal to the missing_value"
 *    - "value = missing_value"
 *    - "value == missing_value"
 *
 *    - "Value is equal to missing value"
 *    - "Value is equal to the missing value"
 *    - "value = missing value"
 *    - "value == missing value"
 *
 *    - "Value is equal to _FillValue"
 *    - "Value is equal to the _FillValue"
 *    - "value = _FillValue"
 *    - "value == _FillValue"
 *
 *  Note: Use dsproc_get_qc_bit_descriptions() to get the list of bit
 *  descriptions for a QC variable.
 *
 *  @param  bit_ndescs - number of bit dscriptions
 *  @param  bit_descs  - list of bit dscriptions
 *
 *  @return
 *    - bit flag
 *    - 0 if not found
 */
unsigned int dsproc_get_missing_value_bit_flag(
    int          bit_ndescs,
    const char **bit_descs)
{
    const char *descs[] = {
        "Value is equal to missing",
        "Value is equal to the missing",
        "value = missing",
        "value == missing",
        "Value is equal to _FillValue",
        "Value is equal to the _FillValue",
        "value = _FillValue",
        "value == _FillValue"
    };
    int ndescs = sizeof(descs)/sizeof(const char *);

    int index = dsproc_find_bit_description(ndescs, descs, bit_ndescs, bit_descs);

    unsigned int bit_flag = 0;

    if (index >= 0) {
        bit_flag = 1 << index;
    }

    return(bit_flag);
}

/**
 *  Get bit flag for a min or max check.
 *
 *  This function will search for a bit description that begins with
 *  one of the following strings:
 *
 *  If name = "warn" and type = '<':
 *  
 *    - "Value is less than warn_min"
 *    - "Value is less than the warn_min"
 *    - "value < warn_min"
 * 
 *  If name = "warn" and type = '>':
 *  
 *    - "Value is greater than warn_min"
 *    - "Value is greater than the warn_min"
 *    - "value > warn_min"
 *
 *  The bit descriptions for "fail" and "valid" are the same as above
 *  with 'warn' replaced with the specified name.
 *
 *  Note: Use dsproc_get_qc_bit_descriptions() to get the list of bit
 *  descriptions for a QC variable.
 *
 *  @param  name       - test name: "valid", "warn", or "fail"
 *  @param  type       - test type: '<' or '>'
 *  @param  bit_ndescs - number of bit dscriptions
 *  @param  bit_descs  - list of bit dscriptions
 *
 *  @return
 *    - bit flag
 *    - 0 if not found
 */
unsigned int dsproc_get_threshold_test_bit_flag(
    const char    *name,
    char           type,
    int            bit_ndescs,
    const char   **bit_descs)
{
    char         desc1[128];
    char         desc2[128];
    char         desc3[128];
    const char  *descs[] = { desc1, desc2, desc3 };
    int          ndescs = 3;
    unsigned int bit_flag = 0;
    int          index;

    if (type == '<') {
        sprintf(desc1, "Value is less than %s_min", name);
        sprintf(desc2, "Value is less than the %s_min", name);
        sprintf(desc3, "value < %s_min", name);
    }
    else {
        sprintf(desc1, "Value is greater than %s_max", name);
        sprintf(desc2, "Value is greater than the %s_max", name);
        sprintf(desc3, "value > %s_max", name);
    }

    index = dsproc_find_bit_description(ndescs, descs, bit_ndescs, bit_descs);

    if (index >= 0) {
        bit_flag = 1 << index;
    }

    return(bit_flag);
}

/*******************************************************************************
 *  Public Functions
 */

/**
 *  Consolidate all QC bits for a variable into bad or indeterminate.
 *
 *  This function will consolidate all the bad and indeterminate QC bits in
 *  the input QC variable into a single bad or indeterminate bit in the output
 *  QC variable. By default (reset == 0) the bit values of the output variable
 *  will be updated without affecting any of the bits that may have already been
 *  set. The output QC values can be reset to zero before setting the bad or
 *  indeterminate bit by specifying 1 for the reset parameter.
 *
 *  The bad_mask can be determined using the bit assessment attributes by
 *  passing in 0 for it's value (see dsproc_get_bad_qc_mask()).
 *
 *  The bad and indeteminate flag values are *not* the bit numbers. They are
 *  the actual numeric values of the bits. For example:
 *
 *    - bit_1 = pow(2,0) = (1 << 0) = 0x01 =   1
 *    - bit_2 = pow(2,1) = (1 << 1) = 0x02 =   2
 *    - bit_3 = pow(2,2) = (1 << 2) = 0x04 =   4
 *    - bit_4 = pow(2,3) = (1 << 3) = 0x08 =   8
 *    - bit_5 = pow(2,4) = (1 << 4) = 0x10 =  16
 *    - bit_6 = pow(2,5) = (1 << 5) = 0x20 =  32
 *    - bit_7 = pow(2,6) = (1 << 6) = 0x40 =  64
 *    - bit_8 = pow(2,7) = (1 << 7) = 0x80 = 128
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  in_qc_var  - pointer to the input QC variable
 *  @param  bad_mask   - mask used to determine bad QC values in the input QC,
 *                       or 0 to indicate that the bit assessment attributes
 *                       should be used to determine it.
 *  @param  out_qc_var - pointer to the output QC variable
 *  @param  bad_flag   - QC flag to use for bad values
 *  @param  ind_flag   - QC flag to use for indeterminate values
 *  @param  reset      - specifies if the output QC values should be reset
 *                       to zero (0 = false, 1 = true)
 *
 *  @retval  1 if successful
 *  @retval  0 if a fatal error occurred
 */
int dsproc_consolidate_var_qc(
    CDSVar       *in_qc_var,
    unsigned int  bad_mask,
    CDSVar       *out_qc_var,
    unsigned int  bad_flag,
    unsigned int  ind_flag,
    int           reset)
{
    int status = _dsproc_consolidate_var_qc(
        in_qc_var,  0, 0, bad_mask,
        out_qc_var, 0,    bad_flag, ind_flag,
        reset);

    return(status);
}

/**
 *  Get the QC mask used to determine bad QC values.
 *
 *  This function will use the bit assessment attributes to create a mask
 *  with all bits set for bad assessment values. It will first check for
 *  field level bit assessment attributes, and then for the global attributes
 *  if they are not found.
 *
 *  @param   qc_var   pointer to the QC variable
 *
 *  @retval  qc_mask  the QC mask with all bad bits set.
 */
unsigned int dsproc_get_bad_qc_mask(CDSVar *qc_var)
{
    return(dsproc_get_qc_assessment_mask(qc_var, "Bad", NULL, NULL));
}
