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

/*******************************************************************************
 *  Test offset to time value conversions
 */

static int offsets_to_times_tests()
{
    time_t     base_time;
    double     offsets[21];
    timeval_t *timevals;
    time_t    *times;
    int        i;

    fprintf(gLogFP,
        "============================================================\n"
        "Time Offsets to timeval_t values:\n"
        "============================================================\n");

    base_time = 1348488000; // 1348488000: 2012-09-24 12:00:00

    offsets[0] = -9.9;

    for (i = 1; i < 21; i++) {
        offsets[i] = offsets[i-1] + 1.1;
    }

    timevals = cds_offsets_to_timevals(
        CDS_DOUBLE, 21, base_time, offsets, NULL);

    fprintf(gLogFP, "\n");

    for (i = 0; i < 21; i++) {

        fprintf(gLogFP, "%ld + %f = %ld, %ld\n",
            base_time, offsets[i], timevals[i].tv_sec, timevals[i].tv_usec);
    }

    free(timevals);

    fprintf(gLogFP,
        "\n============================================================\n"
        "Time Offsets to time_t values:\n"
        "============================================================\n");

    times = cds_offsets_to_times(
        CDS_DOUBLE, 21, base_time, offsets, NULL);

    fprintf(gLogFP, "\n");

    for (i = 0; i < 21; i++) {

        fprintf(gLogFP, "%ld + %f = %ld\n",
            base_time, offsets[i], times[i]);
    }

    free(times);

    return(1);
}

/*******************************************************************************
 *  QC Check Tests
 */

#define PRINT_MATRIX_VECTORS(data_t) \
{ \
    data_t *datap; \
\
    for (d1 = 0; d1 < ndims; ++d1) { \
\
        memset(index, 0, ndims * sizeof(size_t)); \
\
        fprintf(fp, "    Dimension %d vectors\n", (int)d1); \
\
        for (;;) { \
\
            fprintf(fp, "        [ %d ", (int)index[0]); \
            for (d2 = 1; d2 < ndims; ++d2) { \
                fprintf(fp, ", %d", (int)index[d2]); \
            } \
            fprintf(fp, "]: "); \
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
            fprintf(fp, " (%d, %d): ", (int)start, (int)stride); \
\
            datap = (data_t *)data_vp + start; \
            fprintf(fp, " %g", (double)*datap); \
            datap += stride; \
\
            for (li = 1; li < dim_lengths[d1]; ++li) { \
                fprintf(fp, ", %g", (double)*datap); \
                datap += stride; \
            } \
\
            fprintf(fp, "\n"); \
\
            for (d2 = ndims-1; d2 > 0; --d2) { \
                if (d2 == d1)  continue; \
                if (++index[d2] != dim_lengths[d2]) break; \
                index[d2] = 0; \
            } \
\
            if (d2 == 0) { \
                if (++index[0] == dim_lengths[0]) { \
                    break; \
                } \
            } \
\
            if (d1 == 0 && index[0] == 1) break; \
        } \
    } \
}

void print_matix_vectors(
    FILE        *fp,
    CDSDataType  data_type,
    size_t       ndims,
    size_t      *dim_lengths,
    void        *data_vp)
{
    size_t  index[ndims];
    size_t  strides[ndims];
    size_t  start;
    size_t  stride;
    size_t  d1, d2, li;

    /* Calculate the dimension strides */

    strides[0]       = dim_lengths[ndims-1];
    strides[ndims-1] = 1;

    if (ndims > 2) {
        for (d1 = ndims-2; d1 > 0; --d1) {
            strides[d1] = strides[0];
            strides[0] *= dim_lengths[d1];
        }
    }

    switch (data_type) {
        case CDS_DOUBLE: PRINT_MATRIX_VECTORS(double);        break;
        case CDS_FLOAT:  PRINT_MATRIX_VECTORS(float);         break;
        case CDS_INT:    PRINT_MATRIX_VECTORS(int);           break;
        case CDS_SHORT:  PRINT_MATRIX_VECTORS(short);         break;
        case CDS_BYTE:   PRINT_MATRIX_VECTORS(signed char);   break;
        case CDS_CHAR:   PRINT_MATRIX_VECTORS(unsigned char); break;
        /* NetCDF4 extended data types */
        case CDS_INT64:  PRINT_MATRIX_VECTORS(long long);     break;
        case CDS_UBYTE:  PRINT_MATRIX_VECTORS(unsigned char);  break;
        case CDS_USHORT: PRINT_MATRIX_VECTORS(unsigned short); break;
        case CDS_UINT:   PRINT_MATRIX_VECTORS(unsigned int);   break;
        case CDS_UINT64: PRINT_MATRIX_VECTORS(unsigned long long); break;
        default:
            break;
    }
}

