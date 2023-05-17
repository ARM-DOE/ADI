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
*     email: brian.ermold@pnnl.gov
*
*******************************************************************************/

/** @file cds_times.c
 *  CDS Time Functions.
 */

#include <errno.h>

#include "cds3.h"
#include "cds_private.h"

/*******************************************************************************
 *  Private Functions
 */
/** @privatesection */

/**
 *  PRIVATE: Get sample times from a time variable.
 *
 *  Memory will be allocated for the returned array of sample times if the
 *  output array is NULL. In this case the calling process is responsible
 *  for freeing the allocated memory.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  var          - pointer to the variable
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
time_t *_cds_get_sample_times(
    CDSVar *var,
    size_t  sample_start,
    size_t *sample_count,
    time_t *sample_times)
{
    size_t   nsamples;
    time_t   base_time;
    size_t   sample_size;
    size_t   nelems;
    size_t   type_size;
    CDSData  data;

    /* Check if the variable has any data for the requested sample_start */

    if (!var || !var->data.vp || var->sample_count <= sample_start) {
        if (sample_count) *sample_count = 0;
        return((time_t *)NULL);
    }

    /* Determine the number of samples to get */

    nsamples = var->sample_count - sample_start;

    if (sample_times && sample_count && *sample_count > 0) {
        if (nsamples > *sample_count) {
            nsamples = *sample_count;
        }
    }

    if (sample_count) *sample_count = nsamples;

    /* Get the base time for this variable */

    base_time = cds_get_base_time(var);
    if (base_time < 0) {
        if (sample_count) *sample_count = (size_t)-1;
        return((time_t *)NULL);
    }

    /* Convert the time offsets to time_t values */

    sample_size  = cds_var_sample_size(var);
    type_size    = cds_data_type_size(var->type);
    data.bp      = var->data.bp + (sample_start * sample_size * type_size);
    nelems       = nsamples * sample_size;

    sample_times = cds_offsets_to_times(
        var->type, nelems, base_time, data.vp, sample_times);

    if (!sample_times) {

        ERROR( CDS_LIB_NAME,
            "Could not get sample times for variable: %s\n"
            " -> memory allocation error\n", cds_get_object_path(var));

        if (sample_count) *sample_count = (size_t)-1;
        return((time_t *)NULL);
    }

    return(sample_times);
}

/**
 *  PRIVATE: Get sample times from a time variable.
 *
 *  Memory will be allocated for the returned array of sample times if the
 *  output array is NULL. In this case the calling process is responsible
 *  for freeing the allocated memory.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  var          - pointer to the variable
 *  @param  sample_start - start sample (0 based indexing)
 *  @param  sample_count - pointer to the sample_count
 *                           - input:
 *                               - length of the output array
 *                               - ignored if the output array is NULL
 *                           - output:
 *                               - number of samples returned
 *                               - 0 if no data was found for sample_start
 *                               - (size_t)-1 if a memory allocation error occurs
 *  @param  sample_times - pointer to the output array
 *                         or NULL to dynamically allocate the memory needed.
 *
 *  @return
 *    - pointer to the array of sample times in seconds since 1970
 *    - NULL if:
 *        - the variable has no data for sample_start (sample_count == 0)
 *        - an error occurred (sample_count == (size_t)-1)
 */
timeval_t *_cds_get_sample_timevals(
    CDSVar    *var,
    size_t     sample_start,
    size_t    *sample_count,
    timeval_t *sample_times)
{
    size_t     nsamples;
    time_t     base_time;
    size_t     sample_size;
    size_t     nelems;
    size_t     type_size;
    CDSData    data;

    /* Check if the variable has any data for the requested sample_start */

    if (!var || !var->data.vp || var->sample_count <= sample_start) {
        if (sample_count) *sample_count = 0;
        return((timeval_t *)NULL);
    }

    /* Determine the number of samples to get */

    nsamples = var->sample_count - sample_start;

    if (sample_times && sample_count && *sample_count > 0) {
        if (nsamples > *sample_count) {
            nsamples = *sample_count;
        }
    }

    if (sample_count) *sample_count = nsamples;

    /* Get the base time for this variable */

    base_time = cds_get_base_time(var);
    if (base_time < 0) {
        if (sample_count) *sample_count = (size_t)-1;
        return((timeval_t *)NULL);
    }

    /* Convert the time offsets to timeval_t values */

    sample_size  = cds_var_sample_size(var);
    type_size    = cds_data_type_size(var->type);
    data.bp      = var->data.bp + (sample_start * sample_size * type_size);
    nelems       = nsamples * sample_size;

    sample_times = cds_offsets_to_timevals(
        var->type, nelems, base_time, data.vp, sample_times);

   if (!sample_times) {

        ERROR( CDS_LIB_NAME,
            "Could not get sample times for variable: %s\n"
            " -> memory allocation error\n", cds_get_object_path(var));

        if (sample_count) *sample_count = (size_t)-1;
        return((timeval_t *)NULL);
    }

    return(sample_times);
}

