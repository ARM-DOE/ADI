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

#include "libcds3_test.h"

extern const char *gProgramName;
extern FILE       *gLogFP;
extern CDSGroup   *gRoot;

typedef struct DimDef   DimDef;
typedef struct AttDef   AttDef;
typedef struct VarDef   VarDef;
typedef struct GroupDef GroupDef;

struct DimDef {
    char   *name;
    size_t  length;
    int     is_unlimited;
};

struct AttDef {
    char        *name;
    CDSDataType  type;
    size_t       length;
    void        *value;
};

struct VarDef {
    char         *name;
    CDSDataType   type;
    char        **dim_names;
    AttDef       *atts;
    int           nsamples;
    void         *data;
    char         *vargroup;
    char         *vararray;
};

struct GroupDef {
    char     *name;
    DimDef   *dims;
    AttDef   *atts;
    VarDef   *vars;
    GroupDef *groups;
};

/*******************************************************************************
*  Test Data
*/

#define NSAMPLES 10

static unsigned char CharData[] = {
    'a', 'b', 'c', 'y', 'z', 'A', 'B', 'C', 'Y', 'Z'
};

static signed char ByteData[] = {
    -128, -64, -32, -16, -8, 8,  16,  32,  64, 127
};

static short ShortData[] = {
    -32768, -16384, -8192, -4096, -2048, 2048, 4096, 8192, 16384, 32767
};

static int IntData[] = {
    -2147483648, -268435456, -33554432, -4194304, -524288,
    524288, 4194304, 33554432, 268435456, 2147483647
};

static float FloatData[] = {
    -1234567.0, -12345.67, -123.4567, -1.234567, -0.01234567,
    0.01234567, 1.234567, 123.4567, 12345.67, 1234567.0
};

static double DoubleData[] = {
    -123456789123456.0, -12345678912.3456, -1234567.89123456, -123.456789123456,
    -0.0123456789123456, 0.0123456789123456, 123.456789123456, 1234567.89123456,
    12345678912.3456, 123456789123456.0
};

long long Int64Data[] = {
    CDS_MIN_INT64, -4611686018427387904LL, -2147483648, -32768, -128,
    127, 32767, 2147483647, 4611686018427387904LL, 9223372036854775807LL
};

unsigned char UByteData[] = {
    0, 2, 3, 4, 8, 16, 32, 64, 128, 255
};

unsigned short UShortData[] = {
    0, 127, 128, 255, 256, 1024, 8192, 16384, 32768, 65535
};

unsigned int UIntData[] = {
    0, 127, 128, 255, 256, 65535, 65536, 2147483647, 2147483648, 4294967295U
};

unsigned long long UInt64Data[] ={
    0, 127, 128, 65535, 65536, 2147483647, 2147483648, 4294967295U, 4294967296U, 18446744073709551615ULL
};

static char *StringData[] = {
    "string 1", "string 2", "string 3", "string 4", "string 5",
    "string 6", "string 7", "string 8", "string 9", "string 10"
};

static char CharAtt[] =
    "Single line text attribute.";

static char MultiLineCharAtt[] =
    "Multi line text attribute:\n"
    "    - Line 1\n"
    "    - Line 2\n"
    "    - Line 3";

static char *StringAtt[] = {
    "string array value 1", "string array value 2", "string array value 3"
};

static timeval_t TimeData[] = {
    { 1234567890, 999995 },
    { 1234567890, 999996 },
    { 1234567890, 999997 },
    { 1234567890, 999998 },
    { 1234567890, 999999 },
    { 1234567891, 000000 },
    { 1234567891, 000001 },
    { 1234567891, 000002 },
    { 1234567891, 000003 },
    { 1234567891, 000004 }
};

/*******************************************************************************
*  Dimension Definitions
*/

static DimDef Dims_1[] = {
    { "dim_delete_1", 10, 0 },
    { "dim_1_1",       0, 1 },
    { "dim_1_2",      20, 0 },
    { NULL,            0, 0 }
};

static DimDef Dims_2[] = {
    { "dim_2_1",       0, 1 },
    { "dim_delete_2", 20, 0 },
    { "dim_2_2",      20, 0 },
    { NULL,            0, 0 }
};

static DimDef Dims_3[] = {
    { "dim_3_1",       0, 1 },
    { "dim_3_2",      20, 0 },
    { "dim_delete_3", 30, 0 },
    { NULL,            0, 0 }
};

/*******************************************************************************
*  Attribute Definitions
*/

