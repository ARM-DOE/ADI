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

/** @file dsproc_dataset_compare.c
 *  Dataset Compare Functions.
 */

#include "dsproc3.h"
#include "dsproc_private.h"

extern DSProc *_DSProc; /**< Internal DSProc structure */

/** @privatesection */

/*******************************************************************************
 *  Static Data and Functions Visible Only To This Module
 */

/**
 *  Typedef for structure used to specify attributes and static
 *  data that should be excluded from dod compare checks.
 */
typedef struct ExAtts ExAtts;

/**
 *  Structure used to specify attributes and static
 *  data that should be excluded from dod compare checks.
 */
struct ExAtts {

    ExAtts *next;         /**< next structure in list               */
    char   *var_name;     /**< variable name (NULL for global)      */
    int     natts;        /**< number of attributes to exclude      */
    char  **att_names;    /**< names of the attributes to exclude   */
    int     exclude_data; /**< exclude static data from dod compare */

};

/** Linked list of ExAtts */
static ExAtts *_ExAtts = (ExAtts *)NULL;

/**
 *  Static: Get the ExAtts structure for the specified variable.
 *
 *  @param  var_name - the variable name or NULL for global attributes
 *
 *  @return
 *    - the ExAtts structure
 *    - NULL if not found
 */
static ExAtts *_dsproc_get_exclude_atts(const char *var_name)
{
    ExAtts *ex_atts;

    for (ex_atts = _ExAtts; ex_atts; ex_atts = ex_atts->next) {

        if ((var_name == NULL) || (ex_atts->var_name == NULL) ) {
            if ((var_name == NULL) && (ex_atts->var_name == NULL) ) {
                return(ex_atts);
            }
        }
        else if (strcmp(ex_atts->var_name, var_name) == 0) {
            return(ex_atts);
        }
    }

    return((ExAtts *)NULL);
}

static const char *_DSName;     /**< name of the current dataset          */
static const char *_Header;     /**< metadata change message header       */
static int         _NumChanges; /**< track the number of metadata changes */
static int         _Warn;       /**< flag if warnings should be generated */

/**
 *  Initialize metadata change warning messages.
 */
#define INIT_METADATA_WARNINGS(warn, ds_name, header) \
_dsproc_init_metadata_warnings(warn, ds_name, header)

/**
 *  Generate a metadata change warning messages.
 */
#define METADATA_WARNING(...) \
_dsproc_metadata_warning(__func__, __FILE__, __LINE__, __VA_ARGS__)

/**
 *  Finish metadata change warning messages.
 */
#define FINISH_METADATA_WARNINGS() \
_dsproc_finish_metadata_warnings(__func__, __FILE__, __LINE__)

/**
 *  Static: Initialize metadata change warning messages.
 *
 *  @param  warn    - generate metadata change warning messages
 *  @param  ds_name - the name of the current dataset
 *  @param  header  - the header to use to start a new metadata change message
 */
static void _dsproc_init_metadata_warnings(
    int         warn,
    const char *ds_name,
    const char *header)
{
    _Warn       = warn;
    _DSName     = ds_name;
    _Header     = header;
    _NumChanges = 0;
}

/**
 *  Static: Finish metadata change warning messages.
 *
 *  @param  func - the name of the function sending the message (__func__)
 *  @param  file - the source file the message came from (__FILE__)
 *  @param  line - the line number in the source file (__LINE__)
 *
 *  @return  the number of metadata changes found
 */
static int _dsproc_finish_metadata_warnings(
    const char *func,
    const char *file,
    int         line)
{
    Mail *warning_mail;

    if (_NumChanges && _Warn) {

        warning_mail = msngr_get_mail(MSNGR_WARNING);

        if (warning_mail) {
            mail_set_flags(warning_mail, MAIL_ADD_NEWLINE);
        }

        msngr_send(
            DSPROC_LIB_NAME, func, file, line, MSNGR_WARNING,
            " - number of changes found: %d\n",
            _NumChanges);
    }

    return(_NumChanges);
}

/**
 *  Static: Generate a metadata change warning messages.
 *
 *  This function will append a warning message to the log file
 *  and warning mail message.
 *
 *  @param  func   - the name of the function sending the message (__func__)
 *  @param  file   - the source file the message came from (__FILE__)
 *  @param  line   - the line number in the source file (__LINE__)
 *  @param  format - format string (see printf)
 *  @param  ...    - arguments for the format string
 */