/**
 *  PRIVATE: Set the data values for a time variable.
 *
 *  Note: The data type of the time variable must be CDS_SHORT,
 *  CDS_INT, CDS_INT64, CDS_FLOAT or CDS_DOUBLE.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  var          - pointer to the variable
 *  @param  base_time    - base time value
 *  @param  sample_start - start sample (0 based indexing)
 *  @param  sample_count - number of samples in the times array
 *  @param  sample_times - pointer to the array of sample times
 *                         in seconds since 1970
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int _cds_set_sample_times(
    CDSVar *var,
    time_t  base_time,
    size_t  sample_start,
    size_t  sample_count,
    time_t *sample_times)
{
    size_t   sample_size;
    size_t   nelems;
    CDSData  data;
    time_t  *timep;

    /* Get the sample size of this variable */

    sample_size = cds_var_sample_size(var);
    if (!sample_size) {

        ERROR( CDS_LIB_NAME,
            "Could not set sample times for variable: %s\n"
            " -> static dimension has 0 length\n",
            cds_get_object_path(var));

        return(0);
    }

    /* Allocate memory for the new sample times */

    data.vp = cds_alloc_var_data(var, sample_start, sample_count);
    if (!data.vp) {
        return(0);
    }

    nelems = sample_count * sample_size + 1;
    timep  = sample_times;

    switch (var->type) {

        case CDS_SHORT:
            while (--nelems) *data.sp++ = (short)(*timep++ - base_time);
            break;
        case CDS_INT:
            while (--nelems) *data.ip++ = (int)(*timep++ - base_time);
            break;
        case CDS_FLOAT:
            while (--nelems) *data.fp++ = (float)(*timep++ - base_time);
            break;
        case CDS_DOUBLE:
            while (--nelems) *data.dp++ = (double)(*timep++ - base_time);
            break;
       /* NetCDF4 extended data types */
        case CDS_INT64:
            while (--nelems) *data.i64p++ = (long long)(*timep++ - base_time);
            break;
        default:

            ERROR( CDS_LIB_NAME,
                "Could not set sample times for variable: %s\n"
                " -> unsupported time variable data type: %s\n",
                cds_get_object_path(var), cds_data_type_name(var->type));

            return(0);
    }

    return(1);
}

/**
 *  PRIVATE: Set the data values for a time variable.
 *
 *  The data type of the time variable must be CDS_SHORT, CDS_INT, CDS_INT64,
 *  CDS_FLOAT, or CDS_DOUBLE. If the variable data type is CDS_SHORT, CDS_INT,
 *  or CDS_INT64 any fractional seconds will be rounded in the conversion.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  var          - pointer to the variable
 *  @param  base_time    - base time value
 *  @param  sample_start - start sample (0 based indexing)
 *  @param  sample_count - number of samples in the times array
 *  @param  sample_times - pointer to the array of sample times
 *                         in seconds since 1970 UTC
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int _cds_set_sample_timevals(
    CDSVar    *var,
    time_t     base_time,
    size_t     sample_start,
    size_t     sample_count,
    timeval_t *sample_times)
{
    size_t     sample_size;
    size_t     nelems;
    CDSData    data;
    timeval_t *timevalp;
    double     dbl_val;

    /* Get the sample size of this variable */

    sample_size = cds_var_sample_size(var);
    if (!sample_size) {

        ERROR( CDS_LIB_NAME,
            "Could not set sample times for variable: %s\n"
            " -> static dimension has 0 length\n",
            cds_get_object_path(var));

        return(0);
    }

    /* Allocate memory for the new sample times */

    data.vp = cds_alloc_var_data(var, sample_start, sample_count);
    if (!data.vp) {
        return(0);
    }

    nelems   = sample_count * sample_size + 1;
    timevalp = sample_times;

    switch (var->type) {

        case CDS_SHORT:
            while (--nelems) {

                dbl_val = (double)(timevalp->tv_sec - base_time)
                        + (double)timevalp->tv_usec * 1E-6;

                if (dbl_val < 0) *data.sp++ = (short)(dbl_val - 0.5);
                else             *data.sp++ = (short)(dbl_val + 0.5);

                ++timevalp;
            }
            break;
        case CDS_INT:
            while (--nelems) {

                dbl_val = (double)(timevalp->tv_sec - base_time)
                        + (double)timevalp->tv_usec * 1E-6;

                if (dbl_val < 0) *data.ip++ = (int)(dbl_val - 0.5);
                else             *data.ip++ = (int)(dbl_val + 0.5);

                ++timevalp;
            }
            break;
        case CDS_FLOAT:
            while (--nelems) {

                dbl_val = (double)(timevalp->tv_sec - base_time)
                        + (double)timevalp->tv_usec * 1E-6;

                *data.fp++ = (float)dbl_val;

                ++timevalp;
            }
            break;
        case CDS_DOUBLE:
            while (--nelems) {

                *data.dp++ = (double)(timevalp->tv_sec - base_time)
                           + (double)timevalp->tv_usec * 1E-6;

                ++timevalp;
            }
            break;
        /* NetCDF4 extended data types */
        case CDS_INT64:
            while (--nelems) {

                dbl_val = (double)(timevalp->tv_sec - base_time)
                        + (double)timevalp->tv_usec * 1E-6;

                if (dbl_val < 0) *data.i64p++ = (long long)(dbl_val - 0.5);
                else             *data.i64p++ = (long long)(dbl_val + 0.5);

                ++timevalp;
            }
            break;
        default:

            ERROR( CDS_LIB_NAME,
                "Could not set sample times for variable: %s\n"
                " -> unsupported time variable data type: %s\n",
                cds_get_object_path(var), cds_data_type_name(var->type));

            return(0);
    }

    return(1);
}

