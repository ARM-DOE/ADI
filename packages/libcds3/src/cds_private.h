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

/** @file cds_private.h
 *  Private CDS Functions.
 */

#ifndef _CDS_PRIVATE_
#define _CDS_PRIVATE_

#include "cds3.h"

/** @privatesection */

/*****  Object Functions  *****/

void        _cds_free_object_members(void *cds_object);
int         _cds_init_object_members(
                void          *cds_object,
                CDSObjectType  obj_type,
                void          *parent,
                const char    *name);

void       *_cds_get_object(void **array, int nobjects, const char *name);
const char *_cds_obj_type_name(CDSObjectType obj_type);
void        _cds_remove_object(void **array, int *nobjects, void *object);

/*****  Data Type Functions  *****/

void       *_cds_data_type_min(CDSDataType type);
double      _cds_data_type_min_double(CDSDataType type);
int         _cds_data_type_mincmp(CDSDataType type1, CDSDataType type2);
void       *_cds_data_type_max(CDSDataType type);
double      _cds_data_type_max_double(CDSDataType type);
int         _cds_data_type_maxcmp(CDSDataType type1, CDSDataType type2);
void       *_cds_default_fill_value(CDSDataType type);

/*****  Utility Functions  *****/

void        _cds_free_array(CDSDataType type, size_t length, void *array);
size_t      _cds_max_strlen(size_t length, char **strpp);
size_t      _cds_total_strlen(size_t length, char **strpp);

/*****  Group Functions  *****/

CDSGroup   *_cds_create_group(CDSGroup *parent, const char *name);
void        _cds_destroy_group(CDSGroup *group);

/*****  Dimension Functions  *****/

CDSDim     *_cds_create_dim(
                CDSGroup   *group,
                const char *name,
                size_t      length,
                int         is_unlimited);

void        _cds_destroy_dim(CDSDim *dim);
int         _cds_is_dim_used(CDSGroup *group, CDSDim *dim);
void        _cds_delete_dependant_vars(CDSGroup *group, CDSDim *dim);

/*****  Attribute Functions  *****/

int         _cds_change_att_value(
                CDSAtt      *att,
                CDSDataType  type,
                size_t       length,
                void        *value);

CDSAtt     *_cds_create_att(
                void        *parent,
                const char  *name,
                CDSDataType  type,
                size_t       length,
                void        *value);

CDSAtt      *_cds_define_att(
                void        *parent,
                const char  *name,
                CDSDataType  type,
                size_t       length,
                void        *value);

void        _cds_destroy_att(CDSAtt *att);

void        _cds_free_att_value(CDSAtt *att);

int         _cds_set_att_value(
                CDSAtt      *att,
                CDSDataType  type,
                size_t       length,
                void        *value);

int         _cds_set_att_va_list(
                CDSAtt     *att,
                const char *format,
                va_list     args);

/*****  Variable Functions  *****/

CDSVar     *_cds_create_var(
                CDSGroup     *group,
                const char   *name,
                CDSDataType   type,
                int           ndims,
                CDSDim      **dims);

void        _cds_destroy_var(CDSVar *var);

/*****  Variable Data Functions  *****/

void       *_cds_create_var_data_index(
                CDSVar *var,
                size_t  sample_start);

void        _cds_delete_var_data_index(CDSVar *var);

/*****  Variable Array Functions  *****/

CDSVarArray *_cds_create_vararray(CDSVarGroup *vargroup, const char *name);
void         _cds_destroy_vararray(CDSVarArray *vararray);

/*****  Variable Group Functions  *****/

CDSVarGroup *_cds_create_vargroup(CDSGroup *group, const char *name);
void         _cds_destroy_vargroup(CDSVarGroup *vargroup);

/*****  Copy Functions *****/

int _cds_copy_att_value(
        CDSConverter  converter,
        int           att_flags,
        CDSAtt       *src_att,
        CDSAtt       *dest_att);

int _cds_copy_att(
        CDSConverter  converter,
        CDSAtt       *src_att,
        void         *dest_parent,
        const char   *dest_name,
        int           flags,
        CDSAtt      **dest_att);

int _cds_copy_atts(
        CDSConverter  converter,
        int           src_natts,
        CDSAtt      **src_atts,
        void         *dest_parent,
        const char  **src_names,
        const char  **dest_names,
        int           flags);

/*****  Print Functions *****/

#define CDS_PRINT_TO_BUFFER(index,length,format,datap,bufp,bufsize,maxline,linepos,indent) \
{ \
    size_t  count  = length - index; \
    size_t  indlen = (indent) ? strlen(indent) : 0 ; \
    char   *bufend = bufp + bufsize - 32; \
    char    string[32]; \
    size_t  nbytes; \
    \
    if (bufsize < 32) return(0); \
    \
    if (maxline) { \
        if (maxline < indlen + 3) maxline += indlen + 2; \
        else maxline -= 1; \
        \
        if (index) { datap += index; count += 1; } \
        else { \
            nbytes   = sprintf(string, format, *datap++); \
            linepos += nbytes; \
            if (linepos > maxline) { \
                *bufp++ = '\n'; \
                if (indlen) { memcpy(bufp, indent, indlen); bufp += indlen; } \
                linepos = indlen + nbytes; \
            } \
            memcpy(bufp, string, nbytes); bufp += nbytes; \
            *bufp++  = ','; linepos += 1; \
        } \
        maxline -= 1; \
        while (--count && bufp < bufend) { \
            nbytes   = sprintf(string, format, *datap++); \
            linepos += nbytes; \
            if (linepos > maxline) { \
                *bufp++ = '\n'; \
                if (indlen) { memcpy(bufp, indent, indlen); bufp += indlen; } \
                linepos = indlen + nbytes; \
            } \
            else { *bufp++ = ' '; linepos += 1; } \
            memcpy(bufp, string, nbytes); bufp += nbytes; \
            *bufp++ = ','; linepos += 1; \
        } \
    } \
    else { \
        if (index) { datap += index; count += 1; } \
        else       { bufp  += sprintf(bufp, format, *datap++); *bufp++ = ','; } \
        while (--count && bufp < bufend) { \
            *bufp++ = ' '; \
            bufp += sprintf(bufp, format, *datap++); *bufp++ = ','; \
        } \
    } \
    index = length - count; \
    if (!count) bufp -= 1; \
    *bufp = '\0'; \
}

int         _cds_print_att_array(FILE *fp, CDSAtt *att);
int         _cds_print_att_array_char(FILE *fp, CDSAtt *att);
int         _cds_print_att_array_string(FILE *fp, const char *indent, CDSAtt *att);

int         _cds_print_data_array(
                FILE        *fp,
                int          line_length,
                CDSDataType  type,
                size_t       start,
                size_t       count,
                CDSData      data);

int         _cds_print_data_array_char(
                FILE   *fp,
                size_t  start,
                size_t  count,
                char   *data);

int         _cds_print_data_array_string(
                FILE        *fp,
                const char  *indent,
                size_t       start,
                size_t       count,
                char       **strpp);

/*****  Transformation Parameters *****/

typedef struct CDSParam {

    char        *name;      /**< parameter name                          */
    CDSDataType  type;      /**< parameter data type                     */
    size_t       length;    /**< length of the parameter value           */
    CDSData      value;     /**< parameter value                         */

} CDSParam;

typedef struct CDSParamList {

    char        *name;      /**< parameter list name                     */
    int          nalloced;  /**< number of parameters allocated          */
    int          nparams;   /**< number of parameters in the list        */
    CDSParam    *params;    /**< array of parameters                     */

} CDSParamList;

