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
*     email: brian.ermold@pnnl.gov
*
*******************************************************************************/

/** @file cds_transform_params.c
 *  CDS Transformation Parameters.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "cds3.h"
#include "cds_private.h"

/*******************************************************************************
 *  Private Functions
 */
/** @privatesection */

/**
 *  PRIVATE: Free the memory used by the members of a CDSParam.
 *
 *  @param  param - pointer to the CDSParam
 */
void _cds_free_param_members(CDSParam *param)
{
    if (param) {
        if (param->name)     free(param->name);
        if (param->value.vp) free(param->value.vp);
    }
}

/**
 *  PRIVATE: Free the memory used by the members of a CDSParamList.
 *
 *  @param  list - pointer to the CDSParamList
 */
void _cds_free_param_list_members(CDSParamList *list)
{
    int pi;

    if (list) {
        if (list->name) free(list->name);
        if (list->params) {
            for (pi = 0; pi < list->nparams; pi++) {
                _cds_free_param_members(&(list->params[pi]));
            }
            free(list->params);
        }
    }
}

/**
 *  PRIVATE: Free the memory used by a CDSTransformParams Structure.
 *
 *  @param  transform_params - pointer to the CDSTransformParams Structure
 */
void _cds_free_transform_params(CDSTransformParams *transform_params)
{
    int li;

    if (transform_params) {
        if (transform_params->lists) {
            for (li = 0; li < transform_params->nlists; li++) {
                _cds_free_param_list_members(&(transform_params->lists[li]));
            }
            free(transform_params->lists);
        }
        free(transform_params);
    }
}

/**
 *  PRIVATE: Get the parameter for the specified name.
 *
 *  @param  list - pointer to the parameter list
 *  @param  name - the name of the parameter to get
 *
 *  @return
 *    - pointer to the requested parameter
 *    - NULL if not found
 */
CDSParam *_cds_get_param(
    CDSParamList *list,
    const char   *name)
{
    int pi;

    if (list) {
        for (pi = 0; pi < list->nparams; pi++) {
            if (strcmp(list->params[pi].name, name) == 0) {
                return(&(list->params[pi]));
            }
        }
    }

    return((CDSParam *)NULL);
}

/**
 *  PRIVATE: Initialize a parameter.
 *
 *  @param  param - pointer to the parameter
 *  @param  name  - the name of the parameter
 *
 *  @return
 *    - 1 if successful
 *    - 0 if a memory allocation error occurred
 */
int _cds_init_param(
    CDSParam   *param,
    const char *name)
{
    memset((void *)param, 0, sizeof(CDSParam));

    param->name = strdup(name);
    if (!param->name) {
        return(0);
    }

    return(1);
}

/**
 *  PRIVATE: Get the parameter list for the specified name.
 *
 *  @param  nlists - number of lists in the array of parameter lists
 *  @param  lists  - pointer to the array of parameter lists
 *  @param  name   - the name of the list to get
 *
 *  @return
 *    - pointer to the requested parameter list
 *    - NULL if not found
 */
CDSParamList *_cds_get_param_list(
    int           nlists,
    CDSParamList *lists,
    const char   *name)
{
    int li;

    if (lists) {
        for (li = 0; li < nlists; li++) {
            if (strcmp(lists[li].name, name) == 0) {
                return(&(lists[li]));
            }
        }
    }

    return((CDSParamList *)NULL);
}

/**
 *  PRIVATE: Initialize a parameter list.
 *
 *  @param  list - pointer to the parameter list
 *  @param  name - the name of the parameter list
 *
 *  @return
 *    - 1 if successful
 *    - 0 if a memory allocation error occurred
 */
int _cds_init_param_list(
    CDSParamList *list,
    const char   *name)
{
    memset((void *)list, 0, sizeof(CDSParamList));

    list->name = strdup(name);
    if (!list->name) {
        return(0);
    }

    return(1);
}

/**
 *  PRIVATE: Set the value of a parameter.
 *
 *  @param  list   - pointer to the parameter list
 *  @param  name   - name of the parameter
 *  @param  type   - data type of the parameter value
 *  @param  length - length of the parameter value
 *  @param  value  - pointer to the parameter value
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurs
 */