/**
 *  PRIVATE: Set the base time for a time variable.
 *
 *  This function will set the "units" attribute to the string representation
 *  of the base_time value, and the "long_name" attribute will be set to the
 *  specified value. Any existing variable data will also be adjusted for the
 *  new base time value. If a bounds variable exists and has a units attribute,
 *  its units string will be updated appropriately.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  var       - pointer to the variable
 *  @param  long_name - long_name attribute value
 *  @param  units     - units attribute value
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int _cds_set_base_time(
    CDSVar     *var,
    const char *long_name,
    const char *units)
{
    CDSAtt *att;
    size_t  length;

    /* Set/Change the long_name if necessary */

    att = cds_get_att(var, "long_name");
    if (att) {
        if (!att->def_lock) {
            if (!cds_change_att_text(att, "%s", long_name)) {
                return(0);
            }
        }
    }
    else if (!var->def_lock) {
        att = cds_define_att_text(var, "long_name", "%s", long_name);
        if (!att) {
            return(0);
        }
    }

    /* Set/Change the units if necessary */

    if (!cds_change_var_units(var, var->type, units)) {
        return(0);
    }

    if (!cds_get_att(var, "units")) {
        length = strlen(units) + 1;
        if (!_cds_define_att(var, "units", CDS_CHAR, length, (void *)units)) {
            return(0);
        }
    }

    return(1);
}

/**
 *  PRIVATE: Update a base_time variable.
 *
 *  This function will set the the data value and define or update the
 *  "string" attribute to reflect the base_time value.  The "long_name"
 *  and "units" attributes will also be defined or updated if they do
 *  not exist or thier values have not already been defined.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  var       - pointer to the CDS variable
 *  @param  base_time - base time in seconds since 1970
 *  @param  string    - string attribute value
 *  @param  long_name - long_name attribute value
 *  @param  units     - units attribute value
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int _cds_update_base_time_var(
    CDSVar     *var,
    time_t      base_time,
    const char *string,
    const char *long_name,
    const char *units)
{
    int    base_time_int = (int)base_time;
    size_t length;

    if (!cds_set_var_data(var, CDS_INT, 0, 1, NULL, (void *)&base_time_int)) {
        return(0);
    }

    length = strlen(string) + 1;
    if (!cds_change_att(
        var, 1, "string", CDS_CHAR, length, (void *)string)) {
        return(0);
    }

    length = strlen(long_name) + 1;
    if (!cds_change_att(
        var, 1, "long_name", CDS_CHAR, length, (void *)long_name)) {
        return(0);
    }

    length = strlen(units) + 1;
    if (!cds_change_att(
        var, 1, "units", CDS_CHAR, length, (void *)units)) {
        return(0);
    }

    return(1);
}

/*******************************************************************************
 *  Public Functions
 */
/** @publicsection */

