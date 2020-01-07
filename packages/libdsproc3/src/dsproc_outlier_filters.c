/*******************************************************************************
*
*  COPYRIGHT (C) 2012 Battelle Memorial Institute.  All Rights Reserved.
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
*    $Revision: 77895 $
*    $Author: ermold $
*    $Date: 2017-05-05 19:28:15 +0000 (Fri, 05 May 2017) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file dsproc_outlier_filters.c
*  Outlier Filters.
*/

#include <math.h>

#include "dsproc3.h"
#include "dsproc_private.h"

/*
http://docs.oracle.com/cd/E17236_01/epm.1112/cb_statistical/frameset.htm?ch07s02s10s01.html

Mean and Standard Deviation Method

For this outlier detection method, the mean and standard deviation of the residuals are calculated and compared. If a value is a certain number of standard deviations away from the mean, that data point is identified as an outlier. The specified number of standard deviations is called the threshold. The default value is 3.

This method can fail to detect outliers because the outliers increase the standard deviation. The more extreme the outlier, the more the standard deviation is affected.


Median and Median Absolute Deviation Method (MAD)

For this outlier detection method, the median of the residuals is calculated. Then, the difference is calculated between each historical value and this median. These differences are expressed as their absolute values, and a new median is calculated and multiplied by an empirically derived constant to yield the median absolute deviation (MAD). If a value is a certain number of MAD away from the median of the residuals, that value is classified as an outlier. The default threshold is 3 MAD.

This method is generally more effective than the mean and standard deviation method for detecting outliers, but it can be too aggressive in classifying values that are not really extremely different. Also, if more than 50% of the data points have the same value, MAD is computed to be 0, so any value different from the residual median is classified as an outlier.


Median and Interquartile Deviation Method (IQD)

For this outlier detection method, the median of the residuals is calculated, along with the 25th percentile and the 75th percentile. The difference between the 25th and 75th percentile is the interquartile deviation (IQD). Then, the difference is calculated between each historical value and the residual median. If the historical value is a certain number of MAD away from the median of the residuals, that value is classified as an outlier. The default threshold is 2.22, which is equivalent to 3 standard deviations or MADs.

This method is somewhat susceptible to influence from extreme outliers, but less so than the mean and standard deviation method. Box plots are based on this approach. The median and interquartile deviation method can be used for both symmetric and asymmetric data.
*/

/** @privatesection */

/**
 *  qsort function to compare two doubles.
 */
static int _dbl_compare(const void *dp1, const void *dp2)
{
    double diff = *((double *)dp1) - *((double *)dp2);
    return((diff < 0.0) ? -1 : (diff > 0.0));
}

/**
*  Validate input and get data and pointers for an outlier filter.
*
*  The memory used by bar_datap is dynamically allocated and must be freed
*  by the calling process when it is no longer needed.
*
*  @param   dataset      pointer to the dataset
*  @param   var_name     name of the variable
*  @param   nsamples     output: number of samples
*  @param   times        output: pointer to time variable's data array
*  @param   datap        output: pointer to the variable's data cast to double
*  @param   missing      output: missing value used in variable data
*  @param   qcp          output: pointer to the QC variable's data array
*  @param   qc_bad_mask  output: QC mask used to check for bad values
*
*  @retval   1  successful
*  @retval   0  if the outlier test can not be run
*/
static int _init_outlier_filter(
    CDSGroup      *dataset,
    const char    *var_name,
    size_t        *nsamples,
    double       **times,
    double       **datap,
    double        *missing,
    unsigned int **qcp,
    unsigned int  *qc_bad_mask)
{
    const char *errmsg;
    CDSVar     *var;
    CDSVar     *time_var;
    CDSVar     *qc_var;

    /* Get variable and check shape */

    var = cds_get_var(dataset, var_name);
    if (!var) {
        errmsg = "variable not found in dataset";
        goto INVALID_INPUT;
    }

    if (var->ndims != 1) {
        errmsg = "variable has more than one dimesnion";
        goto INVALID_INPUT;
    }

    if (strcmp(var->dims[0]->name, "time") != 0) {
        errmsg = "variable is not dimensioned by time";
        goto INVALID_INPUT;
    }

    /* Get time variable and check type and sample counts */

    time_var = cds_get_var(dataset, "time");
    if (!time_var) {
        errmsg = "time variable not found in dataset";
        goto INVALID_INPUT;
    }

    if (time_var->type != CDS_DOUBLE) {
        errmsg = "invalid time variable data type (expected double)";
        goto INVALID_INPUT;
    }

    if (time_var->sample_count == 0) {
        errmsg = "no sample times have been stored in dataset";
        goto INVALID_INPUT;
    }

    if (time_var->sample_count != var->sample_count) {
        errmsg = "number of sample times does not match number of data values";
        goto INVALID_INPUT;
    }

    /* Get QC variable and intialize data values if necessary */

    qc_var = dsproc_get_qc_var(var);
    if (!qc_var) {
        errmsg = "QC variable not found in dataset";
        goto INVALID_INPUT;
    }

    if (qc_var->type != CDS_INT) {
        errmsg = "invalid QC variable data type (expected int)";
        goto INVALID_INPUT;
    }

    if (qc_var->sample_count == 0) {
        if (!dsproc_init_var_data(qc_var, 0, var->sample_count, 0)) {
            return(0);
        }
    }
    else if (qc_var->sample_count != var->sample_count) {
        errmsg = "number of QC values does not match number of data values";
        goto INVALID_INPUT;
    }

    /* Get variable data cast to double */

    *datap = dsproc_get_var_data(var,
        CDS_DOUBLE, 0, nsamples, missing, NULL);

    if (!(*datap)) {
        return(0);
    }

    /* Set outputs */

    *times       = time_var->data.dp;
    *qcp         = (unsigned int *)qc_var->data.ip;
    *qc_bad_mask = dsproc_get_bad_qc_mask(qc_var);

    return(1);

INVALID_INPUT:

    ERROR( DSPROC_LIB_NAME,
        "Could not run outlier test for variable: %s:%s\n"
        " -> %s\n",
        dataset->name, var_name, errmsg);

    dsproc_set_status("Could Not Run Outlier Test");

    return(0);
}

