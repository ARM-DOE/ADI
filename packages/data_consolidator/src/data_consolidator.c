/*******************************************************************************
*
*  COPYRIGHT (C) 2012 Battelle Memorial Institute.  All Rights Reserved.
*
********************************************************************************
*
*  Authors:
*     name:  Brian Ermold
*     phone: (509) 375-2277
*     email: brian.ermold@pnnl.gov
*
*     name:  Krista Gaustad
*     phone: (509) 375-5950
*     email: krista.gaustad@pnnl.gov
*
********************************************************************************
*
*  REPOSITORY INFORMATION:
*    $Revision: 1.4 $
*    $Author: ermold $
*    $Date: 2012-04-04 04:37:48 $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file data_consolidator.c
 *  Data Consolidator Source File.
 */
#include "dsproc3.h"

#include "../config.h"
static const char *_Version = PACKAGE_NAME"-"PACKAGE_VERSION;

/**
 *  @defgroup DATA_CONSOLIDATOR Data Consolidator
 */
/*@{*/

/**
 *  Hook function called just after data is retrieved.
 *
 *  This function will be called once per processing interval just after
 *  data retrieval, but before the retrieved observations are merged and
 *  QC is applied.
 *
 *  @param user_data  - void pointer to the UserData structure
 *  @param begin_date - the begin time of the current processing interval
 *  @param end_date   - the end time of the current processing interval
 *  @param ret_data   - pointer to the parent CDSGroup containing all the
 *                      retrieved data
 *
 *  @return
 *    -  1 if processing should continue normally
 *    -  0 if processing should skip the current processing interval
 *         and continue on to the next one.
 *    - -1 if a fatal error occurred and the process should exit.
 */
int post_retrieval_hook(
    void     *user_data,
    time_t    begin_date,
    time_t    end_date,
    CDSGroup *ret_data)
{
    if (dsproc_get_debug_level() > 1) {

        dsproc_dump_retrieved_datasets(
            "./debug_dumps", "post_retrieval.debug", 0);
    }

    /*********************************************************************
    * Quick hack to "fix" units with MSL and AGL.
    *
    * Note: This is *NOT* how we want to fix this for real in the ADI
    * libraries, for that we want to get the altitude from the input
    * files and actually convert MSL to AGL.
    *
    **********************************************************************/
    {
        int       ndsids;
        int      *dsids;
        int       dsid;
        int       dsi, obsi, vi;
        CDSGroup *obs;
        CDSVar   *var;
        CDSAtt   *att;
        char     *value;
        char     *chrp;
        int       length;

        ndsids = dsproc_get_input_datastream_ids(&dsids);
        if (ndsids < 0) return(0);

        for (dsi = 0; dsi < ndsids; ++dsi) {

            dsid = dsids[dsi];

            for (obsi = 0; ; ++obsi) {

                obs = dsproc_get_retrieved_dataset(dsid, obsi);
                if (!obs) break;

                for (vi = 0; vi < obs->nvars; ++vi) {

                    var = obs->vars[vi];
                    att = cds_get_att(var, "units");
                    if (!att || att->type != CDS_CHAR) continue;

                    value = att->value.cp;

                    if (strstr(value, "MSL") ||
                        strstr(value, "AGL")) {

                        chrp = strchr(value, ' ');

                        if (chrp) {
                            *chrp  = '\0';
                            length = chrp - value + 1;
                            att->length = length;
                        }
                    }
                }
            }
        }
    }

    return(1);
}

/**
 *  Hook function called just prior to data transformation.
 *
 *  This function will be called once per processing interval just prior to
 *  data transformation, and after the retrieved observations are merged.
 *
 *  @param user_data  - void pointer to the UserData structure
 *  @param begin_date - the begin time of the current processing interval
 *  @param end_date   - the end time of the current processing interval
 *  @param ret_data   - pointer to the parent CDSGroup containing all the
 *                      retrieved data
 *
 *  @return
 *    -  1 if processing should continue normally
 *    -  0 if processing should skip the current processing interval
 *         and continue on to the next one.
 *    - -1 if a fatal error occurred and the process should exit.
 */
int pre_transform_hook(
    void     *user_data,
    time_t    begin_date,
    time_t    end_date,
    CDSGroup *ret_data)
{
    if (dsproc_get_debug_level() > 1) {

        dsproc_dump_retrieved_datasets(
            "./debug_dumps", "pre_transform.debug", 0);
    }

    return(1);
}

/**
 *  Hook function called just after data transformation.
 *
 *  This function will be called once per processing interval just after data
 *  transformation, but before the output datasets are created.
 *
 *  @param user_data  - void pointer to the UserData structure
 *  @param begin_date - the begin time of the current processing interval
 *  @param end_date   - the end time of the current processing interval
 *  @param trans_data - pointer to the parent CDSGroup containing all the
 *                      transformed data
 *
 *  @return
 *    -  1 if processing should continue normally
 *    -  0 if processing should skip the current processing interval
 *         and continue on to the next one.
 *    - -1 if a fatal error occurred and the process should exit.
 */
int post_transform_hook(
    void     *user_data,
    time_t    begin_date,
    time_t    end_date,
    CDSGroup *trans_data)
{
    if (dsproc_get_debug_level() > 1) {

        dsproc_dump_transformed_datasets(
            "./debug_dumps", "post_transform.debug", 0);
    }

    return(1);
}

/**
 *  Main data processing function.
 *
 *  This function will be called once per processing interval just after the
 *  output datasets are created, but before they are stored to disk.
 *
 *  @param  user_data  - void pointer to the UserData structure
 *  @param  begin_date - begin time of the processing interval
 *  @param  end_date   - end time of the processing interval
 *  @param  input_data - parent CDSGroup containing the input data
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int process_data(
    void     *user_data,
    time_t    begin_date,
    time_t    end_date,
    CDSGroup *input_data)
{
    if (dsproc_get_debug_level() > 1) {

        dsproc_dump_output_datasets(
            "./debug_dumps", "process_data.debug", 0);
    }

    return(1);
}

/**
 *  Main entry function.
 *
 *  @param  argc - number of command line arguments
 *  @param  argv - command line arguments
 *
 *  @return
 *    - 0 if successful
 *    - 1 if an error occurred
 */
int main(int argc, char **argv)
{
    dsproc_set_post_retrieval_hook(post_retrieval_hook);
    dsproc_set_pre_transform_hook(pre_transform_hook);
    dsproc_set_post_transform_hook(post_transform_hook);
    dsproc_set_process_data_hook(process_data);

    dsproc_enable_legacy_time_vars(1);

    return(dsproc_main(argc, argv, PM_TRANSFORM_VAP, _Version, 0, NULL));
}

/*@}*/
