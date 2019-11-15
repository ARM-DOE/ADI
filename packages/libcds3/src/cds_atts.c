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
*    $Revision: 60386 $
*    $Author: ermold $
*    $Date: 2015-02-19 20:43:41 +0000 (Thu, 19 Feb 2015) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file cds_atts.c
 *  CDS Attributes.
 */

#include "cds3.h"
#include "cds_private.h"

/*******************************************************************************
 *  Private Functions
 */
/** @privatesection */

/**
 *  PRIVATE: Change the type and value of a CDS Attribute.
 *
 *  @param  att    - pointer to the attribute
 *  @param  type   - attribute data type
 *  @param  length - attribute length
 *  @param  value  - pointer to the attribute value
 *
 *  @return
 *    - 1 if successful
 *    - 0 if a memory allocation error occurred
 */
int _cds_change_att_value(
    CDSAtt      *att,
    CDSDataType  type,
    size_t       length,
    void        *value)
{
    void   *new_value = (void *)NULL;
    size_t  type_size;

    /* Create the attribute value */

    if (length) {

        type_size = cds_data_type_size(type);

        /* Add 1 to the allocated length. The main reason for this is to
         * ensure character strings are derminated by a trailing \0...
         */

        new_value = calloc(length+1, type_size);
        if (!new_value) {
            return(0);
        }

        if (value) {
            memcpy(new_value, value, length * type_size);
        }
    }

    /* Set the value in the attribute structure */

    if (att->value.vp) {
        free(att->value.vp);
    }

    att->type     = type;
    att->length   = length;
    att->value.vp = new_value;

    return(1);
}

/**
 *  PRIVATE: Create a CDS Attribute.
 *
 *  Private function used to create an attribute.
 *
 *  @param  parent - pointer to the parent CDSGroup or CDSVar
 *  @param  name   - attribute name
 *  @param  type   - attribute data type
 *  @param  length - attribute length
 *  @param  value  - pointer to the attribute value
 *
 *  @return
 *    - pointer to the new attribute
 *    - NULL if a memory allocation error occurred
 */
CDSAtt *_cds_create_att(
    void        *parent,
    const char  *name,
    CDSDataType  type,
    size_t       length,
    void        *value)
{
    CDSAtt *att;

    att = (CDSAtt *)calloc(1, sizeof(CDSAtt));
    if (!att) {
        return((CDSAtt *)NULL);
    }

    if (!_cds_init_object_members(att, CDS_ATT, parent, name)) {
        free(att);
        return((CDSAtt *)NULL);
    }

    if (!_cds_change_att_value(att, type, length, value)) {
        _cds_free_object_members(att);
        free(att);
        return((CDSAtt *)NULL);
    }

    return(att);
}

/**
 *  Private: Define a CDS Attribute.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  parent   - pointer to the parent CDSGroup or CDSVar
 *  @param  name     - attribute name
 *  @param  type     - attribute data type
 *  @param  length   - attribute length
 *  @param  value    - pointer to the attribute value
 *
 *  @return
 *    - pointer to the attribute
 *    - NULL if:
 *        - the parent object is not a group or variable
 *        - the parent group or variable is locked
 *        - a memory allocation error occurred
 */
