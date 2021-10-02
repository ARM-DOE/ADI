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

/** @file libncds3_test.c
 *  Test file for libncds.
 */

#include "ncds3.h"
#include "libncds3_test.h"

int create_nc4_test_file(GroupDef *root_def)
{
    char     *file  = "out.netcdf4.nc";
    int       cmode = NC_CLOBBER | NC_NETCDF4;
    CDSGroup *group;
    int       ncid;

    /* Create the CDS */

    group = cds_factory_define_groups(NULL, root_def);
    if (!group) {
        return(0);
    }

    /* Create the output file */

    ncid = ncds_create_file(group, file, cmode, 1, 0);

    /* Cleanup and return */

    if (ncid) {
        nc_close(ncid);
        cds_delete_group(group);
        return(1);
    }

    return(0);
}

int strip_time_var(
    const char *in_file,
    const char *out_file)
{
    CDSGroup *group;
    CDSVar   *var;
    int       in_format;
    int       cmode;
    int       ncid;
    int       recursive;

    /* Read the input file */

    group = ncds_read_file(in_file, 1, 0, &in_format, NULL);
    if (!group) {
        return(0);
    }

    /* Set the default output cmode */

    switch (in_format) {
        case NC_FORMAT_CLASSIC:
            cmode = 0;
            break;
        case NC_FORMAT_64BIT:
            cmode = NC_64BIT_OFFSET;
            break;
        case NC_FORMAT_NETCDF4:
            cmode = NC_NETCDF4;
            break;
        case NC_FORMAT_NETCDF4_CLASSIC:
            cmode = NC_NETCDF4 | NC_CLASSIC_MODEL;
            break;
        default:
            cmode = NC_NETCDF4;
    }

    var = cds_get_var(group, "time");
    if (var) {
        cds_delete_var(var);
    }

    /* Create the ouput file */

    if ((cmode & NC_NETCDF4) && !(cmode & NC_CLASSIC_MODEL)) {
        recursive = 1;
    }
    else {
        recursive = 0;
    }

    ncid = ncds_create_file(group, out_file, cmode, recursive, 0);

    /* Cleanup and return */

    cds_delete_group(group);

    if (ncid) {
        nc_close(ncid);
        return(1);
    }

    return(0);
}

int bounds_var_test_1()
{
    CDSGroup *group;
    CDSVar   *var;
    int       in_ncid;
    int       out_ncid;

    /* Open the input file */

    if (!ncds_open("ceil.nc", 0, &in_ncid)) {
        return(0);
    }

    /* Create the CDS group */

    group = cds_define_group(NULL, "bounds_var_test_1");
    if (!group) return(0);

    /* Get variables */

    var = ncds_get_var(
            in_ncid, "backscatter", 0, NULL,
            group, "backscatter", CDS_NAT, NULL, 0,
            0, NULL, NULL, NULL, NULL);

    if (!var) return(0);

    cds_delete_var(var);

    ncds_close(in_ncid);

    /* Create the output file */

    out_ncid = ncds_create_file(group, "out.bounds_var_test_1.nc", 0, 0, 0);

    /* Cleanup and return */

    cds_delete_group(group);

    if (out_ncid) {
        nc_close(out_ncid);
        return(1);
    }

    return(0);
}

int bounds_var_test_2()
{
    CDSGroup *group;
    CDSVar   *var;
    int       in_ncid;
    int       out_ncid;

    const char *nc_dim_names[]  = { "range" };
    const char *cds_dim_names[] = { "height" };
    const char *cds_dim_units[] = { "km" };

    /* Open the input file */

    if (!ncds_open("ceil.nc", 0, &in_ncid)) {
        return(0);
    }

    /* Create the CDS group */

    group = cds_define_group(NULL, "bounds_var_test_2");
    if (!group) return(0);

    /* Get variables */

    var = ncds_get_var(
            in_ncid, "backscatter", 0, NULL,
            group, "backscatter", CDS_NAT, NULL, 0,
            1, nc_dim_names, cds_dim_names, NULL, cds_dim_units);

    if (!var) return(0);

    cds_delete_var(var);

    ncds_close(in_ncid);

    /* Create the output file */

    out_ncid = ncds_create_file(group, "out.bounds_var_test_2.nc", 0, 0, 0);

    /* Cleanup and return */

    cds_delete_group(group);

    if (out_ncid) {
        nc_close(out_ncid);
        return(1);
    }

    return(0);
}