int _cds_set_param(
    CDSParamList *list,
    const char   *name,
    CDSDataType   type,
    size_t        length,
    void         *value)
{
    CDSParam *param;
    CDSParam *new_params;
    int       new_nalloced;
    size_t    type_size;
    void     *new_value;

    if (type == CDS_STRING) {
        ERROR( CDS_LIB_NAME,
            "Could not set transformation parameter for: %s\n"
            " -> unsupported data type: %s\n",
            name, cds_data_type_name(type));
        return(0);
    }

    /* Get the parameter from the list or create it if it doesn't exist */

    param = _cds_get_param(list, name);

    if (!param) {

        if (list->nparams == list->nalloced) {

            new_nalloced = list->nalloced + 8;
            new_params   = (CDSParam *)realloc(
                list->params, new_nalloced * sizeof(CDSParam));

            if (!new_params) {
                return(0);
            }

            list->nalloced = new_nalloced;
            list->params   = new_params;
        }

        param = &(list->params[list->nparams]);

        if (!_cds_init_param(param, name)) {
            return(0);
        }

        list->nparams++;
    }

    /* Create the new value for this parameter */

    if (length) {

        type_size = cds_data_type_size(type);
        new_value = calloc(length+1, type_size);

        if (!new_value) {
            return(0);
        }

        if (value) {
            memcpy(new_value, value, length * type_size);
        }
    }
    else {
        new_value = (void *)NULL;
    }

    /* Set the new value for this parameter */

    if (param->value.vp) {
        free(param->value.vp);
    }

    param->type     = type;
    param->length   = length;
    param->value.vp = new_value;

    return(1);
}

/**
 *  PRIVATE: Parse a transformation parameter token from a text string.
 *
 *  @param  cp        - pointer to the current location in the text string
 *  @param  delim     - the delimiter we are looking for
 *  @param  errchars  - string of characters that indicate a format error
 *  @param  nlines    - pointer to the line counter variable
 *  @param  token     - output: pointer to the token string
 *  @param  endp      - output: pointer to the next character after the delimiter
 *
 *  @return
 *    -  1 if successful
 *    -  0 if the end of the string was reached before the token was found
 *    - -1 if a format error occurred
 */
int _cds_get_token(
    char        *cp,
    const char   delim,
    const char  *errchars,
    int         *nlines,
    char       **token,
    char       **endp)
{
    if (!cp || !(*cp)) {
        *token = cp;
        *endp  = cp;
        return(0);
    }

    /* skip white space */

    while (isspace(*cp)) {
        if (*cp == '\n') (*nlines)++;
        cp++;
    }

    /* find the specified delimiter */

    *token = cp;

    if (!(*cp)) {
        *endp = cp;
        return(0);
    }

    if (*cp == delim) {

        ERROR( CDS_LIB_NAME,
            "Invalid format on line %d in transformation parameters string\n"
            " -> empty string found before delimiter '%c'\n",
            *nlines, delim);

        *endp = cp + 1;
        return(-1);
    }

    cp++;

    while (*cp && *cp != delim) {

        if (errchars && strchr(errchars, *cp) != NULL) {

            ERROR( CDS_LIB_NAME,
                "Invalid format on line %d in transformation parameters string\n"
                " -> expected delimiter '%c' but found '%c'\n",
                *nlines, delim, *cp);

            *endp = cp;
            return(-1);
        }

        if (*cp == '\n') (*nlines)++;
        cp++;
    }

    if (!(*cp) && delim != '\n') {

        ERROR( CDS_LIB_NAME,
            "Invalid format on line %d in transformation parameters string\n"
            " -> expected delimiter '%c' but found end of string\n",
            *nlines, delim);

        *endp = cp;
        return(-1);
    }

    *endp = cp + 1;

    /* trim trailing spaces from the token */

    for (cp--; isspace(*cp); cp--);

    *(++cp) = '\0';

    return(1);
}

/**
 *  PRIVATE: Print parameters list.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  fp     - pointer to the open file to print to
 *  @param  indent - line indent string
 *  @param  list   - pointer to the CDSParamList
 *
 *  @return
 *    - number of bytes printed
 *    - negative value if an error occurred
 */
