/*******************************************************************************
*
*  COPYRIGHT (C) 2013 Battelle Memorial Institute.  All Rights Reserved.
*
********************************************************************************
*
*  Authors:
*     name:  Brian Ermold
*     phone: (509) 375-2277
*     email: brian.ermold@pnl.gov
*
********************************************************************************
*
*  REPOSITORY INFORMATION:
*    $Revision: 67119 $
*    $Author: ermold $
*    $Date: 2016-01-27 19:22:37 +0000 (Wed, 27 Jan 2016) $
*    $State:$
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file dsproc_csv2cds.c
 *  CSV to CDS Mapping Functions.
 */

#include "dsproc3.h"

/*******************************************************************************
 *  Private Data and Functions
 */
/** @privatesection */

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Add a string to double conversion function to a CSV2CDS Mapping structure.
 *
 *  The str_to_double function must return a double and set the status value
 *  to non-zero if successful or 0 for an invalid input string.
 *
 *  @param  map         pointer to the CSV2CDS Map structure
 *  @param  csv_name    name of the column in the csv file
 *  @param  str_to_dbl  function used to convert csv string to a double.
 *
 *  @retval   1  if the specified csv_name was found
 *  @retval   0  if the specified csv_name was not found
 */
int dsproc_add_csv_str_to_dbl_function(
    CSV2CDSMap *map,
    const char *csv_name,
    double    (*str_to_dbl)(const char *strval, int *status))
{
    int mi;

    for (mi = 0; map[mi].csv_name; ++mi) {
        if (strcmp(csv_name, map[mi].csv_name) == 0) {
            map[mi].str_to_dbl = str_to_dbl;
            return(1);
        }
    }

    return(0);
}

/**
 *  Map CSVParser data to variables in a CDSGroup.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param   csv        pointer to the CSVParser structure
 *  @param   csv_start  index of the start record in the CSVParser structure
 *  @param   csv_count  number of records to map (0 for all)
 *  @param   map        pointer to the CSV2CDS Map structure
 *  @param   cds        pointer to the CDSGroup structure
 *  @param   cds_start  index of the start record in the CDSGroup
 *  @param   flags      control flags
 *
 *  @retval   1  if successful
 *  @retval   0  if an error occurred
 */
int dsproc_map_csv_to_cds(
    CSVParser  *csv,
    int         csv_start,
    int         csv_count,
    CSV2CDSMap *map,
    CDSGroup   *cds,
    int         cds_start,
    int         flags)
{
    int *indexes;
    int  status;
    int  ri, idx;

    if (csv_count <= 0 ||
        csv_count > csv->nrecs - csv_start) {

        csv_count = csv->nrecs - csv_start;
    }

    indexes = (int *)calloc(csv_count, sizeof(int));
    if (!indexes) {

        ERROR( DSPROC_LIB_NAME,
            "Memory allocation error creating CSV to CDS mapping index\n");

        dsproc_set_status(DSPROC_ENOMEM);

        return(0);
    }

    for (ri = 0, idx = csv_start; ri < csv_count; ++ri, ++idx) {
        indexes[ri] = idx;
    }

    status = dsproc_map_csv_to_cds_by_index(
        csv, indexes, csv_count, map, cds, cds_start, flags);

    free(indexes);

    return(status);
}

/**
 *  Macro used by dsproc_map_csv_to_cds_by_index.
 */
