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
*     email: brian.ermold@pnl.gov
*
*******************************************************************************/

/** @file dsproc_datastreams.c
 *  Datastream Functions.
 */

#include "dsproc3.h"
#include "dsproc_private.h"

extern DSProc *_DSProc; /**< Internal DSProc structure */
extern int _DisableDBUpdates; /**< Flag indicating if DB updates are disabled */

/** @privatesection */

/*******************************************************************************
 *  Static Functions Visible Only To This Module
 */

static const char *gNCExtension     = "nc";
static const char *gNetcdfExtension = "cdf";
static DSFormat    gOutputFormat    = DSF_NETCDF;

/**
 *  Static: Create a new DataStream structure.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  site      - site name, or NULL to use the process site
 *  @param  facility  - facility name, or NULL to use the process facility
 *  @param  dsc_name  - datastream class name
 *  @param  dsc_level - datastream class level
 *  @param  role      - specifies input or output datastream
 *
 *  @return
 *    - pointer to the new DataStream structure
 *    - NULL if an error occurs
 */
static DataStream *_dsproc_create_datastream(
    const char  *site,
    const char  *facility,
    const char  *dsc_name,
    const char  *dsc_level,
    DSRole       role)
{
    DataStream *ds;
    const char *role_name;

    if (!site)     site     = _DSProc->site;
    if (!facility) facility = _DSProc->facility;

    role_name = _dsproc_dsrole_to_name(role);

    ds = (DataStream *)calloc(1, sizeof(DataStream));

    if (!ds) {

        ERROR( DSPROC_LIB_NAME,
            "Could not create new %s DataStream structure for: %s%s%s.%s\n"
            " -> memory allocation error\n",
            role_name, site, dsc_name, facility, dsc_level);

        dsproc_set_status(DSPROC_ENOMEM);
        return((DataStream *)NULL);
    }

    strncpy((char *)ds->site,      site,      7);
    strncpy((char *)ds->facility,  facility,  7);
    strncpy((char *)ds->dsc_name,  dsc_name, 63);
    strncpy((char *)ds->dsc_level, dsc_level, 7);

    ds->role = role;
    ds->name = string_create("%s%s%s.%s", site, dsc_name, facility, dsc_level);

    if (!ds->name) {

        ERROR( DSPROC_LIB_NAME,
            "Could not create new %s DataStream structure for: %s%s%s.%s\n"
            " -> memory allocation error\n",
            role_name, site, dsc_name, facility, dsc_level);

        _dsproc_free_datastream(ds);
        dsproc_set_status(DSPROC_ENOMEM);
        return((DataStream *)NULL);
    }

    return(ds);
}

/*******************************************************************************
 *  Private Functions Visible Only To This Library
 */

/**
 *  Private: Add file that has been created or updated by the current process.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  ds   - pointer to the DataStream structure
 *  @param  file - name of the file that was created or updated
 *
 *  @return
 *    - 1 if successful
 *    - 0 if a memory allocation error occurred
 */
int _dsproc_add_updated_dsfile_name(DataStream *ds, char *file)
{
    int    nfiles;
    int    fi;
    char **new_list;

    /* Check if the file has already been added to the list,
     * and get the length of the list. */

    nfiles = 0;
    if (ds->updated_files) {
        for (fi = 0; ds->updated_files[fi]; ++fi) {
            nfiles += 1;
            if (strcmp(ds->updated_files[fi], file) == 0) {
                /* File already exists in the list */
                return(1);
            }
        }
    }

    /* If we get here we need to add the file to the list. */

    new_list = (char **)realloc(ds->updated_files, (nfiles+2)*sizeof(char *));
    if (!new_list) goto MEMORY_ERROR;

    ds->updated_files = new_list;

    ds->updated_files[nfiles] = strdup(file);
    if (!ds->updated_files[nfiles]) goto MEMORY_ERROR;

    ds->updated_files[nfiles+1] = (char *)NULL;

    return(1);

MEMORY_ERROR:

    ERROR( DSPROC_LIB_NAME,
        "Memory allocation error adding file to updated datastream files list.\n");
    dsproc_set_status(DSPROC_ENOMEM);
    return(0);
}

/**
 *  Private: Get the last file that was created or updated by the current process.
 *
 *  @param  ds     - pointer to the DataStream structure
 *  @param  dsfile - output: pointer to the DSFile structure for the last file
 *                   that was created or updated by the current process
 *
 *  @return
 *    -  1  success
 *    -  0  no files have been created or updated
 *    - -1  if an error occured
 */
int _dsproc_get_last_updated_dsfile(DataStream *ds, DSFile **dsfile)
{
    int fi;

    if (ds->updated_files) {

        for (fi = 0; ds->updated_files[fi]; ++fi);

       *dsfile = _dsproc_get_dsfile(ds->dir, ds->updated_files[fi-1]);
        if (!*dsfile) return(-1);

        if ((*dsfile)->ntimes == 0) {
            return(0);
        }

        return(1);
    }

    return(0);
}

/**
 *  Private: Free the fetched dataset in a DataStream structure.
 *
 *  @param  ds - pointer to the DataStream structure
 */
void _dsproc_free_datastream_fetched_cds(DataStream *ds)
{
    if (ds) {

        if (ds->fetched_cds) {
            cds_set_definition_lock(ds->fetched_cds, 0);
            cds_delete_group(ds->fetched_cds);
        }

        ds->fetched_cds = (CDSGroup *)NULL;
        memset(&(ds->fetch_begin), 0, sizeof(timeval_t));
        memset(&(ds->fetch_end), 0, sizeof(timeval_t));
    }
}

/**
 *  Private: Free the output dataset in a DataStream structure.
 *
 *  @param  ds - pointer to the DataStream structure
 */
void _dsproc_free_datastream_out_cds(DataStream *ds)
{
    if (ds && ds->out_cds) {

        cds_set_definition_lock(ds->out_cds, 0);
        cds_delete_group(ds->out_cds);

        ds->out_cds = (CDSGroup *)NULL;
    }
}

/**
 *  Private: Free the output dataset in a DataStream structure.
 *
 *  @param  ds - pointer to the DataStream structure
 */
void _dsproc_free_datastream_metadata(DataStream *ds)
{
    if (ds && ds->metadata) {

        cds_set_definition_lock(ds->metadata, 0);
        cds_delete_group(ds->metadata);

        ds->metadata = (CDSGroup *)NULL;
    }
}

/**
 *  Private: Free all memory used by a DataStream structure.
 *
 *  @param  ds - pointer to the DataStream structure
 */
void _dsproc_free_datastream(DataStream *ds)
{
    int fi;

    if (ds) {

        if (ds->name)        free((void *)ds->name);
        if (ds->dir)         _dsproc_free_dsdir(ds->dir);

        if (ds->dsdod)       dsdb_free_dsdod(ds->dsdod);
        if (ds->metadata)    _dsproc_free_datastream_metadata(ds);

        if (ds->dsprops)     dsdb_free_ds_properties(ds->dsprops);

        if (ds->fetched_cds) _dsproc_free_datastream_fetched_cds(ds);
        if (ds->out_cds)     _dsproc_free_datastream_out_cds(ds);

        if (ds->ret_cache)   _dsproc_free_ret_ds_cache(ds->ret_cache);
        if (ds->dsvar_dqrs)  _dsproc_free_dsvar_dqrs(ds->dsvar_dqrs);

        if (ds->updated_files) {
            for (fi = 0; ds->updated_files[fi]; ++fi) {
                free(ds->updated_files[fi]);
            }
            free(ds->updated_files);
        }

        free(ds);
    }
}