static int qc_check_tests(void)
{
    size_t  dim_lengths[]   = {  3, 4, 5 };
    size_t  ndims           = sizeof(dim_lengths)/sizeof(size_t);

    float   missings[]      = { -9999.0, -8888.0 };
    int     missing_flags[] = {  0x01,     0x02   };
    size_t  nmissings       = sizeof(missings)/sizeof(float);

    float   min_value       = 22.0;
    int     min_flag        = 0x04;

    float   max_value       = 77.0;
    int     max_flag        = 0x08;

    float   deltas[]        = { 20.0,  5.0, 1.0  };
    int     delta_flags[]   = { 0x10, 0x20, 0x40 };
    size_t  ndeltas         = sizeof(deltas)/sizeof(float);

    int     bad_flags       = missing_flags[0] | missing_flags[1]
                            | min_flag | max_flag;

    size_t  prev_dim_lengths[ndims];
    float  *prev_data;
    int    *prev_flags;
    float  *data;
    int    *qc_flags;
    int    *data_flags;
    size_t  sample_size;
    size_t  nvalues;
    size_t  di, vi;

    /* Create arrays of test data */

    sample_size = 1;
    for (di = 1; di < ndims; ++di) {
        sample_size *= dim_lengths[di];
    }

    nvalues   = dim_lengths[0] * sample_size;

    prev_data   = (float *)calloc(sample_size, sizeof(float));
    prev_flags  =   (int *)calloc(sample_size, sizeof(int));
    data        = (float *)calloc(nvalues,     sizeof(float));
    data_flags  =   (int *)calloc(nvalues,     sizeof(int));

    for (vi = 0, di = 0; di < sample_size; ++di) {
        prev_data[di] = (float)vi++ + 0.5;
    }
    
    prev_data[2]  = missings[0];
    prev_flags[2] = missing_flags[0];

    prev_data[3] -= 1.0;

    for (di = 0; di < nvalues; ++di) {
        data[di] = (float)vi++  + 0.5;
    }

    for (di = 10; di < 15; ++di) {
        data[di] = missings[0];
    }

    for (di = 47; di < 52; ++di) {
        data[di] = missings[1];
    }

    for (di = 18; di < 23; ++di) {
        data[di] = 40;
    }

    for (di = 35; di < 40; ++di) {
        data[di] = 57;
    }

    memcpy(prev_dim_lengths, dim_lengths, ndims * sizeof(size_t));
    prev_dim_lengths[0] = 1;

    fprintf(gLogFP,
        "------------------------------------------------------------\n"
        "Test Data Values\n"
        "------------------------------------------------------------\n\n");

    fprintf(gLogFP, "Previous Sample:\n\n");

    print_matix_vectors(
        gLogFP,
        CDS_FLOAT,
        ndims,
        prev_dim_lengths,
        prev_data);

    fprintf(gLogFP, "\nData:\n\n");

    print_matix_vectors(
        gLogFP,
        CDS_FLOAT,
        ndims,
        dim_lengths,
        data);

    fprintf(gLogFP,
        "\n------------------------------------------------------------\n"
        "QC Limit Checks: min = %g; max = %g\n"
        "------------------------------------------------------------\n\n",
        min_value, max_value);

    qc_flags = cds_qc_limit_checks(
            CDS_FLOAT,
            nvalues,
            (void *)data,
            0, //nmissings,
            NULL,
            missing_flags,
            (void *)&min_value,
            min_flag,
            (void *)&max_value,
            max_flag,
            NULL);

    print_matix_vectors(
        gLogFP,
        CDS_INT,
        ndims,
        dim_lengths,
        qc_flags);

    free(qc_flags);

    fprintf(gLogFP,
        "\n------------------------------------------------------------\n"
        "QC Limit Checks: missing = %g \n"
        "------------------------------------------------------------\n\n",
        missings[0]);

    qc_flags = cds_qc_limit_checks(
            CDS_FLOAT,
            nvalues,
            (void *)data,
            1, //nmissings,
            (void *)missings,
            missing_flags,
            NULL,
            min_flag,
            NULL,
            max_flag,
            NULL);

    print_matix_vectors(
        gLogFP,
        CDS_INT,
        ndims,
        dim_lengths,
        qc_flags);

    free(qc_flags);

    fprintf(gLogFP,
        "\n------------------------------------------------------------\n"
        "QC Limit Checks: min = %g; max = %g; missing = %g \n"
        "------------------------------------------------------------\n\n",
        min_value, max_value, missings[0]);

    qc_flags = cds_qc_limit_checks(
            CDS_FLOAT,
            nvalues,
            (void *)data,
            1, //nmissings,
            (void *)missings,
            missing_flags,
            (void *)&min_value,
            min_flag,
            (void *)&max_value,
            max_flag,
            NULL);

    print_matix_vectors(
        gLogFP,
        CDS_INT,
        ndims,
        dim_lengths,
        qc_flags);

    free(qc_flags);

    fprintf(gLogFP,
        "\n------------------------------------------------------------\n"
        "QC Limit Checks: min = %g; max = %g; missings = [ %g, %g ] \n"
        "------------------------------------------------------------\n\n",
        min_value, max_value, missings[0], missings[1]);

    qc_flags = cds_qc_limit_checks(
            CDS_FLOAT,
            nvalues,
            (void *)data,
            nmissings,
            (void *)missings,
            missing_flags,
            (void *)&min_value,
            min_flag,
            (void *)&max_value,
            max_flag,
            NULL);

    print_matix_vectors(
        gLogFP,
        CDS_INT,
        ndims,
        dim_lengths,
        qc_flags);

    fprintf(gLogFP,
        "\n------------------------------------------------------------\n"
        "QC Delta Checks across sample dimension, no previous sample:\n"
        "\n"
        "  delta = %g\n"
        "------------------------------------------------------------\n\n",
        deltas[0]);

    memcpy(data_flags, qc_flags, nvalues);

    cds_qc_delta_checks(
        CDS_FLOAT,
        ndims,
        dim_lengths,
        (void *)data,
        1, // ndeltas,
        (void *)deltas,
        delta_flags,
        (void *)NULL,
        (int *)NULL,
        bad_flags,
        qc_flags);

    print_matix_vectors(
        gLogFP,
        CDS_INT,
        ndims,
        dim_lengths,
        qc_flags);

    fprintf(gLogFP,
        "\n------------------------------------------------------------\n"
        "QC Delta Checks across sample dimension, with previous sample:\n"
        "\n"
        "  delta = %g\n"
        "------------------------------------------------------------\n\n",
        deltas[0]);

    memcpy(qc_flags, data_flags, nvalues);

    cds_qc_delta_checks(
        CDS_FLOAT,
        ndims,
        dim_lengths,
        (void *)data,
        1, // ndeltas,
        (void *)deltas,
        delta_flags,
        (void *)prev_data,
        prev_flags,
        bad_flags,
        qc_flags);

    print_matix_vectors(
        gLogFP,
        CDS_INT,
        ndims,
        dim_lengths,
        qc_flags);

    fprintf(gLogFP,
        "\n------------------------------------------------------------\n"
        "QC Delta Checks: deltas[0] = %g; deltas[1] = %g; deltas[2] = %g\n"
        "------------------------------------------------------------\n\n",
        deltas[0], deltas[1], deltas[2]);

    memcpy(qc_flags, data_flags, nvalues);

    cds_qc_delta_checks(
        CDS_FLOAT,
        ndims,
        dim_lengths,
        (void *)data,
        ndeltas,
        (void *)deltas,
        delta_flags,
        (void *)prev_data,
        prev_flags,
        bad_flags,
        qc_flags);

    print_matix_vectors(
        gLogFP,
        CDS_INT,
        ndims,
        dim_lengths,
        qc_flags);

    free(qc_flags);
    free(prev_data);
    free(prev_flags);
    free(data);
    free(data_flags);

    return(1);
}

