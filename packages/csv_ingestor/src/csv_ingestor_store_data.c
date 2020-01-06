/*****************************************************************************
* Copyright (C) 2014, Battelle Memorial Institute
* All rights reserved.
*
* Author:
*    name:  Brian Ermold
*    email: brian.ermold@pnnl.gov
*
******************************************************************************/

/** @file csv_ingestor_store_data.c
 *  CSV Ingestor Store Data.
 */

#include "csv_ingestor.h"

/**
 *  Store CSV data in output NetCDF file.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param   data   pointer to the UserData structure
 *  @param   ds     pointer to the DsData structure
 *
 *  @retval  nrecs  number of records stored
 *  @retval  -1     if a fatal error occurred
 */
int csv_ingestor_store_data(UserData *data, DsData *ds)
{
    int         dsid  = ds->dsid;
    CSVConf    *conf  = ds->conf;
    CSVParser  *csv   = ds->csv;
    timeval_t  *times = csv->tvs;
    int         nrecs = csv->nrecs;

    CDSGroup   *dataset;
    int         nstored;

    /************************************************************
    * Create the output dataset
    *************************************************************/

    dataset = dsproc_create_output_dataset(dsid, times[0].tv_sec, 1);
    if (!dataset) return(-1);

    /************************************************************
    * Map the CSV fields to the output dataset variables
    *************************************************************/

    if (!ds->map) {
        ds->map = dsproc_create_csv_to_cds_map(conf, csv, dataset, 0);
    }

    if (!dsproc_map_csv_to_cds(csv, 0, 0, ds->map, dataset, 0, 0)) {
        return(-1);
    }

    /************************************************************
    * Set times in output dataset
    *************************************************************/

    if (!dsproc_set_sample_timevals(dataset, 0, nrecs, times)) {
        return(-1);
    }

    /************************************************************
    * Store the output dataset
    *************************************************************/

    /* Dump the output dataset to a file if debug level is > 3 */

    if (dsproc_get_debug_level() > 3) {

        dsproc_dump_output_datasets(
            "./debug_dumps", "before_store.debug", 0);
    }

    nstored = dsproc_store_dataset(dsid, 0);

    return(nstored);
}
