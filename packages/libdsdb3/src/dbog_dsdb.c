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
*    $Revision: 48761 $
*    $Author: ermold $
*    $Date: 2013-10-28 20:35:44 +0000 (Mon, 28 Oct 2013) $
*
*******************************************************************************/

/** @file dbog_dsdb.c
 *  DSDB object group functions.
 */

#include "dbconn.h"

/** @privatesection */

/*******************************************************************************
*  Facilities
*/

DBStatus dsdbog_get_facility_location(
    DBConn      *dbconn,
    const char  *site,
    const char  *facility,
    DBResult   **result)
{
    const char *command = "SELECT * FROM get_facility_location($1,$2)";
    const char *params[2];

    params[0] = site;
    params[1] = facility;

    return(dbconn_query(dbconn, command, 2, params, result));
}

DBStatus dsdbog_get_site_description(
    DBConn      *dbconn,
    const char  *site,
    DBResult   **result)
{
    const char *command = "SELECT * FROM inquire_sites($1)";
    const char *params[1];

    params[0] = site;

    return(dbconn_query(dbconn, command, 1, params, result));
}

/*******************************************************************************
*  Process Config Values
*/

DBStatus dsdbog_define_process_config_key(
    DBConn     *dbconn,
    const char *key,
    int        *result)
{
    const char *command = "SELECT define_process_config_key($1)";
    const char *params[1];

    params[0] = key;

    return(dbconn_query_int(dbconn, command, 1, params, result));
}

DBStatus dsdbog_delete_process_config_key(
    DBConn     *dbconn,
    const char *key,
    int        *result)
{
    const char *command = "SELECT delete_process_config_key($1)";
    const char *params[1];

    params[0] = key;

    return(dbconn_query_bool(dbconn, command, 1, params, result));
}

DBStatus dsdbog_delete_process_config_value(
    DBConn     *dbconn,
    const char *proc_type,
    const char *proc_name,
    const char *site,
    const char *facility,
    const char *key,
    int        *result)
{
    const char *command = "SELECT delete_process_config_value($1,$2,$3,$4,$5)";
    const char *params[5];

    params[0] = proc_type;
    params[1] = proc_name;
    params[2] = site;
    params[3] = facility;
    params[4] = key;

    return(dbconn_query_bool(dbconn, command, 5, params, result));
}

DBStatus dsdbog_get_process_config_values(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    const char  *site,
    const char  *facility,
    const char  *key,
    DBResult   **result)
{
    const char *command = "SELECT * FROM get_process_config_values($1,$2,$3,$4,$5)";
    const char *params[5];

    params[0] = proc_type;
    params[1] = proc_name;
    params[2] = site;
    params[3] = facility;
    params[4] = key;

    return(dbconn_query(dbconn, command, 5, params, result));
}

DBStatus dsdbog_update_process_config_value(
    DBConn     *dbconn,
    const char *proc_type,
    const char *proc_name,
    const char *site,
    const char *facility,
    const char *key,
    const char *value,
    int        *result)
{
    const char *command = "SELECT update_process_config_value($1,$2,$3,$4,$5,$6)";
    const char *params[6];

    params[0] = proc_type;
    params[1] = proc_name;
    params[2] = site;
    params[3] = facility;
    params[4] = key;
    params[5] = value;

    return(dbconn_query_int(dbconn, command, 6, params, result));
}

/*******************************************************************************
*  Process Families
*/

DBStatus dsdbog_inquire_process_families(
    DBConn      *dbconn,
    const char  *category,
    const char  *proc_class,
    const char  *site,
    const char  *facility,
    DBResult   **result)
{
    const char *command = "SELECT * FROM inquire_process_families($1,$2,$3,$4)";
    const char *params[4];

    params[0] = category;
    params[1] = proc_class;
    params[2] = site;
    params[3] = facility;

    return(dbconn_query(dbconn, command, 4, params, result));
}

/*******************************************************************************
*  Family Processes
*/