/*******************************************************************************
 *  Print Array Tests
 */

static char *test_string_1 =
    "The cds_print_array() function can be used to print an array of data "
    "values. By default data arrays will be beigin and end with open and close "
    "brackets, and character arrays will begin and end with a quote and quotes "
    "inside the string will be \"escaped\" with a backslash character.\n"
    "\n"
    "Parameters:\n"
    "    fp      - pointer to the output stream to write to\n"
    "    type    - data type of the array\n"
    "    length  - number of values to print\n"
    "    array   - pointer to the array of values\n"
    "    indent  - line indent string to use for new lines\n"
    "    maxline - maximum number of characters to print per line,\n"
    "              or 0 for no line breaks in numeric arrays and to only\n"
    "              split character arrays on newlines.\n"
    "    linepos - starting line position when this function was called,\n"
    "              ignored if maxline == 0\n"
    "    flags   - control flags:\n"
    "                - 0x01: Print data type name for numeric arrays.\n"
    "                - 0x02: Print padded data type name for numeric arrays.\n"
    "                - 0x04: Print data type name at end of numeric arrays.\n"
    "                - 0x08: Do not print brackets around numeric arrays.\n"
    "                - 0x10: Strip trailing NULLs from the end of strings.\n"
    "\n"
    "Returns:\n"
    "    - number of bytes printed\n"
    "    - (size_t)-1 if an error occurs\n";