static AttDef TypeAtts[] = {
    { "att_char",       CDS_CHAR,   NSAMPLES,  (void *)CharData },
    { "att_byte",       CDS_BYTE,   NSAMPLES,  (void *)ByteData },
    { "att_short",      CDS_SHORT,  NSAMPLES,  (void *)ShortData },
    { "att_int",        CDS_INT,    NSAMPLES,  (void *)IntData },
    { "att_float",      CDS_FLOAT,  NSAMPLES,  (void *)FloatData },
    { "att_double",     CDS_DOUBLE, NSAMPLES,  (void *)DoubleData },
    { "att_text",       CDS_CHAR,   0,         (void *)CharAtt },
    { "att_multi_line", CDS_CHAR,   0,         (void *)MultiLineCharAtt },
    { "att_int64",      CDS_INT64,  NSAMPLES,  (void *)Int64Data },
    { "att_ubyte",      CDS_UBYTE,  NSAMPLES,  (void *)UByteData },
    { "att_ushort",     CDS_USHORT, NSAMPLES,  (void *)UShortData },
    { "att_uint",       CDS_UINT,   NSAMPLES,  (void *)UIntData },
    { "att_uint64",     CDS_UINT64, NSAMPLES,  (void *)UInt64Data },
    { "att_string",     CDS_STRING, 3,         (void *)StringAtt },
    { NULL,             CDS_NAT,    0,         NULL }
};

static AttDef Atts_1[] = {
    { "att_delete_1",   CDS_CHAR, 0, NULL },
    { "att_1_1",        CDS_CHAR, 0, NULL },
    { "att_1_2",        CDS_CHAR, 0, NULL },
    { NULL,             CDS_NAT,  0, NULL }
};

static AttDef Atts_2[] = {
    { "att_2_1",        CDS_CHAR, 0, NULL },
    { "att_delete_2",   CDS_CHAR, 0, NULL },
    { "att_2_2",        CDS_CHAR, 0, NULL },
    { NULL,             CDS_NAT,  0, NULL }
};

static AttDef Atts_3[] = {
    { "att_3_1",        CDS_CHAR, 0, NULL },
    { "att_3_2",        CDS_CHAR, 0, NULL },
    { "att_delete_3",   CDS_CHAR, 0, NULL },
    { NULL,             CDS_NAT,  0, NULL }
};

static AttDef StdAtts[] = {
    { "long_name", CDS_CHAR, 0, NULL },
    { "units",     CDS_CHAR, 0, NULL },
    { NULL,        CDS_NAT,  0, NULL }
};

/*******************************************************************************
*  Variable Definitions
*/

static char *VarDims_1_1[] = { "dim_1_1", NULL };
static char *VarDims_1_2[] = { "time", "dim_1_2", NULL };
static char *VarDims_2_1[] = { "dim_2_1", NULL };
static char *VarDims_2_2[] = { "time", "dim_2_2", NULL };
static char *VarDims_3_1[] = { "dim_3_1", NULL };
static char *VarDims_3_2[] = { "time", "dim_3_2", NULL };

static char *DelVarDims_1[] = { "root_dim_delete", NULL };
static char *DelVarDims_2[] = { "dim_1_1", "root_dim_delete", NULL };

static VarDef Vars_1[] = {
    { "var_delete_1",   CDS_CHAR,  VarDims_1_1,  StdAtts, 0, NULL, NULL, NULL },
    { "var_1_1",        CDS_CHAR,  VarDims_1_1,  Atts_1,  0, NULL, "vargroup_3", "vararray_1" },
    { "var_1_2",        CDS_BYTE,  VarDims_1_2,  Atts_2,  0, NULL, "vargroup_3", "vararray_1" },
    { "var_dimdel_1_1", CDS_INT,   DelVarDims_1, StdAtts, 0, NULL, NULL, NULL },
    { "var_dimdel_1_2", CDS_FLOAT, DelVarDims_2, StdAtts, 0, NULL, NULL, NULL },
    { NULL,             CDS_NAT,   NULL,         NULL,    0, NULL, NULL, NULL }
};

static VarDef Vars_2[] = {
    { "var_2_1",        CDS_SHORT, VarDims_2_1,  Atts_1,  0, NULL, "vargroup_3", "vararray_2" },
    { "var_delete_2",   CDS_INT,   VarDims_2_1,  StdAtts, 0, NULL, NULL, NULL },
    { "var_2_2",        CDS_INT,   VarDims_2_2,  Atts_2,  0, NULL, "vargroup_3", "vararray_2" },
    { NULL,             CDS_NAT,   NULL,         NULL,    0, NULL, NULL, NULL }
};

static VarDef Vars_3[] = {
    { "var_3_1",        CDS_FLOAT,  VarDims_3_1, Atts_1,  0, NULL, "vargroup_3", "vararray_3" },
    { "var_3_2",        CDS_DOUBLE, VarDims_3_2, Atts_3,  0, NULL, "vargroup_3", "vararray_3" },
    { "var_delete_3",   CDS_DOUBLE, VarDims_3_1, StdAtts, 0, NULL, NULL, NULL },
    { NULL,             CDS_NAT,    NULL,        NULL,    0, NULL, NULL, NULL }
};

/*******************************************************************************
*  Subgroup Definitions
*/