DBStatus dsdbog_get_family_process(
    DBConn      *dbconn,
    const char  *site,
    const char  *facility,
    const char  *proc_type,
    const char  *proc_name,
    DBResult   **result)
{
    const char *command = "SELECT * FROM get_family_process($1,$2,$3,$4)";
    const char *params[4];

    params[0] = site;
    params[1] = facility;
    params[2] = proc_type;
    params[3] = proc_name;

    return(dbconn_query(dbconn, command, 4, params, result));
}

DBStatus dsdbog_inquire_family_processes(
    DBConn      *dbconn,
    const char  *category,
    const char  *proc_class,
    const char  *site,
    const char  *facility,
    const char  *proc_type,
    const char  *proc_name,
    DBResult   **result)
{
    const char *command = "SELECT * FROM inquire_family_processes($1,$2,$3,$4,$5,$6)";
    const char *params[6];

    params[0] = category;
    params[1] = proc_class;
    params[2] = site;
    params[3] = facility;
    params[4] = proc_type;
    params[5] = proc_name;

    return(dbconn_query(dbconn, command, 6, params, result));
}

DBStatus dsdbog_get_family_process_location(
    DBConn      *dbconn,
    const char  *site,
    const char  *facility,
    const char  *proc_type,
    const char  *proc_name,
    DBResult   **result)
{
    const char *command = "SELECT * FROM get_family_process_location($1,$2,$3,$4)";
    const char *params[4];

    params[0] = site;
    params[1] = facility;
    params[2] = proc_type;
    params[3] = proc_name;

    return(dbconn_query(dbconn, command, 4, params, result));
}

/*******************************************************************************
*  Family Process States
*/

DBStatus dsdbog_delete_family_process_state(
    DBConn     *dbconn,
    const char *site,
    const char *facility,
    const char *proc_type,
    const char *proc_name,
    int        *result)
{
    const char *command = "SELECT delete_family_process_state($1,$2,$3,$4)";
    const char *params[4];

    params[0] = site;
    params[1] = facility;
    params[2] = proc_type;
    params[3] = proc_name;

    return(dbconn_query_bool(dbconn, command, 4, params, result));
}

DBStatus dsdbog_get_family_process_state(
    DBConn      *dbconn,
    const char  *site,
    const char  *facility,
    const char  *proc_type,
    const char  *proc_name,
    DBResult   **result)
{
    const char *command = "SELECT * FROM get_family_process_state($1,$2,$3,$4)";
    const char *params[4];

    params[0] = site;
    params[1] = facility;
    params[2] = proc_type;
    params[3] = proc_name;

    return(dbconn_query(dbconn, command, 4, params, result));
}

DBStatus dsdbog_inquire_family_process_states(
    DBConn      *dbconn,
    const char  *category,
    const char  *proc_class,
    const char  *site,
    const char  *facility,
    const char  *proc_type,
    const char  *proc_name,
    DBResult   **result)
{
    const char *command = "SELECT * FROM inquire_family_process_states($1,$2,$3,$4,$5,$6)";
    const char *params[6];

    params[0] = category;
    params[1] = proc_class;
    params[2] = site;
    params[3] = facility;
    params[4] = proc_type;
    params[5] = proc_name;

    return(dbconn_query(dbconn, command, 6, params, result));
}

DBStatus dsdbog_is_family_process_enabled(
    DBConn     *dbconn,
    const char *site,
    const char *facility,
    const char *proc_type,
    const char *proc_name,
    int        *result)
{
    const char *command = "SELECT is_family_process_enabled($1,$2,$3,$4)";
    const char *params[4];

    params[0] = site;
    params[1] = facility;
    params[2] = proc_type;
    params[3] = proc_name;

    return(dbconn_query_bool(dbconn, command, 4, params, result));
}

