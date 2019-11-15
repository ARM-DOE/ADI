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
*    $Revision: 9751 $
*    $Author: ermold $
*    $Date: 2011-12-01 20:31:50 +0000 (Thu, 01 Dec 2011) $
*    $Version:$
*
*******************************************************************************/

#include "libcds3_test.h"

extern const char *gProgramName;
extern CDSGroup   *gRoot;
extern FILE       *gLogFP;

/*******************************************************************************
 *  Get Att Value Tests
 */

static int get_att_value_tests(void)
{
    CDSGroup *cds = gRoot;
    CDSAtt   *att;
    CDSData   data;
    size_t    length;

    LOG( gProgramName,
        "------------------------------------------------------------\n"
        "Get short Attribute:\n"
        "------------------------------------------------------------\n\n");

    att = cds_get_att(cds, "att_short");
    cds_print_att(gLogFP, "", 0, att);

    data.cp = cds_get_att_text(att, NULL, NULL);
    if (!data.cp) return(0);
    fprintf(gLogFP, "string      \"%s\"\n", data.cp);
    free(data.vp);

    length  = 0;
    data.vp = cds_get_att_value(att, CDS_INT, &length, NULL);
    if (!data.vp) return(0);
    log_array_values("     ", CDS_INT, length, data.vp);
    free(data.vp);

    length  = 0;
    data.vp = cds_get_att_value(att, CDS_FLOAT, &length, NULL);
    if (!data.vp) return(0);
    log_array_values("     ", CDS_FLOAT, length, data.vp);
    free(data.vp);

    length  = 0;
    data.vp = cds_get_att_value(att, CDS_DOUBLE, &length, NULL);
    if (!data.vp) return(0);
    log_array_values("     ", CDS_DOUBLE, length, data.vp);
    free(data.vp);

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "Get int Attribute:\n"
        "------------------------------------------------------------\n\n");

    att = cds_get_att(cds, "att_int");
    cds_print_att(gLogFP, "", 0, att);

    data.cp = cds_get_att_text(att, NULL, NULL);
    if (!data.cp) return(0);
    fprintf(gLogFP, "string    \"%s\"\n", data.cp);
    free(data.vp);

    length  = 0;
    data.vp = cds_get_att_value(att, CDS_INT, &length, NULL);
    if (!data.vp) return(0);
    log_array_values("   ", CDS_INT, length, data.vp);
    free(data.vp);

    length  = 0;
    data.vp = cds_get_att_value(att, CDS_DOUBLE, &length, NULL);
    if (!data.vp) return(0);
    log_array_values("   ", CDS_DOUBLE, length, data.vp);
    free(data.vp);

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "Get float Attribute:\n"
        "------------------------------------------------------------\n\n");

    att = cds_get_att(cds, "att_float");
    cds_print_att(gLogFP, "", 0, att);

    data.cp = cds_get_att_text(att, NULL, NULL);
    if (!data.cp) return(0);
    fprintf(gLogFP, "string      \"%s\"\n", data.cp);
    free(data.vp);

    length  = 0;
    data.vp = cds_get_att_value(att, CDS_FLOAT, &length, NULL);
    if (!data.vp) return(0);
    log_array_values("     ", CDS_FLOAT, length, data.vp);
    free(data.vp);

    length  = 0;
    data.vp = cds_get_att_value(att, CDS_DOUBLE, &length, NULL);
    if (length <= 0) return(0);
    log_array_values("     ", CDS_DOUBLE, length, data.vp);
    free(data.vp);

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "Get double Attribute:\n"
        "------------------------------------------------------------\n\n");

    att = cds_get_att(cds, "att_double");
    cds_print_att(gLogFP, "", 0, att);

    data.cp = cds_get_att_text(att, NULL, NULL);
    if (!data.cp) return(0);
    fprintf(gLogFP, "string       \"%s\"\n", data.cp);
    free(data.vp);

    length  = 0;
    data.vp = cds_get_att_value(att, CDS_DOUBLE, &length, NULL);
    if (!data.vp) return(0);
    log_array_values("      ", CDS_DOUBLE, length, data.vp);
    free(data.vp);

    return(1);
}

/*******************************************************************************
 *  Set Att Value Tests
 */

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