int bounds_var_test_3()
{
    CDSGroup *group;
    CDSVar   *var;
    int       in_ncid;
    int       out_ncid;

    const char *nc_dim_names[]  = { "range" };
    const char *cds_dim_names[] = { "height" };
    const char *cds_dim_units[] = { "m" };

    /* Open the input file */

    if (!ncds_open("ceil.nc", 0, &in_ncid)) {
        return(0);
    }

    /* Create the CDS group */

    group = cds_define_group(NULL, "bounds_var_test_3");
    if (!group) return(0);

    /* Get variables */

    var = ncds_get_var(
            in_ncid, "first_cbh", 0, NULL,
            group, NULL, CDS_NAT, NULL, 0,
            1, nc_dim_names, cds_dim_names, NULL, cds_dim_units);

    if (!var) return(0);

    var = ncds_get_var(
            in_ncid, "range", 0, NULL,
            group, "height", CDS_NAT, "km", 0,
            1, nc_dim_names, cds_dim_names, NULL, cds_dim_units);

    if (!var) return(0);

    var = ncds_get_var(
            in_ncid, "backscatter", 0, NULL,
            group, "backscatter", CDS_NAT, NULL, 0,
            1, nc_dim_names, cds_dim_names, NULL, cds_dim_units);

    if (!var) return(0);

    cds_delete_var(var);

    ncds_close(in_ncid);

    /* Create the output file */

    out_ncid = ncds_create_file(group, "out.bounds_var_test_3.nc", 0, 0, 0);

    /* Cleanup and return */

    cds_delete_group(group);

    if (out_ncid) {
        nc_close(out_ncid);
        return(1);
    }

    return(0);
}

int bounds_var_test_4()
{
    CDSGroup *group;
    CDSVar   *var;
    int       in_ncid;
    int       out_ncid;

    const char *nc_dim_names[]  = { "range" };
    const char *cds_dim_names[] = { "height" };
    const char *cds_dim_units[] = { "km" };

    /* Open the input file */

    if (!ncds_open("ceil.nc", 0, &in_ncid)) {
        return(0);
    }

    /* Create the CDS group */

    group = cds_define_group(NULL, "bounds_var_test_4");
    if (!group) return(0);

    /* Get variables */

    var = ncds_get_var(
            in_ncid, "first_cbh", 0, NULL,
            group, NULL, CDS_NAT, NULL, 0,
            1, nc_dim_names, cds_dim_names, NULL, cds_dim_units);

    if (!var) return(0);

    var = ncds_get_var(
            in_ncid, "range_bounds", 0, NULL,
            group, "height_bounds", CDS_NAT, "m", 0,
            1, nc_dim_names, cds_dim_names, NULL, cds_dim_units);

    if (!var) return(0);

    var = ncds_get_var(
            in_ncid, "backscatter", 0, NULL,
            group, "backscatter", CDS_NAT, NULL, 0,
            1, nc_dim_names, cds_dim_names, NULL, cds_dim_units);

    if (!var) return(0);

    cds_delete_var(var);

    ncds_close(in_ncid);

    /* Create the output file */

    out_ncid = ncds_create_file(group, "out.bounds_var_test_4.nc", 0, 0, 0);

    /* Cleanup and return */

    cds_delete_group(group);

    if (out_ncid) {
        nc_close(out_ncid);
        return(1);
    }

    return(0);
}