#define CSV_MAP_TO_CDS(data_t, data_p, miss_p, ato_func) \
if (csv_str_map) { \
    for (ri = 0; ri < csv_count; ++ri) { \
        csvi = csv_indexes[ri]; \
        is_missing = 0; \
        if (!csv_strvals[csvi] || *csv_strvals[csvi] == '\0') { \
            is_missing = 1; \
        } \
        else if (csv_missings) { \
            for (mvi = 0; csv_missings[mvi]; ++mvi) { \
                if (strcmp(csv_strvals[csvi], csv_missings[mvi]) == 0) { \
                    is_missing = 1; \
                    break; \
                } \
            } \
        } \
        if (is_missing) { \
            *data_p++ = *miss_p; \
        } \
        else { \
            for (smi = 0; csv_str_map[smi].strval; ++smi) { \
                if (strcasecmp(csv_strvals[csvi], csv_str_map[smi].strval) == 0) { \
                    *data_p++ = (data_t)csv_str_map[smi].dblval; \
                    break; \
                } \
            } \
            if (!csv_str_map[smi].strval) { \
                ERROR( DSPROC_LIB_NAME, \
                    "Invalid '%s' value '%s' in file: %s\n", \
                    csv_name, csv_strvals[csvi], csv->file_name); \
                    dsproc_set_status(DSPROC_ECSV2CDS); \
                return(0); \
            } \
        } \
    } \
} \
else if (csv_str_to_dbl) { \
    for (ri = 0; ri < csv_count; ++ri) { \
        csvi = csv_indexes[ri]; \
        is_missing = 0; \
        if (!csv_strvals[csvi] || *csv_strvals[csvi] == '\0') { \
            is_missing = 1; \
        } \
        else if (csv_missings) { \
            for (mvi = 0; csv_missings[mvi]; ++mvi) { \
                if (strcmp(csv_strvals[csvi], csv_missings[mvi]) == 0) { \
                    is_missing = 1; \
                    break; \
                } \
            } \
        } \
        if (is_missing) { \
            *data_p++ = *miss_p; \
        } \
        else { \
            *data_p++ = (data_t)csv_str_to_dbl(csv_strvals[csvi], &status); \
            if (!status) { \
                ERROR( DSPROC_LIB_NAME, \
                    "Invalid '%s' value '%s' in file: %s\n", \
                    csv_name, csv_strvals[csvi], csv->file_name); \
                    dsproc_set_status(DSPROC_ECSV2CDS); \
                return(0); \
            } \
        } \
    } \
} \
else { \
    for (ri = 0; ri < csv_count; ++ri) { \
        csvi = csv_indexes[ri]; \
        is_missing = 0; \
        if (!csv_strvals[csvi] || *csv_strvals[csvi] == '\0') { \
            is_missing = 1; \
        } \
        else if (csv_missings) { \
            for (mvi = 0; csv_missings[mvi]; ++mvi) { \
                if (strcmp(csv_strvals[csvi], csv_missings[mvi]) == 0) { \
                    is_missing = 1; \
                    break; \
                } \
            } \
        } \
        if (is_missing) { \
            *data_p++ = *miss_p; \
        } \
        else { \
            *data_p++ = (data_t)ato_func(csv_strvals[csvi]); \
        } \
    } \
}

/**
 *  Map CSVParser data to variables in a CDSGroup using CSV record indexes.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param   csv          pointer to the CSVParser structure
 *  @param   csv_indexes  indexes of the CSV records
 *  @param   csv_count    number of indexes
 *  @param   map          pointer to the CSV2CDS Map structure
 *  @param   cds          pointer to the CDSGroup structure
 *  @param   cds_start    index of the start record in the CDSGroup
 *  @param   flags        control flags
 *
 *  @retval   1  if successful
 *  @retval   0  if an error occurred
 */