static void _dsproc_metadata_warning(
    const char *func,
    const char *file,
    int         line,
    const char *format, ...)
{
    Mail    *warning_mail;
    va_list  args;

    if (_Warn) {

        /* Check if this is the first change found */

        if (_NumChanges == 0) {

            warning_mail = msngr_get_mail(MSNGR_WARNING);

            if (warning_mail) {
                mail_unset_flags(warning_mail, MAIL_ADD_NEWLINE);
            }

            WARNING( DSPROC_LIB_NAME,
                "%s: %s\n",
                _DSName, _Header);
        }

        /* Generate the warning message */

        va_start(args, format);

        msngr_vsend(
            DSPROC_LIB_NAME, func, file, line, MSNGR_WARNING, format, args);

        va_end(args);
    }

    _NumChanges++;
}

/**
 *  Static: Compare the attributes in two attribute lists.
 *
 *  @param  var_name   - name of the variable or NULL for globals
 *  @param  ex_atts    - list of attributes to exclude
 *  @param  prev_natts - number of attributes in the previous attributes list
 *  @param  prev_atts  - previous attributes list
 *  @param  curr_natts - number of attributes in the current attributes list
 *  @param  curr_atts  - current attributes list
 *
 *  @return
 *    -  the number of attribute changes found
 *    - -1 if a memory allocation error occurred generating a warning message
 */
static int _dsproc_compare_atts(
    const char  *var_name,
    ExAtts      *ex_atts,
    int          prev_natts,
    CDSAtt     **prev_atts,
    int          curr_natts,
    CDSAtt     **curr_atts)
{
    CDSAtt *prev_att = (CDSAtt *)NULL;
    CDSAtt *curr_att;
    char   *curr_value;
    char   *prev_value;
    size_t  nbytes;
    size_t  length;
    char   *null_string;
    char   *indent_string;
    int     nchanges;
    int     ai, aj, xi;

    null_string   = "NULL";
    indent_string = (var_name) ? "   " : "";
    nchanges      = 0;

    int special_att_count = 0;
    
    /* Loop over attributes in the current attributes list */

    for (ai = 0; ai < curr_natts; ++ai) {

        curr_att = curr_atts[ai];
        
        if (strcmp(curr_att->name, "_Format")       == 0 ||
            strcmp(curr_att->name, "_DeflateLevel") == 0 ||
            strcmp(curr_att->name, "_ChunkSizes")   == 0 ||
            strcmp(curr_att->name, "_Shuffle")      == 0 ||
            strcmp(curr_att->name, "_Endianness")   == 0 ||
            strcmp(curr_att->name, "_Fletcher32")   == 0 ||
            strcmp(curr_att->name, "_NoFill")      == 0) {

            special_att_count++;
            continue;
        }

        /* Check if we need to exclude this attribute */

/*  BDE: At some point we may want to skip the long_name and units
 *  attribute checks for the time variable, and let the ncds write
 *  logic do the units conversion when appending to an existing file.
 *
 *  if (var_name && strcmp(var_name, "time") == 0 &&
 *      (strcmp(curr_att->name, "units") == 0 ||
 *       strcmp(curr_att->name, "long_name") == 0) ) {
 *
 *      continue;
 *  }
 */

        /* Check for user defined attributes to exclude */

        if (ex_atts) {

            for (xi = 0; xi < ex_atts->natts; ++xi) {
                if (strcmp(curr_att->name, ex_atts->att_names[xi]) == 0) {
                    break;
                }
            }

            if (xi != ex_atts->natts) {
                continue;
            }
        }

        /* Check if this attribute exists in the previous attributes list */

        for (aj = 0; aj < prev_natts; ++aj) {

            prev_att = prev_atts[aj];

            if (strcmp(curr_att->name, prev_att->name) == 0) {
                break;
            }
        }

        if (aj == prev_natts) {

            if (_Warn) {

                if (var_name && !nchanges) {

                    METADATA_WARNING(
                        " - %s: variable attribute changes\n",
                        var_name);
                }

                METADATA_WARNING(
                    "%s - %s: attribute not found in previous dataset\n",
                    indent_string, curr_att->name);
            }
            else {
                _NumChanges++;
            }

            nchanges++;
            continue;
        }

        /* Check if the attribute values are equal */

        nbytes = curr_att->length * cds_data_type_size(curr_att->type);

        if (prev_att->length == curr_att->length &&
            prev_att->type   == curr_att->type   &&
            memcmp(prev_att->value.vp, curr_att->value.vp, nbytes) == 0) {

            continue;
        }

        /* Generate the warning message */

        if (_Warn) {

            prev_value = null_string;
            curr_value = null_string;

            if (prev_att->length && prev_att->value.vp) {

                prev_value = cds_sprint_array(
                    prev_att->type,
                    prev_att->length,
                    prev_att->value.vp,
                    &length, NULL,
                    NULL, 0, 0,
                    0x02 | 0x10);
            }

            if (curr_att->length && curr_att->value.vp) {

                curr_value = cds_sprint_array(
                    curr_att->type,
                    curr_att->length,
                    curr_att->value.vp,
                    &length, NULL,
                    NULL, 0, 0,
                    0x02 | 0x10);
            }

            if (!prev_value || !curr_value) {

                ERROR( DSPROC_LIB_NAME,
                    "Could not generate warning message for attribute change\n"
                    " -> memory allocation error\n");

                if (prev_value && (prev_value != null_string)) free(prev_value);
                if (curr_value && (curr_value != null_string)) free(curr_value);

                dsproc_set_status(DSPROC_ENOMEM);
                return(-1);
            }

            if (var_name && !nchanges) {

                METADATA_WARNING(
                    " - %s: variable attribute changes\n",
                    var_name);
            }

            METADATA_WARNING(
                "%s - %s: attribute value changed\n"
                "%s    - from: %s\n"
                "%s    - to:   %s\n",
                indent_string, curr_att->name,
                indent_string, prev_value,
                indent_string, curr_value);

            if (prev_value != null_string) free(prev_value);
            if (curr_value != null_string) free(curr_value);
        }
        else {
            _NumChanges++;
        }

        nchanges++;

    } /* end loop over attributes in the current attributes list */

    /* Check if the number of attributes has changed */

    if (prev_natts != (curr_natts - special_att_count)) {

        if (_Warn) {

            if (var_name && !nchanges) {

                METADATA_WARNING(
                    " - %s: variable attribute changes\n",
                    var_name);
            }

            METADATA_WARNING(
                "%s - number of attributes changed from %d to %d\n",
                indent_string, prev_natts, curr_natts);
        }
        else {
            _NumChanges++;
        }

        nchanges++;
    }

    return(nchanges);
}