static char *test_string_2 =
    "supercalifragilisticexpialidocious\n"
    "\n"
    "even though the sound of it\n"
    "is something quite attrocious\n"
    "if you say it loud enough\n"
    "you'll aways sound precocious\n"
    "\n"
    "supercalifragilisticexpialidocious\n"
    "\n"
    "i was afriad to speak\n"
    "when i was just a lad\n"
    "my father gave my nose a tweak\n"
    "and told me i was bad\n"
    "\n"
    "and then one day i heard a word\n"
    "to save my aching nose\n"
    "it was the biggest word you ever heard\n"
    "and this is how it goes..\n"
    "\n"
    "supercalifragilisticexpialidocious\n";

static void print_char_array_test(
    int         sprint,
    const char *string,
    const char *indent,
    size_t      maxline,
    int         flags)
{
    const char *indentp = (indent) ? indent : "";
    size_t      linepos = 0;
    size_t      length;
    size_t      nchars;
    char       *out_string;

    fprintf(gLogFP,
        "\n------------------------------------------------------------\n"
        "maxline = %d, indent = \"%s\", flags = 0x%x\n"
        "------------------------------------------------------------\n\n",
        (int)maxline, indentp, flags);

    length = strlen(string) + 1;

    if (indent) {
        linepos = fprintf(gLogFP, "string: ");
    }

    if (sprint) {

        out_string = cds_sprint_array(
            CDS_CHAR, length, (void *)string, &nchars, NULL,
            indent, maxline, linepos, flags);

        if (out_string) {
            fprintf(gLogFP, "%s", out_string);
            free(out_string);
        }
    }
    else {
        nchars  = cds_print_array(
            gLogFP, CDS_CHAR, length, (void *)string,
            indent, maxline, linepos, flags);
    }
    fprintf(gLogFP, "\n\nlength = %d\n", (int)nchars);
}