typedef struct CDSTransformParams {

    int           nalloced;  /**< number of parameter lists allocates     */
    int           nlists;    /**< number of parameter lists used          */
    CDSParamList *lists;     /**< array of parameter lists                */

} CDSTransformParams;

void _cds_free_transform_params(CDSTransformParams *transform_params);

/*****  Data Conversion Functions *****/

int _cds_has_conversion(CDSConverter converter, int flags);

/**
 *  CDS data converter.
 */
typedef struct _CDSConverter {

    CDSDataType       in_type;    /* data type of the input data              */
    size_t            in_size;    /* size in bytes of the input data type     */
    char             *in_units;   /* units of the input data                  */

    CDSDataType       out_type;   /* data type of the output data             */
    size_t            out_size;   /* size in bytes of the output data type    */
    char             *out_units;  /* units of the output data                 */

    CDSUnitConverter  uc;         /* units converter                          */

    size_t            map_length; /* number of values in the map arrays       */
    void             *in_map;     /* array of input map values                */
    void             *out_map;    /* array of output map values               */
    int               map_ident;  /* flag indicating an identity map          */

    void             *out_min;    /* valid min value in output data           */
    void             *orv_min;    /* value to use for values less than min    */
    void             *out_max;    /* valid max value in output data           */
    void             *orv_max;    /* value to use for values greater than max */

} _CDSConverter;

/*****  Macros *****/

/**
*  Macro for checking if a signed integer = an unsigned integer.
*/
#define EQ_SU(x, y_t, y) \
( (x >= 0) && ( (y_t)x == y ) )

/**
*  Macro for checking if an unsigned integer = a signed integer.
*/
#define EQ_US(x_t, x, y) \
( (y >= 0) && ( x == (x_t)y ) )

/**
*  Macro for checking if a signed integer != an unsigned integer.
*/
#define NEQ_SU(x, y_t, y) \
( (x < 0) || ( (y_t)x != y ) )

/**
*  Macro for checking if an unsigned integer != a signed integer.
*/
#define NEQ_US(x_t, x, y) \
( (y < 0) || ( x != (x_t)y ) )

/**
*  Macro for checking if a signed integer < an unsigned integer.
*/
#define LT_SU(x, y_t, y) \
( (x < 0) || ( (y_t)x < y ) )

/**
*  Macro for checking if an unsigned integer < a signed integer.
*/
#define LT_US(x_t, x, y) \
( (y >= 0) && ( x < (x_t)y ) )

/**
*  Macro for checking if a signed integer <= an unsigned integer.
*/
#define LTEQ_SU(x, y_t, y) \
( (x < 0) || ( (y_t)x <= y ) )

/**
*  Macro for checking if an unsigned integer <= a signed integer.
*/
#define LTEQ_US(x_t, x, y) \
( (y >= 0) && ( x <= (x_t)y ) )

/**
*  Macro for checking if a signed integer > an unsigned integer.
*/
#define GT_SU(x, y_t, y) \
( (x >= 0) && ( (y_t)x > y ) )

/**
*  Macro for checking if an unsigned integer > a signed integer.
*/
#define GT_US(x_t, x, y) \
( (y < 0) || ( x > (x_t)y ) )

/**
*  Macro for checking if a signed integer >= an unsigned integer.
*/
#define GTEQ_SU(x, y_t, y) \
( (x >= 0) && ( (y_t)x >= y ) )

/**
*  Macro for checking if an unsigned integer >= a signed integer.
*/
#define GTEQ_US(x_t, x, y) \
( (y <= 0) || ( x >= (x_t)y ) )

/**
*  Macro used to set the data in a cell boundary variable
*  using constant data offsets.
*/
#define CDS_SET_BOUNDS_DATA(nelems, data, noffsets, offsets, bounds) \
{ \
    int oi, count = nelems + 1; \
    while (--count) { \
        for (oi = 0; oi < noffsets; ++oi) { \
            *bounds++ = *data + offsets[oi]; \
        } \
        ++data; \
    } \
}

/**
*  Macro used to compare the values in two arrays.
*/
#define CDS_COMPARE_ARRAYS(res, len, a1p, a2p, threshp) \
res = 0; \
len++; \
if (threshp) { \
    while (--len) { \
        if (*a1p != *a2p) { \
            if ( *a1p < *a2p) { \
                if (*a2p - *a1p > *threshp) { \
                    res = -1; \
                    break; \
                } \
            } \
            else { \
                if (*a1p - *a2p > *threshp) { \
                    res = 1; \
                    break; \
                } \
            } \
        } \
        ++a1p; ++a2p; \
    } \
} \
else { \
    while (--len) { \
        if (*a1p != *a2p) { \
            res = (*a1p < *a2p) ? -1 : 1; \
            break; \
        } \
        ++a1p; ++a2p; \
    } \
}

/**
*  Macro used to compare the values in two arrays.
*  (compare signed with unsigned)
*/
#define CDS_COMPARE_ARRAYS_SU(res, len, a1p, a2_t, a2p, threshp) \
res = 0; \
len++; \
if (threshp) { \
    while (--len) { \
        if ( NEQ_SU(*a1p, a2_t, *a2p) ) { \
            if ( LT_SU(*a1p, a2_t, *a2p) ) { \
                if (*a2p - *a1p > *threshp) { \
                    res = -1; \
                    break; \
                } \
            } \
            else { \
                if (*a1p - *a2p > *threshp) { \
                    res = 1; \
                    break; \
                } \
            } \
        } \
        ++a1p; ++a2p; \
    } \
} \
else { \
    while (--len) { \
        if ( NEQ_SU(*a1p, a2_t, *a2p) ) { \
            res = ( LT_SU(*a1p, a2_t, *a2p) ) ? -1 : 1; \
            break; \
        } \
        ++a1p; ++a2p; \
    } \
}

/**
*  Macro used to compare the values in two arrays.
*  (compare unsigned with signed)
*/
#define CDS_COMPARE_ARRAYS_US(res, len, a1_t, a1p, a2p, threshp) \
res = 0; \
len++; \
if (threshp) { \
    while (--len) { \
        if ( NEQ_US(a1_t, *a1p, *a2p) ) { \
            if ( LT_US(a1_t, *a1p, *a2p) ) { \
                if (*a2p - *a1p > (a1_t)*threshp) { \
                    res = -1; \
                    break; \
                } \
            } \
            else { \
                if (*a1p - *a2p > (a1_t)*threshp) { \
                    res = 1; \
                    break; \
                } \
            } \
        } \
        ++a1p; ++a2p; \
    } \
} \
else { \
    while (--len) { \
        if ( NEQ_US(a1_t, *a1p, *a2p) ) { \
            res = ( LT_US(a1_t, *a1p, *a2p) ) ? -1 : 1; \
            break; \
        } \
        ++a1p; ++a2p; \
    } \
}

/**
*  Macro used to determine the out-of-range value to use in an output array.
*/
#define CDS_FIND_ORV(nmv, imvp, out_t, ominp, omaxp, orvp, ofillp) \
*orvp = *ofillp; \
for (mi = 0; mi < nmv; ++mi) { \
    if ((imvp[mi] >= *ominp) && (imvp[mi] <= *omaxp)) { \
        *orvp = (out_t)imvp[mi]; \
        break; \
    } \
}

