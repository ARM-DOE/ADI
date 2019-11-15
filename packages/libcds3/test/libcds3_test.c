/*******************************************************************************
*
*  COPYRIGHT (C) 2010 Battelle Memorial Institute.  All Rights Reserved.
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
*    $Version: afl-libcds3-1.1-0 $
*
*******************************************************************************/

#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "libcds3_test.h"

const char *gProgramName = NULL;
const char *gTopTestDir  = NULL;
CDSGroup   *gRoot        = NULL;
LogFile    *gLog         = NULL;
FILE       *gLogFP       = NULL;
int         gFailCount   = 0;

/*******************************************************************************
 *  Create test data arrays
 */

static size_t NumTestDataTypes =  5;
static size_t NumTestValues    = 19;
static size_t NumTestFills     =  2;

static CDSDataType TestDataTypes[] = {
    CDS_BYTE,
    CDS_SHORT,
    CDS_INT,
    CDS_FLOAT,
    CDS_DOUBLE
};

static const char *TestBytesString =
    "-128, -123, -90, -78, -56, -34, -12, -3, "
    "0, 3, 12, 34, 56, 78, 90, 123, 127, "
    "-99, -127";

static signed char TestByteFills[] = { -99, CDS_FILL_BYTE };
static signed char TestBytes[] = {
    CDS_MIN_BYTE,
       -123,
        -90,
        -78,
        -56,
        -34,
        -12,
         -3,
          0,
          3,
         12,
         34,
         56,
         78,
         90,
        123,
    CDS_MAX_BYTE,
        -99,
    CDS_FILL_BYTE,
};

static const char *TestShortsString =
    "-32768, -128, -8901, -4567, -123, -90, -78, "
    "0, 78, 90, 123, 4567, 8901, 23456, 127, 32767, "
    "-9999, -127, -32767";

static short TestShortFills[] = { -9999, CDS_FILL_SHORT };
static short TestShorts[] = {
    CDS_MIN_SHORT,
    CDS_MIN_BYTE,
      -8901,
      -4567,
       -123,
        -90,
        -78,
          0,
         78,
         90,
        123,
       4567,
       8901,
      23456,
    CDS_MAX_BYTE,
    CDS_MAX_SHORT,
      -9999,
    CDS_FILL_BYTE,
    CDS_FILL_SHORT,
};

static const char *TestIntsString =
    "-2147483648, -32768, -128, -345678, -89012, -4567, -123, "
    "0, 123, 4567, 89012, 345678, 127, 32767, 2147483647, "
    "-9999, -127, -32767, -2147483647";

static int TestIntFills[] = { -9999, CDS_FILL_INT };
static int TestInts[] = {
    CDS_MIN_INT,
    CDS_MIN_SHORT,
    CDS_MIN_BYTE,
    -345678,
     -89012,
      -4567,
       -123,
          0,
        123,
       4567,
      89012,
     345678,
    CDS_MAX_BYTE,
    CDS_MAX_SHORT,
    CDS_MAX_INT,
      -9999,
    CDS_FILL_BYTE,
    CDS_FILL_SHORT,
    CDS_FILL_INT,
};

static const char *TestFloatsString =
    "-3.402823e+38, -2.147484e+09, -32768, -128, -123.4567, -34.56789, "
    "0, 34.56789, 123.4567, 123.5678, 127, 32767, 2.147484e+09, 3.402823e+38, "
    "-9999, -127, -32767, -2.147484e+09, 9.96921e+36";

static float TestFloatFills[] = { -9999.0, CDS_FILL_FLOAT };
static float TestFloats[] = {
    CDS_MIN_FLOAT,
    CDS_MIN_INT,
    CDS_MIN_SHORT,
    CDS_MIN_BYTE,
       -123.4567,
        -34.56789,
          0.0,
         34.56789,
        123.4567,
        123.5678,
    CDS_MAX_BYTE,
    CDS_MAX_SHORT,
    CDS_MAX_INT,
    CDS_MAX_FLOAT,
      -9999.0,
    CDS_FILL_BYTE,
    CDS_FILL_SHORT,
    CDS_FILL_INT,
    CDS_FILL_FLOAT,
};

static const char *TestDoublesString =
    "-1.79769313486232e+308, -3.40282346638529e+38, -2147483648, -32768, "
    "-128, -123.456789123456, 0, 123.456789123456, 127, 32767, 2147483647, "
    "3.40282346638529e+38, 1.79769313486232e+308, -9999, -127, -32767, "
    "-2147483647, 9.96920996838687e+36, 9.96920996838687e+36";

