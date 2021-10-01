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

/** @file cds_print.c
 *  CDS Print Functions.
 */

#include "cds3.h"
#include "cds_private.h"

/*******************************************************************************
 *  Private Functions
 */
/** @privatesection */

int _cds_print_att_array(FILE *fp, CDSAtt *att)
{
    int    nbytes;
    int    tbytes;
    size_t i;

    nbytes = fprintf(fp, "[");
    if (nbytes < 0) return(nbytes);
    tbytes = nbytes;

    if (att->length > 0) {

        for (i = 0; i < att->length; i++) {

            switch (att->type) {
                case CDS_BYTE:
                    nbytes = fprintf(fp, "%hhd", att->value.bp[i]);
                    break;
                case CDS_SHORT:
                    nbytes = fprintf(fp, "%hd", att->value.sp[i]);
                    break;
                case CDS_INT:
                    nbytes = fprintf(fp, "%d", att->value.ip[i]);
                    break;
                case CDS_FLOAT:
                    nbytes = fprintf(fp, "%.7g", att->value.fp[i]);
                    break;
                case CDS_DOUBLE:
                    nbytes = fprintf(fp, "%.15g", att->value.dp[i]);
                    break;
                /* NetCDF4 extended data types */
                case CDS_INT64:
                    nbytes = fprintf(fp, "%lld", att->value.i64p[i]);
                    break;
                case CDS_UBYTE:
                    nbytes = fprintf(fp, "%hhu", att->value.ubp[i]);
                    break;
                case CDS_USHORT:
                    nbytes = fprintf(fp, "%hu", att->value.usp[i]);
                    break;
                case CDS_UINT:
                    nbytes = fprintf(fp, "%u", att->value.uip[i]);
                    break;
                case CDS_UINT64:
                    nbytes = fprintf(fp, "%llu", att->value.ui64p[i]);
                    break;
                default:
                    break;
            }

            if (nbytes < 0) return(nbytes);
            tbytes += nbytes;

            if (i < att->length - 1) {
                nbytes = fprintf(fp, ", ");

                if (nbytes < 0) return(nbytes);
                tbytes += nbytes;
            }
        }
    }

    nbytes = fprintf(fp, "]");
    if (nbytes < 0) return(nbytes);
    tbytes += nbytes;

    return(tbytes);
}

int _cds_print_att_array_char(FILE *fp, CDSAtt *att)
{
    int   nbytes;
    int   tbytes;
    int   length;
    int   i;
    char *cp;
    char  uc;

    length = att->length;

    nbytes = fprintf(fp, "\"");
    if (nbytes < 0) return(nbytes);
    tbytes = nbytes;

    if (length > 0) {

        /* Adjust the length so trailing nulls don't get printed */

        cp = att->value.cp + length - 1;
        while (length && *cp == '\0') {
            cp--;
            length--;
        }

        for (i = 0; i < length; i++) {

            switch (uc = att->value.cp[i] & 0377) {
                case '\b': nbytes = fprintf(fp, "\\b");    break;
                case '\f': nbytes = fprintf(fp, "\\f");    break;
                case '\r': nbytes = fprintf(fp, "\\r");    break;
                case '\v': nbytes = fprintf(fp, "\\v");    break;
                default:   nbytes = fprintf(fp, "%c", uc); break;
            }

            if (nbytes < 0) return(nbytes);
            tbytes += nbytes;
        }
    }

    nbytes = fprintf(fp, "\"");
    if (nbytes < 0) return(nbytes);
    tbytes += nbytes;

    return(tbytes);
}

int _cds_print_att_array_string(FILE *fp, const char *indent, CDSAtt *att)
{
    int    nbytes;
    int    tbytes = 0;
    size_t i;

    if (att->length <= 0) {
        return(0);
    }

    nbytes = fprintf(fp, "\"%s\"", att->value.strp[0]);
    if (nbytes < 0) return(nbytes);
    tbytes += nbytes;

    for (i = 1; i < att->length; i++) {
        nbytes = fprintf(fp, ",\n%s\"%s\"", indent, att->value.strp[i]);
        if (nbytes < 0) return(nbytes);
        tbytes += nbytes;
    }

    return(tbytes);
}

