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

/** @file dsproc_datastream_dod.c
 *  Datastream DOD Functions.
 */

#include "dsproc3.h"
#include "dsproc_private.h"

extern DSProc *_DSProc; /**< Internal DSProc structure */

/** @privatesection */

/*******************************************************************************
 *  Static Data Visible Only To This Module
 */

static char *COLON = " : ";

/*******************************************************************************
 *  Static Functions Visible Only To This Module
 */

#if 0
/**
 *  Static: Example showing how to get input file information.
 */
static void dsproc_print_retrieved_files_info()
{
    DataStream *datastream;
    RetDsCache *ret_cache;
    RetDsFile  *ret_file;
    int         dsid;
    int         fi;
    char        ts1[32], ts2[32];

    for (dsid = 0; dsid < _DSProc->ndatastreams; dsid++) {

        datastream = _DSProc->datastreams[dsid];
        ret_cache  = datastream->ret_cache;

        if (!ret_cache) {
            continue;
        }

        printf("Input DataStream: %s\n", datastream->name);

        if (ret_cache->nfiles <= 0) {
            printf(" - no data retrieved from this datastream\n");
            continue;
        }

        for (fi = 0; fi < ret_cache->nfiles; fi++) {

            ret_file = ret_cache->files[fi];

            format_timeval(&(ret_file->start_time), ts1);
            format_timeval(&(ret_file->end_time), ts2);

            printf(" - %s\n", ret_file->name);
            printf("    - version string:    '%s'\n", ret_file->version_string);
            printf("    - first record time: '%s'\n", ts1);
            printf("    - last  record time: '%s'\n", ts2);
            printf("    - number of records: '%d'\n", (int)ret_file->sample_count);
        }
    }
}
#endif

/**
 *  Static: Verify that an attribute has the correct value.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  att    - pointer to the attribute
 *  @param  type   - data type of the correct value
 *  @param  length - length of the correct value
 *  @param  value  - pointer to the correct value
 *
 *  @return
 *    - 1 if verified
 *    - 0 if the attribute value does not match the correct value
 */
static int _dsproc_verify_att_value(
    CDSAtt      *att,
    CDSDataType  type,
    size_t       length,
    void        *value)
{
    size_t  nbytes = length * cds_data_type_size(type);
    char   *att_value;
    char   *correct_value;
    size_t  nchars;

    /* Check if the attribute has the required value */

    if (att->length == length &&
        att->type   == type   &&
        memcmp(att->value.vp, value, nbytes) == 0) {

        return(1);
    }

    /* Generate the error message */

    att_value = cds_sprint_array(
        att->type, att->length, att->value.vp,
        &nchars, NULL, NULL, 0, 0, 0x02 | 0x10);

    correct_value = cds_sprint_array(
        type, length, value,
        &nchars, NULL, NULL, 0, 0, 0x02 | 0x10);

    if (!att_value || !correct_value) {

        ERROR( DSPROC_LIB_NAME,
            "Invalid global attribute value for: %s\n"
            " -> memory allocation error generating error message\n",
            cds_get_object_path(att));
    }
    else {

        ERROR( DSPROC_LIB_NAME,
            "Invalid global attribute value found for: %s\n"
            " - found value:    %s\n"
            " - expected value: %s\n",
            cds_get_object_path(att),
            att_value,
            correct_value);
    }

    if (att_value)     free(att_value);
    if (correct_value) free(correct_value);

    dsproc_set_status(DSPROC_EGLOBALATT);
    return(0);
}

/**
 *  Static: Set a runtime attribute value.
 *
 *  This function will set an attribute value in a dataset if the attribute
 *  exists and the definition lock is not set.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  dataset    - pointer to the dataset
 *  @param  var_name   - variable name (NULL for global attribute)
 *  @param  att_name   - attribute name
 *  @param  verify     - verify that the attribute value is correct
 *                       if it has already been set.
 *  @param  depricated - specifies if the attribute has been depricated
 *                       and should not be created in dynamic DOD mode.
 *  @param  type       - data type of the specified value
 *  @param  length     - length of the specified value
 *  @param  value      - pointer to the attribute value
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
static int _dsproc_set_runtime_att_value(
    CDSGroup    *dataset,
    const char  *var_name,
    const char  *att_name,
    int          verify,
    int          depricated,
    CDSDataType  type,
    size_t       length,
    void        *value)
{
    CDSAtt *att;
    void   *parent;

    if (var_name) {
        parent = (void *)cds_get_var(dataset, var_name);
    }
    else {
        parent = (void *)dataset;
    }

    if (parent) {

        att = cds_get_att(parent, att_name);

        if (att) {

            if (!att->length || !att->value.vp) {
                att->def_lock = 0;
            }

            if (!att->def_lock) {
                if (!cds_set_att_value(att, type, length, value)) {
                    dsproc_set_status(DSPROC_ENOMEM);
                    return(0);
                }
            }
            else if (verify) {
                if (!_dsproc_verify_att_value(att, type, length, value)) {
                    return(0);
                }
            }
        }
        else if (!depricated && dsproc_get_dynamic_dods_mode()) {

            att = cds_set_att(parent, 1, att_name, type, length, value);
            if (!att) {
                dsproc_set_status(DSPROC_ENOMEM);
                return(0);
            }
        }
    }

    return(1);
}

/**
 *  Static: Set a runtime attribute value.
 *
 *  This function will set an attribute value in a dataset if the attribute
 *  exists and the definition lock is not set.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  dataset    - pointer to the dataset
 *  @param  var_name   - variable name (NULL for global attribute)
 *  @param  att_name   - attribute name
 *  @param  verify     - verify that the attribute value is correct
 *                       if it has already been set.
 *  @param  depricated - specifies if the attribute has been depricated
 *                       and should not be created in dynamic DOD mode.
 *  @param  format     - attribute value format string (see printf)
 *  @param  ...        - arguments for the format string
 *
 *  @return
 *    - 1 if successful
 *    - 0 if a memory allocation error occurred
 */