CDSAtt *_cds_define_att(
    void        *parent,
    const char  *name,
    CDSDataType  type,
    size_t       length,
    void        *value)
{
    CDSObject *parent_object = (CDSObject *)parent;
    CDSGroup  *group;
    CDSVar    *var;
    CDSAtt   **atts;
    CDSAtt  ***attsp;
    int       *nattsp;

    /* Make sure the parent is a group or variable */

    if (parent_object->obj_type == CDS_GROUP) {

        group  = (CDSGroup *)parent;
        nattsp = &(group->natts);
        attsp  = &(group->atts);

        /* Check if the group is locked */

        if (group->def_lock) {

            ERROR( CDS_LIB_NAME,
                "Could not define attribute: %s/_atts_/%s\n"
                " -> the group definition lock is set to: %d\n",
                cds_get_object_path(parent), name, group->def_lock);

            return((CDSAtt *)NULL);
        }
    }
    else if (parent_object->obj_type == CDS_VAR) {

        var    = (CDSVar *)parent_object;
        nattsp = &(var->natts);
        attsp  = &(var->atts);

        /* Check if the variable is locked */

        if (var->def_lock) {

            ERROR( CDS_LIB_NAME,
                "Could not define attribute: %s/_atts_/%s\n"
                " -> the variable definition lock is set to: %d\n",
                cds_get_object_path(parent), name, var->def_lock);

            return((CDSAtt *)NULL);
        }
    }
    else {

        ERROR( CDS_LIB_NAME,
            "Could not define attribute: %s/_atts_/%s\n"
            " -> parent object must be a group or variable\n",
            cds_get_object_path(parent), name);

        return((CDSAtt *)NULL);
    }

    /* Allocate space for a new attribute */

    atts = (CDSAtt **)realloc(
        *attsp, (*nattsp + 2) * sizeof(CDSAtt *));

    if (!atts) {

        ERROR( CDS_LIB_NAME,
            "Could not define attribute: %s/_atts_/%s\n"
            " -> memory allocation error\n",
            cds_get_object_path(parent), name);

        return((CDSAtt *)NULL);
    }

    *attsp = atts;

    /* Create the attribute */

    atts[*nattsp] = _cds_create_att(
        parent, name, type, length, value);

    if (!atts[*nattsp]) {

        ERROR( CDS_LIB_NAME,
            "Could not define attribute: %s/_atts_/%s\n"
            " -> memory allocation error\n",
            cds_get_object_path(parent), name);

        return((CDSAtt *)NULL);
    }

    (*nattsp)++;
    atts[*nattsp] = (CDSAtt *)NULL;

    return(atts[*nattsp - 1]);
}

/**
 *  PRIVATE: Destroy a CDS Attribute.
 *
 *  Private function used to destroy an attribute.
 *
 *  @param  att - pointer to the attribute
 */
void _cds_destroy_att(CDSAtt *att)
{
    if (att) {

        if (att->value.vp) free(att->value.vp);

        _cds_free_object_members(att);

        free(att);
    }
}

/**
 *  PRIVATE: Set the value of a CDS attribute.
 *
 *  This function will set the value of an attribute by casting the
 *  specified value into the data type of the attribute. The functions
 *  cds_string_to_array() and cds_array_to_string() are used to convert
 *  between text (CDS_CHAR) and numeric data types.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  att    - pointer to the attribute
 *  @param  type   - data type of the specified value
 *  @param  length - length of the specified value
 *  @param  value  - pointer to the attribute value
 *
 *  @return
 *    - 1 if successful
 *    - 0 if a memory allocation error occurred
 */
int _cds_set_att_value(
    CDSAtt      *att,
    CDSDataType  type,
    size_t       length,
    void        *value)
{
    void  *new_value = (void *)NULL;
    size_t type_size;

    /* Create the new attribute value */

    if ((type   == CDS_CHAR) &&
        (length == 0       ) &&
        (value  != NULL    )) {

        length = strlen(value) + 1;
    }

    if (value && length) {

        if (type == CDS_CHAR) {

            if (att->type == CDS_CHAR) {
                new_value = calloc(length+1, sizeof(char));
                if (!new_value) {
                    length = (size_t)-1;
                }
                else {
                    memcpy(new_value, value, length * sizeof(char));
                }
            }
            else {
                new_value = cds_string_to_array(
                    (const char *)value, att->type, &length, NULL);
            }
        }
        else if (att->type == CDS_CHAR) {
            new_value = cds_array_to_string(type, length, value, &length, NULL);
            if (new_value) {
                length = strlen(new_value) + 1;
            }
        }
        else {
            new_value = cds_copy_array(
                type, length, value, att->type, NULL,
                0, NULL, NULL, NULL, NULL, NULL, NULL);
            if (!new_value) {
                length = (size_t)-1;
            }
        }
    }
    else if (length) {
        type_size = cds_data_type_size(att->type);
        new_value = calloc(length+1, type_size);
    }
    else {
        new_value = (void *)NULL;
        length    = 0;
    }

    if (length == (size_t)-1) {

        ERROR( CDS_LIB_NAME,
            "Could not set attribute value for: %s\n"
            " -> memory allocation error\n", cds_get_object_path(att));

        return(0);
    }

    /* Set the value in the attribute structure */

    if (att->value.vp) {
        free(att->value.vp);
    }

    att->length   = length;
    att->value.vp = new_value;

    return(1);
}