int _cds_print_data_array(
    FILE        *fp,
    int          line_length,
    CDSDataType  type,
    size_t       start,
    size_t       count,
    CDSData      data)
{
    int    nbytes;
    int    tbytes;
    size_t end;
    int    str_length;
    char   str_value[32];
    size_t i;

    tbytes = 0;
    end    = start + count;

    if (!data.vp) {
        return(0);
    }

    for (i = start; i < end; i++) {

        switch (type) {
            case CDS_BYTE:
                str_length = sprintf(str_value, "%hhd", data.bp[i]);
                break;
            case CDS_SHORT:
                str_length = sprintf(str_value, "%hd", data.sp[i]);
                break;
            case CDS_INT:
                str_length = sprintf(str_value, "%d", data.ip[i]);
                break;
            case CDS_FLOAT:
                str_length = sprintf(str_value, "%.7g", data.fp[i]);
                break;
            case CDS_DOUBLE:
                str_length = sprintf(str_value, "%.15g", data.dp[i]);
                break;
            /* NetCDF4 extended data types */
            case CDS_INT64:
                str_length = sprintf(str_value, "%lld", data.i64p[i]);
                break;
            case CDS_UBYTE:
                str_length = sprintf(str_value, "%hhu", data.ubp[i]);
                break;
            case CDS_USHORT:
                str_length = sprintf(str_value, "%hu", data.usp[i]);
                break;
            case CDS_UINT:
                str_length = sprintf(str_value, "%u", data.uip[i]);
                break;
            case CDS_UINT64:
                str_length = sprintf(str_value, "%llu", data.ui64p[i]);
                break;
            default:
                str_length = sprintf(str_value, "NaT");
                break;
        }

        if (i == start) {
            nbytes = fprintf(fp, "%s", str_value);
            line_length += str_length;
        }
        else if ((line_length + str_length + 4) > 80) {
            nbytes = fprintf(fp, ",\n    %s", str_value);
            line_length = str_length + 4;
        }
        else {
            nbytes = fprintf(fp, ", %s", str_value);
            line_length += str_length + 2;
        }

        if (nbytes < 0) return(nbytes);
        tbytes += nbytes;
    }

    return(tbytes);
}

int _cds_print_data_array_char(
    FILE   *fp,
    size_t  start,
    size_t  count,
    char   *chrp)
{
    int    nbytes;
    int    tbytes;
    size_t end;
    char   uc;
    size_t i;

    nbytes = fprintf(fp, "\"");
    if (nbytes < 0) return(nbytes);
    tbytes = nbytes;

    end = start + count;

    for (i = start; i < end; i++) {

        switch (uc = chrp[i] & 0377) {
            case '\0': nbytes = fprintf(fp, "\\0");    break;
            case '\b': nbytes = fprintf(fp, "\\b");    break;
            case '\f': nbytes = fprintf(fp, "\\f");    break;
            case '\n': nbytes = fprintf(fp, "\\n");    break;
            case '\r': nbytes = fprintf(fp, "\\r");    break;
            case '\t': nbytes = fprintf(fp, "\\t");    break;
            case '\v': nbytes = fprintf(fp, "\\v");    break;
            case '\"': nbytes = fprintf(fp, "\\\"");   break;
            default:   nbytes = fprintf(fp, "%c", uc); break;
        }

        if (nbytes < 0) return(nbytes);
        tbytes += nbytes;
    }

    nbytes = fprintf(fp, "\"");
    if (nbytes < 0) return(nbytes);
    tbytes += nbytes;

    return(tbytes);
}

