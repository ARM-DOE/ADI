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

/** @file dsproc_dataset_times.c
 *  Dataset Time Functions.
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
 *  Get the base time of a dataset or time variable.
 *
 *  This function will convert the units attribute of a time variable to 
 *  seconds since 1970.  If the input object is a CDSGroup, the specified
 *  dataset and then its parent datasets will be searched until a "time"
 *  or "time_offset" variable is found.
 *
 *  @param  cds_object - pointer to a CDSGroup or CDSVar
 *
 *  @return
 *    - base time in seconds since 1970 UTC
 *    - -1 if not found
 */
time_t dsproc_get_base_time(void *cds_object)
{
    time_t base_time = cds_get_base_time(cds_object);

    return(base_time);
}

/**
 *  Get the time range of a dataset or time variable.
 *
 *  This function will get the start and end times of a time variable.
 *  If the input object is a CDSGroup, the specified dataset and then
 *  its parent datasets will be searched until a "time" or "time_offset"
 *  variable is found.
 *
 *  @param  cds_object   - pointer to a CDSGroup or CDSVar
 *  @param  start_time   - pointer to the timeval_t structure to store the
 *                         start time in.
 *  @param  end_time     - pointer to the timeval_t structure to store the
 *                         end time in.
 *
 *  @return
 *    - number of time values
 *    - 0 if no time values were found
 */
size_t dsproc_get_time_range(
    void      *cds_object,
    timeval_t *start_time,
    timeval_t *end_time)
{
    size_t ntimes;

    ntimes = cds_get_time_range(cds_object, start_time, end_time);

    return(ntimes);
}

/**
 *  Get the time variable used by a dataset.
 *
 *  This function will find the first dataset that contains either the "time"
 *  or "time_offset" variable and return a pointer to that variable.
 *
 *  @param  cds_object - pointer to a CDSGroup or CDSVar
 *
 *  @return
 *    - pointer to the time variable
 *    - NULL if not found
 */
CDSVar *dsproc_get_time_var(void *cds_object)
{
    CDSVar *var = cds_find_time_var(cds_object);

    return(var);
}

/**
 *  Get the sample times for a dataset or time variable.
 *
 *  This function will convert the data values of a time variable to seconds
 *  since 1970. If the input object is a CDSGroup, the specified dataset and
 *  then its parent datasets will be searched until a "time" or "time_offset"
 *  variable is found.
 *
 *  Memory will be allocated for the returned array of sample times if the
 *  output array is NULL. In this case the calling process is responsible
 *  for freeing the allocated memory.
 *
 *  Note: If the sample times can have fractional seconds the
 *  dsproc_get_sample_timevals() function should be used instead.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  cds_object   - pointer to a CDSGroup or CDSVar
 *  @param  sample_start - start sample (0 based indexing)
 *  @param  sample_count - pointer to the sample_count
 *                            - input:
 *                                - length of the output array
 *                                - ignored if the output array is NULL
 *                            - output:
 *                                - number of sample times returned
 *                                - 0 if no data was found for sample_start
 *                                - (size_t)-1 if a memory allocation error occurs
 *  @param  sample_times  - pointer to the output array
 *                          or NULL to dynamically allocate the memory needed.
 *
 *  @return
 *    - pointer to the array of sample times in seconds since 1970
 *    - NULL if:
 *        - there is no data for sample_start (sample_count == 0)
 *        - an error occurred (sample_count == (size_t)-1)
 */
time_t *dsproc_get_sample_times(
    void   *cds_object,
    size_t  sample_start,
    size_t *sample_count,
    time_t *sample_times)
{
    size_t tmp_sample_count = 0;

    if (!sample_count) {
        sample_count = &tmp_sample_count;
    }

    sample_times = cds_get_sample_times(
        cds_object, sample_start, sample_count, sample_times);

    if (*sample_count == (size_t)-1) {
        dsproc_set_status(DSPROC_ECDSGETTIME);
    }

    return(sample_times);
}

/**
 *  Get the sample times for a dataset or time variable.
 *
 *  This function will convert the data values of a time variable to an
 *  array of timeval_t structures. If the input object is a CDSGroup, the
 *  specified dataset and then its parent datasets will be searched until
 *  a "time" or "time_offset" variable is found.
 *
 *  Memory will be allocated for the returned array of sample times if the
 *  output array is NULL. In this case the calling process is responsible
 *  for freeing the allocated memory.
 *
 *  Note: Consider using the cds_get_sample_times() function if the sample
 *  times can not have fractional seconds.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  cds_object   - pointer to a CDSGroup or CDSVar
 *  @param  sample_start - start sample (0 based indexing)
 *  @param  sample_count - pointer to the sample_count
 *                            - input:
 *                                - length of the output array
 *                                - ignored if the output array is NULL
 *                            - output:
 *                                - number of sample times returned
 *                                - 0 if no data was found for sample_start
 *                                - (size_t)-1 if a memory allocation error occurs
 *  @param  sample_times  - pointer to the output array
 *                          or NULL to dynamically allocate the memory needed.
 *
 *  @return
 *    - pointer to the array of sample times in seconds since 1970
 *    - NULL if:
 *        - the variable has no data for sample_start (sample_count == 0)
 *        - an error occurred (sample_count == (size_t)-1)
 */