/**
 *  Set the value of a CDS attribute.
 *
 *  The cds_string_to_array() function will be used to set the attribute
 *  value if the data type of the attribute is not CDS_CHAR.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  att    - pointer to the attribute
 *  @param  format - format string (see printf)
 *  @param  args   - arguments for the format string
 *
 *  @return
 *    - 1 if successful
 *    - 0 if a memory allocation error occurred
 */
int _cds_set_att_va_list(
    CDSAtt     *att,
    const char *format,
    va_list     args)
{
    char   *string;
    size_t  length;
    int     retval;

    string = msngr_format_va_list(format, args);

    if (!string) {

        ERROR( CDS_LIB_NAME,
            "Could not set attribute value for: %s\n"
            " -> memory allocation error\n", cds_get_object_path(att));

        return(0);
    }

    length = strlen(string) + 1;
    retval = _cds_set_att_value(att, CDS_CHAR, length, string);

    free(string);

    return(retval);
}

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Change an attribute of a CDS group or variable.
 *
 *  This function will define the specified attribute if it does not exist.
 *  If the attribute does exist and the overwrite flag is set, the data type
 *  and value will be changed.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  parent    - pointer to the parent CDSGroup or CDSVar
 *  @param  overwrite - overwrite flag (1 = TRUE, 0 = FALSE)
 *  @param  name      - attribute name
 *  @param  type      - attribute data type
 *  @param  length    - attribute length
 *  @param  value     - pointer to the attribute value
 *
 *  @return
 *    - pointer to the attribute
 *    - NULL if:
 *        - the parent object is not a group or variable
 *        - the parent group or variable is locked, or the attribute is locked
 *        - a memory allocation error occurred
 */
CDSAtt *cds_change_att(
    void        *parent,
    int          overwrite,
    const char  *name,
    CDSDataType  type,
    size_t       length,
    void        *value)
{
    CDSAtt *att;

    att = cds_get_att(parent, name);
    if (!att) {
        att = _cds_define_att(parent, name, type, length, value);
        if (!att) {
            return((CDSAtt *)NULL);
        }
    }
    else if (!att->length || !att->value.vp || overwrite) {
        if (!cds_change_att_value(att, type, length, value)) {
            return((CDSAtt *)NULL);
        }
    }

    return(att);
}

/**
 *  Change the type and value of a CDS Attribute.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  att     - pointer to the attribute
 *  @param  type    - new attribute data type
 *  @param  length  - new attribute length
 *  @param  value   - pointer to the new attribute value
 *
 *  @return
 *    - 1 if successful
 *    - 0 if:
 *        - the attribute is locked
 *        - a memory allocation error occurred
 */
int cds_change_att_value(
    CDSAtt      *att,
    CDSDataType  type,
    size_t       length,
    void        *value)
{
    /* Check if the attribute is locked */

    if (att->def_lock) {

        ERROR( CDS_LIB_NAME,
            "Could not change attribute value for: %s\n"
            " -> the attribute definition lock is set to: %d\n",
            cds_get_object_path(att), att->def_lock);

        return(0);
    }

    /* Change the attribute value */

    if (!_cds_change_att_value(att, type, length, value)) {

        ERROR( CDS_LIB_NAME,
            "Could not change attribute value for: %s\n"
            " -> memory allocation error\n", cds_get_object_path(att));

        return(0);
    }

    return(1);
}

/**
 *  Change the type and value of a CDS Attribute.
 *
 *  This function will change the data type of an attribute to CDS_CHAR
 *  and set the new value.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  att     - pointer to the attribute
 *  @param  format  - format string (see printf)
 *  @param  ...     - arguments for the format string
 *
 *  @return
 *    - 1 if the attribute value was changed
 *    - 0 if:
 *        - the attribute is locked
 *        - a memory allocation error occurred
 */
int cds_change_att_text(
    CDSAtt     *att,
    const char *format, ...)
{
    va_list args;
    int     retval;

    /* Check if a format string was specified */

    if (!format) {

        if (att->value.vp) free(att->value.vp);

        att->type     = CDS_CHAR;
        att->length   = 0;
        att->value.vp = (void *)NULL;

        return(1);
    }

    /* Change the attribute value */

    va_start(args, format);
    retval = cds_change_att_va_list(att, format, args);
    va_end(args);

    return(retval);
}

/**
 *  Change the type and value of a CDS Attribute.
 *
 *  This function will change the data type of an attribute to CDS_CHAR
 *  and set the new value.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  att    - pointer to the attribute
 *  @param  format - format string (see printf)
 *  @param  args   - arguments for the format string
 *
 *  @return
 *    - 1 if the attribute value was changed
 *    - 0 if:
 *        - the attribute is locked
 *        - a memory allocation error occurred
 */