/**
*  Macro used to determine the out-of-range value to use in an output array.
*  (signed to unsigned)
*/
#define CDS_FIND_ORV_SU(nmv, imvp, out_t, ominp, omaxp, orvp, ofillp) \
*orvp = *ofillp; \
for (mi = 0; mi < nmv; ++mi) { \
    if ( (GTEQ_SU(imvp[mi], out_t, *ominp)) && (LTEQ_SU(imvp[mi], out_t, *omaxp)) ) { \
        *orvp = (out_t)imvp[mi]; \
        break; \
    } \
}

/**
*  Macro used to determine the out-of-range value to use in an output array.
*  (unsigned to signed)
*/
#define CDS_FIND_ORV_US(in_t, nmv, imvp, out_t, ominp, omaxp, orvp, ofillp) \
*orvp = *ofillp; \
for (mi = 0; mi < nmv; ++mi) { \
    if ( (GTEQ_US(in_t, imvp[mi], *ominp)) && (LTEQ_US(in_t, imvp[mi], *omaxp)) ) { \
        *orvp = (out_t)imvp[mi]; \
        break; \
    } \
}

/**
*  Macro used in other macros to map missing values to the output array.
*/
#define CDS_MAP_VALUES(inp, nmv, imvp, outp, omvp) \
for (mi = 0; mi < nmv; ++mi) { \
    if (*inp == imvp[mi]) { \
        *outp++ = omvp[mi]; \
        ++inp; \
        break; \
    } \
} \
if (mi != nmv) continue;

/**
*  Macro used to check for below min values in CDS_COPY_ARRAY.
*/
#define CDS_CHECK_MIN(inp, outp, minp, orminp) \
if (*inp < *minp) { \
    *outp++ = *orminp; \
    ++inp; \
    continue; \
}

/**
*  Macro used to check for below min values in CDS_COPY_ARRAY_SU.
*  (signed to unsigned conversion)
*/
#define CDS_CHECK_MIN_SU(out_t, inp, outp, minp, orminp) \
if ( LT_SU(*inp, out_t, *minp) ) { \
    *outp++ = *orminp; \
    ++inp; \
    continue; \
}

/**
*  Macro used to check for below min values in CDS_COPY_ARRAY_US.
*  (unsigned to signed conversion)
*/
#define CDS_CHECK_MIN_US(in_t, inp, outp, minp, orminp) \
if ( LT_US(in_t, *inp, *minp) ) { \
    *outp++ = *orminp; \
    ++inp; \
    continue; \
}

/**
*  Macro used to check for above max values in CDS_COPY_ARRAY.
*/
#define CDS_CHECK_MAX(inp, outp, maxp, ormaxp) \
if (*inp > *maxp) { \
    *outp++ = *ormaxp; \
    ++inp; \
    continue; \
}

/**
*  Macro used to check for above max values in CDS_COPY_ARRAY_SU.
*  (signed to unsigned conversion)
*/
#define CDS_CHECK_MAX_SU(out_t, inp, outp, maxp, ormaxp) \
if ( GT_SU(*inp, out_t, *maxp) ) { \
    *outp++ = *ormaxp; \
    ++inp; \
    continue; \
}

/**
*  Macro used to check for above max values in CDS_COPY_ARRAY_US.
*  (unsigned to signed conversion)
*/
#define CDS_CHECK_MAX_US(in_t, inp, outp, maxp, ormaxp) \
if ( GT_US(in_t, *inp, *maxp) ) { \
    *outp++ = *ormaxp; \
    ++inp; \
    continue; \
}

/**
*  Macro used to copy an array of data.
*/
#define CDS_COPY_ARRAY(len, inp, nmv, imvp, out_t, outp, omvp, minp, orminp, maxp, ormaxp, round) \
len++; \
if (orminp) { \
    if (ormaxp) { \
        if (nmv) { \
            size_t mi; \
            if (round) { \
                while (--len) { \
                    CDS_MAP_VALUES(inp, nmv, imvp, outp, omvp); \
                    CDS_CHECK_MIN(inp, outp, minp, orminp); \
                    CDS_CHECK_MAX(inp, outp, maxp, ormaxp); \
                    *outp++ = (out_t)( (*inp < 0) ? *inp++ - 0.5 : *inp++ + 0.5 ); \
                } \
            } \
            else { \
                while (--len) { \
                    CDS_MAP_VALUES(inp, nmv, imvp, outp, omvp); \
                    CDS_CHECK_MIN(inp, outp, minp, orminp); \
                    CDS_CHECK_MAX(inp, outp, maxp, ormaxp); \
                    *outp++ = (out_t)*inp++; \
                } \
            } \
        } \
        else if (round) { \
            while (--len) { \
                CDS_CHECK_MIN(inp, outp, minp, orminp); \
                CDS_CHECK_MAX(inp, outp, maxp, ormaxp); \
                *outp++ = (out_t)( (*inp < 0) ? *inp++ - 0.5 : *inp++ + 0.5 ); \
            } \
        } \
        else { \
            while (--len) { \
                CDS_CHECK_MIN(inp, outp, minp, orminp); \
                CDS_CHECK_MAX(inp, outp, maxp, ormaxp); \
                *outp++ = (out_t)*inp++; \
            } \
        } \
    } \
    else { \
        if (nmv) { \
            size_t mi; \
            if (round) { \
                while (--len) { \
                    CDS_MAP_VALUES(inp, nmv, imvp, outp, omvp); \
                    CDS_CHECK_MIN(inp, outp, minp, orminp); \
                    *outp++ = (out_t)( (*inp < 0) ? *inp++ - 0.5 : *inp++ + 0.5 ); \
                } \
            } \
            else { \
                while (--len) { \
                    CDS_MAP_VALUES(inp, nmv, imvp, outp, omvp); \
                    CDS_CHECK_MIN(inp, outp, minp, orminp); \
                    *outp++ = (out_t)*inp++; \
                } \
            } \
        } \
        else if (round) { \
            while (--len) { \
                CDS_CHECK_MIN(inp, outp, minp, orminp); \
                *outp++ = (out_t)( (*inp < 0) ? *inp++ - 0.5 : *inp++ + 0.5 ); \
            } \
        } \
        else { \
            while (--len) { \
                CDS_CHECK_MIN(inp, outp, minp, orminp); \
                *outp++ = (out_t)*inp++; \
            } \
        } \
    } \
} \
else if (ormaxp) { \
    if (nmv) { \
        size_t mi; \
        if (round) { \
            while (--len) { \
                CDS_MAP_VALUES(inp, nmv, imvp, outp, omvp); \
                CDS_CHECK_MAX(inp, outp, maxp, ormaxp); \
                *outp++ = (out_t)( (*inp < 0) ? *inp++ - 0.5 : *inp++ + 0.5 ); \
            } \
        } \
        else { \
            while (--len) { \
                CDS_MAP_VALUES(inp, nmv, imvp, outp, omvp); \
                CDS_CHECK_MAX(inp, outp, maxp, ormaxp); \
                *outp++ = (out_t)*inp++; \
            } \
        } \
    } \
    else if (round) { \
        while (--len) { \
            CDS_CHECK_MAX(inp, outp, maxp, ormaxp); \
            *outp++ = (out_t)( (*inp < 0) ? *inp++ - 0.5 : *inp++ + 0.5 ); \
        } \
    } \
    else { \
        while (--len) { \
            CDS_CHECK_MAX(inp, outp, maxp, ormaxp); \
            *outp++ = (out_t)*inp++; \
        } \
    } \
} \
else { \
    if (nmv) { \
        size_t mi; \
        if (round) { \
            while (--len) { \
                CDS_MAP_VALUES(inp, nmv, imvp, outp, omvp); \
                *outp++ = (out_t)( (*inp < 0) ? *inp++ - 0.5 : *inp++ + 0.5 ); \
            } \
        } \
        else { \
            while (--len) { \
                CDS_MAP_VALUES(inp, nmv, imvp, outp, omvp); \
                *outp++ = (out_t)*inp++; \
            } \
        } \
    } \
    else if (round) { \
        while (--len) { \
            *outp++ = (out_t)( (*inp < 0) ? *inp++ - 0.5 : *inp++ + 0.5 ); \
        } \
    } \
    else { \
        while (--len) { \
            *outp++ = (out_t)*inp++; \
        } \
    } \
}