/**
 *  Convert base time to a units string.
 *
 *  The units_string must be large enough to hold at least 39
 *  characters and will be of the form:
 *
 *      seconds since YYYY-MM-DD hh:mm:ss 0:00
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  base_time    - seconds since 1970 UTC
 *  @param  units_string - pointer to the string to store the result in
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int cds_base_time_to_units_string(time_t base_time, char *units_string)
{
    struct tm gmt;

    *units_string = '\0';

    memset(&gmt, 0, sizeof(struct tm));

    if (!gmtime_r(&base_time, &gmt)) {

        ERROR( CDS_LIB_NAME,
            "Could not convert base time to units string.\n"
            " -> gmtime error: %s\n", strerror(errno));

        return(0);
    }

    gmt.tm_year += 1900;
    gmt.tm_mon++;

    sprintf(units_string,
        "seconds since %04d-%02d-%02d %02d:%02d:%02d 0:00",
        gmt.tm_year,
        gmt.tm_mon,
        gmt.tm_mday,
        gmt.tm_hour,
        gmt.tm_min,
        gmt.tm_sec);

    return(1);
}

/**
 *  Convert a time units string to a base time.
 *
 *  This is a wrapper function for cds_validate_time_units() that
 *  will generate an error if the format of the units_string could
 *  not be determined.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  units_string - pointer to the units string
 *  @param  base_time    - output: seconds since 1970 UTC
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int cds_units_string_to_base_time(char *units_string, time_t *base_time)
{
    *base_time = cds_validate_time_units(units_string);

    if (*base_time < 0) {

        if (*base_time == -1) {

            ERROR( CDS_LIB_NAME,
                "Could not convert units string to base time.\n"
                " -> invalid time units format: '%s'\n", units_string);
        }

        return(0);
    }

    return(1);
}

/**
 *  Find an index in an array of times.
 *
 *  @param  ntimes   - number of times
 *  @param  times    - array of times
 *  @param  ref_time - reference time for search
 *  @param  mode     - search mode
 *
 *  Search Modes:
 *
 *    - CDS_EQ   = find the index of the first time that is equal to
 *                 the reference time.
 *
 *    - CDS_LT   = find the index of the last time that is less than
 *                 the reference time.
 *
 *    - CDS_LTEQ = find the index of the last time that is less than
 *                 or equal to the specified time.
 *
 *    - CDS_GT   = find the index of the first time that is greater than
 *                 the reference time.
 *
 *    - CDS_GTEQ = find the index of the first time that is greater than
 *                 or equal to the specified time.
 *
 *  @return
 *    - index of the requested time value
 *    - -1 if not found
 */
int cds_find_time_index(
    size_t   ntimes,
    time_t  *times,
    time_t   ref_time,
    int      mode)
{
    size_t bi, ei, mi;

    /* Find the indexes that bracket the requested index */

    bi = 0;
    ei = ntimes-1;

    if (ref_time < times[bi]) {
        ei = bi;
    }
    else if (ref_time >= times[ei]) {
        bi = ei;
    }
    else {

        /* Find bi and ei such that:
         *
         * time[bi] <= ref_time < time[ei] */

        while (ei > bi + 1) {

            mi = (bi + ei)/2;

            if (ref_time < times[mi])
                ei = mi;
            else
                bi = mi;
        }
    }

    /* Return requested index */

    switch (mode) {

        case CDS_GTEQ:

            /* We want the index of the first time that is >= the ref_time */

            if (ref_time >  times[ei]) return(-1);
            if (ref_time != times[bi]) return(ei);

            /* find first time equal to the ref_time */

            if (ref_time == times[0])  return(0);
            while (ref_time == times[--bi]);
            return(bi+1);

        case CDS_GT:

            /* We want the index of the first time that is > the ref_time */

            if (ref_time >= times[ei]) return(-1);
            return(ei);

        case CDS_LTEQ:

            /* We want the index of the last time that is <= the ref_time */

            if (ref_time < times[bi])  return(-1);
            return(bi);

        case CDS_LT:

            /* We want the index of the last time that is < the ref_time */

            if (ref_time <= times[0])  return(-1);
            if (ref_time != times[bi]) return(bi);

            /* find the last time that is < the ref_time */

            while (ref_time == times[--bi]);
            return(bi);

        case CDS_EQ:

            /* We want the index of the first time that is == the ref_time */

            if (ref_time != times[bi]) return(-1);

            /* find first time equal to the ref_time */

            if (ref_time == times[0])  return(0);
            while (ref_time == times[--bi]);
            return(bi+1);

        default:
            return(-1);
    }
}

/**
 *  Find an index in an array of timevals.
 *
 *  @param  ntimes   - number of times
 *  @param  times    - array of times
 *  @param  ref_time - reference time for search
 *  @param  mode     - search mode
 *
 *  Search Modes:
 *
 *    - CDS_EQ   = find the index of the first time that is equal to
 *                 the reference time.
 *
 *    - CDS_LT   = find the index of the last time that is less than
 *                 the reference time.
 *
 *    - CDS_LTEQ = find the index of the last time that is less than
 *                 or equal to the specified time.
 *
 *    - CDS_GT   = find the index of the first time that is greater than
 *                 the reference time.
 *
 *    - CDS_GTEQ = find the index of the first time that is greater than
 *                 or equal to the specified time.
 *
 *  @return
 *    - index of the requested time value
 *    - -1 if not found
 */
