/*******************************************************************************
*
*  COPYRIGHT (C) 2009 Battelle Memorial Institute.  All Rights Reserved.
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
*    $Revision: 60277 $
*    $Author: ermold $
*    $Date: 2015-02-15 00:42:57 +0000 (Sun, 15 Feb 2015) $
*    $Version: afl-libcds3-1.3-0 $
*
*******************************************************************************/

#ifndef _LIBCDS3_TEST_H
#define _LIBCDS3_TEST_H

#include "cds3.h"

/* Test data */

void    get_test_data_types(
            size_t       *ntypes,
            CDSDataType **types);

void    get_test_data(
            CDSDataType  type,
            size_t      *type_size,
            size_t      *nvals,
            void       **values,
            size_t      *nfills,
            void       **fills,
            const char **string);

CDSVar *create_time_var(
            CDSGroup    *group,
            time_t       base_time,
            size_t       nsamples,
            double       delta);

CDSVar *create_temperature_var(
            CDSGroup    *group,
            CDSDataType  type,
            size_t       nsamples,
            int          with_missing,
            int          with_fill);

int     define_missing_value_atts(
            CDSVar      *var,
            CDSDataType  type,
            double       dval,
            int          define_miss,
            int          define_fill);

/* Run test function */

int  compare_files(
        char *file1,
        char *file2);

void log_array_values(
        const char  *prefix,
        CDSDataType  type,
        int          nelems,
        void        *array);

int  check_run_test_log(
        const char *log_name,
        char       *status_text);

void close_run_test_log(void);

int  open_run_test_log(
        const char *log_name);

int  run_test(
        const char *test_name,
        const char *log_name,
        int       (*test_func)(void));

/* Test functions */

void libcds3_test_utils(void);
void libcds3_test_units(void);
void libcds3_test_defines(void);
void libcds3_test_att_values(void);
void libcds3_test_var_data(void);
void libcds3_test_time_data(void);
void libcds3_test_copy(void);
void libcds3_test_transform_params(void);

#endif
