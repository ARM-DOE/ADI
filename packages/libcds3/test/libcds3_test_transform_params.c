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
*    $Revision: 68010 $
*    $Author: ermold $
*    $Date: 2016-03-11 20:27:44 +0000 (Fri, 11 Mar 2016) $
*    $Version:$
*
*******************************************************************************/

#include "libcds3_test.h"

extern const char *gProgramName;
extern const char *gTopTestDir;
extern CDSGroup   *gRoot;
extern FILE       *gLogFP;

/*******************************************************************************
 *  Transformation Parameters Tests
 */

int trans_params_tests(void)
{
    const char *file = "transform_params.cfg";
    CDSGroup   *group = gRoot;
    CDSGroup   *group_1;
    CDSVar     *var;
    const char *param_name;
    char       *str_value;
    double      dbl_value;
    double     *dbl_array;
    int         int_value;
    size_t      length;
    char        prefix[256];
    CDSGroup   *copy;

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "Loading file: %s\n"
        "------------------------------------------------------------\n\n",
        file);

    if (!cds_load_transform_params_file(group, gTopTestDir, file)) {
        return(0);
    }

    cds_print_transform_params(gLogFP, "", group, NULL);

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "cds_get_transform_param tests\n"
        "------------------------------------------------------------\n\n");

    /* Get var_2D */

    var = cds_get_var(group, "var_2D");
    if (!var) {
        ERROR( gProgramName, "Could not find variable: var_2D\n");
        return(0);
    }

    /* var_2D:transformation */

    param_name = "transformation";
    length     = 0;
    str_value  = cds_get_transform_param(
        var, param_name, CDS_CHAR, &length, NULL);

    fprintf(gLogFP, "string %s:%s = \"%s\"\n",
        var->name, param_name, str_value);

    free(str_value);

    /* var_2D:weight */

    param_name = "weight";
    length     = 1;
    if (!cds_get_transform_param(
        var, param_name, CDS_DOUBLE, &length, &dbl_value)) {

        return(0);
    }

    sprintf(prefix, "%s:%s         = ", var->name, param_name);
    log_array_values(prefix, CDS_DOUBLE, length, (void *)&dbl_value);

    /* var_2D:missing_value */

    param_name = "missing_value";
    length     = 1;
    if (!cds_get_transform_param(
        var, param_name, CDS_INT, &length, &int_value)) {

        return(0);
    }

    sprintf(prefix, "%s:%s  = ", var->name, param_name);
    log_array_values(prefix, CDS_INT, length, (void *)&int_value);

    /* Get group_1/_vars_/var_1_2 */

    group_1 = cds_get_group(group, "group_1");
    if (!group_1) {
        ERROR( gProgramName, "Could not find sub group: group_1\n");
        return(0);
    }

    var = cds_get_var(group_1, "var_1_2");
    if (!var) {
        ERROR( gProgramName, "Could not find variable: var_1_2\n");
        return(0);
    }

    /* var_1_2:test_values */

    param_name = "test_values";
    length     = 0;
    dbl_array  = cds_get_transform_param(
        var, param_name, CDS_DOUBLE, &length, NULL);

    if (!dbl_array) {
        return(0);
    }

    sprintf(prefix, "%s:%s   = ", var->name, param_name);
    log_array_values(prefix, CDS_DOUBLE, length, (void *)dbl_array);

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "cds_set_transform_param tests\n"
        "------------------------------------------------------------\n\n");

    /* var_2D:dbl_array */

    if (!cds_set_transform_param(
            group, "var_2D", "dbl_array", CDS_DOUBLE, length, dbl_array)) {

        return(0);
    }

    free(dbl_array);

    /* var_2D:dbl_value */

    length = 1;

    if (!cds_set_transform_param(
            group, "var_2D", "dbl_value", CDS_DOUBLE, length, &dbl_value)) {

        return(0);
    }

    /* var_2D:int_value */

    length = 1;

    if (!cds_set_transform_param(
            group, "var_2D", "int_value", CDS_INT, length, &int_value)) {

        return(0);
    }

    cds_print_transform_params(gLogFP, "", group, NULL);

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "cds_copy_transform_params test\n"
        "------------------------------------------------------------\n\n");

    copy = cds_define_group(NULL, "trans params copy test");

    if (!cds_copy_transform_params(group, copy)) {
        return(0);
    }

    cds_print_transform_params(gLogFP, "", group, NULL);

    cds_delete_group(copy);

    return(1);
}

/*******************************************************************************
 *  Run Transformation Parameter Tests
 */

void libcds3_test_transform_params(void)
{
    fprintf(stdout, "\nTransformation Parameter Tests:\n");

    run_test(" - trans_params_tests",
        "trans_params_tests", trans_params_tests);
}