int cds_find_timeval_index(
    size_t     ntimes,
    timeval_t *times,
    timeval_t  ref_time,
    int        mode)
{
    size_t bi, ei, mi;

    /* Find the indexes that bracket the requested index */

    bi = 0;
    ei = ntimes-1;

    if (TV_LT(ref_time, times[bi])) {
        ei = bi;
    }
    else if (TV_GTEQ(ref_time, times[ei])) {
        bi = ei;
    }
    else {

        /* Find bi and ei such that:
         *
         * time[bi] <= ref_time < time[ei] */

        while (ei > bi + 1) {

            mi = (bi + ei)/2;

            if (TV_LT(ref_time, times[mi]))
                ei = mi;
            else
                bi = mi;
        }
    }

    /* Return requested index */

    switch (mode) {

        case CDS_GTEQ:

            /* We want the index of the first time that is >= the ref_time */

            if (TV_GT(ref_time, times[ei]))  return(-1);
            if (TV_NEQ(ref_time, times[bi])) return(ei);

            /* find first time equal to the ref_time */

            if (TV_EQ(ref_time, times[0]))  return(0);
            for (--bi; TV_EQ(ref_time, times[bi]); --bi);
            return(bi+1);

        case CDS_GT:

            /* We want the index of the first time that is > the ref_time */

            if (TV_GTEQ(ref_time, times[ei])) return(-1);
            return(ei);

        case CDS_LTEQ:

            /* We want the index of the last time that is <= the ref_time */

            if (TV_LT(ref_time, times[bi]))  return(-1);
            return(bi);

        case CDS_LT:

            /* We want the index of the last time that is < the ref_time */

            if (TV_LTEQ(ref_time, times[0])) return(-1);
            if (TV_NEQ(ref_time, times[bi])) return(bi);

            /* find the last time that is < the ref_time */

            for (--bi; TV_EQ(ref_time, times[bi]); --bi);
            return(bi);

        case CDS_EQ:

            /* We want the index of the first time that is == the ref_time */

            if (TV_NEQ(ref_time, times[bi])) return(-1);

            /* find first time equal to the ref_time */

            if (TV_EQ(ref_time, times[0]))  return(0);
            for (--bi; TV_EQ(ref_time, times[bi]); --bi);
            return(bi+1);

        default:
            return(-1);
    }
}

/**
 *  Find the CDS time variable.
 *
 *  This function will find the first CDS group that contains either
 *  the "time" or "time_offset" variable and return a pointer to that
 *  variable.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  object - pointer to the CDS object
 *
 *  @return
 *    - pointer to the time variable
 *    - NULL if not found
 */
CDSVar *cds_find_time_var(void *object)
{
    CDSObject *obj = (CDSObject *)object;
    CDSVar    *var = (CDSVar *)NULL;

    while (obj && obj->obj_type != CDS_GROUP) {
        obj = obj->parent;
    }

    if (!obj) {
        return((CDSVar *)NULL);
    }

    while (obj && !var) {

        var = cds_get_var((CDSGroup *)obj, "time");
        if (!var) {
            var = cds_get_var((CDSGroup *)obj, "time_offset");
        }

        if (var) break;

        obj = obj->parent;
    }

    return(var);
}

/**
 *  Get the base time of a CDS group or time variable.
 *
 *  This function will convert the units attribute of a time variable to
 *  seconds since 1970.  If the input object is a CDSGroup, the specified
 *  group and then its parent groups will be searched until a "time" or
 *  "time_offset" variable is found.
 *
 *  @param  object - pointer to the CDSGroup or CDSVar
 *
 *  @return
 *    - base time in seconds since 1970 UTC
 *    - -1 if a base time was not found
 */
time_t cds_get_base_time(void *object)
{
    CDSObject *obj = (CDSObject *)object;
    CDSVar    *var;
    time_t     base_time;
    const char *units;

    if (obj->obj_type == CDS_VAR) {
        var = (CDSVar *)object;
    }
    else {
        var = cds_find_time_var(object);
        if (!var) return(-1);
    }

    units = cds_get_var_units(var);
    if (!units || strlen(units) == 0) {
        return(-1);
    }

    if (!cds_units_string_to_base_time((char *)units, &base_time)) {
        return(-1);
    }

    return(base_time);
}

/**
 *  Get the time of midnight just prior to the data time.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  data_time - seconds since 1970 UTC
 *
 *  @return
 *    - midnight in seconds since 1970 UTC
 *    - 0 if an error occurred
 */
