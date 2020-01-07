/*******************************************************************************
*
*  COPYRIGHT (C) 2012 Battelle Memorial Institute.  All Rights Reserved.
*
********************************************************************************
*
*  Author:
*     name:  Brian Ermold
*     phone: (509) 375-5950
*     email: brian.ermold@pnnl.gov
*
********************************************************************************
*
*  REPOSITORY INFORMATION:
*    $Revision: 13180 $
*    $Author: ermold $
*    $Date: 2012-03-25 00:16:40 +0000 (Sun, 25 Mar 2012) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file dsproc_deprecated.c
 *  Deprecated Functions.
 */

#include "dsproc3.h"

/** @publicsection */

/*******************************************************************************
 *  Internal Functions Visible To The Public
 */

/*******************************************************************************
 *  Public Functions
 */

/**
 *  Deprecated: use dsproc_create_output_dataset() instead.
 *
 *  This function has been deprecated. New software should use the
 *  dsproc_create_output_dataset() function.
 *
 *  @param  ds_id        - datastream ID
 *  @param  data_time    - the time used to determine the DOD version and
 *                         set the time varying attribute values
 *  @param  set_location - specifies if the location should be set using
 *                         the process location defined in the database.
 *
 *  @return
 *    - pointer to the new dataset
 *    - NULL if an error occurred
 */
CDSGroup *dsproc_create_dataset(
    int      ds_id,
    time_t   data_time,
    int      set_location)
{
    return(dsproc_create_output_dataset(ds_id, data_time, set_location));
}

/**
 *  Deprecated: use dsproc_map_datasets() instead.
 *
 *  This function has been deprecated. New software should use the
 *  dsproc_map_datasets() function.
 *
 *  @param  trans_cds - pointer to the transformation dataset
 *  @param  out_cds   - pointer to the output dataset
 *  @param  flags     - reserved for control flags
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int dsproc_trans_dataset_pass_through(
    CDSGroup *trans_cds,
    CDSGroup *out_cds,
    int       flags)
{
    return(dsproc_map_datasets(trans_cds, out_cds, 0));
}

/**
 *  Deprecated: Run retriever VAP without transformation logic.
 *
 *  This function has been deprecated. New software should use the
 *  dsproc_main() function.
 *
 *  @param  argc              - argc value from the main function
 *  @param  argv              - argv value from the main function
 *  @param  version_tag       - vap software version
 *  @param  proc_name         - process name (uses -n switch if NULL)
 *  @param  valid_proc_names  - function used to get valid process names
 *  @param  init_process      - function used to init process specific data
 *  @param  finish_process    - function used to cleanup process specific data
 *  @param  process_data      - function used to process and store the retrieved data
 */
void dsproc_vap_main(
    int            argc,
    char          *argv[],
    char          *version_tag,
    char          *proc_name,
    const char **(*valid_proc_names)(int *nproc_names),
    void        *(*init_process)(void),
    void         (*finish_process)(void *user_data),
    int          (*process_data)(void *user_data,
                     time_t begin_date, time_t end_date, CDSGroup *ret_data))
{
    const char **proc_names  = (const char **)NULL;
    int          nproc_names = 0;
    int          exit_value;

    if (init_process)   dsproc_set_init_process_hook(init_process);
    if (finish_process) dsproc_set_finish_process_hook(finish_process);
    if (process_data)   dsproc_set_process_data_hook(process_data);

    if (proc_name) {
        nproc_names = 1;
        proc_names  = (const char **)&proc_name;
    }
    else if (valid_proc_names) {
        proc_names = valid_proc_names(&nproc_names);
    }

    exit_value = dsproc_main(
        argc, (char **)argv, PM_RETRIEVER_VAP, version_tag,
        nproc_names, proc_names);

    DEBUG_LV1( DSPROC_LIB_NAME,
        "Exit Value: %d\n", exit_value);

    exit(exit_value);
}

/**
 *  Deprecated: Run VAP using transformation logic.
 *
 *  @param  argc              - argc value from the main function
 *  @param  argv              - argv value from the main function
 *  @param  version_tag       - vap software version
 *  @param  proc_name         - process name (uses -n switch if NULL)
 *  @param  valid_proc_names  - function used to get valid process names
 *  @param  init_process      - function used to init process specific data
 *  @param  finish_process    - function used to cleanup process specific data
 *  @param  process_data      - function used to process and store the transformed data
 */
void dsproc_transform_main(
    int            argc,
    char          *argv[],
    char          *version_tag,
    char          *proc_name,
    const char **(*valid_proc_names)(int *nproc_names),
    void        *(*init_process)(void),
    void         (*finish_process)(void *user_data),
    int          (*process_data)(void *user_data,
                     time_t begin_date, time_t end_date, CDSGroup *trans_data))
{
    const char **proc_names  = (const char **)NULL;
    int          nproc_names = 0;
    int          exit_value;

    if (init_process)   dsproc_set_init_process_hook(init_process);
    if (finish_process) dsproc_set_finish_process_hook(finish_process);
    if (process_data)   dsproc_set_process_data_hook(process_data);

    if (proc_name) {
        nproc_names = 1;
        proc_names  = (const char **)&proc_name;
    }
    else if (valid_proc_names) {
        proc_names = valid_proc_names(&nproc_names);
    }

    exit_value = dsproc_main(
        argc, (char **)argv, PM_TRANSFORM_VAP, version_tag,
        nproc_names, proc_names);

    DEBUG_LV1( DSPROC_LIB_NAME,
        "Exit Value: %d\n", exit_value);

    exit(exit_value);
}