/** @publicsection */

/*******************************************************************************
 *  Functions Visible To The Public
 */

/**
*  Flag outliers using the Median and Interquartile Deviation Method (IQD).
*
*  For this outlier detection method, the median of the residuals is
*  calculated, along with the 25th percentile and the 75th percentile. The
*  difference between the 25th and 75th percentile is the interquartile
*  deviation (IQD). Then, the difference is calculated between each
*  value and the residual median. If the value is a certain number of IQD
*  away from the median of the residuals, that value is classified as an
*  outlier. A typical default value for the threshold is 3.
*
*  The 'analyze' option can be used to analyze the outlier detection results
*  during development and help determine the best window width and threshold
*  to use.  Remember to set this value to 0 before creating a production
*  release.  Available options are:
*
*    - 1 = prints ranges of interquartile deviations from the median,
*          and the number of points that fall within each range.
*
*    - 2 = for each point prints time offset, data value, window median,
*          deviation from the median, interquartile deviation, and number
*          of interquartile deviations the value is from the median.
*
*  If an error occurs in this function it will be appended to the log and
*  error mail messages, and the process status will be set appropriately.
*
*  @param   dataset        pointer to the dataset
*  @param   var_name       name of the variable
*  @param   window_width   width of window centered on data point (in seconds)
*  @param   min_npoints    minimum number of values within window required
*                          to perform the test (default is 2).
*  @param   skipped_flag   QC flag value to use for values that do not have
*                          enough points in window to perform the test.
*  @param   bad_threshold  IQD factor used to flag outliers as bad
*  @param   bad_flag       QC flag value to use for bad outliers
*  @param   ind_threshold  IQD factor used to flag outliers as indeterminate
*  @param   ind_flag       QC flag value to use for indeterminate outliers
*  @param   analyze        Print statistics that may be helpfull during
*                          development (see above).
*
*  @retval   1  successful
*  @retval   0  a fatal error occurred
*/
int dsproc_flag_outliers_iqd(
    CDSGroup     *dataset,
    const char   *var_name,
    double        window_width,
    int           min_npoints,
    unsigned int  skipped_flag,
    double        bad_threshold,
    unsigned int  bad_flag,
    double        ind_threshold,
    unsigned int  ind_flag,
    unsigned int  analyze)
{
    double        half_width = window_width / 2;
    size_t        nsamples;
    double       *times;
    double       *datap;
    double        missing;
    unsigned int *qcp;
    unsigned int  qc_bad_mask;
    unsigned int *new_qc;
    double       *buffer;
    int           bn, qi, qn;
    double        Q1, Q2, Q3, median, iqd, dev;
    size_t        ei, si, ti, wi;
    int           status;

    const char *method = "Median and Interquartile Deviation Method (IQD)";
    int         nanalyze_bins = 60;
    int         analyze_bins[nanalyze_bins];
    int         abi, nbad, nskipped;
    double      niqds;

    DEBUG_LV1( DSPROC_LIB_NAME,
        "%s:%s: Flagging Outliers using %s\n",
        dataset->name, var_name, method);

    if (analyze) {

        memset(analyze_bins, 0, nanalyze_bins * sizeof(int));

        printf(
            "\n"
            "Analyzing Outlier Detection using %s\n"
            "\n"
            " - dataset:       %s\n"
            " - variable:      %s\n"
            " - window width:  %g\n",
            method, dataset->name, var_name, window_width);

        if (analyze >= 2) {
            printf(
            " - bad threshold: %g\t\n"
            " - ind threshold: %g\t\n"
            "\n"
            "%15s\t%15s\t%15s\t%15s\t%15s\t%15s\n",
            bad_threshold,
            ind_threshold,
            "time_offset", "value", "median", "dev", "iqd", "dev/iqd");
        }
    }

    if (min_npoints < 2) {
        min_npoints = 2;
    }

    status = _init_outlier_filter(
        dataset,    var_name,
        &nsamples, &times,
        &datap,    &missing,
        &qcp,      &qc_bad_mask);

    if (status <= 0) return(0);

    if (nsamples < (size_t)min_npoints) {

        for (ti = 0; ti < nsamples; ++ti) {
            qcp[ti] |= skipped_flag;
        }
        
        free(datap);
        return(1);
    }

    if (!(buffer = malloc(nsamples * sizeof(double))) ||
        !(new_qc = calloc(nsamples, sizeof(unsigned int)))) {

        ERROR( DSPROC_LIB_NAME,
            "Could not run outlier test for variable: %s:%s\n"
            " -> memory allocation error\n",
            dataset->name, var_name);
            
        dsproc_set_status(DSPROC_ENOMEM);

        free(datap);
        return(0);
    }

    nbad = nskipped = 0;
    si = ei = 0;

    for (ti = 0; ti < nsamples; ++ti) {

        /* Skip bad and missing values */

        if (qcp[ti] & qc_bad_mask || datap[ti] == missing) {
            if (analyze) {
                nbad += 1;
                if (analyze >= 2) {
                    printf("%15.6f\t%15.6f\tbad\n",
                        times[ti], datap[ti]);
                }
            }
            continue;
        }

        /* Find start of window */

        while (times[ti] - times[si] > half_width) ++si;

        /* Find end of window */

        while ((ei < nsamples) && (times[ei] - times[ti]) <= half_width) ++ei;

        /* Find median of window */

        bn = 0;

        for (wi = si; wi < ei; ++wi) {
            if (!(qcp[wi] & qc_bad_mask) && (datap[wi] != missing)) {
                buffer[bn++] = datap[wi];
            }
        }

        if (bn < min_npoints) {
            new_qc[ti] |= skipped_flag;
            if (analyze) {
                nskipped += 1;
                if (analyze >= 2) {
                    printf("%15.6f\t%15.6f\tnot enough points (%d < %d)\n",
                        times[ti], datap[ti], bn, min_npoints);
                }
            }
            continue;
        }

        qsort(buffer, bn, sizeof(double), _dbl_compare);

        if (bn & 0x1) {
            /* odd number of points */
            qn = (bn-1)/2;
            Q2 = buffer[qn];
        }
        else {
            /* even number of points */
            qn = bn/2;
            Q2 = (buffer[qn-1] + buffer[qn])/2;
        }

        median = Q2;

        /* Find the first and third quartiles */

        if (qn & 0x1) {
            /* odd number of points */
            qi = (qn-1)/2;
            Q1 = buffer[qi];
            Q3 = buffer[bn - qi - 1];
        }
        else {
            /* even number of points */
            qi = qn/2;
            Q1 = (buffer[qi-1] + buffer[qi])/2;
            Q3 = (buffer[bn - qi - 1] + buffer[bn - qi])/2;
        }

        iqd = Q3 - Q1;
        
        /* Check for outlier */

        dev = fabs(datap[ti] - median);

        if (dev > bad_threshold * iqd) {
            new_qc[ti] |= bad_flag;
        }

        if (dev > ind_threshold * iqd) {
            new_qc[ti] |= ind_flag;
        }

        if (analyze) {

            if (dev == 0) {
                abi   = 0;
                niqds = 0;
            }
            else if (iqd == 0) {
                abi   = nanalyze_bins - 1;
                niqds = 1.0 / 0.0;
            }
            else {

                niqds = dev/iqd;
                abi   = (int)(niqds * 2);

                if ((niqds * 2) - abi == 0) abi -= 1;

                if (abi >= nanalyze_bins) {
                    abi = nanalyze_bins - 1;
                }
            }

            analyze_bins[abi] += 1;

            if (analyze >= 2) {

                printf("%15.6f\t%15.6f\t%15.6f\t%15.8f\t%15.8f\t%15.2f",
                    times[ti], datap[ti], median, dev, iqd, niqds);

                if      (new_qc[ti] & bad_flag) printf("\tBAD");
                else if (new_qc[ti] & ind_flag) printf("\tind");

                printf("\n");
            }
        }
    }

    for (ti = 0; ti < nsamples; ++ti) {
        qcp[ti] |= new_qc[ti];
    }

    if (analyze) {

        printf(
            "\n"
            " - skipped %d bad points\n"
            " - skipped %d points that had less than %d points in window\n"
            "\n"
            "%-12s\t%s\n",
            nbad, nskipped, min_npoints,
            "# of IQDs", "# of points\n");

        for (abi = 0; abi < nanalyze_bins; ++abi) {

            if (abi == nanalyze_bins-1) {
                printf("     >= %4.1f", (double)nanalyze_bins/2);
            }
            else {
                printf("%4.1f to %4.1f", (float)abi/2, (float)abi/2 + 0.5);
            }

            printf("\t%8d\n", analyze_bins[abi]);

        }
    }

    free(buffer);
    free(new_qc);
    free(datap);
    return(1);
}