/**
*  Macro used to copy an array of data.
*  (unsigned to float/double conversion)
*/
#define CDS_COPY_ARRAY_U(len, inp, nmv, imvp, out_t, outp, omvp, minp, orminp, maxp, ormaxp) \
len++; \
if (orminp) { \
    if (ormaxp) { \
        if (nmv) { \
            size_t mi; \
            while (--len) { \
                CDS_MAP_VALUES(inp, nmv, imvp, outp, omvp); \
                CDS_CHECK_MIN(inp, outp, minp, orminp); \
                CDS_CHECK_MAX(inp, outp, maxp, ormaxp); \
                *outp++ = (out_t)*inp++; \
            } \
        } \
        else { \
            while (--len) { \
                CDS_CHECK_MIN(inp, outp, minp, orminp); \
                CDS_CHECK_MAX(inp, outp, maxp, ormaxp); \
                *outp++ = (out_t)*inp++; \
            } \
        } \
    } \
    else { \
        if (nmv) { \
            size_t mi; \
            while (--len) { \
                CDS_MAP_VALUES(inp, nmv, imvp, outp, omvp); \
                CDS_CHECK_MIN(inp, outp, minp, orminp); \
                *outp++ = (out_t)*inp++; \
            } \
        } \
        else { \
            while (--len) { \
                CDS_CHECK_MIN(inp, outp, minp, orminp); \
                *outp++ = (out_t)*inp++; \
            } \
        } \
    } \
} \
else if (ormaxp) { \
    if (nmv) { \
        size_t mi; \
        while (--len) { \
            CDS_MAP_VALUES(inp, nmv, imvp, outp, omvp); \
            CDS_CHECK_MAX(inp, outp, maxp, ormaxp); \
            *outp++ = (out_t)*inp++; \
        } \
    } \
    else { \
        while (--len) { \
            CDS_CHECK_MAX(inp, outp, maxp, ormaxp); \
            *outp++ = (out_t)*inp++; \
        } \
    } \
} \
else { \
    if (nmv) { \
        size_t mi; \
        while (--len) { \
            CDS_MAP_VALUES(inp, nmv, imvp, outp, omvp); \
            *outp++ = (out_t)*inp++; \
        } \
    } \
    else { \
        while (--len) { \
            *outp++ = (out_t)*inp++; \
        } \
    } \
}

/**
*  Macro used to copy an array of data.
*  (signed to unsigned conversion)
*/
#define CDS_COPY_ARRAY_SU(len, inp, nmv, imvp, out_t, outp, omvp, minp, orminp, maxp, ormaxp) \
len++; \
if (orminp) { \
    if (ormaxp) { \
        if (nmv) { \
            size_t mi; \
            while (--len) { \
                CDS_MAP_VALUES(inp, nmv, imvp, outp, omvp); \
                CDS_CHECK_MIN_SU(out_t, inp, outp, minp, orminp); \
                CDS_CHECK_MAX_SU(out_t, inp, outp, maxp, ormaxp); \
                *outp++ = (out_t)*inp++; \
            } \
        } \
        else { \
            while (--len) { \
                CDS_CHECK_MIN_SU(out_t, inp, outp, minp, orminp); \
                CDS_CHECK_MAX_SU(out_t, inp, outp, maxp, ormaxp); \
                *outp++ = (out_t)*inp++; \
            } \
        } \
    } \
    else { \
        if (nmv) { \
            size_t mi; \
            while (--len) { \
                CDS_MAP_VALUES(inp, nmv, imvp, outp, omvp); \
                CDS_CHECK_MIN_SU(out_t, inp, outp, minp, orminp); \
                *outp++ = (out_t)*inp++; \
            } \
        } \
        else { \
            while (--len) { \
                CDS_CHECK_MIN_SU(out_t, inp, outp, minp, orminp); \
                *outp++ = (out_t)*inp++; \
            } \
        } \
    } \
} \
else if (ormaxp) { \
    if (nmv) { \
        size_t mi; \
        while (--len) { \
            CDS_MAP_VALUES(inp, nmv, imvp, outp, omvp); \
            CDS_CHECK_MAX_SU(out_t, inp, outp, maxp, ormaxp); \
            *outp++ = (out_t)*inp++; \
        } \
    } \
    else { \
        while (--len) { \
            CDS_CHECK_MAX_SU(out_t, inp, outp, maxp, ormaxp); \
            *outp++ = (out_t)*inp++; \
        } \
    } \
} \
else { \
    if (nmv) { \
        size_t mi; \
        while (--len) { \
            CDS_MAP_VALUES(inp, nmv, imvp, outp, omvp); \
            *outp++ = (out_t)*inp++; \
        } \
    } \
    else { \
        while (--len) { \
            *outp++ = (out_t)*inp++; \
        } \
    } \
}

/**
*  Macro used to copy an array of data.
*  (unsigned to signed conversion)
*/
#define CDS_COPY_ARRAY_US(in_t, len, inp, nmv, imvp, out_t, outp, omvp, minp, orminp, maxp, ormaxp) \
len++; \
if (orminp) { \
    if (ormaxp) { \
        if (nmv) { \
            size_t mi; \
            while (--len) { \
                CDS_MAP_VALUES(inp, nmv, imvp, outp, omvp); \
                CDS_CHECK_MIN_US(in_t, inp, outp, minp, orminp); \
                CDS_CHECK_MAX_US(in_t, inp, outp, maxp, ormaxp); \
                *outp++ = (out_t)*inp++; \
            } \
        } \
        else { \
            while (--len) { \
                CDS_CHECK_MIN_US(in_t, inp, outp, minp, orminp); \
                CDS_CHECK_MAX_US(in_t, inp, outp, maxp, ormaxp); \
                *outp++ = (out_t)*inp++; \
            } \
        } \
    } \
    else { \
        if (nmv) { \
            size_t mi; \
            while (--len) { \
                CDS_MAP_VALUES(inp, nmv, imvp, outp, omvp); \
                CDS_CHECK_MIN_US(in_t, inp, outp, minp, orminp); \
                *outp++ = (out_t)*inp++; \
            } \
        } \
        else { \
            while (--len) { \
                CDS_CHECK_MIN_US(in_t, inp, outp, minp, orminp); \
                *outp++ = (out_t)*inp++; \
            } \
        } \
    } \
} \
else if (ormaxp) { \
    if (nmv) { \
        size_t mi; \
        while (--len) { \
            CDS_MAP_VALUES(inp, nmv, imvp, outp, omvp); \
            CDS_CHECK_MAX_US(in_t, inp, outp, maxp, ormaxp); \
            *outp++ = (out_t)*inp++; \
        } \
    } \
    else { \
        while (--len) { \
            CDS_CHECK_MAX_US(in_t, inp, outp, maxp, ormaxp); \
            *outp++ = (out_t)*inp++; \
        } \
    } \
} \
else { \
    if (nmv) { \
        size_t mi; \
        while (--len) { \
            CDS_MAP_VALUES(inp, nmv, imvp, outp, omvp); \
            *outp++ = (out_t)*inp++; \
        } \
    } \
    else { \
        while (--len) { \
            *outp++ = (out_t)*inp++; \
        } \
    } \
}