static GroupDef Subgroups_1[] = {

   { "group_delete_1", NULL, NULL, NULL, NULL },
   { "group_1_1",      NULL, NULL, NULL, NULL },
   { "group_1_2",      NULL, NULL, NULL, NULL },

   { NULL, NULL, NULL, NULL, NULL }
};

static GroupDef Subgroups_2[] = {

   { "group_2_1",      NULL, NULL, NULL, NULL },
   { "group_delete_2", NULL, NULL, NULL, NULL },
   { "group_2_2",      NULL, NULL, NULL, NULL },

   { NULL, NULL, NULL, NULL, NULL }
};

static GroupDef Subgroups_3[] = {

   { "group_3_1",      NULL, NULL, NULL, NULL },
   { "group_3_2",      NULL, NULL, NULL, NULL },
   { "group_delete_3", NULL, NULL, NULL, NULL },

   { NULL, NULL, NULL, NULL, NULL }
};

/*******************************************************************************
*  Root Group Definition
*/

static DimDef RootDims[] = {
    { "time",             0, 1 },
    { "range",           38, 0 },
    { "string",          16, 0 },
    { "static",          10, 0 },
    { "root_dim_delete",  5, 0 },
    { NULL,               0, 0 }
};

static char *TimeDim[]        = { "time",   NULL  };
static char *RangeDim[]       = { "range",  NULL };
static char *StaticDim[]      = { "static", NULL };
static char *TimeRangeDims[]  = { "time", "range",  NULL };
static char *TimeStringDims[] = { "time", "string", NULL };

static VarDef RootVars[] = {

    { "time",              CDS_DOUBLE, TimeDim,   NULL,     NSAMPLES, TimeData,   "vargroup_1", "vararray_1" },
    { "range",             CDS_FLOAT,  RangeDim,  StdAtts,  0,        NULL,       "vargroup_1", "vararray_2" },

    { "var_char_static",   CDS_CHAR,   StaticDim, StdAtts,  NSAMPLES, CharData,   "vargroup_2", "vararray_1" },
    { "var_byte_static",   CDS_BYTE,   StaticDim, StdAtts,  NSAMPLES, ByteData,   "vargroup_2", "vararray_2" },
    { "var_short_static",  CDS_SHORT,  StaticDim, StdAtts,  NSAMPLES, ShortData,  "vargroup_2", "vararray_3" },
    { "var_int_static",    CDS_INT,    StaticDim, StdAtts,  NSAMPLES, IntData,    "vargroup_2", "vararray_4" },
    { "var_float_static",  CDS_FLOAT,  StaticDim, StdAtts,  NSAMPLES, FloatData,  "vargroup_2", "vararray_5" },
    { "var_double_static", CDS_DOUBLE, StaticDim, StdAtts,  NSAMPLES, DoubleData, "vargroup_2", "vararray_6" },

    { "var_char",          CDS_CHAR,   TimeDim,   StdAtts,  NSAMPLES, CharData,   "vargroup_2", "vararray_1" },
    { "var_byte",          CDS_BYTE,   TimeDim,   StdAtts,  NSAMPLES, ByteData,   "vargroup_2", "vararray_2" },
    { "var_short",         CDS_SHORT,  TimeDim,   StdAtts,  NSAMPLES, ShortData,  "vargroup_2", "vararray_3" },
    { "var_int",           CDS_INT,    TimeDim,   TypeAtts, NSAMPLES, IntData,    "vargroup_2", "vararray_4" },
    { "var_float",         CDS_FLOAT,  TimeDim,   StdAtts,  NSAMPLES, FloatData,  "vargroup_2", "vararray_5" },
    { "var_double",        CDS_DOUBLE, TimeDim,   StdAtts,  NSAMPLES, DoubleData, "vargroup_2", "vararray_6" },

    { "var_int64",         CDS_INT64,  TimeDim,   StdAtts,  NSAMPLES, Int64Data,  "vargroup_2", "vararray_4" },
    { "var_ubyte",         CDS_UBYTE,  TimeDim,   StdAtts,  NSAMPLES, UByteData,  "vargroup_2", "vararray_2" },
    { "var_ushort",        CDS_USHORT, TimeDim,   StdAtts,  NSAMPLES, UShortData, "vargroup_2", "vararray_3" },
    { "var_uint",          CDS_UINT,   TimeDim,   StdAtts,  NSAMPLES, UIntData,   "vargroup_2", "vararray_4" },
    { "var_uint64",        CDS_UINT64, TimeDim,   StdAtts,  NSAMPLES, UInt64Data, "vargroup_2", "vararray_4" },
    { "var_string",        CDS_STRING, TimeDim,   StdAtts,  NSAMPLES, StringData, "vargroup_2", "vararray_1" },

    { "var_2D",            CDS_DOUBLE, TimeRangeDims,  StdAtts,  0,   NULL,       "vargroup_2", "vararray_6" },
    { "var_char_2D",       CDS_CHAR,   TimeStringDims, StdAtts,  0,   NULL,       "vargroup_2", "vararray_1" },
    { NULL,                CDS_NAT,    NULL,           NULL,     0,   NULL,       NULL, NULL }
};