static double TestDoubleFills[] = { -9999.0, CDS_FILL_DOUBLE };
static double TestDoubles[] = {
    CDS_MIN_DOUBLE,
    CDS_MIN_FLOAT,
    CDS_MIN_INT,
    CDS_MIN_SHORT,
    CDS_MIN_BYTE,
       -123.456789123456,
          0.0,
        123.456789123456,
    CDS_MAX_BYTE,
    CDS_MAX_SHORT,
    CDS_MAX_INT,
    CDS_MAX_FLOAT,
    CDS_MAX_DOUBLE,
      -9999.0,
    CDS_FILL_BYTE,
    CDS_FILL_SHORT,
    CDS_FILL_INT,
    CDS_FILL_FLOAT,
    CDS_FILL_DOUBLE
};

void get_test_data_types(
    size_t        *ntypes,
    CDSDataType  **types)
{
    if (ntypes) *ntypes = NumTestDataTypes;
    if (types)  *types  = TestDataTypes;
}

void get_test_data(
    CDSDataType  type,
    size_t      *type_size,
    size_t      *nvals,
    void       **values,
    size_t      *nfills,
    void       **fills,
    const char **string)
{
    if (type_size) *type_size = cds_data_type_size(type);
    if (nvals)     *nvals     = NumTestValues;
    if (nfills)    *nfills    = NumTestFills;

    switch (type) {
        case CDS_DOUBLE:
            if (values) *values = (void *)TestDoubles;
            if (fills)  *fills  = (void *)TestDoubleFills;
            if (string) *string = TestDoublesString;
            break;
        case CDS_FLOAT:
            if (values) *values = (void *)TestFloats;
            if (fills)  *fills  = (void *)TestFloatFills;
            if (string) *string = TestFloatsString;
            break;
        case CDS_INT:
            if (values) *values = (void *)TestInts;
            if (fills)  *fills  = (void *)TestIntFills;
            if (string) *string = TestIntsString;
            break;
        case CDS_SHORT:
            if (values) *values = (void *)TestShorts;
            if (fills)  *fills  = (void *)TestShortFills;
            if (string) *string = TestShortsString;
            break;
        case CDS_BYTE:
            if (values) *values = (void *)TestBytes;
            if (fills)  *fills  = (void *)TestByteFills;
            if (string) *string = TestBytesString;
            break;
        default: break;
    }
}

/*******************************************************************************
 *  Create test variables
 */

CDSVar *create_time_var(
    CDSGroup    *group,
    time_t       base_time,
    size_t       nsamples,
    double       delta)
{
    const char  *name        = "time";
    const char  *dim_names[] = { "time", NULL };
    int          ndims       = 1;
    CDSVar      *var;
    double      *datap;
    size_t       di;

    /* define variable */

    if (!(var = cds_define_var(group, name, CDS_DOUBLE, ndims, dim_names)))
        return((CDSVar *)NULL);

    if (!cds_set_base_time(var, NULL, base_time))
        goto RETURN_ERROR;

    /* create data */

    if (nsamples) {

        if (!(datap = (double *)cds_alloc_var_data(var, 0, nsamples)))
            goto RETURN_ERROR;

        for (di = 0; di < nsamples; di++) {
            datap[di] = di * delta;
        }
    }

    return(var);

    RETURN_ERROR:

    cds_delete_var(var);
    return((CDSVar *)NULL);
}

CDSVar *create_temperature_var(
    CDSGroup    *group,
    CDSDataType  type,
    size_t       nsamples,
    int          with_missing,
    int          with_fill)
{
    const char  *name          = "temperature";
    const char  *dim_names[]   = { "time", NULL };
    int          ndims         = 1;
    const char  *long_name     = "Temperature variable";
    const char  *units         = "degC";
    double       valid_min     = -10.0;
    double       valid_max     = 110.0;
    double       valid_delta   =   1.0;
    double       missing_value = (type == CDS_BYTE) ? -99.0 : -9999.0;
    double       _FillValue    = CDS_FILL_DOUBLE;
    CDSVar      *var;
    double      *datap;
    double       delta;
    size_t       di;
    int          mv_index;

    /* define variable */

    if (!(var = cds_define_var(group, name, CDS_DOUBLE, ndims, dim_names)))
        return((CDSVar *)NULL);

    if (!cds_define_att_text(var, "long_name",   long_name)   ||
        !cds_define_att_text(var, "units",       units)       ||
        !cds_define_att     (var, "valid_min",   CDS_DOUBLE, 1, &valid_min)   ||
        !cds_define_att     (var, "valid_max",   CDS_DOUBLE, 1, &valid_max)   ||
        !cds_define_att     (var, "valid_delta", CDS_DOUBLE, 1, &valid_delta)) {

        goto RETURN_ERROR;
    }

    if (!define_missing_value_atts(
        var, CDS_DOUBLE, missing_value, with_missing, with_fill)) {

        return(0);
    }

    /* create data */

    if (nsamples) {

        if (!(datap = (double *)cds_alloc_var_data(var, 0, nsamples)))
            goto RETURN_ERROR;

        delta = (valid_max - valid_min + 20.0) / (nsamples - 1);

        for (di = 0; di < nsamples; di++) {
            datap[di] = (valid_min - 10.0) + (di * delta);
        }

        mv_index = 0;
        if (with_fill)    datap[mv_index++] = _FillValue;
        if (with_missing) datap[mv_index]   = missing_value;
    }

    /* change variable type */

    if (!cds_change_var_type(var, type))
        goto RETURN_ERROR;

    return(var);

    RETURN_ERROR:

    cds_delete_var(var);
    return((CDSVar *)NULL);
}