static void print_data_array_tests(
    int         sprint,
    const char *indent,
    size_t      maxline,
    int         flags)
{
    const char  *indentp = (indent) ? indent : "";
    size_t       linepos = 0;
    size_t       ntypes;
    CDSDataType *types;
    size_t       length;
    void        *array;
    size_t       nchars;
    char        *out_string;
    size_t       ti;

    fprintf(gLogFP,
        "\n------------------------------------------------------------\n"
        "maxline = %d, indent = \"%s\", flags = 0x%x\n"
        "------------------------------------------------------------\n",
        (int)maxline, indentp, flags);

    get_test_data_types(&ntypes, &types);

    for (ti = 0; ti < ntypes; ti++) {

        fprintf(gLogFP, "\n");

        get_test_data(types[ti], NULL, &length, &array, NULL, NULL, NULL);

        if (sprint) {

            out_string = cds_sprint_array(
                types[ti], length, array, &nchars, NULL,
                indent, maxline, linepos, flags);

            if (out_string) {
                fprintf(gLogFP, "%s", out_string);
                free(out_string);
            }
        }
        else {
            nchars = cds_print_array(
                gLogFP, types[ti], length, array,
                indent, maxline, linepos, flags);
        }

        fprintf(gLogFP, "\nlength = %d\n", (int)nchars);
    }
}

static void print_int_array_test(
    int         sprint,
    size_t      length,
    int        *array,
    const char *indent,
    size_t      maxline,
    int         flags)
{
    const char *indentp = (indent) ? indent : "";
    size_t      nchars;
    char       *out_string;

    fprintf(gLogFP,
        "\n------------------------------------------------------------\n"
        "maxline = %d, indent = \"%s\", flags = 0x%x\n"
        "------------------------------------------------------------\n\n",
        (int)maxline, indentp, flags);

    if (sprint) {

        out_string = cds_sprint_array(
            CDS_INT, length, (void *)array, &nchars, NULL,
            indent, maxline, 0, flags);

        if (out_string) {
            fprintf(gLogFP, "%s", out_string);
            free(out_string);
        }
    }
    else {
        nchars = cds_print_array(
            gLogFP, CDS_INT, length, (void *)array,
            indent, maxline, 0, flags);

    }

    fprintf(gLogFP, "\n\nlength = %d\n", (int)nchars);
}

static void _print_array_tests(int sprint)
{
    int    int_data[100];
    size_t i;

    fprintf(gLogFP,
        "============================================================\n"
        "Integer Array Tests\n"
        "============================================================\n");

    for (i = 0; i < 100; i++) int_data[i] = i;

    print_int_array_test(sprint, 100, int_data, NULL,        0, 0x00);
    print_int_array_test(sprint, 100, int_data, NULL,       80, 0x00);
    print_int_array_test(sprint, 100, int_data, "        ", 80, 0x02);
    print_int_array_test(sprint, 100, int_data, NULL,       30, 0x00);
    print_int_array_test(sprint, 100, int_data, "        ", 30, 0x02);
    print_int_array_test(sprint, 100, int_data, "        ",  1, 0x02);
    print_int_array_test(sprint, 100, int_data, NULL,        1, 0x08);

    fprintf(gLogFP,
        "\n============================================================\n"
        "All Data Type Array Tests\n"
        "============================================================\n");

    print_data_array_tests(sprint, NULL,        0, 0x00);
    print_data_array_tests(sprint, "        ",  0, 0x01);
    print_data_array_tests(sprint, "        ", 80, 0x02);
    print_data_array_tests(sprint, NULL,       80, 0x04);
    print_data_array_tests(sprint, NULL,       80, 0x08);
    print_data_array_tests(sprint, "        ", 40, 0x02);
    print_data_array_tests(sprint, "        ",  1, 0x02);

    fprintf(gLogFP,
        "\n============================================================\n"
        "String 1 Character Array Tests\n"
        "============================================================\n");

    print_char_array_test(sprint, test_string_1, NULL,        0, 0x00);
    print_char_array_test(sprint, test_string_1, "        ",  0, 0x00);
    print_char_array_test(sprint, test_string_1, "        ",  0, 0x10);

    print_char_array_test(sprint, test_string_1, NULL,       80, 0x00);
    print_char_array_test(sprint, test_string_1, "        ", 80, 0x00);
    print_char_array_test(sprint, test_string_1, "        ", 50, 0x01);

    fprintf(gLogFP,
        "\n============================================================\n"
        "String 2 Character Array Tests\n"
        "============================================================\n");

    print_char_array_test(sprint, test_string_2, NULL,        0, 0x00);
    print_char_array_test(sprint, test_string_2, "        ",  0, 0x10);

    print_char_array_test(sprint, test_string_2, NULL,       20, 0x00);
    print_char_array_test(sprint, test_string_2, "        ",  1, 0x10);
    print_char_array_test(sprint, test_string_2, NULL,        1, 0x10);
}

