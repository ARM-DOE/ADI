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
*    $Revision: 15324 $
*    $Author: ermold $
*    $Date: 2012-09-11 22:17:39 +0000 (Tue, 11 Sep 2012) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file cds_copy.c
 *  CDS Copy Functions.
 */

#include "cds3.h"
#include "cds_private.h"

/*******************************************************************************
 *  Private Functions
 */
/** @privatesection */

/**
 *  PRIVATE: Copy an attribute value from one attribute to another.
 *
 *  @param  converter - the data converter
 *  @param  att_flags - data attribute flags
 *  @param  src_att   - pointer to the source attribute
 *  @param  dest_att  - pointer to the destination attribute
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int _cds_copy_att_value(
    CDSConverter  converter,
    int           att_flags,
    CDSAtt       *src_att,
    CDSAtt       *dest_att)
{
    _CDSConverter *dc = (_CDSConverter *)converter;
    void          *value;

    if (converter) {

        value = cds_convert_array(
            converter, att_flags,
            src_att->length, src_att->value.vp, NULL);

        if (!value) {

            ERROR( CDS_LIB_NAME,
                "Could not copy attribute value\n"
                " -> from: %s\n"
                " -> to:   %s\n",
                cds_get_object_path(src_att),
                cds_get_object_path(dest_att));

            return(0);
        }

        if (dest_att->value.vp) {
            free(dest_att->value.vp);
        }

        dest_att->type     = dc->out_type;
        dest_att->length   = src_att->length;
        dest_att->value.vp = value;
    }
    else {

        if (!_cds_set_att_value(
            dest_att, src_att->type, src_att->length, src_att->value.vp)) {

            ERROR( CDS_LIB_NAME,
                "Could not copy attribute value\n"
                " -> from: %s\n"
                " -> to:   %s\n",
                cds_get_object_path(src_att),
                cds_get_object_path(dest_att));

            return(0);
        }
    }

    return(1);
}

/**
 *  PRIVATE: Copy a CDS Attribute.
 *
 *  See cds_copy_att() for details.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  converter   - converter to use for data attributes
 *  @param  src_att     - pointer to the source attribute
 *  @param  dest_parent - pointer to the destination group or variable
 *  @param  dest_name   - name of the destination attribute,
 *                        or NULL to use the source attribute name
 *  @param  flags       - control flags
 *  @param  dest_att    - output: pointer to the destination attribute
 *
 *  @return
 *    -  1 if sucessful
 *    -  0 if the attribute or value was not copied
 *    - -1 if an error occurred
 */
int _cds_copy_att(
    CDSConverter  converter,
    CDSAtt       *src_att,
    void         *dest_parent,
    const char   *dest_name,
    int           flags,
    CDSAtt      **dest_att)
{
    _CDSConverter *dc          = (_CDSConverter *)converter;
    CDSObject     *dest_object = (CDSObject *)dest_parent;
    CDSDataType    dest_type   = src_att->type;
    int            defined_att = 0;
    int            att_flags   = 0;
    CDSAtt        *tmp_att;

    /* Initialize variables */

    if (!dest_name) { dest_name = src_att->name;  }

    if (dest_att)   { *dest_att = (CDSAtt *)NULL; }
    else            {  dest_att = &tmp_att;       }

    /* Check if we need to skip this attribute */

    *dest_att = cds_get_att(dest_parent, dest_name);

    if (*dest_att) {

        if ((*dest_att)->def_lock) {
            return(0);
        }

        if ((*dest_att)->length) {

            if (!(flags & CDS_OVERWRITE_ATTS)) {
                return(0);
            }

            if (converter && (strcmp(src_att->name, "units") == 0)) {
                return(0);
            }
        }
    }
    else if (dest_object->def_lock || (flags & CDS_EXCLUSIVE)) {
        return(0);
    }

    /* Check if this is a data attribute */

    if (converter) {
        if (cds_is_data_att(src_att, &att_flags)) {
            dest_type = dc->out_type;
        }
        else {
            converter = (CDSConverter)NULL;
        }
    }

    /* Check if we need to define the attribute in the destination parent */

    if (!(*dest_att)) {

        *dest_att = _cds_define_att(
            dest_parent, dest_name, dest_type, 0, NULL);

        if (!(*dest_att)) {
            return(-1);
        }

        defined_att = 1;
    }

    /* Copy the attribute value */

    if (src_att->length && src_att->value.vp) {

        if (!(_cds_copy_att_value(
            converter, att_flags, src_att, *dest_att))) {

            if (defined_att) {
                cds_delete_att(*dest_att);
                *dest_att = (CDSAtt *)NULL;
            }

            return(-1);
        }
    }

    /* Check if we need to copy the definition lock value */

    if (src_att->def_lock && (flags & CDS_COPY_LOCKS)) {
        (*dest_att)->def_lock = src_att->def_lock;
    }

    return(1);
}

/**
 *  PRIVATE: Copy CDS Attributes.
 *
 *  See cds_copy_atts() for details.
 *
 *  @param  converter   - converter to use for data attributes
 *  @param  src_natts   - number of src attributes
 *  @param  src_atts    - array of src attributes
 *  @param  dest_parent - pointer to the destination CDSGroup or CDSVar
 *  @param  src_names   - NULL terminated list of attributes to copy,
 *                        or NULL to copy all attributes
 *  @param  dest_names  - corresponding destination attribute names,
 *                        or NULL to use the source attribute names
 *  @param  flags       - control flags
 *
 *  @return
 *    -  1 if successful
 *    -  0 if an error occurred
 */
