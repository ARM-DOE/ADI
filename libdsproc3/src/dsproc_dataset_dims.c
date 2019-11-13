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
*    $Revision: 13180 $
*    $Author: ermold $
*    $Date: 2012-03-25 00:16:40 +0000 (Sun, 25 Mar 2012) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file dsproc_dataset_dims.c
 *  Dataset Dimension Functions.
 */

#include "dsproc3.h"
#include "dsproc_private.h"

extern DSProc *_DSProc; /**< Internal DSProc structure */

/** @privatesection */

/*******************************************************************************
 *  Static Functions Visible Only To This Module
 */

/*******************************************************************************
 *  Private Functions Visible Only To This Library
 */

/** @publicsection */

/*******************************************************************************
 *  Internal Functions Visible To The Public
 */

/*******************************************************************************
 *  Public Functions
 */

/**
 *  Get a dimension from a dataset.
 *
 *  @param  dataset - pointer to the dataset
 *  @param  name    - name of the dimension
 *
 *  @return
 *    - pointer to the dimension
 *    - NULL if the dimension does not exist
 */
CDSDim *dsproc_get_dim(
    CDSGroup   *dataset,
    const char *name)
{
    return(cds_get_dim(dataset, name));
}

/**
 *  Get the length of a dimension in a dataset.
 *
 *  @param  dataset - pointer to the dataset
 *  @param  name    - name of the dimension
 *
 *  @return
 *    - dimension length
 *    - 0 if the dimension does not exist or has 0 length
 */
size_t dsproc_get_dim_length(
    CDSGroup   *dataset,
    const char *name)
{
    CDSDim *dim = cds_get_dim(dataset, name);

    if (!dim) {
        return(0);
    }

    return(dim->length);
}

/**
 *  Set the length of a dimension in a dataset.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  dataset - pointer to the dataset
 *  @param  name    - name of the dimension
 *  @param  length  - new length of the dimension
 *
 *  @return
 *    - 1 if successful
 *    - 0 if:
 *        - the dimension does not exist
 *        - the dimension definition is locked
 *        - data has already been added to a variable using this dimension
 */
int dsproc_set_dim_length(
    CDSGroup    *dataset,
    const char  *name,
    size_t       length)
{
    CDSDim *dim    = cds_get_dim(dataset, name);
    int     status = 1;

    if (dim) {

        if (dim->def_lock) {

            ERROR( DSPROC_LIB_NAME,
                "Could not set dimension length for: %s\n"
                " -> dimension length was defined in the DOD\n",
                cds_get_object_path(dim));

            status = 0;
        }
        else {
            status = cds_change_dim_length(dim, length);
        }
    }
    else {

        ERROR( DSPROC_LIB_NAME,
            "Could not set dimension length for: %s/_dims_/%s\n"
            " -> dimension does not exist\n",
            cds_get_object_path(dataset), name);

        status = 0;
    }

    if (!status) {
        dsproc_set_status(DSPROC_ECDSSETDIM);
        return(0);
    }

    return(1);
}