static GroupDef RootSubgroups[] = {

   { "group_1", Dims_1, Atts_1, Vars_1, Subgroups_1 },
   { "group_2", Dims_2, Atts_2, Vars_2, Subgroups_2 },
   { "group_3", Dims_3, Atts_3, Vars_3, Subgroups_3 },

   { NULL, NULL, NULL, NULL, NULL }
};

static GroupDef RootDef[] = {
   { "root", RootDims, TypeAtts, RootVars, RootSubgroups },

   { NULL, NULL, NULL, NULL, NULL }
};

/*******************************************************************************
 *  Define Dims
 */

static int define_dims(CDSGroup *group, DimDef *dim_defs)
{
    DimDef *def;
    CDSDim *dim;
    CDSDim *check;
    int     di;

    for (di = 0; dim_defs[di].name; di++) {

        def = &dim_defs[di];

        dim = cds_define_dim(group, def->name, def->length, def->is_unlimited);
        if (!dim) return(0);

        LOG( gProgramName,
            "defined:    %s\n", cds_get_object_path(dim));

        check = cds_get_dim(group, def->name);
        if (!check || check != dim) {

            ERROR( gProgramName,
                "Failed cds_get_dim() check\n");

            return(0);
        }

        /* Redefining a dimension should return the existing dimension */

        check = cds_define_dim(group, def->name, def->length, def->is_unlimited);
        if (!check || check != dim) {

            ERROR( gProgramName,
                "Redefining a dimension should return the existing dimension\n");

            return(0);
        }
    }

    return(1);
}

/*******************************************************************************
 *  Define Atts
 */

static int define_atts(void *parent, AttDef *att_defs)
{
    AttDef    *def;
    CDSAtt    *att;
    CDSAtt    *check;
    int        ai;

    for (ai = 0; att_defs[ai].name; ai++) {

        def = &att_defs[ai];

        if (def->type == CDS_CHAR && def->length == 0) {
            if (def->value) {
                att = cds_define_att_text(parent, def->name, def->value);
            }
            else {
                att = cds_define_att_text(parent, def->name,
                    "%s attribute value", def->name);
            }
        }
        else {
            att = cds_define_att(
                parent, def->name, def->type, def->length, def->value);
        }

        if (!att) return(0);

        LOG( gProgramName,
            "defined:    %s\n", cds_get_object_path(att));

        check = cds_get_att(parent, def->name);
        if (!check || check != att) {

            ERROR( gProgramName,
                "Failed cds_get_att() check\n");

            return(0);
        }

        /* Redefining an attribute should return the existing attribute */

        if (def->type == CDS_CHAR && def->length == 0) {
            if (def->value) {
                check = cds_define_att_text(parent, def->name, def->value);
            }
            else {
                check = cds_define_att_text(parent, def->name,
                    "%s attribute value", def->name);
            }
        }
        else {
            check = cds_define_att(
                parent, def->name, def->type, def->length, def->value);
        }

        if (!check || check != att) {

            ERROR( gProgramName,
                "Redefining an attribute should return the existing attribute\n");

            return(0);
        }
    }

    return(1);
}

/*******************************************************************************
 *  Define Vars
 */

static int define_vars(CDSGroup *group, VarDef *var_defs)
{
    VarDef       *def;
    CDSVar       *var;
    CDSVar       *check;
    CDSVarGroup  *vargroup;
    int           ndims;
    int           vi;
    double      **var_2D;
    size_t        si;
    size_t        i, j;

    for (vi = 0; var_defs[vi].name; vi++) {

        def = &var_defs[vi];

        ndims = 0;
        if (def->dim_names) {
            for (; def->dim_names[ndims]; ndims++);
        }

        var = cds_define_var(
            group, def->name, def->type, ndims, (const char **)def->dim_names);

        if (!var) return(0);

        LOG( gProgramName,
            "defined:    %s\n", cds_get_object_path(var));

        check = cds_get_var(group, def->name);
        if (!check || check != var) {

            ERROR( gProgramName,
                "Failed cds_get_var() check\n");

            return(0);
        }

        /* Redefining a variable should return the existing variable */

        check = cds_define_var(
            group, def->name, def->type, ndims, (const char **)def->dim_names);

        if (!check || check != var) {

            ERROR( gProgramName,
                "Redefining a variable should return the existing variable\n");

            return(0);
        }

        /* Define variable attributes */

        if (def->atts) {
            if (!define_atts(var, def->atts)) {
                return(0);
            }
        }

        /* Define variable data */

        if (def->nsamples) {

            if (strcmp(def->name, "time") == 0) {

                if (!cds_set_sample_timevals(
                    group, 0, def->nsamples, (timeval_t *)def->data)) {

                    return(0);
                }
            }
            else {

                if (!cds_put_var_data(
                    var, 0, def->nsamples, def->type, def->data)) {

                    return(0);
                }
            }

            LOG( gProgramName,
                "added data: %s\n", cds_get_object_path(var));
        }
        else if (strcmp(def->name, "var_2D") == 0) {

            for (si = 0; si < var->dims[0]->length; si += 2) {

                var_2D = (double **)cds_alloc_var_data_index(var, si, 2);

                for (i = 0; i < 2; i++) {

                    var_2D[i][0] = (si + i) * 100;

                    for (j = 1; j < var->dims[1]->length; j++) {
                        var_2D[i][j] = var_2D[i][j-1] + 1;
                    }
                }
            }
        }

        /* Add variable to variable group */

        if (def->vargroup && def->vararray) {

            vargroup = cds_define_vargroup(gRoot, def->vargroup);
            if (!vargroup) {
                return(0);
            }

            if (!cds_add_vargroup_vars(vargroup, def->vararray, 1, &var)) {
                return(0);
            }
        }
    }

    return(1);
}

