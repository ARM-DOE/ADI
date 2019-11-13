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
*    $Revision: 15432 $
*    $Author: ermold $
*    $Date: 2012-09-28 06:06:58 +0000 (Fri, 28 Sep 2012) $
*    $Version:$
*
*******************************************************************************/

#include "libcds3_test.h"

extern const char *gProgramName;
extern CDSGroup   *gRoot;
extern FILE       *gLogFP;

/*******************************************************************************
 *  Test Find Time Index Functions
 */

static int find_timeval_index_tests_1(void)
{
    timeval_t times[] = {
        {  5, 0 },
        {  5, 0 },
        {  5, 0 },
        {  6, 0 },
        {  7, 0 },
        {  8, 0 },
        {  9, 0 },
        { 10, 0 },
        { 10, 0 },
        { 10, 0 },
        { 11, 0 },
        { 12, 0 },
        { 13, 0 },
        { 14, 0 },
        { 15, 0 },
        { 15, 0 },
        { 15, 0 }
    };

    size_t    ntimes = sizeof(times)/sizeof(timeval_t);

    timeval_t ref_times[] = {
        {  3, 0},
        {  5, 0},
        {  7, 0},
        { 10, 0},
        { 12, 0},
        { 15, 0},
        { 17, 0} };

    size_t   ref_ntimes  = sizeof(ref_times)/sizeof(timeval_t);

    int      modes[]     = { CDS_LT, CDS_LTEQ, CDS_GT, CDS_GTEQ, CDS_EQ };
    size_t   nmodes      = sizeof(modes)/sizeof(int);

    const char *mode_str[] = {
        "CDS_LT", "CDS_LTEQ", "CDS_GT", "CDS_GTEQ", "CDS_EQ" };

    size_t ri, mi, ti;
    int    index;

    LOG( gProgramName,
        "\n============================================================\n"
        "Find Timeval Index Tests 1\n"
        "============================================================\n\n");

    for (ti = 0; ti < ntimes; ti++) {
        fprintf(gLogFP, "%ld:\t%ld.%.6ld\n",
            ti, times[ti].tv_sec, times[ti].tv_usec);
    }
    fprintf(gLogFP, "\n");

    for (ri = 0; ri < ref_ntimes; ri++) {
        for (mi = 0; mi < nmodes; mi++) {

            index = cds_find_timeval_index(
                ntimes, times, ref_times[ri], modes[mi]);

            if (index < 0) {
                fprintf(gLogFP, "%ld.%.6ld, %s:\t-1\n",
                    ref_times[ri].tv_sec, ref_times[ri].tv_usec, mode_str[mi]);
            }
            else {
                fprintf(gLogFP, "%ld.%.6ld, %s:\t%d, %ld.%.6ld\n",
                    ref_times[ri].tv_sec, ref_times[ri].tv_usec, mode_str[mi],
                    index, times[index].tv_sec, times[index].tv_usec);
            }
        }    

        fprintf(gLogFP, "\n");
    }

    return(1);
}

static int find_timeval_index_tests_2(void)
{
    timeval_t times[] = {
        {  5, 555555 },
        {  5, 555556 },
        {  5, 555557 },
        {  6, 666666 },
        {  7, 777777 },
        {  8, 888888 },
        {  9, 999999 },
        { 10, 111111 },
        { 10, 111112 },
        { 10, 111113 },
        { 11, 111111 },
        { 12, 222222 },
        { 13, 333333 },
        { 14, 444444 },
        { 15, 555555 },
        { 15, 555556 },
        { 15, 555557 }
    };

    size_t    ntimes = sizeof(times)/sizeof(timeval_t);

    timeval_t ref_times[] = {
        {  5, 555554},
        {  5, 555555},
        {  5, 555556},
        {  7, 000000},
        { 10, 111112},
        { 12, 222222},
        { 15, 555556},
        { 15, 555557},
        { 15, 555558} };

    size_t   ref_ntimes  = sizeof(ref_times)/sizeof(timeval_t);

    int      modes[]     = { CDS_LT, CDS_LTEQ, CDS_GT, CDS_GTEQ, CDS_EQ };
    size_t   nmodes      = sizeof(modes)/sizeof(int);

    const char *mode_str[] = {
        "CDS_LT", "CDS_LTEQ", "CDS_GT", "CDS_GTEQ", "CDS_EQ" };

    size_t ri, mi, ti;
    int    index;

    LOG( gProgramName,
        "\n============================================================\n"
        "Find Timeval Index Tests 2\n"
        "============================================================\n\n");

    for (ti = 0; ti < ntimes; ti++) {
        fprintf(gLogFP, "%ld:\t%ld.%.6ld\n",
            ti, times[ti].tv_sec, times[ti].tv_usec);
    }
    fprintf(gLogFP, "\n");

    for (ri = 0; ri < ref_ntimes; ri++) {
        for (mi = 0; mi < nmodes; mi++) {

            index = cds_find_timeval_index(
                ntimes, times, ref_times[ri], modes[mi]);

            if (index < 0) {
                fprintf(gLogFP, "%ld.%.6ld, %s:\t-1\n",
                    ref_times[ri].tv_sec, ref_times[ri].tv_usec, mode_str[mi]);
            }
            else {
                fprintf(gLogFP, "%ld.%.6ld, %s:\t%d, %ld.%.6ld\n",
                    ref_times[ri].tv_sec, ref_times[ri].tv_usec, mode_str[mi],
                    index, times[index].tv_sec, times[index].tv_usec);
            }
        }    

        fprintf(gLogFP, "\n");
    }

    return(1);
}