int _cds_print_param_list(
    FILE         *fp,
    const char   *indent,
    CDSParamList *list)
{
    int       nbytes;
    int       tbytes;
    int       length;
    int       min_width;
    char      format[64];
    CDSParam *param;
    int       line_length;
    int       str_length;
    char      str_value[32];
    int       pi;
    size_t    vi;

    tbytes = 0;

    if (!list) return(0);

    min_width = 0;

    for (pi = 0; pi < list->nparams; pi++) {
        length = strlen(list->params[pi].name);
        if (min_width < length) {
            min_width = length;
        }
    }

    sprintf(format, "%%s%%s:%%-%ds = ", min_width);

    for (pi = 0; pi < list->nparams; pi++) {

        param = &(list->params[pi]);

        nbytes = fprintf(fp, format, indent, list->name, param->name);
        if (nbytes < 0) return(nbytes);
        tbytes += nbytes;

        if (param->type == CDS_CHAR) {

            nbytes = fprintf(fp, "%s", param->value.cp);
            if (nbytes < 0) return(nbytes);
            tbytes += nbytes;
        }
        else {

            line_length = nbytes;

            for (vi = 0; vi < param->length; vi++) {

                switch (param->type) {
                    case CDS_BYTE:
                        str_length = sprintf(str_value, "%hhd", param->value.bp[vi]);
                        break;
                    case CDS_SHORT:
                        str_length = sprintf(str_value, "%hd", param->value.sp[vi]);
                        break;
                    case CDS_INT:
                        str_length = sprintf(str_value, "%d", param->value.ip[vi]);
                        break;
                    case CDS_FLOAT:
                        str_length = sprintf(str_value, "%.7g", param->value.fp[vi]);
                        break;
                    case CDS_DOUBLE:
                        str_length = sprintf(str_value, "%.15g", param->value.dp[vi]);
                        break;
                    /* NetCDF4 extended data types */
                    case CDS_INT64:
                        str_length = sprintf(str_value, "%lld", param->value.i64p[vi]);
                        break;
                    case CDS_UBYTE:
                        str_length = sprintf(str_value, "%hhu", param->value.ubp[vi]);
                        break;
                    case CDS_USHORT:
                        str_length = sprintf(str_value, "%hu", param->value.usp[vi]);
                        break;
                    case CDS_UINT:
                        str_length = sprintf(str_value, "%u", param->value.uip[vi]);
                        break;
                    case CDS_UINT64:
                        str_length = sprintf(str_value, "%llu", param->value.ui64p[vi]);
                        break;
                    default:
                        str_length = sprintf(str_value, "NaT");
                        break;
                }

                if (vi == 0) {
                    nbytes = fprintf(fp, "%s", str_value);
                    line_length += str_length;
                }
                else if ((line_length + str_length + 4) > 80) {
                    nbytes = fprintf(fp, ",\n%s    %s", indent, str_value);
                    line_length = strlen(indent) + str_length + 4;
                }
                else {
                    nbytes = fprintf(fp, ", %s", str_value);
                    line_length += str_length + 2;
                }

                if (nbytes < 0) return(nbytes);
                tbytes += nbytes;
            }
        }

        nbytes = fprintf(fp, ";\n");
        if (nbytes < 0) return(nbytes);
        tbytes += nbytes;
    }

    return(tbytes);
}

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Copy transformation parameters from one group to another.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  src_group  - pointer to the source CDSGroup
 *  @param  dest_group - pointer to the destination CDSGroup
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurs
 */
int cds_copy_transform_params(
    CDSGroup    *src_group,
    CDSGroup    *dest_group)
{
    CDSTransformParams *tp;
    CDSParamList       *list;
    CDSParam           *param;
    int                 status;
    int                 li, pi;

    if (!src_group || !dest_group ||
        !src_group->transform_params) {

        return(1);
    }

    tp = (CDSTransformParams *)src_group->transform_params;

    /* Loop over the parameter lists */

    for (li = 0; li < tp->nlists; ++li) {

        list = &(tp->lists[li]);

        /* Loop over the parameters in the list */

        for (pi = 0; pi < list->nparams; ++pi) {

            param = &(list->params[pi]);


            status = cds_set_transform_param(
                dest_group,
                list->name,
                param->name,
                param->type,
                param->length,
                param->value.vp);

            if (!status) return(0);
        }
    }

    return(1);
}