int _cds_copy_atts(
    CDSConverter  converter,
    int           src_natts,
    CDSAtt      **src_atts,
    void         *dest_parent,
    const char  **src_names,
    const char  **dest_names,
    int           flags)
{
    CDSAtt     *src_att;
    const char *dest_name;
    int         ai, ni;

    /* Copy the attributes */

    if (src_names) {

        for (ni = 0; src_names[ni]; ni++) {

            for (ai = 0; ai < src_natts; ai++) {
                if (strcmp(src_names[ni], src_atts[ai]->name) == 0) {
                    break;
                }
            }

            if (ai != src_natts) {

                src_att = src_atts[ai];

                if (dest_names && dest_names[ni]) {
                    dest_name = dest_names[ni];
                }
                else {
                    dest_name = src_att->name;
                }

                if (_cds_copy_att(converter,
                    src_att, dest_parent, dest_name, flags, NULL) < 0) {
                    return(0);
                }
            }
        }
    }
    else {

        for (ai = 0; ai < src_natts; ai++) {

            src_att   = src_atts[ai];
            dest_name = src_att->name;

            if (_cds_copy_att(converter,
                src_att, dest_parent, dest_name, flags, NULL) < 0) {
                return(0);
            }
        }
    }

    return(1);
}

/**
 *  Copy a data from one CDS Variable to another.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  converter   - converter to use for data attributes
 *  @param  src_var      - pointer to the source CDSVar
 *  @param  dest_var     - pointer to the destination CDSVar
 *  @param  src_start    - start sample in the source variable
 *  @param  dest_start   - start sample in the destination variable
 *  @param  sample_count - number of samples to copy
 *  @param  flags        - control flags
 *
 *  @return
 *    -  1 if sucessful
 *    -  0 if the variable data was not copied
 *    - -1 if an error occurred
 */
int _cds_copy_var_data(
    CDSConverter  converter,
    CDSVar       *src_var,
    CDSVar       *dest_var,
    size_t        src_start,
    size_t        dest_start,
    size_t        sample_count,
    int           flags)
{
    size_t  src_sample_size  = cds_var_sample_size(src_var);
    size_t  dest_sample_size = cds_var_sample_size(dest_var);
    int     free_converter   = 0;
    int     nvalues;
    void   *src_data;
    void   *dest_data;

    /* Check if we need to skip the variable data */

    if ((flags & CDS_SKIP_DATA) ||
        (src_start >= src_var->sample_count)) {

        return(0);
    }

    if (!sample_count ||
        sample_count > src_var->sample_count - src_start) {
        sample_count = src_var->sample_count - src_start;
    }

    if (!(flags & CDS_OVERWRITE_DATA)) {

        if (dest_start + sample_count <= dest_var->sample_count) {
            return(0);
        }

        if (dest_start < dest_var->sample_count) {
            src_start    += (dest_var->sample_count - dest_start);
            sample_count -= (dest_var->sample_count - dest_start);
            dest_start    = dest_var->sample_count;
        }
    }

    /* Make sure the sample sizes match */

    if (src_sample_size != dest_sample_size) {

        ERROR( CDS_LIB_NAME,
            "Could not copy variable data\n"
            " -> from: %s\n"
            " -> to:   %s\n"
            " -> source sample size (%d) != destination sample size (%d)\n",
            cds_get_object_path(src_var), cds_get_object_path(dest_var),
            (int)src_sample_size, (int)dest_sample_size);

        return(-1);
    }

    /* Get source and destination data pointers */

    src_data = cds_get_var_datap(src_var, src_start);
    if (!src_data) {
        return(0);
    }

    dest_data = cds_alloc_var_data(dest_var, dest_start, sample_count);
    if (!dest_data) {

        ERROR( CDS_LIB_NAME,
            "Could not copy variable data\n"
            " -> from: %s\n"
            " -> to:   %s\n",
            cds_get_object_path(src_var),
            cds_get_object_path(dest_var));

        return(-1);
    }

    /* Create converter if necessary */

    if (!converter) {

        converter = cds_create_converter_var_to_var(src_var, dest_var);
        if (!converter) {

            ERROR( CDS_LIB_NAME,
                "Could not copy variable data\n"
                " -> from: %s\n"
                " -> to:   %s\n",
                cds_get_object_path(src_var),
                cds_get_object_path(dest_var));

            return(-1);
        }

        free_converter = 1;
    }

    /* Copy data */

    nvalues = sample_count * dest_sample_size;

    if (!cds_convert_array(
        converter, 0, nvalues, src_data, dest_data)) {

        ERROR( CDS_LIB_NAME,
            "Could not copy variable data\n"
            " -> from: %s\n"
            " -> to:   %s\n",
            cds_get_object_path(src_var),
            cds_get_object_path(dest_var));

        if (free_converter) cds_destroy_converter(converter);
        return(-1);
    }

    if (free_converter) cds_destroy_converter(converter);
    return(1);
}

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Copy a CDS Attribute.
 *
 *  By default (flags = 0) the attribute will be copied to the destination
 *  parent if it does not already exist. If it does exist, the attribute
 *  value will only be copied if the destination attribute has zero length.
 *  When variable data attributes are copied, the values will be converted
 *  to the data type and units of the destination variable as necessary. All
 *  other attribute values will be cast into the data type of the destination
 *  attribute.
 *
 *  The attribute will not be copied if the definition lock is set for the
 *  destination parent. Likewise, the attribute value will not be copied or
 *  overwritten if the definition lock is set for the destination attribute.
 *
 *  Control Flags:
 *
 *    - CDS_COPY_LOCKS     = copy definition lock value
 *
 *    - CDS_EXCLUSIVE      = exclude attributes that have not been
 *                           defined in the destination parent
 *
 *    - CDS_OVERWRITE_ATTS = overwrite existing attribute values if the
 *                           definition lock is not set on the attribute
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  src_att     - pointer to the source attribute
 *  @param  dest_parent - pointer to the destination group or variable
 *  @param  dest_name   - name of the destination attribute,
 *                        or NULL to use the source attribute name
 *  @param  flags       - control flags
 *  @param  dest_att    - output: pointer to the destination attribute
 *
 *  @return
 *    -  1 if sucessful
 *    -  0 if the attribute or value was not copied
 *    - -1 if an error occurred
 */