/*******************************************************************************
 *  Private Functions Visible Only To This Library
 */

/**
 *  Private: Free all memory used by the interval ExAtts list.
 */
void _dsproc_free_exclude_atts(void)
{
    ExAtts *ex_atts;
    ExAtts *next;
    int     ai;

    for (ex_atts = _ExAtts; ex_atts; ex_atts = next) {

        next = ex_atts->next;

        if (ex_atts->var_name) free(ex_atts->var_name);
        if (ex_atts->att_names) {

            for (ai = 0; ai < ex_atts->natts; ai++) {
                if (ex_atts->att_names[ai]) free(ex_atts->att_names[ai]);
            }

            free(ex_atts->att_names);
        }

        free(ex_atts);
    }

    _ExAtts = (ExAtts *)NULL;
}

/**
 *  Private: Exclude standard attributes from dod compare.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @return
 *    - 1 if successful
 *    - 0 if a memory allocation error occurred
 */
int _dsproc_set_standard_exclude_atts(void)
{
    /* Global attributes */

    const char *global_atts[] = {
        "command_line",
        "dod_version",   // BDE: Let the library detect true DOD changes.
        "facility_id",   // BDE: temporary hack to prevent file splits during new standards transtion.
        "input_source",
        "input_datastreams",
        "history"
    };
    int global_natts = sizeof(global_atts)/sizeof(char *);

    if (!dsproc_exclude_from_dod_compare(NULL, 0, global_natts, global_atts)) {
        return(0);
    }

    /* base_time variable attributes */

    const char *bt_atts[] = {
        "string"
    };
    int bt_natts = sizeof(bt_atts)/sizeof(char *);

    if (!dsproc_exclude_from_dod_compare("base_time", 1, bt_natts, bt_atts)) {
        return(0);
    }

    /* time_offset variable attributes */

    const char *to_atts[] = {
        "units"
    };
    int to_natts = sizeof(to_atts)/sizeof(char *);

    if (!dsproc_exclude_from_dod_compare("time_offset", 1, to_natts, to_atts)) {
        return(0);
    }

    /* time variable attributes */

    const char *time_atts[] = {
        "units"
    };
    int time_natts = sizeof(time_atts)/sizeof(char *);

    if (!dsproc_exclude_from_dod_compare("time", 1, time_natts, time_atts)) {
        return(0);
    }

    /* time_bounds variable attributes */

    const char *tb_atts[] = {
        "units"
    };
    int tb_natts = sizeof(tb_atts)/sizeof(char *);

    if (!dsproc_exclude_from_dod_compare("time_bounds", 1, tb_natts, tb_atts)) {
        return(0);
    }

    return(1);
}