/**
 *  Set the value of a transformation parameter.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  group      - pointer to the CDSGroup
 *  @param  obj_name   - name of the CDS object
 *  @param  param_name - name of the parameter
 *  @param  type       - data type of the specified value
 *  @param  length     - length of the specified value
 *  @param  value      - pointer to the parameter value
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurs
 */
int cds_set_transform_param(
    CDSGroup    *group,
    const char  *obj_name,
    const char  *param_name,
    CDSDataType  type,
    size_t       length,
    void        *value)
{
    CDSTransformParams *tp;
    CDSParamList       *list;
    CDSParamList       *new_lists;
    int                 new_nalloced;
    int                 status;

    if (type == CDS_STRING) {
        ERROR( CDS_LIB_NAME,
            "Could not set transformation parameter for: %s:%s\n"
            " -> unsupported data type: %s\n",
            obj_name, param_name, cds_data_type_name(type));
        return(0);
    }

    /* Create the transformation parameters structure if
     * it does not already exist */

    if (!group->transform_params) {
        group->transform_params = calloc(1, sizeof(CDSTransformParams));
        if (!group->transform_params) {

            ERROR( CDS_LIB_NAME,
                "Could not set transformation parameter: %s:%s\n"
                " -> memory allocation error\n", obj_name, param_name);

            return(0);
        }
    }

    tp = (CDSTransformParams *)group->transform_params;

    /* Get the parameter list for the specified object name
     * or create it if it doesn't exist */

    list = _cds_get_param_list(tp->nlists, tp->lists, obj_name);

    if (!list) {

        if (tp->nlists == tp->nalloced) {

            new_nalloced = tp->nalloced + 8;
            new_lists    = (CDSParamList *)realloc(
                tp->lists, new_nalloced * sizeof(CDSParamList));

            if (!new_lists) {

                ERROR( CDS_LIB_NAME,
                    "Could not set transformation parameter: %s:%s\n"
                    " -> memory allocation error\n", obj_name, param_name);

                return(0);
            }

            tp->nalloced = new_nalloced;
            tp->lists    = new_lists;
        }

        list = &(tp->lists[tp->nlists]);

        if (!_cds_init_param_list(list, obj_name)) {

            ERROR( CDS_LIB_NAME,
                "Could not set transformation parameter: %s:%s\n"
                " -> memory allocation error\n", obj_name, param_name);

            return(0);
        }

        tp->nlists++;
    }

    /* Set the specified parameter value */

    status = _cds_set_param(list, param_name, type, length, value);

    if (!status) {

        ERROR( CDS_LIB_NAME,
            "Could not set transformation parameter: %s:%s\n"
            " -> memory allocation error\n", obj_name, param_name);

        return(0);
    }

    return(1);
}

/**
 *  Get the value of a transformation parameter.
 *
 *  This function will search for the requested parameter and return a copy
 *  of the value casted into the specified data type. The functions
 *  cds_string_to_array() and cds_array_to_string() are used to convert
 *  between text (CDS_CHAR) and numeric data types.
 *
 *  Memory will be allocated for the returned array if the output array
 *  is NULL. In this case the calling process is responsible for freeing
 *  the allocated memory.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  object     - pointer to the CDS Object to get the parameter for
 *  @param  param_name - name of the parameter
 *  @param  type       - data type of the output array
 *  @param  length     - pointer to the length of the output array
 *                         - input:
 *                             - length of the output array
 *                             - ignored if the output array is NULL
 *                         - output:
 *                             - number of values written to the output array
 *                             - 0 if the attribute value has zero length
 *                             - (size_t)-1 if a memory allocation error occurs
 *  @param  value      - pointer to the output array
 *                       or NULL to dynamically allocate the memory needed.
 *
 *  @return
 *    - pointer to the output array
 *    - NULL if:
 *        - the parameter was not found or has zero length (length == 0)
 *        - a memory allocation error occurs (length == (size_t)-1)
 */