int cds_copy_att(
    CDSAtt      *src_att,
    void        *dest_parent,
    const char  *dest_name,
    int          flags,
    CDSAtt     **dest_att)
{
    CDSObject    *src_object  = (CDSObject *)src_att->parent;
    CDSObject    *dest_object = (CDSObject *)dest_parent;
    CDSConverter  converter   = (CDSConverter)NULL;
    int           status;

    /* Make sure the destination parent is a group or variable */

    if (dest_object->obj_type != CDS_GROUP &&
        dest_object->obj_type != CDS_VAR) {

        ERROR( CDS_LIB_NAME,
            "Could not copy attribute\n"
            " -> from: %s\n"
            " -> to:   %s\n"
            " -> destination parent must be a group or variable\n",
            cds_get_object_path(src_att),
            cds_get_object_path(dest_parent));

        if (dest_att) *dest_att = (CDSAtt *)NULL;
        return(-1);
    }

    /* Get the converter if this is a data attribute */

    if (src_object->obj_type  == CDS_VAR &&
        dest_object->obj_type == CDS_VAR &&
        cds_is_data_att(src_att, NULL)) {

        converter = cds_create_converter_var_to_var(
            (CDSVar *)src_object, (CDSVar *)dest_object);

        if (!converter) {

            ERROR( CDS_LIB_NAME,
                "Could not copy attribute\n"
                " -> from: %s\n"
                " -> to:   %s\n",
                cds_get_object_path(src_att),
                cds_get_object_path(dest_parent));

            return(-1);
        }
    }

    status = _cds_copy_att(
        converter, src_att, dest_parent, dest_name, flags, dest_att);

    cds_destroy_converter(converter);
    return(status);
}

/**
 *  Copy CDS Attributes.
 *
 *  This function will copy attributes from a source group or variable to a
 *  destination group or variable. By default (flags = 0) the attributes will
 *  be copied to the destination parent if they do not already exist. For
 *  attributes that do exist, the attribute values will only be copied if the
 *  destination attribute has zero length. When variable data attributes are
 *  copied, the values will be converted to the data type and units of the
 *  destination variable as necessary. All other attribute values will be cast
 *  into the data type of the destination attribute.
 *
 *  Attributes will not be copied if the definition lock is set for the
 *  destination parent. Likewise, the attribute values will not be copied or
 *  overwritten if the definition lock is set for the destination attribute.
 *
 *  Control Flags:
 *
 *    - CDS_COPY_LOCKS     = copy definition lock value
 *
 *    - CDS_EXCLUSIVE      = exclude attributes that have not been
 *                           defined in the destination parent
 *
 *    - CDS_OVERWRITE_ATTS = overwrite existing attribute values if the
 *                           definition lock is not set on the attribute
 *                           (ignored for the units attribute)
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  src_parent  - pointer to the source CDSGroup or CDSVar
 *  @param  dest_parent - pointer to the destination CDSGroup or CDSVar
 *  @param  src_names   - NULL terminated list of attributes to copy,
 *                        or NULL to copy all attributes
 *  @param  dest_names  - corresponding destination attribute names,
 *                        or NULL to use the source attribute names
 *  @param  flags       - control flags
 *
 *  @return
 *    -  1 if successful
 *    -  0 if an error occurred
 */
int cds_copy_atts(
    void        *src_parent,
    void        *dest_parent,
    const char **src_names,
    const char **dest_names,
    int          flags)
{
    CDSConverter  converter   = (CDSConverter)NULL;
    CDSObject    *dest_object = (CDSObject *)dest_parent;
    CDSObject    *src_object  = (CDSObject *)src_parent;
    CDSGroup     *src_group;
    CDSVar       *src_var;
    CDSAtt      **src_atts;
    int           src_natts;
    int           status;

    /* Make sure the destination parent is a group or variable */

    if (dest_object->obj_type != CDS_GROUP &&
        dest_object->obj_type != CDS_VAR) {

        ERROR( CDS_LIB_NAME,
            "Could not copy attributes\n"
            " -> from: %s\n"
            " -> to:   %s\n"
            " -> destination parent must be a group or variable\n",
            cds_get_object_path(src_parent),
            cds_get_object_path(dest_parent));

        return(0);
    }

    /* Make sure the source parent is a group or variable */

    if (src_object->obj_type == CDS_GROUP) {
        src_group = (CDSGroup *)src_parent;
        src_natts = src_group->natts;
        src_atts  = src_group->atts;
    }
    else if (src_object->obj_type == CDS_VAR) {
        src_var   = (CDSVar *)src_parent;
        src_natts = src_var->natts;
        src_atts  = src_var->atts;
    }
    else {

        ERROR( CDS_LIB_NAME,
            "Could not copy attributes\n"
            " -> from: %s\n"
            " -> to:   %s\n"
            " -> source parent must be a group or variable\n",
            cds_get_object_path(src_parent),
            cds_get_object_path(dest_parent));

        return(0);
    }

    /* Get the converter */

    if (src_object->obj_type  == CDS_VAR &&
        dest_object->obj_type == CDS_VAR) {

        converter = cds_create_converter_var_to_var(
            (CDSVar *)src_object, (CDSVar *)dest_object);

        if (!converter) {

            ERROR( CDS_LIB_NAME,
                "Could not copy attributes\n"
                " -> from: %s\n"
                " -> to:   %s\n",
                cds_get_object_path(src_parent),
                cds_get_object_path(dest_parent));

            return(0);
        }
    }

    /* Copy the attributes */

    status = _cds_copy_atts(converter,
        src_natts, src_atts, dest_parent,
        src_names, dest_names, flags);

    cds_destroy_converter(converter);
    return(status);
}