/**
 *  Private: Initialize the input datastreams defined in the database.
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int _dsproc_init_input_datastreams(void)
{
    DSClass **ds_classes;
    int       nclasses;
    int       dsci;
    int       ds_id;

    nclasses = dsproc_get_input_ds_classes(&ds_classes);

    if (nclasses < 0) {
        return(0);
    }

    for (dsci = 0; dsci < nclasses; dsci++) {

        ds_id = dsproc_init_datastream(
            _DSProc->site,
            _DSProc->facility,
            ds_classes[dsci]->name,
            ds_classes[dsci]->level,
            DSR_INPUT,
            NULL, 0, -1);

        if (ds_id < 0) {
            return(0);
        }
    }

    return(1);
}

/**
 *  Private: Initialize the output datastreams defined in the database.
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int _dsproc_init_output_datastreams(void)
{
    DSClass **ds_classes;
    int       nclasses;
    int       dsci;
    int       ds_id;

    nclasses = dsproc_get_output_ds_classes(&ds_classes);

    if (nclasses < 0) {
        return(0);
    }

    for (dsci = 0; dsci < nclasses; dsci++) {

        ds_id = dsproc_init_datastream(
            _DSProc->site,
            _DSProc->facility,
            ds_classes[dsci]->name,
            ds_classes[dsci]->level,
            DSR_OUTPUT,
            NULL, 0, -1);

        if (ds_id < 0) {
            return(0);
        }
    }

    return(1);
}

/**
 *  Private: datastream role name.
 *
 *  @param  role - datastream role
 *
 *  @return  string name
 */
const char *_dsproc_dsrole_to_name(DSRole role)
{
    static const char *role_name;

    role_name = (role == DSR_INPUT) ? "input" : "output";

    return(role_name);
}

/**
 *  Private: data format name.
 *
 *  @param  format - data format
 *
 *  @return  string name
 */
const char *_dsproc_dsformat_to_name(DSFormat format)
{
    static const char *name;

    switch (format) {

        case DSF_NETCDF: name = "NetCDF3"; break;
        case DSF_CSV:    name = "CSV";     break;
        case DSF_RAW:    name = "RAW";     break;
        case DSF_JPG:    name = "JPG";     break;
        case DSF_PNG:    name = "PNG";     break;
        default:         name = "Unknown"; break;
    }

    return(name);
}

/**
 *  Private: data format name.
 *
 *  @param  name - string name
 *
 *  @return  data format
 */
DSFormat _dsproc_name_to_dsformat(const char *name)
{
    DSFormat format;

    if (strcmp(name, "NetCDF") == 0) {
        format = DSF_NETCDF;
    }
    else if (strcmp(name, "NetCDF3") == 0) {
        format = DSF_NETCDF;
    }
    else if (strcmp(name, "CSV") == 0) {
        format = DSF_CSV;
    }
    else if (strcmp(name, "RAW") == 0) {
        format = DSF_RAW;
    }
    else if (strcmp(name, "JPG") == 0) {
        format = DSF_JPG;
    }
    else if (strcmp(name, "PNG") == 0) {
        format = DSF_PNG;
    }
    else {
        format = 0;
    }

    return(format);
}

/**
 *  Private: Free the OutputInterval entries in the global _DSProc structure.
 */
void _dsproc_free_output_intervals(void)
{
    OutputInterval *outint = _DSProc->output_intervals;
    OutputInterval *next;

    while (outint) {
        if (outint->dsc_name)  free(outint->dsc_name);
        if (outint->dsc_level) free(outint->dsc_level);
        next = outint->next;
        free(outint);
        outint = next;
    }

    _DSProc->output_intervals  = (OutputInterval *)NULL;
}

/**
 *  Private: Get an OutputInterval entry from the global _DSProc structure.
 *   
 *  @param  dsc_name  - datastream class name
 *  @param  dsc_level - datastream class level
 *
 *  @return
 *    - outint pointer to the OutputInterval structure
 *    - NULL if not found
 */
OutputInterval *_dsproc_get_output_interval(
    const char *dsc_name,
    const char *dsc_level)
{
    OutputInterval *outint;

    for (outint = _DSProc->output_intervals; outint; outint = outint->next) {

        /* Check if datastream class names match */

        if (!dsc_name) {
            if (outint->dsc_name) {
                continue;
            }
        }
        else if (!outint->dsc_name) {
            continue;
        }
        else if (strcmp(dsc_name, outint->dsc_name) != 0) {
            continue;
        }

        /* Check if datastream class levels match */

        if (!dsc_level) {
            if (outint->dsc_level) {
                continue;
            }
        }
        else if (!outint->dsc_level) {
            continue;
        }
        else if (strcmp(dsc_level, outint->dsc_level) != 0) {
            continue;
        }

        /* Datastream class name and level match */
        break;
    }

    return(outint);
}

/**
 *  Private: Add an output interval to the global _DSProc structure.
 *   
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  dsc_name        - datastream class name
 *  @param  dsc_level       - datastream class level
 *  @param  split_mode      - the file splitting mode
 *  @param  split_start     - start of the split interval
 *  @param  split_interval  - split interval
 *  @param  split_tz_offset - time zone offset
 *
 *  @return
 *    - 1 if successful
 *    - 0 memory allocation failure
 */
int _dsproc_add_output_interval(
    const char *dsc_name,
    const char *dsc_level,
    SplitMode   split_mode,
    double      split_start,
    double      split_interval,
    int         split_tz_offset)
{
    OutputInterval *outint;

    /* Check if an entry for this datastream class already exists */

    outint = _dsproc_get_output_interval(dsc_name, dsc_level);

    if (!outint) {

        outint = (OutputInterval *)calloc(1, sizeof(OutputInterval));
        if (!outint) goto MEMORY_ERROR;

        if (dsc_name) {
            outint->dsc_name = strdup(dsc_name);
            if (!outint->dsc_name) goto MEMORY_ERROR;
        }

        if (dsc_level) {
            outint->dsc_level = strdup(dsc_level);
            if (!outint->dsc_level) goto MEMORY_ERROR;
        }

        outint->next = _DSProc->output_intervals;
        _DSProc->output_intervals = outint;
    }

    outint->split_mode      = split_mode;
    outint->split_start     = split_start;
    outint->split_interval  = split_interval;
    outint->split_tz_offset = split_tz_offset;

    return(1);

MEMORY_ERROR:

    if (outint) free(outint);
    ERROR( DSPROC_LIB_NAME,
        "Memory allocation error adding new output interval entry\n");
    dsproc_set_status(DSPROC_ENOMEM);
    return(0);
}