/** @publicsection */

/*******************************************************************************
 *  Internal Functions Visible To The Public
 */

/**
 *  Compare the DOD versions of two datasets.
 *
 *  @param  prev_ds - previous dataset
 *  @param  curr_ds - current dataset
 *  @param  warn    - generate warning message if DOD version changed
 *
 *  @return
 *    - 1 if the DOD version has changed
 *    - 0 if the DOD version has not changed
 */
int dsproc_compare_dod_versions(CDSGroup *prev_ds, CDSGroup *curr_ds, int warn)
{
    const char *prev_version;
    const char *curr_version;
    CDSAtt     *att;

    att = cds_get_att(prev_ds, "dod_version");
    prev_version = (att && att->type == CDS_CHAR) ? att->value.cp : "NULL";

    att = cds_get_att(curr_ds, "dod_version");
    curr_version = (att && att->type == CDS_CHAR) ? att->value.cp : "NULL";

    if (strcmp(prev_version, curr_version) != 0) {

        if (warn) {

            WARNING( DSPROC_LIB_NAME,
                "%s: DOD version changed\n"
                " - from: %s\n"
                " - to:   %s\n",
                curr_ds->name, prev_version, curr_version);
        }

        return(1);
    }

    return(0);
}

/**
 *  Compare the DOD dimensions of two datasets.
 *
 *  @param  prev_ds - previous dataset
 *  @param  curr_ds - current dataset
 *  @param  warn    - generate warning message if DOD dimensions have changed
 *
 *  @return  the number of changes found
 */
int dsproc_compare_dod_dims(CDSGroup *prev_ds, CDSGroup *curr_ds, int warn)
{
    CDSDim *prev_dim;
    CDSDim *curr_dim;
    int     nchanges;
    int     di;

    INIT_METADATA_WARNINGS(warn, curr_ds->name, "DOD dimension changes");

    /* Loop over dimensions in the current dataset */

    for (di = 0; di < curr_ds->ndims; ++di) {

        curr_dim = curr_ds->dims[di];
        prev_dim = cds_get_dim(prev_ds, curr_dim->name);

        /* Check if this dimension exists in the previous dataset */

        if (!prev_dim) {

            METADATA_WARNING(
                " - %s: dimension not found in previous dataset\n",
                curr_dim->name);

            continue;
        }

        /* Check if this is an unlimited dimension */

        if (prev_dim->is_unlimited || curr_dim->is_unlimited) {

            if (prev_dim->is_unlimited != curr_dim->is_unlimited) {

                if (prev_dim->is_unlimited) {

                    METADATA_WARNING(
                        " - %s: dimension changed from UNLIMITED to %d\n",
                        curr_dim->name, curr_dim->length);
                }
                else {

                    METADATA_WARNING(
                        " - %s: dimension changed from %d to UNLIMITED\n",
                        curr_dim->name, prev_dim->length);
                }
            }

            continue;
        }

        /* Check if the dimension length has changed */

        if (prev_dim->length != curr_dim->length) {

            METADATA_WARNING(
                " - %s: length of dimension changed from %d to %d\n",
                curr_dim->name, prev_dim->length, curr_dim->length);

            continue;
        }

    } /* end loop over dimensions in the current dataset */

    /* Check if the number of dimensions has changed */

    if (curr_ds->ndims != prev_ds->ndims) {

        METADATA_WARNING(
            " - number of dimensions changed from %d to %d\n",
            prev_ds->ndims, curr_ds->ndims);
    }

    nchanges = FINISH_METADATA_WARNINGS();

    return(nchanges);
}

/**
 *  Compare the DOD attributes of two datasets.
 *
 *  @param  prev_ds - previous dataset
 *  @param  curr_ds - current dataset
 *  @param  warn    - generate warning message if DOD attributes have changed
 *
 *  @return
 *    -  the number of changes found
 *    - -1 if a memory allocation error occurred generating a warning message
 */
