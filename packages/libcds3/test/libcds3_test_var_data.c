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
*    $Revision: 60277 $
*    $Author: ermold $
*    $Date: 2015-02-15 00:42:57 +0000 (Sun, 15 Feb 2015) $
*    $Version:$
*
*******************************************************************************/

#include "libcds3_test.h"

extern const char *gProgramName;
extern CDSGroup   *gRoot;
extern FILE       *gLogFP;

static void _print_missings(CDSVar *var)
{
    int     nmissing;
    double *missing;
    int     mi;

    nmissing = cds_get_var_missing_values(var, (void **)&missing);
    if (!nmissing) {
        fprintf(gLogFP, "\nmissing values = (null)\n\n");
        return;
    }

    fprintf(gLogFP, "\nmissing values = [ %g", missing[0]);
    for (mi = 1; mi < nmissing; ++mi) {
        fprintf(gLogFP, ", %g", missing[mi]);
    }
    fprintf(gLogFP, " ]\n\n");

    free(missing);
}

/*******************************************************************************
 *  Create Test Variabless
 */

static int create_test_var_int_degC(CDSGroup **group, CDSVar **var, int define_missing)
{
    CDSDataType  data_type;
    const char  *dim_name;
    int          value[2];

    int data[] =
        { -10, 0, 10, 20, -9999, 40, 50, CDS_FILL_INT, 70, 80, 90, 100, 110 };

    int length = sizeof(data)/sizeof(int);

    data_type = CDS_INT;
    dim_name  = "time";

    if (!*group) {
        *group = cds_define_group(NULL, "root");
        if (!(*group)) return(0);
        if (!cds_define_dim(*group, dim_name, 10, 1)) return(0);
    }

    *var = cds_define_var(*group, "temperature", data_type, 1, &dim_name);
    if (!(*var)) return(0);

    if (!cds_define_att_text(*var, "units", "degC")) return(0);

    value[0] = 0.0;
    if (!cds_define_att(*var, "valid_min", data_type, 1, value)) return(0);

    value[0] = 100.0;
    if (!cds_define_att(*var, "valid_max", data_type, 1, value)) return(0);

    value[0] = 0;
    value[1] = 100;
    if (!cds_define_att(*var, "valid_range", data_type, 2, value)) return(0);

    value[0] = 1.0;
    if (!cds_define_att(*var, "valid_delta", data_type, 1, value)) return(0);

    if (!define_missing_value_atts(
        *var, data_type, -9999, define_missing, define_missing)) {

        return(0);
    }

    value[0] = 30;
    value[1] = 60;
    if (!cds_define_att(*var, "convert_units", data_type, 2, value)) return(0);

    value[0] = 1.0;
    value[1] = 2.0;
    if (!cds_define_att(*var, "convert_delta", data_type, 2, value)) return(0);

    value[0] = 10;
    value[1] = 20;
    if (!cds_define_att(*var, "no_conversion", data_type, 2, value)) return(0);

    cds_put_var_data(*var, 0, length, data_type, (void *)data);

    cds_add_data_att("convert_units", 0);
    cds_add_data_att("convert_delta", CDS_DELTA_UNITS);

    return(1);
}

static int create_test_var_short_km(CDSGroup **group, CDSVar **var, int define_missing)
{
    CDSDataType  data_type;
    const char  *dim_name;
    short        value[2];

    short data[] =
        { -64, -32, CDS_FILL_SHORT, -16, -8, 0, 8, 16, -9999, 32, 64 };

    int   length = sizeof(data)/sizeof(short);

    data_type = CDS_SHORT;
    dim_name  = "time";

    if (!*group) {
        *group = cds_define_group(NULL, "root");
        if (!(*group)) return(0);
        if (!cds_define_dim(*group, dim_name, 10, 1)) return(0);
    }

    *var = cds_define_var(*group, "distance", data_type, 1, &dim_name);
    if (!(*var)) return(0);

    if (!cds_define_att_text(*var, "units", "km")) return(0);

    value[0] = -30;
    if (!cds_define_att(*var, "valid_min", data_type, 1, value)) return(0);

    value[0] = 30;
    if (!cds_define_att(*var, "valid_max", data_type, 1, value)) return(0);

    value[0] = -30;
    value[1] = 30;
    if (!cds_define_att(*var, "valid_range", data_type, 2, value)) return(0);

    value[0] = 1;
    if (!cds_define_att(*var, "valid_delta", data_type, 1, value)) return(0);

    if (!define_missing_value_atts(
        *var, data_type, -9999.0, define_missing, define_missing)) {

        return(0);
    }

    value[0] = -20;
    value[1] = 20;
    if (!cds_define_att(*var, "convert_units", data_type, 2, value)) return(0);

    value[0] = 2;
    value[1] = 3;
    if (!cds_define_att(*var, "convert_delta", data_type, 2, value)) return(0);

    value[0] = 30;
    value[1] = 40;
    if (!cds_define_att(*var, "no_conversion", data_type, 2, value)) return(0);

    cds_put_var_data(*var, 0, length, data_type, (void *)data);

    cds_add_data_att("convert_units", 0);
    cds_add_data_att("convert_delta", CDS_DELTA_UNITS);

    return(1);
}