static int set_att_value_tests(void)
{
    CDSGroup *cds = gRoot;
    CDSAtt *att;
    char   *string;
    char   *old_string;

    LOG( gProgramName,
        "------------------------------------------------------------\n"
        "Set int Attribute:\n"
        "------------------------------------------------------------\n\n");

    att = cds_get_att(cds, "att_int");
    cds_print_att(gLogFP, "", 0, att);

    log_array_values("   ", CDS_SHORT, 10, (void *)ShortData);
    if (!cds_set_att_value(att, CDS_SHORT, 10, (void *)ShortData)) {
        return(0);
    }
    cds_print_att(gLogFP, "", 0, att);

    string = cds_array_to_string(CDS_INT, 10, (void *)IntData, NULL, NULL);
    fprintf(gLogFP, "string    \"%s\"\n", string);
    if (!cds_set_att_text(att, string)) {
        return(0);
    }
    cds_print_att(gLogFP, "", 0, att);
    free(string);

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "Set float Attribute:\n"
        "------------------------------------------------------------\n\n");

    att = cds_get_att(cds, "att_float");
    cds_print_att(gLogFP, "", 0, att);

    log_array_values("     ", CDS_SHORT, 10, (void *)ShortData);
    if (!cds_set_att_value(att, CDS_SHORT, 10, (void *)ShortData)) {
        return(0);
    }
    cds_print_att(gLogFP, "", 0, att);

    string = cds_array_to_string(CDS_FLOAT, 10, (void *)FloatData, NULL, NULL);
    fprintf(gLogFP, "string      \"%s\"\n", string);
    if (!cds_set_att_text(att, string)) {
        return(0);
    }
    cds_print_att(gLogFP, "", 0, att);
    free(string);

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "Set double Attribute:\n"
        "------------------------------------------------------------\n\n");

    att = cds_get_att(cds, "att_double");
    cds_print_att(gLogFP, "", 0, att);

    log_array_values("      ", CDS_INT, 10, (void *)IntData);
    if (!cds_set_att_value(att, CDS_INT, 10, (void *)IntData)) {
        return(0);
    }
    cds_print_att(gLogFP, "", 0, att);

    log_array_values("      ", CDS_FLOAT, 10, (void *)FloatData);
    if (!cds_set_att_value(att, CDS_FLOAT, 10, (void *)FloatData)) {
        return(0);
    }
    cds_print_att(gLogFP, "", 0, att);

    string = cds_array_to_string(CDS_DOUBLE, 10, (void *)DoubleData, NULL, NULL);
    fprintf(gLogFP, "string       \"%s\"\n", string);
    if (!cds_set_att_text(att, string)) {
        return(0);
    }
    cds_print_att(gLogFP, "", 0, att);
    free(string);

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "Set text Attribute:\n"
        "------------------------------------------------------------\n\n");

    att = cds_get_att(cds, "att_text");
    cds_print_att(gLogFP, "", 0, att);

    old_string = cds_get_att_text(att, NULL, NULL);

    log_array_values("    ", CDS_INT, 10, (void *)IntData);
    if (!cds_set_att_value(att, CDS_INT, 10, (void *)IntData)) {
        return(0);
    }
    cds_print_att(gLogFP, "", 0, att);

    log_array_values("    ", CDS_FLOAT, 10, (void *)FloatData);
    if (!cds_set_att_value(att, CDS_FLOAT, 10, (void *)FloatData)) {
        return(0);
    }
    cds_print_att(gLogFP, "", 0, att);

    log_array_values("    ", CDS_DOUBLE, 10, (void *)DoubleData);
    if (!cds_set_att_value(att, CDS_DOUBLE, 10, (void *)DoubleData)) {
        return(0);
    }
    cds_print_att(gLogFP, "", 0, att);

    fprintf(gLogFP, "string     \"%s\"\n", old_string);
    if (!cds_set_att_text(att, old_string)) {
        return(0);
    }
    free(old_string);

    cds_print_att(gLogFP, "", 0, att);

    return(1);
}

/*******************************************************************************
 *  Run Att Value Tests
 */

void libcds3_test_att_values(void)
{
    fprintf(stdout, "\nAttribute Value Tests:\n");

    run_test(" - get_att_value_tests",
        "get_att_value_tests", get_att_value_tests);

    run_test(" - set_att_value_tests",
        "set_att_value_tests", set_att_value_tests);
}