/**
 *  Copy a CDS Dimension.
 *
 *  By default (flags = 0) the dimension will be copied to the destination
 *  group if it does not already exist. If it does exist, the dimension
 *  length will only be copied if the destination dimension has zero length.
 *
 *  The dimension will not be copied if the definition lock is set for the
 *  destination group. Likewise, the dimension length will not be copied or
 *  overwritten if the definition lock is set for the destination dimension.
 *  The length of unlimited dimensions will also not be overwritten.
 *
 *  Control Flags:
 *
 *    - CDS_COPY_LOCKS     = copy definition lock value
 *
 *    - CDS_EXCLUSIVE      = exclude dimensions that have not been
 *                           defined in the destination group
 *
 *    - CDS_OVERWRITE_DIMS = overwrite existing dimension lengths if the
 *                           definition lock is not set on the dimension
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  src_dim    - pointer to the source CDSDim
 *  @param  dest_group - pointer to the destination CDSGroup
 *  @param  dest_name  - name of the destination dimension,
 *                       or NULL to use the source dimension name
 *  @param  flags      - control flags
 *  @param  dest_dim   - output: pointer to the destination dimension
 *
 *  @return
 *    -  1 if sucessful
 *    -  0 if the dimension or length was not copied
 *    - -1 if an error occurred
 */
int cds_copy_dim(
    CDSDim     *src_dim,
    CDSGroup   *dest_group,
    const char *dest_name,
    int         flags,
    CDSDim    **dest_dim)
{
    CDSDim *tmp_dim;

    /* Initialize variables */

    if (!dest_name) { dest_name = src_dim->name;  }

    if (dest_dim)   { *dest_dim = (CDSDim *)NULL; }
    else            {  dest_dim = &tmp_dim;       }

    /* Check if this dimension is defined in the destination group */

    *dest_dim = cds_get_dim(dest_group, dest_name);

    if (*dest_dim) {

        /* Check if we need to skip this dimension */

        if ((*dest_dim)->def_lock     ||
            (*dest_dim)->is_unlimited ||
            ((*dest_dim)->length && !(flags & CDS_OVERWRITE_DIMS))) {
            return(0);
        }

        /* Change the length of the destination dimension */

        if (!cds_change_dim_length(*dest_dim, src_dim->length)) {
            return(-1);
        }
    }
    else {

        /* Check if we need to skip this dimension */

        if (dest_group->def_lock ||
            flags & CDS_EXCLUSIVE) {
            return(0);
        }

        /* Define this dimension in the destination group */

        if (src_dim->is_unlimited) {
            *dest_dim = cds_define_dim(
                dest_group, dest_name, 0, 1);
        }
        else {
            *dest_dim = cds_define_dim(
                dest_group, dest_name, src_dim->length, 0);
        }

        if (!(*dest_dim)) {
            return(-1);
        }
/*
BDE TODO: Should coordinate variables be copied with dimensions?
*/
    }

    /* Check if we need to copy the definition lock value */

    if (src_dim->def_lock && (flags & CDS_COPY_LOCKS)) {
        (*dest_dim)->def_lock = src_dim->def_lock;
    }

    return(1);
}

/**
 *  Copy CDS Dimensions.
 *
 *  This function will copy dimensions from a source group to a destination
 *  group. By default (flags = 0) the dimensions will be copied to the
 *  destination group if they do not already exist. For dimensions that do
 *  exist, the dimension lengths will only be copied if the destination
 *  dimension has zero length.
 *
 *  Dimensions will not be copied if the definition lock is set for the
 *  destination group. Likewise, the dimension lengths will not be copied or
 *  overwritten if the definition lock is set for the destination dimension.
 *  The length of unlimited dimensions will also not be overwritten.
 *
 *  Control Flags:
 *
 *    - CDS_COPY_LOCKS     = copy definition lock value
 *
 *    - CDS_EXCLUSIVE      = exclude dimensions that have not been
 *                           defined in the destination group
 *
 *    - CDS_OVERWRITE_DIMS = overwrite existing dimension lengths if the
 *                           definition lock is not set on the dimension
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  src_group   - pointer to the source group
 *  @param  dest_group  - pointer to the destination group
 *  @param  src_names   - NULL terminated list of dimensions to copy,
 *                        or NULL to copy all dimensions
 *  @param  dest_names  - corresponding destination dimension names,
 *                        or NULL to use the source dimension names
 *  @param  flags       - control flags
 *
 *  @return
 *    -  1 if successful
 *    -  0 if an error occurred
 */
int cds_copy_dims(
    CDSGroup    *src_group,
    CDSGroup    *dest_group,
    const char **src_names,
    const char **dest_names,
    int          flags)
{
    CDSDim     *src_dim;
    const char *dest_name;
    int         di, ni;

    if (src_names) {

        for (ni = 0; src_names[ni]; ni++) {

            for (di = 0; di < src_group->ndims; di++) {
                if (strcmp(src_names[ni], src_group->dims[di]->name) == 0) {
                    break;
                }
            }

            if (di != src_group->ndims) {

                src_dim = src_group->dims[di];

                if (dest_names && dest_names[ni]) {
                    dest_name = dest_names[ni];
                }
                else {
                    dest_name = src_dim->name;
                }

                if (cds_copy_dim(
                    src_dim, dest_group, dest_name, flags, NULL) < 0) {
                    return(0);
                }
            }
        }
    }
    else {

        for (di = 0; di < src_group->ndims; di++) {

            src_dim   = src_group->dims[di];
            dest_name = src_dim->name;

            if (cds_copy_dim(
                src_dim, dest_group, dest_name, flags, NULL) < 0) {
                return(0);
            }
        }
    }

    return(1);
}