static int _dsproc_set_runtime_att_text(
    CDSGroup   *dataset,
    const char *var_name,
    const char *att_name,
    int         verify,
    int         depricated,
    const char *format, ...)
{
    va_list  args;
    char    *string;
    size_t   length;
    int      status;

    va_start(args, format);

    string = msngr_format_va_list(format, args);
    if (!string) {

        ERROR( DSPROC_LIB_NAME,
            "Could not set runtime attribute value in dataset: %s\n"
            " -> memory allocation error\n",
            dataset->name);

        dsproc_set_status(DSPROC_ENOMEM);
        return(0);
    }

    length = strlen(string) + 1;
    status = _dsproc_set_runtime_att_value(
        dataset, var_name, att_name,
        verify, depricated,
        CDS_CHAR, length, (void *)string);

    va_end(args);

    free(string);

    return(status);
}

/**
 *  Static: Extract the version number from a version string.
 *
 *  This function will parse the various version string formats known to exist
 *  in ARM data files.  When available the package version will be returned.
 *  For older files that do not contain a package version the RCS ID will be
 *  returned. This function will also check for version strings of the form
 *  "Release_#_#".
 *
 *  @param  version_string  pointer to the version string
 *  @param  major           output: major version number
 *  @param  minor           output: minor version number
 *
 *  @return
 *    - 1 if a valid version number was found
 *    - 0 if a valid version number was not found
 */
static int _dsproc_parse_version_string(
    const char *version_string,
    int        *major,
    int        *minor)
{
    const char *string = version_string;
    const char *strp   = string;
    int         nfound = 0;
    int         micro;

    if (major) *major = 0;
    if (minor) *minor = 0;

    if (!version_string) return(0);

    /* Check for and skip the RCS ID */

    strp = strstr(version_string, ",v ");
    if (strp) {
        strp = strchr(strp, ' ');
        while(*strp == ' ') strp++;
        strp = strchr(strp, ' ');
        if (strp) {
            string = strp;
        }
    }

    nfound = parse_version_string(string, major, minor, &micro);

    if (nfound >= 2) return(1);

    /* Use the RCS ID if one was found */

    if (string != version_string) {
        nfound = parse_version_string(version_string, major, minor, &micro);
        if (nfound >= 2) return(1);
    }

    /* Check for a version string of the form Release_#_# */

    strp = strstr(version_string, "Release_");
    if (strp) {
        nfound = sscanf(strp, "Release_%d_%d", major, minor);
        if (nfound == 2) return(1);
    }

    return(0);
}

