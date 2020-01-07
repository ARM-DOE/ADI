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
*    $Revision: 55592 $
*    $Author: ermold $
*    $Date: 2014-07-19 23:21:50 +0000 (Sat, 19 Jul 2014) $
*
********************************************************************************
*
*  NOTE: DOXYGEN is used to generate documentation for this file.
*
*******************************************************************************/

/** @file dsenv.h
 *  DataSystem Environment Functions
 */

#ifndef _DSENV_H
#define _DSENV_H

/**
 *  @defgroup ARMUTILS_DSENV DataSystem Environment
 */
/*@{*/

char *dsenv_get_hostname(void);

int dsenv_getenv(const char *name, char **value);
int dsenv_setenv(const char *name, const char *format, ...);

int dsenv_get_apps_conf_root(
        const char  *proc_name,
        const char  *proc_type,
        char       **path);

int dsenv_get_apps_conf_dir(
        const char  *proc_name,
        const char  *proc_type,
        const char  *site,
        const char  *facility,
        const char  *name,
        const char  *level,
        char       **path);

int dsenv_get_collection_root(char **path);
int dsenv_get_collection_dir(
        const char  *site,
        const char  *facility,
        const char  *name,
        const char  *level,
        char       **path);

int dsenv_get_data_conf_root(char **path);
int dsenv_get_data_conf_dir(
        const char  *site,
        const char  *facility,
        const char  *name,
        const char  *level,
        char       **path);

int dsenv_get_datastream_root(char **path);
int dsenv_get_datastream_dir(
        const char  *site,
        const char  *facility,
        const char  *name,
        const char  *level,
        char       **path);

int dsenv_get_input_datastream_root(char **path);
int dsenv_get_input_datastream_dir(
        const char  *site,
        const char  *facility,
        const char  *name,
        const char  *level,
        char       **path);

int dsenv_get_output_datastream_root(char **path);
int dsenv_get_output_datastream_dir(
        const char  *site,
        const char  *facility,
        const char  *name,
        const char  *level,
        char       **path);

int dsenv_get_tmp_root(char **path);
int dsenv_get_logs_root(char **path);
int dsenv_get_quicklook_root(char **path);

/*@}*/

#endif /* _DSENV_H */