/*******************************************************************************
 *  Define Groups
 */

static int define_groups(CDSGroup *parent, GroupDef *group_defs)
{
    GroupDef *def;
    CDSGroup *group;
    CDSGroup *check;
    int       gi;

    for (gi = 0; group_defs[gi].name; gi++) {

        /* Define group */

        def = &group_defs[gi];

        group = cds_define_group(parent, def->name);
        if (!group) {
            return(0);
        }

        LOG( gProgramName,
            "defined:    %s\n", cds_get_object_path(group));

        if (parent) {

            check = cds_get_group(parent, def->name);
            if (!check || check != group) {

                ERROR( gProgramName,
                    "Failed cds_get_group() check\n");

                return(0);
            }

            /* Redefining a group should return the existing group */

            check = cds_define_group(parent, def->name);
            if (!check || check != group) {

                ERROR( gProgramName,
                    "Redefining a group should return the existing group\n");

                return(0);
            }
        }
        else {
            gRoot = group;
        }

        /* Define dimensions */

        if (def->dims) {
            if (!define_dims(group, def->dims)) {
                return(0);
            }
        }

        /* Define attributes */

        if (def->atts) {
            if (!define_atts(group, def->atts)) {
                return(0);
            }
        }

        /* Define variables */

        if (def->vars) {
            if (!define_vars(group, def->vars)) {
                return(0);
            }
        }

        /* Recurse into subgroups */

        if (def->groups) {
            if (!define_groups(group, def->groups)) {
                return(0);
            }
        }
    }

    return(1);
}

/*******************************************************************************
 *  Delete Dims
 */

static int delete_dims(CDSGroup *group)
{
    CDSDim  *dim;
    CDSDim  *check;
    char     dim_name[32];
    int      di;

    for (di = 0; di < group->ndims; di++) {

        dim = group->dims[di];

        if (strstr(dim->name, "delete")) {

            sprintf(dim_name, dim->name);

            LOG( gProgramName,
                "deleting:   %s\n", cds_get_object_path(dim));

            if (!cds_delete_dim(dim)) {
                return(0);
            }

            check = cds_get_dim(group, dim_name);
            if (check) {

                ERROR( gProgramName,
                    "Failed cds_get_dim() check\n");

                return(0);
            }

            di--;
        }
    }

    return(1);
}

/*******************************************************************************
 *  Delete Atts
 */

static int delete_atts(void *parent)
{
    CDSObject  *parent_object = (CDSObject *)parent;
    CDSGroup   *group;
    CDSVar     *var;
    int        *nattsp;
    CDSAtt   ***attsp;
    CDSAtt     *att;
    CDSAtt     *check;
    char        att_name[32];
    int         ai;

    if (parent_object->obj_type == CDS_GROUP) {

        group  = (CDSGroup *)parent;
        nattsp = &(group->natts);
        attsp  = &(group->atts);
    }
    else if (parent_object->obj_type == CDS_VAR) {

        var    = (CDSVar *)parent_object;
        nattsp = &(var->natts);
        attsp  = &(var->atts);
    }
    else {
        return(0);
    }

    for (ai = 0; ai < *nattsp; ai++) {

        att = (*attsp)[ai];

        if (strstr(att->name, "delete")) {

            sprintf(att_name, att->name);

            LOG( gProgramName,
                "deleting:   %s\n", cds_get_object_path(att));

            if (!cds_delete_att(att)) {
                return(0);
            }

            check = cds_get_att(parent, att_name);
            if (check) {

                ERROR( gProgramName,
                    "Failed cds_get_att() check\n");

                return(0);
            }

            ai--;
        }
    }

    return(1);
}

/*******************************************************************************
 *  Delete Vars
 */