/**
 *  Private: Parse output interval(s) string.
 *
 *  The format of the input string is:
 * 
 *  [name.level-]hourly|daily|monthly|yearly[-utc|local]
 *
 *  Additional entries can be added by separating them with commas.
 *   
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  string - string containing the output interval information
 *
 *  @return
 *    - 1 if successful
 *    - 0 invalid output interval format or memory allocation failure
 */
int _dsproc_parse_output_interval_string(const char *string)
{
    DSClass   **ds_classes;
    int         nclasses;
    int         dsci;

    char       *str_copy = (char *)NULL;

    int         max_entries = 64;
    char       *entries[max_entries];
    int         nentries;
    char       *entry;
    char       *entry_copy = (char *)NULL;
    int         ei;

    int         max_parts = 3;
    char       *parts[3];
    int         nparts;
    char       *part;
    int         pi;

    char       *dotp;
    char       *dsc_name;
    char       *dsc_level;
    SplitMode   split_mode;
    double      split_start;
    double      split_interval;
    int         split_local;
    int         split_tz_offset;

    if (!string || *string == '\0') {
        return(1);
    }

    /* Get list of output datastream class so we can 
     * verify the entries in the output insterval string. */

    nclasses = dsproc_get_output_ds_classes(&ds_classes);
    if (nclasses < 0) return(0);

    /* Parse the output interval string */

    DEBUG_LV1( DSPROC_LIB_NAME,
        "Parsing output interval string: '%s'\n", string);

    str_copy = strdup(string);
    if (!str_copy) goto MEMORY_ERROR;

    nentries = dsproc_split_csv_string(str_copy, ',', max_entries, entries);
    if (nentries > max_entries) {

        ERROR( DSPROC_LIB_NAME,
            "Too many output intervals specified in string: %s\n"
            " - maximum number is 64 but found %d\n",
            string, nentries);

        dsproc_set_status("Exceeded Maximum Number of Output Interval Specifications");
        free(str_copy);
        return(0);
    }

    /* Loop over the entries in the output interval string */

    for (ei = 0; ei < nentries; ++ei) {

        entry = entries[ei];

        if (entry_copy) free(entry_copy);
        entry_copy = strdup(entry);
        if (!entry_copy) goto MEMORY_ERROR;

        DEBUG_LV1( DSPROC_LIB_NAME,
            "  - parsing: '%s'\n", entry);

        /* Parse the output interval entry, it should have the form:
         *
         * [name.level-]hourly|daily|monthly|yearly[-utc|local] */

        nparts = dsproc_split_csv_string(entry_copy, '-', max_parts, parts);
        if (nparts > max_parts) goto INVALID_ENTRY;

        dsc_name        = (char *)NULL;
        dsc_level       = (char *)NULL;
        split_mode      = (SplitMode)-1;
        split_local     = 0;
        split_tz_offset = 0;

        for (pi = 0; pi < nparts; pi++) {

            part = parts[pi];

            if (strcmp(part, "hourly") == 0) {
                split_mode     = SPLIT_ON_HOURS;
                split_start    = 0;
                split_interval = 1;
            }
            else if (strcmp(part, "daily") == 0) {
                split_mode     = SPLIT_ON_HOURS;
                split_start    = 0;
                split_interval = 24;
            }
            else if (strcmp(part, "monthly") == 0) {
                split_mode     = SPLIT_ON_MONTHS;
                split_start    = 1;
                split_interval = 1;
            }
            else if (strcmp(part, "yearly") == 0) {
                split_mode     = SPLIT_ON_MONTHS;
                split_start    = 1;
                split_interval = 12;
            }
            else if (strcmp(part, "always")   == 0 ||
                     strcmp(part, "on_store") == 0) {
                split_mode     = SPLIT_ON_STORE;
                split_start    = 0;
                split_interval = 0;
            }
            else if (strcmp(part, "never") == 0 ||
                     strcmp(part, "none")  == 0) {
                split_mode     = SPLIT_NONE;
                split_start    = 0;
                split_interval = 0;
            }
            else if (strcmp(part, "utc") == 0) {
                split_local = 0;
            }
            else if (strcmp(part, "local") == 0) {
                split_local = 1;
            }
            else if ( (dotp = strchr(part, '.')) ) {

                *dotp     = '\0';
                dsc_name  = part;
                dsc_level = dotp + 1;

                /* Check for valid output datastream class */

                for (dsci = 0; dsci < nclasses; dsci++) {

                    if (strcmp(dsc_name, ds_classes[dsci]->name) == 0 &&
                        strcmp(dsc_level, ds_classes[dsci]->level) == 0) {
                        break;
                    }
                }

                if (dsci == nclasses) {

                    /* not a valid output datastream class */

                    ERROR( DSPROC_LIB_NAME,
                        "Invalid datastream class '%s.%s' in output interval string: '%s'\n",
                        dsc_name, dsc_level, string);

                    dsproc_set_status("Invalid Datastream Class in Output Interval String");
                    free(str_copy);
                    free(entry_copy);
                    return(0);
                }
            }
            else {
                ERROR( DSPROC_LIB_NAME,
                    "Invalid entry '%s' in output interval string: '%s'\n",
                    part, entry);

                dsproc_set_status("Invalid Entry in Output Interval String");

                free(entry_copy);
                free(str_copy);
                return(0);
            }
        }

        if (split_mode == (SplitMode)-1) {
            goto INVALID_ENTRY;
        }

        /* Add entry to array of output interval structures in _DSProc struct */

        if (split_local) {
            if (dsproc_estimate_timezone(&split_tz_offset) < 0) {
                free(str_copy);
                free(entry_copy);
                return(0);
            }
        }

        if (msngr_debug_level || msngr_provenance_level) {

            const char *db_split_mode;
            const char *db_dsc_name;
            const char *db_dsc_level;

            db_dsc_name  = (dsc_name)  ? dsc_name  : "<null>";
            db_dsc_level = (dsc_level) ? dsc_level : "<null>";

            switch (split_mode) {
                case SPLIT_ON_STORE:  db_split_mode = "on store"; break;
                case SPLIT_ON_HOURS:  db_split_mode = "hourly";   break;
                case SPLIT_ON_DAYS:   db_split_mode = "daily";    break;
                case SPLIT_ON_MONTHS: db_split_mode = "monthly";  break;
                case SPLIT_NONE:      db_split_mode = "none";     break;
                default:              db_split_mode = "<not set>";
            }

            DEBUG_LV1( DSPROC_LIB_NAME,
                "      - dsc_name:        '%s'\n"
                "      - dsc_level:       '%s'\n"
                "      - split_mode:      '%s'\n"
                "      - split_start:     '%g'\n"
                "      - split_interval:  '%g'\n"
                "      - split_tz_offset: '%d'\n",
                db_dsc_name, db_dsc_level, db_split_mode,
                split_start, split_interval, split_tz_offset);
        }

        if (!_dsproc_add_output_interval(dsc_name, dsc_level,
            split_mode, split_start, split_interval, split_tz_offset)) {

            free(str_copy);
            free(entry_copy);
            return(0);
        }
    }

    free(str_copy);
    if (entry_copy) free(entry_copy);
    return(1);

MEMORY_ERROR:

    ERROR( DSPROC_LIB_NAME,
        "Could not parse output interval string: %s\n"
        " -> memory allocation error\n",
        string);

    dsproc_set_status(DSPROC_ENOMEM);

    if (str_copy)   free(str_copy);
    if (entry_copy) free(entry_copy);
    return(0);

INVALID_ENTRY:

    ERROR( DSPROC_LIB_NAME,
        "Invalid entry '%s' in output interval string: '%s'\n",
        entry, string);

    dsproc_set_status("Invalid Entry in Output Interval String");

    free(entry_copy);
    free(str_copy);
    return(0);
}