DBStatus dsdbog_update_family_process_state(
    DBConn     *dbconn,
    const char *site,
    const char *facility,
    const char *proc_type,
    const char *proc_name,
    const char *state,
    const char *description,
    time_t      state_time,
    int        *result)
{
    const char *command = "SELECT update_family_process_state($1,$2,$3,$4,$5,$6,$7)";
    const char *params[7];
    char        time_string[32];

    params[0] = site;
    params[1] = facility;
    params[2] = proc_type;
    params[3] = proc_name;
    params[4] = state;
    params[5] = description;

    if (state_time) {
        params[6] = dbconn_time_to_text(dbconn, state_time, time_string);
        if (!params[6]) {
            *result = 0;
            return(DB_ERROR);
        }
    }
    else {
        params[6] = (const char *)NULL;
    }

    return(dbconn_query_int(dbconn, command, 7, params, result));
}

/*******************************************************************************
*  Family Process Statuses
*/

DBStatus dsdbog_delete_family_process_status(
    DBConn     *dbconn,
    const char *site,
    const char *facility,
    const char *proc_type,
    const char *proc_name,
    int        *result)
{
    const char *command = "SELECT delete_family_process_status($1,$2,$3,$4)";
    const char *params[4];

    params[0] = site;
    params[1] = facility;
    params[2] = proc_type;
    params[3] = proc_name;

    return(dbconn_query_bool(dbconn, command, 4, params, result));
}

DBStatus dsdbog_get_family_process_status(
    DBConn      *dbconn,
    const char  *site,
    const char  *facility,
    const char  *proc_type,
    const char  *proc_name,
    DBResult   **result)
{
    const char *command = "SELECT * FROM get_family_process_status($1,$2,$3,$4)";
    const char *params[4];

    params[0] = site;
    params[1] = facility;
    params[2] = proc_type;
    params[3] = proc_name;

    return(dbconn_query(dbconn, command, 4, params, result));
}

DBStatus dsdbog_inquire_family_process_statuses(
    DBConn      *dbconn,
    const char  *category,
    const char  *proc_class,
    const char  *site,
    const char  *facility,
    const char  *proc_type,
    const char  *proc_name,
    DBResult   **result)
{
    const char *command = "SELECT * FROM inquire_family_process_statuses($1,$2,$3,$4,$5,$6)";
    const char *params[6];

    params[0] = category;
    params[1] = proc_class;
    params[2] = site;
    params[3] = facility;
    params[4] = proc_type;
    params[5] = proc_name;

    return(dbconn_query(dbconn, command, 6, params, result));
}

DBStatus dsdbog_update_family_process_started(
    DBConn     *dbconn,
    const char *site,
    const char *facility,
    const char *proc_type,
    const char *proc_name,
    time_t      started_time,
    int        *result)
{
    const char *command = "SELECT update_family_process_started($1,$2,$3,$4,$5)";
    const char *params[5];
    char        time_string[32];

    params[0] = site;
    params[1] = facility;
    params[2] = proc_type;
    params[3] = proc_name;

    if (started_time) {
        params[4] = dbconn_time_to_text(dbconn, started_time, time_string);
        if (!params[4]) {
            *result = 0;
            return(DB_ERROR);
        }
    }
    else {
        params[4] = (const char *)NULL;
    }

    return(dbconn_query_int(dbconn, command, 5, params, result));
}

DBStatus dsdbog_update_family_process_completed(
    DBConn     *dbconn,
    const char *site,
    const char *facility,
    const char *proc_type,
    const char *proc_name,
    time_t      completed_time,
    int        *result)
{
    const char *command = "SELECT update_family_process_completed($1,$2,$3,$4,$5)";
    const char *params[5];
    char        time_string[32];

    params[0] = site;
    params[1] = facility;
    params[2] = proc_type;
    params[3] = proc_name;

    if (completed_time) {
        params[4] = dbconn_time_to_text(dbconn, completed_time, time_string);
        if (!params[4]) {
            *result = 0;
            return(DB_ERROR);
        }
    }
    else {
        params[4] = (const char *)NULL;
    }

    return(dbconn_query_int(dbconn, command, 5, params, result));
}