void *cds_get_transform_param(
    void        *object,
    const char  *param_name,
    CDSDataType  type,
    size_t      *length,
    void        *value)
{
    CDSObject  *obj      = (CDSObject *)object;
    const char *obj_name = obj->name;
    CDSGroup   *group;
    void       *valuep;

    if (type == CDS_STRING) {
        ERROR( CDS_LIB_NAME,
            "Could not get transformation parameter for: %s:%s\n"
            " -> unsupported data type: %s\n",
            obj_name, param_name, cds_data_type_name(type));
        return(0);
    }

    /* Find the first parent group for the specified object */

    while (obj && obj->obj_type != CDS_GROUP) {
        obj = obj->parent;
    }

    group = (CDSGroup *)obj;

    while (group) {

        if (group->transform_params) {

            valuep = cds_get_transform_param_from_group(
                group, obj_name, param_name, type, length, value);

            if (valuep) return(valuep);
        }

        group = (CDSGroup *)group->parent;
    }

    if (length) *length = 0;
    return((void *)NULL);
}

/**
 *  Get the value of a transformation parameter defined in the specified group.
 *
 *  This function will return a copy of the requested parameter value casted
 *  into the specified data type. The functions cds_string_to_array() and
 *  cds_array_to_string() are used to convert between text (CDS_CHAR) and
 *  numeric data types.
 *
 *  Memory will be allocated for the returned array if the output array
 *  is NULL. In this case the calling process is responsible for freeing
 *  the allocated memory.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  group      - pointer to the CDSGroup
 *  @param  obj_name   - name of the CDSObject
 *  @param  param_name - name of the parameter
 *  @param  type       - data type of the output array
 *  @param  length     - pointer to the length of the output array
 *                         - input:
 *                             - length of the output array
 *                             - ignored if the output array is NULL
 *                         - output:
 *                             - number of values written to the output array
 *                             - 0 if the attribute value has zero length
 *                             - (size_t)-1 if a memory allocation error occurs
 *  @param  value      - pointer to the output array
 *                       or NULL to dynamically allocate the memory needed.
 *
 *  @return
 *    - pointer to the output array
 *    - NULL if:
 *        - the parameter was not found or has zero length (length == 0)
 *        - a memory allocation error occurs (length == (size_t)-1)
 */
void *cds_get_transform_param_from_group(
    CDSGroup    *group,
    const char  *obj_name,
    const char  *param_name,
    CDSDataType  type,
    size_t      *length,
    void        *value)
{
    CDSTransformParams *tp;
    CDSParamList       *list;
    CDSParam           *param;
    size_t              out_length;

    if (type == CDS_STRING) {
        ERROR( CDS_LIB_NAME,
            "Could not get transformation parameter for: %s:%s\n"
            " -> unsupported data type: %s\n",
            obj_name, param_name, cds_data_type_name(type));
        return(0);
    }

    if (!group || !obj_name || !param_name ||
        !group->transform_params) {

        if (length) *length = 0;
        return((void *)NULL);
    }

    tp = (CDSTransformParams *)group->transform_params;

    /* Get the parameter list for the specified object */

    list = _cds_get_param_list(tp->nlists, tp->lists, obj_name);

    if (!list) {
        if (length) *length = 0;
        return((void *)NULL);
    }

    /* Get the specified parameter from the list */

    param = _cds_get_param(list, param_name);

    if (!param || !param->length || !param->value.vp) {
        if (length) *length = 0;
        return((void *)NULL);
    }

    /* Convert parameter value to the requested return type */

    out_length = param->length;

    if (value && length && *length > 0) {
        if (out_length > *length) {
            out_length = *length;
        }
    }

    if (type == CDS_CHAR) {

        if (param->type == CDS_CHAR) {

            if (!value) {
                value = calloc(out_length + 1, sizeof(char));
                if (!value) {
                    out_length = (size_t)-1;
                }
            }

            if (value) {
                memcpy(value, param->value.vp, out_length * sizeof(char));
            }
        }
        else {
            value = (void *)cds_array_to_string(
                param->type, param->length, param->value.vp,
                &out_length, value);
        }
    }
    else if (param->type == CDS_CHAR) {

        value = cds_string_to_array(
            param->value.cp, type, &out_length, value);
    }
    else {

        value = cds_copy_array(
            param->type, out_length, param->value.vp, type, value,
            0, NULL, NULL, NULL, NULL, NULL, NULL);

        if (!value) {
            out_length = (size_t)-1;
        }
    }

    if (length) *length = out_length;

    if (out_length == (size_t)-1) {

        ERROR( CDS_LIB_NAME,
            "Could not get transformation parameter: %s:%s\n"
            " -> memory allocation error\n", obj_name, param_name);

        return((void *)NULL);
    }

    return(value);
}