static int delete_vars(CDSGroup *group)
{
    CDSVar  *var;
    CDSVar  *check;
    char     var_name[32];
    int      vi;

    for (vi = 0; vi < group->nvars; vi++) {

        var = group->vars[vi];

        if (strstr(var->name, "delete")) {

            sprintf(var_name, var->name);

            LOG( gProgramName,
                "deleting:   %s\n", cds_get_object_path(var));

            if (!cds_delete_var(var)) {
                return(0);
            }

            check = cds_get_var(group, var_name);
            if (check) {

                ERROR( gProgramName,
                    "Failed cds_get_var() check\n");

                return(0);
            }

            vi--;
        }
        else {

            if (!delete_atts(var)) {
                return(0);
            }
        }
    }

    return(1);
}

/*******************************************************************************
 *  Delete Objects
 */

static int delete_objects(CDSGroup *group)
{
    CDSGroup *subgroup;
    CDSGroup *check;
    char      subgroup_name[32];
    int       gi;

    /* Delete dimensions */

    if (!delete_dims(group)) {
        return(0);
    }

    /* Delete attributes */

    if (!delete_atts(group)) {
        return(0);
    }

    /* Delete variables */

    if (!delete_vars(group)) {
        return(0);
    }

    /* Recurse into subgroups */

    for (gi = 0; gi < group->ngroups; gi++) {

        subgroup = group->groups[gi];

        if (strstr(subgroup->name, "delete")) {

            sprintf(subgroup_name, subgroup->name);

            LOG( gProgramName,
                "deleting:   %s\n", cds_get_object_path(subgroup));

            if (!cds_delete_group(subgroup)) {
                return(0);
            }

            check = cds_get_group(group, subgroup_name);
            if (check) {

                ERROR( gProgramName,
                    "Failed cds_get_group() check\n");

                return(0);
            }

            gi--;
        }
        else if (!delete_objects(subgroup)) {
            return(0);
        }
    }

    return(1);
}

/*******************************************************************************
 *  Define Tests
 */

static int define_tests(void)
{
    LOG( gProgramName,
        "\n============================================================\n"
        "Define Tests\n"
        "============================================================\n\n");

    if (!define_groups(NULL, RootDef)) {
        return(0);
    }

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "Dataset After Defines\n"
        "------------------------------------------------------------\n");

    cds_print(gLogFP, gRoot, CDS_PRINT_VARGROUPS);

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "Variable Groups\n"
        "------------------------------------------------------------\n");

    cds_print_vargroups(gLogFP, "", gRoot, 0);

    return(1);
}

/*******************************************************************************
 *  Test Error Handling
 */