int cds_change_att_va_list(
    CDSAtt     *att,
    const char *format,
    va_list     args)
{
    char *att_text;

    /* Check if the attribute is locked */

    if (att->def_lock) {

        ERROR( CDS_LIB_NAME,
            "Could not change attribute value for: %s\n"
            " -> the attribute definition lock is set to: %d\n",
            cds_get_object_path(att), att->def_lock);

        return(0);
    }

    /* Check if a format string was specified */

    if (!format) {

        if (att->value.vp) free(att->value.vp);

        att->type     = CDS_CHAR;
        att->length   = 0;
        att->value.vp = (void *)NULL;

        return(1);
    }

    /* Create text string from format string and argument list */

    att_text = msngr_format_va_list(format, args);

    if (!att_text) {

        ERROR( CDS_LIB_NAME,
            "Could not change attribute value for: %s\n"
            " -> memory allocation error\n", cds_get_object_path(att));

        return(0);
    }

    /* Change the attribute value */

    if (att->value.vp) free(att->value.vp);

    att->type     = CDS_CHAR;
    att->length   = strlen(att_text) + 1;
    att->value.cp = att_text;

    return(1);
}

/**
 *  Create a missing value attribute if it does not already exist.
 *
 *  This function will check if the variable already has either a
 *  missing_value or _FillValue attibute defined.  If it does not
 *  one will be created.
 *
 *  The missing value used will be determined by first checking for
 *  non-standard missing value attributes defined at either the field
 *  or global level (see cds_is_missing_value_att_name()).  If no
 *  known missing value attributes are found the default value for
 *  the variables data type will be used.
 *
 *  @param var   - pointer to the variable
 *  @param flags - reserved for control flags
 *
 *  @retval  1  if the missing value attribute already existed or was created.
 *  @retval  0  if a fatal error occurred
 */
int cds_create_missing_value_att(CDSVar *var, int flags)
{
    int   nmissings;
    void *missings;
    int   free_missings;

    flags = flags;

    free_missings = 0;

    if (cds_get_att(var, "missing_value") ||
        cds_get_att(var, "_FillValue")) {

        /* The variable already has a standard
         * missing value attribute defined */

        return(1);
    }

    nmissings = cds_get_var_missing_values(var, &missings);
    if (nmissings < 0) return(0);

    if (nmissings == 0) {
        /* Use the default fill value for the variable data type */
        nmissings = 1;
        missings  = _cds_default_fill_value(var->type);
    }
    else {

        free_missings = 1;

        /* Remove the default fill value from the missing values
         * array if more than 1 missing value was found. */

        if (var->default_fill && nmissings > 1) {
            nmissings -= 1;
        }
    }

    if (!cds_define_att(
        var, "missing_value", var->type, nmissings, missings)) {

        if (free_missings) free(missings);
        return(0);
    }

    if (free_missings) free(missings);

    return(1);
}

/**
 *  Define a CDS Attribute.
 *
 *  This function will first check if an attribute with the same
 *  definition already exists in the specified group or variable.
 *  If it does, the existing attribute will be returned.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  parent   - pointer to the parent CDSGroup or CDSVar
 *  @param  name     - attribute name
 *  @param  type     - attribute data type
 *  @param  length   - attribute length
 *  @param  value    - pointer to the attribute value
 *
 *  @return
 *    - pointer to the attribute
 *    - NULL if:
 *        - the parent object is not a group or variable
 *        - the parent group or variable is locked
 *        - a memory allocation error occurred
 *        - an attribute with the same name but different definition
 *          has already been defined
 */
CDSAtt *cds_define_att(
    void        *parent,
    const char  *name,
    CDSDataType  type,
    size_t       length,
    void        *value)
{
    CDSAtt *att;
    size_t  type_size;

    /* Check if an attribute with this name already exists */

    att = cds_get_att(parent, name);
    if (att) {

        if (type   == att->type  &&
            length == att->length) {

            if (value == (void *)NULL || att->value.vp == (void *)NULL) {

                if (value == (void *)NULL && att->value.vp == (void *)NULL) {
                    return(att);
                }
            }
            else {

                type_size = cds_data_type_size(type);

                if (memcmp(value, att->value.vp, length * type_size) == 0) {
                    return(att);
                }
            }
        }

        ERROR( CDS_LIB_NAME,
            "Could not define attribute: %s\n"
            " -> attribute exists\n", cds_get_object_path(att));

        return((CDSAtt *)NULL);
    }

    return(_cds_define_att(parent, name, type, length, value));
}