int bounds_var_tests()
{
    if (!bounds_var_test_1()) return(0);
    if (!bounds_var_test_2()) return(0);
    if (!bounds_var_test_3()) return(0);
    if (!bounds_var_test_4()) return(0);

    return(1);
}

int special_attribute_tests()
{
    CDSGroup *group;
    CDSVar   *var;
    int       nc_format;
    int       out_ncid;
    int       deflate_level;

    /* Read in the input file */

    group = ncds_read_file("ceil.nc", 0, 0, &nc_format, NULL);
    if (!group) return(0);

    /* Set the global _Format attribute */

    if (!cds_define_att_text(group,
        "_Format", "%s", "netCDF-4 classic model")) {

        return(0);
    }

    /* Get the backscatter variable */

    var = cds_get_var(group, "backscatter");
    if (!var) return(0);

    /* Set the _DeflateLevel attribute */

    deflate_level = 5;
    if (!cds_define_att(var,
        "_DeflateLevel", CDS_INT, 1, &deflate_level)) {

        return(0);
    }

    /* Create the output file */

    out_ncid = ncds_create_file(
        group, "out.special_attribute_tests.nc", 0, 0, 0);

    /* Cleanup and return */

    cds_delete_group(group);

    if (out_ncid) {
        nc_close(out_ncid);
        return(1);
    }

    return(0);
}

int unit_conversion_test(int test_num)
{
    CDSGroup *group;
    CDSVar   *var;
    int       in_ncid;
    int       out_ncid;

    const char *in_file;
    const char *out_file;
    const char *grp_name;

    switch (test_num) {
        case 1:
            in_file  = "ceil.nc";
            out_file = "out.unit_conversion_test_1.nc";
            grp_name = "unit_conversion_test_1";
            break;
        case 2:
            in_file  = "ceil_global_missing_att.nc";
            out_file = "out.unit_conversion_test_2.nc";
            grp_name = "unit_conversion_test_2";
            break;
        case 3:
            in_file  = "ceil_global_missing_att.nc";
            out_file = "out.unit_conversion_test_3.nc";
            grp_name = "unit_conversion_test_3";
            break;
    }

    /* Open the input file */

    if (!ncds_open(in_file, 0, &in_ncid)) {
        return(0);
    }

    /* Create the CDS group */

    group = cds_define_group(NULL, grp_name);
    if (!group) {
        return(0);
    }

    /* Read in global attributes if test number is 3 */

    if (test_num == 3) {
        if (ncds_read_atts(in_ncid, group) < 0) {
            return(0);
        }
    }

    /* Get variables */

    var = ncds_get_var(
            in_ncid, "first_cbh", 0, NULL,
            group, "first_cbh_float_m", CDS_FLOAT, "m", 0,
            0, NULL, NULL, NULL, NULL);

    if (!var) {
        return(0);
    }

    var = ncds_get_var(
            in_ncid, "first_cbh", 0, NULL,
            group, "first_cbh_float_ft", CDS_FLOAT, "ft", 0,
            0, NULL, NULL, NULL, NULL);

    if (!var) {
        return(0);
    }

    var = ncds_get_var(
            in_ncid, "first_cbh", 0, NULL,
            group, "first_cbh_int_m", CDS_INT, "m", 0,
            0, NULL, NULL, NULL, NULL);

    if (!var) {
        return(0);
    }

    var = ncds_get_var(
            in_ncid, "first_cbh", 0, NULL,
            group, "first_cbh_int_ft", CDS_INT, "ft", 0,
            0, NULL, NULL, NULL, NULL);

    if (!var) {
        return(0);
    }

    ncds_close(in_ncid);

    /* Create the output file */

    out_ncid = ncds_create_file(group, out_file, 0, 0, 0);

    /* Cleanup and return */

    cds_delete_group(group);

    if (out_ncid) {
        nc_close(out_ncid);
        return(1);
    }

    return(0);
}