DBStatus dsdbog_update_family_process_status(
    DBConn     *dbconn,
    const char *site,
    const char *facility,
    const char *proc_type,
    const char *proc_name,
    const char *status,
    const char *description,
    time_t      completed_time,
    int        *result)
{
    const char *command = "SELECT update_family_process_status($1,$2,$3,$4,$5,$6,$7)";
    const char *params[7];
    char        time_string[32];

    params[0] = site;
    params[1] = facility;
    params[2] = proc_type;
    params[3] = proc_name;
    params[4] = status;
    params[5] = description;

    if (completed_time) {
        params[6] = dbconn_time_to_text(dbconn, completed_time, time_string);
        if (!params[6]) {
            *result = 0;
            return(DB_ERROR);
        }
    }
    else {
        params[6] = (const char *)NULL;
    }

    return(dbconn_query_int(dbconn, command, 7, params, result));
}

/*******************************************************************************
*  Process Input Datastream Classes
*/

DBStatus dsdbog_get_process_input_ds_classes(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    DBResult   **result)
{
    const char *command = "SELECT * FROM get_process_input_ds_classes($1,$2)";
    const char *params[2];

    params[0] = proc_type;
    params[1] = proc_name;

    return(dbconn_query(dbconn, command, 2, params, result));
}

DBStatus dsdbog_inquire_process_input_ds_classes(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    const char  *dsc_name,
    const char  *dsc_level,
    DBResult   **result)
{
    const char *command = "SELECT * FROM inquire_process_input_ds_classes($1,$2,$3,$4)";
    const char *params[4];

    params[0] = proc_type;
    params[1] = proc_name;
    params[2] = dsc_name;
    params[3] = dsc_level;

    return(dbconn_query(dbconn, command, 4, params, result));
}

/*******************************************************************************
*  Process Output Datastream Classes
*/

DBStatus dsdbog_get_process_output_ds_classes(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    DBResult   **result)
{
    const char *command = "SELECT * FROM get_process_output_ds_classes($1,$2)";
    const char *params[2];

    params[0] = proc_type;
    params[1] = proc_name;

    return(dbconn_query(dbconn, command, 2, params, result));
}

DBStatus dsdbog_inquire_process_output_ds_classes(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    const char  *dsc_name,
    const char  *dsc_level,
    DBResult   **result)
{
    const char *command = "SELECT * FROM inquire_process_output_ds_classes($1,$2,$3,$4)";
    const char *params[4];

    params[0] = proc_type;
    params[1] = proc_name;
    params[2] = dsc_name;
    params[3] = dsc_level;

    return(dbconn_query(dbconn, command, 4, params, result));
}

/*******************************************************************************
*  Process Output Datastreams
*/

DBStatus dsdbog_delete_process_output_datastream(
    DBConn     *dbconn,
    const char *proc_type,
    const char *proc_name,
    const char *dsc_name,
    const char *dsc_level,
    const char *site,
    const char *facility,
    int        *result)
{
    const char *command = "SELECT delete_process_output_datastream($1,$2,$3,$4,$5,$6)";
    const char *params[6];

    params[0] = proc_type;
    params[1] = proc_name;
    params[2] = dsc_name;
    params[3] = dsc_level;
    params[4] = site;
    params[5] = facility;

    return(dbconn_query_bool(dbconn, command, 6, params, result));
}

DBStatus dsdbog_get_process_output_datastream(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    const char  *dsc_name,
    const char  *dsc_level,
    const char  *site,
    const char  *facility,
    DBResult   **result)
{
    const char *command = "SELECT * FROM get_process_output_datastream($1,$2,$3,$4,$5,$6)";
    const char *params[6];

    params[0] = proc_type;
    params[1] = proc_name;
    params[2] = dsc_name;
    params[3] = dsc_level;
    params[4] = site;
    params[5] = facility;

    return(dbconn_query(dbconn, command, 6, params, result));
}

DBStatus dsdbog_inquire_process_output_datastreams(
    DBConn      *dbconn,
    const char  *proc_type,
    const char  *proc_name,
    const char  *dsc_name,
    const char  *dsc_level,
    const char  *site,
    const char  *facility,
    DBResult   **result)
{
    const char *command = "SELECT * FROM inquire_process_output_datastreams($1,$2,$3,$4,$5,$6)";
    const char *params[6];

    params[0] = proc_type;
    params[1] = proc_name;
    params[2] = dsc_name;
    params[3] = dsc_level;
    params[4] = site;
    params[5] = facility;

    return(dbconn_query(dbconn, command, 6, params, result));
}