/**
 *  Define a CDS Text Attribute.
 *
 *  See cds_define_att() for details.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  parent   - pointer to the parent CDSGroup or CDSVar
 *  @param  name     - attribute name
 *  @param  format   - format string (see printf)
 *  @param  ...      - arguments for the format string
 *
 *  @return
 *    - pointer to the new attribute
 *    - NULL if:
 *        - the parent object is not a group or variable
 *        - the parent group or variable is locked
 *        - a memory allocation error occurred
 *        - an attribute with the same name but a differnt value
 *          has already been defined
 */
CDSAtt *cds_define_att_text(
    void       *parent,
    const char *name,
    const char *format, ...)
{
    va_list  args;
    CDSAtt  *att;

    va_start(args, format);
    att = cds_define_att_va_list(parent, name, format, args);
    va_end(args);

    return(att);
}

/**
 *  Define a CDS Text Attribute.
 *
 *  See cds_define_att() for details.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  parent   - pointer to the parent CDSGroup or CDSVar
 *  @param  name     - attribute name
 *  @param  format   - format string (see printf)
 *  @param  args     - arguments for the format string
 *
 *  @return
 *    - pointer to the new attribute
 *    - NULL if:
 *        - the parent object is not a group or variable
 *        - the parent group or variable is locked
 *        - a memory allocation error occurred
 *        - an attribute with the same name but a differnt value
 *          has already been defined
 */
CDSAtt *cds_define_att_va_list(
    void       *parent,
    const char *name,
    const char *format,
    va_list     args)
{
    char   *att_text;
    size_t  att_length;
    CDSAtt *att;

    /* Create text string from format string and argument list */

    att_text = msngr_format_va_list(format, args);

    if (!att_text) {

        ERROR( CDS_LIB_NAME,
            "Could not define attribute: %s/_atts_/%s\n"
            " -> memory allocation error\n",
            cds_get_object_path(parent), name);

        free(att_text);
        return((CDSAtt *)NULL);
    }

    att_length = strlen(att_text) + 1;

    att = cds_define_att(parent, name, CDS_CHAR, att_length, att_text);

    free(att_text);

    return(att);
}

/**
 *  Delete a CDS Attribute.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  att - pointer to the attribute
 *
 *  @return
 *    - 1 if the attribute was deleted
 *    - 0 if:
 *        - the attribute is locked
 *        - the parent group or variable is locked
 */
int cds_delete_att(CDSAtt *att)
{
    CDSObject *parent = att->parent;
    CDSGroup  *group;
    CDSVar    *var;

    /* Check if the attribute is locked */

    if (att->def_lock) {

        ERROR( CDS_LIB_NAME,
            "Could not delete attribute: %s\n"
            " -> the attribute definition lock is set to: %d\n",
            cds_get_object_path(att), att->def_lock);

        return(0);
    }

    /* Check if the parent is a group or variable */

    if (parent->obj_type == CDS_GROUP) {

        group = (CDSGroup *)att->parent;

        /* Check if the group is locked */

        if (group->def_lock) {

            ERROR( CDS_LIB_NAME,
                "Could not delete attribute: %s\n"
                " -> the group definition lock is set to: %d\n",
                cds_get_object_path(att), group->def_lock);

            return(0);
        }

        /* Remove this attribute from the parent object */

        _cds_remove_object((void **)group->atts, &(group->natts), att);
    }
    else if (parent->obj_type == CDS_VAR) {

        var = (CDSVar *)att->parent;

        /* Check if the variable is locked */

        if (var->def_lock) {

            ERROR( CDS_LIB_NAME,
                "Could not delete attribute: %s\n"
                " -> the variable definition lock is set to: %d\n",
                cds_get_object_path(att), var->def_lock);

            return(0);
        }

        /* Remove this attribute from the parent object */

        _cds_remove_object((void **)var->atts, &(var->natts), att);
    }
    else {

        ERROR( CDS_LIB_NAME,
            "Could not delete attribute: %s\n"
            " -> parent object must be a group or variable\n",
            cds_get_object_path(att));

        return(0);
    }

    /* Destroy the attribute */

    _cds_destroy_att(att);

    return(1);
}