static int create_test_var_double_mm(CDSGroup **group, CDSVar **var, int define_missing)
{
    CDSDataType  data_type;
    const char  *dim_name;
    double       value[2];

    double data[] =
    {
        -4.0e+38,
        -2.2e+9,
        (double)CDS_FILL_INT,
        -33000.0,
        (double)CDS_FILL_SHORT,
        -9999.0,
        -129.0,
        (double)CDS_FILL_BYTE,
        -32.0,
        0.0,
        32.0,
        128.0,
        32768.0,
        2.2e+9,
        CDS_FILL_DOUBLE,
        4.0e+38
    };

    int length = sizeof(data)/sizeof(double);

    data_type = CDS_DOUBLE;
    dim_name  = "time";

    if (!*group) {
        *group = cds_define_group(NULL, "root");
        if (!(*group)) return(0);
        if (!cds_define_dim(*group, dim_name, 10, 1)) return(0);
    }

    *var = cds_define_var(*group, "length", data_type, 1, &dim_name);
    if (!(*var)) return(0);

    if (!cds_define_att_text(*var, "units", "mm")) return(0);

    value[0] = -1000000.0;
    if (!cds_define_att(*var, "valid_min", data_type, 1, value)) return(0);

    value[0] = 1000000.0;
    if (!cds_define_att(*var, "valid_max", data_type, 1, value)) return(0);

    value[0] = -1000.0;
    value[1] = 1000.0;
    if (!cds_define_att(*var, "valid_range", data_type, 2, value)) return(0);

    value[0] = 1000.0;
    if (!cds_define_att(*var, "valid_delta", data_type, 1, value)) return(0);

    if (!define_missing_value_atts(
        *var, data_type, -9999.0, define_missing, define_missing)) {

        return(0);
    }

    value[0] = 3000.0;
    value[1] = 6000.0;
    if (!cds_define_att(*var, "convert_units", data_type, 2, value)) return(0);

    value[0] = 1000.0;
    value[1] = 2000.0;
    if (!cds_define_att(*var, "convert_delta", data_type, 2, value)) return(0);

    value[0] = 10;
    value[1] = 20;
    if (!cds_define_att(*var, "no_conversion", data_type, 2, value)) return(0);

    cds_put_var_data(*var, 0, length, data_type, (void *)data);

    cds_add_data_att("convert_units", 0);
    cds_add_data_att("convert_delta", CDS_DELTA_UNITS);

    return(1);
}

/*******************************************************************************
 *  Change Var Type Tests
 */

static int _change_var_type_tests_1(int define_missing)
{
    CDSGroup *group = NULL;
    CDSVar   *var;

    if (!create_test_var_double_mm(&group, &var, define_missing)) {
        return(0);
    }

    LOG( gProgramName,
        "------------------------------------------------------------\n"
        "double\n"
        "------------------------------------------------------------\n\n");

    cds_print(gLogFP, group, 0);

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "double -> float\n"
        "------------------------------------------------------------\n\n");

    cds_change_var_type(var, CDS_FLOAT);
    cds_print(gLogFP, group, 0);

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "float -> int\n"
        "------------------------------------------------------------\n\n");

    cds_change_var_type(var, CDS_INT);
    cds_print(gLogFP, group, 0);

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "int -> short\n"
        "------------------------------------------------------------\n\n");

    cds_change_var_type(var, CDS_SHORT);
    cds_print(gLogFP, group, 0);

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "short -> byte\n"
        "------------------------------------------------------------\n\n");

    cds_change_var_type(var, CDS_BYTE);
    cds_print(gLogFP, group, 0);

    cds_delete_group(group);

    return(1);
}

static int _change_var_type_tests_2(int define_missing, int define_fill)
{
    CDSGroup *group = NULL;
    CDSVar   *var;
    size_t    nsamples;

    LOG( gProgramName,
        "\n============================================================\n"
        "test temprature variable double\n"
        "============================================================\n\n");

    group = cds_define_group(NULL, "root");
    if (!group) return(0);
    if (!cds_define_dim(group, "time", 0, 1)) return(0);

    nsamples = 10;

    var = create_temperature_var(group, CDS_DOUBLE, nsamples, define_missing, define_fill);
    cds_print(gLogFP, group, 0);
    cds_delete_var(var);

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "double -> float\n"
        "------------------------------------------------------------\n\n");

    var = create_temperature_var(group, CDS_FLOAT, nsamples, define_missing, define_fill);
    cds_print(gLogFP, group, 0);
    cds_delete_var(var);

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "double -> int\n"
        "------------------------------------------------------------\n\n");

    var = create_temperature_var(group, CDS_INT, nsamples, define_missing, define_fill);
    cds_print(gLogFP, group, 0);
    cds_delete_var(var);

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "double -> short\n"
        "------------------------------------------------------------\n\n");

    var = create_temperature_var(group, CDS_SHORT, nsamples, define_missing, define_fill);
    cds_print(gLogFP, group, 0);
    cds_delete_var(var);

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "double -> byte\n"
        "------------------------------------------------------------\n\n");

    var = create_temperature_var(group, CDS_BYTE, nsamples, define_missing, define_fill);
    cds_print(gLogFP, group, 0);
    cds_delete_var(var);

    cds_delete_group(group);

    return(1);
}

static int change_var_type_tests(void)
{
    if (!_change_var_type_tests_1(1)) {
        return(0);
    }

    if (!_change_var_type_tests_2(1, 1)) {
        return(0);
    }

    if (!_change_var_type_tests_1(2)) {
        return(0);
    }

    if (!_change_var_type_tests_2(2, 2)) {
        return(0);
    }

    if (!_change_var_type_tests_1(3)) {
        return(0);
    }

    if (!_change_var_type_tests_2(3, 3)) {
        return(0);
    }

    return(1);
}


/*******************************************************************************
 *  Change Var Units Tests
 */