DBStatus dsdbog_update_process_output_datastream(
    DBConn          *dbconn,
    const char      *proc_type,
    const char      *proc_name,
    const char      *dsc_name,
    const char      *dsc_level,
    const char      *site,
    const char      *facility,
    const timeval_t *first_time,
    const timeval_t *last_time,
    int             *result)
{
    const char *command = "SELECT update_process_output_datastream($1,$2,$3,$4,$5,$6,$7,$8)";
    const char *params[8];
    char        first_string[32];
    char        last_string[32];

    params[0] = proc_type;
    params[1] = proc_name;
    params[2] = dsc_name;
    params[3] = dsc_level;
    params[4] = site;
    params[5] = facility;

    if (first_time && first_time->tv_sec) {
        params[6] = dbconn_timeval_to_text(dbconn, first_time, first_string);
        if (!params[6]) {
            *result = 0;
            return(DB_ERROR);
        }
    }
    else {
        params[6] = (const char *)NULL;
    }

    if (last_time && last_time->tv_sec) {
        params[7] = dbconn_timeval_to_text(dbconn, last_time, last_string);
        if (!params[7]) {
            *result = 0;
            return(DB_ERROR);
        }
    }
    else {
        params[7] = (const char *)NULL;
    }

    return(dbconn_query_int(dbconn, command, 8, params, result));
}

/*******************************************************************************
*  Datastream Config Values
*/

DBStatus dsdbog_define_datastream_config_key(
    DBConn     *dbconn,
    const char *key,
    int        *result)
{
    const char *command = "SELECT define_datastream_config_key($1)";
    const char *params[1];

    params[0] = key;

    return(dbconn_query_int(dbconn, command, 1, params, result));
}

DBStatus dsdbog_delete_datastream_config_key(
    DBConn     *dbconn,
    const char *key,
    int        *result)
{
    const char *command = "SELECT delete_datastream_config_key($1)";
    const char *params[1];

    params[0] = key;

    return(dbconn_query_bool(dbconn, command, 1, params, result));
}

DBStatus dsdbog_delete_datastream_config_value(
    DBConn     *dbconn,
    const char *dsc_name,
    const char *dsc_level,
    const char *site,
    const char *facility,
    const char *key,
    int        *result)
{
    const char *command = "SELECT delete_datastream_config_value($1,$2,$3,$4,$5)";
    const char *params[5];

    params[0] = dsc_name;
    params[1] = dsc_level;
    params[2] = site;
    params[3] = facility;
    params[4] = key;

    return(dbconn_query_bool(dbconn, command, 5, params, result));
}

DBStatus dsdbog_get_datastream_config_values(
    DBConn      *dbconn,
    const char  *dsc_name,
    const char  *dsc_level,
    const char  *site,
    const char  *facility,
    const char  *key,
    DBResult   **result)
{
    const char *command = "SELECT * FROM get_datastream_config_values($1,$2,$3,$4,$5)";
    const char *params[5];

    params[0] = dsc_name;
    params[1] = dsc_level;
    params[2] = site;
    params[3] = facility;
    params[4] = key;

    return(dbconn_query(dbconn, command, 5, params, result));
}

DBStatus dsdbog_update_datastream_config_value(
    DBConn     *dbconn,
    const char *dsc_name,
    const char *dsc_level,
    const char *site,
    const char *facility,
    const char *key,
    const char *value,
    int        *result)
{
    const char *command = "SELECT update_process_config_value($1,$2,$3,$4,$5,$6";
    const char *params[6];

    params[0] = dsc_name;
    params[1] = dsc_level;
    params[2] = site;
    params[3] = facility;
    params[4] = key;
    params[5] = value;

    return(dbconn_query_int(dbconn, command, 6, params, result));
}