/**
*  Flag outliers using the Absolute Deviation from the Mean Method.
*
*  For this outlier detection method values are flagged based on their
*  absolute deviation from the mean of the surrounding values.
*
*  The 'analyze' option can be used to analyze the outlier detection results
*  during development and help determine the best window width and threshold
*  to use.  Remember to set this value to 0 before creating a production
*  release.  Available options are:
*
*    - 1 = prints ranges of absolute deviations from the mean,
*          and the number of points that fall within each range.
*
*    - 2 = for each point prints time offset, data value, window mean,
*          and absolute deviation from the mean.
*
*  If an error occurs in this function it will be appended to the log and
*  error mail messages, and the process status will be set appropriately.
*
*  @param   dataset        pointer to the dataset
*  @param   var_name       name of the variable
*  @param   window_width   width of window centered on data point (in seconds)
*  @param   min_npoints    minimum number of values within window required
*                          to perform the test (default is 2).
*  @param   skipped_flag   QC flag value to use for values that do not have
*                          enough points in window to perform the test.
*  @param   bad_threshold  delta from mean used to flag outliers as bad
*  @param   bad_flag       QC flag value to use for bad outliers
*  @param   ind_threshold  delta from mean used to flag outliers as indeterminate
*  @param   ind_flag       QC flag value to use for indeterminate outliers
*  @param   analyze        Print statistics that may be helpfull during
*                          development (see above).
*
*  @retval   1  successful
*  @retval   0  a fatal error occurred
*/
int dsproc_flag_outliers_mean_dev(
    CDSGroup     *dataset,
    const char   *var_name,
    double        window_width,
    int           min_npoints,
    unsigned int  skipped_flag,
    double        bad_threshold,
    unsigned int  bad_flag,
    double        ind_threshold,
    unsigned int  ind_flag,
    unsigned int  analyze)
{
    double        half_width = window_width / 2;
    size_t        nsamples;
    double       *times;
    double       *datap;
    double        missing;
    unsigned int *qcp;
    unsigned int  qc_bad_mask;
    unsigned int *new_qc;
    double        sum, mean, delta;
    size_t        ei, si, ti, wi;
    int           n, status;

    const char *method = "Absolute Deviation from the Mean Method";
    int         nanalyze_bins = 60;
    int         analyze_bins[nanalyze_bins];
    int         abi, nbad, nskipped;

    DEBUG_LV1( DSPROC_LIB_NAME,
        "%s:%s: Flagging Outliers using %s\n",
        dataset->name, var_name, method);

    if (analyze) {

        memset(analyze_bins, 0, nanalyze_bins * sizeof(int));

        printf(
            "\n"
            "Analyzing Outlier Detection using %s\n"
            "\n"
            " - dataset:       %s\n"
            " - variable:      %s\n"
            " - window width:  %g\n",
            method, dataset->name, var_name, window_width);

        if (analyze >= 2) {
            printf(
            " - bad threshold: %g\t\n"
            " - ind threshold: %g\t\n"
            "\n"
            "%15s\t%15s\t%15s\t%15s\n",
            bad_threshold,
            ind_threshold,
            "time_offset", "value", "mean", "dev");
        }
    }

    if (min_npoints < 2) {
        min_npoints = 2;
    }

    status = _init_outlier_filter(
        dataset,    var_name,
        &nsamples, &times,
        &datap,    &missing,
        &qcp,      &qc_bad_mask);

    if (status <= 0) return(0);

    if (nsamples < (size_t)min_npoints) {

        for (ti = 0; ti < nsamples; ++ti) {
            qcp[ti] |= skipped_flag;
        }

        free(datap);
        return(1);
    }

    new_qc = calloc(nsamples, sizeof(unsigned int));
    if (!new_qc) {

        ERROR( DSPROC_LIB_NAME,
            "Could not run outlier test for variable: %s:%s\n"
            " -> memory allocation error\n",
            dataset->name, var_name);
            
        dsproc_set_status(DSPROC_ENOMEM);

        free(datap);
        return(0);
    }

    nbad = nskipped = 0;
    si = ei = 0;

    for (ti = 0; ti < nsamples; ++ti) {

        /* Skip bad and missing values */

        if (qcp[ti] & qc_bad_mask || datap[ti] == missing) {
            if (analyze) {
                nbad += 1;
                if (analyze >= 2) {
                    printf("%15.6f\t%15.6f\tbad\n",
                        times[ti], datap[ti]);
                }
            }
            continue;
        }

        /* Find start of window */

        while (times[ti] - times[si] > half_width) ++si;

        /* Find end of window */

        while ((ei < nsamples) && (times[ei] - times[ti]) <= half_width) ++ei;

        /* Compute mean */

        n = sum = 0;

        for (wi = si; wi < ei; ++wi) {
            if (!(qcp[wi] & qc_bad_mask) && (datap[wi] != missing)) {
                sum += datap[wi];
                n   += 1;
            }
        }

        if (n < min_npoints) {
            new_qc[ti] |= skipped_flag;
            if (analyze) {
                nskipped += 1;
                if (analyze >= 2) {
                    printf("%15.6f\t%15.6f\tnot enough points (%d < %d)\n",
                        times[ti], datap[ti], n, min_npoints);
                }
            }
            continue;
        }

        mean = sum/n;

        /* Check for outlier */

        delta = fabs(datap[ti] - mean);

        if (delta > bad_threshold) {
            new_qc[ti] |= bad_flag;
        }

        if (delta > ind_threshold) {
            new_qc[ti] |= ind_flag;
        }

        if (analyze) {

            if (delta < 10) {
                abi = (int)delta;
            }
            else if (delta < 100) {
                abi = 10 + (int)((delta-10) / 5);
            }
            else {
                abi = 28 + (int)((delta-100) / 10);
            }

            if (abi >= nanalyze_bins) {
                abi  = nanalyze_bins - 1;
            }

            analyze_bins[abi] += 1;

            if (analyze >= 2) {

                printf("%15.6f\t%15.6f\t%15.6f\t%15.8f",
                    times[ti], datap[ti], mean, delta);

                if      (new_qc[ti] & bad_flag) printf("\tBAD");
                else if (new_qc[ti] & ind_flag) printf("\tind");

                printf("\n");
            }
        }
    }

    for (ti = 0; ti < nsamples; ++ti) {
        qcp[ti] |= new_qc[ti];
    }

    if (analyze) {

        printf(
            "\n"
            " - skipped %d bad points\n"
            " - skipped %d points that had less than %d points in window\n"
            "\n"
            "%-15s\t%s\n",
            nbad, nskipped, min_npoints,
            "dist from mean", "# of points\n");

        for (abi = 0; abi < nanalyze_bins; ++abi) {

            if (abi < 10) {
                printf("%4d to %4d", abi, abi+1);
            }
            else if (abi < 28) {

                printf("%4d to %4d",
                    ((abi-10) * 5) + 10,
                    ((abi- 9) * 5) + 10);
            }
            else if (abi < nanalyze_bins-1) {
                printf("%4d to %4d",
                    ((abi-28) * 10) + 100,
                    ((abi-27) * 10) + 100);
            }
            else {
                printf("     >= %4d",
                    ((abi-28) * 10) + 100);
            }

            printf("\t%8d\n", analyze_bins[abi]);
        }
    }

    free(new_qc);
    free(datap);
    return(1);
}