int dsproc_map_csv_to_cds_by_index(
    CSVParser  *csv,
    int        *csv_indexes,
    int         csv_count,
    CSV2CDSMap *map,
    CDSGroup   *cds,
    int         cds_start,
    int         flags)
{
    int           dynamic_dod = dsproc_get_dynamic_dods_mode();
    const char   *csv_name;
    const char   *csv_units;
    const char  **csv_missings;
    CSVStrMap    *csv_str_map;
    double      (*csv_str_to_dbl)(const char *strval, int *status);
    int         (*csv_set_data)(
                    const char  *csv_strval,
                    const char **csv_missings,
                    CDSVar      *cds_var,
                    size_t       cds_sample_size,
                    CDSData      cds_missing,
                    CDSData      cds_datap);
    char        **csv_strvals;
    const char   *cds_name;
    const char   *cds_units;
    CDSVar       *cds_var;
    CDSDim       *cds_dim;
    CDSData       cds_data;
    void         *cds_data_start;
    CDSData       cds_missing;
    int           cds_nmissing;
    int           skip_debug_msg;
    int           status;
    size_t        sample_size;
    size_t        type_size;
    size_t        nbytes;
    int           is_missing;
    float         mv;
    int           mi, mvi, ri, smi, csvi;

    CDSUnitConverter unit_converter;

    cds_units = (const char *)NULL;

    DEBUG_LV1( DSPROC_LIB_NAME,
        "Mapping input CSV data to output dataset variables\n"
        " - input file:      %s\n"
        "     - start index: %d\n"
        "     - num samples: %d\n"
        " - output dataset:  %s\n"
        "     - start index: %d\n",
        csv->file_name, csv_indexes[0], csv_count, cds->name, cds_start);

    /* Loop over each entry in the variable map */

    for (mi = 0; map[mi].csv_name; ++mi) {

        cds_name       = map[mi].cds_name;

        csv_name       = map[mi].csv_name;
        csv_units      = map[mi].csv_units;
        csv_missings   = map[mi].csv_missings;

        csv_str_map    = map[mi].str_map;
        csv_str_to_dbl = map[mi].str_to_dbl;
        csv_set_data   = map[mi].set_data;

        skip_debug_msg = 0;

        /* Get the CSV field */

        csv_strvals = dsproc_get_csv_field_strvals(csv, csv_name);

        if (!csv_strvals) {

            ERROR( DSPROC_LIB_NAME,
                "Required column '%s' not found in CSV file: %s\n",
                csv_name, csv->file_name);

            dsproc_set_status(DSPROC_ECSV2CDS);
            return(0);
        }

        /* Get the CDS variable */

        cds_var = cds_get_var(cds, cds_name);

        if (!cds_var && dynamic_dod) {

            /* Define time variable if necessary */

            cds_dim = cds_get_dim(cds, "time");
            if (!cds_dim) {

                cds_dim = cds_define_dim(cds, "time", 0, 1);
                if (!cds_dim) {

                    ERROR( DSPROC_LIB_NAME,
                        "Could not create time dimension in dataset: %s\n",
                        cds_get_object_path(cds));

                    dsproc_set_status(DSPROC_ECSV2CDS);
                    return(0);
                }

                cds_var = cds_define_var(cds,
                    "time", CDS_DOUBLE, 1, (const char **)&(cds_dim->name));

                if (!cds_var) {

                    ERROR( DSPROC_LIB_NAME,
                        "Could not create 'time' variable in dataset: %s\n",
                        cds_get_object_path(cds));

                    dsproc_set_status(DSPROC_ECSV2CDS);
                    return(0);
                }
            }

/* BDE TODO:

Loop over CSV values to determine variable type (int, float, or char).
Autoset strlen_# dimension length for char type.

for (ri = 0; ri < csv_count; ++ri) {

    csvi = csv_indexes[ri];
    csv_strvals[csvi],
}
*/

            /* Define the variable */

            cds_var = cds_define_var(cds,
                cds_name, CDS_FLOAT, 1, (const char **)&(cds_dim->name));

            if (!cds_var) {

                ERROR( DSPROC_LIB_NAME,
                    "Could not create '%s' variable in dataset: %s\n",
                    cds_name, cds_get_object_path(cds));

                dsproc_set_status(DSPROC_ECSV2CDS);
                return(0);
            }

            /* Define the units attribute */

            if (csv_units) {
                if (!cds_define_att_text(cds_var, "units", "%s", csv_units)) {
                    dsproc_set_status(DSPROC_ECSV2CDS);
                    return(0);
                }
            }

            /* Define the missing_values attribute */

            mv = -9999.0;

            if (!cds_define_att(cds_var, "missing_value", CDS_FLOAT, 1, &mv)) {
                dsproc_set_status(DSPROC_ECSV2CDS);
                return(0);
            }
        }

        if (!cds_var) {

            ERROR( DSPROC_LIB_NAME,
               "Required variable '%s' not found in dataset: %s\n",
                cds_name, cds->name);

            dsproc_set_status(DSPROC_ECSV2CDS);
            return(0);
        }

        /* Check if data already exists in the CDS variable */

        if (cds_var->sample_count > (size_t)cds_start) {

            if (flags & CSV_OVERWRITE) {

                DEBUG_LV2( DSPROC_LIB_NAME,
                    " - * OVERWRITING EXISTING DATA * %s\t-> %s\n",
                    csv_name, cds_name);

                skip_debug_msg = 1;
            }
            else {
                DEBUG_LV2( DSPROC_LIB_NAME,
                    " - * NOT OVERWRITING EXISTING DATA * %s\t-> %s\n",
                    csv_name, cds_name);
                continue;
            }
        }

        /* Check if we need to do a unit conversion */

        unit_converter = (CDSUnitConverter)NULL;

        if (csv_units) {

            cds_units = cds_get_var_units(cds_var);
            if (cds_units) {

                status = cds_get_unit_converter(csv_units, cds_units, &unit_converter);
                if (status < 0) {

                    ERROR( DSPROC_LIB_NAME,
                        "Could not convert csv units '%s' to cds units '%s'\n",
                        csv_units, cds_units);

                    dsproc_set_status(DSPROC_ECSV2CDS);
                    return(0);
                }
            }
        }

        /* Get the missing value to use for the CDS variable */

        cds_missing.vp = (void *)NULL;
        cds_nmissing   = cds_get_var_missing_values(cds_var, &cds_missing.vp);

        if (cds_nmissing < 0) {

            ERROR( DSPROC_LIB_NAME,
                "Could not get missing value for variable: %s\n"
                " -> memory allocation error",
                cds_var->name);

            dsproc_set_status(DSPROC_ENOMEM);

            return(0);
        }

        if (cds_nmissing == 0) {

            if (csv_missings) {

                ERROR( DSPROC_LIB_NAME,
                    "Could not get missing value for variable: %s\n"
                    " -> missing_value attribute not defined",
                    cds_var->name);

                dsproc_set_status(DSPROC_ECSV2CDS);

                return(0);
            }

            cds_missing.vp = calloc(1, sizeof(double));
            cds_get_default_fill_value(cds_var->type, cds_missing.vp);
        }

        /* Map the data from the CSV field to the CDS variable */

        if (!skip_debug_msg) {
            DEBUG_LV2( DSPROC_LIB_NAME,
                " - %s\t-> %s\n",
                csv_name, cds_name);
        }

        cds_data.vp = cds_alloc_var_data(cds_var, cds_start, csv_count);
        if (!cds_data.vp) {

            ERROR( DSPROC_LIB_NAME,
                "Memory allocation error mapping CSV dataset to CDS dataset\n");

            dsproc_set_status(DSPROC_ENOMEM);
            if (cds_missing.vp) free(cds_missing.vp);
            return(0);
        }

        cds_data_start = cds_data.vp;

        if (csv_set_data) {

            sample_size = cds_var_sample_size(cds_var);
            type_size   = cds_data_type_size(cds_var->type);
            nbytes      = sample_size * type_size;

            for (ri = 0; ri < csv_count; ++ri) {

                csvi = csv_indexes[ri];

                status = csv_set_data(
                    csv_strvals[csvi],
                    csv_missings,
                    cds_var,
                    sample_size,
                    cds_missing,
                    cds_data);

                if (status == 0) {
                    if (cds_missing.vp) free(cds_missing.vp);
                    return(0);
                }

                cds_data.vp += nbytes;
            }
        }
        else if (cds_var->type == CDS_CHAR) {

            sample_size = cds_var_sample_size(cds_var);

            for (ri = 0; ri < csv_count; ++ri) {

                memset(cds_data.cp, *cds_missing.cp, sample_size);

                csvi       = csv_indexes[ri];
                is_missing = 0;

                if (!csv_strvals[csvi] || *csv_strvals[csvi] == '\0') {
                    is_missing = 1;
                }
                else if (csv_missings) {

                    for (mvi = 0; csv_missings[mvi]; ++mvi) {
                        if (strcmp(csv_strvals[csvi], csv_missings[mvi]) == 0) {
                            is_missing = 1;
                            break;
                        }
                    }
                }

                if (!is_missing) {
                    strncpy(cds_data.cp, csv_strvals[csvi], sample_size);
                }

                cds_data.cp += sample_size;
            }
        }
        else {

            switch (cds_var->type) {
                case CDS_BYTE:   CSV_MAP_TO_CDS(signed char, cds_data.bp, cds_missing.bp, atoi); break;
                case CDS_SHORT:  CSV_MAP_TO_CDS(short,       cds_data.sp, cds_missing.sp, atoi); break;
                case CDS_INT:    CSV_MAP_TO_CDS(int,         cds_data.ip, cds_missing.ip, atoi); break;
                case CDS_FLOAT:  CSV_MAP_TO_CDS(float,       cds_data.fp, cds_missing.fp, atof); break;
                case CDS_DOUBLE: CSV_MAP_TO_CDS(double,      cds_data.dp, cds_missing.dp, atof); break;
                default:

                    ERROR( DSPROC_LIB_NAME,
                        "Could not map CSV data to CDS variable: %s:%s\n"
                        " -> invalid CDSDataType: %d\n",
                        cds->name, cds_var->name, (int)cds_var->type);

                    dsproc_set_status(DSPROC_ECSV2CDS);
                    if (cds_missing.vp) free(cds_missing.vp);
                    return(0);
            }
        }

        /* Convert CSV units to CDS units */

        if (unit_converter) {

            DEBUG_LV2( DSPROC_LIB_NAME,
                "     - converting units: '%s' to '%s'\n",
                csv_units, cds_units);

            sample_size = cds_var_sample_size(cds_var);

            cds_convert_units(unit_converter,
                cds_var->type,           // in type
                csv_count * sample_size, // length,
                cds_data_start,          // void *  in_data,
                cds_var->type,           // CDSDataType     out_type,
                cds_data_start,          // void *  out_data,
                cds_nmissing,            // size_t  nmap,
                cds_missing.vp,          // void *  in_map,
                cds_missing.vp,          // void *  out_map,
                NULL, NULL, NULL, NULL);
        }

        if (cds_missing.vp) free(cds_missing.vp);
    }

    return(1);
}