static int _change_var_units_tests(int define_missing)
{
    CDSGroup *group = NULL;
    CDSVar   *var;
    size_t    nsamples;
    time_t    base_time;
    char      units[64];

    short     default_fill = CDS_FILL_SHORT;

    if (!create_test_var_int_degC(&group, &var, define_missing)) {
        return(0);
    }

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "int degC\n"
        "------------------------------------------------------------\n\n");

    cds_print(gLogFP, group, 0);

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "int degC -> int K\n"
        "------------------------------------------------------------\n\n");

    cds_change_var_units(var, CDS_INT, "K");
    cds_print(gLogFP, group, 0);

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "int K -> float degF\n"
        "------------------------------------------------------------\n\n");

    cds_change_var_units(var, CDS_FLOAT, "degF");
    cds_print(gLogFP, group, 0);

    cds_delete_group(group);

    LOG( gProgramName,
        "\n============================================================\n\n");

    group = NULL;

    if (!create_test_var_short_km(&group, &var, define_missing)) {
        return(0);
    }

    if (!cds_set_var_default_fill_value(var, &default_fill)) {
        return(0);
    }

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "short km\n"
        "------------------------------------------------------------\n\n");

    cds_print(gLogFP, group, 0);

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "short km -> int m\n"
        "------------------------------------------------------------\n\n");

    cds_change_var_units(var, CDS_INT, "m");
    cds_print(gLogFP, group, 0);

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "int m -> float cm\n"
        "------------------------------------------------------------\n\n");

    cds_change_var_units(var, CDS_FLOAT, "cm");
    cds_print(gLogFP, group, 0);

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "float cm -> double mm\n"
        "------------------------------------------------------------\n\n");

    cds_change_var_units(var, CDS_DOUBLE, "mm");
    cds_print(gLogFP, group, 0);

    cds_delete_group(group);

    LOG( gProgramName,
        "\n============================================================\n"
        "test time variable unit changes\n"
        "============================================================\n\n");

    group = cds_define_group(NULL, "root");
    if (!group) return(0);
    if (!cds_define_dim(group, "time", 0, 1)) return(0);

    nsamples  = 1001;
    base_time = 1320276600; /* 2011-11-02 23:30:00 */
    var       = create_time_var(group, base_time, nsamples, 0.001);

    cds_print(gLogFP, group, 0);

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "base_time -= 1799\n"
        "------------------------------------------------------------\n\n");

    base_time -= 1799;
    if (!cds_base_time_to_units_string(base_time, units)) return(0);

    cds_change_var_units(var, CDS_DOUBLE, units);
    cds_print(gLogFP, group, 0);

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "base_time -= 23 * 3600 + 1\n"
        "------------------------------------------------------------\n\n");

    base_time -= 23 * 3600 + 1;
    if (!cds_base_time_to_units_string(base_time, units)) return(0);

    cds_change_var_units(var, CDS_DOUBLE, units);
    cds_print(gLogFP, group, 0);

    cds_delete_group(group);
    cds_free_unit_system();

    return(1);
}

static int change_var_units_tests(void)
{
    if (!_change_var_units_tests(1)) {
        return(0);
    }

    if (!_change_var_units_tests(2)) {
        return(0);
    }

    if (!_change_var_units_tests(3)) {
        return(0);
    }

    return(1);
}

/*******************************************************************************
 *  Data Index Tests
 */

static int data_index_tests(void)
{
    CDSGroup   *cds;
    CDSVar     *vars[4];
    int         lengths[4];
    int         ndims;
    const char *dim_names[4];
    int         i, j, k, l;
    double     *data;
    int         nelems;

    double     *dp;
    double    **dpp;
    double   ***dppp;
    double  ****dpppp;

    int        *ip;
    int       **ipp;
    int      ***ippp;
    int     ****ipppp;

    lengths[0] = 3;
    lengths[1] = 4;
    lengths[2] = 2;
    lengths[3] = 5;

    /* Create Dataset */

    cds = cds_define_group(NULL, "data_index_test");

    /* Define dims */

    if (!cds_define_dim(cds, "time", 0, 1))           return(0);
    if (!cds_define_dim(cds, "dim1", lengths[1], 0))  return(0);
    if (!cds_define_dim(cds, "dim2", lengths[2], 0))  return(0);
    if (!cds_define_dim(cds, "dim3", lengths[3], 0))  return(0);

    /* Define vars */

    ndims        = 1;
    dim_names[0] = "time";
    vars[0]      = cds_define_var(cds, "var1D", CDS_DOUBLE, ndims, dim_names);
    if (!vars[0]) return(0);

    ndims        = 2;
    dim_names[0] = "time";
    dim_names[1] = "dim1";
    vars[1]      = cds_define_var(cds, "var2D", CDS_DOUBLE, ndims, dim_names);
    if (!vars[1]) return(0);

    ndims        = 3;
    dim_names[0] = "time";
    dim_names[1] = "dim1";
    dim_names[2] = "dim2";
    vars[2]      = cds_define_var(cds, "var3D", CDS_DOUBLE, ndims, dim_names);
    if (!vars[2]) return(0);

    ndims        = 4;
    dim_names[0] = "time";
    dim_names[1] = "dim1";
    dim_names[2] = "dim2";
    dim_names[3] = "dim3";
    vars[3]      = cds_define_var(cds, "var4D", CDS_DOUBLE, ndims, dim_names);
    if (!vars[3]) return(0);

    /* Add data */

    nelems = lengths[0];
    for (i = 1; i < 4; i++) {
        nelems *= lengths[i];
    }

    LOG( gProgramName,
        "------------------------------------------------------------\n"
        "Data Array:\n"
        "------------------------------------------------------------\n\n");

    data = (double *)malloc(nelems * sizeof(double));
    if (!data) {
        ERROR( gProgramName, "Memory allocation error\n");
        return(0);
    }

    for (i = 0; i < nelems; i++) {
        data[i] = (i+1) + (double)(i+1)/1000;
        fprintf(gLogFP, "%f, ", data[i]);
    }
    fprintf(gLogFP, "\n");

    for (i = 0; i < 4; i++) {
        if (!cds_put_var_data(
            vars[i], 0, lengths[0], CDS_DOUBLE, (void *)data)) {

            return(0);
        }
    }

    free(data);

    /* Print dataset */

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "Dataset:\n"
        "------------------------------------------------------------\n");

    cds_print(gLogFP, cds, 0);

    /* Test var1D index */

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "Print var1D using index array:\n"
        "------------------------------------------------------------\n\n");

    dp = (double *)cds_create_var_data_index(vars[0]);
    if (!dp) return(0);

    for (i = 0; i < lengths[0]; i++) {
        fprintf(gLogFP, "%f, ", dp[i]);
    }
    fprintf(gLogFP, "\n");

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "Convert var1D to int type:\n"
        "------------------------------------------------------------\n\n");

    if (!cds_change_var_type(vars[0], CDS_INT)) {
        return(0);
    }

    ip = (int *)cds_create_var_data_index(vars[0]);
    if (!ip) return(0);

    for (i = 0; i < lengths[0]; i++) {
        fprintf(gLogFP, "%d, ", ip[i]);
    }
    fprintf(gLogFP, "\n");

    /* Test var2D index */

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "Print var2D using index array:\n"
        "------------------------------------------------------------\n\n");

    dpp = (double **)cds_create_var_data_index(vars[1]);
    if (!dpp) return(0);

    for (i = 0; i < lengths[0]; i++) {
        for (j = 0; j < lengths[1]; j++) {
            fprintf(gLogFP, "%f, ", dpp[i][j]);
        }
        fprintf(gLogFP, "\n");
    }

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "Convert var2D to int type:\n"
        "------------------------------------------------------------\n\n");

    if (!cds_change_var_type(vars[1], CDS_INT)) {
        return(0);
    }

    ipp = (int **)cds_create_var_data_index(vars[1]);
    if (!ipp) return(0);

    for (i = 0; i < lengths[0]; i++) {
        for (j = 0; j < lengths[1]; j++) {
            fprintf(gLogFP, "%d, ", ipp[i][j]);
        }
        fprintf(gLogFP, "\n");
    }

    /* Test var3D index */

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "Print var3D using index array:\n"
        "------------------------------------------------------------\n\n");

    dppp = (double ***)cds_create_var_data_index(vars[2]);
    if (!dppp) return(0);

    for (i = 0; i < lengths[0]; i++) {
        fprintf(gLogFP, "i = %d:\n", i);
        for (j = 0; j < lengths[1]; j++) {
            for (k = 0; k < lengths[2]; k++) {
                fprintf(gLogFP, "%f, ", dppp[i][j][k]);
            }
            fprintf(gLogFP, "\n");
        }
    }

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "Convert var3D to int type:\n"
        "------------------------------------------------------------\n\n");

    if (!cds_change_var_type(vars[2], CDS_INT)) {
        return(0);
    }

    ippp = (int ***)cds_create_var_data_index(vars[2]);
    if (!ippp) return(0);

    for (i = 0; i < lengths[0]; i++) {
        fprintf(gLogFP, "i = %d:\n", i);
        for (j = 0; j < lengths[1]; j++) {
            for (k = 0; k < lengths[2]; k++) {
                fprintf(gLogFP, "%d, ", ippp[i][j][k]);
            }
            fprintf(gLogFP, "\n");
        }
    }

    /* Get var4D index */

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "Print var4D using index array:\n"
        "------------------------------------------------------------\n\n");

    dpppp = (double ****)cds_create_var_data_index(vars[3]);
    if (!dpppp) return(0);

    for (i = 0; i < lengths[0]; i++) {
        for (j = 0; j < lengths[1]; j++) {
            fprintf(gLogFP, "i = %d, j = %d:\n", i, j);
            for (k = 0; k < lengths[2]; k++) {
                for (l = 0; l < lengths[3]; l++) {
                    fprintf(gLogFP, "%f, ", dpppp[i][j][k][l]);
                }
                fprintf(gLogFP, "\n");
            }
        }
    }

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "Convert var4D to int type:\n"
        "------------------------------------------------------------\n\n");

    if (!cds_change_var_type(vars[3], CDS_INT)) {
        return(0);
    }

    ipppp = (int ****)cds_create_var_data_index(vars[3]);
    if (!ippp) return(0);

    for (i = 0; i < lengths[0]; i++) {
        for (j = 0; j < lengths[1]; j++) {
            fprintf(gLogFP, "i = %d, j = %d:\n", i, j);
            for (k = 0; k < lengths[2]; k++) {
                for (l = 0; l < lengths[3]; l++) {
                    fprintf(gLogFP, "%d, ", ipppp[i][j][k][l]);
                }
                fprintf(gLogFP, "\n");
            }
        }
    }
    fprintf(gLogFP, "\n");

    cds_delete_group(cds);

    return(1);
}