/**
 *  Copy a CDS Variable.
 *
 *  This function will also copy all dependant dimensions that have not already
 *  been defined in the destination group.
 *
 *  By default (flags = 0) the variable will be copied to the destination group
 *  if it does not already exist. If it does exist, the variable data will only
 *  be copied for samples that have not already been defined in the variable.
 *  When variable data and data attributes are copied, the values will be
 *  converted to the data type and units of the destination variable as
 *  necessary.
 *
 *  The variable will not be copied if the definition lock is set for the
 *  destination group. Likewise, variable attributes will not be copied if the
 *  definition lock is set for the destination variable, and attribute values
 *  will not be copied or overwritten if the definition lock is set for the
 *  destination attribute.
 *
 *  Control Flags:
 *
 *    - CDS_SKIP_VAR_ATTS  = do not copy variable attributes
 *    - CDS_SKIP_DATA      = do not copy variable data
 *
 *    - CDS_COPY_LOCKS     = copy definition lock values
 *
 *    - CDS_EXCLUSIVE      = exclude variables and attributes that have
 *                           not been defined in the destination parent
 *
 *    - CDS_OVERWRITE_ATTS = overwrite existing attribute values if the
 *                           definition lock is not set on the attribute
 *
 *    - CDS_OVERWRITE_DATA = overwrite existing variable data
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  src_var        - pointer to the source CDSVar
 *  @param  dest_group     - pointer to the destination CDSGroup
 *  @param  dest_name      - name of the destination variable,
 *                           or NULL to use the source variable name
 *
 *  @param  src_dim_names  - NULL terminated list of source dimension names,
 *                           or NULL if all destination dimension names are
 *                           the same as the source dimension names
 *  @param  dest_dim_names - corresponding destination dimension names
 *
 *  @param  src_att_names  - NULL terminated list of attributes to copy,
 *                           or NULL to copy all attributes
 *  @param  dest_att_names - corresponding destination attribute names,
 *                           or NULL to use the source attribute names
 *
 *  @param  src_start      - start sample in the source variable
 *  @param  dest_start     - start sample in the destination variable
 *  @param  sample_count   - number of samples to copy
 *                           or 0 to copy all available samples
 *
 *  @param  flags          - control flags
 *  @param  dest_var       - output: pointer to the destination variable
 *
 *  @return
 *    -  1 if sucessful
 *    -  0 if the variable was not copied
 *    - -1 if an error occurred
 */
int cds_copy_var(
    CDSVar      *src_var,
    CDSGroup    *dest_group,
    const char  *dest_name,
    const char **src_dim_names,
    const char **dest_dim_names,
    const char **src_att_names,
    const char **dest_att_names,
    size_t       src_start,
    size_t       dest_start,
    size_t       sample_count,
    int          flags,
    CDSVar     **dest_var)
{
    CDSConverter converter   = (CDSConverter)NULL;
    int          defined_var = 0;
    char       **dim_names   = (char **)NULL;
    const char  *dest_dim_name;
    CDSDim      *dest_dim;
    CDSDim      *src_dim;
    CDSVar      *tmp_var;
    int          status;
    int          di, ni;

    /* Initialize variables */

    if (!dest_name) { dest_name = src_var->name;  }

    if (dest_var)   { *dest_var = (CDSVar *)NULL; }
    else            {  dest_var = &tmp_var;       }

    /* Define the variable in the destination group if it does not
     * already exist and the CDS_EXCLUSIVE flag has not been set */

    *dest_var = cds_get_var(dest_group, dest_name);

    if (!(*dest_var)) {

        if (flags & CDS_EXCLUSIVE || dest_group->def_lock) {
            return(0);
        }

        /* Create the dimension names list (and dimensions if necessary) */

        if (src_var->ndims) {

            /* Allocate memory for the dimension names list */

            dim_names = (char **)malloc(src_var->ndims * sizeof(char *));

            if (!dim_names) {

                ERROR( CDS_LIB_NAME,
                    "Could not copy variable\n"
                    " -> from: %s\n"
                    " -> to:   %s\n"
                    " -> memory allocation error\n",
                    cds_get_object_path(src_var),
                    cds_get_object_path(dest_group));

                return(-1);
            }

            /* Loop over the source variable's dimensions */

            for (di = 0; di < src_var->ndims; di++) {

                src_dim       = src_var->dims[di];
                dest_dim_name = src_dim->name;

                if (src_dim_names && dest_dim_names) {
                    for (ni = 0; src_dim_names[ni]; ni++) {
                        if (strcmp(src_dim->name, src_dim_names[ni]) == 0) {
                            if (dest_dim_names[ni]) {
                                dest_dim_name = dest_dim_names[ni];
                            }
                            break;
                        }
                    }
                }

                dest_dim = cds_get_dim(dest_group, dest_dim_name);

                if (!dest_dim) {

                    /* Copy the dimension to the destination group */

                    if (cds_copy_dim(src_dim,
                        dest_group, dest_dim_name, flags, &dest_dim) < 0) {

                        free(dim_names);
                        return(-1);
                    }
                }

                dim_names[di] = dest_dim->name;
            }
        }

        /* Create the variable in the destination group */

        *dest_var = cds_define_var(
            dest_group,
            dest_name,
            src_var->type,
            src_var->ndims,
            (const char **)dim_names);

        if (dim_names) {
            free(dim_names);
        }

        if (!(*dest_var)) {
            return(-1);
        }

        defined_var = 1;

        /* Create a simple copy converter */

        if (!(flags & CDS_SKIP_VAR_ATTS) ||
            !(flags & CDS_SKIP_DATA)) {

            converter = cds_create_converter(
                src_var->type, NULL, src_var->type, NULL);

            if (!converter) {

                ERROR( CDS_LIB_NAME,
                    "Could not copy variable\n"
                    " -> from: %s\n"
                    " -> to:   %s\n",
                    cds_get_object_path(src_var),
                    cds_get_object_path(*dest_var));

                cds_delete_var(*dest_var);
                *dest_var = (CDSVar *)NULL;

                return(-1);
            }
        }
    }
    else {

        /* Get the var to var converter */

        if (!(flags & CDS_SKIP_VAR_ATTS) ||
            !(flags & CDS_SKIP_DATA)) {

            converter = cds_create_converter_var_to_var(src_var, *dest_var);
            if (!converter) {

                ERROR( CDS_LIB_NAME,
                    "Could not copy variable\n"
                    " -> from: %s\n"
                    " -> to:   %s\n",
                    cds_get_object_path(src_var),
                    cds_get_object_path(*dest_var));

                *dest_var = (CDSVar *)NULL;

                return(-1);
            }
        }
    }

    /* Copy variable attributes if the CDS_SKIP_VAR_ATTS
     * flag has not been set*/

    if (!(flags & CDS_SKIP_VAR_ATTS)) {

        if (!_cds_copy_atts(converter,
            src_var->natts, src_var->atts, *dest_var,
            src_att_names, dest_att_names, flags)) {

            if (defined_var) {
                cds_delete_var(*dest_var);
                *dest_var = (CDSVar *)NULL;
            }

            cds_destroy_converter(converter);
            return(-1);
        }
    }

    /* Copy the default fill value */

    if (src_var->default_fill && !(*dest_var)->default_fill) {

        (*dest_var)->default_fill = cds_convert_array(
            converter, 0, 1, src_var->default_fill, NULL);

        if (!(*dest_var)->default_fill) {

            ERROR( CDS_LIB_NAME,
                "Could not copy variable\n"
                " -> from: %s\n"
                " -> to:   %s\n",
                cds_get_object_path(src_var),
                cds_get_object_path(*dest_var));

            if (defined_var) {
                cds_delete_var(*dest_var);
                *dest_var = (CDSVar *)NULL;
            }

            cds_destroy_converter(converter);
            return(-1);
        }
    }

    /* Copy definition lock value */

    if (flags & CDS_COPY_LOCKS) {
        (*dest_var)->def_lock = src_var->def_lock;
    }

    /* Copy the data */

    status = _cds_copy_var_data(converter,
        src_var, *dest_var, src_start, dest_start, sample_count, flags);

    cds_destroy_converter(converter);

    if (status < 0) {

        if (defined_var) {
            cds_delete_var(*dest_var);
            *dest_var = (CDSVar *)NULL;
        }

        return(-1);
    }

    return(1);
}