/**
*  Macro used to check for below min values in CDS_CONVERT_UNITS_FLOAT.
*/
#define CDS_CHECK_MIN_FLOAT(fval, outp, minp, orminp) \
if (fval < *minp) { \
    *outp++ = *orminp; \
    continue; \
}

/**
*  Macro used to check for above max values in CDS_CONVERT_UNITS_FLOAT.
*/
#define CDS_CHECK_MAX_FLOAT(fval, outp, maxp, ormaxp) \
if (fval > *maxp) { \
    *outp++ = *ormaxp; \
    continue; \
}

/**
*  Macro used to convert the units of an array using single precision.
*/
#define CDS_CONVERT_UNITS_FLOAT(uc, len, inp, nmv, imvp, out_t, outp, omvp, minp, orminp, maxp, ormaxp, round) \
len++; \
if (orminp) { \
    float fval; \
    if (ormaxp) { \
        if (nmv) { \
            size_t mi; \
            if (round) { \
                while (--len) { \
                    CDS_MAP_VALUES(inp, nmv, imvp, outp, omvp); \
                    fval  = cv_convert_float(uc, (float)*inp++); \
                    fval += (fval < 0) ? - 0.5 : + 0.5; \
                    CDS_CHECK_MIN_FLOAT(fval, outp, minp, orminp); \
                    CDS_CHECK_MAX_FLOAT(fval, outp, maxp, ormaxp); \
                    *outp++ = (out_t)fval; \
                } \
            } \
            else { \
                while (--len) { \
                    CDS_MAP_VALUES(inp, nmv, imvp, outp, omvp); \
                    fval  = cv_convert_float(uc, (float)*inp++); \
                    CDS_CHECK_MIN_FLOAT(fval, outp, minp, orminp); \
                    CDS_CHECK_MAX_FLOAT(fval, outp, maxp, ormaxp); \
                    *outp++ = (out_t)fval; \
                } \
            } \
        } \
        else if (round) { \
            while (--len) { \
                fval  = cv_convert_float(uc, (float)*inp++); \
                fval += (fval < 0) ? - 0.5 : + 0.5; \
                CDS_CHECK_MIN_FLOAT(fval, outp, minp, orminp); \
                CDS_CHECK_MAX_FLOAT(fval, outp, maxp, ormaxp); \
                *outp++ = (out_t)fval; \
            } \
        } \
        else { \
            while (--len) { \
                fval  = cv_convert_float(uc, (float)*inp++); \
                CDS_CHECK_MIN_FLOAT(fval, outp, minp, orminp); \
                CDS_CHECK_MAX_FLOAT(fval, outp, maxp, ormaxp); \
                *outp++ = (out_t)fval; \
            } \
        } \
    } \
    else { \
        float fval; \
        if (nmv) { \
            size_t mi; \
            if (round) { \
                while (--len) { \
                    CDS_MAP_VALUES(inp, nmv, imvp, outp, omvp); \
                    fval  = cv_convert_float(uc, (float)*inp++); \
                    fval += (fval < 0) ? - 0.5 : + 0.5; \
                    CDS_CHECK_MIN_FLOAT(fval, outp, minp, orminp); \
                    *outp++ = (out_t)fval; \
                } \
            } \
            else { \
                while (--len) { \
                    CDS_MAP_VALUES(inp, nmv, imvp, outp, omvp); \
                    fval  = cv_convert_float(uc, (float)*inp++); \
                    CDS_CHECK_MIN_FLOAT(fval, outp, minp, orminp); \
                    *outp++ = (out_t)fval; \
                } \
            } \
        } \
        else if (round) { \
            while (--len) { \
                fval  = cv_convert_float(uc, (float)*inp++); \
                fval += (fval < 0) ? - 0.5 : + 0.5; \
                CDS_CHECK_MIN_FLOAT(fval, outp, minp, orminp); \
                *outp++ = (out_t)fval; \
            } \
        } \
        else { \
            while (--len) { \
                fval  = cv_convert_float(uc, (float)*inp++); \
                CDS_CHECK_MIN_FLOAT(fval, outp, minp, orminp); \
                *outp++ = (out_t)fval; \
            } \
        } \
    }\
} \
else if (ormaxp) { \
    float fval; \
    if (nmv) { \
        size_t mi; \
        if (round) { \
            while (--len) { \
                CDS_MAP_VALUES(inp, nmv, imvp, outp, omvp); \
                fval  = cv_convert_float(uc, (float)*inp++); \
                fval += (fval < 0) ? - 0.5 : + 0.5; \
                CDS_CHECK_MAX_FLOAT(fval, outp, maxp, ormaxp); \
                *outp++ = (out_t)fval; \
            } \
        } \
        else { \
            while (--len) { \
                CDS_MAP_VALUES(inp, nmv, imvp, outp, omvp); \
                fval  = cv_convert_float(uc, (float)*inp++); \
                CDS_CHECK_MAX_FLOAT(fval, outp, maxp, ormaxp); \
                *outp++ = (out_t)fval; \
            } \
        } \
    } \
    else if (round) { \
        while (--len) { \
            fval  = cv_convert_float(uc, (float)*inp++); \
            fval += (fval < 0) ? - 0.5 : + 0.5; \
            CDS_CHECK_MAX_FLOAT(fval, outp, maxp, ormaxp); \
            *outp++ = (out_t)fval; \
        } \
    } \
    else { \
        while (--len) { \
            fval  = cv_convert_float(uc, (float)*inp++); \
            CDS_CHECK_MAX_FLOAT(fval, outp, maxp, ormaxp); \
            *outp++ = (out_t)fval; \
        } \
    } \
} \
else { \
    if (nmv) { \
        size_t mi; \
        if (round) { \
            float fval; \
            while (--len) { \
                CDS_MAP_VALUES(inp, nmv, imvp, outp, omvp); \
                fval  = cv_convert_float(uc, (float)*inp++); \
                fval += (fval < 0) ? - 0.5 : + 0.5; \
                *outp++ = (out_t)fval; \
            } \
        } \
        else { \
            while (--len) { \
                CDS_MAP_VALUES(inp, nmv, imvp, outp, omvp); \
                *outp++ = (out_t)cv_convert_float(uc, (float)*inp++); \
            } \
        } \
    } \
    else if (round) { \
        float fval; \
        while (--len) { \
            fval  = cv_convert_float(uc, (float)*inp++); \
            fval += (fval < 0) ? - 0.5 : + 0.5; \
            *outp++ = (out_t)fval; \
        } \
    } \
    else { \
        while (--len) { \
            *outp++ = (out_t)cv_convert_float(uc, (float)*inp++); \
        } \
    } \
}

/**
*  Macro used to check for below min values in CDS_CONVERT_UNITS_DOUBLE.
*/
#define CDS_CHECK_MIN_DOUBLE(dval, outp, minp, orminp) \
if (dval < *minp) { \
    *outp++ = *orminp; \
    continue; \
}

/**
*  Macro used to check for above max values in CDS_CONVERT_UNITS_DOUBLE.
*/
#define CDS_CHECK_MAX_DOUBLE(dval, outp, maxp, ormaxp) \
if (dval > *maxp) { \
    *outp++ = *ormaxp; \
    continue; \
}