/*******************************************************************************
 *  Get Coord Var Tests
 */

static int print_coord_vars(CDSVar *var)
{
    const char *var_path;
    CDSVar     *coord_var;
    const char *coord_var_path;
    int         di;

    var_path = cds_get_object_path(var);

    fprintf(gLogFP, "var: %s\n", var_path);

    for (di = 0; di < var->ndims; di++) {

        coord_var = cds_get_coord_var(var, di);

        if (coord_var) {
            coord_var_path = cds_get_object_path(coord_var);
        }
        else {
            coord_var_path = "not found";
        }

        fprintf(gLogFP, "  - dim %d: %s\n", di, coord_var_path);
    }

    return(1);
}

static int get_coord_var_tests(void)
{
    CDSGroup *cds = gRoot;
    CDSGroup *group_1;
    CDSVar   *var_2D;
    CDSVar   *var_1_2;

    LOG( gProgramName,
        "------------------------------------------------------------\n"
        "Get var_2D coordinate variables:\n"
        "------------------------------------------------------------\n\n");

    var_2D = cds_get_var(cds, "var_2D");
    if (!var_2D) {
        ERROR( gProgramName, "Could not find variable: var_2D\n");
        return(0);
    }

    if (!print_coord_vars(var_2D)) {
        return(0);
    }

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "Get group_1/_vars_/var_1_2 coordinate variables:\n"
        "------------------------------------------------------------\n\n");

    group_1 = cds_get_group(cds, "group_1");
    if (!group_1) {
        ERROR( gProgramName, "Could not find sub group: group_1\n");
        return(0);
    }

    var_1_2 = cds_get_var(group_1, "var_1_2");
    if (!var_2D) {
        ERROR( gProgramName, "Could not find variable: var_1_2\n");
        return(0);
    }

    if (!print_coord_vars(var_1_2)) {
        return(0);
    }

    return(1);
}

/*******************************************************************************
 *  Get Var Data Tests
 */