static int find_timeval_index_tests(void)
{
    find_timeval_index_tests_1();
    find_timeval_index_tests_2();
    return(1);
}

static int find_time_index_tests(void)
{
    time_t   times[] = {
        5, 5, 5, 6, 7, 8, 9, 10, 10, 10, 11, 12, 13, 14, 15, 15, 15 };
    size_t   ntimes = sizeof(times)/sizeof(time_t);

    time_t   ref_times[] = { 3, 5, 7, 10, 12, 15, 17 };
    size_t   ref_ntimes  = sizeof(ref_times)/sizeof(time_t);

    int      modes[]     = { CDS_LT, CDS_LTEQ, CDS_GT, CDS_GTEQ, CDS_EQ };
    size_t   nmodes      = sizeof(modes)/sizeof(int);

    const char *mode_str[] = {
        "CDS_LT", "CDS_LTEQ", "CDS_GT", "CDS_GTEQ", "CDS_EQ" };

    size_t ri, mi, ti;
    int    index;

    LOG( gProgramName,
        "\n============================================================\n"
        "Find Time Index Tests\n"
        "============================================================\n\n");

    for (ti = 0; ti < ntimes; ti++) {
        fprintf(gLogFP, "%ld:\t%ld\n", ti, times[ti]);
    }
    fprintf(gLogFP, "\n");

    for (ri = 0; ri < ref_ntimes; ri++) {
        for (mi = 0; mi < nmodes; mi++) {

            index = cds_find_time_index(
                ntimes, times, ref_times[ri], modes[mi]);

            if (index < 0) {
                fprintf(gLogFP, "%ld, %s:\t-1\n",
                    ref_times[ri], mode_str[mi]);
            }
            else {
                fprintf(gLogFP, "%ld, %s:\t%d, %ld\n",
                    ref_times[ri], mode_str[mi], index, times[index]);
            }
        }    

        fprintf(gLogFP, "\n");
    }

    return(1);
}

/*******************************************************************************
 *  Test Time Functions
 */

static void print_time_var(CDSVar *var)
{
    CDSGroup  *group = (CDSGroup *)var->parent;
    size_t     ntimes;
    timeval_t *tvs;
    size_t     i;

    cds_print_var(gLogFP, "    ", var, 0);
    fprintf(gLogFP, "\n");

    ntimes = 0;
    tvs    = cds_get_sample_timevals(group, 0, &ntimes, NULL);

    fprintf(gLogFP, "    timevals =\n\n");
    for (i = 0; i < ntimes; i++) {
        fprintf(gLogFP, "        %ld.%.6ld\n",
            tvs[i].tv_sec, tvs[i].tv_usec);
    }
    fprintf(gLogFP, "\n");

    free(tvs);
}