/** @publicsection */

/*******************************************************************************
 *  Internal Functions Visible To The Public
 */

/**
 *  Initialize a new datastream.
 *
 *  If the specified datastream already exists, the ID of the existing
 *  datastream will be returned.
 *
 *  The default datastream path will be set if path = NULL,
 *  see dsproc_set_datastream_path() for details.
 *
 *  The default datastream format will be set if format = 0,
 *  see dsproc_set_datastream_format() for details.
 *
 *  The default datastream flags will be set if flags < 0,
 *  see dsproc_set_datastream_flags() for details.
 *
 *  For output datastreams:
 *
 *    - The datastream DOD information will be loaded from the database.
 *    - The previously processed data times will be loaded from the database
 *      (if database updates have not been disabled).
 *    - The preserve_dots value used by the rename functions will be set to the
 *      default value (see dsproc_set_rename_preserve_dots()).
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  site      - site name, or NULL to use the process site
 *  @param  facility  - facility name, or NULL to use the process facility
 *  @param  dsc_name  - datastream class name
 *  @param  dsc_level - datastream class level
 *  @param  role      - specifies input or output datastream
 *  @param  path      - path to the datastream directory
 *  @param  format    - datastream data format
 *  @param  flags     - control flags
 *
 *  @return
 *    - datastream ID
 *    - -1 if an error occurs
 */
int dsproc_init_datastream(
    const char  *site,
    const char  *facility,
    const char  *dsc_name,
    const char  *dsc_level,
    DSRole       role,
    const char  *path,
    DSFormat     format,
    int          flags)
{
    DataStream **datastreams;
    DataStream  *ds;
    DSProp     **dsprops;
    const char  *role_name;
    int          ds_id;
    int          status;

    OutputInterval *outint;

    if (!site)     site     = _DSProc->site;
    if (!facility) facility = _DSProc->facility;

    /* Check if this datastream already exists */

    for (ds_id = 0; ds_id < _DSProc->ndatastreams; ds_id++) {

        ds = _DSProc->datastreams[ds_id];

        if ((strcmp(ds->site,      site)      == 0) &&
            (strcmp(ds->facility,  facility)  == 0) &&
            (strcmp(ds->dsc_name,  dsc_name)  == 0) &&
            (strcmp(ds->dsc_level, dsc_level) == 0) &&
            (ds->role == role)) {

            return(ds_id);
        }
    }

    role_name = _dsproc_dsrole_to_name(role);

    if (msngr_debug_level || msngr_provenance_level) {
        DEBUG_LV1( DSPROC_LIB_NAME,
            "%s%s%s.%s: Initializing %s datastream\n",
            site, dsc_name, facility, dsc_level, role_name);
    }

    /* Create new DataStream structure */

    ds = _dsproc_create_datastream(site, facility, dsc_name, dsc_level, role);

    if (!ds) {
        return(-1);
    }

    /* Add datastream to datastreams array */

    datastreams = (DataStream **)realloc(_DSProc->datastreams,
        (_DSProc->ndatastreams + 1) * sizeof(DataStream *));

    if (!datastreams) {

        ERROR( DSPROC_LIB_NAME,
            "Could not intialize %s datastream: %s%s%s.%s\n"
            " -> memory allocation error\n",
            role_name, site, dsc_name, facility, dsc_level);

        _dsproc_free_datastream(ds);
        dsproc_set_status(DSPROC_ENOMEM);
        return(-1);
    }

    ds_id = _DSProc->ndatastreams;
    datastreams[ds_id] = ds;

    _DSProc->datastreams = datastreams;
    _DSProc->ndatastreams++;

    /* Set the datastream path */

    if (!dsproc_set_datastream_path(ds_id, path)) {
        _dsproc_free_datastream(ds);
        _DSProc->ndatastreams--;
        return(-1);
    }

    /* Set the datastream format */

    dsproc_set_datastream_format(ds_id, format);

    /* Set datastream flags */

    dsproc_set_datastream_flags(ds_id, flags);

    /* Get datastream properties from the database */

    status = dsproc_get_datastream_properties(ds_id, &dsprops);
    if (status < 0) {
        dsproc_db_disconnect();
        _dsproc_free_datastream(ds);
        _DSProc->ndatastreams--;
        return(-1);
    }

    /* Get/Set additional information for output datastreams */

    if (role == DSR_OUTPUT) {

        /* Set datastream file splitting mode */

        outint = _dsproc_get_output_interval(dsc_name, dsc_level);
        if (!outint) {
            outint = _dsproc_get_output_interval(NULL, NULL);
        }

        if (outint) {
            dsproc_set_datastream_split_mode(
                ds_id,
                outint->split_mode,
                outint->split_start,
                outint->split_interval);

            if (outint->split_tz_offset) {
                dsproc_set_datastream_split_tz_offset(
                    ds_id, outint->split_tz_offset);
            }
        }
        else {
            /* Set default split mode */
            if (_DSProc->model & DSP_INGEST) {
                dsproc_set_datastream_split_mode(ds_id, SPLIT_ON_HOURS, 0.0, 24.0);
            }
            else {
                dsproc_set_datastream_split_mode(ds_id, SPLIT_ON_STORE, 0.0, 0.0);
            }
        }

        /* Set the default preserve dots value for renaming raw files */

        if (ds->dsc_level[0] == '0') {
            dsproc_set_rename_preserve_dots(ds_id, -1);
        }

        /* Get information stored in the database */

        if (!dsproc_db_connect()) {
            _dsproc_free_datastream(ds);
            _DSProc->ndatastreams--;
            return(-1);
        }

        if (ds->format == DSF_NETCDF ||
            ds->format == DSF_CSV) {

            /* Get the DSDOD if it has been defined */

            status = _dsproc_get_dsdod(ds, _DSProc->cmd_line_begin);

            if (status < 0) {
                dsproc_db_disconnect();
                _dsproc_free_datastream(ds);
                _DSProc->ndatastreams--;
                return(-1);
            }
        }

        /* Load the previously processed data times */

        if (!_DisableDBUpdates) {

            if (!_dsproc_get_output_datastream_times(ds)) {

                dsproc_db_disconnect();
                _dsproc_free_datastream(ds);
                _DSProc->ndatastreams--;
                return(-1);
            }
        }

        dsproc_db_disconnect();
    }

    return(ds_id);
}