static int print_array_tests(void)
{
    _print_array_tests(0);
    return(1);
}

/*******************************************************************************
 *  Print Array to String Tests
 */

static int sprint_data_array_buffer_tests(
    size_t      buflen,
    const char *indent,
    size_t      maxline,
    int         flags)
{
    const char  *indentp = (indent) ? indent : "";
    size_t       linepos = 0;
    char        *buffer;
    size_t       ntypes;
    CDSDataType *types;
    size_t       length;
    void        *array;
    size_t       nchars;
    char        *out_string;
    size_t       ti;

    buffer = (char *)malloc(buflen * sizeof(char));
    if (!buffer) {
        ERROR( gProgramName,
            "memory allocation error in: sprint_data_array_buffer_tests\n");
        return(0);
    }

    fprintf(gLogFP,
        "\n------------------------------------------------------------\n"
        "buflen = %d, maxline = %d, indent = \"%s\", flags = 0x%x\n"
        "------------------------------------------------------------\n",
        (int)buflen, (int)maxline, indentp, flags);

    /* data array tests */

    get_test_data_types(&ntypes, &types);

    for (ti = 0; ti < ntypes; ti++) {

        fprintf(gLogFP, "\n");

        get_test_data(types[ti], NULL, &length, &array, NULL, NULL, NULL);

        nchars = buflen;

        out_string = cds_sprint_array(
            types[ti], length, array, &nchars, buffer,
            indent, maxline, linepos, flags);

        fprintf(gLogFP, "%s\n\nlength = %d\n", out_string, (int)nchars);
    }

    free(buffer);
    return(1);
}

static int sprint_char_array_buffer_test(
    const char *string,
    size_t      buflen,
    const char *indent,
    size_t      maxline,
    int         flags)
{
    const char *indentp = (indent) ? indent : "";
    size_t      linepos = 0;
    char       *buffer;
    size_t      length;
    char       *out_string;

    buffer = (char *)malloc(buflen * sizeof(char));
    if (!buffer) {
        ERROR( gProgramName,
            "memory allocation error in: sprint_char_array_buffer_test\n");
        return(0);
    }

    fprintf(gLogFP,
        "\n------------------------------------------------------------\n"
        "buflen = %d, maxline = %d, indent = \"%s\", flags = 0x%x\n"
        "------------------------------------------------------------\n",
        (int)buflen, (int)maxline, indentp, flags);

    length = strlen(string) + 1;

    if (indent) {
        linepos = fprintf(gLogFP, "string: ");
    }

    out_string = cds_sprint_array(
        CDS_CHAR, length, (void *)string, &buflen, buffer,
        indent, maxline, linepos, flags);

    fprintf(gLogFP, "%s\n\nlength = %d\n", out_string, (int)buflen);

    free(buffer);
    return(1);
}