int dsproc_compare_dod_atts(CDSGroup *prev_ds, CDSGroup *curr_ds, int warn)
{
    ExAtts *ex_atts = _dsproc_get_exclude_atts(NULL);
    int     nchanges;

    INIT_METADATA_WARNINGS(warn, curr_ds->name, "DOD attribute changes");

    nchanges = _dsproc_compare_atts(
        NULL, ex_atts,
        prev_ds->natts, prev_ds->atts,
        curr_ds->natts, curr_ds->atts);

    if (nchanges < 0) {
        return(-1);
    }

    nchanges = FINISH_METADATA_WARNINGS();

    return(nchanges);
}

/**
 *  Compare the DOD variables of two datasets.
 *
 *  @param  prev_ds - previous dataset
 *  @param  curr_ds - current dataset
 *  @param  warn    - generate warning message if DOD variables have changed
 *
 *  @return
 *    -  the number of changes found
 *    - -1 if a memory allocation error occurred generating a warning message
 */
int dsproc_compare_dod_vars(CDSGroup *prev_ds, CDSGroup *curr_ds, int warn)
{
    CDSVar *prev_var;
    CDSVar *curr_var;
    ExAtts *ex_atts;
    int     nchanges;
    int     natt_changes;
    size_t  prev_sample_size;
    size_t  curr_sample_size;
    size_t  nbytes;
    int     di, vi;

    INIT_METADATA_WARNINGS(warn, curr_ds->name, "DOD variable changes");

    /* Loop over variables in the current dataset */

    for (vi = 0; vi < curr_ds->nvars; ++vi) {

        nchanges = 0;
        curr_var = curr_ds->vars[vi];
        prev_var = cds_get_var(prev_ds, curr_var->name);

        /* Check if this variable exists in the previous dataset */

        if (!prev_var) {

            METADATA_WARNING(
                " - %s: variable not found in previous dataset\n",
                curr_var->name);

            continue;
        }

        /* Check if the variable data type has changed */

        if (prev_var->type != curr_var->type) {

            METADATA_WARNING(
                " - %s: variable data type changed from %s to %s\n",
                curr_var->name,
                cds_data_type_name(prev_var->type),
                cds_data_type_name(curr_var->type));

            nchanges += 1;
        }

        /* Check if the variable dimensions have changed */

        if (prev_var->ndims != curr_var->ndims) {

            /* The number of dimensions has changed */

            METADATA_WARNING(
                " - %s: number of variable dimensions changed from %d to %d\n",
                curr_var->name, prev_var->ndims, curr_var->ndims);

            nchanges += 1;
        }
        else {

            /* Check if the variable dimensions have changed */

            for (di = 0; di < curr_var->ndims; ++di) {

                if (strcmp(curr_var->dims[di]->name,
                           prev_var->dims[di]->name) != 0) {

                    METADATA_WARNING(
                        " - %s: variable dimension changed from %s to %s\n",
                        curr_var->name,
                        curr_var->dims[di]->name,
                        prev_var->dims[di]->name);

                    nchanges += 1;
                }
            }
        }

        /* Check if the variable attributes have changed */

        ex_atts = _dsproc_get_exclude_atts(curr_var->name);

        natt_changes = _dsproc_compare_atts(
            curr_var->name, ex_atts,
            prev_var->natts, prev_var->atts,
            curr_var->natts, curr_var->atts);

        if (natt_changes < 0) {
            return(-1);
        }

        /* Check if we need to compare static data */

        if (nchanges) {

            /* variable type or shape has changed */
            continue;
        }

        if ((curr_var->ndims > 0) &&
            (curr_var->dims[0]->is_unlimited)) {

            /* not a static variable */
            continue;
        }

        if (ex_atts &&
            ex_atts->exclude_data) {

            /* variable excluded from static data check */
            continue;
        }

        /* Make sure the sample count has not changed */

        if (prev_var->sample_count != curr_var->sample_count) {

            /* Removed this warning because it is redundant with the
             * warnings for dimension length changes and only serves
             * to overly clutter the output mail message.
             *
             * METADATA_WARNING(
             *     " - %s: variable sample count changed from %d to %d\n",
             *     curr_var->name,
             *     (int)prev_var->sample_count,
             *     (int)curr_var->sample_count);
             */
            continue;
        }

        /* Make sure the sample size has not changed */

        prev_sample_size = cds_var_sample_size(prev_var);
        curr_sample_size = cds_var_sample_size(curr_var);

        if (prev_sample_size != curr_sample_size) {

            /* Removed this warning because it is redundant with the
             * warnings for dimension length changes and only serves
             * to overly clutter the output mail message.
             *
             * METADATA_WARNING(
             *     " - %s: variable sample sizes changed from %d to %d\n",
             *     curr_var->name,
             *     (int)prev_sample_size,
             *     (int)curr_sample_size);
             */
            continue;
        }

        /* Compare static data */

        nbytes = curr_var->sample_count
               * curr_sample_size
               * cds_data_type_size(curr_var->type);

        if (nbytes > 0 &&
            memcmp(prev_var->data.vp, curr_var->data.vp, nbytes) != 0) {

            METADATA_WARNING(
                " - %s: static variable data changed\n",
                curr_var->name);
        }

    } /* end loop over variables in the current dataset */

    /* Check if the number of variables has changed */

    if (curr_ds->nvars != prev_ds->nvars) {

        METADATA_WARNING(
            " - number of variables changed from %d to %d\n",
            prev_ds->nvars, curr_ds->nvars);
    }

    nchanges = FINISH_METADATA_WARNINGS();

    return(nchanges);
}