time_t cds_get_midnight(time_t data_time)
{
    struct tm gmt;
    time_t    midnight;

    memset(&gmt, 0, sizeof(struct tm));

    if (!gmtime_r(&data_time, &gmt)) {

        ERROR( CDS_LIB_NAME,
            "Could not get time of midnight just prior to data time.\n"
            " -> gmtime error: %s\n", strerror(errno));

        return(0);
    }

    gmt.tm_hour = 0;
    gmt.tm_min  = 0;
    gmt.tm_sec  = 0;

    midnight  = mktime(&gmt);
    midnight -= timezone;

    return(midnight);
}

/**
 *  Get the time range of a CDS group or time variable.
 *
 *  This function will get the start and end times of a time variable.
 *  If the input object is a CDSGroup, the specified group and then
 *  its parent groups will be searched until a "time" or "time_offset"
 *  variable is found.
 *
 *  @param  object       - pointer to the CDSGroup or CDSVar
 *  @param  start_time   - pointer to the timeval_t structure to store the
 *                         start time in.
 *  @param  end_time     - pointer to the timeval_t structure to store the
 *                         end time in.
 *
 *  @return
 *    - number of time values
 *    - 0 if no time values were found
 */
size_t cds_get_time_range(
    void      *object,
    timeval_t *start_time,
    timeval_t *end_time)
{
    CDSObject *obj = (CDSObject *)object;
    CDSVar    *var;
    size_t     ntimes;
    size_t     count;

    start_time->tv_sec  = 0;
    start_time->tv_usec = 0;
    end_time->tv_sec    = 0;
    end_time->tv_usec   = 0;

    if (obj->obj_type == CDS_VAR) {
        var = (CDSVar *)object;
    }
    else {
        var = cds_find_time_var(object);
        if (!var) {
            return(0);
        }
    }

    ntimes = var->sample_count;

    if (ntimes > 0) {

        count = 1;
        _cds_get_sample_timevals(var, 0, &count, start_time);

        count = 1;
        _cds_get_sample_timevals(var, ntimes-1, &count, end_time);
    }

    return(ntimes);
}

/**
 *  Get the sample times of a CDS group or time variable.
 *
 *  This function will convert the data values of a time variable to seconds
 *  since 1970.  If the input object is a CDSGroup, the specified group and
 *  then its parent groups will be searched until a "time" or "time_offset"
 *  variable is found.
 *
 *  Memory will be allocated for the returned array of sample times if the
 *  output array is NULL. In this case the calling process is responsible
 *  for freeing the allocated memory.
 *
 *  Note: If the sample times can have fractional seconds the
 *  cds_get_sample_timevals() function should be used instead.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  object       - pointer to the CDSGroup or CDSVar
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
time_t *cds_get_sample_times(
    void   *object,
    size_t  sample_start,
    size_t *sample_count,
    time_t *sample_times)
{
    CDSObject *obj = (CDSObject *)object;
    CDSVar    *var;

    if (obj->obj_type == CDS_VAR) {
        var = (CDSVar *)object;
    }
    else {
        var = cds_find_time_var(object);
        if (!var) {
            if (sample_count) *sample_count = 0;
            return((time_t *)NULL);
        }
    }

    return(_cds_get_sample_times(
        var, sample_start, sample_count, sample_times));
}

/**
 *  Get the sample times of a CDS group or time variable.
 *
 *  This function will convert the data values of a time variable to an
 *  array of timeval_t structures.  If the input object is a CDSGroup,
 *  the specified group and then its parent groups will be searched until
 *  a "time" or "time_offset" variable is found.
 *
 *  Memory will be allocated for the returned array of sample times if the
 *  output array is NULL. In this case the calling process is responsible
 *  for freeing the allocated memory.
 *
 *  Note: Consider using the cds_get_sample_times() function if the
 *  sample times can not have fractional seconds.
 *
 *  Error messages from this function are sent to the message
 *  handler (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  object       - pointer to the CDSGroup or CDSVar
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
timeval_t *cds_get_sample_timevals(
    void      *object,
    size_t     sample_start,
    size_t    *sample_count,
    timeval_t *sample_times)
{
    CDSObject *obj = (CDSObject *)object;
    CDSVar    *var;

    if (obj->obj_type == CDS_VAR) {
        var = (CDSVar *)object;
    }
    else {
        var = cds_find_time_var(object);
        if (!var) {
            if (sample_count) *sample_count = 0;
            return((timeval_t *)NULL);
        }
    }

    return(_cds_get_sample_timevals(
        var, sample_start, sample_count, sample_times));
}

/**
 *  Check if a variable is one of the standard time variables.
 *
 *  Standard time variables are:
 *
 *    - time
 *    - time_offset
 *    - base_time
 *
 *  @param  var          - pointer to the variable
 *  @param  is_base_time - output: flag indicating a base_time variable
 *
 *  @return
 *    - 1 if this is a standard time variable
 *    - 0 if this is not a standard time variable
 */