/**
 *  Copy CDS Variables.
 *
 *  This function will copy all variables from the source group to the
 *  destination group. All dependant dimensions that have not already been
 *  defined in the destination group will also be copied.
 *
 *  By default (flags = 0) the variables will be copied to the destination group
 *  if they do not already exist. For variables that do exist, the variable data
 *  will only be copied for samples that have not already been defined in the
 *  destination variables. When variable data and data attributes are copied,
 *  the values will be converted to the data type and units of the destination
 *  variable as necessary.
 *
 *  Variables will not be copied if the definition lock is set for the
 *  destination group. Likewise, variable attributes will not be copied if the
 *  definition lock is set for the destination variable, and attribute values
 *  will not be copied or overwritten if the definition lock is set for the
 *  destination attribute.
 *
 *  Control Flags:
 *
 *    - CDS_SKIP_VAR_ATTS  = do not copy variable attributes
 *    - CDS_SKIP_DATA      = do not copy variable data
 *
 *    - CDS_COPY_LOCKS     = copy definition lock values
 *
 *    - CDS_EXCLUSIVE      = exclude attributes and variables that have
 *                           not been defined in the destination parent
 *
 *    - CDS_OVERWRITE_ATTS = overwrite existing attribute values if the
 *                           definition lock is not set on the attribute
 *
 *    - CDS_OVERWRITE_DATA = overwrite existing variable data
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  src_group      - pointer to the source group
 *  @param  dest_group     - pointer to the destination group
 *
 *  @param  src_dim_names  - NULL terminated list of source dimension names,
 *                           or NULL if all destination dimension names are
 *                           the same as the source dimension names
 *  @param  dest_dim_names - corresponding destination dimension names
 *
 *  @param  src_var_names  - NULL terminated list of variables to copy,
 *                           or NULL to copy all variables
 *  @param  dest_var_names - corresponding destination variable names,
 *                           or NULL to use the source variable names
 *
 *  @param  src_start      - start sample to use for source variables
 *                           that have an unlimited dimension
 *  @param  dest_start     - start sample to use for the corresponding
 *                           destination variables
 *  @param  sample_count   - number of samples to copy
 *                           or 0 to copy all available samples
 *
 *  @param  flags          - control flags
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int cds_copy_vars(
    CDSGroup    *src_group,
    CDSGroup    *dest_group,
    const char **src_dim_names,
    const char **dest_dim_names,
    const char **src_var_names,
    const char **dest_var_names,
    size_t       src_start,
    size_t       dest_start,
    size_t       sample_count,
    int          flags)
{
    CDSVar     *src_var;
    const char *dest_var_name;
    int         status;
    int         vi, ni;

    if (src_var_names) {

        for (ni = 0; src_var_names[ni]; ni++) {

            for (vi = 0; vi < src_group->nvars; vi++) {
                if (strcmp(src_var_names[ni], src_group->vars[vi]->name) == 0) {
                    break;
                }
            }

            if (vi != src_group->nvars) {

                src_var = src_group->vars[vi];

                if (dest_var_names && dest_var_names[ni]) {
                    dest_var_name = dest_var_names[ni];
                }
                else {
                    dest_var_name = src_var->name;
                }

                if (cds_var_is_unlimited(src_var)) {
                    status = cds_copy_var(
                        src_var, dest_group, dest_var_name,
                        src_dim_names, dest_dim_names, NULL, NULL,
                        src_start, dest_start, sample_count,
                        flags, NULL);
                }
                else {
                    status = cds_copy_var(
                        src_var, dest_group, dest_var_name,
                        src_dim_names, dest_dim_names, NULL, NULL,
                        0, 0, 0, flags, NULL);
                }

                if (status < 0) {
                    return(0);
                }
            }
        }
    }
    else {

        for (vi = 0; vi < src_group->nvars; vi++) {

            src_var       = src_group->vars[vi];
            dest_var_name = src_var->name;

            if (cds_var_is_unlimited(src_var)) {
                status = cds_copy_var(
                    src_var, dest_group, dest_var_name,
                    src_dim_names, dest_dim_names, NULL, NULL,
                    src_start, dest_start, sample_count,
                    flags, NULL);
            }
            else {
                status = cds_copy_var(
                    src_var, dest_group, dest_var_name,
                    src_dim_names, dest_dim_names, NULL, NULL,
                    0, 0, 0, flags, NULL);
            }

            if (status < 0) {
                return(0);
            }
        }
    }

    return(1);
}

/**
 *  Copy a CDS Group.
 *
 *  By default (flags = 0) the group will be copied to the destination parent
 *  if it does not already exist. If it does exist, all objects in the source
 *  group will be copied to the destination group if they do not already exist.
 *  For objects that do exist, their values and/or data will be copied if the
 *  destination object has zero length.
 *
 *  Dimensions, attributes, variables, and groups will not be copied if the
 *  definition lock is set for the destination group. Likewise, variable
 *  attributes will not be copied if the definition lock is set for the
 *  destination variable. Dimension lengths and attribute values will not be
 *  copied or overwritten if the definition lock is set for the destination
 *  object. The length of unlimited dimensions will also not be overwritten.
 *
 *  Control Flags:
 *
 *    - CDS_SKIP_DIMS       = do not copy dimensions
 *    - CDS_SKIP_GROUP_ATTS = do not copy group attributes
 *    - CDS_SKIP_VAR_ATTS   = do not copy variable attributes
 *    - CDS_SKIP_VARS       = do not copy variables
 *    - CDS_SKIP_DATA       = do not copy variable data
 *    - CDS_SKIP_SUBGROUPS  = do not traverse subgroups
 *
 *    - CDS_COPY_LOCKS      = copy definition lock values
 *
 *    - CDS_EXCLUSIVE       = exclude dimensions, attributes, variables,
 *                            and groups that have not been defined in
 *                            the destination parent
 *
 *    - CDS_OVERWRITE_DIMS  = overwrite existing dimension lengths if the
 *                            definition lock is not set on the dimension
 *
 *    - CDS_OVERWRITE_ATTS  = overwrite existing attribute values if the
 *                            definition lock is not set on the attribute
 *
 *    - CDS_OVERWRITE_DATA  = overwrite existing variable data
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  src_group           - pointer to the source group
 *  @param  dest_parent         - pointer to the destination parent,
 *                                or NULL to create a new root group
 *  @param  dest_name           - name of the destination group
 *                                or NULL to use the source group name
 *
 *  @param  src_dim_names       - NULL terminated list of dimensions to copy,
 *                                or NULL to copy all dimensions
 *  @param  dest_dim_names      - corresponding destination dimension names,
 *                                or NULL to use the source dimension names
 *
 *  @param  src_att_names       - NULL terminated list of group attributes to copy,
 *                                or NULL to copy all group attributes
 *  @param  dest_att_names      - corresponding destination attribute names,
 *                                or NULL to use the source attribute names
 *
 *  @param  src_var_names       - NULL terminated list of variables to copy,
 *                                or NULL to copy all variables
 *  @param  dest_var_names      - corresponding destination variable names,
 *                                or NULL to use the source variable names
 *
 *  @param  src_subgroup_names  - NULL terminated list of subgroups to copy,
 *                                or NULL to copy all subgroups
 *  @param  dest_subgroup_names - corresponding destination subgroup names,
 *                                or NULL to use the source subgroup names
 *
 *  @param  src_start           - start sample to use for source variables
 *                                that have an unlimited dimension
 *  @param  dest_start          - start sample to use for the corresponding
 *                                destination variables
 *  @param  sample_count        - number of samples to copy
 *                                or 0 to copy all available samples
 *
 *  @param  flags               - control flags
 *  @param  dest_group          - output: pointer to the destination group
 *
 *  @return
 *    -  1 if sucessful
 *    -  0 if the group was not copied
 *    - -1 if an error occurred
 */