/**
 *  Compare the DODs of two datasets.
 *
 *  @param  prev_ds - previous dataset
 *  @param  curr_ds - current dataset
 *  @param  warn    - generate warning message if changes are found
 *
 *  @return
 *    -  the number of changes found
 *    - -1 if a memory allocation error occurred generating a warning message
 */
int dsproc_compare_dods(CDSGroup *prev_ds, CDSGroup *curr_ds, int warn)
{
    int nchanges = 0;
    int status;

    nchanges = dsproc_compare_dod_dims(prev_ds, curr_ds, warn);

    status = dsproc_compare_dod_atts(prev_ds, curr_ds, warn);
    if (status < 0) return (-1);

    nchanges += status;

    status = dsproc_compare_dod_vars(prev_ds, curr_ds, warn);
    if (status < 0) return (-1);

    nchanges += status;

    return(nchanges);
}

/**
 *  Exclude attributes and/or static data from dod compare.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  var_name     - the variable name
 *                         or NULL for global attributes
 *
 *  @param  exclude_data - flag specifying if static data should be excluded,
 *                         (1 == TRUE, 0 == FALSE)
 *
 *  @param  natts         - the number of attribute names in the list
 *  @param  att_names     - the list attribute names
 *
 *  @return
 *    - 1 if successful
 *    - 0 if a memory allocation error occurred
 */
int dsproc_exclude_from_dod_compare(
    const char  *var_name,
    int          exclude_data,
    int          natts,
    const char **att_names)
{
    ExAtts *ex_atts = _dsproc_get_exclude_atts(var_name);
    int     new_natts;
    char  **new_att_names;
    int     ai, aj;

    /* Create a new structure if necessary */

    if (!ex_atts) {

        ex_atts = (ExAtts *)calloc(1, sizeof(ExAtts));
        if (!ex_atts) goto MEMORY_ERROR;

        if (var_name) {
            ex_atts->var_name = strdup(var_name);
            if (!ex_atts->var_name) {
                free(ex_atts);
                goto MEMORY_ERROR;
            }
        }

        ex_atts->next = _ExAtts;
        _ExAtts       = ex_atts;
    }

    /* Set the exclude data flag */

    ex_atts->exclude_data = exclude_data;

    /* Add attribute names */

    if (natts && att_names) {

        /* Re-allocate memory for the attribute names list */

        new_natts     = natts + ex_atts->natts;
        new_att_names = (char **)realloc(
            ex_atts->att_names, new_natts * sizeof(char *));

        if (!new_att_names) goto MEMORY_ERROR;

        ex_atts->att_names = new_att_names;

        /* Add new attributes to the list */

        for (ai = 0; ai < natts; ++ai) {

            /* Check if this attribute already exists in the list */

            for (aj = 0; aj < ex_atts->natts; ++aj) {
                if (strcmp(att_names[ai], ex_atts->att_names[aj]) == 0) {
                    break;
                }
            }

            if (aj != ex_atts->natts) continue;

            /* Add this attribute to the list */

            if (!(ex_atts->att_names[ex_atts->natts] = strdup(att_names[ai]))) {
                goto MEMORY_ERROR;
            }

            ex_atts->natts += 1;
        }
    }

    return(1);

MEMORY_ERROR:

    if (var_name) {
        ERROR( DSPROC_LIB_NAME,
            "Could not exclude variable attributes from dod compare for: %s\n"
            " -> memory allocation error\n", var_name);
    }
    else {
        ERROR( DSPROC_LIB_NAME,
            "Could not exclude global attributes from dod compare\n"
            " -> memory allocation error\n");
    }

    dsproc_set_status(DSPROC_ENOMEM);
    return(0);
}