/**
 *  Set the control flags for a datastream.
 *
 *  Default datastream flags set if flags < 0:
 *
 *    - DS_STANDARD_QC            'b' level datastreams
 *    - DS_FILTER_NANS            'a' and 'b' level datastreams
 *    - DS_OVERLAP_CHECK          all output datastreams
 *    - DS_FILTER_VERSIONED_FILES input datastreams that are not level '0'
 *
 *  <b>Control Flags:</b>
 *
 *    - DS_STANDARD_QC     = Apply standard QC before storing a dataset.
 *
 *    - DS_FILTER_NANS     = Replace NaN and Inf values with missing values
 *                           before storing a dataset.
 *
 *    - DS_OVERLAP_CHECK   = Check for overlap with previously processed data.
 *                           This flag will be ignored and the overlap check
 *                           will be skipped if reprocessing mode is enabled,
 *                           or asynchronous processing mode is enabled.
 *
 *    - DS_PRESERVE_OBS    = Preserve distinct observations when retrieving
 *                           data. Only observations that start within the
 *                           current processing interval will be read in.
 *
 *    - DS_DISABLE_MERGE   = Do not merge multiple observations in retrieved
 *                           data. Only data for the current processing interval
 *                           will be read in.
 *
 *    - DS_SKIP_TRANSFORM  = Skip the transformation logic for all variables
 *                           in this datastream.
 *
 *    - DS_ROLLUP_TRANS_QC = Consolidate the transformation QC bits for all
 *                           variables when mapped to the output datasets.
 *
 *    - DS_SCAN_MODE       = Enable scan mode for datastream that are not
 *                           expected to be continuous. This prevents warning
 *                           messages from being generated when data is not
 *                           found within a processing interval. Instead, a
 *                           message will be written to the log file indicating
 *                           that the procesing interval was skipped.
 *
 *    - DS_OBS_LOOP        = Loop over observations instead of time intervals.
 *                           This also sets the DS_PRESERVE_OBS flag.
 *
 *    - DS_FILTER_VERSIONED_FILES = Check for files with .v# version extensions
 *                                  and filter out lower versioned files. Files
 *                                  without a version extension take precedence.
 * 
 *  @param  ds_id - datastream ID
 *  @param  flags - flags to set
 *
 *  @see dsproc_unset_datastream_flags()
 */
void dsproc_set_datastream_flags(int ds_id, int flags)
{
    DataStream *ds = _DSProc->datastreams[ds_id];

    if (flags < 0) {

        flags = 0;

        if (ds->role == DSR_INPUT) {
            if (ds->dsc_level[0] != '0') {
                flags |= DS_FILTER_VERSIONED_FILES;
            }
        }

        if (ds->role == DSR_OUTPUT) {

            if (ds->dsc_level[0] != '0') {
                flags |= DS_OVERLAP_CHECK;
            }

            if (ds->dsc_level[0] == 'a') {
                flags |= DS_FILTER_NANS;
            }
            else if (ds->dsc_level[0] == 'b') {
                flags |= DS_STANDARD_QC;
                flags |= DS_FILTER_NANS;
            }
        }
    }

    if (flags & DS_OBS_LOOP) {
        flags |= DS_PRESERVE_OBS;
    }

    if (flags & DS_FILTER_VERSIONED_FILES) {
        if (ds->dir) {
            ds->dir->filter_versioned_files = 1;
        }
    }

    if (msngr_debug_level || msngr_provenance_level) {

        DEBUG_LV1( DSPROC_LIB_NAME,
            "%s: Setting %s datastream control flags\n",
            ds->name, _dsproc_dsrole_to_name(ds->role));

        if (flags & DS_STANDARD_QC) {
            DEBUG_LV1( DSPROC_LIB_NAME, " - DS_STANDARD_QC\n");
        }

        if (flags & DS_FILTER_NANS) {
            DEBUG_LV1( DSPROC_LIB_NAME, " - DS_FILTER_NANS\n");
        }

        if (flags & DS_OVERLAP_CHECK) {
            DEBUG_LV1( DSPROC_LIB_NAME, " - DS_OVERLAP_CHECK\n");
        }

        if (flags & DS_PRESERVE_OBS) {
            DEBUG_LV1( DSPROC_LIB_NAME, " - DS_PRESERVE_OBS\n");
        }

        if (flags & DS_DISABLE_MERGE) {
            DEBUG_LV1( DSPROC_LIB_NAME, " - DS_DISABLE_MERGE\n");
        }

        if (flags & DS_SKIP_TRANSFORM) {
            DEBUG_LV1( DSPROC_LIB_NAME, " - DS_SKIP_TRANSFORM\n");
        }

        if (flags & DS_ROLLUP_TRANS_QC) {
            DEBUG_LV1( DSPROC_LIB_NAME, " - DS_ROLLUP_TRANS_QC\n");
        }

        if (flags & DS_SCAN_MODE) {
            DEBUG_LV1( DSPROC_LIB_NAME, " - DS_SCAN_MODE\n");
        }

        if (flags & DS_OBS_LOOP) {
            DEBUG_LV1( DSPROC_LIB_NAME, " - DS_OBS_LOOP\n");
        }

        if (flags & DS_FILTER_VERSIONED_FILES) {
            DEBUG_LV1( DSPROC_LIB_NAME, " - DS_FILTER_VERSIONED_FILES\n");
        }
    }

    ds->flags |= flags;
}

/**
 *  Set the data format of a datastream.
 *
 *  Default datastream format set if format = 0:
 *
 *    - DSF_RAW    for level '0' datastreams
 *    - DSF_NETCDF for all other datastream levels
 *
 *  <b>DataStream Formats:</b>
 *
 *    - DSF_NETCDF = NetCDF 3 Format;    extension = "cdf".
 *    - DSF_CSV    = CSV Format;         extension = "csv".
 *    - DSF_RAW    = Generic Raw Format; extension = "raw".
 *    - DSF_PNG    = PNG Image Format;   extension = "png".
 *    - DSF_JPG    = JPG Image Format;   extension = "jpg".
 *
 *  @param  ds_id  - datastream ID
 *  @param  format - datastream data format
 */
void dsproc_set_datastream_format(int ds_id, DSFormat format)
{
    DataStream *ds = _DSProc->datastreams[ds_id];
    const char *name;
    const char *extension;

    if (!format) {
        format = (ds->dsc_level[0] == '0') ? DSF_RAW: gOutputFormat;
    }

    switch (format) {

        case DSF_NETCDF: name = "NetCDF3"; extension = gNetcdfExtension; break;
        case DSF_CSV:    name = "CSV";     extension = "csv"; break;
        case DSF_RAW:    name = "RAW";     extension = "raw"; break;
        case DSF_JPG:    name = "JPG";     extension = "jpg"; break;
        case DSF_PNG:    name = "PNG";     extension = "png"; break;
        default:         name = "Unknown"; extension = "raw"; break;
    }

    if (msngr_debug_level || msngr_provenance_level) {

        DEBUG_LV1( DSPROC_LIB_NAME,
            "%s: Setting %s datastream format: %s\n",
            ds->name, _dsproc_dsrole_to_name(ds->role), name);
    }

    ds->format = format;

    strncpy((char *)ds->extension, extension, 63);
}