/**
 *  Get a CDS Attribute.
 *
 *  This function will search the specified parent group
 *  or variable for an attribute with the specified name.
 *
 *  @param  parent - pointer to the parent group or variable
 *  @param  name   - name of the attribute
 *
 *  @return
 *    - pointer to the attribute
 *    - NULL if not found
 */
CDSAtt *cds_get_att(void *parent, const char *name)
{
    CDSObject *parent_object = (CDSObject *)parent;
    CDSAtt    *att           = (CDSAtt *)NULL;
    CDSGroup  *group;
    CDSVar    *var;

    if (parent_object->obj_type == CDS_GROUP) {
        group = (CDSGroup *)parent;
        att   = _cds_get_object((void **)group->atts, group->natts, name);
    }
    else if (parent_object->obj_type == CDS_VAR) {
        var = (CDSVar *)parent;
        att = _cds_get_object((void **)var->atts, var->natts, name);
    }

    return(att);
}

/**
 *  Get a copy of a CDS attribute value.
 *
 *  This function will get a copy of an attribute value casted into
 *  the specified data type. The functions cds_string_to_array() and
 *  cds_array_to_string() are used to convert between text (CDS_CHAR)
 *  and numeric data types.
 *
 *  Memory will be allocated for the returned array if the output array
 *  is NULL. In this case the calling process is responsible for freeing
 *  the allocated memory.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  att    - pointer to the attribute
 *  @param  type   - data type of the output array
 *  @param  length - pointer to the length of the output array
 *                     - input:
 *                         - length of the output array
 *                         - ignored if the output array is NULL
 *                     - output:
 *                         - number of values written to the output array
 *                         - 0 if the attribute value has zero length
 *                         - (size_t)-1 if a memory allocation error occurs
 *  @param  value  - pointer to the output array
 *                   or NULL to dynamically allocate the memory needed.
 *
 *  @return
 *    - pointer to the output array
 *    - NULL if:
 *        - the attribute value has zero length (length == 0)
 *        - a memory allocation error occurs (length == (size_t)-1)
 */
void *cds_get_att_value(
    CDSAtt       *att,
    CDSDataType   type,
    size_t       *length,
    void         *value)
{
    size_t  out_length;
    void   *out_min;
    void   *out_max;
    void   *out_fill;

    /* Check if the attribute has a value defined */

    if (!att->length || !att->value.vp) {
        if (length) *length = 0;
        return((void *)NULL);
    }

    /* Get attribute value casted to the requested data type */

    out_length = att->length;

    if (value && length && *length > 0) {
        if (out_length > *length) {
            out_length = *length;
        }
    }

    if (type == CDS_CHAR) {

        if (att->type == CDS_CHAR) {

            if (!value) {

                value = calloc(out_length + 1, sizeof(char));

                if (!value) {
                    out_length = (size_t)-1;
                }
            }

            if (value) {
                memcpy(value, att->value.vp, out_length * sizeof(char));
            }
        }
        else {
            value = (void *)cds_array_to_string(
                att->type, att->length, att->value.vp, &out_length, value);
        }
    }
    else if (att->type == CDS_CHAR) {

        if (cds_is_missing_value_att_name(att->name)) {
            value = cds_string_to_array_use_fill(
                att->value.cp, type, &out_length, value);
        }
        else {
            value = cds_string_to_array(
                att->value.cp, type, &out_length, value);
        }
    }
    else {

        if (type < att->type) {

            out_fill = _cds_default_fill_value(type);
            out_min  = _cds_data_type_min(type);
            out_max  = _cds_data_type_max(type);

            if (cds_is_missing_value_att_name(att->name)) {
                value = cds_copy_array(
                    att->type, out_length, att->value.vp, type, value,
                    0, NULL, NULL, out_min, out_fill, out_max, out_fill);
            }
            else {
                value = cds_copy_array(
                    att->type, out_length, att->value.vp, type, value,
                    0, NULL, NULL, out_min, out_min, out_max, out_max);
            }
        }
        else {
            value = cds_copy_array(
                att->type, out_length, att->value.vp, type, value,
                0, NULL, NULL, NULL, NULL, NULL, NULL);
        }

        if (!value) {
            out_length = (size_t)-1;
        }
    }

    if (length) *length = out_length;

    if (out_length == (size_t)-1) {

        ERROR( CDS_LIB_NAME,
            "Could not get attribute value for: %s\n"
            " -> memory allocation error\n", cds_get_object_path(att));

        return((void *)NULL);
    }

    return(value);
}