static int get_var_data_all_types(CDSVar *var)
{
    size_t  nsamples;
    char    mv_buffer[8];
    CDSData missing_value;
    CDSData data;

    missing_value.cp = mv_buffer;

    /* get data as bytes */

    nsamples = 0;
    data.vp  = cds_get_var_data(
        var, CDS_BYTE, 0, &nsamples, missing_value.vp, NULL);

    if (!data.vp) {
        return(0);
    }

    fprintf(gLogFP, "\n");
    fprintf(gLogFP, "missing_value: %hd\n", (short)(*(missing_value.bp)));
    log_array_values("", CDS_BYTE, nsamples, data.vp);
    free(data.vp);

    /* get data as shorts */

    nsamples = 0;
    data.vp  = cds_get_var_data(
        var, CDS_SHORT, 0, &nsamples, missing_value.vp, NULL);

    if (!data.vp) {
        return(0);
    }

    fprintf(gLogFP, "\n");
    fprintf(gLogFP, "missing_value: %hd\n", *(missing_value.sp));
    log_array_values("", CDS_SHORT, nsamples, data.vp);
    free(data.vp);

    /* get data as ints */

    nsamples = 0;
    data.vp  = cds_get_var_data(
        var, CDS_INT, 0, &nsamples, missing_value.vp, NULL);

    if (!data.vp) {
        return(0);
    }

    fprintf(gLogFP, "\n");
    fprintf(gLogFP, "missing_value: %d\n", *(missing_value.ip));
    log_array_values("", CDS_INT, nsamples, data.vp);
    free(data.vp);

    /* get data as floats */

    nsamples = 0;
    data.vp  = cds_get_var_data(
        var, CDS_FLOAT, 0, &nsamples, missing_value.vp, NULL);

    if (!data.vp) {
        return(0);
    }

    fprintf(gLogFP, "\n");
    fprintf(gLogFP, "missing_value: %.7g\n", *(missing_value.fp));
    log_array_values("", CDS_FLOAT, nsamples, data.vp);
    free(data.vp);

    /* get data as doubles */

    nsamples = 0;
    data.vp  = cds_get_var_data(
        var, CDS_DOUBLE, 0, &nsamples, missing_value.vp, NULL);

    if (!data.vp) {
        return(0);
    }

    fprintf(gLogFP, "\n");
    fprintf(gLogFP, "missing value: %.15g\n", *(missing_value.dp));
    log_array_values("", CDS_DOUBLE, nsamples, data.vp);
    free(data.vp);

    return(1);
}

static int delete_missing_value_atts(CDSVar *var, int from_parent)
{
    CDSAtt *att;

    att = (from_parent)
        ? cds_get_att(var->parent, "missing_value")
        : cds_get_att(var, "missing_value");

    if (att) {
        cds_delete_att(att);
    }

    att = (from_parent)
        ? cds_get_att(var->parent, "_FillValue")
        : cds_get_att(var, "_FillValue");

    if (att) {
        cds_delete_att(att);
    }

    return(1);
}

static int _get_var_data_tests(int define_missing)
{
    CDSGroup *group = NULL;
    CDSVar   *var;

    LOG( gProgramName,
        "------------------------------------------------------------\n"
        "short var with missing value attributes\n"
        "------------------------------------------------------------\n");

    if (!create_test_var_short_km(&group, &var, define_missing)) {
        return(0);
    }

    cds_print(gLogFP, group, 0);

    if (!get_var_data_all_types(var)) {
        return(0);
    }

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "short var *without* missing value attributes\n"
        "------------------------------------------------------------\n");

    if (define_missing > 1) {
        if (!delete_missing_value_atts(var, 1)) return(0);
    }
    else {
        if (!delete_missing_value_atts(var, 0)) return(0);
    }

    cds_print(gLogFP, group, 0);

    if (!get_var_data_all_types(var)) {
        return(0);
    }

    cds_delete_group(group);

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "double var with missing value attributes\n"
        "------------------------------------------------------------\n");

    group = NULL;

    if (!create_test_var_double_mm(&group, &var, define_missing)) {
        return(0);
    }

    cds_print(gLogFP, group, 0);

    if (!get_var_data_all_types(var)) {
        return(0);
    }

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "double var *without* missing value attributes\n"
        "------------------------------------------------------------\n");

    if (define_missing > 1) {
        if (!delete_missing_value_atts(var, 1)) return(0);
    }
    else {
        if (!delete_missing_value_atts(var, 0)) return(0);
    }

    cds_print(gLogFP, group, 0);

    if (!get_var_data_all_types(var)) {
        return(0);
    }

    cds_delete_group(group);

    return(1);
}

static int get_var_data_tests(void)
{
    if (!_get_var_data_tests(1)) {
        return(0);
    }

    if (!_get_var_data_tests(2)) {
        return(0);
    }

    if (!_get_var_data_tests(3)) {
        return(0);
    }

    return(1);
}

/*******************************************************************************
 *  Set Var Data Tests
 */

static int create_init_data_test_vars(
    CDSGroup    **group,
    CDSVar      **vars,
    int           define_missing,
    int           ndims)
{
    const char  *dim_names[2] = { "time", "range" };

    const char  *var_names[] = {
        "byte_var",
        "short_var",
        "int_var",
        "float_var",
        "double_var",
    };

    CDSDataType  data_types[] = {
        CDS_BYTE,
        CDS_SHORT,
        CDS_INT,
        CDS_FLOAT,
        CDS_DOUBLE
    };

    int     nvars = 5;
    int     vi;
    double  mv_double;
    double  mv_buffer;
    CDSData mv;

    mv.vp = (void *)&mv_buffer;

    if (!(*group = cds_define_group(NULL, "init_data_test_vars"))) return(0);

    if (!cds_define_dim(*group, dim_names[0], 0, 1)) return(0);
    if (ndims == 2) {
        if (!cds_define_dim(*group, dim_names[1], 5, 0)) return(0);
    }

    for (vi = 0; vi < nvars; vi++) {

        mv_double = (data_types[vi] == CDS_BYTE) ? -99 : -9999.0;

        vars[vi] = cds_define_var(
            *group, var_names[vi], data_types[vi], ndims, dim_names);

        if (!(vars[vi])) return(0);

        if (!define_missing) {
            continue;
        }

        cds_copy_array(CDS_DOUBLE, 1, &mv_double, data_types[vi], mv.vp,
            0, NULL, NULL, NULL, NULL, NULL, NULL);

        if (define_missing == 1) {
            if (!cds_define_att(
                vars[vi], "missing_value", data_types[vi], 1, mv.vp)) {

                return(0);
            }
        }
        else if (define_missing == 2 && vi == 3) {
            if (!cds_define_att(
                *group, "missing_value", data_types[vi], 1, mv.vp)) {

                return(0);
            }
        }
        else if (define_missing == 3 && vi == 0) {
            if (!cds_define_att_text(
                *group, "missing_value", "%s", "-9999")) {

                return(0);
            }
        }
    }

    return(1);
}