/**
 *  Static: Set values of the input_datastreams_num and input_datastreams global atts.
 *
 *  @param  dataset - pointer to the dataset
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
static int _dsproc_set_input_datastreams(CDSGroup *dataset)
{
    int dsid,fi,total_num_input_ds = 0;
    char *word;
    int   status, ds_count=0;
    char  buf[256];
    /* BDE: the "file_string" should be made to dynamically grow as necessary,
     * but for now we just set it really large. */
    char file_string[8192];
    char obs_version[64];
    char date[16], last_date[16], date_added[16];
    char last_obs_version[64];
    DataStream *datastream;
    RetDsCache *ret_cache;
    RetDsFile  *ret_file;
    DSFile     *dsfile;
    int         major;
    int         minor;
    int         length;

    /* Check if the input_datastreams attribute exists */

    if (!cds_get_att(dataset, "input_datastreams") &&
        !dsproc_get_dynamic_dods_mode()) {

        return(1);
    }

    /* Loop across each distinct input datastream and note total num observations*/

    strcpy(file_string, "N/A");

    for (dsid = 0; dsid < _DSProc->ndatastreams; dsid++) {

        datastream = _DSProc->datastreams[dsid];
        ret_cache  = datastream->ret_cache;

        if (!ret_cache ||
            ret_cache->nfiles <= 0) {

            continue;
        }

        /* Add each input file to total number of observations */
        total_num_input_ds = total_num_input_ds + ret_cache->nfiles;

        if (ds_count++ == 0) strcpy(file_string,datastream->name);
        else strcat(file_string, datastream->name);
        strcat(file_string, COLON);
        strcpy(last_obs_version,"");
        strcpy(date,"");

        for (fi = 0; fi < ret_cache->nfiles; fi++) {

            ret_file = ret_cache->files[fi];
            dsfile   = ret_file->dsfile;

            /* get the version number of the process that created this file */

            if (_dsproc_parse_version_string(
                ret_file->version_string, &major, &minor)) {

                sprintf(obs_version, "%d.%d", major, minor);
            }
            else {
                strcpy(obs_version, "Unknown");
            }

            /* Get date of file */
            strcpy(buf, dsfile->name);
            word = strtok(buf, ".");
            word = strtok(NULL, ".");
            word = strtok(NULL, ".");
            strcpy(date, word);
            strcat(date, ".");
            word = strtok(NULL, ".");
            strcat(date,word);

            if(fi == 0) {
                /* if first obs then build from here */
                strcat(file_string, obs_version);
                strcat(file_string,COLON);
                strcat(file_string,date);
                strcpy(date_added, date);
                /* If this is only obs and not the
                   last datastream then add return*/
                if((fi == ret_cache->nfiles-1) &&
                    (dsid < _DSProc->ndatastreams-1) )
                    strcat(file_string, "\n");
            } /* end else not fi != 0 */
            else {
                /* If version didn't change and its the last file */
                /* finish date */
                /* If not the last datastream add a return */
                if(!strcmp(obs_version,last_obs_version) &&
                      (fi == ret_cache->nfiles-1) ) {
                    strcat(file_string, "-");
                    strcat(file_string,date);
                    if(dsid < _DSProc->ndatastreams-1)
                        strcat(file_string, "\n");
                }
                /* if version did change finish last entry */
                /* and add new entry */
                else if(strcmp(obs_version,last_obs_version)){
                    if(strcmp(date_added,last_date) ) {
                        strcat(file_string, "-");
                        strcat(file_string,last_date);
                    }
                    strcat(file_string, "\n");
                    strcat(file_string, datastream->name);
                    strcat(file_string,COLON);
                    strcat(file_string,obs_version);
                    strcat(file_string,COLON);
                    strcat(file_string,date);
                    strcpy(date_added, date);

                    /* if last obs and if not */
                    /* the last datastream add a return */
                    if( (fi == ret_cache->nfiles-1) &&
                        (dsid < _DSProc->ndatastreams-1) )
                        strcat(file_string, "\n");
                }
            } /* fi != 0 */
            /* Save this version and date */
            strcpy(last_obs_version,obs_version);
            strcpy(last_date,date);

        } /* end fi < ret_cache->nfiles */

        /* finish current entry */
    } /* end dsid < _DSProc->ndatastreams */

    /* Strip the extra new line from the file_string if one exists. */

    length = strlen(file_string);
    if (file_string[length-1] == '\n') file_string[length-1] = '\0';

    /* Store the attribute values. */

    status = _dsproc_set_runtime_att_value(dataset, NULL,
        "input_datastreams_num", 1, 1, CDS_INT,1,(void *)&total_num_input_ds);

    if (status != 0) {
        status = _dsproc_set_runtime_att_text(dataset, NULL,
            "input_datastreams", 1, 0, "%s", file_string);
    }

    return(status);
}

/**
 *  Static: Set history attribute value in a dataset.
 *
 *  @param  dataset - pointer to the dataset
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
static int _dsproc_set_history_att(CDSGroup *dataset)
{
    const char *host;
    const char *user;
    time_t      now;
    struct tm   now_tm;
    int         status;

    memset(&now_tm, 0, sizeof(struct tm));

    host = dsenv_get_hostname();
    if (!host) {
        host = "unknown";
    }

    user = getenv("USER");
    if (!user) {
        user = "unknown";
    }

    now = time(NULL);

    if (!gmtime_r(&now, &now_tm)) {

        ERROR( DSPROC_LIB_NAME,
            "Could not set history attribute for: %s\n"
            " -> gmtime error: %s\n",
            dataset->name, strerror(errno));

        dsproc_set_status(DSPROC_ETIMECALC);
        return(0);
    }

    status = _dsproc_set_runtime_att_text(dataset, NULL,
        "history", 0, 0,
        "created by user %s on machine %s at %d-%02d-%02d %02d:%02d:%02d, using %s",
        user, host,
        now_tm.tm_year+1900, now_tm.tm_mon+1, now_tm.tm_mday,
        now_tm.tm_hour,      now_tm.tm_min,   now_tm.tm_sec,
        _DSProc->version);

    return(status);
}

/**
 *  Static: Update datastream DOD.
 *
 *  This function will update a datastream DOD if there has been a DOD
 *  version change or any time varying attribute value changes.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  ds        - pointer to the DataStream structure
 *  @param  data_time - the time used to determine the DOD version and
 *                      set the time varying attribute values
 *
 *  @return
 *    -  1 if the DSDOD was updated
 *    -  0 if no updates were needed
 *    - -1 if an error occurred
 */