/**
*  Flag outliers using the Mean and Mean Absolute Deviation Method (MAD).
*
*  For this outlier detection method the mean of the absolute deviations
*  from the data's mean (MAD) is calculated.  If a value is a certain number
*  of MAD away from the mean of the residuals, that value is classified as
*  an outlier. A typical default value for the threshold is 3.
*
*  The MAD is more resilient to outliers in a data set than the standard
*  deviation. In the standard deviation, the distances from the mean are
*  squared, so large deviations are weighted more heavily, and thus outliers
*  can heavily influence it. In the MAD, the deviations of a small number of
*  outliers are irrelevant.
*
*  The 'analyze' option can be used to analyze the outlier detection results
*  during development and help determine the best window width and threshold
*  to use.  Remember to set this value to 0 before creating a production
*  release.  Available options are:
*
*    - 1 = prints ranges of mean absolute deviations from the mean,
*          and the number of points that fall within each range.
*
*    - 2 = for each point prints time offset, data value, window mean,
*          deviation from the mean, mean absolute deviation, and number
*          of mean absolute deviations the value is from the mean.
*
*  If an error occurs in this function it will be appended to the log and
*  error mail messages, and the process status will be set appropriately.
*
*  @param   dataset        pointer to the dataset
*  @param   var_name       name of the variable
*  @param   window_width   width of window centered on data point (in seconds)
*  @param   min_npoints    minimum number of values within window required
*                          to perform the test (default is 2).
*  @param   skipped_flag   QC flag value to use for values that do not have
*                          enough points in window to perform the test.
*  @param   bad_threshold  MAD factor used to flag outliers as bad
*  @param   bad_flag       QC flag value to use for bad outliers
*  @param   ind_threshold  MAD factor used to flag outliers as indeterminate
*  @param   ind_flag       QC flag value to use for indeterminate outliers
*  @param   analyze        Print statistics that may be helpfull during
*                          development (see above).
*
*  @retval   1  successful
*  @retval   0  a fatal error occurred
*/
int dsproc_flag_outliers_mean_mad(
    CDSGroup     *dataset,
    const char   *var_name,
    double        window_width,
    int           min_npoints,
    unsigned int  skipped_flag,
    double        bad_threshold,
    unsigned int  bad_flag,
    double        ind_threshold,
    unsigned int  ind_flag,
    unsigned int  analyze)
{
    double        half_width = window_width / 2;
    size_t        nsamples;
    double       *times;
    double       *datap;
    double        missing;
    unsigned int *qcp;
    unsigned int  qc_bad_mask;
    unsigned int *new_qc;
    double        sum, sum2, mean, mad, dev;
    size_t        ei, si, ti, wi;
    int           n, status;

    const char *method = "Mean and Mean Absolute Deviation Method (MAD)";
    int         nanalyze_bins = 60;
    int         analyze_bins[nanalyze_bins];
    int         abi, nbad, nskipped;
    double      nmads;

    DEBUG_LV1( DSPROC_LIB_NAME,
        "%s:%s: Flagging Outliers using %s\n",
        dataset->name, var_name, method);

    if (analyze) {

        memset(analyze_bins, 0, nanalyze_bins * sizeof(int));

        printf(
            "\n"
            "Analyzing Outlier Detection using %s\n"
            "\n"
            " - dataset:       %s\n"
            " - variable:      %s\n"
            " - window width:  %g\n",
            method, dataset->name, var_name, window_width);

        if (analyze >= 2) {
            printf(
            " - bad threshold: %g\t\n"
            " - ind threshold: %g\t\n"
            "\n"
            "%15s\t%15s\t%15s\t%15s\t%15s\t%15s\n",
            bad_threshold,
            ind_threshold,
            "time_offset", "value", "mean", "dev", "mad", "dev/mad");
        }
    }

    if (min_npoints < 2) {
        min_npoints = 2;
    }

    status = _init_outlier_filter(
        dataset,    var_name,
        &nsamples, &times,
        &datap,    &missing,
        &qcp,      &qc_bad_mask);

    if (status <= 0) return(0);

    if (nsamples < (size_t)min_npoints) {

        for (ti = 0; ti < nsamples; ++ti) {
            qcp[ti] |= skipped_flag;
        }

        free(datap);
        return(1);
    }

    new_qc = calloc(nsamples, sizeof(unsigned int));
    if (!new_qc) {

        ERROR( DSPROC_LIB_NAME,
            "Could not run outlier test for variable: %s:%s\n"
            " -> memory allocation error\n",
            dataset->name, var_name);
            
        dsproc_set_status(DSPROC_ENOMEM);

        free(datap);
        return(0);
    }

    nbad = nskipped = 0;
    si = ei = 0;

    for (ti = 0; ti < nsamples; ++ti) {

        /* Skip bad and missing values */

        if (qcp[ti] & qc_bad_mask || datap[ti] == missing) {
            if (analyze) {
                nbad += 1;
                if (analyze >= 2) {
                    printf("%15.6f\t%15.6f\tbad\n",
                        times[ti], datap[ti]);
                }
            }
            continue;
        }

        /* Find start of window */

        while (times[ti] - times[si] > half_width) ++si;

        /* Find end of window */

        while ((ei < nsamples) && (times[ei] - times[ti]) <= half_width) ++ei;

        /* Compute mean */

        n = sum = sum2 = 0;

        for (wi = si; wi < ei; ++wi) {
            if (!(qcp[wi] & qc_bad_mask) && (datap[wi] != missing)) {
                sum += datap[wi];
                n   += 1;
            }
        }

        if (n < min_npoints) {
            new_qc[ti] |= skipped_flag;
            if (analyze) {
                nskipped += 1;
                if (analyze >= 2) {
                    printf("%15.6f\t%15.6f\tnot enough points (%d < %d)\n",
                        times[ti], datap[ti], n, min_npoints);
                }
            }
            continue;
        }

        mean = sum/n;

        /* Compute sum of the differences from the mean */

        for (wi = si; wi < ei; ++wi) {
            if (!(qcp[wi] & qc_bad_mask) && (datap[wi] != missing)) {
                sum2 += fabs(datap[wi] - mean);
            }
        }

        /* Compute mean absolute deviation */

        mad = sum2/n;

        /* Check for outlier */

        dev = fabs(datap[ti] - mean);

        if (dev > bad_threshold * mad) {
            new_qc[ti] |= bad_flag;
        }

        if (dev > ind_threshold * mad) {
            new_qc[ti] |= ind_flag;
        }

        if (analyze) {

            if (dev == 0) {
                abi   = 0;
                nmads = 0;
            }
            else if (mad == 0) {
                abi   = nanalyze_bins - 1;
                nmads = 1.0 / 0.0;
            }
            else {

                nmads = dev/mad;
                abi   = (int)(nmads * 2);

                if ((nmads * 2) - abi == 0) abi -= 1;

                if (abi >= nanalyze_bins) {
                    abi = nanalyze_bins - 1;
                }
            }

            analyze_bins[abi] += 1;

            if (analyze >= 2) {

                printf("%15.6f\t%15.6f\t%15.6f\t%15.8f\t%15.8f\t%15.2f",
                    times[ti], datap[ti], mean, dev, mad, nmads);

                if      (new_qc[ti] & bad_flag) printf("\tBAD");
                else if (new_qc[ti] & ind_flag) printf("\tind");

                printf("\n");
            }
        }
    }

    for (ti = 0; ti < nsamples; ++ti) {
        qcp[ti] |= new_qc[ti];
    }

    if (analyze) {

        printf(
            "\n"
            " - skipped %d bad points\n"
            " - skipped %d points that had less than %d points in window\n"
            "\n"
            "%-12s\t%s\n",
            nbad, nskipped, min_npoints,
            "# of MADs", "# of points\n");

        for (abi = 0; abi < nanalyze_bins; ++abi) {

            if (abi == nanalyze_bins-1) {
                printf("     >= %4.1f", (double)nanalyze_bins/2);
            }
            else {
                printf("%4.1f to %4.1f", (float)abi/2, (float)abi/2 + 0.5);
            }

            printf("\t%8d\n", analyze_bins[abi]);

        }
    }

    free(new_qc);
    free(datap);
    return(1);
}