int unit_conversion_tests()
{
    if (!unit_conversion_test(1)) return(0);
    if (!unit_conversion_test(2)) return(0);
    if (!unit_conversion_test(3)) return(0);
    return(1);
}

int nc_dump(const char *file, int header_only)
{
    CDSGroup *group;
    int       nc_format;

    /* Read the NetCDF file */

    group = ncds_read_file(file, 1, header_only, &nc_format, NULL);
    if (!group) {
        return(0);
    }

    /* Print the NetCDF file format */

    printf("\nNetCDF Format: ");

    switch (nc_format) {
        case NC_FORMAT_CLASSIC:
            printf("CLASSIC\n\n");
            break;
        case NC_FORMAT_64BIT:
            printf("64BIT_OFFSET\n\n");
            break;
        case NC_FORMAT_NETCDF4:
            printf("NETCDF4\n\n");
            break;
        case NC_FORMAT_NETCDF4_CLASSIC:
            printf("NETCDF4_CLASSIC\n\n");
            break;
        default:
            printf("%d\n\n", nc_format);
            break;
    }

    /* Print the CDS */

    cds_print(stdout, group, header_only);

    /* Cleanup and return */

    cds_delete_group(group);

    return(1);
}

int nc_copy(
    const char *in_file,
    const char *out_file,
    const char *out_format,
    int         header_only)
{
    CDSGroup *group;
    int       in_format;
    int       cmode;
    int       ncid;
    int       recursive;

    /* Read the input file */

    group = ncds_read_file(in_file, 1, header_only, &in_format, NULL);
    if (!group) {
        return(0);
    }

    /* Set the default output cmode */

    switch (in_format) {
        case NC_FORMAT_CLASSIC:
            cmode = 0;
            break;
        case NC_FORMAT_64BIT:
            cmode = NC_64BIT_OFFSET;
            break;
        case NC_FORMAT_NETCDF4:
            cmode = NC_NETCDF4;
            break;
        case NC_FORMAT_NETCDF4_CLASSIC:
            cmode = NC_NETCDF4 | NC_CLASSIC_MODEL;
            break;
        default:
            cmode = NC_NETCDF4;
    }

    /* Update cmode if an output format was specified */

    if (out_format) {

        if (strstr(out_format, "NETCDF4")) {
            if (strstr(out_format, "CLASSIC")) {
                cmode = NC_NETCDF4 | NC_CLASSIC_MODEL;
            }
            else {
                cmode = NC_NETCDF4;
            }
        }
        else if (strstr(out_format, "CLASSIC")) {
            cmode = 0;
        }

        if (strstr(out_format, "NOCLOBBER")) {
            cmode |= NC_NOCLOBBER;
        }
    }

    /* Create the ouput file */

    if ((cmode & NC_NETCDF4) && !(cmode & NC_CLASSIC_MODEL)) {
        recursive = 1;
    }
    else {
        recursive = 0;
    }

    ncid = ncds_create_file(group, out_file, cmode, recursive, header_only);

    /* Cleanup and return */

    cds_delete_group(group);

    if (ncid) {
        nc_close(ncid);
        return(1);
    }

    return(0);
}

int nc_info(const char *file)
{
    int ncid;
//    int format;

    /* Open the file */

    printf("NetCDF File: %s\n", file);

    if (!ncds_open(file, 0, &ncid)) {
        fprintf(stderr, "Could not open file!\n");
        return(0);
    }

    /* Print the NetCDF file format */
/*
    if (!ncds_format(ncid, &format)) return(0);

    switch (format) {
        case NC_FORMAT_CLASSIC:
            printf("%d = NC_CLASSIC\n", format);
            break;
        case NC_FORMAT_64BIT:
            printf("%d = NC_64BIT\n", format);
            break;
        case NC_FORMAT_NETCDF4:
            printf("%d = NC_NETCDF4\n", format);
            break;
        case NC_FORMAT_NETCDF4_CLASSIC:
            printf("%d = NC_NETCDF4_CLASSIC\n", format);
            break;
        default:
            printf("%d = NETCDF4\n", format);
    }
*/
    if (!ncds_close(ncid)) {
        fprintf(stderr, "Could not close file!\n");
        return(0);
    }

    return(1);
}