/**
 *  Get a copy of a CDS attribute value.
 *
 *  This function will get a copy of an attribute value converted
 *  to a text string. If the data type of the attribute is not
 *  CDS_CHAR the cds_array_to_string() function is used to create
 *  the output string.
 *
 *  Memory will be allocated for the returned string if the output string
 *  is NULL. In this case the calling process is responsible for freeing
 *  the allocated memory.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  att    - pointer to the attribute
 *  @param  length - pointer to the length of the output string
 *                     - input:
 *                         - length of the output string
 *                         - ignored if the output string is NULL
 *                     - output:
 *                         - number of characters written to the output string
 *                         - 0 if the attribute value has zero length
 *                         - (size_t)-1 if a memory allocation error occurs
 *  @param  value  - pointer to the output string
 *                   or NULL to dynamically allocate the memory needed.
 *
 *  @return
 *    - pointer to the output string
 *    - NULL if:
 *        - the attribute value has zero length (length == 0)
 *        - a memory allocation error occurs (length == (size_t)-1)
 */
char *cds_get_att_text(
    CDSAtt *att,
    size_t *length,
    char   *value)
{
    return((char *)cds_get_att_value(att, CDS_CHAR, length, value));
}

/**
 *  Rename a CDS Attribute.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  att  - pointer to the attribute
 *  @param  name - pointer to the new attribute name
 *
 *  @return
 *    - 1 if the attribute was deleted
 *    - 0 if:
 *        - an attribute with the new name already exists
 *        - the attribute is locked
 *        - the parent group or variable is locked
 *        - a memory allocation error occured
 */
int cds_rename_att(CDSAtt *att, const char *name)
{
    CDSObject *parent = att->parent;
    CDSGroup  *group;
    CDSVar    *var;
    char      *new_name;

    /* Check if an attribute with the new name already exists */

    if (cds_get_att(parent, name)) {

        ERROR( CDS_LIB_NAME,
            "Could not rename attribute: %s to %s\n"
            " -> attribute exists\n",
            cds_get_object_path(att), name);

        return(0);
    }

    /* Check if the attribute is locked */

    if (att->def_lock) {

        ERROR( CDS_LIB_NAME,
            "Could not rename attribute: %s to %s\n"
            " -> the attribute definition lock is set to: %d\n",
            cds_get_object_path(att), name, att->def_lock);

        return(0);
    }

    /* Check if the parent is a group or variable */

    if (parent->obj_type == CDS_GROUP) {

        group = (CDSGroup *)att->parent;

        /* Check if the group is locked */

        if (group->def_lock) {

            ERROR( CDS_LIB_NAME,
                "Could not rename attribute: %s to %s\n"
                " -> the group definition lock is set to: %d\n",
                cds_get_object_path(att), name, group->def_lock);

            return(0);
        }
    }
    else if (parent->obj_type == CDS_VAR) {

        var = (CDSVar *)att->parent;

        /* Check if the variable is locked */

        if (var->def_lock) {

            ERROR( CDS_LIB_NAME,
                "Could not rename attribute: %s to %s\n"
                " -> the variable definition lock is set to: %d\n",
                cds_get_object_path(att), name, var->def_lock);

            return(0);
        }
    }
    else {

        ERROR( CDS_LIB_NAME,
            "Could not rename attribute: %s to %s\n"
            " -> parent object must be a group or variable\n",
            cds_get_object_path(att), name);

        return(0);
    }

    /* Rename the attribute */

    new_name = strdup(name);
    if (!new_name) {

        ERROR( CDS_LIB_NAME,
            "Could not rename attribute: %s\n"
            " -> memory allocation error\n",
            cds_get_object_path(att));

        return(0);
    }

    free(att->name);
    att->name = new_name;

    return(1);
}

/**
 *  Set an attribute of a CDS group or variable.
 *
 *  This function will define the specified attribute if it does not exist.
 *  If the attribute does exist and the overwrite flag is set, the value will
 *  be set by casting the specified value into the data type of the attribute.
 *  The functions cds_string_to_array() and cds_array_to_string() are used to
 *  convert between text (CDS_CHAR) and numeric data types.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  parent    - pointer to the parent CDSGroup or CDSVar
 *  @param  overwrite - overwrite flag (1 = TRUE, 0 = FALSE)
 *  @param  name      - attribute name
 *  @param  type      - attribute data type
 *  @param  length    - attribute length
 *  @param  value     - pointer to the attribute value
 *
 *  @return
 *    - pointer to the attribute
 *    - NULL if:
 *        - the parent object is not a group or variable
 *        - the parent group or variable is locked, or the attribute is locked
 *        - a memory allocation error occurred
 */