timeval_t *dsproc_get_sample_timevals(
    void      *cds_object,
    size_t     sample_start,
    size_t    *sample_count,
    timeval_t *sample_times)
{
    size_t tmp_sample_count = 0;

    if (!sample_count) {
        sample_count = &tmp_sample_count;
    }

    sample_times = cds_get_sample_timevals(
        cds_object, sample_start, sample_count, sample_times);

    if (*sample_count == (size_t)-1) {
        dsproc_set_status(DSPROC_ECDSGETTIME);
    }

    return(sample_times);
}

/**
 *  Set the base time of a dataset or time variable.
 *
 *  This function will set the base time for a time variable and adjust all
 *  attributes and data values as necessary. If the input object is one of
 *  the standard time variables ("time", "time_offset", or "base_time"), all
 *  standard time variables that exist in its parent dataset will also be
 *  updated. If the input object is a CDSGroup, the specified dataset and then
 *  its parent datasets will be searched until a "time" or "time_offset"
 *  variable is found. All standard time variables that exist in this dataset
 *  will then be updated.
 *
 *  For the base_time variable the data value will be set and the "string"
 *  attribute will be set to the string representation of the base_time value.
 *  The "long_name" and "units" attributes will also be set to
 *  "Base time in Epoch" and "seconds since 1970-1-1 0:00:00 0:00" respectively.
 *
 *  For the time_offset variable the "units" attribute will set to the string
 *  representation of the base_time value, and the "long_name" attribute will
 *  be set to "Time offset from base_time".
 *
 *  For all other time variables the "units" attribute will be set to the
 *  string representation of the base_time value, and the "long_name" attribute
 *  will be set to the specified value. If a long_name attribute is not
 *  specified, the string "Time offset from midnight" will be used for base
 *  times of midnight, and "Sample times" will be used for all other base times.
 *
 *  Any existing data in a time variable will also be adjusted for the new
 *  base_time value.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  cds_object - pointer to a CDSGroup or CDSVar
 *  @param  long_name  - string to use for the long_name attribute,
 *                       or NULL to use the default value
 *  @param  base_time  - base time in seconds since 1970
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int dsproc_set_base_time(
    void       *cds_object,
    const char *long_name,
    time_t      base_time)
{
    int status;

    status = cds_set_base_time(cds_object, long_name, base_time);

    if (status == 0) {
        dsproc_set_status(DSPROC_ECDSSETTIME);
    }

    return(status);
}

/**
 *  Set the sample times for a dataset or time variable.
 *
 *  This function will set the data values for a time variable by subtracting
 *  the base time (as defined by the units attribute) and converting the
 *  remainder to the data type of the variable. If the input object is one of
 *  the standard time variables ("time", "time_offset", or "base_time"), all
 *  standard time variables that exist in its parent dataset will also be
 *  updated. If the input object is a CDSGroup, the specified dataset and then
 *  its parent datasets will be searched until a "time" or "time_offset"
 *  variable is found. All standard time variables that exist in this dataset
 *  will then be updated.
 *
 *  If the specified sample_start value is 0 and a base time value has not
 *  already been set, the base time will be set using the time of midnight
 *  just prior to the first sample time.
 *
 *  The data type of the time variable(s) must be CDS_SHORT, CDS_INT, CDS_FLOAT
 *  or CDS_DOUBLE. However, CDS_DOUBLE is usually recommended.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  cds_object   - pointer to a CDSGroup or CDSVar
 *  @param  sample_start - start sample (0 based indexing)
 *  @param  sample_count - number of samples in the times array
 *  @param  sample_times - pointer to the array of sample times
 *                         in seconds since 1970 UTC.
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int dsproc_set_sample_times(
    void     *cds_object,
    size_t    sample_start,
    size_t    sample_count,
    time_t   *sample_times)
{
    int status;

    status = cds_set_sample_times(
        cds_object, sample_start, sample_count, sample_times);

    if (status == 0) {
        dsproc_set_status(DSPROC_ECDSSETTIME);
    }

    return(status);
}

/**
 *  Set the sample times for a dataset or time variable.
 *
 *  This function will set the data values for a time variable by subtracting
 *  the base time (as defined by the units attribute) and converting the
 *  remainder to the data type of the variable.
 *
 *  If the input object is one of the standard time variables:
 *
 *    - time
 *    - time_offset
 *    - base_time
 *
 *  All standard time variables that exist in its parent dataset will also be
 *  updated.
 *
 *  If the input object is a CDSGroup, the specified dataset and then its parent
 *  datasets will be searched until a "time" or "time_offset" variable is found.
 *  All standard time variables that exist in this dataset will then be updated.
 *
 *  If the specified sample_start value is 0 and a base time value has not
 *  already been set, the base time will be set using the time of midnight
 *  just prior to the first sample time.
 *
 *  The data type of the time variable(s) must be either CDS_FLOAT or
 *  or CDS_DOUBLE. However, CDS_DOUBLE is usually recommended.
 *
 *  If an error occurs in this function it will be appended to the log and
 *  error mail messages, and the process status will be set appropriately.
 *
 *  @param  cds_object   - pointer to a CDSGroup or CDSVar
 *  @param  sample_start - start sample (0 based indexing)
 *  @param  sample_count - number of samples in the times array
 *  @param  sample_times - pointer to the array of sample times
 *                         in seconds since 1970 UTC.
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int dsproc_set_sample_timevals(
    void      *cds_object,
    size_t     sample_start,
    size_t     sample_count,
    timeval_t *sample_times)
{
    int status;

    status = cds_set_sample_timevals(
        cds_object, sample_start, sample_count, sample_times);

    if (status == 0) {
        dsproc_set_status(DSPROC_ECDSSETTIME);
    }

    return(status);
}