int nc_subset(
    const char  *in_file,
    const char  *out_file,
    const char  *out_format,
    size_t       start_sample,
    size_t       sample_count,
    int          nvars,
    const char **var_list)
{
    CDSGroup *group;
    CDSVar   *var;
    int       in_format;
    int       cmode;
    int       ncid;
    int       vi;

    /* Open the input file and get the file format */

    if (!ncds_open(in_file, 0, &ncid)) {
        return(0);
    }

    if (!ncds_format(ncid, &in_format)) {
        return(0);
    }

    /* Create the CDS group */

    group = cds_define_group(NULL, out_file);
    if (!group) {
        return(0);
    }

    /* Read in all global attributes */

    if (!ncds_read_atts(ncid, group)) {
        return(0);
    }

    /* Read in all variables in the list */

    for (vi = 0; vi < nvars; vi++) {

        var = ncds_get_var(
            ncid, var_list[vi], start_sample, &sample_count,
            group, NULL, 0, NULL, 0,
            0, NULL, NULL, NULL, NULL);

        if (var == (CDSVar *)NULL) {
            printf("\nVariable not found: %s\n", var_list[vi]);
        }
        else if (var == (CDSVar *)-1) {
            return(0);
        }
    }

    ncds_close(ncid);

    /* Set the default output cmode */

    switch (in_format) {
        case NC_FORMAT_CLASSIC:
            cmode = 0;
            break;
        case NC_FORMAT_64BIT:
            cmode = NC_64BIT_OFFSET;
            break;
        case NC_FORMAT_NETCDF4:
            cmode = NC_NETCDF4;
            break;
        case NC_FORMAT_NETCDF4_CLASSIC:
            cmode = NC_NETCDF4 | NC_CLASSIC_MODEL;
            break;
        default:
            cmode = NC_NETCDF4;
    }

    /* Update cmode if an output format was specified */

    if (out_format) {

        if (strstr(out_format, "NETCDF4")) {
            if (strstr(out_format, "CLASSIC")) {
                cmode = NC_NETCDF4 | NC_CLASSIC_MODEL;
            }
            else {
                cmode = NC_NETCDF4;
            }
        }
        else if (strstr(out_format, "CLASSIC")) {
            cmode = 0;
        }

        if (strstr(out_format, "NOCLOBBER")) {
            cmode |= NC_NOCLOBBER;
        }
    }

    /* Create the ouput file */

    ncid = ncds_create_file(group, out_file, cmode, 0, 0);

    /* Cleanup and return */

    cds_delete_group(group);

    if (ncid) {
        nc_close(ncid);
        return(1);
    }

    return(0);
}

int nc_get_fill_value(
    const char *file,
    const char *var_name)
{
    int       ncid;
    int         status;
    int         varid;
//    nc_type     type;
//    size_t      length;
    float       fval;

    /* Open the netcdf file */

    if (!ncds_open(file, 0, &ncid)) {
        return(0);
    }

    status = ncds_inq_varid(ncid, var_name, &varid);
    if (status < 0) {
        ncds_close(ncid);
        return(0);
    }
    else if (status == 0) {
        printf("\nVariable not found\n\n");
        ncds_close(ncid);
        return(0);
    }

    status = nc_get_att_float(ncid, varid, "_FillValue", &fval);
    if (status != NC_NOERR) {
        printf("\nCould not get _FillValue\n -> %s\n\n", nc_strerror(status));
        ncds_close(ncid);
        return(0);
    }

    printf("_FillValue = %f\n", fval);

    ncds_close(ncid);

    return(1);
}