/**
*  Macro used to convert the units of an array using double precision.
*/
#define CDS_CONVERT_UNITS_DOUBLE(uc, len, inp, nmv, imvp, out_t, outp, omvp, minp, orminp, maxp, ormaxp, round) \
len++; \
if (orminp) { \
    double dval; \
    if (ormaxp) { \
        if (nmv) { \
            size_t mi; \
            if (round) { \
                while (--len) { \
                    CDS_MAP_VALUES(inp, nmv, imvp, outp, omvp); \
                    dval  = cv_convert_double(uc, (double)*inp++); \
                    dval += (dval < 0) ? - 0.5 : + 0.5; \
                    CDS_CHECK_MIN_DOUBLE(dval, outp, minp, orminp); \
                    CDS_CHECK_MAX_DOUBLE(dval, outp, maxp, ormaxp); \
                    *outp++ = (out_t)dval; \
                } \
            } \
            else { \
                while (--len) { \
                    CDS_MAP_VALUES(inp, nmv, imvp, outp, omvp); \
                    dval  = cv_convert_double(uc, (double)*inp++); \
                    CDS_CHECK_MIN_DOUBLE(dval, outp, minp, orminp); \
                    CDS_CHECK_MAX_DOUBLE(dval, outp, maxp, ormaxp); \
                    *outp++ = (out_t)dval; \
                } \
            } \
        } \
        else if (round) { \
            while (--len) { \
                dval  = cv_convert_double(uc, (double)*inp++); \
                dval += (dval < 0) ? - 0.5 : + 0.5; \
                CDS_CHECK_MIN_DOUBLE(dval, outp, minp, orminp); \
                CDS_CHECK_MAX_DOUBLE(dval, outp, maxp, ormaxp); \
                *outp++ = (out_t)dval; \
            } \
        } \
        else { \
            while (--len) { \
                dval  = cv_convert_double(uc, (double)*inp++); \
                CDS_CHECK_MIN_DOUBLE(dval, outp, minp, orminp); \
                CDS_CHECK_MAX_DOUBLE(dval, outp, maxp, ormaxp); \
                *outp++ = (out_t)dval; \
            } \
        } \
    } \
    else { \
        double dval; \
        if (nmv) { \
            size_t mi; \
            if (round) { \
                while (--len) { \
                    CDS_MAP_VALUES(inp, nmv, imvp, outp, omvp); \
                    dval  = cv_convert_double(uc, (double)*inp++); \
                    dval += (dval < 0) ? - 0.5 : + 0.5; \
                    CDS_CHECK_MIN_DOUBLE(dval, outp, minp, orminp); \
                    *outp++ = (out_t)dval; \
                } \
            } \
            else { \
                while (--len) { \
                    CDS_MAP_VALUES(inp, nmv, imvp, outp, omvp); \
                    dval  = cv_convert_double(uc, (double)*inp++); \
                    CDS_CHECK_MIN_DOUBLE(dval, outp, minp, orminp); \
                    *outp++ = (out_t)dval; \
                } \
            } \
        } \
        else if (round) { \
            while (--len) { \
                dval  = cv_convert_double(uc, (double)*inp++); \
                dval += (dval < 0) ? - 0.5 : + 0.5; \
                CDS_CHECK_MIN_DOUBLE(dval, outp, minp, orminp); \
                *outp++ = (out_t)dval; \
            } \
        } \
        else { \
            while (--len) { \
                dval  = cv_convert_double(uc, (double)*inp++); \
                CDS_CHECK_MIN_DOUBLE(dval, outp, minp, orminp); \
                *outp++ = (out_t)dval; \
            } \
        } \
    } \
}\
else if (ormaxp) { \
    double dval; \
    if (nmv) { \
        size_t mi; \
        if (round) { \
            while (--len) { \
                CDS_MAP_VALUES(inp, nmv, imvp, outp, omvp); \
                dval  = cv_convert_double(uc, (double)*inp++); \
                dval += (dval < 0) ? - 0.5 : + 0.5; \
                CDS_CHECK_MAX_DOUBLE(dval, outp, maxp, ormaxp); \
                *outp++ = (out_t)dval; \
            } \
        } \
        else { \
            while (--len) { \
                CDS_MAP_VALUES(inp, nmv, imvp, outp, omvp); \
                dval  = cv_convert_double(uc, (double)*inp++); \
                CDS_CHECK_MAX_DOUBLE(dval, outp, maxp, ormaxp); \
                *outp++ = (out_t)dval; \
            } \
        } \
    } \
    else if (round) { \
        while (--len) { \
            dval  = cv_convert_double(uc, (double)*inp++); \
            dval += (dval < 0) ? - 0.5 : + 0.5; \
            CDS_CHECK_MAX_DOUBLE(dval, outp, maxp, ormaxp); \
            *outp++ = (out_t)dval; \
        } \
    } \
    else { \
        while (--len) { \
            dval  = cv_convert_double(uc, (double)*inp++); \
            CDS_CHECK_MAX_DOUBLE(dval, outp, maxp, ormaxp); \
            *outp++ = (out_t)dval; \
        } \
    } \
} \
else { \
    if (nmv) { \
        size_t mi; \
        if (round) { \
            double dval; \
            while (--len) { \
                CDS_MAP_VALUES(inp, nmv, imvp, outp, omvp); \
                dval  = cv_convert_double(uc, (double)*inp++); \
                dval += (dval < 0) ? - 0.5 : + 0.5; \
                *outp++ = (out_t)dval; \
            } \
        } \
        else { \
            while (--len) { \
                CDS_MAP_VALUES(inp, nmv, imvp, outp, omvp); \
                *outp++ = (out_t)cv_convert_double(uc, (double)*inp++); \
            } \
        } \
    } \
    else if (round) { \
        double dval; \
        while (--len) { \
            dval  = cv_convert_double(uc, (double)*inp++); \
            dval += (dval < 0) ? - 0.5 : + 0.5; \
            *outp++ = (out_t)dval; \
        } \
    } \
    else { \
        while (--len) { \
            *outp++ = (out_t)cv_convert_double(uc, (double)*inp++); \
        } \
    } \
}

/**
*  Macro used to convert the unit deltas of an array using single precision.
*/
#define CDS_CONVERT_DELTAS_FLOAT(uc, len, inp, nmv, imvp, out_t, outp, omvp, round) \
len++; \
if (nmv) { \
    size_t mi; float fval; \
    if (round) { \
        while (--len) { \
            CDS_MAP_VALUES(inp, nmv, imvp, outp, omvp); \
            fval = cv_convert_float(uc, (float)*inp * 2.0) \
                 - cv_convert_float(uc, (float)*inp); \
            fval += (fval < 0) ? - 0.5 : + 0.5; \
            *outp++ = (out_t)fval; \
            ++inp; \
        } \
    } \
    else { \
        while (--len) { \
            CDS_MAP_VALUES(inp, nmv, imvp, outp, omvp); \
            fval = cv_convert_float(uc, (float)*inp * 2.0) \
                 - cv_convert_float(uc, (float)*inp); \
            *outp++ = (out_t)fval; \
            ++inp; \
        } \
    } \
} \
else if (round) { \
    float fval; \
    while (--len) { \
        fval = cv_convert_float(uc, (float)*inp * 2.0) \
             - cv_convert_float(uc, (float)*inp); \
        fval += (fval < 0) ? - 0.5 : + 0.5; \
        *outp++ = (out_t)fval; \
        ++inp; \
    } \
} \
else { \
    float fval; \
    while (--len) { \
        fval = cv_convert_float(uc, (float)*inp * 2.0) \
             - cv_convert_float(uc, (float)*inp); \
        *outp++ = (out_t)fval; \
        ++inp; \
    } \
}