/**
 *  Set the data format for all output datastreams.
 *
 *  By default the data format for all output datastreams is NETCDF3.
 *  This function can be used to change the output format to CSV by
 *  specifying DSF_CSV.  The only options currently supported by this
 *  function are DSF_NETCDF and DSF_CSV.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  format - datastream data format
 *
 *  @return
 *    - 1 if successful
 *    - 0 invalid output format
 */
int dsproc_set_output_format(DSFormat format)
{
    const char *name = _dsproc_dsformat_to_name(format);
    int         ds_id;

    DEBUG_LV1( DSPROC_LIB_NAME,
        "Setting data format for all output datastreams to: %s\n",
        name);

    /* Set global output format */

    if (format != DSF_NETCDF &&
        format != DSF_CSV) {

        ERROR( DSPROC_LIB_NAME,
            "Invalid output datastream format: %s\n"
            " -> only DSF_NETCDF and DSF_CSV are currently supported\n",
            name);

        dsproc_set_status(DSPROC_EBADOUTFORMAT);
        return(0);
    }

    gOutputFormat = format;

    /* Update any output datastreams that have already been defined */

    for (ds_id = 0; ds_id < _DSProc->ndatastreams; ds_id++) {
        if (_DSProc->datastreams[ds_id]->role == DSR_OUTPUT) {
            dsproc_set_datastream_format(ds_id, format);
        }
    }

    return(1);
}

/**
 *  Unset the control flags for a datastream.
 *
 *  See dsproc_set_datastream_flags() for flags and descriptions.
 *
 *  @param  ds_id - datastream ID
 *  @param  flags - flags to unset
 *
 *  @see dsproc_set_datastream_flags()
 */
void dsproc_unset_datastream_flags(int ds_id, int flags)
{
    DataStream *ds = _DSProc->datastreams[ds_id];

    if (msngr_debug_level || msngr_provenance_level) {

        DEBUG_LV1( DSPROC_LIB_NAME,
            "%s: Unsetting %s datastream control flags\n",
            ds->name, _dsproc_dsrole_to_name(ds->role));

        if (flags & DS_OVERLAP_CHECK) {
            DEBUG_LV1( DSPROC_LIB_NAME, " - DS_OVERLAP_CHECK\n");
        }

        if (flags & DS_STANDARD_QC) {
            DEBUG_LV1( DSPROC_LIB_NAME, " - DS_STANDARD_QC\n");
        }

        if (flags & DS_PRESERVE_OBS) {
            DEBUG_LV1( DSPROC_LIB_NAME, " - DS_PRESERVE_OBS\n");
        }
    }

    ds->flags &= (0xffff ^ flags);
}

/**
 *  Update datastream data stats.
 *
 *  All datastream data stats collected by this function will be appended
 *  to the log file before it is closed.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  ds_id       - datastream ID
 *  @param  num_records - number of data records processed
 *  @param  begin_time  - time of the first record processed
 *  @param  end_time    - time of the last record processed
 */
void dsproc_update_datastream_data_stats(
    int              ds_id,
    int              num_records,
    const timeval_t *begin_time,
    const timeval_t *end_time)
{
    DataStream *ds = _DSProc->datastreams[ds_id];

    if (msngr_debug_level || msngr_provenance_level) {

        char ts1[32], ts2[32];

        if (begin_time && begin_time->tv_sec) {
            format_timeval(begin_time, ts1);
        }
        else {
            strcpy(ts1, "none");
        }

        if (end_time && end_time->tv_sec) {
            format_timeval(end_time, ts2);
        }
        else {
            strcpy(ts2, "none");
        }

        DEBUG_LV1( DSPROC_LIB_NAME,
            "%s: Updating %s datastream data stats\n"
            " - num records: %d\n"
            " - begin time:  %s\n"
            " - end time:    %s\n",
            ds->name, _dsproc_dsrole_to_name(ds->role),
            num_records, ts1, ts2);
    }

    /* Update the datastream data stats */

    ds->total_records += num_records;

    if (begin_time && begin_time->tv_sec) {
        if (!ds->begin_time.tv_sec ||
            TV_LT(*begin_time, ds->begin_time)) {

            ds->begin_time = *begin_time;
        }
    }

    if (end_time && end_time->tv_sec) {
        if (TV_LT(ds->end_time, *end_time)) {
            ds->end_time = *end_time;
        }
    }
}

/**
 *  Update datastream file stats.
 *
 *  All datastream file stats collected by this function will be appended
 *  to the log file before it is closed.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  ds_id       - datastream ID
 *  @param  file_size   - size of the file in bytes
 *  @param  begin_time  - time of the first record processed
 *  @param  end_time    - time of the last record processed
 */
void dsproc_update_datastream_file_stats(
    int              ds_id,
    double           file_size,
    const timeval_t *begin_time,
    const timeval_t *end_time)
{
    DataStream *ds = _DSProc->datastreams[ds_id];

    if (msngr_debug_level || msngr_provenance_level) {

        char ts1[32], ts2[32];

        if (begin_time && begin_time->tv_sec) {
            format_timeval(begin_time, ts1);
        }
        else {
            strcpy(ts1, "none");
        }

        if (end_time && end_time->tv_sec) {
            format_timeval(end_time, ts2);
        }
        else {
            strcpy(ts2, "none");
        }

        DEBUG_LV1( DSPROC_LIB_NAME,
            "%s: Updating %s datastream file stats\n"
            " - file size:  %g bytes\n"
            " - begin time: %s\n"
            " - end time:   %s\n",
            ds->name, _dsproc_dsrole_to_name(ds->role),
            file_size, ts1, ts2);
    }

    /* Update the datastream file stats */

    ds->total_files++;
    ds->total_bytes += file_size;

    if (begin_time && begin_time->tv_sec) {
        if (!ds->begin_time.tv_sec ||
            TV_LT(*begin_time, ds->begin_time)) {

            ds->begin_time = *begin_time;
        }
    }

    if (end_time && end_time->tv_sec) {
        if (TV_LT(ds->end_time, *end_time)) {
            ds->end_time = *end_time;
        }
    }
}

/**
 *  Validate datastream data time.
 *
 *  This function will verify that the specified time is greater than or
 *  equal to the minimum valid time and is not in the future. If the time
 *  is in the future, the auto disable message will be set causing the
 *  process to disable itself when it finishes.
 *
 *  This function will also verify that the time is greater than that of the
 *  latest data processed. This check will be skipped if the DS_OVERLAP_CHECK
 *  flag is 0, reprocessing mode is enabled, or asynchronous processing mode
 *  is enabled.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  ds_id     - datastream ID
 *  @param  data_time - data time to check
 *
 *  @return
 *    - 1 if the data time is valid
 *    - 0 if the data time is not valid
 */