static int _init_var_data_tests(int define_missing, int use_missing, int ndims)
{
    CDSGroup  *group;
    CDSVar    *vars[5];
    int        nvars = 5;
    int        vi;

    LOG( gProgramName,
        "------------------------------------------------------------\n"
        "Init Variable Data Tests:\n"
        "  - define_missing = %d\n"
        "  - use_missing    = %d\n"
        "  - ndims          = %d\n"
        "------------------------------------------------------------\n",
        define_missing, use_missing, ndims);

    if (!create_init_data_test_vars(&group, vars, define_missing, ndims)) {
        return(0);
    }

    for (vi = 0; vi < nvars; vi++) {

        if (!cds_init_var_data(vars[vi], 0, 3, use_missing)) {
            return(0);
        }

        if (!cds_init_var_data(vars[vi], 6, 4, use_missing)) {
            return(0);
        }
    }

    fprintf(gLogFP, "\nDefault Fill Values:\n");

    for (vi = 0; vi < nvars; vi++) {

        if (!vars[vi]->default_fill) {
            fprintf(gLogFP, "%s: NULL\n", vars[vi]->name);
            continue;
        }

        log_array_values("", vars[vi]->type, 1, vars[vi]->default_fill);
    }

    cds_print(gLogFP, group, 0);

    cds_delete_group(group);

    return(1);
}

static int init_var_data_tests(void)
{
    if (!_init_var_data_tests(0, 0, 1)) {
        return(0);
    }

    if (!_init_var_data_tests(0, 0, 2)) {
        return(0);
    }

    if (!_init_var_data_tests(0, 1, 1)) {
        return(0);
    }

    if (!_init_var_data_tests(0, 1, 2)) {
        return(0);
    }

    if (!_init_var_data_tests(1, 0, 1)) {
        return(0);
    }

    if (!_init_var_data_tests(1, 0, 2)) {
        return(0);
    }

    if (!_init_var_data_tests(1, 1, 1)) {
        return(0);
    }

    if (!_init_var_data_tests(1, 1, 2)) {
        return(0);
    }

    if (!_init_var_data_tests(2, 0, 1)) {
        return(0);
    }

    if (!_init_var_data_tests(2, 0, 2)) {
        return(0);
    }

    if (!_init_var_data_tests(2, 1, 1)) {
        return(0);
    }

    if (!_init_var_data_tests(2, 1, 2)) {
        return(0);
    }

    if (!_init_var_data_tests(3, 0, 1)) {
        return(0);
    }

    if (!_init_var_data_tests(3, 0, 2)) {
        return(0);
    }

    if (!_init_var_data_tests(3, 1, 1)) {
        return(0);
    }

    if (!_init_var_data_tests(3, 1, 2)) {
        return(0);
    }

    return(1);
}

/*******************************************************************************
 *  Set Var Data Tests
 */

static int create_set_data_test_vars(
    CDSGroup    **group,
    CDSVar      **vars,
    int           define_missing)
{
    const char  *dim_name = "time";

    const char  *var_names[] = {
        "byte_var",
        "short_var",
        "int_var",
        "float_var",
        "double_var",
    };

    CDSDataType  data_types[] = {
        CDS_BYTE,
        CDS_SHORT,
        CDS_INT,
        CDS_FLOAT,
        CDS_DOUBLE
    };

    int     nvars = 5;
    int     vi;
    double  mv_double;
    double  mv_buffer;
    CDSData mv;

    mv.vp = (void *)&mv_buffer;

    if (!(*group = cds_define_group(NULL, "set_data_test_vars"))) return(0);
    if (!cds_define_dim(*group, dim_name, 0, 1)) return(0);

    for (vi = 0; vi < nvars; vi++) {

        mv_double = (data_types[vi] == CDS_BYTE) ? -99 : -9999.0;

        vars[vi] = cds_define_var(
            *group, var_names[vi], data_types[vi], 1, &dim_name);

        if (!(vars[vi])) return(0);

        if (!define_missing) {
            continue;
        }

        cds_copy_array(CDS_DOUBLE, 1, &mv_double, data_types[vi], mv.vp,
            0, NULL, NULL, NULL, NULL, NULL, NULL);

        if (define_missing == 1) {
            if (!cds_define_att(
                vars[vi], "missing_value", data_types[vi], 1, mv.vp)) {

                return(0);
            }
        }
        else if (define_missing == 2 && vi == 3) {
            if (!cds_define_att(
                *group, "missing_value", data_types[vi], 1, mv.vp)) {

                return(0);
            }
        }
        else if (define_missing == 3 && vi == 0) {
            if (!cds_define_att_text(
                *group, "missing_value", "%s", "-9999")) {

                return(0);
            }
        }
    }

    return(1);
}