/**
*  Macro used to convert the unit deltas of an array using double precision.
*/
#define CDS_CONVERT_DELTAS_DOUBLE(uc, len, inp, nmv, imvp, out_t, outp, omvp, round) \
len++; \
if (nmv) { \
    size_t mi; double dval; \
    if (round) { \
        while (--len) { \
            CDS_MAP_VALUES(inp, nmv, imvp, outp, omvp); \
            dval = cv_convert_double(uc, (double)*inp * 2.0) \
                 - cv_convert_double(uc, (double)*inp); \
            dval += (dval < 0) ? - 0.5 : + 0.5; \
            *outp++ = (out_t)dval; \
            ++inp; \
        } \
    } \
    else { \
        while (--len) { \
            CDS_MAP_VALUES(inp, nmv, imvp, outp, omvp); \
            dval = cv_convert_double(uc, (double)*inp * 2.0) \
                 - cv_convert_double(uc, (double)*inp); \
            *outp++ = (out_t)dval; \
            ++inp; \
        } \
    } \
} \
else if (round) { \
    double dval; \
    while (--len) { \
        dval = cv_convert_double(uc, (double)*inp * 2.0) \
             - cv_convert_double(uc, (double)*inp); \
        dval += (dval < 0) ? - 0.5 : + 0.5; \
        *outp++ = (out_t)dval; \
        ++inp; \
    } \
} \
else { \
    double dval; \
    while (--len) { \
        dval = cv_convert_double(uc, (double)*inp * 2.0) \
             - cv_convert_double(uc, (double)*inp); \
        *outp++ = (out_t)dval; \
        ++inp; \
    } \
}

/**
*  Macro used to check for a missing value in QC checks.
*/
#define CDS_QC_MISSING_CHECK_1() \
if (value == missing) { *flagsp++ |= missing_flag;  continue; }

/**
*  Macro used to check for multiple missing values in QC checks.
*/
#define CDS_QC_MISSING_CHECK_N() \
for (mi = 0; mi < nmissings; ++mi) { \
    if (value == missings[mi]) { \
        *flagsp++ |= missing_flags[mi]; \
        break; \
    } \
} \
if (mi != nmissings) continue;

/**
*  Macro used to check for below min values in QC checks.
*/
#define CDS_QC_MIN_CHECK() \
if (value < min) *flagsp |= min_flag;

/**
*  Macro used to check for above max values in QC checks.
*/
#define CDS_QC_MAX_CHECK() \
if (value > max) *flagsp |= max_flag;

/**
*  Macro used to check for below min or above max values in QC checks.
*/
#define CDS_QC_MIN_MAX_CHECK() \
if      (value < min) *flagsp |= min_flag; \
else if (value > max) *flagsp |= max_flag;

/**
*  Macro used to perform missing, min, and max value QC checks.
*/
#define CDS_QC_LIMITS_CHECK(data_t) \
{ \
    data_t *datap    = (data_t *)data_vp; \
    data_t *missings = (data_t *)NULL; \
    data_t  min      = 0; \
    data_t  max      = 0; \
    data_t  missing; \
    int     missing_flag; \
    data_t  value; \
\
    if (nmissings && missings_vp) missings =   (data_t *)missings_vp; \
    if (min_vp)                   min      = *((data_t *)min_vp); \
    if (max_vp)                   max      = *((data_t *)max_vp); \
\
    nvalues++; \
    if (missings) { \
        if (nmissings == 1) { \
            missing      = *missings; \
            missing_flag = *missing_flags; \
            if (min_vp) { \
                if (max_vp) { \
                    /* missing, min, and max checks */ \
                    while (--nvalues) { \
                        value = *datap++; \
                        CDS_QC_MISSING_CHECK_1(); \
                        CDS_QC_MIN_MAX_CHECK(); \
                        ++flagsp; \
                    } \
                } \
                else { \
                    /* missing and min checks */ \
                    while (--nvalues) { \
                        value = *datap++; \
                        CDS_QC_MISSING_CHECK_1(); \
                        CDS_QC_MIN_CHECK(); \
                        ++flagsp; \
                    } \
                } \
            } \
            else { \
                if (max_vp) { \
                    /* missing and max checks */ \
                    while (--nvalues) { \
                        value = *datap++; \
                        CDS_QC_MISSING_CHECK_1(); \
                        CDS_QC_MAX_CHECK(); \
                        ++flagsp; \
                    } \
                } \
                else { \
                    /* missing checks */ \
                    while (--nvalues) { \
                        value = *datap++; \
                        CDS_QC_MISSING_CHECK_1(); \
                        ++flagsp; \
                    } \
                } \
            } \
        } \
        else { \
            if (min_vp) { \
                if (max_vp) { \
                    /* missing, min, and max checks */ \
                    while (--nvalues) { \
                        value = *datap++; \
                        CDS_QC_MISSING_CHECK_N(); \
                        CDS_QC_MIN_MAX_CHECK(); \
                        ++flagsp; \
                    } \
                } \
                else { \
                    /* missing and min checks */ \
                    while (--nvalues) { \
                        value = *datap++; \
                        CDS_QC_MISSING_CHECK_N(); \
                        CDS_QC_MIN_CHECK(); \
                        ++flagsp; \
                    } \
                } \
            } \
            else { \
                if (max_vp) { \
                    /* missing and max checks */ \
                    while (--nvalues) { \
                        value = *datap++; \
                        CDS_QC_MISSING_CHECK_N(); \
                        CDS_QC_MAX_CHECK(); \
                        ++flagsp; \
                    } \
                } \
                else { \
                    /* missing checks */ \
                    while (--nvalues) { \
                        value = *datap++; \
                        CDS_QC_MISSING_CHECK_N(); \
                        ++flagsp; \
                    } \
                } \
            } \
        } \
    } \
    else { \
        if (min_vp) { \
            if (max_vp) { \
                /* min and max checks */ \
                while (--nvalues) { \
                    value = *datap++; \
                    CDS_QC_MIN_MAX_CHECK(); \
                    ++flagsp; \
                } \
            } \
            else { \
                /* min checks */ \
                while (--nvalues) { \
                    value = *datap++; \
                    CDS_QC_MIN_CHECK(); \
                    ++flagsp; \
                } \
            } \
        } \
        else { \
            if (max_vp) { \
                /* max checks */ \
                while (--nvalues) { \
                    value = *datap++; \
                    CDS_QC_MAX_CHECK(); \
                    ++flagsp; \
                } \
            } \
            else { \
                /* no checks */ \
            } \
        } \
    } \
}