static int test_get_sample_times(CDSGroup *group_1, CDSVar *time_var)
{
    time_t      base_time[3];
    timeval_t  *sample_times[3];
    size_t      ntimes[3];
    size_t      i;

    /* Get base_time and sample times using subgroup group_1 */

    LOG( gProgramName,
        "Get base_time and sample times using time_var and group_1:\n\n");

    /* Get sample times using var_1_2 from subgroup group_1 */

    base_time[0] = cds_get_base_time(gRoot);
    if (!base_time[0]) {
        ERROR( gProgramName, "Could not get base_time using root group\n");
        return(0);
    }

    base_time[1] = cds_get_base_time(time_var);
    if (!base_time[1]) {
        ERROR( gProgramName, "Could not get base_time using time var\n");
        return(0);
    }

    base_time[2] = cds_get_base_time(group_1);
    if (!base_time[2]) {
        ERROR( gProgramName, "Could not get base_time using root/group_1\n");
        return(0);
    }

    if ((base_time[0] != base_time[1]) ||
        (base_time[0] != base_time[2]) ) {

        ERROR( gProgramName, "base_time values do not match!\n");
        return(0);
    }

    fprintf(gLogFP, "\nbase_time = %d\n", (int)base_time[0]);

    ntimes[0] = 0;
    sample_times[0] = cds_get_sample_timevals(gRoot, 0, &ntimes[0], NULL);
    if (ntimes[0] == (size_t)-1) {
        ERROR( gProgramName, "Could not get sample_times using root group\n");
        return(0);
    }

    ntimes[1] = 0;
    sample_times[1] = cds_get_sample_timevals(time_var, 0, &ntimes[1], NULL);
    if (ntimes[1] == (size_t)-1) {
        ERROR( gProgramName, "Could not get sample_times using time var\n");
        return(0);
    }

    ntimes[2] = 0;
    sample_times[2] = cds_get_sample_timevals(group_1, 0, &ntimes[2], NULL);
    if (ntimes[2] == (size_t)-1) {
        ERROR( gProgramName, "Could not get sample_times using root/group_1\n");
        return(0);
    }

    if ((ntimes[0] != ntimes[1]) ||
        (ntimes[0] != ntimes[2]) ) {

        ERROR( gProgramName, "number of sample times do not match!\n");
        return(0);
    }

    fprintf(gLogFP, "\nsample_times =\n\n");
    for (i = 0; i < ntimes[0]; i++) {

        if (TV_NEQ(sample_times[0][i],sample_times[1][i]) ||
            TV_NEQ(sample_times[0][i],sample_times[2][i])) {

            ERROR( gProgramName, "sample_times[%d] do not match!\n", i);
            return(0);
        }

        fprintf(gLogFP, "    %ld.%.6ld\n",
            sample_times[0][i].tv_sec,
            sample_times[0][i].tv_usec);
    }
    fprintf(gLogFP, "\n");

    for (i = 0; i < 3; i++) {
        free(sample_times[i]);
    }

    return(1);
}

static int test_get_time_range(CDSGroup *group_1, CDSVar *time_var)
{
    size_t      ntimes[3];
    timeval_t   start_time[3];
    timeval_t   end_time[3];

    /* Get time range using subgroup group_1 */

    LOG( gProgramName,
        "\nGet time range using time_var and group_1:\n\n");

    ntimes[0] = cds_get_time_range(gRoot, &start_time[0], &end_time[0]);
    if (ntimes[0] == 0) {
        ERROR( gProgramName, "Could not get time range using root group\n");
        return(0);
    }

    ntimes[1] = cds_get_time_range(time_var, &start_time[1], &end_time[1]);
    if (ntimes[1] == 0) {
        ERROR( gProgramName, "Could not get time range using time var\n");
        return(0);
    }

    ntimes[2] = cds_get_time_range(group_1, &start_time[2], &end_time[2]);
    if (ntimes[2] == 0) {
        ERROR( gProgramName, "Could not get time range using root/group_1\n");
        return(0);
    }

    if ((ntimes[0] != ntimes[1]) ||
        (ntimes[0] != ntimes[2]) ) {

        ERROR( gProgramName, "number of sample times do not match!\n");
        return(0);
    }

    if (TV_NEQ(start_time[0],start_time[1]) ||
        TV_NEQ(start_time[0],start_time[2])) {

        ERROR( gProgramName, "start_times do not match!\n");
        return(0);
    }

    if (TV_NEQ(end_time[0],end_time[1]) ||
        TV_NEQ(end_time[0],end_time[2])) {

        ERROR( gProgramName, "end_times do not match!\n");
        return(0);
    }

    fprintf(gLogFP,
        "\n"
        "ntimes:     %ld\n"
        "start time: %ld.%.6ld\n"
        "end time:   %ld.%.6ld\n\n",
        ntimes[0],
        start_time[0].tv_sec, start_time[0].tv_usec,
        end_time[0].tv_sec,   end_time[0].tv_usec);

    return(1);
}

static int test_set_sample_times(CDSGroup *group)
{
    timeval_t new_times[] = {

        { 1266019200, 50000 },
        { 1266019200, 60000 },
        { 1266019200, 70000 },
        { 1266019200, 80000 },
        { 1266019200, 90000 },
        { 1266019201, 00000 },
        { 1266019201, 10000 },
        { 1266019201, 20000 },
        { 1266019201, 30000 },
        { 1266019201, 40000 }
    };

    LOG( gProgramName,
        "Setting new sample times using: %s\n\n",
        cds_get_object_path(group));

    if (!cds_set_base_time(group, NULL, 1266019200)) {
        return(0);
    }

    if (!cds_set_sample_timevals(group, 0, 10, new_times)) {
        return(0);
    }

    return(1);
}