static int _set_var_data_tests(int define_missing)
{
    CDSGroup  *group;
    CDSVar    *vars[5];
    int        nvars = 5;
    int        vi;

    double dbl_missing = -8888.0;
    double dbl_data[] =
    {
        -4.0e+38,
        -2.2e+9,
        -32768.0,
        dbl_missing,
        -128.5,
        -32.4,
        0.0,
        dbl_missing,
        32.4,
        128.5,
        32768.0,
        2.2e+9,
        4.0e+38
    };

    int dbl_length = sizeof(dbl_data)/sizeof(double);

    short short_missing = -8888;
    short short_data[] =
        { -128, -64, -32, short_missing, -16, -8, 0, short_missing, 8, 16, 32, 64, 128 };

    int short_length = sizeof(short_data)/sizeof(short);

    LOG( gProgramName,
        "------------------------------------------------------------\n"
        "Set Variable Data Tests using doubles; define_missing = %d\n"
        "------------------------------------------------------------\n",
        define_missing);

    if (!create_set_data_test_vars(&group, vars, define_missing)) return(0);

    if (!define_missing) {
        for (vi = 0; vi < nvars; vi++) {
            if (vars[vi]->default_fill) {
                ERROR( gProgramName, "Default fill value is not NULL!\n");
                return(0);
            }
        }
    }

    for (vi = 0; vi < nvars; vi++) {

        if (!cds_set_var_data(
            vars[vi], CDS_DOUBLE, 0, dbl_length, &dbl_missing, dbl_data)) {

            return(0);
        }

    }

    if (!define_missing) {

        fprintf(gLogFP, "\nDefault Fill Values:\n");

        for (vi = 0; vi < nvars; vi++) {

            if (!vars[vi]->default_fill) {
                ERROR( gProgramName, "Default fill value is NULL!\n");
                return(0);
            }

            log_array_values("", vars[vi]->type, 1, vars[vi]->default_fill);
        }
    }

    cds_print(gLogFP, group, 0);

    cds_delete_group(group);

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "Set Variable Data Tests using shorts; define_missing = %d\n"
        "------------------------------------------------------------\n",
        define_missing);

    if (!create_set_data_test_vars(&group, vars, define_missing)) return(0);

    if (!define_missing) {
        for (vi = 0; vi < nvars; vi++) {
            if (vars[vi]->default_fill) {
                ERROR( gProgramName, "Default fill value is not NULL!\n");
                return(0);
            }
        }
    }

    for (vi = 0; vi < nvars; vi++) {

        if (!cds_set_var_data(
            vars[vi], CDS_SHORT, 0, short_length, &short_missing, short_data)) {

            return(0);
        }

    }

    if (!define_missing) {

        fprintf(gLogFP, "\nDefault Fill Values:\n");

        for (vi = 0; vi < nvars; vi++) {

            if (!vars[vi]->default_fill) {
                ERROR( gProgramName, "Default fill value is NULL!\n");
                return(0);
            }

            log_array_values("", vars[vi]->type, 1, vars[vi]->default_fill);
        }
    }

    cds_print(gLogFP, group, 0);

    cds_delete_group(group);

    return(1);
}

static int set_var_data_tests(void)
{
    if (!_set_var_data_tests(0)) {
        return(0);
    }
    if (!_set_var_data_tests(1)) {
        return(0);
    }
    if (!_set_var_data_tests(2)) {
        return(0);
    }
    if (!_set_var_data_tests(3)) {
        return(0);
    }

    return(1);
}

/*******************************************************************************
 *  Boundary Var Data Tests
 */

static CDSGroup *create_bounds_tests_group(int with_units)
{
    CDSGroup    *group;
    CDSVar      *var;
    time_t       times[5];
    double       offset_dbl[2];
    int          offset_int[2];
    const char  *dim_names[2];
    int          di;

    group = cds_define_group(NULL, "bounds_var_data_tests");
    if (!group) return(NULL);

    /* Create dimensions */

    dim_names[0] = "time";
    dim_names[1] = "bounds";

    if (!cds_define_dim(group, "time",    0, 1)) return(NULL);
    if (!cds_define_dim(group, "range",  10, 0)) return(NULL);
    if (!cds_define_dim(group, "bounds",  2, 0)) return(NULL);

    /* Create time coordinate and boundary variables */

    var = cds_define_var(group, "time", CDS_DOUBLE, 1, dim_names);
    if (!var) return(NULL);

    if (!cds_define_att(var, "long_name", CDS_CHAR, 0, NULL)) return(NULL);
    if (!cds_define_att(var, "units", CDS_CHAR, 0, NULL)) return(NULL);
    if (!cds_define_att_text(var, "bounds", "%s", "time_bounds")) return(NULL);

    var = cds_define_var(group, "time_bounds", CDS_DOUBLE, 2, dim_names);
    if (!var) return(NULL);

    if (with_units) {
        if (!cds_define_att(var, "units", CDS_CHAR, 0, NULL)) return(NULL);
    }

    offset_dbl[0] = -1.5;
    offset_dbl[1] =  1.5;

    if (!cds_define_att(var, "bound_offsets", CDS_DOUBLE, 2, offset_dbl)) return(NULL);

    /* 1387324800 = 2013-12-18 00:00:00 */
    for (di = 0; di < 5; di++) {
        times[di] = 1387324800 + di * 15;
    }

    if (!cds_set_sample_times(group, 0, 5, times)) return(NULL);

    /* Create range coordinate and boundary variables */

    dim_names[0] = "range";

    var = cds_define_var(group, "range", CDS_INT, 1, dim_names);
    if (!var) return(NULL);

    if (!cds_define_att_text(var, "units", "%s", "km")) return(NULL);
    if (!cds_define_att_text(var, "bounds", "%s", "range_bounds")) return(NULL);

    if (!cds_alloc_var_data(var, 0, 10)) return(NULL);
    for (di = 0; di < 10; di++) {
        var->data.ip[di] = di * 10;
    }

    var = cds_define_var(group, "range_bounds", CDS_INT, 2, dim_names);
    if (!var) return(NULL);

    if (with_units) {
        if (!cds_define_att_text(var, "units", "%s", "km")) return(NULL);
    }

    offset_int[0] = -5;
    offset_int[1] =  5;

    if (!cds_define_att(var, "bound_offsets", CDS_INT, 2, offset_int)) return(NULL);

    /* Set the boundary variable data */

    if (!cds_set_bounds_data(group, 0, 5)) return(NULL);

    return(group);
}