static int _dsproc_update_dsdod(DataStream *ds, time_t data_time)
{
    const char *version_update;
    int         atts_update;
    char        time_string[32];
    int         status;

    atts_update = 0;

    version_update = dsdb_check_for_dsdod_version_update(ds->dsdod, data_time);
    if (!version_update) {
        atts_update = dsdb_check_for_dsdod_time_atts_update(ds->dsdod, data_time);
    }

    if (version_update || atts_update) {

        /* Make sure we are connected to the database */

        if (!dsproc_db_connect()) {
            return(-1);
        }

        /* Update the DSDOD */

        if (msngr_debug_level || msngr_provenance_level) {

            format_secs1970(data_time, time_string);

            DEBUG_LV1( DSPROC_LIB_NAME,
                "%s: Updating datastream DOD for data time: %s\n",
                ds->name, time_string);
        }

        status = dsdb_update_dsdod(_DSProc->dsdb, ds->dsdod, data_time);

        if (status < 0) {

            ERROR( DSPROC_LIB_NAME,
                "Could not update datastream DOD for: %s\n"
                "-> database query error\n",
                ds->name);

            dsproc_set_status(DSPROC_EDBERROR);
            dsproc_db_disconnect();
            return(-1);
        }

        if (msngr_debug_level || msngr_provenance_level) {

            if (version_update) {
                DEBUG_LV1( DSPROC_LIB_NAME,
                    " - updated DOD version to: %s\n", ds->dsdod->version);
            }

            if (atts_update) {
                DEBUG_LV1( DSPROC_LIB_NAME,
                    " - updated time varying attribute values\n");
            }
        }

        dsproc_db_disconnect();

        return(1);
    }

    return(0);
}

/*******************************************************************************
 *  Private Functions Visible Only To This Library
 */

/**
 *  Private: Get datastream DOD.
 *
 *  If the DOD for this datastream has already been loaded, it will be
 *  updated using the specified data time. If the data time is not
 *  specified, the existing datastream DOD will be unchanged.
 *
 *  If the DOD for this datastream has not already been loaded and the
 *  data time is not specified, the current time will be used. If the
 *  data time is less than the time of the earliest DOD version, the
 *  earliest DOD version will be used.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  ds        - pointer to the DataStream structure
 *  @param  data_time - the time used to determine the DOD version and
 *                      set the time varying attribute values
 *
 *  @return
 *    -  1 if successful
 *    -  0 if the requested DSDOD was not found
 *    - -1 if an error occurred
 */
int _dsproc_get_dsdod(DataStream *ds, time_t data_time)
{
    char time_string[32];
    int  status;

    /* Check if the DSDOD has already been loaded */

    if (ds->dsdod) {

        if (data_time) {
            if (_dsproc_update_dsdod(ds, data_time) < 0) {
                return(-1);
            }
        }

        return(1);
    }

    /* Set the data_time to now if it was not specified */

    if (!data_time) {
        data_time = time(NULL);
    }

    /* Make sure we are connected to the database */

    if (!dsproc_db_connect()) {
        return(-1);
    }

    /* Load the DSDOD from the database */

    if (msngr_debug_level || msngr_provenance_level) {

        format_secs1970(data_time, time_string);

        DEBUG_LV1( DSPROC_LIB_NAME,
            "%s: Getting datastream DOD from database\n"
            " - data time:   %s\n",
            ds->name, time_string);
    }

    status = dsdb_get_dsdod(_DSProc->dsdb,
        ds->site, ds->facility, ds->dsc_name, ds->dsc_level, data_time,
        &(ds->dsdod));

    if (status <= 0) {

        if (status < 0) {

            ERROR( DSPROC_LIB_NAME,
                "Could not get datastream DOD for: %s\n"
                "-> database query error\n",
                ds->name);

            dsproc_set_status(DSPROC_EDBERROR);
        }
        else {
            DEBUG_LV1( DSPROC_LIB_NAME,
                " - DOD not defined in database\n");
        }

        dsproc_db_disconnect();
        return(status);
    }

    DEBUG_LV1( DSPROC_LIB_NAME,
        " - DOD version: %s\n",
        ds->dsdod->version);

    dsproc_db_disconnect();

    return(1);
}

/** @publicsection */

/*******************************************************************************
 *  Internal Functions Visible To The Public
 */

/**
 *  Get a datastream DOD attribute.
 *
 *  This function will first check the datastream DOD returned from the
 *  database for the specified attribute. If the attribute is not found
 *  or its value has not been defined, the datastream metadata set by the
 *  user will be checked.
 *
 *  @param  ds_id    - datastream ID
 *  @param  var_name - variable name (NULL for global attribute)
 *  @param  att_name - attribute name
 *
 *  @return
 *    - pointer to the CDSAtt structure
 *    - NULL if the attribute does not exist
 */