/**
*  Flag outliers using the Median and Median Absolute Deviation Method (MAD).
*
*  For this outlier detection method the median of the absolute deviations
*  from the data's median (MAD) is calculated.  If a value is a certain number
*  of MAD away from the median of the residuals, that value is classified as
*  an outlier. A typical default value for the threshold is 3.
*
*  The MAD is more resilient to outliers in a data set than the standard
*  deviation. In the standard deviation, the distances from the mean are
*  squared, so large deviations are weighted more heavily, and thus outliers
*  can heavily influence it. In the MAD, the deviations of a small number of
*  outliers are irrelevant.
*
*  While this method is generally more effective than the mean and standard
*  deviation method, it can be too aggressive in classifying values that are
*  not really extremely different. Also, if more than 50% of the data points
*  have the same value, MAD is computed to be 0, so any value different from
*  the residual median is classified as an outlier.
*
*  The 'analyze' option can be used to analyze the outlier detection results
*  during development and help determine the best window width and threshold
*  to use.  Remember to set this value to 0 before creating a production
*  release.  Available options are:
*
*    - 1 = prints ranges of median absolute deviations from the median,
*          and the number of points that fall within each range.
*
*    - 2 = for each point prints time offset, data value, window median,
*          deviation from the median, median absolute deviation, and number
*          of median absolute deviations the value is from the median.
*
*  If an error occurs in this function it will be appended to the log and
*  error mail messages, and the process status will be set appropriately.
*
*  @param   dataset        pointer to the dataset
*  @param   var_name       name of the variable
*  @param   window_width   width of window centered on data point (in seconds)
*  @param   min_npoints    minimum number of values within window required
*                          to perform the test (default is 2).
*  @param   skipped_flag   QC flag value to use for values that do not have
*                          enough points in window to perform the test.
*  @param   bad_threshold  MAD factor used to flag outliers as bad
*  @param   bad_flag       QC flag value to use for bad outliers
*  @param   ind_threshold  MAD factor used to flag outliers as indeterminate
*  @param   ind_flag       QC flag value to use for indeterminate outliers
*  @param   analyze        Print statistics that may be helpfull during
*                          development (see above).
*
*  @retval   1  successful
*  @retval   0  a fatal error occurred
*/
int dsproc_flag_outliers_median_mad(
    CDSGroup     *dataset,
    const char   *var_name,
    double        window_width,
    int           min_npoints,
    unsigned int  skipped_flag,
    double        bad_threshold,
    unsigned int  bad_flag,
    double        ind_threshold,
    unsigned int  ind_flag,
    unsigned int  analyze)
{
    double        half_width = window_width / 2;
    size_t        nsamples;
    double       *times;
    double       *datap;
    double        missing;
    unsigned int *qcp;
    unsigned int  qc_bad_mask;
    unsigned int *new_qc;
    double       *buffer;
    int           bi, bn;
    double        median, mad, dev;
    size_t        ei, si, ti, wi;
    int           status;

    const char *method = "Median and Median Absolute Deviation Method (MAD)";
    int         nanalyze_bins = 60;
    int         analyze_bins[nanalyze_bins];
    int         abi, nbad, nskipped;
    double      nmads;

    DEBUG_LV1( DSPROC_LIB_NAME,
        "%s:%s: Flagging Outliers using %s\n",
        dataset->name, var_name, method);

    if (analyze) {

        memset(analyze_bins, 0, nanalyze_bins * sizeof(int));

        printf(
            "\n"
            "Analyzing Outlier Detection using %s\n"
            "\n"
            " - dataset:       %s\n"
            " - variable:      %s\n"
            " - window width:  %g\n",
            method, dataset->name, var_name, window_width);

        if (analyze >= 2) {
            printf(
            " - bad threshold: %g\t\n"
            " - ind threshold: %g\t\n"
            "\n"
            "%15s\t%15s\t%15s\t%15s\t%15s\t%15s\n",
            bad_threshold,
            ind_threshold,
            "time_offset", "value", "median", "dev", "mad", "dev/mad");
        }
    }

    if (min_npoints < 2) {
        min_npoints = 2;
    }

    status = _init_outlier_filter(
        dataset,    var_name,
        &nsamples, &times,
        &datap,    &missing,
        &qcp,      &qc_bad_mask);

    if (status <= 0) return(0);

    if (nsamples < (size_t)min_npoints) {

        for (ti = 0; ti < nsamples; ++ti) {
            qcp[ti] |= skipped_flag;
        }

        free(datap);
        return(1);
    }

    if (!(buffer = malloc(nsamples * sizeof(double))) ||
        !(new_qc = calloc(nsamples, sizeof(unsigned int)))) {

        ERROR( DSPROC_LIB_NAME,
            "Could not run outlier test for variable: %s:%s\n"
            " -> memory allocation error\n",
            dataset->name, var_name);
            
        dsproc_set_status(DSPROC_ENOMEM);
        free(datap);
        return(0);
    }

    nbad = nskipped = 0;
    si = ei = 0;

    for (ti = 0; ti < nsamples; ++ti) {

        /* Skip bad and missing values */

        if (qcp[ti] & qc_bad_mask || datap[ti] == missing) {
            if (analyze) {
                nbad += 1;
                if (analyze >= 2) {
                    printf("%15.6f\t%15.6f\tbad\n",
                        times[ti], datap[ti]);
                }
            }
            continue;
        }

        /* Find start of window */

        while (times[ti] - times[si] > half_width) ++si;

        /* Find end of window */

        while ((ei < nsamples) && (times[ei] - times[ti]) <= half_width) ++ei;

        /* Find median of window */

        bn = 0;

        for (wi = si; wi < ei; ++wi) {
            if (!(qcp[wi] & qc_bad_mask) && (datap[wi] != missing)) {
                buffer[bn++] = datap[wi];
            }
        }

        if (bn < min_npoints) {
            new_qc[ti] |= skipped_flag;
            if (analyze) {
                nskipped += 1;
                if (analyze >= 2) {
                    printf("%15.6f\t%15.6f\tnot enough points (%d < %d)\n",
                        times[ti], datap[ti], bn, min_npoints);
                }
            }
            continue;
        }

        qsort(buffer, bn, sizeof(double), _dbl_compare);

        if (bn & 0x1) { /* odd number of points */
            median = buffer[(bn-1)/2];
        }
        else {
            median = (buffer[bn/2 - 1] + buffer[bn/2])/2;
        }

        /* Find median of deltas from the median */

        for (bi = 0; bi < bn; ++bi) {
            buffer[bi] = fabs(buffer[bi] - median);
        }

        qsort(buffer, bn, sizeof(double), _dbl_compare);

        if (bn & 0x1) { /* odd number of points */
            mad = buffer[(bn-1)/2];
        }
        else {
            mad = (buffer[bn/2 - 1] + buffer[bn/2])/2;
        }

        /* Check for outlier */

        dev = fabs(datap[ti] - median);

        if (dev > bad_threshold * mad) {
            new_qc[ti] |= bad_flag;
        }

        if (dev > ind_threshold * mad) {
            new_qc[ti] |= ind_flag;
        }

        if (analyze) {

            if (dev == 0) {
                abi   = 0;
                nmads = 0;
            }
            else if (mad == 0) {
                abi   = nanalyze_bins - 1;
                nmads = 1.0 / 0.0;
            }
            else {

                nmads = dev/mad;
                abi   = (int)(nmads * 2);

                if ((nmads * 2) - abi == 0) abi -= 1;

                if (abi >= nanalyze_bins) {
                    abi = nanalyze_bins - 1;
                }
            }

            analyze_bins[abi] += 1;

            if (analyze >= 2) {

                printf("%15.6f\t%15.6f\t%15.6f\t%15.8f\t%15.8f\t%15.2f",
                    times[ti], datap[ti], median, dev, mad, nmads);

                if      (new_qc[ti] & bad_flag) printf("\tBAD");
                else if (new_qc[ti] & ind_flag) printf("\tind");

                printf("\n");
            }
        }
    }

    for (ti = 0; ti < nsamples; ++ti) {
        qcp[ti] |= new_qc[ti];
    }

    if (analyze) {

        printf(
            "\n"
            " - skipped %d bad points\n"
            " - skipped %d points that had less than %d points in window\n"
            "\n"
            "%-12s\t%s\n",
            nbad, nskipped, min_npoints,
            "# of MADs", "# of points\n");

        for (abi = 0; abi < nanalyze_bins; ++abi) {

            if (abi == nanalyze_bins-1) {
                printf("     >= %4.1f", (double)nanalyze_bins/2);
            }
            else {
                printf("%4.1f to %4.1f", (float)abi/2, (float)abi/2 + 0.5);
            }

            printf("\t%8d\n", analyze_bins[abi]);

        }
    }

    free(buffer);
    free(new_qc);
    free(datap);
    return(1);
}