static int error_tests(void)
{
    CDSGroup *group;
    CDSGroup *subgroup;
    CDSDim   *dim;
    CDSAtt   *att;
    CDSVar   *var;
    CDSAtt   *var_att;
    char      group_name[32];
    char      subgroup_name[32];
    char      dim_name[32];
    char      att_name[32];
    char      var_name[32];
    char     *var_dims[2];
    char      var_att_name[32];

    LOG( gProgramName,
        "\n============================================================\n"
        "Group Error Handling Tests\n"
        "============================================================\n");

    /* Get group */

    sprintf(group_name, "group_1");

    group = cds_get_group(gRoot, group_name);
    if (!group) {

        ERROR( gProgramName,
            "Failed to get group '/%s/%s'.\n",
            gRoot->name, group_name);

        return(0);
    }

    /* Get subgroup */

    sprintf(subgroup_name, "group_1_1");

    subgroup = cds_get_group(group, subgroup_name);
    if (!subgroup) {

        ERROR( gProgramName,
            "Failed to get subgroup '/%s/%s/%s'.\n",
            gRoot->name, group_name, subgroup_name);

        return(0);
    }

    /* Get dimension */

    sprintf(dim_name, "dim_1_2");

    dim = cds_get_dim(group, dim_name);
    if (!dim) {

        ERROR( gProgramName,
            "Failed to get dimension '/%s/%s.%s'.\n",
            gRoot->name, group_name, dim_name);

        return(0);
    }

    /* Get attribute */

    sprintf(att_name, "att_1_1");

    att = cds_get_att(group, att_name);
    if (!att) {

        ERROR( gProgramName,
            "Failed to get attribute '/%s/%s.%s'.\n",
            gRoot->name, group_name, att_name);

        return(0);
    }

    /* Get variable */

    sprintf(var_name, "var_1_1");

    var = cds_get_var(group, var_name);
    if (!var) {

        ERROR( gProgramName,
            "Failed to get variable '/%s/%s.%s'.\n",
            gRoot->name, group_name, var_name);

        return(0);
    }

    /* Get variable attribute */

    sprintf(var_att_name, "att_1_1");

    var_att = cds_get_att(var, var_att_name);
    if (!var_att) {

        ERROR( gProgramName,
            "Failed to get attribute '/%s/%s.%s.%s'.\n",
            gRoot->name, group_name, var->name, var_att_name);

        return(0);
    }

    /***************************************************************************
     * Group Error Handling Tests
     **************************************************************************/

    /* Deleting a locked group should fail */

    cds_set_definition_lock(group, 1);

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "- Deleting a locked group should fail.\n\n");

    if (cds_delete_group(group)) {

        ERROR( gProgramName,
            "Failed test for: '/%s/%s'\n",
            gRoot->name, group_name);

        return(0);
    }

    /* Defining a subgroup in a locked group should fail */

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "- Defining a subgroup in a locked group should fail.\n\n");

    if (cds_define_group(group, "subgroup_def_in_locked_group")) {

        ERROR( gProgramName,
            "Failed test for: '/%s/%s/%s'\n",
            gRoot->name, group_name, "subgroup_def_in_locked_group");

        return(0);
    }

    /* Deleting a subgroup from a locked group should fail */

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "- Deleting a subgroup from a locked group should fail.\n\n");

    if (cds_delete_group(subgroup)) {

        ERROR( gProgramName,
            "Failed test for: '/%s/%s/%s'\n",
            gRoot->name, group_name, subgroup_name);

        return(0);
    }

    /* Defining a dimension in a locked group should fail */

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "- Defining a dimension in a locked group should fail.\n\n");

    if (cds_define_dim(group, "dim_def_in_locked_group", 10, 0)) {

        ERROR( gProgramName,
            "Failed test for: '/%s/%s.%s'\n",
            gRoot->name, group_name, "dim_def_in_locked_group");

        return(0);
    }

    /* Deleting a dimension from a locked group should fail */

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "- Deleting a dimension from a locked group should fail.\n\n");

    if (cds_delete_dim(dim)) {

        ERROR( gProgramName,
            "Failed test for: '/%s/%s.%s'\n",
            gRoot->name, group_name, dim_name);

        return(0);
    }

    /* Defining an attribute in a locked group should fail */

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "- Defining an attribute in a locked group should fail.\n\n");

    if (cds_define_att_text(group, "att_def_in_locked_group", "att value")) {

        ERROR( gProgramName,
            "Failed test for: '/%s/%s.%s'\n",
            gRoot->name, group_name, "att_def_in_locked_group");

        return(0);
    }

    /* Deleting an attribute from a locked group should fail */

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "- Deleting an attribute from a locked group should fail.\n\n");

    if (cds_delete_att(att)) {

        ERROR( gProgramName,
            "Failed test for: '/%s/%s.%s'\n",
            gRoot->name, group_name, att_name);

        return(0);
    }

    /* Defining a variable in a locked group should fail */

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "- Defining a variable in a locked group should fail.\n\n");

    if (cds_define_var(group, "var_def_in_locked_group", CDS_INT, 0, NULL)) {

        ERROR( gProgramName,
            "Failed test for: '/%s/%s.%s'\n",
            gRoot->name, group_name, "var_def_in_locked_group");

        return(0);
    }

    /* Deleting a variable from a locked group should fail */

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "- Deleting a variable from a locked group should fail.\n\n");

    if (cds_delete_var(var)) {

        ERROR( gProgramName,
            "Failed test for: '/%s/%s.%s'\n",
            gRoot->name, group_name, var_name);

        return(0);
    }

    cds_set_definition_lock(group, 0);

    /***************************************************************************
     * Dimension Error Handling Tests
     **************************************************************************/

    LOG( gProgramName,
        "\n============================================================\n"
        "Dimension Error Handling Tests\n"
        "============================================================\n");

    /* Defining a dimension that already exists should fail */

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "- Defining a dimension that already exists should fail.\n\n");

    if (cds_define_dim(group, dim_name, 10, 0)) {

        ERROR( gProgramName,
            "Failed test for: '/%s/%s.%s'\n",
            gRoot->name, group_name, dim_name);

        return(0);
    }

    /* Deleting a locked dimension should fail */

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "- Deleting a locked dimension should fail.\n\n");

    cds_set_definition_lock(dim, 1);

    if (cds_delete_dim(dim)) {

        ERROR( gProgramName,
            "Failed test for: '/%s/%s.%s'\n",
            gRoot->name, group_name, dim_name);

        return(0);
    }

    /* Changing the length of a locked dimension should fail */

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "- Changing the length of a locked dimension should fail.\n\n");

    if (cds_change_dim_length(dim, 30)) {

        ERROR( gProgramName,
            "Failed test for: '/%s/%s.%s'\n",
            gRoot->name, group_name, dim_name);

        return(0);
    }

    cds_set_definition_lock(dim, 0);

    /***************************************************************************
     * Attribute Error Handling Tests
     **************************************************************************/

    LOG( gProgramName,
        "\n============================================================\n"
        "Attribute Error Handling Tests\n"
        "============================================================\n");

    /* Defining an attribute that already exists should fail */

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "- Defining an attribute that already exists should fail.\n\n");

    if (cds_define_att_text(group, att_name, "att value")) {

        ERROR( gProgramName,
            "Failed test for: '/%s/%s.%s'\n",
            gRoot->name, group_name, att_name);

        return(0);
    }

    /* Deleting a locked attribute should fail */

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "- Deleting a locked attribute should fail.\n\n");

    cds_set_definition_lock(att, 1);

    if (cds_delete_att(att)) {

        ERROR( gProgramName,
            "Failed test for: '/%s/%s.%s'\n",
            gRoot->name, group_name, att_name);

        return(0);
    }

    /* Changing the value of a locked attribute should fail */

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "- Changing the value of a locked attribute should fail.\n\n");

    if (cds_change_att_text(att, "changed att value")) {

        ERROR( gProgramName,
            "Failed test for: '/%s/%s.%s'\n",
            gRoot->name, group_name, att_name);

        return(0);
    }

    cds_set_definition_lock(att, 0);

    /***************************************************************************
     * Variable Error Handling Tests
     **************************************************************************/

    LOG( gProgramName,
        "\n============================================================\n"
        "Variable Error Handling Tests\n"
        "============================================================\n");

    /* Defining a variable that already exists should fail */

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "- Defining a variable that already exists should fail.\n\n");

    if (cds_define_var(group, var_name, CDS_INT, 0, NULL)) {

        ERROR( gProgramName,
            "Failed test for: '/%s/%s.%s'\n",
            gRoot->name, group_name, var_name);

        return(0);
    }

    /* Defining a variable with an undefined dimension should fail */

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "- Defining a variable with an undefined dimension should fail.\n\n");

    var_dims[0] = dim_name;
    var_dims[1] = "undefined_dim";

    if (cds_define_var(group, "var_with_undef_dim", CDS_FLOAT, 2, (const char **)var_dims)) {

        ERROR( gProgramName,
            "Failed test for: '/%s/%s.%s'\n",
            gRoot->name, group_name, "var_with_undef_dim");

        return(0);
    }

    /* The record dimension must be first in a variable definition */

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "- The record dimension must be first in a variable definition.\n\n");

    var_dims[0] = "range";
    var_dims[1] = "time";

    if (cds_define_var(group, "var_with_unlim_dim_last", CDS_INT, 2, (const char **)var_dims)) {

        ERROR( gProgramName,
            "Failed test for: '/%s/%s.%s'\n",
            gRoot->name, group_name, "var_with_unlim_dim_last");

        return(0);
    }

    /* Deleting a locked variable should fail */

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "- Deleting a locked variable should fail.\n\n");

    cds_set_definition_lock(var, 1);

    if (cds_delete_var(var)) {

        ERROR( gProgramName,
            "Failed test for: '/%s/%s.%s'\n",
            gRoot->name, group_name, var_name);

        return(0);
    }

    /* Changing the data type of a locked variable should fail */

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "- Changing the data type of a locked variable should fail.\n\n");

    if (cds_change_var_type(var, CDS_DOUBLE)) {

        ERROR( gProgramName,
            "Failed test for: '/%s/%s.%s'\n",
            gRoot->name, group_name, var_name);

        return(0);
    }

    /* Defining an attribute in a locked variable should fail */

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "- Defining an attribute in a locked variable should fail.\n\n");

    if (cds_define_att_text(var, "att_def_in_locked_var", "att value")) {

        ERROR( gProgramName,
            "Failed test for: '/%s/%s.%s.%s'\n",
            gRoot->name, group_name, var_name, "att_def_in_locked_var");

        return(0);
    }

    /* Deleting an attribute from a locked variable should fail */

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "- Deleting an attribute from a locked variable should fail.\n\n");

    if (cds_delete_att(var_att)) {

        ERROR( gProgramName,
            "Failed test for: '/%s/%s.%s.%s'\n",
            gRoot->name, group_name, var_name, var_att_name);

        return(0);
    }

    cds_set_definition_lock(var, 0);

    return(1);
}

/*******************************************************************************
 *  Delete Tests
 */

static int delete_tests(void)
{
    LOG( gProgramName,
        "\n============================================================\n"
        "Delete Tests\n"
        "============================================================\n\n");

    if (!delete_objects(gRoot))
        return(0);

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "Dataset After Deletes\n"
        "------------------------------------------------------------\n");

    cds_print(gLogFP, gRoot, CDS_PRINT_VARGROUPS);

    return(1);
}

/*******************************************************************************
 *  Run Definition Tests
 */

void libcds3_test_defines(void)
{
    fprintf(stdout, "\nStructure Definition Tests:\n");

    run_test(" - define_tests", "define_tests", define_tests);
    run_test(" - delete_tests", "delete_tests", delete_tests);
    run_test(" - error_tests",  "error_tests",  error_tests);

}