CDSAtt *cds_set_att(
    void        *parent,
    int          overwrite,
    const char  *name,
    CDSDataType  type,
    size_t       length,
    void        *value)
{
    CDSAtt *att;

    att = cds_get_att(parent, name);
    if (!att) {
        att = _cds_define_att(parent, name, type, length, value);
        if (!att) {
            return((CDSAtt *)NULL);
        }
    }
    else if (!att->length || !att->value.vp || overwrite) {
        if (!cds_set_att_value(att, type, length, value)) {
            return((CDSAtt *)NULL);
        }
    }

    return(att);
}

/**
 *  Set the value of a CDS attribute.
 *
 *  This function will set the value of an attribute by casting the
 *  specified value into the data type of the attribute. The functions
 *  cds_string_to_array() and cds_array_to_string() are used to convert
 *  between text (CDS_CHAR) and numeric data types.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  att    - pointer to the attribute
 *  @param  type   - data type of the specified value
 *  @param  length - length of the specified value
 *  @param  value  - pointer to the attribute value
 *
 *  @return
 *    - 1 if successful
 *    - 0 if:
 *        - the attribute is locked
 *        - a memory allocation error occurred
 */
int cds_set_att_value(
    CDSAtt      *att,
    CDSDataType  type,
    size_t       length,
    void        *value)
{
    if (att->def_lock) {

        ERROR( CDS_LIB_NAME,
            "Could not set attribute value for: %s\n"
            " -> the attribute definition lock is set to: %d\n",
            cds_get_object_path(att), att->def_lock);

        return(0);
    }

    return(_cds_set_att_value(att, type, length, value));
}

/**
 *  Set the value of a CDS attribute.
 *
 *  The cds_string_to_array() function will be used to set the attribute
 *  value if the data type of the attribute is not CDS_CHAR.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  att    - pointer to the attribute
 *  @param  format - format string (see printf)
 *  @param  ...    - arguments for the format string
 *
 *  @return
 *    - 1 if successful
 *    - 0 if:
 *        - the attribute is locked
 *        - a memory allocation error occurred
 */
int cds_set_att_text(
    CDSAtt     *att,
    const char *format, ...)
{
    va_list args;
    int     retval;

    /* Check if the attribute is locked */

    if (att->def_lock) {

        ERROR( CDS_LIB_NAME,
            "Could not change attribute value for: %s\n"
            " -> the attribute definition lock is set to: %d\n",
            cds_get_object_path(att), att->def_lock);

        return(0);
    }

    /* Check if a format string was specified */

    if (!format) {

        if (att->value.vp) {
            free(att->value.vp);
        }

        att->length   = 0;
        att->value.vp = (void *)NULL;

        return(1);
    }

    va_start(args, format);
    retval = _cds_set_att_va_list(att, format, args);
    va_end(args);

    return(retval);
}

/**
 *  Set the value of a CDS attribute.
 *
 *  The cds_string_to_array() function will be used to set the attribute
 *  value if the data type of the attribute is not CDS_CHAR.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  att    - pointer to the attribute
 *  @param  format - format string (see printf)
 *  @param  args   - arguments for the format string
 *
 *  @return
 *    - 1 if successful
 *    - 0 if:
 *        - the attribute is locked
 *        - a memory allocation error occurred
 */
int cds_set_att_va_list(
    CDSAtt     *att,
    const char *format,
    va_list     args)
{
    /* Check if the attribute is locked */

    if (att->def_lock) {

        ERROR( CDS_LIB_NAME,
            "Could not change attribute value for: %s\n"
            " -> the attribute definition lock is set to: %d\n",
            cds_get_object_path(att), att->def_lock);

        return(0);
    }

    /* Check if a format string was specified */

    if (!format) {

        if (att->value.vp) {
            free(att->value.vp);
        }

        att->length   = 0;
        att->value.vp = (void *)NULL;

        return(1);
    }

    return(_cds_set_att_va_list(att, format, args));
}