int cds_is_time_var(CDSVar *var, int *is_base_time)
{
    if ((strcmp(var->name, "time")        == 0) ||
        (strcmp(var->name, "time_offset") == 0)) {

        *is_base_time = 0;
        return(1);
    }

    if ((strcmp(var->name, "base_time") == 0)) {

        *is_base_time = 1;
        return(1);
    }

    return(0);
}

/**
 *  Set the base time of a CDS group or time variable.
 *
 *  This function will set the base time for a time variable and adjust all
 *  attributes and data values as necessary. If the input object is one of the
 *  standard time variables ("time", "time_offset", and "base_time"), all
 *  standard time variables that exist in its parent group will also be updated.
 *  If the input object is a CDSGroup, the specified group and then its parent
 *  groups will be searched until a "time" or "time_offset" variable is found.
 *  All standard time variables that exist in this group will then be updated.
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
 *  If a bounds variable exists and has a units attribute, its units string
 *  will be updated appropriately.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  object    - pointer to the CDS object
 *  @param  long_name - string to use for the long_name attribute,
 *                      or NULL to use the default value
 *  @param  base_time - base time in seconds since 1970
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int cds_set_base_time(void *object, const char *long_name, time_t base_time)
{
    CDSObject  *obj              = (CDSObject *)object;
    const char *base_time_desc   = "Base time in Epoch";
    const char *base_time_units  = "seconds since 1970-1-1 0:00:00 0:00";
    const char *time_offset_desc = "Time offset from base_time";
    char        units_string[64];
    CDSGroup   *group;
    CDSVar     *var;

    if (!long_name) {
        if (base_time == cds_get_midnight(base_time)) {
            long_name = "Time offset from midnight";
        }
        else {
            long_name = "Sample times";
        }
    }

    /* Convert base time to units string */

    if (!cds_base_time_to_units_string(base_time, units_string)) {
        return(0);
    }

    /* Check if this is a non-standard time variable
     * or get the parent group of the time variables */

    if (obj->obj_type == CDS_VAR) {

        var = (CDSVar *)object;

        if ((strcmp(var->name, "base_time")   != 0) &&
            (strcmp(var->name, "time")        != 0) &&
            (strcmp(var->name, "time_offset") != 0)) {

            if (!_cds_set_base_time(var, long_name, units_string)) {
                return(0);
            }

            return(1);
        }
    }
    else {

        var = cds_find_time_var(object);

        if (!var) {

            ERROR( CDS_LIB_NAME,
                "Could not set base time for: %s\n"
                " -> time variable not found\n",
                cds_get_object_path(object));

            return(0);
        }
    }

    group = (CDSGroup *)var->parent;

    /* Update the base_time variable if it exists */

    var = cds_get_var(group, "base_time");
    if (var) {
        if (!_cds_update_base_time_var(var, base_time,
            &units_string[14], base_time_desc, base_time_units)) {

            return(0);
        }
    }

    /* Update the time_offset variable if it exists */

    var = cds_get_var(group, "time_offset");
    if (var) {
        if (!_cds_set_base_time(var, time_offset_desc, units_string)) {
            return(0);
        }
    }

    /* Update the time variable if it exists */

    var = cds_get_var(group, "time");
    if (var) {
        if (!_cds_set_base_time(var, long_name, units_string)) {
            return(0);
        }
    }

    return(1);
}