/**
*  Macro used to perform min and max delta time offset QC checks.
*/
#define CDS_QC_TIME_OFFSETS_CHECK(data_t) \
{ \
    data_t *offsets   = (data_t *)offsets_vp; \
    data_t  min_delta = 0; \
    data_t  max_delta = 0; \
    data_t  prev_offset; \
    data_t  offset; \
    data_t  delta_t; \
\
    if (min_delta_vp) min_delta = *((data_t *)min_delta_vp); \
    if (max_delta_vp) max_delta = *((data_t *)max_delta_vp); \
\
    if (prev_offset_vp) { \
        prev_offset = *((data_t *)prev_offset_vp); \
        noffsets++; \
    } \
    else { \
        prev_offset = *offsets++; \
        ++flagsp; \
    } \
\
    if (min_delta_vp) { \
        if (max_delta_vp) { \
            /* min and max delta checks */ \
            while (--noffsets) { \
                offset = *offsets++; \
                delta_t = offset - prev_offset; \
                if      (delta_t <= 0)        *flagsp |= lteq_zero_flag; \
                else if (delta_t < min_delta) *flagsp |= min_delta_flag; \
                else if (delta_t > max_delta) *flagsp |= max_delta_flag; \
                prev_offset = offset; \
                ++flagsp; \
            } \
        } \
        else { \
            /* min delta checks */ \
            while (--noffsets) { \
                offset = *offsets++; \
                delta_t = offset - prev_offset; \
                if      (delta_t <= 0)        *flagsp |= lteq_zero_flag; \
                else if (delta_t < min_delta) *flagsp |= min_delta_flag; \
                prev_offset = offset; \
                ++flagsp; \
            } \
        } \
    } \
    else { \
        if (max_delta_vp) { \
            /* max delta checks */ \
            while (--noffsets) { \
                offset = *offsets++; \
                delta_t = offset - prev_offset; \
                if      (delta_t <= 0)        *flagsp |= lteq_zero_flag; \
                else if (delta_t > max_delta) *flagsp |= max_delta_flag; \
                prev_offset = offset; \
                ++flagsp; \
            } \
        } \
        else { \
            /* <= 0 check only */ \
            while (--noffsets) { \
                offset = *offsets++; \
                delta_t = offset - prev_offset; \
                if (delta_t <= 0) *flagsp |= lteq_zero_flag; \
                prev_offset = offset; \
                ++flagsp; \
            } \
        } \
    } \
}

/**
*  Macro used by cds_qc_delta_checks() to perform QC delta checks across
*  an array of data with only 1 dimension.
*/
#define CDS_QC_DELTA_CHECKS_1D_1(data_t) \
{ \
    data_t  max_delta   = *((data_t *)deltas_vp); \
    int     delta_flag  = *delta_flags; \
    data_t *valuep      = (data_t *)data_vp; \
    int    *flagsp      = qc_flags; \
    data_t  value; \
    data_t  prev_value; \
    int     prev_flags; \
\
    if (max_delta > 0) { \
\
        if (prev_sample_vp) { \
            prev_value = *((data_t *)prev_sample_vp); \
            prev_flags = *prev_qc_flags; \
            ++nvalues; \
        } \
        else { \
            prev_value = *valuep++; \
            prev_flags = *flagsp++; \
        } \
\
        while (--nvalues) { \
\
            value = *valuep++; \
\
            if (!(*flagsp    & bad_flags) && \
                !(prev_flags & bad_flags)) { \
\
                if (value > prev_value) { \
                    if (value - prev_value > max_delta) { \
                        *flagsp |= delta_flag; \
                    } \
                } \
                else { \
                    if (prev_value - value > max_delta) { \
                        *flagsp |= delta_flag; \
                    } \
                } \
            } \
\
            prev_value =  value; \
            prev_flags = *flagsp++; \
        } \
    } \
}

/**
*  Macro used by cds_qc_delta_checks() to perform sample to sample QC delta
*  checks for arrays that have more than 1 dimension.
*/
#define CDS_QC_DELTA_CHECKS_1D_N(data_t) \
{ \
    data_t  max_delta    = *((data_t *)deltas_vp); \
    int     delta_flag   = *delta_flags; \
    data_t *sample       = (data_t *)data_vp; \
    int    *sample_flags = qc_flags; \
\
    data_t *prev_sample; \
    int    *prev_sample_flags; \
\
    data_t *prev_valuep; \
    int    *prev_flagsp; \
    data_t *valuep; \
    int    *flagsp; \
\
    data_t  value; \
    data_t  prev_value; \
    int     prev_flags; \
\
    if (max_delta > 0) { \
\
        if (prev_sample_vp) { \
            prev_sample       = (data_t *)prev_sample_vp; \
            prev_sample_flags = prev_qc_flags; \
            ++sample_count; \
        } \
        else { \
            prev_sample        = sample; \
            prev_sample_flags  = sample_flags; \
            sample            += sample_size; \
            sample_flags      += sample_size; \
        } \
\
        while (--sample_count) { \
\
            prev_valuep = prev_sample; \
            prev_flagsp = prev_sample_flags; \
            valuep      = sample; \
            flagsp      = sample_flags; \
            nvalues     = sample_size + 1; \
\
            while (--nvalues) { \
\
                value      = *valuep++; \
                prev_value = *prev_valuep++; \
                prev_flags = *prev_flagsp++; \
\
                if (!(*flagsp    & bad_flags) && \
                    !(prev_flags & bad_flags)) { \
\
                    if (value > prev_value) { \
                        if (value - prev_value > max_delta) { \
                            *flagsp |= delta_flag; \
                        } \
                    } \
                    else { \
                        if (prev_value - value > max_delta) { \
                            *flagsp |= delta_flag; \
                        } \
                    } \
                } \
\
                ++flagsp; \
\
            } \
\
            prev_sample        = sample; \
            prev_sample_flags  = sample_flags; \
            sample            += sample_size; \
            sample_flags      += sample_size; \
        } \
    } \
}

/**
*  Macro used by cds_qc_delta_checks() to perform N-dimensional QC delta checks.
*/
#define CDS_QC_DELTA_CHECKS_ND(data_t) \
{ \
    data_t  max_delta; \
    int     delta_flag; \
    size_t  start; \
    size_t  stride; \
\
    data_t *prev_valuep; \
    int    *prev_flagsp; \
    data_t *valuep; \
    int    *flagsp; \
\
    size_t  d1, d2; \
\
    strides[0]       = dim_lengths[ndims-1]; \
    strides[ndims-1] = 1; \
\
    if (ndims > 2) { \
        for (di = ndims-2; di > 0; --di) { \
            strides[di] = strides[0]; \
            strides[0] *= dim_lengths[di]; \
        } \
    } \
\
    for (d1 = 1; d1 < ndeltas; ++d1) { \
\
        max_delta  = ((data_t *)deltas_vp)[d1]; \
        delta_flag = delta_flags[d1]; \
\
        if (max_delta <= 0) { \
            continue; \
        } \
\
        memset(index, 0, ndims * sizeof(size_t)); \
\
        for (;;) { \
\
            stride = strides[d1]; \
            start  = 0; \
\
            for (d2 = 0; d2 < ndims; ++d2) { \
                if (d2 != d1) { \
                    start += index[d2] * strides[d2]; \
                } \
            } \
\
            prev_valuep = (data_t *)data_vp + start; \
            prev_flagsp = qc_flags + start; \
            valuep      = prev_valuep + stride; \
            flagsp      = prev_flagsp + stride; \
            nvalues     = dim_lengths[d1]; \
\
            while (--nvalues) { \
\
                if (!(*flagsp      & bad_flags) && \
                    !(*prev_flagsp & bad_flags)) { \
\
                    if (*valuep > *prev_valuep) { \
                        if (*valuep - *prev_valuep > max_delta) { \
                            *flagsp |= delta_flag; \
                        } \
                    } \
                    else { \
                        if (*prev_valuep - *valuep > max_delta) { \
                            *flagsp |= delta_flag; \
                        } \
                    } \
                } \
\
                prev_valuep  = valuep; \
                prev_flagsp  = flagsp; \
                valuep      += stride; \
                flagsp      += stride; \
            } \
\
            for (d2 = ndims-1; d2 > 0; --d2) { \
\
                if (d2 == d1) { \
                    continue; \
                } \
\
                if (++index[d2] != dim_lengths[d2]) { \
                    break; \
                } \
\
                index[d2] = 0; \
            } \
\
            if (d2 == 0) { \
                if (++index[0] == dim_lengths[0]) { \
                    break; \
                } \
            } \
        } \
    } \
}

#endif /* _CDS_PRIVATE_ */