int dsproc_validate_datastream_data_time(
    int              ds_id,
    const timeval_t *data_time)
{
    DataStream *ds  = _DSProc->datastreams[ds_id];
    time_t      now = time(NULL);
    int         overlap_check;
    char        ts1[32], ts2[32];

    /* Make sure the time is greater than the minimum valid time */

    if (data_time->tv_sec < _DSProc->min_valid_time) {

        format_timeval(data_time, ts1);
        format_secs1970(_DSProc->min_valid_time, ts2);

        ERROR( DSPROC_LIB_NAME,
            "Invalid data time '%s' for datastream: %s\n"
            " -> data time is earlier than the minimum valid time: %s\n",
            ts1, ds->name, ts2);

        dsproc_set_status(DSPROC_EMINTIME);
        return(0);
    }

    /* Make sure the time is not in the future */

    if (data_time->tv_sec > now) {

        format_timeval(data_time, ts1);
        format_secs1970(now, ts2);

        ERROR( DSPROC_LIB_NAME,
            "Invalid data time '%s' for datastream: %s\n"
            " -> data time is in the future (current time: %s)\n",
            ts1, ds->name, ts2);

        dsproc_disable(DSPROC_EFUTURETIME);
        return(0);
    }

    /* Determine if the overlap check should be performed */

    if (dsproc_get_reprocessing_mode() ||
        dsproc_get_asynchrounous_mode()) {

        overlap_check = 0;
    }
    else {
        overlap_check = ds->flags & DS_OVERLAP_CHECK;
    }

    if (overlap_check) {

        if (TV_LTEQ(*data_time, ds->ppdt_end)) {

            format_timeval(data_time, ts1);
            format_timeval(&ds->ppdt_end, ts2);

            ERROR( DSPROC_LIB_NAME,
                "Invalid data time '%s' for datastream: %s\n"
                " -> less than or equal to the latest processed data time: %s\n",
                ts1, ds->name, ts2);

            dsproc_set_status(DSPROC_ETIMEOVERLAP);
            return(0);
        }
    }

    return(1);
}

/*******************************************************************************
 *  Public Functions
 */

/**
 *  Get the ID of a datastream.
 *
 *  @param  site      - site name, or NULL to find first match
 *  @param  facility  - facility name, or NULL to find first match
 *  @param  dsc_name  - datastream class name
 *  @param  dsc_level - datastream class level
 *  @param  role      - specifies input or output datastream
 *
 *  @return
 *    - datastream ID
 *    - -1 if the datastream has not beed defined
 */
int dsproc_get_datastream_id(
    const char *site,
    const char *facility,
    const char *dsc_name,
    const char *dsc_level,
    DSRole      role)
{
    DataStream *ds;
    int         ds_id;

    for (ds_id = 0; ds_id < _DSProc->ndatastreams; ds_id++) {

        ds = _DSProc->datastreams[ds_id];

        if (site && (strcmp(ds->site, site) != 0)) {
            continue;
        }

        if (facility && (strcmp(ds->facility, facility) != 0)) {
            continue;
        }

        if ((strcmp(ds->dsc_name,  dsc_name)  == 0) &&
            (strcmp(ds->dsc_level, dsc_level) == 0) &&
            (ds->role == role)) {

            return(ds_id);
        }
    }

    return(-1);
}

/**
 *  Get the ID of an input datastream.
 *
 *  This function will generate an error if the specified datastream class
 *  has not been defined in the database as an input for this process.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  dsc_name  - datastream class name
 *  @param  dsc_level - datastream class level
 *
 *  @return
 *    - datastream ID
 *    - -1 if an error occurs
 */
int dsproc_get_input_datastream_id(
    const char *dsc_name,
    const char *dsc_level)
{
    int ds_id;

    ds_id = dsproc_get_datastream_id(
        NULL, NULL, dsc_name, dsc_level, DSR_INPUT);

    if (ds_id < 0) {

        ERROR( DSPROC_LIB_NAME,
            "Invalid input datastream class: %s.%s\n",
            dsc_name, dsc_level);

        dsproc_set_status(DSPROC_EBADINDSC);
    }

    return(ds_id);
}

/**
 *  Get the IDs of all input datastreams.
 *
 *  This function will return an array of all input datastream ids. The
 *  memory used by the returned array is dynamically allocated and must
 *  be freed by the calling process.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  ids  - output: pointer to a dymanically allocated array
 *                         of input datastream ids
 *
 *  @return
 *    - number of input datastreams
 *    - -1 if a memory allocation error occurs
 */
int dsproc_get_input_datastream_ids(int **ids)
{
    int ds_id;
    int nids;

    *ids = malloc(_DSProc->ndatastreams * sizeof(int));

    if (!(*ids)) {

        ERROR( DSPROC_LIB_NAME,
            "Could not get array of output datastream IDs\n"
            " -> memory allocation error\n");

        dsproc_set_status(DSPROC_ENOMEM);
        return(-1);
    }

    nids = 0;

    for (ds_id = 0; ds_id < _DSProc->ndatastreams; ds_id++) {

        if (_DSProc->datastreams[ds_id]->role == DSR_INPUT) {
            (*ids)[nids] = ds_id;
            nids++;
        }
    }

    return(nids);
}

/**
 *  Get the ID of an output datastream.
 *
 *  This function will generate an error if the specified datastream class
 *  has not been defined in the database as an output for this process.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  dsc_name  - datastream class name
 *  @param  dsc_level - datastream class level
 *
 *  @return
 *    - datastream ID
 *    - -1 if an error occurs
 */
int dsproc_get_output_datastream_id(
    const char *dsc_name,
    const char *dsc_level)
{
    int ds_id;

    ds_id = dsproc_get_datastream_id(
        NULL, NULL, dsc_name, dsc_level, DSR_OUTPUT);

    if (ds_id < 0) {

        ERROR( DSPROC_LIB_NAME,
            "Invalid output datastream class: %s.%s\n",
            dsc_name, dsc_level);

        dsproc_set_status(DSPROC_EBADOUTDSC);
    }

    return(ds_id);
}

/**
 *  Get the IDs of all output datastreams.
 *
 *  This function will return an array of all output datastream ids. The
 *  memory used by the returned array is dynamically allocated and must
 *  be freed by the calling process.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  ids  - output: pointer to a dymanically allocated array
 *                         of output datastream ids
 *
 *  @return
 *    - number of output datastreams
 *    - -1 if a memory allocation error occurs
 */
int dsproc_get_output_datastream_ids(int **ids)
{
    int ds_id;
    int nids;

    *ids = malloc(_DSProc->ndatastreams * sizeof(int));

    if (!(*ids)) {

        ERROR( DSPROC_LIB_NAME,
            "Could not get array of output datastream IDs\n"
            " -> memory allocation error\n");

        dsproc_set_status(DSPROC_ENOMEM);
        return(-1);
    }

    nids = 0;

    for (ds_id = 0; ds_id < _DSProc->ndatastreams; ds_id++) {

        if (_DSProc->datastreams[ds_id]->role == DSR_OUTPUT) {
            (*ids)[nids] = ds_id;
            nids++;
        }
    }

    return(nids);
}

/**
 *  Returns the fully qualified datastream name.
 *
 *  The returned datastream name belongs to the internal datastream
 *  structure and must not be freed or altered by the calling process.
 *
 *  @param  ds_id - datastream ID
 *
 *  @return
 *    - pointer to the datastream name
 *    - NULL if the datastream ID is not valid
 */
const char *dsproc_datastream_name(int ds_id)
{
    if ((ds_id < 0) ||
        (ds_id >= _DSProc->ndatastreams)) {

        return((const char *)NULL);
    }

    return((const char *)_DSProc->datastreams[ds_id]->name);
}