int cds_copy_group(
    CDSGroup    *src_group,
    CDSGroup    *dest_parent,
    const char  *dest_name,
    const char **src_dim_names,
    const char **dest_dim_names,
    const char **src_att_names,
    const char **dest_att_names,
    const char **src_var_names,
    const char **dest_var_names,
    const char **src_subgroup_names,
    const char **dest_subgroup_names,
    size_t       src_start,
    size_t       dest_start,
    size_t       sample_count,
    int          flags,
    CDSGroup   **dest_group)
{
    int       defined_group = 0;
    CDSGroup *tmp_group;

    /* Initialize variables */

    if (!dest_name)  { dest_name  = src_group->name;  }
    if (!dest_group) { dest_group = &tmp_group;       }

    /* Define the group in the destination parent if it does not
     * already exist and the CDS_EXCLUSIVE flag has not been set */

    if (dest_parent) {
        *dest_group = cds_get_group(dest_parent, dest_name);
    }
    else {
        *dest_group = (CDSGroup *)NULL;
    }

    if (!(*dest_group)) {

        if ((flags & CDS_EXCLUSIVE) || (dest_parent && dest_parent->def_lock)) {
            return(0);
        }

        *dest_group = cds_define_group(dest_parent, dest_name);

        if (!(*dest_group)) {
            return(-1);
        }

        defined_group = 1;
    }

    /* Copy dimensions */

    if (!(flags & CDS_SKIP_DIMS)) {

        if (!cds_copy_dims(
            src_group,    *dest_group,
            src_dim_names, dest_dim_names, flags)) {

            if (defined_group) {
                cds_delete_group(*dest_group);
                *dest_group = (CDSGroup *)NULL;
            }

            return(-1);
        }
    }

    /* Copy attributes */

    if (!(flags & CDS_SKIP_GROUP_ATTS)) {

        if (!cds_copy_atts(
            src_group,    *dest_group,
            src_att_names, dest_att_names, flags)) {

            if (defined_group) {
                cds_delete_group(*dest_group);
                *dest_group = (CDSGroup *)NULL;
            }

            return(-1);
        }
    }

    /* Copy variables */

    if (!(flags & CDS_SKIP_VARS)) {

        if (!cds_copy_vars(
            src_group,    *dest_group,
            src_dim_names, dest_dim_names,
            src_var_names, dest_var_names,
            src_start,     dest_start, sample_count, flags)) {

            if (defined_group) {
                cds_delete_group(*dest_group);
                *dest_group = (CDSGroup *)NULL;
            }

            return(-1);
        }
    }

    /* Copy Groups */

    if (!(flags & CDS_SKIP_SUBGROUPS)) {

        if (!cds_copy_subgroups(
            src_group,         *dest_group,
            src_dim_names,      dest_dim_names,
            src_att_names,      dest_att_names,
            src_var_names,      dest_var_names,
            src_subgroup_names, dest_subgroup_names,
            src_start,          dest_start,
            sample_count, flags)) {

            if (defined_group) {
                cds_delete_group(*dest_group);
                *dest_group = (CDSGroup *)NULL;
            }

            return(-1);
        }
    }

    /* Copy definition lock value */

    if (flags & CDS_COPY_LOCKS) {
        (*dest_group)->def_lock = src_group->def_lock;
    }

    return(1);
}