static int sprint_array_tests(void)
{
    char *s;

    _print_array_tests(1);

    fprintf(gLogFP,
        "\n============================================================\n"
        "Print Data Array to String Tests Using Output Buffer\n"
        "============================================================\n");

    if (!sprint_data_array_buffer_tests(512, NULL,        0, 0x00))  return(0);
    if (!sprint_data_array_buffer_tests(256, "        ", 80, 0x02))  return(0);
    if (!sprint_data_array_buffer_tests(128, NULL,       60, 0x00))  return(0);
    if (!sprint_data_array_buffer_tests(256, NULL,       80, 0x00))  return(0);

    fprintf(gLogFP,
        "\n============================================================\n"
        "String 1 Character Array to String Tests Using Output Buffer\n"
        "============================================================\n");

    s = test_string_1;

    if (!sprint_char_array_buffer_test(s,1024, NULL,        0, 0x00)) return(0);
    if (!sprint_char_array_buffer_test(s,1024, "        ",  0, 0x10)) return(0);
    if (!sprint_char_array_buffer_test(s, 256, NULL,       80, 0x00)) return(0);
    if (!sprint_char_array_buffer_test(s, 256, "        ", 80, 0x00)) return(0);

    fprintf(gLogFP,
        "\n============================================================\n"
        "String 2 Character Array to String Tests Using Output Buffer\n"
        "============================================================\n");

    s = test_string_2;

    if (!sprint_char_array_buffer_test(s,1024, NULL,        0, 0x00)) return(0);
    if (!sprint_char_array_buffer_test(s,1024, "        ",  0, 0x10)) return(0);
    if (!sprint_char_array_buffer_test(s, 256, NULL,       80, 0x00)) return(0);
    if (!sprint_char_array_buffer_test(s, 256, "        ", 80, 0x00)) return(0);

    return(1);
}

/*******************************************************************************
 *  Array to String Tests
 */

static int array_to_string_test()
{
    char         buffer[512];
    size_t       buflen;
    size_t       ntypes;
    CDSDataType *types;
    const char  *type_name;
    size_t       length;
    void        *array;
    char        *string;
    size_t       ti;

    get_test_data_types(&ntypes, &types);

    fprintf(gLogFP,
        "============================================================\n"
        "Array to String Tests (with buffer length 80):\n"
        "============================================================\n");

    for (ti = 0; ti < ntypes; ti++) {

        fprintf(gLogFP, "\n");

        get_test_data(types[ti], NULL, &length, &array, NULL, NULL, NULL);
        type_name = cds_data_type_name(types[ti]);

        buflen = 80;
        string = cds_array_to_string(types[ti], length, array, &buflen, buffer);

        fprintf(gLogFP, "%-7s \"%s\"\n", type_name, string);
        fprintf(gLogFP, "length = %d\n", (int)buflen);
    }

    fprintf(gLogFP,
        "\n============================================================\n"
        "Array to String Tests (with buffer length 512):\n"
        "============================================================\n");

    for (ti = 0; ti < ntypes; ti++) {

        fprintf(gLogFP, "\n");

        get_test_data(types[ti], NULL, &length, &array, NULL, NULL, NULL);
        type_name = cds_data_type_name(types[ti]);

        buflen = 512;
        string = cds_array_to_string(types[ti], length, array, &buflen, buffer);

        fprintf(gLogFP, "%-7s \"%s\"\n", type_name, string);
        fprintf(gLogFP, "length = %d\n", (int)buflen);
    }

    fprintf(gLogFP,
        "\n============================================================\n"
        "Array to String Tests (with dynamic allocation):\n"
        "============================================================\n");

    for (ti = 0; ti < ntypes; ti++) {

        fprintf(gLogFP, "\n");

        get_test_data(types[ti], NULL, &length, &array, NULL, NULL, NULL);
        type_name = cds_data_type_name(types[ti]);

        string = cds_array_to_string(types[ti], length, array, &buflen, NULL);

        fprintf(gLogFP, "%-7s \"%s\"\n", type_name, string);
        fprintf(gLogFP, "length = %d\n", (int)buflen);

        free(string);
    }

    return(1);
}

/*******************************************************************************
 *  String to Array Tests
 */

