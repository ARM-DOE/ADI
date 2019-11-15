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
*    $Revision: 77481 $
*    $Author: ermold $
*    $Date: 2017-04-12 20:12:06 +0000 (Wed, 12 Apr 2017) $
*    $Version:$
*
*******************************************************************************/

#include "libcds3_test.h"

extern const char *gProgramName;
extern FILE       *gLogFP;

/*******************************************************************************
 *  Test Data
 */

static CDSDataType  degC_type  = CDS_INT;
static const char  *degC_units = "degC";
static int          degC[]     = { -3, 0, 20, -9999, 60, CDS_FILL_INT, 100, 103 };
static int          degC_mv[]  = { -9999, CDS_FILL_INT };
static size_t       degC_len   = sizeof(degC)/sizeof(int);
static size_t       degC_nmv   = sizeof(degC_mv)/sizeof(int);

static CDSDataType  km_type  = CDS_BYTE;
static const char  *km_units = "km";
static signed char  km[]     = { -64, -32, -128, -8, 8, CDS_FILL_BYTE, 64, 127 };
static signed char  km_mv[]  = { -128, CDS_FILL_BYTE };
static size_t       km_len   = sizeof(km)/sizeof(signed char);
static size_t       km_nmv   = sizeof(km_mv)/sizeof(signed char);

/*******************************************************************************
 *  Test Unit Symbol Map
 */

static int symbol_map_tests(void)
{
    CDSUnitConverter  converter;
    int               status;

    LOG( gProgramName,
        "------------------------------------------------------------\n"
        "C -> degree_Celsius\n"
        "------------------------------------------------------------\n\n");

    if (!cds_map_symbol_to_unit("C", "degree_Celsius")) {
        return(0);
    }

    status = cds_get_unit_converter("C", "degC", &converter);
    if (status < 0) {
        return(0);
    }
    else if (status == 0) {
        fprintf(gLogFP, "C == degC\n");
    }
    else {
        fprintf(gLogFP, "FAILED: C != degC\n");
        return(0);
    }

    cds_free_unit_converter(converter);

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "unitless -> 1\n"
        "------------------------------------------------------------\n\n");

    if (!cds_map_symbol_to_unit("unitless", "1")) {
        return(0);
    }

    status = cds_get_unit_converter("counts", "unitless", &converter);
    if (status < 0) {
        return(0);
    }
    else if (status == 0) {
        fprintf(gLogFP, "unitless == counts\n");
    }
    else {
        fprintf(gLogFP, "FAILED: unitless != counts\n");
        return(0);
    }

    cds_free_unit_converter(converter);

    return(1);
}

/*******************************************************************************
 *  Test Unit Conversions
 */

static int units_conversion_test(
    const char  *in_units,
    const char  *out_units,

    CDSDataType  in_type,
    size_t       length,
    void        *in_data,
    CDSDataType  out_type,
    void        *out_data,
    size_t       nmissing,
    void        *in_missing,
    void        *out_missing,
    void        *out_min,
    void        *out_max,
    void        *oor_value)
{
    CDSUnitConverter  converter;
    char              in_prefix[16];
    char              out_prefix[16];
    void             *result;
    int               status;

    sprintf(in_prefix,  "%s: ", in_units);
    sprintf(out_prefix, "%s: ", out_units);

    status = cds_get_unit_converter(in_units, out_units, &converter);
    if (status < 0) {
        return(0);
    }
    else if (status == 0) {
        fprintf(gLogFP, "Units are equal: %s == %s\n", in_units, out_units);
        return(1);
    }

    log_array_values(in_prefix, in_type, length, in_data);

    result = cds_convert_units(converter,
        in_type, length, in_data, out_type, out_data,
        nmissing, in_missing, out_missing,
        out_min, oor_value, out_max, oor_value);

    cds_free_unit_converter(converter);

    log_array_values(out_prefix, out_type, length, result);

    if (!out_data) free(result);

    return(1);
}

#define degC_UNITS_TEST(out_units, out_type, out_data, out_missing) \
    units_conversion_test( \
        degC_units, out_units, degC_type, degC_len, degC, out_type, out_data, \
        degC_nmv, degC_mv, out_missing, \
        NULL, NULL, out_missing)

#define km_UNITS_TEST(out_units, out_type, out_data, out_missing) \
    units_conversion_test( \
        km_units, out_units, km_type, km_len, km, out_type, out_data, \
        km_nmv, km_mv, out_missing, \
        NULL, NULL, out_missing)