int define_missing_value_atts(
    CDSVar      *var,
    CDSDataType  type,
    double       dval,
    int          define_miss,
    int          define_fill)
{
    CDSGroup     *grp  = (CDSGroup *)var->parent;
    signed char   bval = (signed char)dval;
    unsigned char cval = (unsigned char)dval;
    short         sval = (short)dval;
    int           ival = (int)dval;
    float         fval = (float)dval;
    void         *fill;
    void         *miss;
    char          fillbuf[8];
    char          missstr[128];
    char          fillstr[128];

    cds_get_default_fill_value(type, &fillbuf);
    fill = (void *)&fillbuf;

    switch(type) {
        case CDS_BYTE:
            miss = &bval;
            sprintf(missstr, "%hhd", *(signed char *)miss);
            sprintf(fillstr, "%hhd", *(signed char *)fill);
            break;
        case CDS_CHAR:
            miss = &cval;
            sprintf(missstr, "%c", *(char *)miss);
            sprintf(fillstr, "%c", *(char *)fill);
            break;
        case CDS_SHORT:
            miss = &sval;
            sprintf(missstr, "%hd", *(short *)miss);
            sprintf(fillstr, "%hd", *(short *)fill);
            break;
        case CDS_INT:
            miss = &ival;
            sprintf(missstr, "%d", *(int *)miss);
            sprintf(fillstr, "%d", *(int *)fill);
            break;
        case CDS_FLOAT:
            miss = &fval;
            sprintf(missstr, "%f", *(float *)miss);
            sprintf(fillstr, "%f", *(float *)fill);
            break;
        case CDS_DOUBLE:
        default:
            miss = &dval;
            sprintf(missstr, "%f", *(double *)miss);
            sprintf(fillstr, "%f", *(double *)fill);
            break;
    }

    if (define_miss == 1) {
        if (!cds_define_att(var, "missing_value", type, 1, miss)) return(0);
    }
    else if (define_miss == 2) {
        if (!cds_change_att(grp, 1, "missing_value", type, 1, miss)) return(0);
    }
    else if (define_miss == 3) {
        if (!cds_change_att(grp, 1,
            "missing_value", CDS_CHAR, strlen(missstr), missstr)) {

            return(0);
        }
    }

    if (define_fill == 1) {
        if (!cds_define_att(var, "_FillValue", type, 1, fill)) return(0);
    }
    else if (define_fill == 2) {
        if (!cds_change_att(grp, 1, "_FillValue", type, 1, fill)) return(0);
    }
    else if (define_fill == 3) {

        if (!cds_change_att(grp, 1,
            "_FillValue", CDS_CHAR, strlen(fillstr), fillstr)) {

            return(0);
        }
    }

    return(1);
}

/*******************************************************************************
 *  File Compare
 */

int compare_files(char *file1, char *file2)
{
    char        *files[2];
    struct stat  stats[2];
    int          fds[2]      = { -1, -1 };
    char        *buffers[2]  = { NULL,  NULL };
    int          retval      = 1;
    int          i;

    files[0] = file1;
    files[1] = file2;

    /* Compare file sizes */

    for (i = 0; i < 2; i++) {

        if (stat(files[i], &stats[i]) < 0) {

            ERROR( gProgramName,
                "Could not stat file: %s\n -> %s\n",
                files[i], strerror(errno));

            return(0);
        }
    }

    if (stats[0].st_size != stats[1].st_size) {
        return(0);
    }

    /* Read in the files */

    for (i = 0; i < 2; i++) {

        buffers[i] = (char *)malloc(stats[i].st_size * sizeof(char));

        if (!buffers[i]) {

            ERROR( gProgramName,
                "Memory allocation error\n");

            retval = 0;
            break;
        }

        fds[i] = open(files[i], O_RDONLY);

        if (fds[i] < 0) {

            ERROR( gProgramName,
                "Could not open file: %s\n -> %s\n",
                files[i], strerror(errno));

            retval = 0;
            break;
        }

        if (read(fds[i], buffers[i], stats[i].st_size) != stats[i].st_size) {

            ERROR( gProgramName,
                "Could not read file: %s\n -> %s\n",
                files[i], strerror(errno));

            retval = 0;
            break;
        }
    }

    if (retval) {
        if (memcmp(buffers[0], buffers[1], stats[0].st_size) != 0) {
            retval = 0;
        }
    }

    for (i = 0; i < 2; i++) {
        if (buffers[i])  free(buffers[i]);
        if (fds[i] > -1) close(fds[i]);
    }

    return(retval);
}