/**
 *  Copy CDS Subgroups.
 *
 *  This function will copy all subgroups from the source group to the
 *  destination group.
 *
 *  By default (flags = 0) the subgroups will be copied to the destination group
 *  if they do not already exist. For subgroups that do exist, all objects in
 *  the source subgroups will be copied to the destination subgroups if they do
 *  not already exist. For objects that do exist, their values and/or data will
 *  be copied if the destination object has zero length.
 *
 *  Dimensions, attributes, variables, and groups will not be copied if the
 *  definition lock is set for the destination group. Likewise, variable
 *  attributes will not be copied if the definition lock is set for the
 *  destination variable. Dimension lengths and attribute values will not be
 *  copied or overwritten if the definition lock is set for the destination
 *  object. The length of unlimited dimensions will also not be overwritten.
 *
 *  Control Flags:
 *
 *    - CDS_SKIP_DIMS       = do not copy dimensions
 *    - CDS_SKIP_GROUP_ATTS = do not copy group attributes
 *    - CDS_SKIP_VAR_ATTS   = do not copy variable attributes
 *    - CDS_SKIP_VARS       = do not copy variables
 *    - CDS_SKIP_DATA       = do not copy variable data
 *    - CDS_SKIP_SUBGROUPS  = do not traverse subgroups
 *
 *    - CDS_COPY_LOCKS      = copy definition lock values
 *
 *    - CDS_EXCLUSIVE       = exclude dimensions, attributes, variables,
 *                            and groups that have not been defined in
 *                            the destination parent
 *
 *    - CDS_OVERWRITE_DIMS  = overwrite existing dimension lengths if the
 *                            definition lock is not set on the dimension
 *
 *    - CDS_OVERWRITE_ATTS  = overwrite existing attribute values if the
 *                            definition lock is not set on the attribute
 *
 *    - CDS_OVERWRITE_DATA  = overwrite existing variable data
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  src_group           - pointer to the source group
 *  @param  dest_group          - pointer to the destination group
 *
 *  @param  src_dim_names       - NULL terminated list of dimensions to copy,
 *                                or NULL to copy all dimensions
 *  @param  dest_dim_names      - corresponding destination dimension names,
 *                                or NULL to use the source dimension names
 *
 *  @param  src_att_names       - NULL terminated list of group attributes to copy,
 *                                or NULL to copy all group attributes
 *  @param  dest_att_names      - corresponding destination attribute names,
 *                                or NULL to use the source attribute names
 *
 *  @param  src_var_names       - NULL terminated list of variables to copy,
 *                                or NULL to copy all variables
 *  @param  dest_var_names      - corresponding destination variable names,
 *                                or NULL to use the source variable names
 *
 *  @param  src_subgroup_names  - NULL terminated list of subgroups to copy,
 *                                or NULL to copy all subgroups
 *  @param  dest_subgroup_names - corresponding destination subgroup names,
 *                                or NULL to use the source subgroup names
 *
 *  @param  src_start           - start sample to use for source variables
 *                                that have an unlimited dimension
 *  @param  dest_start          - start sample to use for the corresponding
 *                                destination variables
 *  @param  sample_count        - number of samples to copy
 *                                or 0 to copy all available samples
 *
 *  @param  flags               - control flags
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int cds_copy_subgroups(
    CDSGroup    *src_group,
    CDSGroup    *dest_group,
    const char **src_dim_names,
    const char **dest_dim_names,
    const char **src_att_names,
    const char **dest_att_names,
    const char **src_var_names,
    const char **dest_var_names,
    const char **src_subgroup_names,
    const char **dest_subgroup_names,
    size_t       src_start,
    size_t       dest_start,
    size_t       sample_count,
    int          flags)
{
    CDSGroup   *src_subgroup;
    const char *dest_subgroup_name;
    int         gi, ni;

    if (src_subgroup_names) {

        for (ni = 0; src_subgroup_names[ni]; ni++) {

            for (gi = 0; gi < src_group->ngroups; gi++) {
                if (strcmp(src_subgroup_names[ni], src_group->groups[gi]->name) == 0) {
                    break;
                }
            }

            if (gi != src_group->ngroups) {

                src_subgroup = src_group->groups[gi];

                if (dest_subgroup_names && dest_subgroup_names[ni]) {
                    dest_subgroup_name = dest_subgroup_names[ni];
                }
                else {
                    dest_subgroup_name = src_subgroup->name;
                }

                if (!cds_copy_group(
                    src_subgroup,       dest_group, dest_subgroup_name,
                    src_dim_names,      dest_dim_names,
                    src_att_names,      dest_att_names,
                    src_var_names,      dest_var_names,
                    src_subgroup_names, dest_subgroup_names,
                    src_start,          dest_start,
                    sample_count, flags, NULL)) {

                    return(0);
                }
            }
        }
    }
    else {

        for (gi = 0; gi < src_group->ngroups; gi++) {

            src_subgroup       = src_group->groups[gi];
            dest_subgroup_name = src_subgroup->name;

            if (!cds_copy_group(
                src_subgroup,       dest_group, dest_subgroup_name,
                src_dim_names,      dest_dim_names,
                src_att_names,      dest_att_names,
                src_var_names,      dest_var_names,
                src_subgroup_names, dest_subgroup_names,
                src_start,          dest_start,
                sample_count, flags, NULL)) {

                return(0);
            }
        }
    }

    return(1);
}