CDSAtt *dsproc_get_dsdod_att(
    int         ds_id,
    const char *var_name,
    const char *att_name)
{
    DataStream *ds = _DSProc->datastreams[ds_id];
    void       *parent;
    CDSAtt     *att;
    CDSAtt     *md_att;

    att = (CDSAtt *)NULL;

    /* Check the dsdod returned from the database first */

    if (ds->dsdod) {

        if (var_name) {
            parent = (void *)cds_get_var(ds->dsdod->cds_group, var_name);
        }
        else {
            parent = (void *)ds->dsdod->cds_group;
        }

        if (parent) {
            att = cds_get_att(parent, att_name);
        }
    }

    /* Check the runtime metadata if the attribute value isn’t defined */

    if (!att || !att->length || !att->value.vp) {

        if (ds->metadata) {

            if (var_name) {
                parent = (void *)cds_get_var(ds->metadata, var_name);
            }
            else {
                parent = (void *)ds->metadata;
            }

            if (parent) {

                md_att = cds_get_att(parent, att_name);

                if (md_att) {
                    att = md_att;
                }
            }
        }
    }

    return(att);
}

/**
 *  Get a datastream DOD dimension.
 *
 *  This function will first check the datastream DOD returned from the
 *  database for the specified dimension. If the dimension is not found
 *  or its length has not been set, the datastream metadata set by the
 *  user will be checked.
 *
 *  @param  ds_id    - datastream ID
 *  @param  dim_name - dimension name
 *
 *  @return
 *    - pointer to the CDSDim structure
 *    - NULL if the dimension does not exist
 */
CDSDim *dsproc_get_dsdod_dim(int ds_id, const char *dim_name)
{
    DataStream *ds = _DSProc->datastreams[ds_id];
    CDSDim     *dim;
    CDSDim     *md_dim;

    dim = (CDSDim *)NULL;

    /* Check the dsdod returned from the database first */

    if (ds->dsdod) {
        dim = cds_get_dim(ds->dsdod->cds_group, dim_name);
    }

    /* Check the runtime metadata if the dimension length isn’t set */

    if (!dim || (!dim->is_unlimited && !dim->length) ) {

        if (ds->metadata) {

            md_dim = cds_get_dim(ds->metadata, dim_name);

            if (md_dim) {
                dim = md_dim;
            }
        }
    }

    return(dim);
}

/**
 *  Set the runtime metadata for a datastream in the specified dataset.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  ds_id   - datastream ID
 *  @param  dataset - pointer to the dataset
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int dsproc_set_runtime_metadata(int ds_id, CDSGroup *dataset)
{
    DataStream *ds           = _DSProc->datastreams[ds_id];
    const char *command_line = _dsproc_get_command_line();
    const char *input_source = dsproc_get_input_source();
    int         dynamic_dod  = dsproc_get_dynamic_dods_mode();
    int         copy_flags;
    int         use_loc_desc;

    if (dynamic_dod) {
        copy_flags = 0;
    }
    else {
        copy_flags = CDS_EXCLUSIVE;
    }

    DEBUG_LV1( DSPROC_LIB_NAME,
        "%s: Setting runtime attribute values\n",
        ds->name);

    /* Set standard attribute values */

    if (command_line) {

        if (!_dsproc_set_runtime_att_text(dataset, NULL,
            "command_line", 1, 0, "%s", command_line)) {

            return(0);
        }
    }

    if (!_dsproc_set_runtime_att_text(dataset, NULL,
        "process_version", 1, 0, "%s", _DSProc->version)) {

        return(0);
    }

    // this attribute is deprecated, use process_version instead.
    if (!_dsproc_set_runtime_att_text(dataset, NULL,
        "ingest_software", 1, 1, "%s", _DSProc->version)) {

        return(0);
    }

    if (ds->dsdod) {
        if (!_dsproc_set_runtime_att_text(dataset, NULL,
            "dod_version", 1, 0, "%s-%s-%s",
            ds->dsdod->name, ds->dsdod->level, ds->dsdod->version)) {

            return(0);
        }
    }
    else {
        if (!_dsproc_set_runtime_att_text(dataset, NULL,
            "dod_version", 0, 0, "N/A")) {

            return(0);
        }
    }

    if (!_dsproc_set_input_datastreams(dataset)) {
       return(0);
    }

    if (input_source) {
        if (!_dsproc_set_runtime_att_text(dataset, NULL,
            "input_source",  1, 0, "%s", input_source)) {

            return(0);
        }
    }

    if (!_dsproc_set_runtime_att_text(dataset, NULL,
        "site_id", 1, 0, "%s", ds->site)) {

        return(0);
    }

    if (!_dsproc_set_runtime_att_text(dataset, NULL,
        "platform_id", 1, 0, "%s", ds->dsc_name)) {

        return(0);
    }

    if (dynamic_dod ||
        cds_get_att(dataset, "location_description")) {

        use_loc_desc = 1;
    }
    else {
        use_loc_desc = 0;
    }

    if (use_loc_desc) {

        // new standards

        if (!_dsproc_set_runtime_att_text(dataset, NULL,
            "facility_id", 1, 0, "%s", ds->facility)) {

            return(0);
        }
    }
    else {

        // old standards

        if (!_dsproc_set_runtime_att_text(dataset, NULL,
            "facility_id", 1, 0, "%s: %s",
            ds->facility, _DSProc->location->name)) {

            return(0);
        }
    }

    if (!_dsproc_set_runtime_att_text(dataset, NULL,
        "data_level", 1, 0, "%s", ds->dsc_level)) {

        return(0);
    }

    if (use_loc_desc) {

        if (!_dsproc_set_runtime_att_text(dataset, NULL,
            "location_description", 1, 0,
            "%s, %s",
            _DSProc->site_desc,
            _DSProc->location->name)) {

            return(0);
        }
    }

    if (!_dsproc_set_runtime_att_text(dataset, NULL,
        "datastream", 1, 0, "%s", ds->name)) {

        return(0);
    }

    /* Copy metadata set by the user into the dataset */

    if (ds->metadata) {
        if (!cds_copy_dims(ds->metadata, dataset, NULL, NULL, copy_flags) ||
            !cds_copy_atts(ds->metadata, dataset, NULL, NULL, copy_flags) ||
            !cds_copy_vars(ds->metadata, dataset,
                NULL, NULL, NULL, NULL, 0, 0, 0, copy_flags)) {

            ERROR( DSPROC_LIB_NAME,
                "Could not copy runtime metadata to dataset: %s\n"
                " -> error copying runtime metadata\n",
                ds->name);

            dsproc_set_status(DSPROC_ECDSCOPY);
            return(0);
        }
    }

    if (!_dsproc_set_history_att(dataset)) {
       return(0);
    }

    return(1);
}