/**
 *  Load transformation parameters from a configuration file.
 *
 *  This function will load transformation parameter values from
 *  the specified configuration file. Parameter values must be
 *  specified in this file using the following format:
 *
 *  object_name:param_name = value;
 *
 *  where value can be a string, a number, or an array of comma
 *  separated numeric values. The value can span multiple lines
 *  but must be terminated with a semicolon.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  group - pointer to the CDSGroup
 *  @param  path  - full path to the config files directory
 *  @param  file  - file name
 *
 *  @return
 *    -  1 if successful
 *    -  0 if the file does not exist
 *    - -1 if an error occurred
 */
int cds_load_transform_params_file(
    CDSGroup   *group,
    const char *path,
    const char *file)
{
    char         full_path[PATH_MAX];
    struct stat  fstats;
    int          fd;
    char        *buffer;
    off_t        buflen;
    ssize_t      nread;

    /* Open the file */

    if (!path) path = ".";

    snprintf(full_path, PATH_MAX, "%s/%s", path, file);

    if (access(full_path, F_OK) != 0) {
        return(0);
    }

    fd = open(full_path, O_RDONLY);
    if (fd < 0) {

        ERROR( CDS_LIB_NAME,
            "Could not open transformation parameters file: %s\n"
            " -> %s\n", full_path, strerror(errno));

        return(-1);
    }

    /* Read the file into memory */

    if (fstat(fd, &fstats) < 0) {

        ERROR( CDS_LIB_NAME,
            "Could not fstat transformation parameters file: %s\n"
            " -> %s\n", full_path, strerror(errno));

        return(-1);
    }

    if (!fstats.st_size) {

        ERROR( CDS_LIB_NAME,
            "Could not load transformation parameters file: %s\n"
            " -> file has zero length\n", full_path);

        return(-1);
    }

    buflen = fstats.st_size;
    buffer = malloc((buflen+1) * sizeof(char));

    if (!buffer) {

        ERROR( CDS_LIB_NAME,
            "Could not load transformation parameters file: %s\n"
            " -> memory allocation error\n", full_path);

        return(-1);
    }

    nread = read(fd, buffer, buflen);

    if (nread != buflen) {

        if (nread < 0) {

            ERROR( CDS_LIB_NAME,
                "Could not read transformation parameters file: %s\n"
                " -> %s\n", full_path, strerror(errno));
        }
        else {

            ERROR( CDS_LIB_NAME,
                "Could not read transformation parameters file: %s\n"
                " -> number of bytes read (%ld) != file size (%ld)\n",
                (long)nread, (long)buflen, full_path);
        }

        free(buffer);
        return(-1);
    }

    buffer[buflen] = '\0';

    close(fd);

    /* Parse the file */

    if (!cds_parse_transform_params(group, buffer, path)) {

        ERROR( CDS_LIB_NAME,
            "Could not parse transformation parameters file: %s\n",
            full_path);

        free(buffer);
        return(-1);
    }

    free(buffer);

    return(1);
}

/**
 *  Parse a text string containing transformation parameters.
 *
 *  Transformation parameters must be specified in the text string
 *  using the following format:
 *
 *  object_name:param_name = value;
 *
 *  where value can be a string, a number, or an array of comma
 *  separated numeric values. The value can span multiple lines
 *  but must be terminated with a semicolon.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  group  - pointer to the CDSGroup
 *  @param  string - text string containing the transformation params
 *  @param  path   - full path to the directory to look for transformation
 *                   parameter files if #include is used in the specified
 *                   text string.
 *
 *  @return
 *    -  1 if successful
 *    -  0 if an error occurred
 */
