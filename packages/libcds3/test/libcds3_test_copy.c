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
extern CDSGroup   *gRoot;
extern FILE       *gLogFP;

CDSGroup *gClone = NULL;

/*******************************************************************************
 *  Copy Tests
 */

static int copy_tests(void)
{
    CDSGroup *copy;

    const char *dim_names[] = {
        "time",
        "range",
        NULL
    };

    const char *att_names[] = {
        "att_char",
        "att_byte",
        "att_short",
        "att_int",
        "att_float",
        "att_double",
        NULL
    };

    const char *var_names[] = {
        "time",
        "range",
        "var_int_static",
        "var_float_static",
        "var_double_static",
        "var_int",
        "var_float",
        "var_double",
        NULL
    };

    LOG( gProgramName,
        "------------------------------------------------------------\n"
        "Copy all dimensions, attributes and variables.\n"
        "------------------------------------------------------------\n\n");

    copy = cds_define_group(NULL, "copy tests");
    if (!copy) {
        return(0);
    }

    if (!cds_copy_atts(gRoot, copy, NULL, NULL, 0)) {
        return(0);
    }

    if (!cds_copy_dims(gRoot, copy, NULL, NULL, 0)) {
        return(0);
    }

    if (!cds_copy_vars(gRoot, copy, NULL, NULL, NULL, NULL, 0, 0, 0, 0)) {
        return(0);
    }

    cds_print(gLogFP, copy, 0);
    cds_delete_group(copy);

    LOG( gProgramName,
        "------------------------------------------------------------\n"
        "Copy selected dimensions, attributes and variables.\n"
        "------------------------------------------------------------\n\n");

    copy = cds_define_group(NULL, "copy tests");
    if (!copy) {
        return(0);
    }

    if (!cds_copy_atts(gRoot, copy, att_names, NULL, 0)) {
        return(0);
    }

    if (!cds_copy_dims(gRoot, copy, dim_names, NULL, 0)) {
        return(0);
    }

    if (!cds_copy_vars(gRoot, copy, NULL, NULL, var_names, NULL, 0, 0, 0, 0)) {
        return(0);
    }

    cds_print(gLogFP, copy, 0);
    cds_delete_group(copy);

    return(1);
}

/*******************************************************************************
 *  Clone Tests
 */

static int clone_tests(void)
{
    char *orig_name = "clone_test.orig";
    char *copy_name = "clone_test.copy";
    int   status;

    /* Print out the original CDS group for referense */

    if (!open_run_test_log(orig_name)) {
        return(0);
    }

    cds_print(gLogFP, gRoot, 0);

    close_run_test_log();

    /* Clone the CDS group */

    if (!open_run_test_log(copy_name)) {
        return(0);
    }

    status = cds_copy_group(
        gRoot, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        0, 0, 0, 0, &gClone);

    if (status > 0) {
        cds_print(gLogFP, gClone, 0);
    }
    else {
        status = 0;
    }

    close_run_test_log();

    /* Compare the clone with the original */

    if (status != 0) {
        status = compare_files("out/clone_test.orig", "out/clone_test.copy");
    }

    return(status);
}

/*******************************************************************************
 *  Test Rename Functions
 */

static int rename_tests(void)
{
    CDSGroup *cds = gClone;
    CDSDim   *dim;
    CDSAtt   *att;
    CDSVar   *var;

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "Rename dimension test:\n"
        "------------------------------------------------------------\n\n");

    dim = cds_get_dim(cds, "range");
    if (!dim) {
        ERROR( gProgramName, "Could not find range dimension\n");
        return(0);
    }

    LOG( gProgramName,
        "- Renaming a dimension to an existing name should fail.\n\n");

    if (cds_rename_dim(dim, "string") != 0) {
        ERROR( gProgramName,
            "Rename of dim range to string should have failed!\n");
        return(0);
    }

    cds_print_dim(gLogFP, "\n - Before rename: ", 0, dim);

    cds_rename_dim(dim, "range_renamed");

    cds_print_dim(gLogFP, " - After rename:  ", 0, dim);

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "Rename attribute test:\n"
        "------------------------------------------------------------\n\n");

    att = cds_get_att(cds, "att_char");
    if (!att) {
        ERROR( gProgramName, "Could not find att_char attribute\n");
        return(0);
    }

    LOG( gProgramName,
        "- Renaming an attribute to an existing name should fail.\n\n");

    if (cds_rename_att(att, "att_int") != 0) {
        ERROR( gProgramName,
            "Rename of att_char to att_int should have failed!\n");
        return(0);
    }

    cds_print_att(gLogFP, "\n - Before rename: ", 0, att);

    cds_rename_att(att, "att_char_renamed");

    cds_print_att(gLogFP, " - After rename:  ", 0, att);

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "Rename variable test:\n"
        "------------------------------------------------------------\n\n");

    var = cds_get_var(cds, "var_2D");
    if (!var) {
        ERROR( gProgramName, "Could not find var_2D variable\n");
        return(0);
    }

    LOG( gProgramName,
        "- Renaming a variable to an existing name should fail.\n\n");

    if (cds_rename_var(var, "var_char_2D") != 0) {
        ERROR( gProgramName,
            "Rename of var_2D to var_char_2D should have failed!\n");
        return(0);
    }

    LOG( gProgramName, "\n - Before rename:\n\n");

    cds_print_var(gLogFP, "", var, CDS_SKIP_DATA);

    cds_rename_var(var, "var_2D_renamed");

    LOG( gProgramName, "\n - After rename:\n\n");

    cds_print_var(gLogFP, "", var, CDS_SKIP_DATA);

    return(1);
}

/*******************************************************************************
 *  Run Copy and Rename Tests
 */

void libcds3_test_copy(void)
{
    fprintf(stdout, "\nCopy Tests:\n");

    run_test(" - copy_tests", "copy_tests", copy_tests);
    run_test(" - clone_tests", NULL, clone_tests);
    run_test(" - rename_tests", "rename_tests", rename_tests);

    cds_delete_group(gClone);
}