/*******************************************************************************
 *  Logging Functions
 */

void log_array_values(
    const char *prefix, CDSDataType type, int nelems, void *array)
{
    switch (type) {
        case CDS_BYTE:   fprintf(gLog->fp, "byte   "); break;
        case CDS_CHAR:   fprintf(gLog->fp, "char   "); break;
        case CDS_SHORT:  fprintf(gLog->fp, "short  "); break;
        case CDS_INT:    fprintf(gLog->fp, "int    "); break;
        case CDS_FLOAT:  fprintf(gLog->fp, "float  "); break;
        case CDS_DOUBLE: fprintf(gLog->fp, "double "); break;

        default:
            break;
    }

    fprintf(gLog->fp, "%s", prefix);

    cds_print_array(
        gLog->fp, type, nelems, array, NULL, 0, 0, 0);

    fprintf(gLog->fp, "\n");
}

/*******************************************************************************
 *  Run Test Functions
 */

int check_run_test_log(const char *log_name, char *status_text)
{
    char out_file[PATH_MAX];
    char ref_file[PATH_MAX];

    sprintf(out_file, "out/%s", log_name);
    sprintf(ref_file, "%s/ref/%s", gTopTestDir, log_name);

    if (access(out_file, F_OK) != 0) {
        sprintf(status_text, "missing out file");
        return(0);
    }

    if (access(ref_file, F_OK) != 0) {
        sprintf(status_text, "missing ref file");
        return(0);
    }

    if (!compare_files(ref_file, out_file)) {
        sprintf(status_text, "FAIL");
        return(0);
    }

    return(1);
}

void close_run_test_log(void)
{
    msngr_finish_log();
    gLog   = (LogFile *)NULL;
    gLogFP = (FILE *)NULL;
}

int open_run_test_log(const char *log_name)
{
    char out_file[PATH_MAX];
    char errstr[MAX_LOG_ERROR];

    sprintf(out_file, "out/%s", log_name);
    unlink(out_file);

    if (!msngr_init_log("out", log_name, 0, MAX_LOG_ERROR, errstr)) {
        fprintf(stderr, errstr);
        return(0);
    }

    gLog   = msngr_get_log_file();
    gLogFP = gLog->fp;

    return(1);
}

int run_test(
    const char *test_name,
    const char *log_name,
    int (*test_func)(void))
{
    int  status;
    char status_text[32];
    int  ndots;

    if (log_name) {
        if (!open_run_test_log(log_name)) {
            return(0);
        }
    }

    status = test_func();

    if (!status) {
        sprintf(status_text, "FAIL");
    }

    if (log_name) {

        close_run_test_log();

        if (status) {
            status = check_run_test_log(log_name, status_text);
        }
    }

    ndots = 51 - strlen(test_name);

    fprintf(stdout, "%s", test_name);

    while (--ndots) {
        fprintf(stdout, ".");
    }

    if (status) {
        fprintf(stdout, "pass\n");
    }
    else {
        gFailCount += 1;
        fprintf(stdout, "%s\n", status_text);
    }

    return(status);
}

void skip_test(const char *test_name)
{
    int ndots;

    ndots = 51 - strlen(test_name);

    fprintf(stdout, " - %s", test_name);

    while (--ndots) {
        fprintf(stdout, ".");
    }

    fprintf(stdout, "skipped\n");
}

/*******************************************************************************
 *  Main
 */

int main(int argc, char **argv)
{
    gProgramName = argv[0];

    if (argc == 2) {
        gTopTestDir = argv[1];
    }
    else {
        gTopTestDir = ".";
    }

    gFailCount = 0;
/*
    msngr_set_debug_level(1);

    if (!msngr_init_mail(
        MSNGR_ERROR,
        NULL,
        "brian.ermold@pnl.gov",
        NULL,
        "libcds3 Error Tests",
        MAIL_ADD_NEWLINE,
        MAX_LOG_ERROR, errstr)) {

        fprintf(stderr, errstr);
    }
*/

    fprintf(stdout, "\nTesting build for libcds3 version: %s\n",
        cds_lib_version());

    libcds3_test_utils();
    libcds3_test_units();
    libcds3_test_defines();
    libcds3_test_att_values();
    libcds3_test_var_data();
    libcds3_test_time_data();
    libcds3_test_copy();
    libcds3_test_transform_params();

    cds_free_unit_system();
    cds_delete_group(gRoot);

    return(gFailCount);
}