static int bounds_var_data_tests(void)
{
    CDSGroup *group;
    CDSVar   *var;

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "Boundary variable test with no bounds units\n"
        "------------------------------------------------------------\n");

    group = create_bounds_tests_group(0);
    if (!group) return(0);

    cds_print(gLogFP, group, 0);

    /* Adjust the base time by 12 hours */

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "Boundary variable test with no bounds units - with time shift\n"
        "------------------------------------------------------------\n");

    if (!cds_set_base_time(group, "Seconds since noon", 1387281600)) return(0);
    cds_print(gLogFP, group, 0);

    /* Convert range units to meters */

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "Boundary variable test with no bounds units - range in meters\n"
        "------------------------------------------------------------\n");

    var = cds_get_var(group, "range");
    if (!cds_change_var_units(var, CDS_FLOAT, "m")) return(0);

    cds_print(gLogFP, group, 0);
    cds_delete_group(group);

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "Boundary variable test with bounds units\n"
        "------------------------------------------------------------\n");

    group = create_bounds_tests_group(1);
    if (!group) return(0);

    cds_print(gLogFP, group, 0);

    /* Adjust the base time by 12 hours */

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "Boundary variable test with bounds units - with time shift\n"
        "------------------------------------------------------------\n");

    if (!cds_set_base_time(group, "Seconds since noon", 1387281600)) return(0);
    cds_print(gLogFP, group, 0);

    /* Convert range units to meters */

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "Boundary variable test with bounds units - range in meters\n"
        "------------------------------------------------------------\n");

    var = cds_get_var(group, "range");
    if (!cds_change_var_units(var, CDS_FLOAT, "m")) return(0);

    cds_print(gLogFP, group, 0);
    cds_delete_group(group);

    return(1);
}

/*******************************************************************************
 *  Get missing values test
 */

static int get_missing_values_test(void)
{
    CDSGroup *group = NULL;
    CDSVar   *var;
    double    dblval;
    CDSAtt   *att, *att1, *att2;
    double    default_fill = CDS_FILL_DOUBLE;

    LOG( gProgramName,
        "------------------------------------------------------------\n"
        "Get missing values test\n"
        "------------------------------------------------------------\n");

    if (!create_test_var_double_mm(&group, &var, 0)) {
        return(0);
    }

    cds_print(gLogFP, group, 0);
    _print_missings(var);

    LOG( gProgramName,
        "------------------------------------------------------------\n"
        "Call cds_create_missing_value_att() with flags == 1\n"
        "------------------------------------------------------------\n");

    if (!cds_create_missing_value_att(var, 1)) {
        return(0);
    }

    cds_print(gLogFP, group, 0);
    _print_missings(var);

    LOG( gProgramName,
        "------------------------------------------------------------\n"
        "Call cds_create_missing_value_att() with flags == 0\n"
        "------------------------------------------------------------\n");

    if (!cds_create_missing_value_att(var, 0)) {
        return(0);
    }

    cds_print(gLogFP, group, 0);
    _print_missings(var);

    LOG( gProgramName,
        "------------------------------------------------------------\n"
        "Change missing_value to -9999\n"
        "Add 'missing-value = -8888' as char type\n"
        "Add 'missing_data = -7777' as char type\n"
        "------------------------------------------------------------\n");

    dblval = -9999;
    if (!cds_change_att(var, 1, "missing_value", CDS_DOUBLE, 1, &dblval)) {
        return(0);
    }

    att1 = cds_define_att(var, "missing-value", CDS_CHAR, 5, "-8888");
    if (!att1) return(0);

    att2 = cds_define_att(var, "missing_data", CDS_CHAR, 5, "-7777");
    if (!att2) return(0);

    cds_print(gLogFP, group, 0);
    _print_missings(var);

    LOG( gProgramName,
        "------------------------------------------------------------\n"
        "Set default fill value\n"
        "------------------------------------------------------------\n");

    if (!cds_set_var_default_fill_value(var, &default_fill)) {
        return(0);
    }

    cds_print(gLogFP, group, 0);
    _print_missings(var);

    LOG( gProgramName,
        "------------------------------------------------------------\n"
        "Change 'missing_data = -9999'\n"
        "------------------------------------------------------------\n");

    if (!cds_change_att(var, 1, "missing_data", CDS_CHAR, 5, "-9999")) {
        return(0);
    }

    cds_print(gLogFP, group, 0);
    _print_missings(var);

    LOG( gProgramName,
        "------------------------------------------------------------\n"
        "Remove 'missing_value' and \n"
        "call cds_create_missing_value_att() with flags == 0\n"
        "------------------------------------------------------------\n");

    att = cds_get_att(var, "missing_value");
    cds_delete_att(att);

    if (!cds_create_missing_value_att(var, 0)) {
        return(0);
    }

    cds_print(gLogFP, group, 0);
    _print_missings(var);

    LOG( gProgramName,
        "------------------------------------------------------------\n"
        "Remove 'missing_value' and \n"
        "call cds_create_missing_value_att() with flags == 1\n"
        "------------------------------------------------------------\n");

    att = cds_get_att(var, "missing_value");
    cds_delete_att(att);

    if (!cds_create_missing_value_att(var, 1)) {
        return(0);
    }

    cds_print(gLogFP, group, 0);
    _print_missings(var);

    cds_delete_group(group);

    return(1);
}

/*******************************************************************************
 *  Run Var Data Tests
 */

void libcds3_test_var_data(void)
{
    fprintf(stdout, "\nVariable Data Tests:\n");

    run_test(" - change_var_type_tests",
        "change_var_type_tests", change_var_type_tests);

    run_test(" - change_var_units_tests",
        "change_var_units_tests", change_var_units_tests);

    run_test(" - data_index_tests",
        "data_index_tests", data_index_tests);

    run_test(" - get_coord_var_tests",
        "get_coord_var_tests", get_coord_var_tests);

    run_test(" - get_var_data_tests",
        "get_var_data_tests", get_var_data_tests);

    run_test(" - init_var_data_tests",
        "init_var_data_tests", init_var_data_tests);

    run_test(" - set_var_data_tests",
        "set_var_data_tests", set_var_data_tests);

    run_test(" - bounds_var_data_tests",
        "bounds_var_data_tests", bounds_var_data_tests);
    
    run_test(" - get_missing_values_test",
        "get_missing_values_test", get_missing_values_test);
}