/**
 *  Returns the datastream class name.
 *
 *  The returned datastream class name belongs to the internal datastream
 *  structure and must not be freed or altered by the calling process.
 *
 *  @param  ds_id - datastream ID
 *
 *  @return
 *    - pointer to the datastream class name
 *    - NULL if the datastream ID is not valid
 */
const char *dsproc_datastream_class_name(int ds_id)
{
    if ((ds_id < 0) ||
        (ds_id >= _DSProc->ndatastreams)) {

        return((const char *)NULL);
    }

    return((const char *)_DSProc->datastreams[ds_id]->dsc_name);
}

/**
 *  Returns the datastream class level.
 *
 *  The returned datastream class level belongs to the internal datastream
 *  structure and must not be freed or altered by the calling process.
 *
 *  @param  ds_id - datastream ID
 *
 *  @return
 *    - pointer to the datastream class level
 *    - NULL if the datastream ID is not valid
 */
const char *dsproc_datastream_class_level(int ds_id)
{
    if ((ds_id < 0) ||
        (ds_id >= _DSProc->ndatastreams)) {

        return((const char *)NULL);
    }

    return((const char *)_DSProc->datastreams[ds_id]->dsc_level);
}

/**
 *  Returns the datastream site.
 *
 *  The returned site code belongs to the internal datastream
 *  structure and must not be freed or altered by the calling process.
 *
 *  @param  ds_id - datastream ID
 *
 *  @return
 *    - pointer to the datastream site code
 *    - NULL if the datastream ID is not valid
 */
const char *dsproc_datastream_site(int ds_id)
{
    if ((ds_id < 0) ||
        (ds_id >= _DSProc->ndatastreams)) {

        return((const char *)NULL);
    }

    return((const char *)_DSProc->datastreams[ds_id]->site);
}

/**
 *  Returns the datastream facility.
 *
 *  The returned facility code belongs to the internal datastream
 *  structure and must not be freed or altered by the calling process.
 *
 *  @param  ds_id - datastream ID
 *
 *  @return
 *    - pointer to the datastream facility code
 *    - NULL if the datastream ID is not valid
 */
const char *dsproc_datastream_facility(int ds_id)
{
    if ((ds_id < 0) ||
        (ds_id >= _DSProc->ndatastreams)) {

        return((const char *)NULL);
    }

    return((const char *)_DSProc->datastreams[ds_id]->facility);
}

/**
 *  Returns the path to the datastream directory.
 *
 *  The returned path belongs to the internal datastream structure
 *  and must not be freed or altered by the calling process.
 *
 *  @param  ds_id - datastream ID
 *
 *  @return
 *    - pointer to the path to the datastream directory
 *    - NULL if the datastream path has not been set
 */
const char *dsproc_datastream_path(int ds_id)
{
    DataStream *ds;

    if ((ds_id < 0) ||
        (ds_id >= _DSProc->ndatastreams)) {

        return((const char *)NULL);
    }

    ds = _DSProc->datastreams[ds_id];

    if (!ds->dir || !ds->dir->path) {
        return((const char *)NULL);
    }

    return((const char *)ds->dir->path);
}

/**
 *  Set the file splitting mode for output files.
 *
 *  Default for VAPs: always create a new file when data is stored
 *
 *    - split_mode     = SPLIT_ON_STORE
 *    - split_start    = ignored
 *    - split_interval = ignored
 *
 *  Default for Ingests: daily files that split at midnight
 *
 *    - split_mode     = SPLIT_ON_HOURS
 *    - split_start    = 0
 *    - split_interval = 24
 *
 *  @param  ds_id          - datastream ID
 *  @param  split_mode     - the file splitting mode (see SplitMode)
 *  @param  split_start    - the start of the split interval (see SplitMode)
 *  @param  split_interval - the split interval (see SplitMode)
 */
void dsproc_set_datastream_split_mode(
    int       ds_id,
    SplitMode split_mode,
    double    split_start,
    double    split_interval)
{
    DataStream *ds = _DSProc->datastreams[ds_id];

    if (msngr_debug_level || msngr_provenance_level) {

        switch (split_mode) {

            case SPLIT_ON_STORE:

                DEBUG_LV1( DSPROC_LIB_NAME,
                    "%s: Setting datastream file splitting mode:\n"
                    " -> always create a new file when data is stored\n",
                    ds->name);

                break;

            case SPLIT_ON_HOURS:

                DEBUG_LV1( DSPROC_LIB_NAME,
                    "%s: Setting datastream file splitting mode:\n"
                    "  - split_start:    hour %g\n"
                    "  - split_interval: %g hours\n",
                    ds->name, split_start, split_interval);

                break;

            case SPLIT_ON_DAYS:

                DEBUG_LV1( DSPROC_LIB_NAME,
                    "%s: Setting datastream file splitting mode:\n"
                    "  - split_start:    day %g\n"
                    "  - split_interval: %g days\n",
                    ds->name, split_start, split_interval);

                break;

            case SPLIT_ON_MONTHS:

                DEBUG_LV1( DSPROC_LIB_NAME,
                    "%s: Setting datastream file splitting mode:\n"
                    "  - split_start:    month %g\n"
                    "  - split_interval: %g months\n",
                    ds->name, split_start, split_interval);

                break;

            case SPLIT_NONE:

                DEBUG_LV1( DSPROC_LIB_NAME,
                    "%s: Setting datastream file splitting mode:\n"
                    " -> always append output to the previous file unless otherwise\n"
                    " -> specified in the call to dsproc_store_dataset.\n",
                    ds->name);

                break;

            default:

                DEBUG_LV1( DSPROC_LIB_NAME,
                    "%s: Invalid file splitting mode: %d\n",
                    ds->name, split_mode);
        }
    }

    ds->split_mode     = split_mode;
    ds->split_start    = split_start;
    ds->split_interval = split_interval;
}

/**
 *  Set the timezone offset to use when splitting files.
 *
 *  Note that this should be the timezone offset for loaction of the data
 *  being processed and is subtracted from the UTC time when determining
 *  the time of the next file split.  For example, If a timezone offset
 *  of -6 hours is set for SGP data, the files will split at 6:00 a.m. GMT.
 *
 *  @param  ds_id           - datastream ID
 *  @param  split_tz_offset - time zone offset (in hours)
 */
void dsproc_set_datastream_split_tz_offset(int ds_id, int split_tz_offset)
{
    DataStream *ds = _DSProc->datastreams[ds_id];

    DEBUG_LV1( DSPROC_LIB_NAME,
        "%s: Setting timezone offset for file splitting to: %d hours\n",
        ds->name, split_tz_offset);

    ds->split_tz_offset = split_tz_offset;
}

/**
 *  Set the default NetCDF file extension to 'nc' for output files.
 *
 *  The NetCDF file extension used by ARM has historically been "cdf".
 *  This function can be used to change it to the new prefered extension
 *  of "nc", and must be called *before* calling dsproc_main().
 */
void dsproc_use_nc_extension(void)
{
    gNetcdfExtension = gNCExtension;
}