int cds_parse_transform_params(
    CDSGroup   *group,
    char       *string,
    const char *path)
{
    int     status;
    char   *cp;
    int     nlines;
    char   *obj_name;
    char   *param_name;
    char   *param_value;
    char   *inc_file;
    size_t  length;

    status = 1;
    nlines = 1;
    cp     = string;

    while (*cp) {

        /* Skip white space */

        while (isspace(*cp)) {
            if (*cp == '\n') nlines++;
            cp++;
        }

        /* Check for include files and comments */

        if (*cp == '#') {

            if (strncmp(cp, "#include ", 9) == 0) {

                cp += 9;

                status = _cds_get_token(
                    cp, '\n', NULL, &nlines, &inc_file, &cp);

                if (status <= 0) break;

                status = cds_load_transform_params_file(group, path, inc_file);
                if (status < 0) {
                    return(0);
                }
            }
            else {
                while (*cp && *cp != '\n') cp++;
                nlines++;
            }

            continue;
        }

        /* Get object name */

        status = _cds_get_token(
            cp, ':', "=;", &nlines, &obj_name, &cp);

        if (status <= 0) break;

        /* Get parameter name */

        status = _cds_get_token(
            cp, '=', ";", &nlines, &param_name, &cp);

        if (status <= 0) break;

        /* Get parameter value */

        status = _cds_get_token(
            cp, ';', NULL, &nlines, &param_value, &cp);

        if (status <= 0) break;

        /* Trim leading and trailing string quote from the parameter value */

        if (param_value[0] == '"') param_value++;

        length = strlen(param_value);

        if (length > 0) {
            if (param_value[length-1] == '"') {
                param_value[length-1] = '\0';
            }
            else {
                length++;
            }
        }

        /* Set the transformation parameter */

        status = cds_set_transform_param(
            group, obj_name, param_name, CDS_CHAR, length, param_value);

        if (status == 0) {
            return(0);
        }
    }

    return((status < 0) ? 0 : 1);
}

/**
 *  Print transformation parameters defined in the specified group.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  fp       - pointer to the open file to print to
 *  @param  indent   - line indent string
 *  @param  group    - pointer to the CDSGroup
 *  @param  obj_name - name of the object to print the parameters for
 *                     or NULL to print all
 *
 *  @return
 *    - number of bytes printed
 *    - negative value if an error occurred
 */
int cds_print_group_transform_params(
    FILE       *fp,
    const char *indent,
    CDSGroup   *group,
    const char *obj_name)
{
    CDSTransformParams *tp = (CDSTransformParams *)group->transform_params;
    int    nbytes;
    int    tbytes;
    int    li;

    tbytes = 0;

    if (tp) {
        for (li = 0; li < tp->nlists; li++) {

            if (obj_name && strcmp(tp->lists[li].name, obj_name) != 0) {
                continue;
            }

            nbytes = _cds_print_param_list(fp, indent, &(tp->lists[li]));
            if (nbytes < 0) {
                return(nbytes);
            }
            tbytes += nbytes;

            if (li < tp->nlists - 1) {
                nbytes = fprintf(fp, "\n");
                if (nbytes < 0) return(nbytes);
                tbytes += nbytes;
            }
        }
    }

    return(tbytes);
}

/**
 *  Print all transformation parameters in the group path.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  fp       - pointer to the open file to print to
 *  @param  indent   - line indent string
 *  @param  group    - pointer to the start CDSGroup
 *  @param  obj_name - name of the object to print the parameters for
 *                     or NULL to print all
 *
 *  @return
 *    - number of bytes printed
 *    - negative value if an error occurred
 */
int cds_print_transform_params(
    FILE       *fp,
    const char *indent,
    CDSGroup   *group,
    const char *obj_name)
{
    char *indent4;
    int   nbytes;
    int   tbytes;

    tbytes  = 0;
    indent4 = msngr_create_string("%s    ", indent);
    if (!indent4) return(-1);

    while (group) {

        if (group->transform_params) {

            nbytes = fprintf(fp, "%sGroup: %s\n\n",
                indent, cds_get_object_path(group));

            if (nbytes < 0) {
                free(indent4);
                return(nbytes);
            }
            tbytes += nbytes;

            nbytes = cds_print_group_transform_params(
                fp, indent4, group, obj_name);

            if (nbytes < 0) return(nbytes);
            tbytes += nbytes;

            nbytes = fprintf(fp, "\n");
            if (nbytes < 0) {
                free(indent4);
                return(nbytes);
            }
            tbytes += nbytes;
        }

        group = (CDSGroup *)group->parent;
    }

    free(indent4);
    return(tbytes);
}