/*******************************************************************************
 *  Public Functions
 */

/**
 *  Check if a datastream DOD needs to be updated.
 *
 *  This function will check if there has been a DSDOD version change or any
 *  time varying attribute value changes between the currently loaded DSDOD
 *  and the DSDOD for the specified data time.
 *
 *  @param  ds_id     - datastream ID
 *  @param  data_time - the data time
 *
 *  @return
 *    -  1 if the DSDOD needs to be updated
 *    -  0 if the DSDOD does not need to be updated
 */
int dsproc_check_for_dsdod_update(int ds_id, time_t data_time)
{
    DataStream *ds = _DSProc->datastreams[ds_id];
    const char *version_update;
    int         atts_update;

    if (!ds->dsdod) {
        return(1);
    }

    atts_update = 0;

    version_update = dsdb_check_for_dsdod_version_update(ds->dsdod, data_time);
    if (!version_update) {
        atts_update = dsdb_check_for_dsdod_time_atts_update(ds->dsdod, data_time);
    }

    if (version_update || atts_update) {
        return(1);
    }

    return(0);
}

/**
 *  Get a copy of a datastream DOD attribute value.
 *
 *  This function will get a copy of a datastream DOD attribute value casted
 *  into the specified data type. The datastream DOD returned from the database
 *  will be checked first for the specified attribute. If the attribute is not
 *  found or its value has not been defined, the datastream metadata set by the
 *  user will be checked. The functions cds_string_to_array() and
 *  cds_array_to_string() are used to convert between text (CDS_CHAR) and numeric
 *  data types.
 *
 *  Memory will be allocated for the returned array if the output array
 *  is NULL. In this case the calling process is responsible for freeing
 *  the allocated memory.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  ds_id    - datastream ID
 *  @param  var_name - variable name (NULL for global attribute)
 *  @param  att_name - attribute name
 *  @param  type     - data type of the output array
 *  @param  length   - pointer to the length of the output array
 *                       - input:
 *                           - length of the output array
 *                           - ignored if the output array is NULL
 *                       - output:
 *                           - number of values written to the output array
 *                           - 0 if the attribute value has zero length
 *                           - (size_t)-1 if a memory allocation error occurs
 *  @param  value    - pointer to the output array
 *                     or NULL to dynamically allocate the memory needed.
 *
 *  @return
 *    - pointer to the output array
 *    - NULL if:
 *        - the attribute value has zero length (length == 0)
 *        - a memory allocation error occurs (length == (size_t)-1)
 */
void *dsproc_get_dsdod_att_value(
    int           ds_id,
    const char   *var_name,
    const char   *att_name,
    CDSDataType   type,
    size_t       *length,
    void         *value)
{
    CDSAtt *att        = dsproc_get_dsdod_att(ds_id, var_name, att_name);
    size_t  tmp_length = 0;

    if (!length) {
        length = &tmp_length;
    }

    if (!att || !att->length || !att->value.vp) {
        *length = 0;
        return((void *)NULL);
    }

    value = cds_get_att_value(att, type, length, value);

    if (*length == (size_t)-1) {
        dsproc_set_status(DSPROC_ENOMEM);
    }

    return(value);
}