/**
*  Flag outliers using the Mean and Standard Deviation Method.
*
*  For this outlier detection method, the mean and standard deviation of the
*  residuals are calculated and compared. If a value is a certain number of
*  standard deviations away from the mean, that data point is identified as
*  an outlier. The specified number of standard deviations is called the
*  threshold. A typical default value for the threshold is 3.
*
*  This method can fail to detect outliers because the outliers increase the
*  standard deviation. The more extreme the outlier, the more the standard
*  deviation is affected.
*
*  The 'analyze' option can be used to analyze the outlier detection results
*  during development and help determine the best window width and threshold
*  to use.  Remember to set this value to 0 before creating a production
*  release.  Available options are:
*
*    - 1 = print ranges of standard deviations from the mean,
*          and the number of points that fall within each range.
*
*    - 2 = for each point prints time offset, data value, window mean,
*          deviation from the mean, standard deviation, and number of standard
*          deviations the value is from the mean.
*
*  If an error occurs in this function it will be appended to the log and
*  error mail messages, and the process status will be set appropriately.
*
*  @param   dataset        pointer to the dataset
*  @param   var_name       name of the variable
*  @param   window_width   width of window centered on data point (in seconds)
*  @param   min_npoints    minimum number of values within window required
*                          to perform the test (default is 2).
*  @param   skipped_flag   QC flag value to use for values that do not have
*                          enough points in window to perform the test.
*  @param   bad_threshold  STD factor used to flag outliers as bad
*  @param   bad_flag       QC flag value to use for bad outliers
*  @param   ind_threshold  STD factor used to flag outliers as indeterminate
*  @param   ind_flag       QC flag value to use for indeterminate outliers
*  @param   analyze        Print statistics that may be helpfull during
*                          development (see above).
*
*  @retval   1  successful
*  @retval   0  a fatal error occurred
*/
int dsproc_flag_outliers_std(
    CDSGroup     *dataset,
    const char   *var_name,
    double        window_width,
    int           min_npoints,
    unsigned int  skipped_flag,
    double        bad_threshold,
    unsigned int  bad_flag,
    double        ind_threshold,
    unsigned int  ind_flag,
    unsigned int  analyze)
{
    double        half_width = window_width / 2;
    size_t        nsamples;
    double       *times;
    double       *datap;
    double        missing;
    unsigned int *qcp;
    unsigned int  qc_bad_mask;
    unsigned int *new_qc;
    double        sum, sum2, diff, mean, var, std, dev;
    size_t        ei, si, ti, wi;
    int           n, status;

    const char *method = "Mean and Standard Deviation Method";
    int         nanalyze_bins = 60;
    int         analyze_bins[nanalyze_bins];
    int         abi, nbad, nskipped;
    double      nstds;

    DEBUG_LV1( DSPROC_LIB_NAME,
        "%s:%s: Flagging Outliers using %s\n",
        dataset->name, var_name, method);

    if (analyze) {

        memset(analyze_bins, 0, nanalyze_bins * sizeof(int));

        printf(
            "\n"
            "Analyzing Outlier Detection using %s\n"
            "\n"
            " - dataset:       %s\n"
            " - variable:      %s\n"
            " - window width:  %g\n",
            method, dataset->name, var_name, window_width);

        if (analyze >= 2) {
            printf(
            " - bad threshold: %g\t\n"
            " - ind threshold: %g\t\n"
            "\n"
            "%15s\t%15s\t%15s\t%15s\t%15s\t%15s\n",
            bad_threshold,
            ind_threshold,
            "time_offset", "value", "mean", "dev", "std", "dev/std");
        }
    }

    if (min_npoints < 2) {
        min_npoints = 2;
    }

    status = _init_outlier_filter(
        dataset,    var_name,
        &nsamples, &times,
        &datap,    &missing,
        &qcp,      &qc_bad_mask);

    if (status <= 0) return(0);

    if (nsamples < (size_t)min_npoints) {

        for (ti = 0; ti < nsamples; ++ti) {
            qcp[ti] |= skipped_flag;
        }

        free(datap);
        return(1);
    }

    new_qc = calloc(nsamples, sizeof(unsigned int));
    if (!new_qc) {

        ERROR( DSPROC_LIB_NAME,
            "Could not run outlier test for variable: %s:%s\n"
            " -> memory allocation error\n",
            dataset->name, var_name);
            
        dsproc_set_status(DSPROC_ENOMEM);

        free(datap);
        return(0);
    }

    nbad = nskipped = 0;
    si = ei = 0;

    for (ti = 0; ti < nsamples; ++ti) {

        /* Skip bad and missing values */

        if (qcp[ti] & qc_bad_mask || datap[ti] == missing) {
            if (analyze) {
                nbad += 1;
                if (analyze >= 2) {
                    printf("%15.6f\t%15.6f\tbad\n",
                        times[ti], datap[ti]);
                }
            }
            continue;
        }

        /* Find start of window */

        while (times[ti] - times[si] > half_width) ++si;

        /* Find end of window */

        while ((ei < nsamples) && (times[ei] - times[ti]) <= half_width) ++ei;

        /* Using two pass method for numerical stability */

        /* Compute mean */

        n = sum = sum2 = 0;
        
        for (wi = si; wi < ei; ++wi) {
            if (!(qcp[wi] & qc_bad_mask) && (datap[wi] != missing)) {
                sum += datap[wi];
                n   += 1;
            }
        }

        if (n < min_npoints) {
            new_qc[ti] |= skipped_flag;
            if (analyze) {
                nskipped += 1;
                if (analyze >= 2) {
                    printf("%15.6f\t%15.6f\tnot enough points (%d < %d)\n",
                        times[ti], datap[ti], n, min_npoints);
                }
            }
            continue;
        }

        mean = sum/n;

        /* Compute sum of the squares of the differences from the mean */

        for (wi = si; wi < ei; ++wi) {
            if (!(qcp[wi] & qc_bad_mask) && (datap[wi] != missing)) {
                diff  = datap[wi] - mean;
                sum2 += (diff * diff);
            }
        }

        /* Compute standard deviation */

        var = sum2 / n;
        std = sqrt(var);

        /* Check for outlier */

        dev = fabs(datap[ti] - mean);

        if (dev > bad_threshold * std) {
            new_qc[ti] |= bad_flag;
        }

        if (dev > ind_threshold * std) {
            new_qc[ti] |= ind_flag;
        }

        if (analyze) {

            if (dev == 0) {
                abi   = 0;
                nstds = 0;
            }
            else if (std == 0) {
                abi   = nanalyze_bins - 1;
                nstds = 1.0 / 0.0;
            }
            else {

                nstds = dev/std;
                abi   = (int)(nstds * 2);

                if ((nstds * 2) - abi == 0) abi -= 1;

                if (abi >= nanalyze_bins) {
                    abi = nanalyze_bins - 1;
                }
            }

            analyze_bins[abi] += 1;

            if (analyze >= 2) {

                printf("%15.6f\t%15.6f\t%15.6f\t%15.8f\t%15.8f\t%15.2f",
                    times[ti], datap[ti], mean, dev, std, nstds);

                if      (new_qc[ti] & bad_flag) printf("\tBAD");
                else if (new_qc[ti] & ind_flag) printf("\tind");

                printf("\n");
            }
        }
    }

    for (ti = 0; ti < nsamples; ++ti) {
        qcp[ti] |= new_qc[ti];
    }

    if (analyze) {

        printf(
            "\n"
            " - skipped %d bad points\n"
            " - skipped %d points that had less than %d points in window\n"
            "\n"
            "%-12s\t%s\n",
            nbad, nskipped, min_npoints,
            "# of STDs", "# of points\n");

        for (abi = 0; abi < nanalyze_bins; ++abi) {

            if (abi == nanalyze_bins-1) {
                printf("     >= %4.1f", (double)nanalyze_bins/2);
            }
            else {
                printf("%4.1f to %4.1f", (float)abi/2, (float)abi/2 + 0.5);
            }

            printf("\t%8d\n", analyze_bins[abi]);

        }
    }

    free(new_qc);
    free(datap);
    return(1);
}