/**
 *  Set the sample times for a CDS time variable or group.
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
 *  All standard time variables that exist in its parent group will also be
 *  updated.
 *
 *  If the input object is a CDSGroup, the specified group and then its parent
 *  groups will be searched until a "time" or "time_offset" variable is found.
 *  All standard time variables that exist in this group will then be updated.
 *
 *  If the specified sample_start value is 0 and a base time value has not been
 *  set, the cds_set_base_time() function will be called using the time of
 *  midnight just prior to the first sample time.
 *
 *  The data type of the time variable(s) must be CDS_SHORT, CDS_INT, CDS_INT64,
 *  CDS_FLOAT or CDS_DOUBLE. However, CDS_DOUBLE is usually recommended.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  object       - pointer to the CDS object
 *  @param  sample_start - start sample (0 based indexing)
 *  @param  sample_count - number of samples in the times array
 *  @param  sample_times - pointer to the array of sample times
 *                         in seconds since 1970 UTC.
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int cds_set_sample_times(
    void    *object,
    size_t   sample_start,
    size_t   sample_count,
    time_t  *sample_times)
{
    CDSObject  *obj       = (CDSObject *)object;
    const char *long_name = "Time offset from midnight";
    CDSVar     *var;
    CDSGroup   *group;
    time_t      base_time;

    /* Check if we need to set the base time value */

    base_time = cds_get_base_time(object);
    if ((base_time < 0) && sample_start == 0) {
        base_time = cds_get_midnight(sample_times[0]);
        if (!cds_set_base_time(object, long_name, base_time)) {
            return(0);
        }
    }

    /* Check if this is a non-standard time variable
     * or get the parent group of the time variables */

    if (obj->obj_type == CDS_VAR) {

        var = (CDSVar *)object;

        if ((strcmp(var->name, "base_time")   != 0) &&
            (strcmp(var->name, "time")        != 0) &&
            (strcmp(var->name, "time_offset") != 0)) {

            if (!_cds_set_sample_times(
                var, base_time, sample_start, sample_count, sample_times)) {

                return(0);
            }

            return(1);
        }
    }
    else {

        var = cds_find_time_var(object);

        if (!var) {

            ERROR( CDS_LIB_NAME,
                "Could not set sample times for: %s\n"
                " -> time variable not found\n",
                cds_get_object_path(object));

            return(0);
        }
    }

    group = (CDSGroup *)var->parent;

    /* Set the data values in the time variable */

    var = cds_get_var(group, "time");
    if (var) {

        if (!_cds_set_sample_times(
            var, base_time, sample_start, sample_count, sample_times)) {

            return(0);
        }
    }

    /* Set the data values in the time_offset variable */

    var = cds_get_var(group, "time_offset");
    if (var) {

        if (!_cds_set_sample_times(
            var, base_time, sample_start, sample_count, sample_times)) {

            return(0);
        }
    }

    return(1);
}

/**
 *  Set the sample times for a CDS time variable or group.
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
 *  All standard time variables that exist in its parent group will also be
 *  updated.
 *
 *  If the input object is a CDSGroup, the specified group and then its parent
 *  groups will be searched until a "time" or "time_offset" variable is found.
 *  All standard time variables that exist in this group will then be updated.
 *
 *  If the specified sample_start value is 0 and a base time value has not been
 *  set, the cds_set_base_time() function will be called using the time of
 *  midnight just prior to the first sample time.
 *
 *  The data type of the time variable must be CDS_SHORT, CDS_INT, CDS_INT64,
 *  CDS_FLOAT, or CDS_DOUBLE. If the variable data type is CDS_SHORT, CDS_INT,
 *  or CDS_INT64 any fractional seconds will be rounded in the conversion.
 *
 *  Error messages from this function are sent to the message handler
 *  (see msngr_init_log() and msngr_init_mail()).
 *
 *  @param  object       - pointer to the CDS object
 *  @param  sample_start - start sample (0 based indexing)
 *  @param  sample_count - number of samples in the times array
 *  @param  sample_times - pointer to the array of sample times
 *                         in seconds since 1970 UTC.
 *
 *  @return
 *    - 1 if successful
 *    - 0 if an error occurred
 */
int cds_set_sample_timevals(
    void      *object,
    size_t     sample_start,
    size_t     sample_count,
    timeval_t *sample_times)
{
    CDSObject  *obj       = (CDSObject *)object;
    const char *long_name = "Time offset from midnight";
    CDSVar     *var;
    CDSGroup   *group;
    time_t      base_time;

    /* Check if we need to set the base time value */

    base_time = cds_get_base_time(object);
    if ((base_time < 0) && sample_start == 0) {
        base_time = cds_get_midnight(sample_times[0].tv_sec);
        if (!cds_set_base_time(object, long_name, base_time)) {
            return(0);
        }
    }

    /* Check if this is a non-standard time variable
     * or get the parent group of the time variables */

    if (obj->obj_type == CDS_VAR) {

        var = (CDSVar *)object;

        if ((strcmp(var->name, "base_time")   != 0) &&
            (strcmp(var->name, "time")        != 0) &&
            (strcmp(var->name, "time_offset") != 0)) {

            if (!_cds_set_sample_timevals(
                var, base_time, sample_start, sample_count, sample_times)) {

                return(0);
            }

            return(1);
        }
    }
    else {

        var = cds_find_time_var(object);

        if (!var) {

            ERROR( CDS_LIB_NAME,
                "Could not set sample times for: %s\n"
                " -> time variable not found\n",
                cds_get_object_path(object));

            return(0);
        }
    }

    group = (CDSGroup *)var->parent;

    /* Set the data values in the time variable */

    var = cds_get_var(group, "time");
    if (var) {

        if (!_cds_set_sample_timevals(
            var, base_time, sample_start, sample_count, sample_times)) {

            return(0);
        }
    }

    /* Set the data values in the time_offset variable */

    var = cds_get_var(group, "time_offset");
    if (var) {

        if (!_cds_set_sample_timevals(
            var, base_time, sample_start, sample_count, sample_times)) {

            return(0);
        }
    }

    return(1);
}