static int units_conversion_tests(void)
{
    char out_buffer[1024];
    char out_missing[1024];

    LOG( gProgramName,
        "------------------------------------------------------------\n"
        "int degC -> float degF (in place)\n"
        "------------------------------------------------------------\n\n");

    memcpy(out_buffer, degC, degC_len * sizeof(int));

    cds_get_missing_values_map(
        degC_type, degC_nmv, degC_mv, CDS_FLOAT, out_missing);

    units_conversion_test(
        degC_units, "degF", degC_type, degC_len, out_buffer, CDS_FLOAT, out_buffer,
        degC_nmv, degC_mv, out_missing,
        NULL, NULL, out_missing);

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "int degC -> float degF (with output buffer)\n"
        "------------------------------------------------------------\n\n");

    degC_UNITS_TEST("degF", CDS_FLOAT, out_buffer, out_missing);

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "int degC -> float degF (dynamic allocation)\n"
        "------------------------------------------------------------\n\n");

    degC_UNITS_TEST("degF", CDS_FLOAT, NULL, out_missing);

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "byte km -> short m\n"
        "------------------------------------------------------------\n\n");

    cds_get_missing_values_map(
        km_type, km_nmv, km_mv, CDS_SHORT, out_missing);

    km_UNITS_TEST("m", CDS_SHORT, out_buffer, out_missing);

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "byte km -> int m\n"
        "------------------------------------------------------------\n\n");

    cds_get_missing_values_map(
        km_type, km_nmv, km_mv, CDS_INT, out_missing);

    km_UNITS_TEST("m", CDS_INT, out_buffer, out_missing);

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "byte km -> float m\n"
        "------------------------------------------------------------\n\n");

    cds_get_missing_values_map(
        km_type, km_nmv, km_mv, CDS_FLOAT, out_missing);

    km_UNITS_TEST("m", CDS_FLOAT, out_buffer, out_missing);

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "byte km -> double m\n"
        "------------------------------------------------------------\n\n");

    cds_get_missing_values_map(
        km_type, km_nmv, km_mv, CDS_DOUBLE, out_missing);

    km_UNITS_TEST("m", CDS_DOUBLE, out_buffer, out_missing);

    cds_free_unit_system();

    return(1);
}

/*******************************************************************************
 *  Validate Time Units Tests
 */

static int validate_time_units_tests(void)
{
    time_t  time0 = 1339200000; // 2012-06-09 00:00:00
    time_t  time1;
    time_t  time2;
    int     ti;
    char   *time_units;
    
    LOG( gProgramName,
        "------------------------------------------------------------\n"
        "testing good time unit strings function\n"
        "------------------------------------------------------------\n\n");

    time_units = (char *)malloc(64 * sizeof(char));

    for (ti = 0; ti < 1450; ti++) {
    
        time1 = time0 + (60 * ti);

        cds_base_time_to_units_string(time1, time_units);
        time2 = cds_validate_time_units(time_units);

        if (time2 != time1) {
            ERROR( gProgramName,
                "Error converting between time_t values and units string\n"
                " -> %ld != %ld: '%s'\n",
                time1, time2, time_units);
            return(0);
        }

        fprintf(gLogFP, "%ld: '%s'\n", time2, time_units);
    }

    LOG( gProgramName,
        "\n------------------------------------------------------------\n"
        "testing bad time unit strings function\n"
        "------------------------------------------------------------\n\n");

    /***************************/

    sprintf(time_units, "seconds since 2012/06/10 00:09:00.00");
    fprintf(gLogFP, "\n%s:\n", time_units);

    time1 = cds_validate_time_units(time_units);
    fprintf(gLogFP, " - time_t: %ld\n", time1);
    fprintf(gLogFP, " - fixed:  %s\n", time_units);

    /***************************/

    sprintf(time_units, "seconds since 2012-06-10, 00:09:00");
    fprintf(gLogFP, "\n%s:\n", time_units);

    time1 = cds_validate_time_units(time_units);
    fprintf(gLogFP, " - time_t: %ld\n", time1);
    fprintf(gLogFP, " - fixed:  %s\n", time_units);

    /***************************/

    sprintf(time_units, "seconds since 2012-06-10T00:09:00Z0:00");
    fprintf(gLogFP, "\n%s:\n", time_units);

    time1 = cds_validate_time_units(time_units);
    fprintf(gLogFP, " - time_t: %ld\n", time1);
    fprintf(gLogFP, " - fixed:  %s\n", time_units);

    /***************************/

    sprintf(time_units, "seconds");
    fprintf(gLogFP, "\n%s:\n", time_units);

    time1 = cds_validate_time_units(time_units);
    fprintf(gLogFP, " - time_t: %ld\n", time1);
    fprintf(gLogFP, " - fixed:  %s\n", time_units);
    
    free(time_units);
    return(1);
}

/*******************************************************************************
 *  Run Units Function Tests
 */

void libcds3_test_units(void)
{
    fprintf(stdout, "\nUnits Tests:\n");

    run_test(" - symbol_map_tests",
        "symbol_map_tests", symbol_map_tests);

    run_test(" - units_conversion_tests",
        "units_conversion_tests", units_conversion_tests);

    run_test(" - validate_time_units_tests",
        "validate_time_units_tests", validate_time_units_tests);
}