/**
 *  Get a copy of a datastream DOD attribute value.
 *
 *  This function will get a copy of a datastream DOD attribute value converted
 *  to a text string. The datastream DOD returned from the database will be
 *  checked first for the specified attribute. If the attribute is not found or
 *  its value has not been defined, the datastream metadata set by the user will
 *  be checked. If the data type of the attribute is not CDS_CHAR the
 *  cds_array_to_string() function is used to create the output string.
 *
 *  Memory will be allocated for the returned string if the output string
 *  is NULL. In this case the calling process is responsible for freeing
 *  the allocated memory.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  ds_id    - datastream ID
 *  @param  var_name - variable name (NULL for global attribute)
 *  @param  att_name - attribute name
 *  @param  length   - pointer to the length of the output string
 *                       - input:
 *                           - length of the output string
 *                           - ignored if the output string is NULL
 *                       - output:
 *                           - number of values written to the output string
 *                           - 0 if the attribute value has zero length
 *                           - (size_t)-1 if a memory allocation error occurs
 *  @param  value    - pointer to the output string
 *                     or NULL to dynamically allocate the memory needed.
 *
 *  @return
 *    - pointer to the output string
 *    - NULL if:
 *        - the attribute does not exist or has zero length (length = 0)
 *        - a memory allocation error occurs (length == (size_t)-1)
 */
char *dsproc_get_dsdod_att_text(
    int         ds_id,
    const char *var_name,
    const char *att_name,
    size_t     *length,
    char       *value)
{
    CDSAtt *att        = dsproc_get_dsdod_att(ds_id, var_name, att_name);
    size_t  tmp_length = 0;

    if (!length) {
        length = &tmp_length;
    }

    if (!att || !att->length || !att->value.vp) {
        *length = 0;
        return((void *)NULL);
    }

    value = cds_get_att_text(att, length, value);

    if (*length == (size_t)-1) {
        dsproc_set_status(DSPROC_ENOMEM);
    }

    return(value);
}

/**
 *  Get the length of a datastream DOD dimension.
 *
 *  This function will first check the datastream DOD returned from the
 *  database for the specified dimension. If the dimension is not found
 *  or its length has not been set, the datastream metadata set by the
 *  user will be checked.
 *
 *  @param  ds_id    - datastream ID
 *  @param  dim_name - dimension name
 *
 *  @return
 *    - length of the dimension
 *    - 0 if the dimension was not found or has zero length
 */
size_t dsproc_get_dsdod_dim_length(int ds_id, const char *dim_name)
{
    CDSDim *dim;

    dim = dsproc_get_dsdod_dim(ds_id, dim_name);
    if (!dim) {
        return(0);
    }

    return(dim->length);
}

/**
 *  Get the version of the datastream DOD currently loaded.
 *
 *  The memory used by the return string is internal to the DSDOD structure and
 *  must not be altered or freed.
 *
 *  @param  ds_id  datastream ID
 *  @param  major  output: major version number
 *  @param  minor  output: minor version number
 *  @param  micro  output: reserved for micro version number (this not currently
 *                 implemented but I can see a need for it coming soon...)
 *
 *  @return
 *    - version of the datastream DOD currently loaded (as a character string)
 *    - NULL if a datastream DOD has not been loaded
 */
const char *dsproc_get_dsdod_version(
    int  ds_id,
    int *major,
    int *minor,
    int *micro)
{
    DataStream *ds    = _DSProc->datastreams[ds_id];
    DSDOD      *dsdod = ds->dsdod;

    if (major) *major = 0;
    if (minor) *minor = 0;
    if (micro) *micro = 0;

    if (!dsdod ||
        !dsdod->version) {

        return((const char *)NULL);
    }

    parse_version_string(dsdod->version, major, minor, micro);

    return(dsdod->version);
}

/**
 *  Set a datastream DOD attribute value.
 *
 *  This function will set a runtime defined attribute value that will be
 *  used when new datasets are created. Only attributes that exist in the
 *  datastream DOD with a NULL value will be set in the datasets.
 *
 *  If a current dataset has already been created for the specified datastream,
 *  it's attribute value will also be updated.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  ds_id    - datastream ID
 *  @param  var_name - variable name (NULL for global attribute)
 *  @param  att_name - attribute name
 *  @param  type     - data type of the specified value
 *  @param  length   - length of the specified value
 *  @param  value    - pointer to the attribute value
 *
 *  @return
 *    - 1 if successful
 *    - 0 if a memory allocation error occurred
 */
int dsproc_set_dsdod_att_value(
    int          ds_id,
    const char  *var_name,
    const char  *att_name,
    CDSDataType  type,
    size_t       length,
    void        *value)
{
    DataStream *ds = _DSProc->datastreams[ds_id];
    CDSGroup   *cds;
    CDSAtt     *att;
    void       *parent;

    /* Create the metadata CDS group if it doesn't already exist */

    if (!ds->metadata) {

        ds->metadata = cds_define_group(NULL, ds->name);

        if (!ds->metadata) {
            dsproc_set_status(DSPROC_ENOMEM);
            return(0);
        }
    }

    cds = ds->metadata;

    /* Get attribute parent */

    if (var_name) {
        parent = (void *)cds_get_var(cds, var_name);
        if (!parent) {
            /* define a dummy variable for this attribute */
            parent = cds_define_var(cds, var_name, CDS_INT, 0, NULL);
            if (!parent) {
                dsproc_set_status(DSPROC_ENOMEM);
                return(0);
            }
        }
    }
    else {
        parent = (void *)cds;
    }

    /* Set the attribute value in the runtime metadata group */

    att = cds_get_att(parent, att_name);
    if (att) {
        if (!cds_set_att_value(att, type, length, value)) {
            dsproc_set_status(DSPROC_ENOMEM);
            return(0);
        }
    }
    else {
        if (!cds_define_att(parent, att_name, type, length, value)) {
            dsproc_set_status(DSPROC_ENOMEM);
            return(0);
        }
    }

    /* Set current output dataset attribute value */

    if (ds->out_cds) {

        if (!_dsproc_set_runtime_att_value(
            ds->out_cds, var_name, att_name, 0, 0, type, length, value)) {

            return(0);
        }
    }

    return(1);
}