/*******************************************************************************
 *  Usage
 */

void exit_usage(const char *program_name)
{

fprintf(stdout,
"USAGE:\n"
"\n"
"    %s create_nc4_test_file\n"
"    %s nc_dump   [-h] in_file\n"
"    %s nc_copy   [-f format] [-h] in_file out_file\n"
"    %s nc_subset [-f format] [-s start] [-c count] in_file out_file var_name(s)\n"
"\n"
"    -f format => output file format, this can be any combination of:\n"
"\n"
"                 NOCLOBBER = do not overwrite existing files\n"
"                 CLASSIC   = classic NetCDF file (ignores subgroups)\n"
"                 NETCDF4   = HDF5/NetCDF-4 file\n"
"\n"
"    -h        => header only\n"
"    -v        => display libncds3 version\n"
"\n",
program_name, program_name, program_name, program_name);

    exit(1);
}

/*******************************************************************************
 *  Main
 */

int main(int argc, char **argv)
{
    const char *program_name = argv[0];
    const char *command      = NULL;
    const char *in_file      = NULL;
    const char *out_file     = NULL;
    const char *out_format   = NULL;
    int         header_only  = 0;
    int         nvars        = 0;
    size_t      start_sample = 0;
    size_t      sample_count = 0;
    const char *var_list[NC_MAX_VARS];
    int         status;

    char *switches;
    char  c;

    while(--argc > 0) {
        if ((*++argv)[0] == '-') {
            switches = *argv;
            while ((c = *++switches)) {
                switch(c) {
                case 'c':
                    sample_count = atoi(*++argv);
                    argc--;
                    break;
                case 'f':
                    out_format = *++argv;
                    argc--;
                    break;
                case 'h':
                    header_only = 1;
                    break;
                case 's':
                    start_sample = atoi(*++argv);
                    argc--;
                    break;
                case 'v':
                    printf("\nLIBNCDS Version: %s\n\n", ncds_lib_version());
                    exit(0);
                default:
                    exit_usage(program_name);
                }
            }
        }
        else if (!command) {
            command = *argv;
        }
        else if (!in_file) {
            in_file = *argv;
        }
        else if (!out_file) {
            out_file = *argv;
        }
        else if (strcmp(command, "nc_subset") == 0) {
            var_list[nvars++] = *argv;
        }
        else {
            exit_usage(program_name);
        }
    }

    if (!command) {
        exit_usage(program_name);
    }
    else if (strcmp(command, "strip_time_var") == 0) {
        status = strip_time_var(in_file, out_file);
    }
    else if (strcmp(command, "create_nc4_test_file") == 0) {
        status = create_nc4_test_file(RootDef);
    }
    else if (strcmp(command, "special_attribute_tests") == 0) {
        status = special_attribute_tests();
    }
    else if (strcmp(command, "unit_conversion_tests") == 0) {
        status = unit_conversion_tests();
    }
    else if (strcmp(command, "bounds_var_tests") == 0) {
        status = bounds_var_tests();
    }
    else if (!in_file) {
        exit_usage(program_name);
    }
    else if (strcmp(command, "nc_info") == 0) {
        status = nc_info(in_file);
    }
    else if (strcmp(command, "nc_get_fill_value") == 0) {
        status = nc_get_fill_value(in_file, var_list[0]);
    }
    else if (strcmp(command, "nc_dump") == 0) {
        status = nc_dump(in_file, header_only);
    }
    else if (!out_file) {
        exit_usage(program_name);
    }
    else if (strcmp(command, "nc_copy") == 0) {
        status = nc_copy(in_file, out_file, out_format, header_only);
    }
    else if (strcmp(command, "nc_subset") == 0) {
        status = nc_subset(
            in_file, out_file, out_format,
            start_sample, sample_count, nvars, var_list);
    }

    return( (status) ? 0 : 1 );
}