static int time_data_tests(void)
{
    CDSVar     *time_var;
    CDSVar     *base_time_var;
    CDSVar     *time_offset_var;
    CDSAtt     *units_att;
    time_t      old_base_time;
    char       *old_units_string;
    time_t      new_base_time;
    char        new_units_string[64];
    const char *time_dim;
    CDSGroup   *group_1;

    LOG( gProgramName,
        "\n============================================================\n"
        "Time Tests\n"
        "============================================================\n\n");

    /* Add base_time and time_offset variables */

    time_dim = "time";

    base_time_var = cds_define_var(gRoot, "base_time", CDS_INT, 1, &time_dim);
    if (!base_time_var) {
        ERROR( gProgramName, "Could not create base_time variable\n");
        return(0);
    }

    time_offset_var = cds_define_var(gRoot, "time_offset", CDS_DOUBLE, 1, &time_dim);
    if (!time_offset_var) {
        ERROR( gProgramName, "Could not create time_offset variable\n");
        return(0);
    }

    /* Get pointer to time variable */

    time_var = cds_find_time_var(gRoot);
    if (!time_var) {
        ERROR( gProgramName, "Could not find time variable\n");
        return(0);
    }

    /* Get pointer to time variable's units attribute */

    units_att = cds_get_att(time_var, "units");
    if (!units_att) {
        ERROR( gProgramName, "Could not find time.units attribute\n");
        return(0);
    }

    /* Get current base time from units string */

    old_units_string = units_att->value.cp;

    if (!cds_units_string_to_base_time(old_units_string, &old_base_time)) {
        return(0);
    }

    LOG( gProgramName, "Time variables before base time change:\n\n");

    cds_print_var(gLogFP, "    ", base_time_var, 0);
    fprintf(gLogFP, "\n");
    cds_print_var(gLogFP, "    ", time_offset_var, 0);
    fprintf(gLogFP, "\n");
    print_time_var(time_var);

    /* Change base time to 2009-02-13 23:31:30 */

    new_base_time = 1234567890;

    if (!cds_base_time_to_units_string(new_base_time, new_units_string)) {
        return(0);
    }

    LOG( gProgramName,
        "\nChanging base time value:\n"
        "  - from: %d = '%s'\n"
        "  - to:   %d = '%s'\n\n",
        old_base_time, old_units_string,
        new_base_time, new_units_string);

    if (!cds_set_base_time(time_var, NULL, new_base_time)) {
        return(0);
    }

    LOG( gProgramName, "Time variables after base time change:\n\n");

    cds_print_var(gLogFP, "    ", base_time_var, 0);
    fprintf(gLogFP, "\n");
    cds_print_var(gLogFP, "    ", time_offset_var, 0);
    fprintf(gLogFP, "\n");
    print_time_var(time_var);

    /* Get base_time and sample times using subgroup group_1 */

    group_1 = cds_get_group(gRoot, "group_1");
    if (!group_1) {
        ERROR( gProgramName, "Could not find group_1\n");
        return(0);
    }

    if (!test_get_time_range(group_1, time_var)) {
        return(0);
    }

    if (!test_get_sample_times(group_1, time_var)) {
        return(0);
    }

    /* Set new sample times */

    if (!test_set_sample_times(group_1)) {
        return(0);
    }

    LOG( gProgramName, "Time variables after set new sample times:\n\n");

    cds_print_var(gLogFP, "    ", base_time_var, 0);
    fprintf(gLogFP, "\n");
    cds_print_var(gLogFP, "    ", time_offset_var, 0);
    fprintf(gLogFP, "\n");
    print_time_var(time_var);

    /* Re-run get sample times test with new values */

    if (!test_get_time_range(group_1, time_var)) {
        return(0);
    }

    if (!test_get_sample_times(group_1, time_var)) {
        return(0);
    }

    return(1);
}

/*******************************************************************************
 *  Test: CDS Utility Functions
 */

void libcds3_test_time_data(void)
{
    fprintf(stdout, "\nTime Data Tests:\n");

    run_test(" - find_time_index_tests",
        "find_time_index_tests", find_time_index_tests);

    run_test(" - find_timeval_index_tests",
        "find_timeval_index_tests", find_timeval_index_tests);

    run_test(" - time_data_tests",
        "time_data_tests", time_data_tests);
}