/**
 *  Set a datastream DOD attribute value.
 *
 *  See dsproc_set_dsdod_att_value() for details.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  ds_id    - datastream ID
 *  @param  var_name - variable name (NULL for global attribute)
 *  @param  att_name - attribute name
 *  @param  format   - attribute value format string (see printf)
 *  @param  ...      - arguments for the format string
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int dsproc_set_dsdod_att_text(
    int         ds_id,
    const char *var_name,
    const char *att_name,
    const char *format, ...)
{
    DataStream *ds = _DSProc->datastreams[ds_id];
    va_list     args;
    char       *string;
    size_t      length;
    int         retval;

    va_start(args, format);

    string = msngr_format_va_list(format, args);
    if (!string) {

        ERROR( DSPROC_LIB_NAME,
            "Could not set attribute value in dataset: %s\n"
            " -> memory allocation error\n",
            ds->name);

        dsproc_set_status(DSPROC_ENOMEM);
        return(0);
    }

    length = strlen(string) + 1;
    retval = dsproc_set_dsdod_att_value(
        ds_id, var_name, att_name, CDS_CHAR, length, (void *)string);

    va_end(args);

    free(string);

    return(retval);
}

/**
 *  Set a datastream DOD dimension length.
 *
 *  This function will set a runtime defined dimension length that will be
 *  used when new datasets are created. Only static dimensions that exist
 *  in the datastream DOD with a length of 0 will be set in the datasets.
 *
 *  If a current dataset has already been created for the specified datastream,
 *  it’s dimension length will also be updated.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  ds_id      - datastream ID
 *  @param  dim_name   - dimension name
 *  @param  dim_length - dimension length
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int dsproc_set_dsdod_dim_length(
    int         ds_id,
    const char *dim_name,
    size_t      dim_length)
{
    DataStream *ds = _DSProc->datastreams[ds_id];
    CDSGroup   *cds;
    CDSDim     *dim;

    /* Create the metadata CDS group if it doesn't already exist */

    if (!ds->metadata) {

        ds->metadata = cds_define_group(NULL, ds->name);

        if (!ds->metadata) {
            dsproc_set_status(DSPROC_ENOMEM);
            return(0);
        }
    }

    cds = ds->metadata;

    /* Set the dimension length in the runtime metadata group */

    dim = cds_get_dim(cds, dim_name);
    if (dim) {
        if (!cds_change_dim_length(dim, dim_length)) {
            dsproc_set_status(DSPROC_ECDSSETDIM);
            return(0);
        }
    }
    else {
        if (!cds_define_dim(cds, dim_name, dim_length, 0)) {
            dsproc_set_status(DSPROC_ENOMEM);
            return(0);
        }
    }

    /* Set current output dataset dimension length */

    if (ds->out_cds) {

        dim = cds_get_dim(ds->out_cds, dim_name);

        if (dim && !dim->def_lock) {
            if (!cds_change_dim_length(dim, dim_length)) {
                dsproc_set_status(DSPROC_ECDSSETDIM);
                return(0);
            }
        }
    }

    return(1);
}

/**
 *  Update the DSDOD for a datastream.
 *
 *  This function will update the specified datastream's DSDOD for DOD
 *  version and time varying attribute value changes.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  ds_id     - datastream ID
 *  @param  data_time - the time used to determine the DOD version and
 *                      set the time varying attribute values
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int dsproc_update_datastream_dsdod(int ds_id, time_t data_time)
{
    DataStream *ds = _DSProc->datastreams[ds_id];

    if (ds->dsdod) {
        if (_dsproc_update_dsdod(ds, data_time) < 0) {
            return(0);
        }
    }

    return(1);
}

/**
 *  Update the DSDODs for all datastreams.
 *
 *  This function will update all available datastream DSDODs for DOD
 *  version and time varying attribute value changes.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  data_time - the time used to determine the DOD version and
 *                      set the time varying attribute values
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int dsproc_update_datastream_dsdods(time_t data_time)
{
    DataStream *ds;
    int         ds_id;
    int         retval;

    retval = 1;

    for (ds_id = 0; ds_id < _DSProc->ndatastreams; ds_id++) {

        ds = _DSProc->datastreams[ds_id];

        if (ds->dsdod) {
            if (_dsproc_update_dsdod(ds, data_time) < 0) {
                retval = 0;
            }
        }
    }

    return(retval);
}