static int string_to_array_test()
{
    char         buffer[512];
    size_t       ntypes;
    CDSDataType *types;
    size_t       length;
    void        *array;
    const char  *string;
    size_t       ti;

    get_test_data_types(&ntypes, &types);

    fprintf(gLogFP,
        "============================================================\n"
        "String to Array Tests (with buffer length 10):\n"
        "============================================================\n");

    for (ti = 0; ti < ntypes; ti++) {

        fprintf(gLogFP, "\n");

        get_test_data(types[ti], NULL, NULL, NULL, NULL, NULL, &string);

        length = 10;
        array = cds_string_to_array(string, types[ti], &length, buffer);
        cds_print_array(gLogFP, types[ti], length, array, NULL, 0, 0, 0x2);
        fprintf(gLogFP, "\nlength = %d\n", (int)length);
    }

    fprintf(gLogFP,
        "\n============================================================\n"
        "String to Array Tests (with buffer length 30):\n"
        "============================================================\n\n");

    for (ti = 0; ti < ntypes; ti++) {

        fprintf(gLogFP, "\n");

        get_test_data(types[ti], NULL, NULL, NULL, NULL, NULL, &string);

        length = 30;
        array = cds_string_to_array(string, types[ti], &length, buffer);
        cds_print_array(gLogFP, types[ti], length, array, NULL, 0, 0, 0x2);
        fprintf(gLogFP, "\nlength = %d\n", (int)length);
    }

    fprintf(gLogFP,
        "\n============================================================\n"
        "String to Array Test: (with dynamic allocation)\n"
        "============================================================\n\n");

    for (ti = 0; ti < ntypes; ti++) {

        fprintf(gLogFP, "\n");

        get_test_data(types[ti], NULL, NULL, NULL, NULL, NULL, &string);

        array = cds_string_to_array(string, types[ti], &length, NULL);
        cds_print_array(gLogFP, types[ti], length, array, NULL, 0, 0, 0x2);
        fprintf(gLogFP, "\nlength = %d\n", (int)length);
        free(array);
    }

    fprintf(gLogFP,
        "\n============================================================\n"
        "String to Array Test: (with out-of-range values - using limits)\n"
        "============================================================\n\n");

    get_test_data(CDS_DOUBLE, NULL, NULL, NULL, NULL, NULL, &string);

    for (ti = 0; ti < ntypes; ti++) {

        fprintf(gLogFP, "\n");

        array = cds_string_to_array(string, types[ti], &length, NULL);
        cds_print_array(gLogFP, types[ti], length, array, NULL, 0, 0, 0x2);
        fprintf(gLogFP, "\nlength = %d\n", (int)length);
        free(array);
    }

    fprintf(gLogFP,
        "\n============================================================\n"
        "String to Array Test: (with out-of-range values - using fills)\n"
        "============================================================\n\n");

    get_test_data(CDS_DOUBLE, NULL, NULL, NULL, NULL, NULL, &string);

    for (ti = 0; ti < ntypes; ti++) {

        fprintf(gLogFP, "\n");

        array = cds_string_to_array_use_fill(string, types[ti], &length, NULL);
        cds_print_array(gLogFP, types[ti], length, array, NULL, 0, 0, 0x2);
        fprintf(gLogFP, "\nlength = %d\n", (int)length);
        free(array);
    }

    return(1);
}

/*******************************************************************************
 *  Run Utility Function Tests
 */

void libcds3_test_utils(void)
{
    fprintf(stdout, "\nUtility Functions Tests:\n");

    run_test(" - qc_check_tests",
        "qc_check_tests", qc_check_tests);

    run_test(" - offsets_to_times_tests",
        "offsets_to_times_tests", offsets_to_times_tests);

    run_test(" - print_array_tests",
        "print_array_tests", print_array_tests);

    run_test(" - sprint_array_tests",
        "sprint_array_tests", sprint_array_tests);

    run_test(" - array_to_string_test",
        "array_to_string_test", array_to_string_test);

    run_test(" - string_to_array_test",
        "string_to_array_test", string_to_array_test);
}