int _cds_print_data_array_string(
    FILE        *fp,
    const char  *indent,
    size_t       start,
    size_t       count,
    char       **strpp)
{
    int    nbytes;
    int    tbytes;
    size_t end;
    size_t i;

    tbytes = 0;
    end    = start + count;

    if (!strpp || end <= start) {
        return(0);
    }

    nbytes = fprintf(fp, "\n%s\"%s\"", indent, strpp[start]);
    if (nbytes < 0) return(nbytes);
    tbytes += nbytes;

    for (i = start + 1; i < end; i++) {
        nbytes = fprintf(fp, ",\n%s\"%s\"", indent, strpp[i]);
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
 *  Print CDS.
 *
 *  By default (flags = 0) this function will print all dimensions,
 *  attributes, variables, groups, and data in the specified group.
 *
 *  Control Flags:
 *
 *    - CDS_SKIP_GROUP_ATTS = do not print group attributes
 *    - CDS_SKIP_VAR_ATTS   = do not print variable attributes
 *    - CDS_SKIP_DATA       = do not print variable data
 *    - CDS_SKIP_SUBGROUPS  = do not print subgroups
 *
 *  @param  fp    - pointer to the open file to print to
 *  @param  group - pointer to the group
 *  @param  flags - control flags
 *
 *  @return
 *    - number of bytes printed
 *    - negative value if an error occurred
 */
int cds_print(
    FILE     *fp,
    CDSGroup *group,
    int       flags)
{
    return(cds_print_group(fp, "", group, flags));
}

/**
 *  Print a CDS Attribute.
 *
 *  @param  fp        - pointer to the open file to print to
 *  @param  indent    - line indent string
 *  @param  min_width - the minimum width used to print the attribute name
 *  @param  att       - pointer to the attribute
 *
 *  @return
 *    - number of bytes printed
 *    - negative value if an error occurred
 */
int cds_print_att(
    FILE       *fp,
    const char *indent,
    int         min_width,
    CDSAtt     *att)
{
    const char *type_name = cds_data_type_name(att->type);
    char        format[64];
    int         nbytes;
    int         tbytes;
    char        indent2[128];

    sprintf(format, "%%s%%-%ds = ", min_width);

    nbytes = fprintf(fp, format, indent, att->name);
    if (nbytes < 0) return(nbytes);
    tbytes = nbytes;

    if (att->type == CDS_CHAR) {

        nbytes = _cds_print_att_array_char(fp, att);
        if (nbytes < 0) return(nbytes);
        tbytes += nbytes;

        nbytes = fprintf(fp, "\n");
    }
    else if (att->type == CDS_STRING) {

        if (nbytes > 127) nbytes = 127;
        memset(indent2, ' ', nbytes);
        indent2[nbytes] = '\0';

        nbytes = _cds_print_att_array_string(fp, indent2, att);
        if (nbytes < 0) return(nbytes);
        tbytes += nbytes;

        nbytes = fprintf(fp, "\n");
    }
    else {
        nbytes = _cds_print_att_array(fp, att);
        if (nbytes < 0) return(nbytes);
        tbytes += nbytes;

        nbytes = fprintf(fp, " : %s\n", type_name);
    }

    if (nbytes < 0) return(nbytes);
    tbytes += nbytes;

    return(tbytes);
}

/**
 *  Print CDS Attributes.
 *
 *  @param  fp     - pointer to the open file to print to
 *  @param  indent - line indent string
 *  @param  parent - pointer to the parent group or variable
 *
 *  @return
 *    - number of bytes printed
 *    - negative value if an error occurred
 */
int cds_print_atts(
    FILE       *fp,
    const char *indent,
    void       *parent)
{
    CDSObject *parent_object = (CDSObject *)parent;
    CDSGroup  *group;
    CDSVar    *var;
    CDSAtt   **atts;
    int        natts;
    int        min_width;
    int        length;
    int        ai;
    int        nbytes;
    int        tbytes;

    /* Make sure the parent is a group or variable */

    if (parent_object->obj_type == CDS_GROUP) {
        group = (CDSGroup *)parent;
        natts = group->natts;
        atts  = group->atts;
    }
    else if (parent_object->obj_type == CDS_VAR) {
        var   = (CDSVar *)parent_object;
        natts = var->natts;
        atts  = var->atts;
    }
    else {
        return(0);
    }

    tbytes = 0;

    if (natts) {

        min_width = 0;

        for (ai = 0; ai < natts; ai++) {
            length = strlen(atts[ai]->name);
            if (min_width < length) {
                min_width = length;
            }
        }

        for (ai = 0; ai < natts; ai++) {
            nbytes = cds_print_att(fp, indent, min_width, atts[ai]);
            if (nbytes < 0) return(nbytes);
            tbytes += nbytes;
        }
    }

    return(tbytes);
}

/**
 *  Print a CDS Dimension.
 *
 *  @param  fp        - pointer to the open file to print to
 *  @param  indent    - line indent string
 *  @param  min_width - the minimum width used to print the dimension name
 *  @param  dim       - pointer to the dimension
 *
 *  @return
 *    - number of bytes printed
 *    - negative value if an error occurred
 */
int cds_print_dim(
    FILE       *fp,
    const char *indent,
    int         min_width,
    CDSDim     *dim)
{
    char format[64];
    int  tbytes;

    if (dim->is_unlimited) {
        sprintf(format, "%%s%%-%ds = UNLIMITED (%%ld currently)\n", min_width);
    }
    else {
        sprintf(format, "%%s%%-%ds = %%ld\n", min_width);
    }

    tbytes = fprintf(fp, format, indent, dim->name, (long)dim->length);

    return(tbytes);
}

/**
 *  Print CDS Dimensions.
 *
 *  @param  fp     - pointer to the open file to print to
 *  @param  indent - line indent string
 *  @param  group  - pointer to the group
 *
 *  @return
 *    - number of bytes printed
 *    - negative value if an output error occurred
 */
int cds_print_dims(
    FILE       *fp,
    const char *indent,
    CDSGroup   *group)
{
    int min_width;
    int length;
    int di;
    int nbytes;
    int tbytes;

    tbytes = 0;

    if (group->ndims) {

        min_width = 0;

        for (di = 0; di < group->ndims; di++) {
            length = strlen(group->dims[di]->name);
            if (min_width < length) {
                min_width = length;
            }
        }

        for (di = 0; di < group->ndims; di++) {
            nbytes = cds_print_dim(fp, indent, min_width, group->dims[di]);
            if (nbytes < 0) return(nbytes);
            tbytes += nbytes;
        }
    }

    return(tbytes);
}

/**
 *  Print a CDS Variable.
 *
 *  By default (flags = 0) all variable attributes will also be printed.
 *
 *  Control Flags:
 *
 *    - CDS_SKIP_VAR_ATTS = do not print variable attributes
 *    - CDS_SKIP_DATA     = do not print variable data
 *
 *  @param  fp     - pointer to the open file to print to
 *  @param  indent - line indent string
 *  @param  var    - pointer to the variable
 *  @param  flags  - control flags
 *
 *  @return
 *    - number of bytes printed
 *    - negative value if an error occurred
 */
int cds_print_var(
    FILE       *fp,
    const char *indent,
    CDSVar     *var,
    int         flags)
{
    const char *type_name = cds_data_type_name(var->type);
    CDSDim     *dim;
    char       *indent4;
    int         di;
    int         nbytes;
    int         tbytes;

    /* Print Variable Name and Dimensions */

    nbytes = fprintf(fp, "%s%s(", indent, var->name);
    if (nbytes < 0) return(nbytes);
    tbytes = nbytes;

    for (di = 0; di < var->ndims; di++) {

        dim = var->dims[di];

        if (di) { nbytes = fprintf(fp, ", %s", dim->name); }
        else    { nbytes = fprintf(fp, "%s",   dim->name); }

        if (nbytes < 0) return(nbytes);
        tbytes += nbytes;
    }

    nbytes = fprintf(fp, ") : %s\n", type_name);
    if (nbytes < 0) return(nbytes);
    tbytes += nbytes;

    /* Print Variable Attributes */

    if (!(flags & CDS_SKIP_VAR_ATTS)) {

        if (var->natts) {

            indent4 = msngr_create_string("%s    ", indent);
            if (!indent4) {
                return(-1);
            }

            nbytes = cds_print_atts(fp, indent4, var);
            if (nbytes < 0) {
                free(indent4);
                return(nbytes);
            }
            tbytes += nbytes;

            free(indent4);
        }
    }

    /* Print Variable Data */

    if (!(flags & CDS_SKIP_DATA)) {

        nbytes = fprintf(fp, "\n");
        if (nbytes < 0) return(nbytes);
        tbytes += nbytes;

        nbytes = cds_print_var_data(fp, indent, "data", var);
        if (nbytes < 0) return(nbytes);
        tbytes += nbytes;
    }

    return(tbytes);
}

/**
 *  Print CDS Variables.
 *
 *  By default (flags = 0) all variable attributes will also be printed.
 *
 *  Control Flags:
 *
 *    - CDS_SKIP_VAR_ATTS = do not print variable attributes
 *    - CDS_SKIP_DATA     = do not print variable data
 *
 *  @param  fp     - pointer to the open file to print to
 *  @param  indent - line indent string
 *  @param  group  - pointer to the group
 *  @param  flags  - control flags
 *
 *  @return
 *    - number of bytes printed
 *    - negative value if an error occurred
 */
int cds_print_vars(
    FILE       *fp,
    const char *indent,
    CDSGroup   *group,
    int         flags)
{
    int vi;
    int nbytes;
    int tbytes;

    tbytes = 0;

    if (group->nvars) {
        for (vi = 0; vi < group->nvars; vi++) {

            nbytes = fprintf(fp, "\n");
            if (nbytes < 0) return(nbytes);
            tbytes += nbytes;

            nbytes = cds_print_var(fp, indent, group->vars[vi], flags);
            if (nbytes < 0) return(nbytes);
            tbytes += nbytes;
        }
    }

    return(tbytes);
}

/**
 *  Print CDS Variable Data.
 *
 *  @param  fp     - pointer to the open file to print to
 *  @param  indent - line indent string
 *  @param  label  - label to print before the = sign,
 *                   or NULL to only print the data values
 *  @param  var    - pointer to the variable
 *
 *  @return
 *    - number of bytes printed
 *    - negative value if an error occurred
 */
int cds_print_var_data(
    FILE       *fp,
    const char *indent,
    const char *label,
    CDSVar     *var)
{
    size_t sample_size;
    int    nbytes;
    int    tbytes;
    char   indent2[128];
    size_t si;

    tbytes = 0;

    if (label) {

        nbytes = fprintf(fp, "%s%s = ", indent, label);
        if (nbytes < 0) return(nbytes);
        tbytes += nbytes;

        if (!var->sample_count) {

            nbytes = fprintf(fp, "NULL\n");
            if (nbytes < 0) return(nbytes);
            tbytes += nbytes;

            return(tbytes);
        }
    }
    else if (!var->sample_count) {
        return(0);
    }
    else {
        nbytes = fprintf(fp, "%s", indent);
        if (nbytes < 0) return(nbytes);
        tbytes += nbytes;
    }

    sample_size = cds_var_sample_size(var);

    if (var->type == CDS_STRING) {

        nbytes = (indent) ? strlen(indent) : 0;
        nbytes += 4;
        if (nbytes > 127) nbytes = 127;
        memset(indent2, ' ', nbytes);
        indent2[nbytes] = '\0';

        nbytes = _cds_print_data_array_string(
            fp, indent2, 0, var->sample_count * sample_size, var->data.strp);

        if (nbytes < 0) return(nbytes);
        tbytes += nbytes;
    }
    else if (sample_size == 1) {

        if (var->type == CDS_CHAR) {
            nbytes = _cds_print_data_array_char(
                fp, 0, var->sample_count, var->data.cp);
        }
        else {
            nbytes = _cds_print_data_array(
                fp, nbytes, var->type, 0, var->sample_count, var->data);
        }

        if (nbytes < 0) return(nbytes);
        tbytes += nbytes;
    }
    else {

        nbytes = fprintf(fp, "\n        ");
        if (nbytes < 0) return(nbytes);
        tbytes += nbytes;

        for (si = 0; si < var->sample_count; si++) {

            if (var->type == CDS_CHAR) {
                nbytes = _cds_print_data_array_char(
                    fp, si * sample_size, sample_size, var->data.cp);
            }
            else {
                nbytes = _cds_print_data_array(
                    fp, 8, var->type, si * sample_size, sample_size, var->data);
            }

            if (nbytes < 0) return(nbytes);
            tbytes += nbytes;

            if (si < var->sample_count - 1) {
                nbytes = fprintf(fp, ",\n        ");
                if (nbytes < 0) return(nbytes);
                tbytes += nbytes;
            }
        }
    }

    nbytes = fprintf(fp, "\n");
    if (nbytes < 0) return(nbytes);
    tbytes = nbytes;

    return(tbytes);
}

/**
 *  Print CDS Data.
 *
 *  This function will print the data for all the variables
 *  in the specified group.
 *
 *  @param  fp     - pointer to the open file to print to
 *  @param  indent - line indent string
 *  @param  group  - pointer to the group
 *
 *  @return
 *    - number of bytes printed
 *    - negative value if an error occurred
 */
int cds_print_data(
    FILE       *fp,
    const char *indent,
    CDSGroup   *group)
{
    CDSVar *var;
    int     nbytes;
    int     tbytes;
    int     vi;

    tbytes = 0;

    if (group->nvars) {

        for (vi = 0; vi < group->nvars; vi++) {

            var = group->vars[vi];

            nbytes = fprintf(fp, "\n");
            if (nbytes < 0) return(nbytes);
            tbytes += nbytes;

            nbytes = cds_print_var_data(fp, indent, var->name, var);
            if (nbytes < 0) return(nbytes);
            tbytes += nbytes;
        }
    }

    return(tbytes);
}

/**
 *  Print CDS Group.
 *
 *  By default (flags = 0) this function will print all dimensions,
 *  attributes, variables, groups, and data in the specified group.
 *
 *  Control Flags:
 *
 *    - CDS_SKIP_DIMS       = do not print dimensions
 *    - CDS_SKIP_GROUP_ATTS = do not print group attributes
 *    - CDS_SKIP_VAR_ATTS   = do not print variable attributes
 *    - CDS_SKIP_VARS       = do not print variables
 *    - CDS_SKIP_DATA       = do not print variable data
 *    - CDS_SKIP_SUBGROUPS  = do not traverse subgroups
 *
 *    - CDS_PRINT_VARGROUPS = print variable groups
 *
 *  @param  fp          - pointer to the open file to print to
 *  @param  indent      - line indent string
 *  @param  group       - pointer to the group
 *  @param  flags       - control flags
 *
 *  @return
 *    - number of bytes printed
 *    - negative value if an error occurred
 */
int cds_print_group(
    FILE       *fp,
    const char *indent,
    CDSGroup   *group,
    int         flags)
{
    char *indent8;
    int   nbytes;
    int   tbytes;

    tbytes = 0;

    /* Create the indent string */

    indent8 = msngr_create_string("%s        ", indent);
    if (!indent8) {
        return(-1);
    }

    /* Print group path */

    nbytes = fprintf(fp, "\n%sGroup: %s\n",
        indent, cds_get_object_path(group));

    if (nbytes < 0) {
        free(indent8);
        return(nbytes);
    }
    tbytes = nbytes;

    /* Print Dimensions */

    if (!(flags & CDS_SKIP_DIMS) && group->ndims) {

        nbytes = fprintf(fp, "\n%s    Dimensions:\n\n", indent);
        if (nbytes < 0) {
            free(indent8);
            return(nbytes);
        }
        tbytes += nbytes;

        nbytes = cds_print_dims(fp, indent8, group);
        if (nbytes < 0) {
            free(indent8);
            return(nbytes);
        }
        tbytes += nbytes;
    }

    /* Print Attributes */

    if (!(flags & CDS_SKIP_GROUP_ATTS) && group->natts) {

        nbytes = fprintf(fp, "\n%s    Attributes:\n\n", indent);
        if (nbytes < 0) {
            free(indent8);
            return(nbytes);
        }
        tbytes += nbytes;

        nbytes = cds_print_atts(fp, indent8, group);
        if (nbytes < 0) {
            free(indent8);
            return(nbytes);
        }
        tbytes += nbytes;
    }

    /* Print Variables */

    if (!(flags & CDS_SKIP_VARS) && group->nvars) {

        nbytes = fprintf(fp, "\n%s    Variables:\n", indent);
        if (nbytes < 0) {
            free(indent8);
            return(nbytes);
        }
        tbytes += nbytes;

        nbytes = cds_print_vars(fp, indent8, group, flags | CDS_SKIP_DATA);
        if (nbytes < 0) {
            free(indent8);
            return(nbytes);
        }
        tbytes += nbytes;
    }

    /* Print Variable Groups */

    if ((flags & CDS_PRINT_VARGROUPS) && group->nvargroups) {

        nbytes = fprintf(fp, "\n%s    Variable Groups:\n", indent);
        if (nbytes < 0) {
            free(indent8);
            return(nbytes);
        }
        tbytes += nbytes;

        nbytes = cds_print_vargroups(fp, indent8, group, CDS_SKIP_VARS);
        if (nbytes < 0) {
            free(indent8);
            return(nbytes);
        }
        tbytes += nbytes;
    }

    /* Print Data */

    if (!(flags & CDS_SKIP_DATA) && group->nvars) {

        nbytes = fprintf(fp, "\n%s    Data:\n", indent);
        if (nbytes < 0) {
            free(indent8);
            return(nbytes);
        }
        tbytes += nbytes;

        nbytes = cds_print_data(fp, indent8, group);
        if (nbytes < 0) {
            free(indent8);
            return(nbytes);
        }
        tbytes += nbytes;
    }

    /* Print Group */

    if (!(flags & CDS_SKIP_SUBGROUPS)) {
        nbytes = cds_print_groups(fp, indent, group, flags);
        if (nbytes < 0) {
            free(indent8);
            return(nbytes);
        }
        tbytes += nbytes;
    }

    free(indent8);

    return(tbytes);
}

/**
 *  Print CDS Groups.
 *
 *  This function will print all supgroups under the specified group.
 *
 *  By default (flags = 0) this function will print all dimensions,
 *  attributes, variables, groups, and data in the subgroups.
 *
 *  Control Flags:
 *
 *    - CDS_SKIP_DIMS       = do not print dimensions
 *    - CDS_SKIP_GROUP_ATTS = do not print group attributes
 *    - CDS_SKIP_VAR_ATTS   = do not print variable attributes
 *    - CDS_SKIP_VARS       = do not print variables
 *    - CDS_SKIP_DATA       = do not print variable data
 *    - CDS_SKIP_SUBGROUPS  = do not traverse subgroups
 *
 *    - CDS_PRINT_VARGROUPS = print variable groups
 *
 *  @param  fp     - pointer to the open file to print to
 *  @param  indent - line indent string
 *  @param  group  - pointer to the group
 *  @param  flags  - control flags
 *
 *  @return
 *    - number of bytes printed
 *    - negative value if an error occurred
 */
int cds_print_groups(
    FILE       *fp,
    const char *indent,
    CDSGroup   *group,
    int         flags)
{
    int nbytes;
    int tbytes;
    int i;

    tbytes = 0;

    for (i = 0; i < group->ngroups; i++) {

        nbytes = cds_print_group(
            fp, indent, group->groups[i], flags);

        if (nbytes < 0) {
            return(nbytes);
        }

        tbytes += nbytes;
    }

    return(tbytes);
}

/**
 *  Print CDS Variable Array.
 *
 *  By default (flags = 0) this function will print all variables,
 *  variable attributes and data in the specified variable array.
 *
 *  Control Flags:
 *
 *    - CDS_SKIP_VAR_ATTS = do not print variable attributes
 *    - CDS_SKIP_VARS     = do not print variables
 *    - CDS_SKIP_DATA     = do not print variable data
 *
 *  @param  fp       - pointer to the open file to print to
 *  @param  indent   - line indent string
 *  @param  vararray - pointer to the variable group
 *  @param  flags    - control flags
 *
 *  @return
 *    - number of bytes printed
 *    - negative value if an error occurred
 */
int cds_print_vararray(
    FILE        *fp,
    const char  *indent,
    CDSVarArray *vararray,
    int          flags)
{
    CDSVar *var;
    char   *indent4;
    int     nbytes;
    int     tbytes;
    int     vi;

    tbytes = 0;

    if (!vararray->nvars) {
        nbytes = fprintf(fp, "\n%s%s = NULL\n", indent, vararray->name);
        return(nbytes);
    }

    if ((flags & CDS_SKIP_VARS) && vararray->nvars) {
        nbytes = fprintf(fp, "\n");
        if (nbytes < 0) return(nbytes);
        tbytes += nbytes;
    }

    /* Create the indent string */

    indent4 = msngr_create_string("%s    ", indent);
    if (!indent4) {
        return(-1);
    }

    /* Print variables */

    for (vi = 0; vi < vararray->nvars; vi++) {

        var = vararray->vars[vi];

        if (!(flags & CDS_SKIP_VARS)) {
            nbytes = fprintf(fp, "\n");
            if (nbytes < 0) return(nbytes);
            tbytes += nbytes;
        }

        if (!var) {

            nbytes = fprintf(fp, "%s%s[%d]: NULL\n",
                indent, vararray->name, vi);

            if (nbytes < 0) return(nbytes);
            tbytes += nbytes;
        }
        else {

            nbytes = fprintf(fp, "%s%s[%d]: %s\n",
                indent, vararray->name, vi, cds_get_object_path(var));

            if (nbytes < 0) return(nbytes);
            tbytes += nbytes;

            if (!(flags & CDS_SKIP_VARS)) {

                nbytes = fprintf(fp, "\n");
                if (nbytes < 0) return(nbytes);
                tbytes += nbytes;

                nbytes = cds_print_var(fp, indent4, var, flags);
                if (nbytes < 0) return(nbytes);
                tbytes += nbytes;
            }
        }
    }

    free(indent4);

    return(tbytes);
}

/**
 *  Print CDS Variable Group.
 *
 *  By default (flags = 0) this function will print all variables,
 *  variable attributes and data in the specified variable group.
 *
 *  Control Flags:
 *
 *    - CDS_SKIP_VAR_ATTS = do not print variable attributes
 *    - CDS_SKIP_VARS     = do not print variables
 *    - CDS_SKIP_DATA     = do not print variable data
 *
 *  @param  fp       - pointer to the open file to print to
 *  @param  indent   - line indent string
 *  @param  vargroup - pointer to the variable group
 *  @param  flags    - control flags
 *
 *  @return
 *    - number of bytes printed
 *    - negative value if an error occurred
 */
int cds_print_vargroup(
    FILE        *fp,
    const char  *indent,
    CDSVarGroup *vargroup,
    int          flags)
{
    char *indent4;
    int   nbytes;
    int   tbytes;
    int   ai;

    tbytes = 0;

    /* Print vargroup path */

    nbytes = fprintf(fp, "\n%sVarGroup: %s\n",
        indent, cds_get_object_path(vargroup));

    if (nbytes < 0) {
        return(nbytes);
    }
    tbytes = nbytes;

    /* Create the indent string */

    indent4 = msngr_create_string("%s    ", indent);
    if (!indent4) {
        return(-1);
    }

    /* Print Variable Arrays */

    for (ai = 0; ai < vargroup->narrays; ai++) {

        nbytes = cds_print_vararray(fp, indent4, vargroup->arrays[ai], flags);
        if (nbytes < 0) {
            free(indent4);
            return(nbytes);
        }
        tbytes += nbytes;
    }

    free(indent4);

    return(tbytes);
}

/**
 *  Print CDS Variable Groups.
 *
 *  By default (flags = 0) this function will print all variables,
 *  variable attributes and data for the variable groups in the
 *  specified CDS group.
 *
 *  Control Flags:
 *
 *    - CDS_SKIP_VAR_ATTS = do not print variable attributes
 *    - CDS_SKIP_VARS     = do not print variables
 *    - CDS_SKIP_DATA     = do not print variable data
 *
 *  @param  fp     - pointer to the open file to print to
 *  @param  indent - line indent string
 *  @param  group  - pointer to the variable group
 *  @param  flags  - control flags
 *
 *  @return
 *    - number of bytes printed
 *    - negative value if an error occurred
 */
int cds_print_vargroups(
    FILE       *fp,
    const char *indent,
    CDSGroup   *group,
    int         flags)
{
    int nbytes;
    int tbytes;
    int gi;

    tbytes = 0;

    /* Print Variable Groups */

    for (gi = 0; gi < group->nvargroups; gi++) {

        nbytes = cds_print_vargroup(fp, indent, group->vargroups[gi], flags);
        if (nbytes < 0) {
            return(nbytes);
        }
        tbytes += nbytes;
    }

    return(tbytes);
}
